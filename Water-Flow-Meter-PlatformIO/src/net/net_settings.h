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
  /**
   * Whether to connect with `mqtts://` rather than `mqtt://`.
   *
   * Off by default. `CONFIG_MQTT_TRANSPORT_SSL=y` in the shipped sdkconfig, so this costs only the
   * certificate handling — but it also raises the per-connection RAM and CPU cost on a device whose
   * §2.1 budget is the measurement itself, which is why it is opt-in.
   */
  bool mqttTls() const { return live_.mqttTls; }

  // ── Staging ───────────────────────────────────────────────────────────────────
  /**
   * Stages a text value. Truncates at the field's capacity rather than rejecting, so a master
   * writing a full register block does not fail on trailing padding.
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
  bool stageMqttTls(bool on);

  /** Reads back a STAGED value, so an editor can show what is pending. */
  bool getStaged(NetField field, char* out, std::size_t size) const;

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
    bool mqttTls = false;
  };

  static bool copyInto(char* dest, std::size_t capacity, const char* value);

  Block live_{};
  Block pending_{};
  uint16_t revision_ = 0;
};

}  // namespace plc
