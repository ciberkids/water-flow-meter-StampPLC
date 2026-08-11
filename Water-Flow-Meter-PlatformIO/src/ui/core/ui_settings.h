#pragma once

#include <cstddef>
#include <cstdint>

#include "led/led_controller.h"
#include "modbus/link_settings.h"
#include "modbus/modbus_manager.h"
#include "modbus/sensor_types.h"
#include "net/net_settings.h"
#include "ui/core/ui_settings_types.h"

namespace ui {

/** Everything the catalogue needs to read and write live values. */
struct SettingsAccess {
  plc::LinkSettingsManager* link = nullptr;
  LedController* leds = nullptr;
  ModbusManager* modbus = nullptr;
  SensorCharacteristics* configs = nullptr;
  uint16_t* connectedBitmap = nullptr;
  /**
   * Which unit the panel shows flows in (REG_DISPLAY_FLOW_UNIT), owned by firmware.cpp.
   *
   * A pointer, like `connectedBitmap`: the value is persisted device-wide state, and the settings
   * layer reads and writes it rather than owning a second copy.
   */
  uint16_t* displayFlowUnit = nullptr;
  std::size_t sensorCount = 0;
  /**
   * WiFi, MQTT and portal configuration (WiFi_MQTT_Connectivity.md §6.1).
   *
   * Reached directly rather than through ModbusManager because the network block is not part of
   * the holding-register path — NetSettings owns its own staged/apply protocol, and
   * NetRegisterMap is the single implementation of the register convention on top of it.
   */
  plc::NetSettings* net = nullptr;
};

/**
 * The NetField a network Text setting reads and writes, or `Count` for anything else.
 *
 * Exposed so a caller holding a descriptor can find the storage without a second switch, and so
 * the mapping is testable on its own.
 */
plc::NetField netFieldFor(SettingTarget target);

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

/**
 * Reads a `SettingKind::Text` value into `out`.
 *
 * A parallel accessor rather than an overload of readSetting (§6.2 item 3): a caller holds a
 * descriptor, knows its kind, and reaches for the matching function — so the compiler catches a
 * mismatch instead of an int32_t silently carrying a truncated pointer-ish value.
 *
 * Reads the STAGED value, matching what the numeric arm does for link settings, so an editor shows
 * what is pending rather than what was last applied.
 *
 * Returns false when the setting is not text, storage is absent, or the buffer is too small.
 * Secrets are returned IN FULL — masking belongs to formatSettingText, because the text editor
 * legitimately needs the real characters to edit them.
 */
bool readSettingText(const SettingDescriptor& setting,
                     const SettingsAccess& access,
                     char* out,
                     std::size_t size);

/**
 * Commits a text value.
 *
 * Stages then applies in one step, for the same reason writeSetting does with the link settings:
 * the operator is standing at the device and watched the change take effect (§4.1.1). Truncates at
 * the field's capacity rather than refusing, matching NetSettings::stage.
 */
bool writeSettingText(const SettingDescriptor& setting,
                      const char* value,
                      const SettingsAccess& access);

}  // namespace ui
