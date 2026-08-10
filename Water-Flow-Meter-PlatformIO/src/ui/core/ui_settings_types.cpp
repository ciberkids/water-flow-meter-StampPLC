#include "ui/core/ui_settings_types.h"

#include "modbus/register_map.h"
#include "net/net_register_map.h"

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
/**
 * The two calibration forms. Labels kept short because they share a 40-column row with their label,
 * and `Pulses/L` is what the datasheet itself says.
 */
constexpr SettingOption kCalibrationOptions[] = {{"Formula", 0}, {"Pulses/L", 1}};
// §4.2 implements QoS 0 and 1. QoS 2 is deliberately absent rather than present-and-rejected:
// an option the wheel can land on but the client refuses is a worse experience than one that
// was never offered.
constexpr SettingOption kQosOptions[] = {{"0", 0}, {"1", 1}};

constexpr uint8_t kBoolOptionCount =
    static_cast<uint8_t>(sizeof(kBoolOptions) / sizeof(kBoolOptions[0]));

// Ranges and steps mirror Display_UI_Requirements §5.2 and §5.3.
constexpr SettingDescriptor kSettings[] = {
    {"config.modbusSlaveId", SettingTarget::LinkSlaveId, SettingKind::Numeric,
     LinkLimits::kMinSlaveId, LinkLimits::kMaxSlaveId, 1, nullptr, 0, nullptr, false, 40, kNoRegister, "Modbus slave address", 0, false},
    {"config.baudRate", SettingTarget::LinkBaudIndex, SettingKind::Enum,
     0, LinkLimits::kBaudCount - 1, 1, kBaudOptions,
     static_cast<uint8_t>(sizeof(kBaudOptions) / sizeof(kBaudOptions[0])), nullptr, false, 41, kNoRegister, "RS485 baud rate (register stores the list index)", 0, false},
    {"config.parity", SettingTarget::LinkParity, SettingKind::Enum,
     0, 2, 1, kParityOptions,
     static_cast<uint8_t>(sizeof(kParityOptions) / sizeof(kParityOptions[0])), nullptr, false, 42, kNoRegister, "UART parity", 0, false},
    {"config.stopBits", SettingTarget::LinkStopBits, SettingKind::Enum,
     1, 2, 1, kStopBitOptions,
     static_cast<uint8_t>(sizeof(kStopBitOptions) / sizeof(kStopBitOptions[0])), nullptr, false, 43, kNoRegister, "UART stop bits", 0, false},
    {"config.ledPulseVolume", SettingTarget::LedVolumeStep, SettingKind::Enum,
     1, 100, 1, kLedVolumeOptions,
     static_cast<uint8_t>(sizeof(kLedVolumeOptions) / sizeof(kLedVolumeOptions[0])), "L", false, 31, kNoRegister, "Volume per red LED pulse", 0, false},
    {"config.ledPulsePeriod", SettingTarget::LedPulsePeriod, SettingKind::Numeric,
     100, 2000, 1, nullptr, 0, "ms", false, 32, kNoRegister, "Red LED pulse period", 0, false},
    {"config.sensor.connected", SettingTarget::SensorConnected, SettingKind::Boolean,
     0, 1, 1, kBoolOptions,
     static_cast<uint8_t>(sizeof(kBoolOptions) / sizeof(kBoolOptions[0])), nullptr, true, 10, kNoRegister, "Sensor enabled (bit n of the connected-sensors bitmap)", 0, false},
    /**
     * Lower bound 1, not -32768. The multiplier is a DIVISOR:
     * `flowLpm = (frequency - adjust) / f_multiplier` (sensor_state_engine.cpp:32).
     *
     * The old domain let the editor reach two values the device cannot use. Zero fails
     * `configIsValid`, so the channel silently never becomes ready — the operator sees `SET?` on a
     * channel they just finished configuring, with nothing saying why. A negative multiplier maps
     * rising frequency to falling flow, which the `flowRateLpm < 0` clamp then pins at zero forever:
     * a channel that reads 0.00 no matter how much water passes it.
     *
     * Neither is reachable by accident from 1 — they were reachable because the bound was written
     * as the storage type's range rather than the quantity's.
     */
    {"config.sensor.multiplier", SettingTarget::SensorMultiplier, SettingKind::Numeric,
     1, 32767, 1, nullptr, 0, nullptr, true, kNoRegister, plc::OFF_CFG_F_MULT, "Frequency multiplier F", 0, false},
    {"config.sensor.adjust", SettingTarget::SensorAdjust, SettingKind::Numeric,
     -32768, 32767, 1, nullptr, 0, nullptr, true, kNoRegister, plc::OFF_CFG_ADJUST, "Frequency offset Adjust", 0, false},
    {"config.sensor.maxFlow", SettingTarget::SensorMaxFlow, SettingKind::Numeric,
     0, 65535, 1, nullptr, 0, "L/min", true, kNoRegister, plc::OFF_CFG_Q_MAX, "Nominal max flow Q", 0, false},
    /**
     * Which of the two calibration forms this channel uses.
     *
     * Meters are specified one way or the other and never both: a datasheet prints either
     * `F = 6*Q - 8` or `450 pulses/L`. Making the operator say which removes the guesswork from
     * every other calibration row — and makes it visible WHY Multiplier reads `--` on a channel
     * calibrated by pulses.
     */
    {"config.sensor.calibrationType", SettingTarget::SensorCalibrationType, SettingKind::Enum,
     0, 1, 1, kCalibrationOptions,
     static_cast<uint8_t>(sizeof(kCalibrationOptions) / sizeof(kCalibrationOptions[0])), nullptr, true,
     kNoRegister, plc::OFF_CFG_CAL_TYPE, "How this channel is calibrated", 0, false},
    /**
     * Pulses per litre, exact. Lower bound 1, because zero fails `configIsValid` and is a divisor.
     *
     * This exists because `f_multiplier` cannot hold it. A meter rated 450 pulses/L needs a
     * multiplier of 7.5, and an int16_t offers 7 or 8 — a 6% error on every reading, on the form
     * most meters are actually sold with.
     */
    {"config.sensor.pulsesPerLiter", SettingTarget::SensorPulsesPerLitre, SettingKind::Numeric,
     1, 65535, 1, nullptr, 0, "p/L", true, kNoRegister, plc::OFF_CFG_PULSES_PER_L,
     "Pulses per litre, when calibrated that way", 0, false},

    // ── Network: WiFi_MQTT_Connectivity.md §6.1, fourteen settings of which seven are text ──
    //
    // registerAddress is the FIRST register of each text field, which is what a Modbus master
    // needs; the span follows from maxLength via net_reg::textRegisters(). The three flags that
    // share register 564 all report 564 — truthful, and the accessor knows which bit each means.
    //
    // Text settings carry min/max/step of 0 because none of the three means anything for a string:
    // adjustSetting has no text arm (§6.2 item 5) and there is no on-device text editor at all
    // (§6.3). They are DISPLAYED at the panel and written from the web portal, RS485 or the SD
    // credential file. A host check asserts no text setting has an editor screen, which is the
    // invariant that replaced "every setting has one".
    {"config.wifi.enabled", SettingTarget::WifiEnabled, SettingKind::Boolean,
     0, 1, 1, kBoolOptions, kBoolOptionCount, nullptr, false,
     plc::net_reg::kWifiEnabled, kNoRegister, "WiFi radio enabled", 0, false},
    {"config.wifi.ssid", SettingTarget::WifiSsid, SettingKind::Text,
     0, 0, 0, nullptr, 0, nullptr, false,
     plc::net_reg::kWifiSsid, kNoRegister, "WiFi network name", 32, false},
    {"config.wifi.psk", SettingTarget::WifiPsk, SettingKind::Text,
     0, 0, 0, nullptr, 0, nullptr, false,
     plc::net_reg::kWifiPsk, kNoRegister, "WiFi passphrase (never read back)", 63, true},

    {"config.mqtt.enabled", SettingTarget::MqttEnabled, SettingKind::Boolean,
     0, 1, 1, kBoolOptions, kBoolOptionCount, nullptr, false,
     plc::net_reg::kMqttEnabled, kNoRegister, "MQTT client enabled", 0, false},
    {"config.mqtt.host", SettingTarget::MqttHost, SettingKind::Text,
     0, 0, 0, nullptr, 0, nullptr, false,
     plc::net_reg::kMqttHost, kNoRegister, "MQTT broker hostname or IP", 64, false},
    {"config.mqtt.port", SettingTarget::MqttPort, SettingKind::Numeric,
     1, 65535, 1, nullptr, 0, nullptr, false,
     plc::net_reg::kMqttPort, kNoRegister, "MQTT broker port", 0, false},
    {"config.mqtt.user", SettingTarget::MqttUser, SettingKind::Text,
     0, 0, 0, nullptr, 0, nullptr, false,
     plc::net_reg::kMqttUser, kNoRegister, "MQTT username", 32, false},
    {"config.mqtt.password", SettingTarget::MqttPassword, SettingKind::Text,
     0, 0, 0, nullptr, 0, nullptr, false,
     plc::net_reg::kMqttPassword, kNoRegister, "MQTT password (never read back)", 32, true},
    {"config.mqtt.baseTopic", SettingTarget::MqttBaseTopic, SettingKind::Text,
     0, 0, 0, nullptr, 0, nullptr, false,
     plc::net_reg::kMqttBaseTopic, kNoRegister, "MQTT topic prefix for this device", 48, false},
    {"config.mqtt.discoveryPrefix", SettingTarget::MqttDiscoveryPrefix, SettingKind::Text,
     0, 0, 0, nullptr, 0, nullptr, false,
     plc::net_reg::kMqttPrefix, kNoRegister,
     "Home Assistant discovery prefix (default homeassistant)", 32, false},
    {"config.mqtt.publishPeriod", SettingTarget::MqttPublishPeriod, SettingKind::Numeric,
     1, 3600, 1, nullptr, 0, "s", false,
     plc::net_reg::kMqttPeriodS, kNoRegister, "Minimum interval between publishes", 0, false},
    {"config.mqtt.haDiscovery", SettingTarget::MqttHaDiscovery, SettingKind::Boolean,
     0, 1, 1, kBoolOptions, kBoolOptionCount, nullptr, false,
     plc::net_reg::kMqttFlags, kNoRegister,
     "Publish Home Assistant discovery messages (bit 0 of 564)", 0, false},
    {"config.mqtt.qos", SettingTarget::MqttQos, SettingKind::Enum,
     0, 1, 1, kQosOptions,
     static_cast<uint8_t>(sizeof(kQosOptions) / sizeof(kQosOptions[0])), nullptr, false,
     plc::net_reg::kMqttFlags, kNoRegister,
     "Publish QoS (bit 1 of 564)", 0, false}};

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

