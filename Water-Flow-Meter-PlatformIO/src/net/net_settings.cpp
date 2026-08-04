#include "net/net_settings.h"

#include <cstdio>
#include <cstring>
#include <initializer_list>

namespace plc {

namespace {
constexpr std::size_t kFieldCount = static_cast<std::size_t>(NetField::Count);

std::size_t indexOf(NetField field) {
  const auto i = static_cast<std::size_t>(field);
  return i < kFieldCount ? i : kFieldCount;
}
}  // namespace

NetSettings::NetSettings() {
  // §7.9a: the device ships with a known login and must nag until it is changed, so the default
  // has to actually BE set rather than left empty — an empty password would lock nobody out.
  copyInto(live_.text[indexOf(NetField::PortalUser)], netFieldCapacity(NetField::PortalUser),
           kDefaultPortalUser);
  copyInto(live_.text[indexOf(NetField::PortalPassword)],
           netFieldCapacity(NetField::PortalPassword), kDefaultPortalPassword);
  // §4.4's default discovery prefix. Set here rather than in the header so one place owns defaults.
  copyInto(live_.text[indexOf(NetField::MqttDiscoveryPrefix)],
           netFieldCapacity(NetField::MqttDiscoveryPrefix), "homeassistant");
  pending_ = live_;
}

bool NetSettings::copyInto(char* dest, std::size_t capacity, const char* value) {
  if (!dest) {
    return false;
  }
  const std::size_t limit = capacity > kMaxValueBytes ? kMaxValueBytes : capacity;
  std::memset(dest, 0, kMaxValueBytes + 1);
  if (!value) {
    return true;
  }
  std::size_t written = 0;
  while (value[written] != '\0' && written < limit) {
    dest[written] = value[written];
    ++written;
  }
  dest[written] = '\0';
  return true;
}

bool NetSettings::get(NetField field, char* out, std::size_t size) const {
  const std::size_t i = indexOf(field);
  if (i >= kFieldCount || !out || size == 0) {
    return false;
  }
  std::snprintf(out, size, "%s", live_.text[i]);
  return true;
}

bool NetSettings::getStaged(NetField field, char* out, std::size_t size) const {
  const std::size_t i = indexOf(field);
  if (i >= kFieldCount || !out || size == 0) {
    return false;
  }
  std::snprintf(out, size, "%s", pending_.text[i]);
  return true;
}

bool NetSettings::isEmpty(NetField field) const {
  const std::size_t i = indexOf(field);
  return i >= kFieldCount || live_.text[i][0] == '\0';
}

bool NetSettings::portalPasswordIsDefault() const {
  const std::size_t i = indexOf(NetField::PortalPassword);
  return std::strcmp(live_.text[i], kDefaultPortalPassword) == 0;
}

bool NetSettings::stagedBaseTopicCommittable() const {
  const std::size_t i = indexOf(NetField::MqttBaseTopic);
  // The buffer is kMaxValueBytes + 1 and zero-initialised, and stageByte() never writes at or past
  // the capacity, so the tail terminator is always present and reading this as a C string is safe
  // however partially a master has written it.
  return pending_.text[i][0] == '\0' || isValidBaseTopic(pending_.text[i]);
}

bool NetSettings::stage(NetField field, const char* value) {
  const std::size_t i = indexOf(field);
  if (i >= kFieldCount) {
    return false;
  }
  // The base topic is the one field a whole-value write must not truncate, and the one it must not
  // repair. Truncation is right for the others because the damage is visible: a shortened SSID
  // fails to associate and the WiFi page says so. A shortened TOPIC is still a well-formed topic —
  // the device would publish contentedly to "watermeter/plant-3/inl" while Home Assistant watches
  // "watermeter/plant-3/inlet", with nothing logged at either end. Refusing is the only outcome the
  // operator can see.
  //
  // Empty falls through to copyInto, which clears the field: "not configured" is a legitimate
  // request, and it is where §4.2's MAC-derived default takes over.
  //
  // Both whole-value callers already act on this return — ui_settings.cpp's writeSettingText fails
  // the edit, and portal_form.cpp reports PortalFieldError::Refused — so the refusal reaches the
  // operator rather than vanishing.
  if (field == NetField::MqttBaseTopic && value != nullptr && value[0] != '\0' &&
      !isValidBaseTopic(value)) {
    return false;
  }
  // Truncates rather than rejecting: a master writing a full 16-register SSID block sends trailing
  // NUL padding, and refusing that would make the documented write sequence fail.
  return copyInto(pending_.text[i], netFieldCapacity(field), value);
}

bool NetSettings::stageByte(NetField field, std::size_t index, char value) {
  const std::size_t i = indexOf(field);
  if (i >= kFieldCount || index >= netFieldCapacity(field)) {
    return false;
  }
  pending_.text[i][index] = value;
  // The buffer is kMaxValueBytes + 1 and zero-initialised, so the terminator beyond the capacity is
  // always present; a NUL written mid-field simply shortens the string, which is how a master
  // truncates one.
  return true;
}

bool NetSettings::stageWifiEnabled(bool on) {
  pending_.wifiEnabled = on;
  return true;
}

bool NetSettings::stageMqttEnabled(bool on) {
  pending_.mqttEnabled = on;
  return true;
}

bool NetSettings::stageMqttPort(uint16_t port) {
  if (port == 0) {
    return false;  // port 0 is not a port; refusing beats storing something unusable
  }
  pending_.mqttPort = port;
  return true;
}

bool NetSettings::stageMqttPublishPeriodS(uint16_t seconds) {
  if (seconds == 0 || seconds > 3600) {
    return false;
  }
  pending_.mqttPublishPeriodS = seconds;
  return true;
}

bool NetSettings::stageMqttHaDiscovery(bool on) {
  pending_.mqttHaDiscovery = on;
  return true;
}

bool NetSettings::stageMqttQos(uint8_t qos) {
  if (qos > 1) {
    return false;  // §4.2 offers 0 and 1; 2 is not implemented and must not be storable
  }
  pending_.mqttQos = qos;
  return true;
}


bool NetSettings::dirty() const {
  if (std::memcmp(&live_, &pending_, sizeof(Block)) != 0) {
    return true;
  }
  return false;
}

bool NetSettings::apply() {
  if (!dirty()) {
    // Distinguishable from a successful apply on purpose: a master polling the revision can tell
    // whether its write took effect or whether it wrote the values that were already live.
    return false;
  }
  // The backstop for stageByte(), which cannot validate anything. That path writes ONE BYTE at a
  // time, so a topic is transiently invalid while it is being written: an in-order block write of
  // "wfm/plant3" leaves the field reading "wfm/" the moment register 635 lands, and refusing there
  // would make the documented §5 write sequence impossible. apply() is the only moment at which a
  // WHOLE topic exists, so it is the only place the rule can be enforced for that surface.
  //
  // Refuses the ENTIRE apply rather than promoting the other fields and dropping this one. A partial
  // apply is precisely the silent failure this validator exists to remove: the operator would watch
  // the SSID take effect and reasonably conclude the topic had too. Pending is left intact, so
  // correcting the topic and re-applying costs nothing else.
  if (!stagedBaseTopicCommittable()) {
    return false;
  }
  live_ = pending_;
  ++revision_;
  return true;
}

void NetSettings::revert() { pending_ = live_; }

void NetSettings::resetPortalCredentials() {
  const std::size_t user = indexOf(NetField::PortalUser);
  const std::size_t pass = indexOf(NetField::PortalPassword);
  for (Block* block : {&live_, &pending_}) {
    copyInto(block->text[user], netFieldCapacity(NetField::PortalUser), kDefaultPortalUser);
    copyInto(block->text[pass], netFieldCapacity(NetField::PortalPassword), kDefaultPortalPassword);
  }
  // Bumped even though nothing else changed, so a master polling the revision can tell the command
  // landed. portalPasswordIsDefault() now reports true again, which is what re-raises the §7.9a nag.
  ++revision_;
}

}  // namespace plc
