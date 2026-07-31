#include "ui/core/ui_settings_types.h"

#include "modbus/register_map.h"

#include <cstdio>
#include <cstring>

namespace ui {

namespace {

using plc::LinkLimits;

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
     LinkLimits::kMinSlaveId, LinkLimits::kMaxSlaveId, 1, nullptr, 0, nullptr, false, 40, kNoRegister, "Modbus slave address"},
    {"config.baudRate", SettingTarget::LinkBaudIndex, SettingKind::Enum,
     0, LinkLimits::kBaudCount - 1, 1, kBaudOptions,
     static_cast<uint8_t>(sizeof(kBaudOptions) / sizeof(kBaudOptions[0])), nullptr, false, 41, kNoRegister, "RS485 baud rate (register stores the list index)"},
    {"config.parity", SettingTarget::LinkParity, SettingKind::Enum,
     0, 2, 1, kParityOptions,
     static_cast<uint8_t>(sizeof(kParityOptions) / sizeof(kParityOptions[0])), nullptr, false, 42, kNoRegister, "UART parity"},
    {"config.stopBits", SettingTarget::LinkStopBits, SettingKind::Enum,
     1, 2, 1, kStopBitOptions,
     static_cast<uint8_t>(sizeof(kStopBitOptions) / sizeof(kStopBitOptions[0])), nullptr, false, 43, kNoRegister, "UART stop bits"},
    {"config.ledPulseVolume", SettingTarget::LedVolumeStep, SettingKind::Enum,
     1, 100, 1, kLedVolumeOptions,
     static_cast<uint8_t>(sizeof(kLedVolumeOptions) / sizeof(kLedVolumeOptions[0])), "L", false, 31, kNoRegister, "Volume per red LED pulse"},
    {"config.ledPulsePeriod", SettingTarget::LedPulsePeriod, SettingKind::Numeric,
     100, 2000, 1, nullptr, 0, "ms", false, 32, kNoRegister, "Red LED pulse period"},
    {"config.sensor.connected", SettingTarget::SensorConnected, SettingKind::Boolean,
     0, 1, 1, kBoolOptions,
     static_cast<uint8_t>(sizeof(kBoolOptions) / sizeof(kBoolOptions[0])), nullptr, true, 10, kNoRegister, "Sensor enabled (bit n of the connected-sensors bitmap)"},
    {"config.sensor.multiplier", SettingTarget::SensorMultiplier, SettingKind::Numeric,
     -32768, 32767, 1, nullptr, 0, nullptr, true, kNoRegister, plc::OFF_CFG_F_MULT, "Frequency multiplier F"},
    {"config.sensor.adjust", SettingTarget::SensorAdjust, SettingKind::Numeric,
     -32768, 32767, 1, nullptr, 0, nullptr, true, kNoRegister, plc::OFF_CFG_ADJUST, "Frequency offset Adjust"},
    {"config.sensor.maxFlow", SettingTarget::SensorMaxFlow, SettingKind::Numeric,
     0, 65535, 1, nullptr, 0, "L/min", true, kNoRegister, plc::OFF_CFG_Q_MAX, "Nominal max flow Q"}};

constexpr std::size_t kSettingCount = sizeof(kSettings) / sizeof(kSettings[0]);

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
