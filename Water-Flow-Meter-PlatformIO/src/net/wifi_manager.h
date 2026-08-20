#pragma once

#include <cstddef>
#include <cstdint>

#include "net/net_settings.h"

namespace plc {

/**
 * The WiFi state machine (WiFi_MQTT_Connectivity §3.1, slice N4).
 *
 * §3.1 opens with "the radio is a state machine with exactly one owner. No other task may call
 * WiFi APIs." This class is that owner, and the design follows from taking the sentence
 * literally:
 *
 *   - the radio is reached only through `WifiRadio`, an abstract interface, so the decision logic
 *     here is Arduino-free and every transition is exercised on a host. Association failures,
 *     a 5-minute backoff ceiling and a 10-minute portal timeout are not things anybody will sit
 *     in front of a bench and reproduce twelve times;
 *   - every radio call happens inside `update()` or one of the two explicit commands. The
 *     accessors return values SAMPLED during the last `update()` and never touch the radio, so
 *     the UI task and the Modbus register publisher cannot call an SDK function behind this
 *     class's back — which is the same rule §3.1 states, enforced by the type system rather than
 *     by a comment.
 *
 * The class is NOT thread-safe and deliberately carries no locking: `update()` and all accessors
 * must be called from the one task that owns the radio (§3.1). Nothing here is atomic.
 *
 * ── Two things this class deliberately does NOT do ────────────────────────────────
 *
 * It never writes `NetSettings` — the reference is const. R5.2 requires that a change which
 * leaves the device unable to associate is NOT rolled back automatically, so the state machine
 * has no business editing configuration; it reports the failure and keeps retrying.
 *
 * It never decides on its own to enable the radio or raise the AP. §7 — "WiFi is never enabled
 * automatically and AP mode is never entered automatically" — makes both consequences of a
 * setting the operator changed, so both are driven by `settings.wifiEnabled()` and by
 * `requestApPortal()`, never by an internal timer.
 */

/**
 * The reported state, and — this matters — the WIRE ENCODING of `NET_WIFI_STATE` (register 501,
 * §5).
 *
 * Nothing else defines those numbers today: `NetRegisterMap::publish()` does not write 501,
 * because the value does not live in `NetSettings`. So these values are the contract a Modbus
 * master decodes, and renumbering them silently changes what every master on the bus reads.
 * Append, never reorder.
 *
 * `Idle` and `ApPortal` have no row in §3.1's table. They are needed anyway: §7.4's AP flow
 * postdates that table, and "enabled, unconfigured, and the portal window has closed" is a real
 * resting state that the five documented states cannot express. Both map onto documented display
 * text (see `wifiStateText`) so §3.1's ASCII vocabulary is unchanged.
 */
enum class WifiState : uint8_t {
  Disabled = 0,   /**< Not enabled, or no credentials. Radio powered down. R3.1.1. */
  Idle = 1,       /**< Enabled, radio powered down, nothing to do: unconfigured, portal closed. */
  Connecting = 2, /**< Associating and awaiting DHCP. */
  Connected = 3,  /**< Associated with an IP address. */
  Retrying = 4,   /**< Failed; waiting out the backoff (R3.1.2). */
  ApPortal = 5,   /**< Offering the provisioning access point (§7.4). */
  Failed = 6      /**< The backoff has saturated at the ceiling — likely wrong PSK or no AP. */
};

/** The §3.1 display text, ASCII only (§4.6). Never longer than five characters. */
const char* wifiStateText(WifiState state);

/**
 * What the driver says about the station link.
 *
 * Narrower than the SDK's status enum on purpose, and one distinction is load-bearing: §9 requires
 * "wrong PSK" and "AP disappears" to be *distinguishable*, because "it doesn't work" is not a
 * diagnosis. The adapter is expected to map the disconnect reason code onto `AuthFailed` versus
 * `ApNotFound`; where it cannot tell, `Down` is the honest answer and the connect timeout below
 * catches it.
 */
enum class RadioLink : uint8_t {
  Down = 0,   /**< Not associated. While `Connecting` this is ambiguous — see `kConnectTimeoutMs`. */
  Connecting, /**< Association or DHCP in progress. */
  Up,         /**< Associated with an IP address. */
  AuthFailed, /**< Rejected: the passphrase is wrong. */
  ApNotFound  /**< The SSID was not found in range. */
};

/** Why the link is not up. Reported to the UI and (see the module note) intended for §5's error register. */
enum class WifiError : uint8_t {
  None = 0,
  AuthFailed,     /**< Wrong passphrase (§9). */
  ApNotFound,     /**< SSID not in range (§9, "AP disappears"). */
  AssocTimeout,   /**< The driver never resolved either way inside `kConnectTimeoutMs`. */
  LinkLost,       /**< Was associated, then was not. A normal condition, not a fault (R3.1.3). */
  RadioFault,     /**< `begin()` or `connect()` was refused by the driver. */
  ApStartFailed   /**< `softAP()` was refused — a local fault, not a network one. */
};

/**
 * The radio, in the operations the state machine needs.
 *
 * Kept this narrow so the Arduino implementation is a thin adapter over `WiFi` and the test double
 * is a few dozen lines — the same split `PackStorage` uses in `ui/pack/ui_pack_loader.h`. Nothing
 * here knows about settings, backoff or timers.
 *
 * Two members are additions to the obvious set, and the adapter author needs to know why:
 *
 *   - `end()` exists because `disconnect()` is not enough. R3.1.1 requires an unconfigured device
 *     to "not power the radio at all"; a disconnected station is still powered, still scanning and
 *     still costing §2.1 the CPU that the measurement needs. The adapter must reach for
 *     `WiFi.mode(WIFI_OFF)` here, not `WiFi.disconnect()`;
 *   - `apIpAddress()` exists because `NET_AP_IP` (registers 708–711) has to report the portal
 *     address, which is the softAP's own address and not the station's.
 */
class WifiRadio {
 public:
  virtual ~WifiRadio() = default;

