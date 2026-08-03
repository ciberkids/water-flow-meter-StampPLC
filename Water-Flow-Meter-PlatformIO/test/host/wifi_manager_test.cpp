// Host tests for the WiFi state machine (WiFi_MQTT_Connectivity §3, §7.4).
//
// Everything interesting about this module is a matter of TIME: a backoff that has to grow to a
// five-minute ceiling, a provisioning window that has to close after ten minutes, and a link that
// drops at an awkward moment. None of that is reproducible on a bench without sitting in front of
// it for the best part of an hour per case, so the radio is an interface, the clock is a
// parameter, and the whole machine runs here in a few milliseconds of simulated hours.
//
// Every test below says what would break it. That is not decoration: this project has repeatedly
// shipped tests that passed because the value they asserted was already true at rest.
#include "net/net_settings.h"
#include "net/wifi_manager.h"

#include <cstdio>
#include <cstring>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-70s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

void checkStr(const char* actual, const char* expected, const char* what) {
  const bool same = std::strcmp(actual, expected) == 0;
  ++checks;
  std::printf("  %-70s %s\n", what, same ? "ok" : "FAIL");
  if (!same) {
    std::printf("      expected \"%s\"\n      actual   \"%s\"\n", expected, actual);
    ++failures;
  }
}

using plc::NetField;
using plc::NetSettings;
using plc::RadioLink;
using plc::WifiError;
using plc::WifiManager;
using plc::WifiState;

/** The simulated tick. 50 ms is finer than the smallest interval any assertion below cares about. */
constexpr uint32_t kTickMs = 50;

const uint8_t kMacA[6] = {0x24, 0x6F, 0x28, 0x1A, 0x2B, 0x3C};
const uint8_t kMacB[6] = {0x24, 0x6F, 0x28, 0x99, 0x88, 0x77};

/**
 * The radio, faked. Counts every call, because several requirements are about what the machine
 * must NOT do to the radio — retry in a tight loop (R3.1.2), leave it powered (R3.1.1), tear the
 * AP down early (R7.6) — and a count is the only way to see an absence.
 */
class FakeRadio : public plc::WifiRadio {
 public:
  explicit FakeRadio(const uint8_t mac[6]) { std::memcpy(mac_, mac, sizeof(mac_)); }

  bool begin() override {
    ++beginCount;
    if (!beginOk) return false;
    powered = true;
    return true;
  }

  void end() override {
    ++endCount;
    powered = false;
  }

  bool connect(const char* ssid, const char* psk) override {
    ++connectCount;
    std::snprintf(lastSsid, sizeof(lastSsid), "%s", ssid);
    std::snprintf(lastPsk, sizeof(lastPsk), "%s", psk);
    if (!connectOk) return false;
    // Set from a field rather than to a fixed value, so a test can make every attempt fail the
    // same way without having to reach in after each one.
    link = linkAfterConnect;
    return true;
  }

  void disconnect() override {
    ++disconnectCount;
    link = RadioLink::Down;
  }

  bool startAp(const char* ssid, const char* password) override {
    ++startApCount;
    std::snprintf(apSsidSeen, sizeof(apSsidSeen), "%s", ssid);
    std::snprintf(apPasswordSeen, sizeof(apPasswordSeen), "%s", password);
    if (!apOk) return false;
    apUp = true;
    return true;
  }

  void stopAp() override {
    ++stopApCount;
    apUp = false;
  }

  RadioLink status() override { return link; }
  int16_t rssi() override { return rssiValue; }
  uint32_t ipAddress() override { return ipValue; }
  uint32_t apIpAddress() override { return apIpValue; }
  void macAddress(uint8_t out[6]) override { std::memcpy(out, mac_, sizeof(mac_)); }

  bool beginOk = true;
  bool connectOk = true;
  bool apOk = true;
  bool powered = false;
  bool apUp = false;
  RadioLink link = RadioLink::Down;
  RadioLink linkAfterConnect = RadioLink::Connecting;
  int16_t rssiValue = -60;
  uint32_t ipValue = 0;
  uint32_t apIpValue = 0xC0A80401u;  // 192.168.4.1, the softAP default
  int beginCount = 0;
  int endCount = 0;
  int connectCount = 0;
  int disconnectCount = 0;
  int startApCount = 0;
  int stopApCount = 0;
  char lastSsid[64] = {};
  char lastPsk[80] = {};
  char apSsidSeen[64] = {};
  char apPasswordSeen[64] = {};

