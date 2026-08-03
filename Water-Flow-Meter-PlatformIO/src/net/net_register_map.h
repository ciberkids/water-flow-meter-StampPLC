#pragma once

#include <cstddef>
#include <cstdint>

#include "net/net_settings.h"

namespace plc {

/**
 * The network holding-register block (WiFi_MQTT_Connectivity.md §5).
 *
 * Placed at 500, leaving 420-499 free so the sensor count can grow — the sensor blocks currently
 * end at 419 (`SENSOR_1_BASE_ADDR` 100 + 40 x 8).
 *
 * Strings are packed **two characters per register, high byte first**, NUL-padded. That convention
 * is stated here and implemented once, because it is the kind of detail two implementations get
 * subtly different: this header is the only definition, and `net_register_map.cpp` is the only
 * code that acts on it.
 */
namespace net_reg {

inline constexpr uint16_t kBase = 500;

// ── WiFi ──────────────────────────────────────────────────────────────────────────
inline constexpr uint16_t kWifiEnabled   = 500;
inline constexpr uint16_t kWifiState     = 501;  // read-only
inline constexpr uint16_t kWifiRssi      = 502;  // read-only, int16
inline constexpr uint16_t kWifiIp        = 503;  // read-only, 2 registers
inline constexpr uint16_t kWifiMac       = 505;  // read-only, 3 registers
inline constexpr uint16_t kWifiSsid      = 510;  // 16 registers, 32 bytes
inline constexpr uint16_t kWifiPsk       = 526;  // 32 registers, 64 bytes, write-only

// ── MQTT ──────────────────────────────────────────────────────────────────────────
inline constexpr uint16_t kMqttEnabled       = 560;
inline constexpr uint16_t kMqttState         = 561;  // read-only
inline constexpr uint16_t kMqttPort          = 562;
inline constexpr uint16_t kMqttPeriodS       = 563;
// bit 0 HA discovery, bit 1 QoS, bit 2 TLS.
//
// Three booleans in one register rather than three registers, because a master that wants to turn
// the whole MQTT client on in one shot should not need three round trips. The cost is that a
// read-modify-write is mandatory: writing this register sets ALL THREE flags, so a caller changing
// one bit must preserve the others. `NetRegisterMap::mqttFlags()` exists so the UI has one place to
// get the current word from rather than reconstructing the bit layout at each call site.
inline constexpr uint16_t kMqttFlags         = 564;
inline constexpr uint16_t kMqttLastCmdResult = 565;  // read-only, R4.4.2d
inline constexpr uint16_t kMqttHost          = 570;  // 32 registers, 64 bytes
inline constexpr uint16_t kMqttUser          = 602;  // 16 registers, 32 bytes
inline constexpr uint16_t kMqttPassword      = 618;  // 16 registers, write-only
inline constexpr uint16_t kMqttBaseTopic     = 634;  // 24 registers, 48 bytes
inline constexpr uint16_t kMqttPrefix        = 658;  // 16 registers, 32 bytes

// ── Portal and AP (§5.2, remote setup) ────────────────────────────────────────────
inline constexpr uint16_t kPortalRemainingS = 675;  // read-only
inline constexpr uint16_t kApSsid           = 676;  // 16 registers, read-only
inline constexpr uint16_t kApPassword       = 692;  // 16 registers, read-only — R5.3
inline constexpr uint16_t kApIp             = 708;  // 2 registers, read-only
inline constexpr uint16_t kPortalUser       = 712;  // 8 registers, 16 bytes
inline constexpr uint16_t kPortalPassword   = 720;  // 16 registers, write-only

// ── Apply protocol, mirroring REG_LINK_APPLY ──────────────────────────────────────
inline constexpr uint16_t kApply      = 730;
inline constexpr uint16_t kRevision   = 731;  // read-only
inline constexpr uint16_t kLastError  = 732;  // read-only
inline constexpr uint16_t kEnd        = 733;  // one past the last register

/** The magic that commits a staged block — the same value REG_LINK_APPLY uses. */
inline constexpr uint16_t kApplyMagic = 0x5AA5;

/** Registers a text field of `bytes` occupies: two characters each, rounded up. */
constexpr uint16_t textRegisters(std::size_t bytes) {
  return static_cast<uint16_t>((bytes + 1) / 2);
}

/** Where each text field starts, so the mapping lives in one place. */
constexpr uint16_t textBase(NetField field) {
  switch (field) {
    case NetField::WifiSsid:            return kWifiSsid;
    case NetField::WifiPsk:             return kWifiPsk;
    case NetField::MqttHost:            return kMqttHost;
    case NetField::MqttUser:            return kMqttUser;
    case NetField::MqttPassword:        return kMqttPassword;
    case NetField::MqttBaseTopic:       return kMqttBaseTopic;
    case NetField::MqttDiscoveryPrefix: return kMqttPrefix;
    case NetField::PortalUser:          return kPortalUser;
    case NetField::PortalPassword:      return kPortalPassword;
    case NetField::Count:               return 0;
  }
  return 0;
}

}  // namespace net_reg

/** Why the last apply failed, reported at `kLastError`. */
enum class NetApplyError : uint16_t {
  None = 0,
  NothingStaged,
  BadMagic,
  InvalidValue
};

/**
 * Translates the register block to and from NetSettings.
 *
 * Separate from NetSettings so the packing convention is testable without the settings semantics,
 * and so a change to one cannot quietly alter the other.
 */
class NetRegisterMap {
 public:
  /** True for an address inside the block. */
  static bool contains(uint16_t address) {
    return address >= net_reg::kBase && address < net_reg::kEnd;
  }