  /** Powers the radio up in station mode. False when the driver refuses. */
  virtual bool begin() = 0;

  /** Powers the radio DOWN — not merely disconnects it. See the note above. */
  virtual void end() = 0;

  /** Starts an association attempt. Returns false when the call itself was refused. */
  virtual bool connect(const char* ssid, const char* psk) = 0;

  /** Abandons the current association. Must stop any driver-side auto-reconnect. */
  virtual void disconnect() = 0;

  /** Raises the WPA2 provisioning AP (R7.5). False when the driver refuses. */
  virtual bool startAp(const char* ssid, const char* password) = 0;

  virtual void stopAp() = 0;

  virtual RadioLink status() = 0;

  /** Signal strength in dBm — negative. Only meaningful while associated. */
  virtual int16_t rssi() = 0;

  /** The station address as `(a << 24) | (b << 16) | (c << 8) | d` for `a.b.c.d`. 0 when unset. */
  virtual uint32_t ipAddress() = 0;

  /** The softAP address, in the same packing. */
  virtual uint32_t apIpAddress() = 0;

  /**
   * The factory MAC, six bytes.
   *
   * Read once, at construction, because it seeds the AP identity (R7.5a) and that identity must
   * not change across a reboot. HAZARD for the adapter: `WiFi.macAddress()` is not dependable
   * before a mode has been selected, and this is called before `begin()`. Use
   * `esp_read_mac(out, ESP_MAC_WIFI_SOFTAP)`, which is valid at any time.
   */
  virtual void macAddress(uint8_t out[6]) = 0;
};

class WifiManager {
 public:
  /** R3.1.2 — exponential backoff from 1 s to a 5 min ceiling. */
  static constexpr uint32_t kInitialBackoffMs = 1000;
  static constexpr uint32_t kMaxBackoffMs = 300000;

  /**
   * The jitter band, as a divisor of the current step: the delay is `base - [0, base/8]`.
   *
   * SUBTRACTIVE, which is the whole point. Additive jitter on a step already clamped to
   * `kMaxBackoffMs` would either push past R3.1.2's ceiling or be clamped away exactly at the
   * ceiling — where it is needed most, because that is where a whole site's meters, all rebooted
   * by the same power cut, would otherwise retry in lockstep forever.
   */
  static constexpr uint32_t kBackoffJitterDivisor = 8;

  /**
   * How long the driver may sit in `Connecting` before it counts as a failure.
   *
   * Without a deadline of our own the machine can never back off: a station facing a captive or
   * misbehaving AP reports "connecting" indefinitely, and R3.1.2's ladder would never start. 20 s
   * covers association plus DHCP with room to spare.
   */
  static constexpr uint32_t kConnectTimeoutMs = 20000;

  /** R7.6 — the AP shuts down after 10 minutes without a completed provisioning. */
  static constexpr uint32_t kApPortalMs = 600000;

  /** R7.5a — the recognisable, stable prefix. Someone scanning on a plant floor must know it. */
  static constexpr const char* kApSsidPrefix = "water_flow_meter_";

  /** Sized to the register fields: `NET_AP_SSID` and `NET_AP_PASSWORD` are 32 bytes each (§5). */
  static constexpr std::size_t kApNameBytes = 33;

  /** WPA2 needs at least 8 characters; 12 stays comfortable to read aloud (R5.3). */
  static constexpr std::size_t kApPasswordChars = 12;