 private:
  uint8_t mac_[6] = {};
};

void enableWifi(NetSettings& settings, const char* ssid, const char* psk) {
  settings.stageWifiEnabled(true);
  if (ssid != nullptr) settings.stage(NetField::WifiSsid, ssid);
  if (psk != nullptr) settings.stage(NetField::WifiPsk, psk);
  settings.apply();
}

/** Ticks `totalMs` of simulated time through the machine. */
void advance(WifiManager& manager, uint32_t& nowMs, uint32_t totalMs, uint32_t stepMs) {
  for (uint32_t elapsed = 0; elapsed < totalMs; elapsed += stepMs) {
    nowMs += stepMs;
    manager.update(nowMs);
  }
}

/** Ticks until the machine issues another connect(); returns the simulated ms that took, or 0. */
uint32_t msUntilReconnect(WifiManager& manager, FakeRadio& radio, uint32_t& nowMs) {
  const int before = radio.connectCount;
  const uint32_t start = nowMs;
  for (uint32_t i = 0; i < 40000; ++i) {  // 40000 * 50 ms = 2000 s, well past the 300 s ceiling
    nowMs += kTickMs;
    manager.update(nowMs);
    if (radio.connectCount != before) return nowMs - start;
  }
  return 0;
}

// ── The happy path ────────────────────────────────────────────────────────────────

void happyPathTests() {
  std::printf("[happy path — off by default, then associate]\n");

  NetSettings settings;
  FakeRadio radio(kMacA);
  WifiManager manager(settings, radio);
  uint32_t now = 1000;

  // Paired deliberately with the transition below: "it is Disabled at rest" is true of a
  // do-nothing stub, so on its own it proves nothing. What it is here for is the call count.
  check(manager.state() == WifiState::Disabled, "a fresh manager is Disabled (R3.1.1)");
  check(radio.beginCount == 0 && !radio.powered,
        "and constructing it does NOT power the radio (R3.1.1)");

  manager.update(now);
  check(manager.state() == WifiState::Disabled && radio.beginCount == 0,
        "ticking a disabled radio still touches nothing");

  enableWifi(settings, "MyNetwork", "hunter2hunter2");
  manager.update(now += kTickMs);
  check(radio.beginCount == 1 && radio.powered, "enabling powers the radio, once");
  check(manager.state() == WifiState::Connecting, "and the machine is Connecting");
  checkStr(radio.lastSsid, "MyNetwork", "the stored SSID reached the radio");
  checkStr(radio.lastPsk, "hunter2hunter2", "and so did the passphrase");
  check(radio.startApCount == 0, "no AP: a configured device provisions nothing (§7.4)");
  checkStr(plc::wifiStateText(manager.state()), "CONN", "the display shows CONN (§3.1)");

  radio.link = RadioLink::Up;
  radio.ipValue = 0xC0A80132u;  // 192.168.1.50
  radio.rssiValue = -57;
  manager.update(now += kTickMs);
  check(manager.state() == WifiState::Connected, "the link coming up is Connected");
  check(manager.ipAddress() == 0xC0A80132u, "NET_WIFI_IP reports the address (§5)");
  check(manager.rssiDbm() == -57, "and NET_WIFI_RSSI the signal (R3.4.2)");
  checkStr(manager.ssid(), "MyNetwork", "the info page shows the SSID actually joined");
  check(manager.attempts() == 0, "with no failures on the counter");
  check(manager.lastError() == WifiError::None, "and no error");
  checkStr(plc::wifiStateText(manager.state()), "OK", "the display shows OK (§3.1)");

  check(manager.consumeJustConnected(), "the association edge fires (R7.13's NTP hook)");
  check(!manager.consumeJustConnected(),
        "exactly once — polling the state would re-sync the RTC every tick");

  // Asserts a CHANGE rather than a value: an implementation that sampled once on association and
  // then reported a frozen figure would pass every check above and fail this one.
  radio.rssiValue = -71;
  radio.ipValue = 0xC0A8017Bu;
  manager.update(now += kTickMs);
  check(manager.rssiDbm() == -71 && manager.ipAddress() == 0xC0A8017Bu,
        "status keeps being sampled while associated, not frozen at association");
  check(manager.portalRemainingS() == 0, "and no portal countdown is running");
}

