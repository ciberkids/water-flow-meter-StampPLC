#pragma once

#include <cstddef>
#include <cstdint>

namespace plc {

/**
 * The WiFi, MQTT and portal configuration — the storage half of the text settings.
 *
 * `SettingKind::Text` gave the catalogue a way to *describe* a string; this is where the strings
 * actually live. Deliberately Arduino-free and allocation-free: the staged/apply protocol and the
 * two-characters-per-register packing are exactly the parts worth testing exhaustively on a host,
 * and they are also the parts a Modbus master can drive into any state it likes.
 *
 * ── The staged/apply protocol (Project_document §4.1.1, WiFi §5.5) ───────────────────
 *
 * Every field is written to a PENDING copy. `apply()` promotes pending to live and bumps a
 * revision. Nothing observes a half-written configuration, which matters more here than anywhere
 * else in the project: an SSID occupies 16 holding registers, so a master writing it one register
 * at a time would otherwise trigger sixteen reconnection attempts against sixteen different
 * nonsense network names.
 *
 * The same protocol serves all three input surfaces — the display editors, a Modbus master, and the
 * web form — because §3.2 requires one implementation rather than three that drift.
 */

/** Field identifiers, shared by the register map, the catalogue and the web form. */
enum class NetField : uint8_t {
  WifiSsid = 0,
  WifiPsk,
  MqttHost,
  MqttUser,
  MqttPassword,
  MqttBaseTopic,
  MqttDiscoveryPrefix,
  PortalUser,
  PortalPassword,
  Count
};

/** Longest value each field accepts, excluding the terminator. */
constexpr std::size_t netFieldCapacity(NetField field) {
  switch (field) {
    case NetField::WifiSsid:            return 32;   // 802.11 caps an SSID at 32 bytes
    case NetField::WifiPsk:             return 63;   // WPA2 passphrase maximum
    case NetField::MqttHost:            return 64;
    case NetField::MqttUser:            return 32;
    case NetField::MqttPassword:        return 32;
    case NetField::MqttBaseTopic:       return 48;
    case NetField::MqttDiscoveryPrefix: return 32;
    case NetField::PortalUser:          return 16;
    case NetField::PortalPassword:      return 32;
    case NetField::Count:               return 0;
  }
  return 0;
}

/** True for a field the device must never hand back — see §5.1 and §8.1. */
constexpr bool netFieldIsSecret(NetField field) {
  switch (field) {
    case NetField::WifiPsk:
    case NetField::MqttPassword:
    case NetField::PortalPassword:
      return true;
    case NetField::WifiSsid:
    case NetField::MqttHost:
    case NetField::MqttUser:
    case NetField::MqttBaseTopic:
    case NetField::MqttDiscoveryPrefix:
    case NetField::PortalUser:
    case NetField::Count:
      return false;
  }
  return false;
}

class NetSettings {
 public:
  /** Longest capacity across all fields, so one buffer size serves every accessor. */
  static constexpr std::size_t kMaxValueBytes = 64;

  /** §7.9a — what the device ships with, and what it must nag about until changed. */
  static constexpr const char* kDefaultPortalUser = "admin";
  static constexpr const char* kDefaultPortalPassword = "admin";

  NetSettings();

  // ── Live values ────────────────────────────────────────────────────────────────
  /** Copies the live value out. Secrets are returned in full — masking is the UI's job. */
  bool get(NetField field, char* out, std::size_t size) const;
  bool isEmpty(NetField field) const;

  /** True while the portal password is still the shipped default (§7.9a). */
  bool portalPasswordIsDefault() const;

  // ── Numeric and boolean values, which need no staging of their own but share apply ──
  bool wifiEnabled() const { return live_.wifiEnabled; }
  bool mqttEnabled() const { return live_.mqttEnabled; }
  uint16_t mqttPort() const { return live_.mqttPort; }
  uint16_t mqttPublishPeriodS() const { return live_.mqttPublishPeriodS; }
  bool mqttHaDiscovery() const { return live_.mqttHaDiscovery; }
  uint8_t mqttQos() const { return live_.mqttQos; }

