#include "net/net_register_map.h"

#include <cstring>

namespace plc {

namespace {

/** The text field an address falls inside, or Count when it is not a text register. */
NetField fieldAt(uint16_t address, uint16_t* offsetOut) {
  for (std::size_t i = 0; i < static_cast<std::size_t>(NetField::Count); ++i) {
    const auto field = static_cast<NetField>(i);
    const uint16_t base = net_reg::textBase(field);
    const uint16_t span = net_reg::textRegisters(netFieldCapacity(field));
    if (address >= base && address < base + span) {
      if (offsetOut) {
        *offsetOut = address - base;
      }
      return field;
    }
  }
  return NetField::Count;
}

}  // namespace

bool NetRegisterMap::readsAsZero(uint16_t address) {
  uint16_t offset = 0;
  const NetField field = fieldAt(address, &offset);
  // The AP password is deliberately absent from this test — R5.3. It describes an access point the
  // device is actively broadcasting, not a secret it was handed, and a remote operator needs it to
  // direct someone standing at the device.
  return field != NetField::Count && netFieldIsSecret(field);
}

bool NetRegisterMap::isWritable(uint16_t address) {
  switch (address) {
    case net_reg::kWifiEnabled:
    case net_reg::kMqttEnabled:
    case net_reg::kMqttPort:
    case net_reg::kMqttPeriodS:
    case net_reg::kMqttFlags:
    case net_reg::kPortalReset:
    case net_reg::kApply:
      return true;
    // Read-only: state, diagnostics, and everything describing the AP the device offers.
    case net_reg::kWifiState:
    case net_reg::kWifiRssi:
    case net_reg::kMqttState:
    case net_reg::kMqttLastCmdResult:
    case net_reg::kPortalRemainingS:
    case net_reg::kRevision:
    case net_reg::kLastError:
      return false;
    default:
      break;
  }
  if (address >= net_reg::kWifiIp && address < net_reg::kWifiIp + 2) return false;
  if (address >= net_reg::kWifiMac && address < net_reg::kWifiMac + 3) return false;
  if (address >= net_reg::kApSsid && address < net_reg::kApSsid + 16) return false;
  if (address >= net_reg::kApPassword && address < net_reg::kApPassword + 16) return false;
  if (address >= net_reg::kApIp && address < net_reg::kApIp + 2) return false;
  // Everything else in a text field is writable.
  return fieldAt(address, nullptr) != NetField::Count;
}

bool NetRegisterMap::stageWrite(NetSettings& settings, uint16_t address, uint16_t value) {
  if (!contains(address) || !isWritable(address) || address == net_reg::kApply) {
    return false;
  }

  switch (address) {
    case net_reg::kWifiEnabled: return settings.stageWifiEnabled(value != 0);
    case net_reg::kMqttEnabled: return settings.stageMqttEnabled(value != 0);
    case net_reg::kMqttPort:    return settings.stageMqttPort(value);
    case net_reg::kMqttPeriodS: return settings.stageMqttPublishPeriodS(value);
    case net_reg::kPortalReset:
      // A command, not a value: only the magic acts, anything else is ignored rather than treated
      // as a password. The magic requirement is what stops a block write across the region from
      // resetting the login as a side effect.
      if (value == net_reg::kApplyMagic) {
        settings.resetPortalCredentials();
      }
      return true;
    case net_reg::kMqttFlags:
      settings.stageMqttHaDiscovery((value & NetRegisterMap::kFlagHaDiscovery) != 0);
      settings.stageMqttTls((value & NetRegisterMap::kFlagTls) != 0);
      return settings.stageMqttQos((value & NetRegisterMap::kFlagQos1) != 0 ? 1 : 0);
    default:
      break;
  }

  uint16_t offset = 0;
  const NetField field = fieldAt(address, &offset);
  if (field == NetField::Count) {
    return false;
  }

  // Two characters per register, high byte first, patched byte by byte. No string round-trip, so a
  // master may write a field's registers in ANY ORDER — high before low, or only the ones that
  // changed — and the result is the same. A NUL lands as a NUL, which is how a string is shortened.
  const std::size_t first = static_cast<std::size_t>(offset) * 2;
  const bool high = settings.stageByte(field, first, static_cast<char>((value >> 8) & 0xFF));
  const bool low = settings.stageByte(field, first + 1, static_cast<char>(value & 0xFF));
  return high || low;
}

NetApplyError NetRegisterMap::applyWrite(NetSettings& settings, uint16_t value) {
  if (value != net_reg::kApplyMagic) {
    // Refusing anything but the magic is what stops a stray block write from committing a
    // half-staged configuration — the same reasoning as REG_LINK_APPLY.
    return NetApplyError::BadMagic;
  }
  return settings.apply() ? NetApplyError::None : NetApplyError::NothingStaged;
}

void NetRegisterMap::publish(const NetSettings& settings, uint16_t* out, std::size_t count) {
  if (!out) {
    return;
  }
  const std::size_t span = net_reg::kEnd - net_reg::kBase;
  const std::size_t limit = count < span ? count : span;
  for (std::size_t i = 0; i < limit; ++i) {
    out[i] = 0;
  }

  const auto put = [&](uint16_t address, uint16_t value) {
    const std::size_t index = address - net_reg::kBase;
    if (index < limit) {
      out[index] = value;
    }
  };

  put(net_reg::kWifiEnabled, settings.wifiEnabled() ? 1 : 0);
  put(net_reg::kMqttEnabled, settings.mqttEnabled() ? 1 : 0);
  put(net_reg::kMqttPort, settings.mqttPort());
  put(net_reg::kMqttPeriodS, settings.mqttPublishPeriodS());
  // One assembler for this word, shared with the UI's read-modify-write path.
  put(net_reg::kMqttFlags, NetRegisterMap::mqttFlags(settings));
  put(net_reg::kRevision, settings.revision());

  for (std::size_t i = 0; i < static_cast<std::size_t>(NetField::Count); ++i) {
    const auto field = static_cast<NetField>(i);
    const uint16_t base = net_reg::textBase(field);
    const uint16_t span_regs = net_reg::textRegisters(netFieldCapacity(field));
    if (netFieldIsSecret(field)) {
      continue;  // §5.1 — reads as zeros, and the block was already zeroed above
    }
    char buffer[NetSettings::kMaxValueBytes + 1] = {};
    settings.get(field, buffer, sizeof(buffer));
    for (uint16_t r = 0; r < span_regs; ++r) {
      const std::size_t at = static_cast<std::size_t>(r) * 2;
      const char high = buffer[at] != '\0' ? buffer[at] : '\0';
      const char low = high != '\0' && buffer[at + 1] != '\0' ? buffer[at + 1] : '\0';
      put(static_cast<uint16_t>(base + r), packChars(high, low));
      if (high == '\0') {
        break;
      }
    }
  }
}

}  // namespace plc