// ── Backoff ──────────────────────────────────────────────────────────────────────

void backoffTests() {
  std::printf("\n[backoff — R3.1.2: 1 s to a 5 min ceiling, with jitter]\n");

  NetSettings settings;
  FakeRadio radio(kMacA);
  WifiManager manager(settings, radio);
  uint32_t now = 5000;

  radio.linkAfterConnect = RadioLink::AuthFailed;  // A wrong passphrase, over and over (§9).
  enableWifi(settings, "MyNetwork", "wrong-passphrase");
  manager.update(now);
  check(manager.state() == WifiState::Connecting, "the first attempt goes out");
  manager.update(now += kTickMs);
  check(manager.state() == WifiState::Retrying, "a refused association enters Retrying (§3.1)");
  check(manager.lastError() == WifiError::AuthFailed,
        "reported as AuthFailed — §9 needs wrong-PSK distinguishable");
  check(radio.disconnectCount >= 1,
        "and the driver is disconnected, or its own retries outpace the backoff");
  check(manager.attempts() == 1, "one failure on the counter");
  checkStr(plc::wifiStateText(manager.state()), "RETRY", "the display shows RETRY (§3.1)");

  // Twelve rungs of the ladder, each one both READ from the machine and MEASURED by ticking a
  // clock. Reading alone would not prove the radio waits; measuring alone cannot see the ceiling
  // exactly, because a ticked measurement is only accurate to one tick.
  constexpr int kRungs = 12;
  uint32_t reported[kRungs] = {};
  uint32_t measured[kRungs] = {};
  for (int i = 0; i < kRungs; ++i) {
    reported[i] = manager.retryDelayMs();
    measured[i] = msUntilReconnect(manager, radio, now);
    now += kTickMs;
    manager.update(now);  // Observe the failure, so the next rung starts where this one ended.
  }

  bool everyRungWaited = true;
  bool growsWhileBelowCeiling = true;
  bool neverExceedsCeiling = true;
  bool jitterIsPresent = false;
  for (int i = 0; i < kRungs; ++i) {
    if (measured[i] < reported[i] || measured[i] > reported[i] + kTickMs) everyRungWaited = false;
    if (reported[i] > WifiManager::kMaxBackoffMs) neverExceedsCeiling = false;
    // The unjittered step, for comparison: 1 s doubling, clamped at the ceiling.
    uint32_t base = WifiManager::kInitialBackoffMs;
    for (int d = 0; d < i && base < WifiManager::kMaxBackoffMs; ++d) base <<= 1;
    if (base > WifiManager::kMaxBackoffMs) base = WifiManager::kMaxBackoffMs;
    if (reported[i] != base) jitterIsPresent = true;
    // 1000 << 8 = 256000 is the last step below the ceiling, so rungs 0–9 must strictly grow;
    // from rung 10 on they all sit inside the jitter band of the ceiling and need not.
    if (i > 0 && i <= 9 && measured[i] <= measured[i - 1]) growsWhileBelowCeiling = false;
  }
  std::printf("      rungs (ms): ");
  for (int i = 0; i < kRungs; ++i) std::printf("%u ", static_cast<unsigned>(reported[i]));
  std::printf("\n");

  check(everyRungWaited, "the delay the machine reports is the delay it actually waits");
  check(growsWhileBelowCeiling, "the measured delay GROWS on every rung below the ceiling");
  check(neverExceedsCeiling, "and never exceeds the 5 min ceiling (R3.1.2)");
  check(reported[kRungs - 1] >= WifiManager::kMaxBackoffMs -
                                    WifiManager::kMaxBackoffMs / WifiManager::kBackoffJitterDivisor,
        "at the ceiling it sits inside the jitter band, not below it");
  check(jitterIsPresent, "at least one rung is jittered off the bare doubling (R3.1.2)");

  check(manager.state() == WifiState::Failed,
        "once the ceiling is reached the display says FAIL, not RETRY (§9)");
  checkStr(plc::wifiStateText(manager.state()), "FAIL", "which is FAIL (§3.1)");

  // §9: never a reboot loop, and never give up.
  const int before = radio.connectCount;
  msUntilReconnect(manager, radio, now);
  check(radio.connectCount == before + 1, "and it keeps trying forever — §9 forbids giving up");

  // Recovery: the operator fixes the passphrase and the ladder is forgotten.
  radio.linkAfterConnect = RadioLink::Up;
  radio.link = RadioLink::Up;
  manager.update(now += kTickMs);
  check(manager.state() == WifiState::Connected && manager.attempts() == 0,
        "a successful association clears the failure counter");
  check(manager.retryDelayMs() == 0, "and there is no pending delay to report");
}