  // ── The one base-topic rule ───────────────────────────────────────────────────
  /**
   * The ONE definition of a base topic this device may publish under (§4.2).
   *
   * It lives here because the field lives here, and it exists because two modules had a rule each
   * and the rules disagreed. `HaDiscovery::configure` refused a leading '/', a trailing '/' and a
   * space, but allowed every byte above 0x7e; `MqttPublisher::configure` silently STRIPPED trailing
   * slashes, ignored a leading one, allowed 0x20, and refused the bytes discovery allowed. A topic
   * one accepts and the other refuses produces ZERO Home Assistant entities while the MQTT state
   * register still reads connected — nothing is logged at either end, and it presents to the
   * operator as a broker fault. So this is the strict INTERSECTION of the two rules, and both
   * modules must defer to it instead of keeping their own.
   *
   * Refused:
   *  - null, and empty. Empty is not a topic. NetSettings treats an empty FIELD as "not
   *    configured" and lets §4.2's `watermeter/<mac-suffix>` default apply, which is a different
   *    statement and a different code path — see stagedBaseTopicCommittable().
   *  - `+` and `#`: subscription wildcards, illegal in a topic a client PUBLISHES to
   *    (MQTT 3.1.1 §4.7.1).
   *  - a leading '/', a trailing '/', and any `//`. Each leaves an empty topic level — legal MQTT
   *    that publishes without complaint and matches nothing the operator configured in HA. Refused
   *    rather than normalised on purpose: one module normalising what the other refused is exactly
   *    how the divergence above arose.
   *  - anything outside 0x21-0x7e. §4.6 binds display-bound text to printable 7-bit ASCII and the
   *    base topic is display-bound (screen `net-mqtt-base-topic`), so a byte that survived here
   *    would be unreadable on the one surface that could tell the operator what went wrong. Space
   *    goes with the other non-graphic bytes: MQTT permits it, nothing else in the toolchain does.
   *  - longer than the field can hold. NOT truncated — see stage() for why this field is the
   *    exception to that rule.
   *
   * A NUL cannot appear: the argument is a C string, and the register path's byte buffer is read as
   * one, so a NUL simply ends the topic rather than sitting inside it.
   *
   * Defined inline so a consumer gains an `#include` rather than a link-time dependency on
   * net_settings.cpp. A shared validator that costs a build-system change is a validator somebody
   * reimplements locally, which is how this ended up with two of them.
   */
  static bool isValidBaseTopic(const char* topic) {
    if (topic == nullptr || topic[0] == '\0') {
      return false;
    }
    std::size_t length = 0;
    while (topic[length] != '\0') {
      const unsigned char c = static_cast<unsigned char>(topic[length]);
      if (c < 0x21 || c > 0x7e) {
        return false;  // control bytes, space, DEL, and everything non-ASCII
      }
      if (c == '+' || c == '#') {
        return false;
      }
      if (c == '/' && length > 0 && topic[length - 1] == '/') {
        return false;  // an empty level, which subscribes to nothing the operator wrote
      }
      ++length;
      if (length > netFieldCapacity(NetField::MqttBaseTopic)) {
        return false;  // bail here rather than scanning a caller's whole buffer
      }
    }
    return topic[0] != '/' && topic[length - 1] != '/';
  }

  /**
   * True when the STAGED base topic may be committed — deliberately NOT the same as valid.
   *
   * Empty is committable and is not valid. An empty field means "not configured", and §4.2's
   * default is `watermeter/<mac-suffix>`, which NetSettings cannot spell because it has no MAC. Gate
   * apply() on isValidBaseTopic() alone and a factory-fresh device could never apply anything at
   * all — not its SSID, not its broker.
   *
   * Public so a UI can ask before committing rather than discovering it from a false return.
   */
  bool stagedBaseTopicCommittable() const;

  // ── Staging ───────────────────────────────────────────────────────────────────
  /**
   * Stages a text value. Truncates at the field's capacity rather than rejecting, so a master
   * writing a full register block does not fail on trailing padding.
   *
   * MqttBaseTopic is the one exception: it is validated and REFUSED. See the body.
   */
  bool stage(NetField field, const char* value);

  /**
   * Stages ONE BYTE of a field.
   *
   * The register map needs this because a holding-register write touches two characters of a field
   * and a master may write them in any order. Patching a C string in place cannot support that: the
   * first version padded the gap with spaces so that writing a high register before a low one would
   * not truncate, and the padding then corrupted the ordinary in-order case, leaving
   * "hunter2hunter2          " where "hunter2hunter2" was written.
   *
   * A field is a BYTE BUFFER until it is read, and only then terminated at its first NUL. Modelling
   * it that way makes order genuinely irrelevant rather than approximately so.
   */
  bool stageByte(NetField field, std::size_t index, char value);
  bool stageWifiEnabled(bool on);
  bool stageMqttEnabled(bool on);
  bool stageMqttPort(uint16_t port);
  bool stageMqttPublishPeriodS(uint16_t seconds);
  bool stageMqttHaDiscovery(bool on);
  bool stageMqttQos(uint8_t qos);

