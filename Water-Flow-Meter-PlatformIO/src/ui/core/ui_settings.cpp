#include "ui/core/ui_settings.h"

#include <cstring>

#include "modbus/register_map.h"

namespace ui {

namespace {

/** Zero-based sensor index, or sensorCount when the 1-based input is out of range. */
std::size_t sensorSlot(uint8_t sensorIndex, const SettingsAccess& access) {
  if (sensorIndex == 0 || sensorIndex > access.sensorCount) {
    return access.sensorCount;
  }
  return static_cast<std::size_t>(sensorIndex - 1);
}

}  // namespace

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


}  // namespace ui