void linkLossTests() {
  std::printf("\n[link loss — R3.1.3: a dropped link is normal, and restarts the ladder]\n");

  NetSettings settings;
  FakeRadio radio(kMacA);
  WifiManager manager(settings, radio);
  uint32_t now = 9000;

  // Deliberately climb the ladder FIRST. Dropping the link from a fresh association would leave
  // the counter at zero either way, and the test could not tell a reset from an omission.
  radio.linkAfterConnect = RadioLink::AuthFailed;
  enableWifi(settings, "MyNetwork", "wrong-passphrase");
  manager.update(now);
  for (int i = 0; i < 6; ++i) {
    manager.update(now += kTickMs);
    if (i < 5) msUntilReconnect(manager, radio, now);
  }
  check(manager.attempts() >= 6, "six consecutive failures are on the counter");
  const uint32_t climbed = manager.retryDelayMs();
  check(climbed > 16000, "so the delay has climbed past 16 s");

  radio.linkAfterConnect = RadioLink::Connecting;
  msUntilReconnect(manager, radio, now);
  radio.link = RadioLink::Up;
  manager.update(now += kTickMs);
  check(manager.state() == WifiState::Connected, "then the network comes back and it associates");

  radio.link = RadioLink::Down;
  manager.update(now += kTickMs);
  check(manager.state() == WifiState::Retrying, "losing the link is Retrying, not Failed (§9)");
  check(manager.lastError() == WifiError::LinkLost, "with LinkLost, distinct from a bad PSK (§9)");
  check(manager.attempts() == 1 && manager.retryDelayMs() <= WifiManager::kInitialBackoffMs,
        "and the ladder restarts at 1 s rather than resuming the climb");
}

// ── The provisioning AP ──────────────────────────────────────────────────────────

void apPortalTests() {
  std::printf("\n[AP portal — §7.4: raised when enabled but unconfigured]\n");

  NetSettings settings;
  FakeRadio radio(kMacA);
  WifiManager manager(settings, radio);
  uint32_t now = 2000;

  enableWifi(settings, nullptr, nullptr);  // Enabled, no credentials — §7.1's owner rule.
  check(!settings.wifiConfigured(), "nothing is stored");
  manager.update(now);
  check(manager.state() == WifiState::ApPortal, "so the AP goes up (§7.1)");
  check(radio.startApCount == 1 && radio.apUp, "exactly once");
  check(radio.connectCount == 0, "and nothing tried to associate with an empty SSID");
  checkStr(radio.apSsidSeen, manager.apSsid(), "under the SSID the registers advertise (R5.3)");
  checkStr(radio.apPasswordSeen, manager.apPassword(), "and the password they advertise");
  check(manager.apIpAddress() == 0xC0A80401u, "NET_AP_IP reports the portal address (§5)");
  check(manager.portalRemainingS() == 600, "the countdown starts at 600 s (R7.6, register 675)");
  checkStr(plc::wifiStateText(manager.state()), "AP", "the display shows AP");

  // Nine minutes. This is the check that pins R7.6's constant: a five-minute window would already
  // have closed here, and the assertion below would find the AP down.
  advance(manager, now, 540000, 1000);
  check(manager.state() == WifiState::ApPortal && radio.stopApCount == 0,
        "nine minutes in, the AP is still up");
  check(manager.portalRemainingS() == 60, "with 60 s left on the countdown");

  advance(manager, now, 59000, 1000);
  check(manager.state() == WifiState::ApPortal, "and at 9:59 it is STILL up");
  check(manager.portalRemainingS() == 1, "with one second left");

  advance(manager, now, 1000, 1000);
  check(manager.state() == WifiState::Idle, "at 10:00 the window closes (R7.6)");
  check(radio.stopApCount == 1 && !radio.apUp, "the AP is brought down");
  check(radio.endCount == 1 && !radio.powered,
        "and the radio is POWERED DOWN, not merely idle (R3.1.1, §2.1)");
  check(manager.portalRemainingS() == 0, "the countdown reads zero");
  check(manager.apIpAddress() == 0, "and NET_AP_IP no longer advertises a portal");
  checkStr(plc::wifiStateText(manager.state()), "OFF", "which reads OFF — the radio is off");

  // R5.4 — a master reopens the window over RS485, without anybody visiting the device.
  check(manager.requestApPortal(now), "a deliberate request reopens the window (R5.4)");
  check(manager.state() == WifiState::ApPortal && radio.startApCount == 2, "the AP is up again");
  check(manager.portalRemainingS() == 600, "for another ten minutes");

  // §7 — nothing switches the radio on by itself, so the request has to be refusable.
  settings.stageWifiEnabled(false);
  settings.apply();
  manager.update(now += kTickMs);
  check(manager.state() == WifiState::Disabled && radio.stopApCount == 2,
        "disabling WiFi takes the AP down with it");
  check(!manager.requestApPortal(now), "and a request is then refused (§7)");
  check(radio.startApCount == 2, "with no AP raised");
}