  /** Reads back a STAGED value, so an editor can show what is pending. */
  bool getStaged(NetField field, char* out, std::size_t size) const;

  /**
   * Staged reads for the three booleans that share register 564.
   *
   * These exist for one specific reason. Changing one bit of `kMqttFlags` requires a
   * read-modify-write, and reading the LIVE word would rebuild it without any bits a Modbus master
   * had staged but not yet applied — silently discarding the master's pending change while
   * appearing to preserve the other two flags. Reading staged makes the RMW compose with whatever
   * else is pending, which is the same choice `getStaged` makes for text.
   */
  bool stagedMqttHaDiscovery() const { return pending_.mqttHaDiscovery; }
  uint8_t stagedMqttQos() const { return pending_.mqttQos; }

  /** True when anything staged differs from live — what the UI shows as an unsaved edit. */
  bool dirty() const;

  /**
   * Promotes pending to live and bumps the revision.
   *
   * Returns false when nothing was staged, so an apply with no changes is distinguishable from one
   * that did something — a master polling the revision can tell.
   */
  bool apply();

  /** Discards staged changes, restoring pending to live. */
  void revert();

  /**
   * Discards ONE staged field, restoring it from live.
   *
   * Exists so the register layer can reject an invalid base topic without refusing the whole apply.
   * The alternative shipped briefly and was a wedge: validation inside apply() meant a single bad
   * byte written over RS485 made every subsequent apply fail — from the display and the portal too —
   * with no way to clear it, because revert() has no call site anywhere in src/. Dropping the one
   * offending field keeps the other surfaces working and leaves nothing latched.
   */
  void revertField(NetField field);

  /**
   * Promotes everything staged EXCEPT `field`, whose staged bytes are left untouched.
   *
   * The resolution of a three-way problem the register path creates. An invalid base topic must not
   * become live, but the two obvious responses are both wrong:
   *
   *   - refuse the whole apply -> LATCHES. One bad byte over RS485 blocked configuration from every
   *     surface, permanently, because revert() has no call site and §6.3 makes text uneditable at
   *     the panel.
   *   - revert the field -> DESTRUCTIVE. A block write is many registers; an apply from another
   *     surface arriving mid-write would throw away the master's partial field and force it to start
   *     the topic again.
   *
   * Skipping just that field does neither. The other fields land, the master's bytes survive in
   * pending so finishing the write still works, and the caller is told InvalidValue. `dirty()`
   * deliberately stays true while the staged topic remains invalid — there IS still an uncommitted
   * change, and saying otherwise would be a lie a polling master could act on.
   */
  bool applyExcept(NetField field);

  /**
   * Restores the portal login to `admin`/`admin` (R8.2a).
   *
   * Writes LIVE and PENDING together and bumps the revision, because this is a command rather than
   * a staged value: it is reached when somebody is locked out of the portal, and a recovery step
   * that silently needs a follow-up apply is one they will believe failed.
   *
   * Touches ONLY the two portal fields. The forgotten-password path must not cost the operator
   * their totals or their calibration, which is what routing this through a factory reset would do.
   */
  void resetPortalCredentials();

  uint16_t revision() const { return revision_; }

  /**
   * True once an SSID exists — the `wifi.configured` guard of R7.12.
   *
   * Deliberately "stored", not "association succeeded": the WiFi info page is most needed when the
   * device is NOT connecting, and gating it on a successful association would hide the only
   * diagnostic available at exactly the wrong moment.
   */
  bool wifiConfigured() const { return !isEmpty(NetField::WifiSsid); }

  /** True once a broker host exists — the `mqtt.configured` guard. */
  bool mqttConfigured() const { return !isEmpty(NetField::MqttHost); }

 private:
  struct Block {
    char text[static_cast<std::size_t>(NetField::Count)][kMaxValueBytes + 1] = {};
    bool wifiEnabled = false;
    bool mqttEnabled = false;
    uint16_t mqttPort = 1883;
    uint16_t mqttPublishPeriodS = 10;
    bool mqttHaDiscovery = true;
    uint8_t mqttQos = 0;
  };

  static bool copyInto(char* dest, std::size_t capacity, const char* value);

  Block live_{};
  Block pending_{};
  uint16_t revision_ = 0;
};

}  // namespace plc
