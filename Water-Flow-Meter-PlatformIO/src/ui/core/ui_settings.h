#pragma once

#include <cstddef>
#include <cstdint>

#include "led/led_controller.h"
#include "modbus/link_settings.h"
#include "modbus/modbus_manager.h"
#include "modbus/sensor_types.h"
#include "ui/core/ui_settings_types.h"

namespace ui {

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

}  // namespace ui