void apKeepsRunningWhileProvisioningTests() {
  std::printf("\n[provisioning — the AP must not vanish under the browser being answered]\n");

  NetSettings settings;
  FakeRadio radio(kMacA);
  WifiManager manager(settings, radio);
  uint32_t now = 3000;

  enableWifi(settings, nullptr, nullptr);
  manager.update(now);
  check(manager.state() == WifiState::ApPortal, "the AP is up");

  // The portal's form handler has just applied the credentials. They are LIVE now — and the
  // browser that submitted them is still associated with this device's own AP, waiting for a reply.
  settings.stage(NetField::WifiSsid, "MyNetwork");
  settings.stage(NetField::WifiPsk, "hunter2hunter2");
  settings.apply();
  check(settings.wifiConfigured(), "an SSID is stored the instant the form applies");

  advance(manager, now, 5000, kTickMs);
  check(manager.state() == WifiState::ApPortal,
        "the AP is STILL up — a stored SSID alone must not tear it down");
  check(radio.stopApCount == 0 && radio.apUp, "nothing was stopped");
  check(radio.connectCount == 0, "and no association was started behind the portal's back");
  check(manager.portalRemainingS() == 595, "the countdown simply keeps running");

  manager.noteProvisioningComplete(now);
  check(radio.stopApCount == 1 && !radio.apUp, "the explicit completion signal drops the AP (R7.6)");
  check(manager.state() == WifiState::Connecting, "and starts the association");
  check(radio.connectCount == 1, "with one attempt");
  checkStr(radio.lastSsid, "MyNetwork", "using the credentials just provisioned");
  check(manager.portalRemainingS() == 0, "and no countdown left to show");

  radio.link = RadioLink::Up;
  manager.update(now += kTickMs);
  check(manager.state() == WifiState::Connected, "which succeeds");

  // The signal is wired into EVERY apply path (R5.5 — there is only one), so it has to be inert
  // when the network did not change. Without the guard, committing an unrelated MQTT setting at
  // the panel would bounce a working link.
  const int connects = radio.connectCount;
  settings.stageMqttEnabled(true);
  settings.apply();
  manager.noteProvisioningComplete(now += kTickMs);
  check(manager.state() == WifiState::Connected && radio.connectCount == connects,
        "an apply that did not change the network does not bounce the link (R5.5)");

  // But a new SSID does move the device, and R5.2 forbids rolling that back.
  settings.stage(NetField::WifiSsid, "OtherNetwork");
  settings.apply();
  manager.noteProvisioningComplete(now += kTickMs);
  check(manager.state() == WifiState::Connecting && radio.connectCount == connects + 1,
        "a new SSID does re-associate (§5.2's fully-remote flow)");
  checkStr(radio.lastSsid, "OtherNetwork", "on the new network");
}

