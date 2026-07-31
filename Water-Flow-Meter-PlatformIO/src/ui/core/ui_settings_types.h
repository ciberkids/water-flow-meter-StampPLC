#pragma once

#include <cstddef>
#include <cstdint>

#include "modbus/link_limits.h"

namespace ui {

enum class SettingKind : uint8_t { Numeric, Enum, Boolean };

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
  SensorMaxFlow
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

}  // namespace ui
