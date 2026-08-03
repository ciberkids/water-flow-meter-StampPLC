#include "net/wifi_manager.h"

#include <cstdio>
#include <cstring>

namespace plc {
namespace {

/** FNV-1a. Chosen for being short, deterministic and dependency-free, not for cryptography. */
uint32_t fnv1a(const uint8_t* data, std::size_t count, uint32_t basis) {
  uint32_t hash = basis;
  for (std::size_t i = 0; i < count; ++i) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
}

uint32_t xorshift(uint32_t& state) {
  uint32_t x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  state = x;
  return x;
}

/**
 * The AP passphrase alphabet: 32 characters with every ambiguous glyph removed.
 *
 * No 0/O and no 1/I/L, because R5.3's flow is somebody reading this off a screen — or out of a
 * register dump, over the phone — to somebody else who has to type it. A passphrase that is
 * unambiguous to *speak* is worth more here than the 1.6 extra bits per character that keeping
 * the full alphanumeric set would buy.
 */
constexpr char kApPasswordAlphabet[] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
constexpr uint32_t kApPasswordAlphabetSize = 32;  // sizeof - 1, kept explicit for the modulo
static_assert(sizeof(kApPasswordAlphabet) - 1 == kApPasswordAlphabetSize,
              "editing the alphabet without the size would index past it or ignore its tail");

}  // namespace

const char* wifiStateText(WifiState state) {
  switch (state) {
    // Idle shares OFF with Disabled, and truthfully: in both the radio is powered down. The only
    // difference is which of "the operator turned it off" and "there is nothing to connect to"
    // put it there, and that distinction belongs on the WiFi info page, not in four characters.
    case WifiState::Disabled:   return "OFF";
    case WifiState::Idle:       return "OFF";
    case WifiState::Connecting: return "CONN";
    case WifiState::Connected:  return "OK";
    case WifiState::Retrying:   return "RETRY";
    case WifiState::ApPortal:   return "AP";
    case WifiState::Failed:     return "FAIL";
  }
  return "?";  // Unreachable for a valid enumerator; reached only by a cast from the wire.
}

WifiManager::WifiManager(const NetSettings& settings, WifiRadio& radio)
    : settings_(settings), radio_(radio) {
  radio_.macAddress(mac_);
  deriveApIdentity();
}

void WifiManager::deriveApIdentity() {
  const uint32_t nameHash = fnv1a(mac_, sizeof(mac_), 2166136261u);

  // Six zero-padded decimal digits: fixed width, so a value read out of a register dump and
  // spoken aloud has no ambiguity about length, and 23 characters total leaves the 32-byte SSID
  // field (§5) comfortable. Two devices collide with probability 1e-6, which is the same order as
  // the truncation any short derivation costs.
  std::snprintf(apSsid_, sizeof(apSsid_), "%s%06u", kApSsidPrefix,
                static_cast<unsigned>(nameHash % 1000000u));

  // A different basis, so the passphrase is not a visible transform of the SSID digits that every
  // phone in radio range can already see. This does not make it secret — see the header — it only
  // stops the trivial inference.
  uint32_t stream = fnv1a(mac_, sizeof(mac_), 0x811C9DC5u ^ 0x5A5A5A5Au);
  if (stream == 0) {
    stream = 1;  // xorshift is absorbing at zero; a MAC hashing to 0 would give a constant.
  }
  for (std::size_t i = 0; i < kApPasswordChars; ++i) {
    apPassword_[i] = kApPasswordAlphabet[xorshift(stream) % kApPasswordAlphabetSize];
  }
  apPassword_[kApPasswordChars] = '\0';

  // The jitter stream starts from the same MAC, which is what makes a whole site's meters diverge
  // after a common power cut without any of them needing an entropy source (R3.1.2).
  random_ = nameHash ^ 0xA5A5A5A5u;
  if (random_ == 0) {
    random_ = 1;
  }
}

uint32_t WifiManager::nextRandom() { return xorshift(random_); }

void WifiManager::macAddress(uint8_t out[6]) const { std::memcpy(out, mac_, sizeof(mac_)); }

uint16_t WifiManager::portalRemainingS() const {
  if (!apActive_) {
    return 0;
  }
  const int32_t left = static_cast<int32_t>(apDeadlineMs_ - nowMs_);
  if (left <= 0) {
    return 0;
  }
  // Rounded UP, so the countdown reads 600 the moment the AP comes up rather than 599.
  return static_cast<uint16_t>((static_cast<uint32_t>(left) + 999u) / 1000u);
}

uint32_t WifiManager::retryDelayMs() const {
  if (state_ == WifiState::Retrying || state_ == WifiState::Failed) {
    return retryDelayMs_;
  }
  return 0;
}

bool WifiManager::consumeJustConnected() {
  const bool value = justConnected_;
  justConnected_ = false;
  return value;
}

void WifiManager::update(uint32_t nowMs) {
  nowMs_ = nowMs;

  // The one condition that overrides every state: the operator turned the radio off (R3.1.1).
  if (!settings_.wifiEnabled()) {
    if (state_ != WifiState::Disabled) {
      shutDown();
    }
    return;
  }

  switch (state_) {
    case WifiState::Disabled:
      enterEnabled();
      break;
    case WifiState::Idle:
      // Credentials arrived from some other surface after the portal window closed — §5.2's
      // remote flow, or a card. This is the only automatic transition out of Idle: the AP is
      // never re-raised on a timer (§7).
      if (settings_.wifiConfigured()) {
        if (ensureRadioUp()) {
          beginConnect();
        } else {
          failBackoff(WifiError::RadioFault);
        }
      }
      break;
    case WifiState::Connecting:
      pollConnecting();
      break;
    case WifiState::Connected:
      pollConnected();
      break;
    case WifiState::Retrying:
    case WifiState::Failed:
      pollBackoff();
      break;
    case WifiState::ApPortal:
      pollApPortal();
      break;
  }
}

bool WifiManager::ensureRadioUp() {
  if (radioUp_) {
    return true;
  }
  if (!radio_.begin()) {
    return false;
  }
  radioUp_ = true;
  return true;
}

void WifiManager::enterEnabled() {
  if (!ensureRadioUp()) {
    failBackoff(WifiError::RadioFault);
    return;
  }
  // §7.1's owner rule, and the whole of §7.4: enabled with credentials associates, enabled
  // without them offers the provisioning AP. Both are consequences of the setting the operator
  // just changed, which is why neither happens on any other trigger.
  if (settings_.wifiConfigured()) {
    beginConnect();
  } else {
    raiseAp();
  }
}

uint64_t WifiManager::fingerprintOf(const char* ssid, const char* psk) {
  // The separator matters: without it ("ab","c") and ("a","bc") would fingerprint identically, so a
  // credential change that only moved the boundary would be invisible.
  uint64_t hash = 1469598103934665603ull;  // FNV-1a 64 offset basis
  const auto mix = [&hash](const char* text) {
    for (const char* p = text; p && *p != '\0'; ++p) {
      hash ^= static_cast<uint8_t>(*p);
      hash *= 1099511628211ull;
    }
    hash ^= 0xFFu;  // separator
    hash *= 1099511628211ull;
  };
  mix(ssid);
  mix(psk);
  return hash;
}

void WifiManager::beginConnect() {
  char ssid[NetSettings::kMaxValueBytes + 1] = {};
  char psk[NetSettings::kMaxValueBytes + 1] = {};
  settings_.get(NetField::WifiSsid, ssid, sizeof(ssid));
  settings_.get(NetField::WifiPsk, psk, sizeof(psk));
  std::memcpy(ssid_, ssid, sizeof(ssid_));
  // Recorded at the moment the credentials reach the radio, so the comparison in
  // noteProvisioningComplete() is against what is actually in use rather than against a name.
  credentialFingerprint_ = fingerprintOf(ssid, psk);

  const bool accepted = radio_.connect(ssid, psk);

  // Best effort only — a compiler is entitled to elide a memset on a dying local, and this one
  // makes no claim to be a secure wipe. It costs nothing and shortens the window in which the
  // operator's passphrase sits on the stack of a task that also runs the web portal (§8).
  std::memset(psk, 0, sizeof(psk));

  if (!accepted) {
    failBackoff(WifiError::RadioFault);
    return;
  }
  state_ = WifiState::Connecting;
  connectStartedMs_ = nowMs_;
}

void WifiManager::enterConnected() {
  state_ = WifiState::Connected;
  attempts_ = 0;
  retryDelayMs_ = 0;
  lastError_ = WifiError::None;
  ipAddress_ = radio_.ipAddress();
  rssiDbm_ = radio_.rssi();
  justConnected_ = true;
}

void WifiManager::enterIdle() {
  if (apActive_) {
    radio_.stopAp();
    apActive_ = false;
  }
  if (radioUp_) {
    // R3.1.1 wants the radio POWERED DOWN, not merely idle: a station left on scans and
    // reassociates, and §2.1's budget is the measurement. Nothing here has anything to connect
    // to, so there is no reason to keep it up.
    radio_.disconnect();
    radio_.end();
    radioUp_ = false;
  }
  apIpAddress_ = 0;
  ipAddress_ = 0;
  rssiDbm_ = 0;
  attempts_ = 0;
  retryDelayMs_ = 0;
  ssid_[0] = '\0';
  state_ = WifiState::Idle;
  // lastError_ is deliberately KEPT. Reaching Idle after a refused softAP is the one case where
  // the state alone tells the operator nothing, and it is exactly when they need a reason.
}

void WifiManager::raiseAp() {
  if (!radio_.startAp(apSsid_, apPassword_)) {
    // A refused softAP is a local fault — no memory, or a driver that never came up — and
    // retrying it every tick fixes nothing while costing the polling loop (§2.1). So the machine
    // parks, with the reason recorded, and waits for a deliberate retry: the operator re-entering
    // the menu, or a master writing NET_PORTAL_ENABLED (R5.4).
    lastError_ = WifiError::ApStartFailed;
    enterIdle();
    return;
  }
  apActive_ = true;
  apIpAddress_ = radio_.apIpAddress();
  apDeadlineMs_ = nowMs_ + kApPortalMs;
  attempts_ = 0;
  retryDelayMs_ = 0;
  state_ = WifiState::ApPortal;
}

void WifiManager::pollConnecting() {
  switch (radio_.status()) {
    case RadioLink::Up:
      enterConnected();
      return;
    case RadioLink::AuthFailed:
      failBackoff(WifiError::AuthFailed);
      return;
    case RadioLink::ApNotFound:
      failBackoff(WifiError::ApNotFound);
      return;
    case RadioLink::Down:
    case RadioLink::Connecting:
      // Both mean "no answer yet". `Down` is ambiguous while an attempt is in flight — the driver
      // reports it between association and DHCP — so it is not treated as a failure. The deadline
      // below is what makes that safe.
      break;
  }
  if (reached(connectStartedMs_ + kConnectTimeoutMs)) {
    failBackoff(WifiError::AssocTimeout);
  }
}

void WifiManager::pollConnected() {
  if (!settings_.wifiConfigured()) {
    // The credentials were erased under us — a factory reset (A13), or a master clearing the SSID.
    // Staying associated to a network the device no longer has any record of would leave the radio
    // up with nothing able to explain why.
    enterIdle();
    return;
  }
  if (radio_.status() != RadioLink::Up) {
    // R3.1.3 — the link going down is a normal condition, not a fault. The ladder restarts from
    // 1 s here without any reset of its own, because enterConnected() already cleared the counter:
    // this network demonstrably worked a moment ago, and carrying a saturated backoff over from an
    // earlier bad passphrase would leave a five-minute hole after a one-second glitch.
    failBackoff(WifiError::LinkLost);
    return;
  }
  // Sampled here, once per tick, by the task that owns the radio. Every accessor then serves the
  // UI and the register publisher without either of them calling into the driver (§3.1).
  ipAddress_ = radio_.ipAddress();
  rssiDbm_ = radio_.rssi();
}

void WifiManager::pollBackoff() {
  if (!reached(retryDeadlineMs_)) {
    return;
  }
  if (!settings_.wifiConfigured()) {
    enterIdle();
    return;
  }
  if (!ensureRadioUp()) {
    failBackoff(WifiError::RadioFault);
    return;
  }
  beginConnect();
}

void WifiManager::pollApPortal() {
  if (!reached(apDeadlineMs_)) {
    return;
  }
  // R7.6 — ten minutes without a completed provisioning and the window closes. Note what does NOT
  // appear in this function: any test of wifiConfigured() other than at expiry. An SSID going live
  // mid-provisioning must not tear the AP down; see noteProvisioningComplete().
  radio_.stopAp();
  apActive_ = false;
  apIpAddress_ = 0;
  if (!settings_.wifiConfigured()) {
    enterIdle();
    return;
  }
  // Credentials did arrive — over RS485, say — but nobody signalled completion. Use them rather
  // than throwing away a provisioning that plainly happened.
  if (!ensureRadioUp()) {
    failBackoff(WifiError::RadioFault);
    return;
  }
  beginConnect();
}

void WifiManager::failBackoff(WifiError error) {
  lastError_ = error;
  if (attempts_ < 0xFFFFu) {
    ++attempts_;
  }
  if (radioUp_) {
    // Without this the driver's own auto-reconnect keeps hammering the AP behind our back and
    // R3.1.2's backoff becomes a number this class prints rather than a rate the radio observes.
    radio_.disconnect();
  }
  ipAddress_ = 0;
  rssiDbm_ = 0;

  // R3.1.2 — double from 1 s, and stop doubling at the 5 min ceiling.
  uint32_t base = kInitialBackoffMs;
  for (uint16_t i = 1; i < attempts_ && base < kMaxBackoffMs; ++i) {
    base <<= 1;
  }
  const bool saturated = base >= kMaxBackoffMs;
  if (saturated) {
    base = kMaxBackoffMs;
  }
  const uint32_t jitter = nextRandom() % (base / kBackoffJitterDivisor + 1u);
  retryDelayMs_ = base - jitter;
  retryDeadlineMs_ = nowMs_ + retryDelayMs_;

  // §9 couples the two: "backoff to the 5 min ceiling; FAIL on the display". So FAIL is not a
  // count of attempts, it is the ceiling being reached — which is a genuinely different
  // operational condition rather than a verdict the device would retract sixteen seconds later.
  // Retrying forever is deliberate: §9 also says never a reboot loop, and never giving up.
  state_ = saturated ? WifiState::Failed : WifiState::Retrying;
}

void WifiManager::shutDown() {
  if (apActive_) {
    radio_.stopAp();
    apActive_ = false;
  }
  if (radioUp_) {
    radio_.disconnect();
    radio_.end();
    radioUp_ = false;
  }
  state_ = WifiState::Disabled;
  lastError_ = WifiError::None;  // Turning the radio off is an instruction, not a failure.
  attempts_ = 0;
  retryDelayMs_ = 0;
  ipAddress_ = 0;
  apIpAddress_ = 0;
  rssiDbm_ = 0;
  ssid_[0] = '\0';
  // A link-up nobody consumed is stale now: delivering it would start the MQTT client and an NTP
  // query (R7.13) against a radio that is powered down.
  justConnected_ = false;
}

void WifiManager::noteProvisioningComplete(uint32_t nowMs) {
  nowMs_ = nowMs;
  if (!settings_.wifiEnabled() || !settings_.wifiConfigured()) {
    return;
  }

  // The idempotence that makes this safe to call from EVERY apply path (R5.5 — there is one).
  // An operator committing an unrelated MQTT setting at the panel goes through the same apply as
  // the portal's form, and it must not bounce a working link. So the test is not "did an apply
  // happen" but "are the stored CREDENTIALS different from the ones the radio was given".
  //
  // Credentials, not the SSID. Comparing the name alone swallowed a passphrase-only correction —
  // the single most likely thing an operator does after a failed association — leaving the device
  // to run out its backoff ladder, or sit with the AP up for the full ten minutes, holding a
  // passphrase it had already been told was right. See credentialFingerprint_.
  char storedSsid[NetSettings::kMaxValueBytes + 1] = {};
  char storedPsk[NetSettings::kMaxValueBytes + 1] = {};
  settings_.get(NetField::WifiSsid, storedSsid, sizeof(storedSsid));
  settings_.get(NetField::WifiPsk, storedPsk, sizeof(storedPsk));
  if (fingerprintOf(storedSsid, storedPsk) == credentialFingerprint_) {
    return;
  }

  if (apActive_) {
    radio_.stopAp();
    apActive_ = false;
    apIpAddress_ = 0;
  }
  if (!ensureRadioUp()) {
    failBackoff(WifiError::RadioFault);
    return;
  }
  attempts_ = 0;  // A new network. The old ladder was about the old credentials.
  beginConnect();
}

bool WifiManager::requestApPortal(uint32_t nowMs) {
  nowMs_ = nowMs;
  if (!settings_.wifiEnabled()) {
    return false;  // §7 — the radio does not switch itself on, and neither does the AP.
  }
  if (!ensureRadioUp()) {
    failBackoff(WifiError::RadioFault);
    return false;
  }
  if (state_ == WifiState::Connecting || state_ == WifiState::Connected) {
    radio_.disconnect();
    ipAddress_ = 0;
    rssiDbm_ = 0;
    ssid_[0] = '\0';
  }
  raiseAp();
  return apActive_;
}

}  // namespace plc