void credentialsDuringApTests() {
  std::printf("\n[credentials by another route — the window closes, then they get used]\n");

  NetSettings settings;
  FakeRadio radio(kMacA);
  WifiManager manager(settings, radio);
  uint32_t now = 4000;

  enableWifi(settings, nullptr, nullptr);
  manager.update(now);
  check(manager.state() == WifiState::ApPortal, "the AP is up for an unconfigured device");

  // A Modbus master writes the block and applies (§5.2). Nobody calls the completion signal.
  settings.stage(NetField::WifiSsid, "BusNetwork");
  settings.stage(NetField::WifiPsk, "hunter2hunter2");
  settings.apply();
  advance(manager, now, 600000, 1000);
  check(radio.stopApCount == 1, "at the ten-minute mark the AP comes down (R7.6)");
  check(manager.state() == WifiState::Connecting,
        "and the credentials that did arrive are used, not discarded");
  checkStr(radio.lastSsid, "BusNetwork", "on the network the master wrote");

  // And the Idle path in the other order: credentials arrive AFTER the window closed.
  NetSettings later;
  FakeRadio radioLater(kMacA);
  WifiManager managerLater(later, radioLater);
  uint32_t then = 4000;
  enableWifi(later, nullptr, nullptr);
  managerLater.update(then);
  advance(managerLater, then, 600000, 1000);
  check(managerLater.state() == WifiState::Idle && !radioLater.powered,
        "with nothing stored the machine parks with the radio off");
  later.stage(NetField::WifiSsid, "LateNetwork");
  later.stage(NetField::WifiPsk, "hunter2hunter2");
  later.apply();
  managerLater.update(then += kTickMs);
  check(managerLater.state() == WifiState::Connecting && radioLater.powered,
        "credentials appearing later power the radio back up and connect");
  check(radioLater.beginCount == 2, "which needed a second begin(), the first having been ended");
}

void disableMidConnectTests() {
  std::printf("\n[disabling mid-connect — the override that beats every state]\n");

  NetSettings settings;
  FakeRadio radio(kMacA);
  WifiManager manager(settings, radio);
  uint32_t now = 7000;

  enableWifi(settings, "MyNetwork", "hunter2hunter2");
  manager.update(now);
  radio.ipValue = 0xC0A80132u;
  check(manager.state() == WifiState::Connecting, "an attempt is in flight");

  settings.stageWifiEnabled(false);
  settings.apply();
  manager.update(now += kTickMs);
  check(manager.state() == WifiState::Disabled, "disabling wins from Connecting (R3.1.1)");
  check(radio.endCount == 1 && !radio.powered,
        "the radio is powered down, not merely disconnected (§2.1)");
  check(radio.disconnectCount >= 1, "the in-flight attempt is abandoned");
  check(manager.ipAddress() == 0 && manager.rssiDbm() == 0, "and stale status is cleared");
  check(manager.lastError() == WifiError::None, "turning it off is an instruction, not a failure");

  const int connects = radio.connectCount;
  const int begins = radio.beginCount;
  advance(manager, now, 60000, 1000);
  check(radio.connectCount == connects && radio.beginCount == begins,
        "and a minute later it is still doing nothing — no ladder runs on a disabled radio");
  check(!manager.consumeJustConnected(),
        "no stale association edge is delivered, which would start MQTT on a dead radio");
}

void radioFaultTests() {
  std::printf("\n[local faults — a refused driver must not become a tight loop (§2.1)]\n");

  NetSettings settings;
  FakeRadio radio(kMacA);
  WifiManager manager(settings, radio);
  uint32_t now = 8000;

  radio.apOk = false;
  enableWifi(settings, nullptr, nullptr);
  manager.update(now);
  check(manager.state() == WifiState::Idle, "a refused softAP parks the machine");
  check(manager.lastError() == WifiError::ApStartFailed, "with the reason recorded, not lost");
  check(!radio.powered, "and the radio powered down (R3.1.1)");
  advance(manager, now, 60000, 1000);
  check(radio.startApCount == 1, "and it is NOT retried every tick for the next minute");

  // A refused begin() is different: there is a network to reach, so the ladder is the right answer.
  NetSettings other;
  FakeRadio deadRadio(kMacA);
  deadRadio.beginOk = false;
  WifiManager deadManager(other, deadRadio);
  uint32_t then = 8000;
  enableWifi(other, "MyNetwork", "hunter2hunter2");
  deadManager.update(then);
  check(deadManager.state() == WifiState::Retrying, "a refused begin() enters the backoff");
  check(deadManager.lastError() == WifiError::RadioFault, "reported as a radio fault");
  check(deadRadio.connectCount == 0, "and nothing is asked of an uninitialised radio");
  advance(deadManager, then, 5000, kTickMs);
  check(deadRadio.connectCount == 0,
        "still nothing, five seconds later — begin() failing again must not fall through");
  check(deadRadio.beginCount >= 2, "though begin() is retried on the ladder");
}

