#include "net/net_register_map.h"

#include "net/net_status.h"

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
  // Checked HERE rather than inside NetSettings::apply(), because apply() returns bool and this is a
  // THIRD outcome. Folding it in made an invalid topic indistinguishable from "nothing was staged",
  // and ui_settings.cpp deliberately treats NothingStaged as success — so a rejected topic reported
  // "saved" at the display while nothing applied. NetApplyError already had InvalidValue; the enum
  // could tell the truth all along.
  //
  // stageByte() cannot validate: the register path writes two characters at a time, so a topic is
  // transiently invalid while it is being written and refusing mid-write would make §5's documented
  // block-write sequence impossible. apply() is the first moment a WHOLE topic exists.
  if (!settings.stagedTopicFieldsCommittable()) {
    // Drop just the offending field instead of refusing everything. Refusing the whole apply latched
    // the fault: with text uneditable at the panel (§6.3), one bad Modbus write blocked configuration
    // from every surface until a reboot. Reverting the field leaves the device usable and the bad
    // value discarded, and the caller still learns it was rejected.
    // Skip that one field; keep its staged bytes. Reverting it instead would discard a master's
    // partially written topic whenever another surface applied mid-write, and refusing the whole
    // apply would latch the fault. See NetSettings::applyExcept.
    // Skip whichever topic-shaped field is invalid — there are two now, and refusing the whole apply
    // would latch the fault exactly as it did before.
    settings.applyExcept(settings.stagedFieldCommittable(NetField::MqttBaseTopic)
                             ? NetField::MqttDiscoveryPrefix
                             : NetField::MqttBaseTopic);
    return NetApplyError::InvalidValue;
  }
  return settings.apply() ? NetApplyError::None : NetApplyError::NothingStaged;
}

namespace {

/**
 * Packs a NUL-terminated string into consecutive registers, two characters each, high byte first.
 *
 * The same convention `publish` uses for the settings text, written once here because the AP strings
 * are read-only and so cannot go through the `NetField` loop that packs the rest.
 */
template <typename Put>
void putText(const Put& put, uint16_t base, std::size_t capacityBytes, const char* value) {
  const uint16_t registers = net_reg::textRegisters(capacityBytes);
  for (uint16_t r = 0; r < registers; ++r) {
    const std::size_t at = static_cast<std::size_t>(r) * 2;
    const char high = value[at] != '\0' ? value[at] : '\0';
    const char low = high != '\0' && value[at + 1] != '\0' ? value[at + 1] : '\0';
    put(static_cast<uint16_t>(base + r), NetRegisterMap::packChars(high, low));
    if (high == '\0') {
      break;  // the block is already zeroed, so a short string needs no padding written
    }
  }
}

}  // namespace

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

void NetRegisterMap::publishStatus(const NetStatusSnapshot& status, uint16_t* out,
                                   std::size_t count) {
  if (!out) {
    return;
  }
  const std::size_t span = net_reg::kEnd - net_reg::kBase;
  const std::size_t limit = count < span ? count : span;
  const auto put = [&](uint16_t address, uint16_t value) {
    const std::size_t index = address - net_reg::kBase;
    if (index < limit) {
      out[index] = value;
    }
  };
  // NOTE the absence of a zero-fill. `publish` did that; this runs after it and adds to the block,
  // so a caller that forgets the ordering gets settings-only rather than status-only — the failure
  // that was already the status quo, instead of a new one that wipes the settings.

  // 501. The enum IS the wire encoding, pinned by wifi_manager_test's wireEncodingTests: nothing
  // else defines these numbers, because the value does not live in NetSettings.
  put(net_reg::kWifiState, static_cast<uint16_t>(status.wifiState));
  // 502, int16 two's complement. RSSI is negative dBm, so the cast is the encoding, not a shortcut.
  put(net_reg::kWifiRssi, static_cast<uint16_t>(status.rssiDbm));
  // 503-504, high word first. The snapshot already holds `(a<<24)|(b<<16)|(c<<8)|d`, which is the
  // order gen-registers publishes, so splitting it is the whole conversion.
  put(net_reg::kWifiIp, static_cast<uint16_t>(status.ipAddress >> 16));
  put(static_cast<uint16_t>(net_reg::kWifiIp + 1), static_cast<uint16_t>(status.ipAddress & 0xFFFFu));
  // 505-507, two bytes per register, high byte first — the same order as text, and the same helper.
  for (uint16_t r = 0; r < 3; ++r) {
    const std::size_t at = static_cast<std::size_t>(r) * 2;
    put(static_cast<uint16_t>(net_reg::kWifiMac + r),
        packChars(static_cast<char>(status.macAddress[at]),
                  static_cast<char>(status.macAddress[at + 1])));
  }
  // 675. Zero means no portal is open, which is why it cannot be left at zero while one is.
  put(net_reg::kPortalRemainingS, status.portalRemainingS);
  // 676-691 and 692-707. The AP password goes on the bus IN CLEAR, and that is R5.3 rather than an
  // oversight: this describes an access point the device is BROADCASTING, which anyone in radio
  // range already sees, and a remote operator needs it to direct somebody standing at the panel.
  // The operator's own WiFi passphrase is a different thing entirely — `netFieldIsSecret` makes
  // `publish` read it back as zeros, and nothing here changes that.
  // The spans come from the ADDRESSES either side of each field, not from a `NetField` capacity that
  // happens to match today: borrowing `WifiSsid`'s 32 bytes would give one span two homes, and the
  // day somebody grew that field the AP strings would quietly start writing past their window.
  static constexpr std::size_t kApSsidBytes = (net_reg::kApPassword - net_reg::kApSsid) * 2;
  static constexpr std::size_t kApPasswordBytes = (net_reg::kApIp - net_reg::kApPassword) * 2;
  static_assert(kApSsidBytes == 32 && kApPasswordBytes == 32,
                "the AP strings are 32 bytes each; the snapshot's buffers are char[33]");
  putText(put, net_reg::kApSsid, kApSsidBytes, status.apSsid);
  putText(put, net_reg::kApPassword, kApPasswordBytes, status.apPassword);
  // 708-709, high word first, as 503-504.
  put(net_reg::kApIp, static_cast<uint16_t>(status.apIpAddress >> 16));
  put(static_cast<uint16_t>(net_reg::kApIp + 1),
      static_cast<uint16_t>(status.apIpAddress & 0xFFFFu));
}

}  // namespace plc