/**
 * Widest listed option list, in characters, before it is summarised.
 *
 * Mirrors `kMaxListedWidth` in `web/mockup/src/utils/settingHints.ts`, which is what draws the
 * mockup. The two are checked against each other by the range-hint unit tests rather than by
 * anyone remembering: a mockup that lists where the device summarises is a mockup that lies about
 * the one thing this row exists to show.
 */
constexpr std::size_t kMaxListedRangeWidth = 20;

void formatSettingRange(const SettingDescriptor& setting, char* out, std::size_t size) {
  if (!out || size == 0) {
    return;
  }
  out[0] = '\0';
  if (setting.kind == SettingKind::Text) {
    return;
  }
  if (setting.options && setting.optionCount > 0) {
    // Measure before writing: the choice between listing and summarising depends on the total
    // width, which is not known until every label has been counted.
    std::size_t listed = 0;
    for (uint8_t i = 0; i < setting.optionCount; ++i) {
      listed += std::strlen(setting.options[i].label);
      if (i + 1 < setting.optionCount) {
        listed += 3;  // the " / " separator
      }
    }
    if (listed <= kMaxListedRangeWidth) {
      std::size_t written = 0;
      for (uint8_t i = 0; i < setting.optionCount && written + 1 < size; ++i) {
        written += static_cast<std::size_t>(std::snprintf(out + written,
                                                         size - written,
                                                         "%s%s",
                                                         i == 0 ? "" : " / ",
                                                         setting.options[i].label));
      }
      return;
    }
    std::snprintf(out,
                  size,
                  "%s..%s (%u)",
                  setting.options[0].label,
                  setting.options[setting.optionCount - 1].label,
                  static_cast<unsigned>(setting.optionCount));
    return;
  }
  if (setting.unit) {
    std::snprintf(out, size, "%ld to %ld %s", static_cast<long>(setting.min),
                  static_cast<long>(setting.max), setting.unit);
  } else {
    std::snprintf(out, size, "%ld to %ld", static_cast<long>(setting.min),
                  static_cast<long>(setting.max));
  }
}

void formatSettingText(const SettingDescriptor& setting,
                       const char* value,
                       char* out,
                       std::size_t size) {
  if (!out || size == 0) {
    return;
  }
  if (!value || value[0] == '\0') {
    // An empty credential is the default state, not an error. Saying so beats a blank line
    // that could equally mean "failed to read".
    std::snprintf(out, size, "%s", "(not set)");
    return;
  }
  if (setting.writeOnly) {
    // Length is deliberately not revealed either — a fixed run of asterisks tells an
    // onlooker nothing about the passphrase.
    std::snprintf(out, size, "%s", "********");
    return;
  }
  std::snprintf(out, size, "%s", value);
}

}  // namespace ui