void connectTimeoutTests() {
  std::printf("\n[the driver that never answers — without a deadline nothing ever backs off]\n");

  NetSettings settings;
  FakeRadio radio(kMacA);
  WifiManager manager(settings, radio);
  uint32_t now = 6000;

  enableWifi(settings, "MyNetwork", "hunter2hunter2");
  manager.update(now);
  radio.link = RadioLink::Connecting;  // Associating, forever.
  advance(manager, now, WifiManager::kConnectTimeoutMs - 1000, kTickMs);
  check(manager.state() == WifiState::Connecting, "just under the deadline it is still trying");
  advance(manager, now, 1050, kTickMs);
  check(manager.state() == WifiState::Retrying, "past it, the attempt counts as failed");
  check(manager.lastError() == WifiError::AssocTimeout, "as a timeout, not as a bad passphrase");
}

// ── The AP identity ──────────────────────────────────────────────────────────────

void apIdentityTests() {
  std::printf("\n[AP identity — R7.5a: derived from the MAC, not randomised per boot]\n");

  NetSettings settings;
  FakeRadio radioA1(kMacA);
  FakeRadio radioA2(kMacA);
  FakeRadio radioB(kMacB);
  WifiManager a1(settings, radioA1);
  WifiManager a2(settings, radioA2);
  WifiManager b(settings, radioB);

  std::printf("      MAC A -> %s / %s\n", a1.apSsid(), a1.apPassword());
  std::printf("      MAC B -> %s / %s\n", b.apSsid(), b.apPassword());

  // The heart of R7.5a: a name read over RS485 and spoken to somebody on site must still be the
  // name after a power cut. Seeding from millis() or a boot-time random would fail exactly here.
  checkStr(a2.apSsid(), a1.apSsid(), "the same MAC gives the same SSID across constructions");
  checkStr(a2.apPassword(), a1.apPassword(), "and the same passphrase");
  check(std::strcmp(b.apSsid(), a1.apSsid()) != 0, "a different MAC gives a different SSID");
  check(std::strcmp(b.apPassword(), a1.apPassword()) != 0, "and a different passphrase");

  check(std::strncmp(a1.apSsid(), "water_flow_meter_", 17) == 0,
        "the name carries R7.5a's recognisable prefix");
  check(std::strlen(a1.apSsid()) > 17, "with a per-device suffix after it");
  check(std::strlen(a1.apSsid()) <= 32, "and fits NET_AP_SSID's 32 bytes (§5)");

  const std::size_t passwordLength = std::strlen(a1.apPassword());
  check(passwordLength >= 8 && passwordLength <= 63, "the passphrase is legal WPA2 (R7.5)");
  bool speakable = true;
  for (std::size_t i = 0; i < passwordLength; ++i) {
    if (std::strchr("23456789ABCDEFGHJKLMNPQRSTUVWXYZ", a1.apPassword()[i]) == nullptr) {
      speakable = false;
    }
  }
  check(speakable, "and contains no glyph that is ambiguous when read aloud (R5.3)");

  uint8_t mac[6] = {};
  a1.macAddress(mac);
  check(std::memcmp(mac, kMacA, sizeof(mac)) == 0,
        "the MAC is exposed for NET_WIFI_MAC — an all-zero one is then visible");

  // Jitter must diverge per device, or a site-wide power cut produces lockstep retries (R3.1.2).
  // Deterministic here: both MACs are fixed constants.
  NetSettings sa;
  NetSettings sb;
  FakeRadio ja(kMacA);
  FakeRadio jb(kMacB);
  WifiManager ma(sa, ja);
  WifiManager mb(sb, jb);
  ja.linkAfterConnect = RadioLink::AuthFailed;
  jb.linkAfterConnect = RadioLink::AuthFailed;
  enableWifi(sa, "Net", "wrong");
  enableWifi(sb, "Net", "wrong");
  uint32_t nowA = 100;
  uint32_t nowB = 100;
  uint32_t seqA[3] = {};
  uint32_t seqB[3] = {};
  ma.update(nowA);
  mb.update(nowB);
  for (int i = 0; i < 3; ++i) {
    ma.update(nowA += kTickMs);
    mb.update(nowB += kTickMs);
    seqA[i] = ma.retryDelayMs();
    seqB[i] = mb.retryDelayMs();
    if (i < 2) {
      msUntilReconnect(ma, ja, nowA);
      msUntilReconnect(mb, jb, nowB);
    }
  }
  check(seqA[0] != seqB[0] || seqA[1] != seqB[1] || seqA[2] != seqB[2],
        "two devices jitter differently, so a fleet does not retry in lockstep");
}

