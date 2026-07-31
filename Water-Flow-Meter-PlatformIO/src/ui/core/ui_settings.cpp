#include "ui/core/ui_settings.h"

#include <cstdio>
#include <cstring>

#include "modbus/register_map.h"

namespace ui {

namespace {

using plc::LinkSettings;

constexpr SettingOption kBaudOptions[] = {
    {"1200", 0},  {"2400", 1},  {"4800", 2},   {"9600", 3},
    {"19200", 4}, {"38400", 5}, {"57600", 6},  {"115200", 7}};
constexpr SettingOption kParityOptions[] = {{"None", 0}, {"Even", 1}, {"Odd", 2}};
constexpr SettingOption kStopBitOptions[] = {{"1", 1}, {"2", 2}};
constexpr SettingOption kLedVolumeOptions[] = {{"1", 1}, {"10", 10}, {"100", 100}};
constexpr SettingOption kBoolOptions[] = {{"Off", 0}, {"On", 1}};

// Ranges and steps mirror Display_UI_Requirements §5.2 and §5.3.
constexpr SettingDescriptor kSettings[] = {
    {"config.modbusSlaveId", SettingTarget::LinkSlaveId, SettingKind::Numeric,
     LinkSettings::kMinSlaveId, LinkSettings::kMaxSlaveId, 1, nullptr, 0, nullptr, false},
    {"config.baudRate", SettingTarget::LinkBaudIndex, SettingKind::Enum,
     0, LinkSettings::kBaudCount - 1, 1, kBaudOptions,
     static_cast<uint8_t>(sizeof(kBaudOptions) / sizeof(kBaudOptions[0])), nullptr, false},
    {"config.parity", SettingTarget::LinkParity, SettingKind::Enum,
     0, 2, 1, kParityOptions,
     static_cast<uint8_t>(sizeof(kParityOptions) / sizeof(kParityOptions[0])), nullptr, false},
    {"config.stopBits", SettingTarget::LinkStopBits, SettingKind::Enum,
     1, 2, 1, kStopBitOptions,
     static_cast<uint8_t>(sizeof(kStopBitOptions) / sizeof(kStopBitOptions[0])), nullptr, false},
    {"config.ledPulseVolume", SettingTarget::LedVolumeStep, SettingKind::Enum,
     1, 100, 1, kLedVolumeOptions,
     static_cast<uint8_t>(sizeof(kLedVolumeOptions) / sizeof(kLedVolumeOptions[0])), "L", false},
    {"config.ledPulsePeriod", SettingTarget::LedPulsePeriod, SettingKind::Numeric,
     100, 2000, 1, nullptr, 0, "ms", false},
    {"config.sensor.connected", SettingTarget::SensorConnected, SettingKind::Boolean,
     0, 1, 1, kBoolOptions,
     static_cast<uint8_t>(sizeof(kBoolOptions) / sizeof(kBoolOptions[0])), nullptr, true},
    {"config.sensor.multiplier", SettingTarget::SensorMultiplier, SettingKind::Numeric,
     -32768, 32767, 1, nullptr, 0, nullptr, true},
    {"config.sensor.adjust", SettingTarget::SensorAdjust, SettingKind::Numeric,
     -32768, 32767, 1, nullptr, 0, nullptr, true},
    {"config.sensor.maxFlow", SettingTarget::SensorMaxFlow, SettingKind::Numeric,
     0, 65535, 1, nullptr, 0, "L/min", true}};

constexpr std::size_t kSettingCount = sizeof(kSettings) / sizeof(kSettings[0]);

/** Zero-based sensor index, or kNumSensors when the 1-based input is out of range. */
std::size_t sensorSlot(uint8_t sensorIndex, const SettingsAccess& access) {
  if (sensorIndex == 0 || sensorIndex > access.sensorCount) {
    return access.sensorCount;
  }
  return static_cast<std::size_t>(sensorIndex - 1);
}

}  // namespace

const SettingDescriptor* findSetting(const char* bindingId) {
  if (!bindingId) {
    return nullptr;
  }
  for (const auto& setting : kSettings) {
    if (std::strcmp(setting.bindingId, bindingId) == 0) {
      return &setting;
    }
  }
  return nullptr;
}

std::size_t settingCount() { return kSettingCount; }

const SettingDescriptor* settingAt(std::size_t index) {
  return index < kSettingCount ? &kSettings[index] : nullptr;
}

int32_t readSetting(const SettingDescriptor& setting,
                    uint8_t sensorIndex,
                    const SettingsAccess& access) {
  switch (setting.target) {
    case SettingTarget::LinkSlaveId:
      return access.link ? access.link->staged().slaveId : 0;
    case SettingTarget::LinkBaudIndex:
      return access.link ? access.link->staged().baudIndex : 0;
    case SettingTarget::LinkParity:
      return access.link ? access.link->staged().parity : 0;
    case SettingTarget::LinkStopBits:
      return access.link ? access.link->staged().stopBits : 1;
    case SettingTarget::LedVolumeStep:
      return access.leds ? access.leds->volumeStepLiters() : 1;
    case SettingTarget::LedPulsePeriod:
      return access.leds ? access.leds->pulsePeriodMs() : 500;
    default:
      break;
  }

  const std::size_t slot = sensorSlot(sensorIndex, access);
  if (slot >= access.sensorCount) {
    return 0;
  }
  switch (setting.target) {
    case SettingTarget::SensorConnected:
      return access.connectedBitmap ? ((*access.connectedBitmap >> slot) & 0x01) : 0;
    case SettingTarget::SensorMultiplier:
      return access.configs ? access.configs[slot].f_multiplier : 0;
    case SettingTarget::SensorAdjust:
      return access.configs ? access.configs[slot].adjust : 0;
    case SettingTarget::SensorMaxFlow:
      return access.configs ? access.configs[slot].q_max : 0;
    default:
      return 0;
  }
}

bool writeSetting(const SettingDescriptor& setting,
                  uint8_t sensorIndex,
                  int32_t value,
                  const SettingsAccess& access) {
  if (!access.modbus) {
    return false;
  }
  const uint16_t word = static_cast<uint16_t>(value & 0xFFFF);

  switch (setting.target) {
    case SettingTarget::LinkSlaveId:
    case SettingTarget::LinkBaudIndex:
    case SettingTarget::LinkParity:
    case SettingTarget::LinkStopBits: {
      const uint16_t reg = (setting.target == SettingTarget::LinkSlaveId)  ? plc::REG_LINK_SLAVE_ID
                           : (setting.target == SettingTarget::LinkBaudIndex) ? plc::REG_LINK_BAUD_INDEX
                           : (setting.target == SettingTarget::LinkParity) ? plc::REG_LINK_PARITY
                                                                          : plc::REG_LINK_STOP_BITS;
      if (!access.modbus->applyHoldingWrite(reg, word)) {
        return false;
      }
      // The operator is standing at the device and is not depending on the link they
      // are about to change, so the UI stages and commits in one step (§4.1.1). It
      // still goes through the apply register, so the restart and rollback machinery
      // is the same one a Modbus master drives.
      return access.modbus->applyHoldingWrite(plc::REG_LINK_APPLY,
                                              plc::LinkSettingsManager::kApplyMagic);
    }
    case SettingTarget::LedVolumeStep:
      return access.modbus->applyHoldingWrite(plc::REG_LED_RED_VOLUME_STEP, word);
    case SettingTarget::LedPulsePeriod:
      return access.modbus->applyHoldingWrite(plc::REG_LED_RED_PULSE_PERIOD, word);
    default:
      break;
  }

  const std::size_t slot = sensorSlot(sensorIndex, access);
  if (slot >= access.sensorCount) {
    return false;
  }

  if (setting.target == SettingTarget::SensorConnected) {
    if (!access.connectedBitmap) {
      return false;
    }
    const uint16_t mask = static_cast<uint16_t>(1u << slot);
    const uint16_t next = value ? static_cast<uint16_t>(*access.connectedBitmap | mask)
                                : static_cast<uint16_t>(*access.connectedBitmap & ~mask);
    return access.modbus->applyHoldingWrite(plc::REG_CONNECTED_SENSORS_BITMAP, next);
  }

  const uint16_t base = plc::sensorBaseAddress(slot);
  switch (setting.target) {
    case SettingTarget::SensorMultiplier:
      return access.modbus->applyHoldingWrite(base + plc::OFF_CFG_F_MULT, word);
    case SettingTarget::SensorAdjust:
      return access.modbus->applyHoldingWrite(base + plc::OFF_CFG_ADJUST, word);
    case SettingTarget::SensorMaxFlow:
      return access.modbus->applyHoldingWrite(base + plc::OFF_CFG_Q_MAX, word);
    default:
      return false;
  }
}

int32_t adjustSetting(const SettingDescriptor& setting, int32_t value, int32_t delta) {
  if (setting.kind == SettingKind::Numeric) {
    // §5.4: numerics clamp at their ends rather than wrapping, so holding UP cannot
    // roll a max flow of 65535 round to zero.
    int64_t next = static_cast<int64_t>(value) + delta;
    if (next < setting.min) next = setting.min;
    if (next > setting.max) next = setting.max;
    return static_cast<int32_t>(next);
  }

  // Enums and booleans cycle. Step through the option list so the stored values do
  // not have to be contiguous (LED volume is 1/10/100).
  if (!setting.options || setting.optionCount == 0) {
    return value;
  }
  int index = 0;
  for (uint8_t i = 0; i < setting.optionCount; ++i) {
    if (setting.options[i].value == value) {
      index = i;
      break;
    }
  }
  const int count = setting.optionCount;
  const int stepCount = delta >= 0 ? 1 : -1;
  index = ((index + stepCount) % count + count) % count;
  return setting.options[index].value;
}

void formatSetting(const SettingDescriptor& setting,
                   int32_t value,
                   char* out,
                   std::size_t size) {
  if (!out || size == 0) {
    return;
  }
  if (setting.options) {
    for (uint8_t i = 0; i < setting.optionCount; ++i) {
      if (setting.options[i].value == value) {
        if (setting.unit) {
          std::snprintf(out, size, "%s %s", setting.options[i].label, setting.unit);
        } else {
          std::snprintf(out, size, "%s", setting.options[i].label);
        }
        return;
      }
    }
  }
  if (setting.unit) {
    std::snprintf(out, size, "%ld %s", static_cast<long>(value), setting.unit);
  } else {
    std::snprintf(out, size, "%ld", static_cast<long>(value));
  }
}

}  // namespace ui