  /**
   * Reads the MAC and derives the AP identity, but touches nothing else — no `begin()`, no
   * association. R3.1.1's "a device that has never been configured must not power the radio at
   * all" has to hold for a device that is merely constructed, including when WiFi is disabled and
   * `update()` is never called.
   */
  WifiManager(const NetSettings& settings, WifiRadio& radio);

  /**
   * One tick of the machine. Performs at most one transition, and is the ONLY place that polls the
   * radio.
   *
   * `nowMs` is a free-running millisecond counter and is allowed to wrap: every deadline is
   * compared as a signed difference, so the 49.7-day rollover is a non-event rather than a
   * 49.7-day hang.
   */
  void update(uint32_t nowMs);

  /**
   * The portal finished: shut the AP down and use the new credentials (R7.6, "and on success").
   *
   * This is an EXPLICIT signal, and the reason it exists rather than the machine simply noticing
   * that an SSID has appeared is a failure that would look like a firmware bug. The portal's form
   * handler applies the settings and *then* has to answer the browser — a browser that is still
   * associated with this device's own AP. Tearing the AP down the instant the SSID goes live kills
   * that response, and the operator is left unable to tell whether their credentials were taken.
   * So `update()` never leaves `ApPortal` because of a stored SSID; the portal calls this once it
   * has replied.
   *
   * A NO-OP unless an SSID is stored, which makes it safe to call from every apply path — the
   * panel, the portal and the Modbus block share one apply (R5.5), and an operator committing an
   * unrelated MQTT setting at the panel must not drop the AP out from under whoever is
   * provisioning. Calling it after a *remote* credential write (§5.2's "fully remote" flow) is
   * what lets the device associate immediately instead of waiting out the 10-minute window.
   */
  void noteProvisioningComplete(uint32_t nowMs);

  /**
   * Raises the provisioning AP for another `kApPortalMs`, abandoning any station attempt.
   *
   * Serves R5.4 — `NET_PORTAL_ENABLED` is writable over RS485, so a remote operator can re-open
   * the window after it has timed out without anybody touching the device. Returns false when
   * WiFi is disabled (§7: the radio does not switch itself on) or when the driver refuses the AP.
   */
  bool requestApPortal(uint32_t nowMs);

  // ── Read-only status: the §7.3 info pages, the footer of R3.4.1, and §5's registers ──
  // Every one of these returns a value sampled during the last update(). None touches the radio.

  WifiState state() const { return state_; }
  WifiError lastError() const { return lastError_; }

  /** Signal strength in dBm while associated, otherwise 0 — which is not a valid RSSI. */
  int16_t rssiDbm() const { return rssiDbm_; }

  /** `NET_WIFI_IP`: `(a << 24) | … | d`. Zero unless associated. */
  uint32_t ipAddress() const { return ipAddress_; }

  /** `NET_AP_IP`: the portal address. Zero unless the AP is up. */
  uint32_t apIpAddress() const { return apIpAddress_; }

  /** `NET_WIFI_MAC`. Also the mitigation for a zero MAC — see the note on `apSsid()`. */
  void macAddress(uint8_t out[6]) const;

  /**
   * The SSID the radio was last asked to join — not the stored setting.
   *
   * They differ for as long as a reconfiguration is in flight, and during exactly that window the
   * WiFi info page (R3.4.2) is the only diagnostic there is. Showing the stored value would tell
   * the operator what the device is *going* to try, when the question they have is what it is
   * failing at now.
   */
  const char* ssid() const { return ssid_; }

  /**
   * FNV-1a over ssid, a separator, then psk — the identity of a credential PAIR.
   *
   * Public and static because it is a pure function of its arguments and the test has to be able to
   * assert the property the guard depends on: that a passphrase-only change moves it, and that
   * moving the boundary between the two fields is not a collision. A private helper would have left
   * that provable only indirectly, through the state machine.
   */
  static uint64_t fingerprintOf(const char* ssid, const char* psk);

  /**
   * `water_flow_meter_<n>`, stable for the life of the device (R7.5a).
   *
   * Derived from the MAC and never randomised per boot, because R5.3 has a remote operator read
   * this over RS485 and speak it to somebody standing at the device: a per-boot name could be
   * stale before it is spoken, and two meters on one wall could swap identities across a power cut.
   *
   * DEGRADATION worth knowing: if the adapter hands back a zero MAC, every device derives the same
   * name — the identity swap R7.5a exists to prevent. That is why the MAC is also exposed above
   * and published at registers 505-507: an all-zero MAC is then visible rather than silent. (It said
   * 503-508 until 2026-08-20, which is the IP's window and one register too many — and nothing
   * published it at all until then, which is why the wrong span went unnoticed.)
   */
  const char* apSsid() const { return apSsid_; }