  /**
   * True when a master may WRITE this address.
   *
   * A read-only address is not an error to write — §5.1 requires a block write across the region to
   * succeed rather than except — it is simply ignored.
   */
  static bool isWritable(uint16_t address);

  /**
   * True when reading this address must return zeros regardless of what is stored (§5.1).
   *
   * The secrets. Note the deliberate asymmetry of R5.3: the AP password is NOT here, because it
   * describes an access point the device is broadcasting rather than an operator secret it was
   * given.
   */
  static bool readsAsZero(uint16_t address);

  /** Stages one register write. False when the address is not writable or the value is refused. */
  static bool stageWrite(NetSettings& settings, uint16_t address, uint16_t value);

  /**
   * Handles a write to `kApply`. Returns the resulting error, `None` on success.
   *
   * Split from stageWrite so the caller can tell "staged" from "committed" without inspecting the
   * address itself.
   */
  static NetApplyError applyWrite(NetSettings& settings, uint16_t value);

  /** Packs the live settings into `out`, which must span the whole block. */
  static void publish(const NetSettings& settings, uint16_t* out, std::size_t count);

  /**
   * The current `kMqttFlags` word, assembled from the LIVE settings.
   *
   * The read half of the read-modify-write that register 564 forces on anyone changing one of its
   * three booleans. Exposed rather than left to callers because reconstructing the bit layout at
   * each call site is how bit 2 would eventually get cleared by a write meant only for bit 0 — the
   * shared-register bug this method exists to make unlikely.
   */
  static uint16_t mqttFlags(const NetSettings& settings) {
    return static_cast<uint16_t>((settings.mqttHaDiscovery() ? kFlagHaDiscovery : 0u) |
                                 (settings.mqttQos() != 0 ? kFlagQos1 : 0u) |
                                 (settings.mqttTls() ? kFlagTls : 0u));
  }

  /**
   * The same word assembled from the STAGED settings — the correct read half of the RMW.
   *
   * `mqttFlags` above serves `publish()`, which must report what is in force. A caller about to
   * modify one bit needs the other bits as they will be after the next apply, not as they are now:
   * rebuilding from live would drop any flag a Modbus master had staged and not yet committed,
   * which is the shared-register bug one level up from the one `mqttFlags` prevents.
   */
  static uint16_t mqttFlagsStaged(const NetSettings& settings) {
    return static_cast<uint16_t>((settings.stagedMqttHaDiscovery() ? kFlagHaDiscovery : 0u) |
                                 (settings.stagedMqttQos() != 0 ? kFlagQos1 : 0u) |
                                 (settings.stagedMqttTls() ? kFlagTls : 0u));
  }

  /** Bit masks within `kMqttFlags`, so no caller writes the literals. */
  static constexpr uint16_t kFlagHaDiscovery = 0x01u;
  static constexpr uint16_t kFlagQos1        = 0x02u;
  static constexpr uint16_t kFlagTls         = 0x04u;

  /** Two characters per register, high byte first. */
  static uint16_t packChars(char high, char low) {
    return static_cast<uint16_t>((static_cast<uint16_t>(static_cast<uint8_t>(high)) << 8) |
                                 static_cast<uint8_t>(low));
  }
};

}  // namespace plc
