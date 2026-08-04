#pragma once

#include <cstddef>
#include <cstdint>

#include "modbus/link_limits.h"

namespace ui {

/**
 * Text is deliberately a separate kind rather than a numeric with a wide range.
 *
 * A numeric editor is built from increment/decrement/commit/discard with hold acceleration; a
 * string has no step, so the model does not extend. It is NOT extended either: there is no
 * on-device text editor. A three-button character wheel is not a usable way to type a 63-character
 * WPA2 passphrase, so text is written from the configuration web portal (§7.6), the RS485 register
 * block (§5.2) or the SD credential file, and only DISPLAYED at the panel — masked when writeOnly.
 *
 * The kind still has to exist, and this is why: the display and the resolver must know not to reach
 * for the int32_t accessors, and the export gate must know to exempt these from the completeness
 * rule. See WiFi_MQTT_Connectivity.md §2.2, §6.2 and §6.3.
 */
enum class SettingKind : uint8_t { Numeric, Enum, Boolean, Text };

/** Which piece of state a setting reads from and writes to. */
enum class SettingTarget : uint8_t {
  LinkSlaveId,
  LinkBaudIndex,
  LinkParity,
  LinkStopBits,
  LedVolumeStep,
  LedPulsePeriod,
  SensorConnected,
  SensorMultiplier,
  SensorAdjust,
  SensorMaxFlow,
  // ── Network (WiFi_MQTT_Connectivity.md §6.1) ───────────────────────────────────
  //
  // One target per setting even where several share a register: kMqttFlags packs HA discovery,
  // QoS and TLS into register 564, and giving them a single target would leave the accessor
  // unable to tell which bit the caller meant.
  WifiEnabled,
  WifiSsid,
  WifiPsk,
  MqttEnabled,
  MqttHost,
  MqttPort,
  MqttUser,
  MqttPassword,
  MqttBaseTopic,
  MqttDiscoveryPrefix,
  MqttPublishPeriod,
  MqttHaDiscovery,
  MqttQos
};

struct SettingOption {
  const char* label;
  int32_t value;
};

/**
 * The firmware's own settings catalogue — see ui_settings.h for how values are read
 * and written.
 *
 * Deliberately in an Arduino-free translation unit. The manifest the web tool validates
 * against is GENERATED from this table (decision D2), and the generator is a host
 * program, so the table cannot depend on Preferences or M5StamPLC. That also makes it
 * host-testable.
 */
struct SettingDescriptor {
  const char* bindingId;
  SettingTarget target;
  SettingKind kind;
  int32_t min;
  int32_t max;
  int32_t step;
  const SettingOption* options;
  uint8_t optionCount;
  const char* unit;
  /** Scoped to the sensor of the current navigation level (Display_UI §5.1). */
  bool perSensor;
  /** Absolute holding register, or kNoRegister when the setting lives in a sensor block. */
  uint16_t registerAddress;
  /**
   * Offset within the sensor's register block for per-sensor settings.
   *
   * These have no single absolute address — it depends on which sensor the navigation
   * level selected — so the offset is the stable fact. The hand-written manifest recorded
   * them as having no register at all, which was simply untrue.
   */
  uint16_t registerOffset;
  const char* description;
  /** Capacity in bytes for a Text setting, excluding the terminator. Zero otherwise. */
  uint16_t maxLength;
  /**
   * A secret: never rendered in full, never readable back over Modbus, never logged.
   *
   * Applies to the WiFi passphrase and the MQTT password. Reading such a register returns
   * zeros rather than raising an exception, so a master doing a block read across the
   * region does not fail (WiFi_MQTT_Connectivity.md §5.1, §8.1).
   */
  bool writeOnly;
};

/** Sentinel for settings that have no single holding register of their own. */
constexpr uint16_t kNoRegister = 0xFFFF;

const SettingDescriptor* findSetting(const char* bindingId);
std::size_t settingCount();
const SettingDescriptor* settingAt(std::size_t index);

/** Clamps to range for numerics, or wraps for enums and booleans. */
int32_t adjustSetting(const SettingDescriptor& setting, int32_t value, int32_t delta);

/** Renders a value for display: an enum label, or the number with its unit. */
void formatSetting(const SettingDescriptor& setting,
                   int32_t value,
                   char* out,
                   std::size_t size);

/**
 * Renders a Text setting for display, masking it when the descriptor says writeOnly.
 *
 * Separate from formatSetting rather than an overload, for the same reason the kinds are
 * separate: a caller holding a descriptor knows which one to reach for, and the compiler
 * enforces it.
 */
void formatSettingText(const SettingDescriptor& setting,
                       const char* value,
                       char* out,
                       std::size_t size);

}  // namespace ui