// ── Timers and the wire encoding ─────────────────────────────────────────────────

void rolloverTests() {
  std::printf("\n[millis rollover — 49.7 days in, a deadline must not park for another 49.7]\n");

  NetSettings settings;
  FakeRadio radio(kMacA);
  WifiManager manager(settings, radio);
  uint32_t now = 0xFFFF0000u;  // ~65 s before the counter wraps

  enableWifi(settings, nullptr, nullptr);
  manager.update(now);
  check(manager.state() == WifiState::ApPortal, "the AP goes up just before the wrap");
  advance(manager, now, 599000, 1000);
  check(now < 0xFFFF0000u, "the clock has wrapped past zero");
  check(manager.state() == WifiState::ApPortal, "and at 9:59 the AP is still up");
  check(manager.portalRemainingS() == 1, "with the countdown still correct across the wrap");
  advance(manager, now, 1000, 1000);
  check(manager.state() == WifiState::Idle, "and it closes on time, not 49 days late");
}

void wireEncodingTests() {
  std::printf("\n[register 501 — this enum IS the wire encoding, so it is pinned here]\n");

  // Nothing else defines these numbers: NetRegisterMap::publish() does not write 501, because the
  // value does not live in NetSettings. Reordering the enum would silently change what every
  // Modbus master on the bus decodes, and this is the only thing that would notice.
  check(static_cast<uint16_t>(WifiState::Disabled) == 0, "Disabled is 0");
  check(static_cast<uint16_t>(WifiState::Idle) == 1, "Idle is 1");
  check(static_cast<uint16_t>(WifiState::Connecting) == 2, "Connecting is 2");
  check(static_cast<uint16_t>(WifiState::Connected) == 3, "Connected is 3");
  check(static_cast<uint16_t>(WifiState::Retrying) == 4, "Retrying is 4");
  check(static_cast<uint16_t>(WifiState::ApPortal) == 5, "ApPortal is 5");
  check(static_cast<uint16_t>(WifiState::Failed) == 6, "Failed is 6");

  // §3.1's table, and §4.6's ASCII-only rule. The footer of R3.4.1 has four columns to spend.
  checkStr(plc::wifiStateText(WifiState::Disabled), "OFF", "OFF");
  checkStr(plc::wifiStateText(WifiState::Connecting), "CONN", "CONN");
  checkStr(plc::wifiStateText(WifiState::Connected), "OK", "OK");
  checkStr(plc::wifiStateText(WifiState::Retrying), "RETRY", "RETRY");
  checkStr(plc::wifiStateText(WifiState::Failed), "FAIL", "FAIL");

  bool asciiAndShort = true;
  for (uint8_t value = 0; value <= 6; ++value) {
    const char* text = plc::wifiStateText(static_cast<WifiState>(value));
    if (std::strlen(text) == 0 || std::strlen(text) > 5) asciiAndShort = false;
    for (const char* p = text; *p != '\0'; ++p) {
      if (*p < 0x20 || *p > 0x7E) asciiAndShort = false;
    }
  }
  check(asciiAndShort, "every state's text is 1-5 printable ASCII characters (§4.6)");
}

}  // namespace

int main() {
  std::printf("plc::WifiManager — the WiFi state machine (§3, §7.4)\n\n");
  happyPathTests();
  backoffTests();
  linkLossTests();
  apPortalTests();
  apKeepsRunningWhileProvisioningTests();
  credentialsDuringApTests();
  disableMidConnectTests();
  radioFaultTests();
  connectTimeoutTests();
  apIdentityTests();
  rolloverTests();
  wireEncodingTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
