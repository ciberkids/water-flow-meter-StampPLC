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

bool NetSettings::fieldIsTopicShaped(NetField field) {
  // Both become published topics AND, for the prefix, a SUBSCRIBED one (`<prefix>/status`, R4.4.7),
  // so both must satisfy the same rule. The prefix was validated only inside HaDiscovery::configure,
  // which meant an invalid one staged cleanly, went live, and was then refused deep in a module the
  // operator cannot see — no entity ever appeared and nothing said why. 5A's principle is that a
  // refusal must be REPORTABLE, and a refusal is only reportable where the value is entered.
  return field == NetField::MqttBaseTopic || field == NetField::MqttDiscoveryPrefix;
}

bool NetSettings::stagedTopicFieldsCommittable() const {
  for (std::size_t f = 0; f < kFieldCount; ++f) {
    const auto field = static_cast<NetField>(f);
    if (!fieldIsTopicShaped(field)) continue;
    if (!stagedFieldCommittable(field)) return false;
  }
  return true;
}

bool NetSettings::stagedFieldCommittable(NetField field) const {
  const std::size_t i = indexOf(field);
  if (i >= kFieldCount || !fieldIsTopicShaped(field)) {
    return true;
  }
  // The buffer is kMaxValueBytes + 1 and zero-initialised, and stageByte() never writes at or past
  // the capacity, so the tail terminator is always present and reading this as a C string is safe
  // however partially a master has written it.
  return pending_.text[i][0] == '\0' || isValidBaseTopic(pending_.text[i]);
}

bool NetSettings::stagedBaseTopicCommittable() const {
  return stagedFieldCommittable(NetField::MqttBaseTopic);
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
  // Both topic-shaped fields, not just the base topic. Refusing HERE is what makes the refusal
  // visible: ui_settings.cpp fails the edit, portal_form.cpp reports PortalFieldError::Refused, and a
  // master gets false. Refusing only later, inside HaDiscovery, produced a device with no Home
  // Assistant entities and no explanation anywhere.
  if (fieldIsTopicShaped(field) && value != nullptr && value[0] != '\0' &&
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
  live_ = pending_;
  ++revision_;
  return true;
}

void NetSettings::revert() { pending_ = live_; }

bool NetSettings::applyExcept(NetField field) {
  const std::size_t skip = indexOf(field);
  if (skip >= kFieldCount) {
    return apply();
  }
  char keep[kMaxValueBytes + 1] = {};
  std::memcpy(keep, pending_.text[skip], sizeof(keep));
  // Make the excluded field a no-op for this promotion by matching live, apply, then put the
  // caller's bytes back so an in-progress block write is not lost.
  std::memcpy(pending_.text[skip], live_.text[skip], sizeof(keep));
  const bool promoted = apply();
  std::memcpy(pending_.text[skip], keep, sizeof(keep));
  return promoted;
}

void NetSettings::revertField(NetField field) {
  const std::size_t i = indexOf(field);
  if (i >= kFieldCount) {
    return;
  }
  std::memcpy(pending_.text[i], live_.text[i], kMaxValueBytes + 1);
}

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
