#include "ui/core/ui_settings.h"

#include <cstring>

#include "modbus/register_map.h"
#include "net/net_register_map.h"

namespace ui {

namespace {

/** Zero-based sensor index, or sensorCount when the 1-based input is out of range. */
std::size_t sensorSlot(uint8_t sensorIndex, const SettingsAccess& access) {
  if (sensorIndex == 0 || sensorIndex > access.sensorCount) {
    return access.sensorCount;
  }
  return static_cast<std::size_t>(sensorIndex - 1);
}

/** True for the targets served by NetSettings rather than by the Modbus holding path. */
bool isNetworkTarget(SettingTarget target) {
  switch (target) {
    case SettingTarget::WifiEnabled:
    case SettingTarget::WifiSsid:
    case SettingTarget::WifiPsk:
    case SettingTarget::MqttEnabled:
    case SettingTarget::MqttHost:
    case SettingTarget::MqttPort:
    case SettingTarget::MqttUser:
    case SettingTarget::MqttPassword:
    case SettingTarget::MqttBaseTopic:
    case SettingTarget::MqttDiscoveryPrefix:
    case SettingTarget::MqttPublishPeriod:
    case SettingTarget::MqttHaDiscovery:
    case SettingTarget::MqttTls:
    case SettingTarget::MqttQos:
      return true;
    case SettingTarget::LinkSlaveId:
    case SettingTarget::LinkBaudIndex:
    case SettingTarget::LinkParity:
    case SettingTarget::LinkStopBits:
    case SettingTarget::LedVolumeStep:
    case SettingTarget::LedPulsePeriod:
    case SettingTarget::SensorConnected:
    case SettingTarget::SensorMultiplier:
    case SettingTarget::SensorAdjust:
    case SettingTarget::SensorMaxFlow:
      return false;
  }
  return false;
}

}  // namespace

plc::NetField netFieldFor(SettingTarget target) {
  switch (target) {
    case SettingTarget::WifiSsid:            return plc::NetField::WifiSsid;
    case SettingTarget::WifiPsk:             return plc::NetField::WifiPsk;
    case SettingTarget::MqttHost:            return plc::NetField::MqttHost;
    case SettingTarget::MqttUser:            return plc::NetField::MqttUser;
    case SettingTarget::MqttPassword:        return plc::NetField::MqttPassword;
    case SettingTarget::MqttBaseTopic:       return plc::NetField::MqttBaseTopic;
    case SettingTarget::MqttDiscoveryPrefix: return plc::NetField::MqttDiscoveryPrefix;
    default:                                 return plc::NetField::Count;
  }
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

    // Network. Reads LIVE rather than staged, because NetSettings exposes no staged accessor for
    // the non-text fields and does not need one: writeSetting applies in the same call, so the two
    // are never observably different from the display's side.
    case SettingTarget::WifiEnabled:
      return access.net && access.net->wifiEnabled() ? 1 : 0;
    case SettingTarget::MqttEnabled:
      return access.net && access.net->mqttEnabled() ? 1 : 0;
    case SettingTarget::MqttPort:
      return access.net ? access.net->mqttPort() : 1883;
    case SettingTarget::MqttPublishPeriod:
      return access.net ? access.net->mqttPublishPeriodS() : 10;
    case SettingTarget::MqttHaDiscovery:
      // Defaults to 1 with no storage attached, matching NetSettings' own default: the discovery
      // toggle being ON is what makes the feature work out of the box.
      return !access.net || access.net->mqttHaDiscovery() ? 1 : 0;
    case SettingTarget::MqttTls:
      return access.net && access.net->mqttTls() ? 1 : 0;
    case SettingTarget::MqttQos:
      return access.net ? access.net->mqttQos() : 0;

    // The text targets have no int32_t reading at all. Falling through to 0 would render an SSID
    // as "0"; readSettingText is the accessor for these, and a caller that got here is holding the
    // wrong one.
    case SettingTarget::WifiSsid:
    case SettingTarget::WifiPsk:
    case SettingTarget::MqttHost:
    case SettingTarget::MqttUser:
    case SettingTarget::MqttPassword:
    case SettingTarget::MqttBaseTopic:
    case SettingTarget::MqttDiscoveryPrefix:
      return 0;

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
  // Network settings are handled before the ModbusManager guard, because they do not travel the
  // holding-register path at all — the network block is owned by NetSettings.
  if (isNetworkTarget(setting.target)) {
    if (!access.net) {
      return false;
    }
    plc::NetSettings& net = *access.net;
    const uint16_t word = static_cast<uint16_t>(value & 0xFFFF);

    // Registers 564's three flags share one word, so changing one is a READ-MODIFY-WRITE. Writing
    // the bare value here would clear the other two — turn on HA discovery and TLS would silently
    // switch off. NetRegisterMap::mqttFlags() supplies the current word.
    bool staged = false;
    switch (setting.target) {
      case SettingTarget::MqttHaDiscovery:
      case SettingTarget::MqttTls:
      case SettingTarget::MqttQos: {
        const uint16_t bit = (setting.target == SettingTarget::MqttHaDiscovery)
                                 ? plc::NetRegisterMap::kFlagHaDiscovery
                                 : (setting.target == SettingTarget::MqttTls)
                                       ? plc::NetRegisterMap::kFlagTls
                                       : plc::NetRegisterMap::kFlagQos1;
        uint16_t flags = plc::NetRegisterMap::mqttFlags(net);
        flags = value ? static_cast<uint16_t>(flags | bit)
                      : static_cast<uint16_t>(flags & ~bit);
        staged = plc::NetRegisterMap::stageWrite(net, plc::net_reg::kMqttFlags, flags);
        break;
      }
      default:
        // Everything else owns its register outright, so the descriptor's address is enough.
        // Routing through NetRegisterMap rather than the NetSettings setters keeps ONE
        // implementation of the register convention — including its range checks, which is how a
        // port of 0 or a publish period of 0 gets refused here exactly as it would over RS485.
        staged = plc::NetRegisterMap::stageWrite(net, setting.registerAddress, word);
        break;
    }
    if (!staged) {
      return false;
    }
    // Commit immediately: §4.1.1's reasoning for the link settings applies unchanged — the operator
    // is standing at the device and watched the value change.
    //
    // R5.5 mandates ONE apply path shared by the display, a Modbus master and the web portal, so
    // this promotes every pending field, not only the one just edited. The consequence worth
    // knowing: if a master is midway through writing a multi-register field when the operator
    // commits here, the master's partial value is promoted too. That is inherent to the single
    // apply path the requirement chose, which is also why readSettingText below reads STAGED — the
    // operator sees what would actually be committed rather than a value that hides it.
    //
    // NothingStaged counts as success for the same reason it does in writeSettingText: re-selecting
    // the value a setting already holds is a successful edit, not an error to report.
    const plc::NetApplyError result =
        plc::NetRegisterMap::applyWrite(net, plc::net_reg::kApplyMagic);
    return result == plc::NetApplyError::None || result == plc::NetApplyError::NothingStaged;
  }

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
      if (!access.modbus->applyHoldingWrite(reg, word, plc::WriteOrigin::Display)) {
        return false;
      }
      // The operator is standing at the device and is not depending on the link they
      // are about to change, so the UI stages and commits in one step (§4.1.1). It
      // still goes through the apply register, so the persist-and-restart machinery is
      // the same one a Modbus master drives.
      //
      // WriteOrigin::Display is what stops the 60 s rollback from arming. Rollback's
      // confirmation signal is an incoming frame; on a unit with no master attached there
      // is no such signal, so an armed window would revert every link change made at the
      // display exactly 60 s later — a silent, unexplained reversion of a setting the
      // operator watched take effect.
      return access.modbus->applyHoldingWrite(
          plc::REG_LINK_APPLY, plc::LinkSettingsManager::kApplyMagic, plc::WriteOrigin::Display);
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

bool readSettingText(const SettingDescriptor& setting,
                     const SettingsAccess& access,
                     char* out,
                     std::size_t size) {
  if (!out || size == 0) {
    return false;
  }
  out[0] = '\0';
  if (setting.kind != SettingKind::Text || !access.net) {
    return false;
  }
  const plc::NetField field = netFieldFor(setting.target);
  if (field == plc::NetField::Count) {
    return false;
  }
  // Staged, not live — see the note in writeSetting. The display should show what an ENTER would
  // actually commit.
  return access.net->getStaged(field, out, size);
}

bool writeSettingText(const SettingDescriptor& setting,
                      const char* value,
                      const SettingsAccess& access) {
  if (setting.kind != SettingKind::Text || !access.net || !value) {
    return false;
  }
  const plc::NetField field = netFieldFor(setting.target);
  if (field == plc::NetField::Count) {
    return false;
  }
  if (!access.net->stage(field, value)) {
    return false;
  }
  // One apply path (R5.5), same as the numeric arm.
  //
  // Note this returns true for an apply that changed nothing: staging the value a field already
  // holds is a successful edit from the operator's point of view, even though NetSettings::apply()
  // reports "nothing was dirty" so a polling master can tell the difference. Treating that as a
  // failure would show an error toast for retyping the same SSID.
  const plc::NetApplyError result =
      plc::NetRegisterMap::applyWrite(*access.net, plc::net_reg::kApplyMagic);
  return result == plc::NetApplyError::None || result == plc::NetApplyError::NothingStaged;
}

}  // namespace ui
