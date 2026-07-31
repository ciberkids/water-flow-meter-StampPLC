#pragma once

#include <cstddef>
#include <cstdint>

#include "led/led_controller.h"
#include "modbus/link_settings.h"
#include "modbus/modbus_manager.h"
#include "modbus/sensor_types.h"

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
 * The firmware's own settings catalogue.
 *
 * This is the counterpart of the `category: "setting"` entries in
 * `web/mockup/src/data/actionManifest.json`, and it is what decision **D2** should
 * eventually generate that manifest *from* — so the min/max/step/enum a value editor
 * obeys and the ones the web simulator shows come from one declaration and cannot
 * drift. Until D2 lands the exporter scrapes this file's binding IDs.
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
  /** Scoped to the sensor of the current navigation level (§5.1). */
  bool perSensor;
};

const SettingDescriptor* findSetting(const char* bindingId);
std::size_t settingCount();
const SettingDescriptor* settingAt(std::size_t index);

/** Everything the catalogue needs to read and write live values. */
struct SettingsAccess {
  plc::LinkSettingsManager* link = nullptr;
  LedController* leds = nullptr;
  ModbusManager* modbus = nullptr;
  SensorCharacteristics* configs = nullptr;
  uint16_t* connectedBitmap = nullptr;
  std::size_t sensorCount = 0;
};

int32_t readSetting(const SettingDescriptor& setting,
                    uint8_t sensorIndex,
                    const SettingsAccess& access);

/**
 * Commits a value.
 *
 * Every write goes through `ModbusManager::applyHoldingWrite`, the same entry point a
 * Modbus master uses. That is deliberate: it means the UI cannot bypass the Nyquist
 * validation, the enable-bitmap side effects or the link staging protocol, and there
 * is one implementation of each rather than a second UI-only path that drifts.
 */
bool writeSetting(const SettingDescriptor& setting,
                  uint8_t sensorIndex,
                  int32_t value,
                  const SettingsAccess& access);

/** Clamps to range for numerics, or wraps for enums and booleans. */
int32_t adjustSetting(const SettingDescriptor& setting, int32_t value, int32_t delta);

/** Renders a value for display: an enum label, or the number with its unit. */
void formatSetting(const SettingDescriptor& setting,
                   int32_t value,
                   char* out,
                   std::size_t size);

}  // namespace ui