  /**
   * The WPA2 passphrase for that AP (R7.5), also MAC-derived and stable.
   *
   * Readable, unlike the operator's own secrets, and R5.3 is explicit about the asymmetry: this
   * describes an access point the device is broadcasting, not a secret it was given.
   *
   * Stated plainly because it is a real limit: the AP's BSSID *is* the MAC, and it is in every
   * beacon. Anyone in range who knows this derivation can therefore compute the passphrase, so it
   * is obfuscation against a drive-by, not secrecy. It buys exactly what R7.5 claims — "someone
   * who is not standing there cannot" reconfigure the device casually — and no more. Closing it
   * properly needs a per-boot random passphrase, which is precisely what R7.5a's
   * read-it-out-over-the-phone flow rules out.
   */
  const char* apPassword() const { return apPassword_; }

  /** `NET_PORTAL_REMAINING_S` (register 675) and the AP info page countdown (R7.6). 0 when down. */
  uint16_t portalRemainingS() const;

  /** True while the AP is up, so the LED override of R7.7 has something to key off. */
  bool apActive() const { return apActive_; }

  /**
   * The delay chosen for the wait now in progress, in ms. 0 when not waiting.
   *
   * Exposed so the R3.1.2 ceiling can be asserted exactly rather than inferred from a ticked
   * measurement, whose resolution is the tick.
   */
  uint32_t retryDelayMs() const;

  /** Consecutive failed attempts since the last successful association. */
  uint16_t attempts() const { return attempts_; }

  /**
   * True once per association, and cleared by reading.
   *
   * R7.13 syncs the RTC from NTP "on association", and §4.1 starts the MQTT client when the link
   * comes up. Both belong to other tasks, and both need the EDGE rather than the state — polling
   * `state() == Connected` would re-sync the clock on the sensor I²C bus every tick, which R2.1.6
   * exists to prevent.
   */
  bool consumeJustConnected();

 private:
  void enterEnabled();
  void beginConnect();
  void enterConnected();
  void enterIdle();
  void raiseAp();
  void pollConnecting();
  void pollConnected();
  void pollBackoff();
  void pollApPortal();
  void failBackoff(WifiError error);
  void shutDown();
  bool ensureRadioUp();

  /** Signed comparison, so a wrapped `nowMs` does not park a deadline 49 days into the future. */
  bool reached(uint32_t deadlineMs) const {
    return static_cast<int32_t>(nowMs_ - deadlineMs) >= 0;
  }

  uint32_t nextRandom();
  void deriveApIdentity();

  const NetSettings& settings_;
  WifiRadio& radio_;

  WifiState state_ = WifiState::Disabled;
  WifiError lastError_ = WifiError::None;

  uint32_t nowMs_ = 0;
  uint32_t connectStartedMs_ = 0;
  uint32_t retryDeadlineMs_ = 0;
  uint32_t retryDelayMs_ = 0;
  uint32_t apDeadlineMs_ = 0;

  uint16_t attempts_ = 0;
  bool radioUp_ = false;
  bool apActive_ = false;
  bool justConnected_ = false;

  int16_t rssiDbm_ = 0;
  uint32_t ipAddress_ = 0;
  uint32_t apIpAddress_ = 0;

  uint8_t mac_[6] = {};
  char ssid_[NetSettings::kMaxValueBytes + 1] = {};
  /**
   * Fingerprint of the credentials LAST HANDED TO THE RADIO — SSID and passphrase together.
   *
   * noteProvisioningComplete() needs to answer "are the stored credentials different from the ones
   * the radio is using". It used to answer it by comparing the SSID alone, which cannot see the
   * commonest correction there is: the operator retyped a mistyped passphrase on the same network.
   * That change was silently swallowed — measured at 73 s of dead time mid-backoff, and a full 600 s
   * with the provisioning AP still up, which is R7.6's "and on success" going unhonoured.
   *
   * A fingerprint rather than a copy of the passphrase, because a second plaintext copy of a secret
   * is a second place it can leak from (§8.1) and nothing here needs to read it back. 64-bit FNV-1a:
   * a collision would swallow one credential change, and at 2^-64 per change that is far below the
   * rate at which an operator mistypes the same passphrase twice.
   */
  uint64_t credentialFingerprint_ = 0;

  char apSsid_[kApNameBytes] = {};
  char apPassword_[kApNameBytes] = {};

  /** Jitter stream, seeded from the MAC so two devices diverge without needing an entropy source. */
  uint32_t random_ = 0;
};

}  // namespace plc
