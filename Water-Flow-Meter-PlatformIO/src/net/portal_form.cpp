#include "net/portal_form.h"

#include <cstdio>
#include <cstring>

namespace plc {

namespace {

constexpr std::size_t kNameBytes = PortalSubmitResult::kMaxFieldNameBytes;
constexpr std::size_t kTextBytes = NetSettings::kMaxValueBytes + 1;

/** The entity for one HTML metacharacter, or null when the character passes through. */
const char* htmlEntity(char c) {
  switch (c) {
    case '&':  return "&amp;";
    case '<':  return "&lt;";
    case '>':  return "&gt;";
    case '"':  return "&quot;";
    case '\'': return "&#39;";
    default:   return nullptr;
  }
}

int hexValue(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return -1;
}

char lowerCase(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c + 32) : c; }

bool equalsNoCase(const char* a, const char* b) {
  while (*a != '\0' && *b != '\0') {
    if (lowerCase(*a) != lowerCase(*b)) return false;
    ++a;
    ++b;
  }
  return *a == '\0' && *b == '\0';
}

enum class DecodeStatus : uint8_t { Ok, Overflow, Bad };

/**
 * Percent-decodes `[begin, end)` into a NUL-terminated `out`.
 *
 * `+` is a space (the form-encoding rule, not the URI rule) and `%2B` is a literal plus, which is
 * why this cannot be skipped for "simple" fields: a WPA2 passphrase containing `+` arrives one way
 * and a topic containing a space the other.
 */
DecodeStatus urlDecode(const char* begin, const char* end, char* out, std::size_t size) {
  if (!out || size == 0) return DecodeStatus::Overflow;
  out[0] = '\0';
  std::size_t written = 0;
  for (const char* p = begin; p != end; ++p) {
    char decoded = *p;
    if (*p == '+') {
      decoded = ' ';
    } else if (*p == '%') {
      if (end - p < 3) return DecodeStatus::Bad;  // an escape cut off by the end of the body
      const int high = hexValue(p[1]);
      const int low = hexValue(p[2]);
      if (high < 0 || low < 0) return DecodeStatus::Bad;
      const int byte = high * 16 + low;
      // %00 is refused rather than dropped. Everything downstream is a NUL-terminated buffer, so a
      // decoded NUL would truncate the value silently — "psk=secret%00xxxxx" would pass a length
      // check and store "secret". That is a validation bypass, not a cosmetic problem.
      if (byte == 0) return DecodeStatus::Bad;
      decoded = static_cast<char>(byte);
      p += 2;
    }
    if (written + 1 >= size) return DecodeStatus::Overflow;
    out[written++] = decoded;
  }
  out[written] = '\0';
  return DecodeStatus::Ok;
}

/**
 * Strict integer parse.
 *
 * Not atoi/strtol: atoi has undefined behaviour on overflow and both accept trailing rubbish, so
 * `"8"` and `"8; DROP"` and `"99999999999999999999"` would all become 8-ish. Every non-conforming
 * input has to be distinguishable from a valid one, because the alternative is a silently wrong
 * broker port.
 */
bool parseInt64(const char* text, int64_t& out) {
  if (!text || *text == '\0') return false;
  const char* p = text;
  bool negative = false;
  if (*p == '+' || *p == '-') {
    negative = (*p == '-');
    ++p;
  }
  if (*p == '\0') return false;
  constexpr uint64_t kLimit = 9223372036854775807ULL;
  uint64_t magnitude = 0;
  for (; *p != '\0'; ++p) {
    if (*p < '0' || *p > '9') return false;
    const uint64_t digit = static_cast<uint64_t>(*p - '0');
    if (magnitude > (kLimit - digit) / 10) return false;  // refused, never wrapped
    magnitude = magnitude * 10 + digit;
  }
  out = negative ? -static_cast<int64_t>(magnitude) : static_cast<int64_t>(magnitude);
  return true;
}

/**
 * Accepts the three spellings a boolean actually arrives as.
 *
 * The generated form sends `0`/`1` (see the hidden companion in renderRow), a hand-written
 * checkbox sends `on`, and someone driving this with curl writes `true`. Anything else is a mistake
 * rather than something to guess at: silently reading "yes" as false is worse than an error.
 */
bool parseBoolean(const char* text, bool& out) {
  if (!text) return false;
  if (equalsNoCase(text, "1") || equalsNoCase(text, "on") || equalsNoCase(text, "true")) {
    out = true;
    return true;
  }
  if (equalsNoCase(text, "0") || equalsNoCase(text, "off") || equalsNoCase(text, "false")) {
    out = false;
    return true;
  }
  return false;
}

/**
 * Control characters are refused in text values.
 *
 * These strings become MQTT topics and discovery payloads (§4.2, §4.4), and a CR or LF in a base
 * topic is a protocol-injection vector rather than a typo. Bytes >= 0x80 are left alone: UTF-8 in
 * an SSID is legitimate.
 */
bool hasControlCharacter(const char* text) {
  for (const char* p = text; *p != '\0'; ++p) {
    const unsigned char c = static_cast<unsigned char>(*p);
    if (c < 0x20 || c == 0x7F) return true;
  }
  return false;
}

/** Where a catalogue setting's value actually lives. */
struct Storage {
  enum class Kind : uint8_t { NetText, NetScalar, External };
  Kind kind = Kind::External;
  NetField textField = NetField::Count;
};

/**
 * Classifies every SettingTarget. Exhaustive, with NO `default:` on purpose.
 *
 * Deliberately not delegating to `ui::netFieldFor`: that one has a `default:` arm — correct for its
 * caller, wrong here, because a new network text setting would fall through it and be classified as
 * an external numeric, i.e. rendered and then written to the wrong store. And ui_settings.h reaches
 * ModbusManager and LedController, which would drag Arduino into this translation unit.
 *
 * The cost of the duplication is one switch; the benefit is that adding a SettingTarget is a
 * -Wswitch build failure here until somebody says where the portal should send it.
 */
Storage storageFor(ui::SettingTarget target) {
  switch (target) {
    case ui::SettingTarget::WifiSsid:
      return Storage{Storage::Kind::NetText, NetField::WifiSsid};
    case ui::SettingTarget::WifiPsk:
      return Storage{Storage::Kind::NetText, NetField::WifiPsk};
    case ui::SettingTarget::MqttHost:
      return Storage{Storage::Kind::NetText, NetField::MqttHost};
    case ui::SettingTarget::MqttUser:
      return Storage{Storage::Kind::NetText, NetField::MqttUser};
    case ui::SettingTarget::MqttPassword:
      return Storage{Storage::Kind::NetText, NetField::MqttPassword};
    case ui::SettingTarget::MqttBaseTopic:
      return Storage{Storage::Kind::NetText, NetField::MqttBaseTopic};
    case ui::SettingTarget::MqttDiscoveryPrefix:
      return Storage{Storage::Kind::NetText, NetField::MqttDiscoveryPrefix};

    case ui::SettingTarget::WifiEnabled:
    case ui::SettingTarget::MqttEnabled:
    case ui::SettingTarget::MqttPort:
    case ui::SettingTarget::MqttPublishPeriod:
    case ui::SettingTarget::MqttHaDiscovery:
    case ui::SettingTarget::MqttQos:
      return Storage{Storage::Kind::NetScalar, NetField::Count};

    case ui::SettingTarget::LinkSlaveId:
    case ui::SettingTarget::LinkBaudIndex:
    case ui::SettingTarget::LinkParity:
    case ui::SettingTarget::LinkStopBits:
    case ui::SettingTarget::LedVolumeStep:
    case ui::SettingTarget::LedPulsePeriod:
    case ui::SettingTarget::SensorConnected:
    case ui::SettingTarget::SensorMultiplier:
    case ui::SettingTarget::SensorAdjust:
    case ui::SettingTarget::SensorMaxFlow:
    // The calibration pair joins the other per-sensor settings: the portal does not serve them, the
    // sensor register block does. Grouped here rather than defaulted, which is the point of the
    // -Wswitch gate this file's header describes — it failed the build until somebody said where they
    // belong, and this is that answer.
    case ui::SettingTarget::SensorCalibrationType:
    case ui::SettingTarget::SensorPulsesPerLitre:
      return Storage{Storage::Kind::External, NetField::Count};
  }
  return Storage{};
}

/** Live value of a network scalar, for pre-filling the form. False for anything else. */
bool readNetScalar(const NetSettings& net, ui::SettingTarget target, int32_t& out) {
  switch (target) {
    case ui::SettingTarget::WifiEnabled:       out = net.wifiEnabled() ? 1 : 0; return true;
    case ui::SettingTarget::MqttEnabled:       out = net.mqttEnabled() ? 1 : 0; return true;
    case ui::SettingTarget::MqttPort:          out = net.mqttPort(); return true;
    case ui::SettingTarget::MqttPublishPeriod: out = net.mqttPublishPeriodS(); return true;
    case ui::SettingTarget::MqttHaDiscovery:   out = net.mqttHaDiscovery() ? 1 : 0; return true;
    case ui::SettingTarget::MqttQos:           out = net.mqttQos(); return true;

    case ui::SettingTarget::WifiSsid:
    case ui::SettingTarget::WifiPsk:
    case ui::SettingTarget::MqttHost:
    case ui::SettingTarget::MqttUser:
    case ui::SettingTarget::MqttPassword:
    case ui::SettingTarget::MqttBaseTopic:
    case ui::SettingTarget::MqttDiscoveryPrefix:
    case ui::SettingTarget::LinkSlaveId:
    case ui::SettingTarget::LinkBaudIndex:
    case ui::SettingTarget::LinkParity:
    case ui::SettingTarget::LinkStopBits:
    case ui::SettingTarget::LedVolumeStep:
    case ui::SettingTarget::LedPulsePeriod:
    case ui::SettingTarget::SensorConnected:
    case ui::SettingTarget::SensorMultiplier:
    case ui::SettingTarget::SensorAdjust:
    case ui::SettingTarget::SensorMaxFlow:
    // The calibration pair joins the other per-sensor settings: the portal does not serve them, the
    // sensor register block does. Grouped here rather than defaulted, which is the point of the
    // -Wswitch gate this file's header describes — it failed the build until somebody said where they
    // belong, and this is that answer.
    case ui::SettingTarget::SensorCalibrationType:
    case ui::SettingTarget::SensorPulsesPerLitre:
      return false;
  }
  return false;
}

/** Stages a network scalar. The apply is the caller's, once, for the whole submission (R7.11). */
bool stageNetScalar(NetSettings& net, ui::SettingTarget target, int32_t value) {
  switch (target) {
    case ui::SettingTarget::WifiEnabled:  return net.stageWifiEnabled(value != 0);
    case ui::SettingTarget::MqttEnabled:  return net.stageMqttEnabled(value != 0);
    case ui::SettingTarget::MqttPort:     return net.stageMqttPort(static_cast<uint16_t>(value));
    case ui::SettingTarget::MqttPublishPeriod:
      return net.stageMqttPublishPeriodS(static_cast<uint16_t>(value));
    case ui::SettingTarget::MqttHaDiscovery: return net.stageMqttHaDiscovery(value != 0);
    case ui::SettingTarget::MqttQos:         return net.stageMqttQos(static_cast<uint8_t>(value));

    case ui::SettingTarget::WifiSsid:
    case ui::SettingTarget::WifiPsk:
    case ui::SettingTarget::MqttHost:
    case ui::SettingTarget::MqttUser:
    case ui::SettingTarget::MqttPassword:
    case ui::SettingTarget::MqttBaseTopic:
    case ui::SettingTarget::MqttDiscoveryPrefix:
    case ui::SettingTarget::LinkSlaveId:
    case ui::SettingTarget::LinkBaudIndex:
    case ui::SettingTarget::LinkParity:
    case ui::SettingTarget::LinkStopBits:
    case ui::SettingTarget::LedVolumeStep:
    case ui::SettingTarget::LedPulsePeriod:
    case ui::SettingTarget::SensorConnected:
    case ui::SettingTarget::SensorMultiplier:
    case ui::SettingTarget::SensorAdjust:
    case ui::SettingTarget::SensorMaxFlow:
    // The calibration pair joins the other per-sensor settings: the portal does not serve them, the
    // sensor register block does. Grouped here rather than defaulted, which is the point of the
    // -Wswitch gate this file's header describes — it failed the build until somebody said where they
    // belong, and this is that answer.
    case ui::SettingTarget::SensorCalibrationType:
    case ui::SettingTarget::SensorPulsesPerLitre:
      return false;
  }
  return false;
}

bool isOption(const ui::SettingDescriptor& setting, int32_t value) {
  for (uint8_t i = 0; i < setting.optionCount; ++i) {
    if (setting.options && setting.options[i].value == value) return true;
  }
  return false;
}

/** `config.mqtt.host` -> `config.mqtt`, for the section headings. Purely presentational. */
void groupOf(const char* bindingId, char* out, std::size_t size) {
  const char* lastDot = nullptr;
  for (const char* p = bindingId; *p != '\0'; ++p) {
    if (*p == '.') lastDot = p;
  }
  const std::size_t length = lastDot ? static_cast<std::size_t>(lastDot - bindingId)
                                     : std::strlen(bindingId);
  const std::size_t limit = length + 1 < size ? length : size - 1;
  std::memcpy(out, bindingId, limit);
  out[limit] = '\0';
}

void writeNumber(PortalSink& out, long value) {
  char text[24] = {};
  std::snprintf(text, sizeof(text), "%ld", value);
  out.writeText(text);
}

/** `config.sensor.multiplier` + sensor 3 -> `config.sensor.multiplier@3`. */
void fieldName(const ui::SettingDescriptor& setting,
               uint8_t sensorIndex,
               bool perSensor,
               char* out,
               std::size_t size) {
  if (perSensor) {
    std::snprintf(out, size, "%s@%u", setting.bindingId, static_cast<unsigned>(sensorIndex));
  } else {
    std::snprintf(out, size, "%s", setting.bindingId);
  }
}

int base64Value(char c) {
  if (c >= 'A' && c <= 'Z') return c - 'A';
  if (c >= 'a' && c <= 'z') return c - 'a' + 26;
  if (c >= '0' && c <= '9') return c - '0' + 52;
  if (c == '+') return 62;
  if (c == '/') return 63;
  return -1;
}

bool base64Decode(const char* begin, const char* end, char* out, std::size_t size) {
  if (!out || size == 0) return false;
  out[0] = '\0';
  std::size_t written = 0;
  uint32_t accumulator = 0;
  int bits = 0;
  for (const char* p = begin; p != end; ++p) {
    if (*p == '=') break;  // padding, and everything after it is padding too
    const int value = base64Value(*p);
    if (value < 0) return false;
    accumulator = (accumulator << 6) | static_cast<uint32_t>(value);
    bits += 6;
    if (bits >= 8) {
      bits -= 8;
      const char byte = static_cast<char>((accumulator >> bits) & 0xFF);
      if (byte == '\0') return false;  // a NUL cannot be part of a credential pair
      if (written + 1 >= size) return false;
      out[written++] = byte;
    }
  }
  out[written] = '\0';
  return written > 0;
}

}  // namespace

void PortalSink::writeText(const char* text) {
  if (!text) return;
  writeBytes(text, std::strlen(text));
}

bool portalEscapeHtml(const char* in, char* out, std::size_t size) {
  if (!out || size == 0) return false;
  out[0] = '\0';
  if (!in) return true;
  std::size_t written = 0;
  for (const char* p = in; *p != '\0'; ++p) {
    const char* entity = htmlEntity(*p);
    const std::size_t length = entity ? std::strlen(entity) : 1;
    if (written + length + 1 > size) {
      out[written] = '\0';  // truncated, but never half an entity
      return false;
    }
    if (entity) {
      std::memcpy(out + written, entity, length);
    } else {
      out[written] = *p;
    }
    written += length;
  }
  out[written] = '\0';
  return true;
}

void portalWriteEscaped(PortalSink& out, const char* text) {
  if (!text) return;
  char chunk[96] = {};
  std::size_t written = 0;
  for (const char* p = text; *p != '\0'; ++p) {
    const char* entity = htmlEntity(*p);
    const std::size_t length = entity ? std::strlen(entity) : 1;
    if (written + length > sizeof(chunk)) {
      out.writeBytes(chunk, written);
      written = 0;
    }
    if (entity) {
      std::memcpy(chunk + written, entity, length);
    } else {
      chunk[written] = *p;
    }
    written += length;
  }
  if (written > 0) out.writeBytes(chunk, written);
}

bool portalConstantTimeEquals(const char* a, const char* b) {
  if (!a || !b) return false;
  const std::size_t lengthA = std::strlen(a);
  const std::size_t lengthB = std::strlen(b);
  // Length inequality is folded in rather than returned early, so "right length, wrong content" and
  // "wrong length" take the same path.
  unsigned difference = static_cast<unsigned>(lengthA ^ lengthB);
  for (std::size_t i = 0; i < NetSettings::kMaxValueBytes; ++i) {
    const unsigned char byteA = i < lengthA ? static_cast<unsigned char>(a[i]) : 0u;
    const unsigned char byteB = i < lengthB ? static_cast<unsigned char>(b[i]) : 0u;
    difference |= static_cast<unsigned>(byteA ^ byteB);
  }
  return difference == 0;
}

const char* portalFieldErrorText(PortalFieldError error) {
  switch (error) {
    case PortalFieldError::None:          return "accepted";
    case PortalFieldError::UnknownField:  return "not a setting this firmware has";
    case PortalFieldError::BadEncoding:   return "badly encoded, or contains a control character";
    case PortalFieldError::TooLong:       return "longer than this field allows";
    case PortalFieldError::NotANumber:    return "not a number";
    case PortalFieldError::OutOfRange:    return "outside the allowed range";
    case PortalFieldError::UnknownOption: return "not one of the allowed choices";
    case PortalFieldError::Refused:       return "refused by the device";
  }
  return "refused";
}

PortalForm::PortalForm(NetSettings& net, PortalSettingStore* store, std::size_t sensorCount)
    : net_(net), store_(store), sensorCount_(sensorCount) {}

bool PortalForm::warningRequired() const { return net_.portalPasswordIsDefault(); }

bool PortalForm::authorize(const char* authorizationHeader) const {
  if (!authorizationHeader) return false;
  const char* p = authorizationHeader;
  while (*p == ' ' || *p == '\t') ++p;

  // RFC 7235 makes the scheme token case-insensitive, and browsers do vary.
  static const char kScheme[] = "basic";
  for (std::size_t i = 0; i < sizeof(kScheme) - 1; ++i) {
    if (lowerCase(p[i]) != kScheme[i]) return false;  // a NUL never matches, so this cannot run off
  }
  p += sizeof(kScheme) - 1;
  if (*p != ' ' && *p != '\t') return false;
  while (*p == ' ' || *p == '\t') ++p;

  const char* end = p;
  while (*end != '\0' && *end != ' ' && *end != '\t' && *end != '\r' && *end != '\n') ++end;

  char decoded[kMaxAuthBytes] = {};
  if (!base64Decode(p, end, decoded, sizeof(decoded))) return false;

  // Split at the FIRST colon: RFC 7617 forbids one in the user-id and says nothing about the
  // password, so `admin:pa:ss` is a valid pair and splitting at the last colon would break it.
  const char* colon = std::strchr(decoded, ':');
  if (!colon) return false;
  const std::size_t userLength = static_cast<std::size_t>(colon - decoded);
  char user[kTextBytes] = {};
  if (userLength >= sizeof(user)) return false;
  std::memcpy(user, decoded, userLength);

  char storedUser[kTextBytes] = {};
  char storedPassword[kTextBytes] = {};
  net_.get(NetField::PortalUser, storedUser, sizeof(storedUser));
  net_.get(NetField::PortalPassword, storedPassword, sizeof(storedPassword));

  // Both comparisons run unconditionally. `userOk && passwordOk` as one expression would let the
  // compiler skip the second whenever the first fails, and "wrong user" would then answer measurably
  // faster than "right user, wrong password" — which tells an attacker they have found the user name.
  const bool userOk = portalConstantTimeEquals(user, storedUser);
  const bool passwordOk = portalConstantTimeEquals(colon + 1, storedPassword);
  return userOk && passwordOk;
}

// ── Rendering ───────────────────────────────────────────────────────────────────────

void PortalForm::renderDocumentStart(PortalSink& out, const char* title) const {
  out.writeText(
      "<!doctype html><html lang=\"en\"><head><meta charset=\"utf-8\">"
      "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>");
  portalWriteEscaped(out, title ? title : "Water Flow Meter");
  out.writeText(
      "</title><style>"
      "body{font-family:system-ui,sans-serif;margin:0;padding:1rem;max-width:44rem;"
      "background:#111;color:#eee}"
      "h1{font-size:1.2rem}h2{font-size:1rem;margin:1.5rem 0 .5rem;border-bottom:1px solid #444}"
      ".row{margin:.5rem 0}label{display:block;font-weight:600;font-size:.85rem}"
      "input,select{width:100%;box-sizing:border-box;padding:.35rem;background:#222;color:#eee;"
      "border:1px solid #555;border-radius:3px}"
      "input[type=checkbox]{width:auto}"
      ".hint{margin:.15rem 0 0;font-size:.75rem;color:#999}"
      ".warn{background:#5a1a1a;border:1px solid #d33;padding:.6rem;border-radius:4px}"
      ".err{background:#5a1a1a;border:1px solid #d33;padding:.6rem;border-radius:4px}"
      ".ok{background:#1a4a1a;border:1px solid #3d3;padding:.6rem;border-radius:4px}"
      "button{margin:1rem 0;padding:.5rem 1rem}"
      "code{color:#fc9}"
      "</style></head><body><h1>");
  portalWriteEscaped(out, title ? title : "Water Flow Meter");
  out.writeText("</h1>");
  // R7.9a: the nag is emitted by the document opener rather than by each page, so a page added
  // later cannot forget it.
  renderDefaultPasswordWarning(out);
}

void PortalForm::renderDefaultPasswordWarning(PortalSink& out) const {
  if (!warningRequired()) return;
  // R7.9a is explicit that this is not merely changeable but must be changed, and that it is
  // impossible to miss. It names the risk rather than saying "warning".
  out.writeText(
      "<div class=\"warn\"><strong>This device still uses the default password.</strong>"
      " Every host on this network can read and change its configuration, including the WiFi"
      " passphrase and the broker credentials. Set a portal password below."
      " The page is served over plain HTTP (R7.10) &mdash; do not port-forward this device.</div>");
}

void PortalForm::renderDocumentEnd(PortalSink& out) const { out.writeText("</body></html>"); }

void PortalForm::renderTextRow(PortalSink& out,
                               const char* name,
                               const char* label,
                               const char* hint,
                               const char* value,
                               std::size_t capacity,
                               bool writeOnly,
                               bool disabled) const {
  out.writeText("<div class=\"row\"><label for=\"");
  portalWriteEscaped(out, name);
  out.writeText("\">");
  portalWriteEscaped(out, label);
  out.writeText("</label><input id=\"");
  portalWriteEscaped(out, name);
  out.writeText("\" name=\"");
  portalWriteEscaped(out, name);
  // autocomplete off on the secrets: a browser that helpfully fills the portal login into the WiFi
  // passphrase field would overwrite a working passphrase on the next save.
  out.writeText(writeOnly ? "\" type=\"password\" autocomplete=\"new-password\" value=\"\""
                          : "\" type=\"text\" value=\"");
  if (!writeOnly) {
    // §8.1 in one line: a writeOnly field is never rendered with its stored value, so there is no
    // branch here that could ever emit one.
    portalWriteEscaped(out, value);
    out.writeText("\"");
  }
  out.writeText(" maxlength=\"");
  writeNumber(out, static_cast<long>(capacity));
  out.writeText("\"");
  if (writeOnly) out.writeText(" placeholder=\"unchanged\"");
  if (disabled) out.writeText(" disabled");
  out.writeText("><p class=\"hint\">");
  portalWriteEscaped(out, hint);
  if (writeOnly) {
    out.writeText(" Leave blank to keep the stored value.");
  }
  out.writeText("</p></div>");
}

void PortalForm::renderRow(PortalSink& out,
                           const ui::SettingDescriptor& setting,
                           uint8_t sensorIndex,
                           bool perSensor) const {
  char name[kNameBytes] = {};
  fieldName(setting, sensorIndex, perSensor, name, sizeof(name));

  const Storage storage = storageFor(setting.target);

  if (setting.kind == ui::SettingKind::Text) {
    char value[kTextBytes] = {};
    std::size_t capacity = setting.maxLength;
    bool disabled = true;
    if (storage.kind == Storage::Kind::NetText) {
      disabled = false;
      const std::size_t stored = netFieldCapacity(storage.textField);
      if (stored < capacity) capacity = stored;
      if (!setting.writeOnly) net_.get(storage.textField, value, sizeof(value));
    }
    renderTextRow(out, name, setting.bindingId, setting.description, value, capacity,
                  setting.writeOnly, disabled);
    return;
  }

  int32_t current = 0;
  bool have = false;
  switch (storage.kind) {
    case Storage::Kind::NetScalar:
      have = readNetScalar(net_, setting.target, current);
      break;
    case Storage::Kind::External:
      have = store_ != nullptr && store_->readValue(setting, sensorIndex, current);
      break;
    case Storage::Kind::NetText:
      break;  // handled above; a non-Text setting cannot be net text
  }
  // A control with no readable value renders DISABLED rather than being omitted. A browser does not
  // submit a disabled input, so an absent store degrades to a read-only row — whereas an omitted row
  // is indistinguishable from a setting the firmware does not have, which is exactly the confusion
  // R7.9c generates the form to prevent.
  const bool disabled = !have;

  out.writeText("<div class=\"row\"><label for=\"");
  portalWriteEscaped(out, name);
  out.writeText("\">");
  portalWriteEscaped(out, setting.bindingId);
  out.writeText("</label>");

  switch (setting.kind) {
    case ui::SettingKind::Boolean: {
      // The hidden companion carries "0" for the same field name, so an UNCHECKED box still submits
      // something. Without it, "unchecked" and "this form never carried the field" are the same wire
      // representation, and a partial POST would silently switch off every boolean it omitted.
      // submit() treats any truthy occurrence as true, so the order of the two is irrelevant.
      out.writeText("<input type=\"hidden\" name=\"");
      portalWriteEscaped(out, name);
      out.writeText("\" value=\"0\"><input id=\"");
      portalWriteEscaped(out, name);
      out.writeText("\" name=\"");
      portalWriteEscaped(out, name);
      out.writeText("\" type=\"checkbox\" value=\"1\"");
      if (have && current != 0) out.writeText(" checked");
      if (disabled) out.writeText(" disabled");
      out.writeText(">");
      break;
    }
    case ui::SettingKind::Enum: {
      out.writeText("<select id=\"");
      portalWriteEscaped(out, name);
      out.writeText("\" name=\"");
      portalWriteEscaped(out, name);
      out.writeText("\"");
      if (disabled) out.writeText(" disabled");
      out.writeText(">");
      for (uint8_t i = 0; i < setting.optionCount; ++i) {
        if (!setting.options) break;
        out.writeText("<option value=\"");
        writeNumber(out, static_cast<long>(setting.options[i].value));
        out.writeText("\"");
        if (have && setting.options[i].value == current) out.writeText(" selected");
        out.writeText(">");
        portalWriteEscaped(out, setting.options[i].label);
        out.writeText("</option>");
      }
      out.writeText("</select>");
      break;
    }
    case ui::SettingKind::Numeric: {
      out.writeText("<input id=\"");
      portalWriteEscaped(out, name);
      out.writeText("\" name=\"");
      portalWriteEscaped(out, name);
      out.writeText("\" type=\"number\" min=\"");
      writeNumber(out, static_cast<long>(setting.min));
      out.writeText("\" max=\"");
      writeNumber(out, static_cast<long>(setting.max));
      out.writeText("\" step=\"");
      writeNumber(out, static_cast<long>(setting.step == 0 ? 1 : setting.step));
      out.writeText("\" value=\"");
      if (have) writeNumber(out, static_cast<long>(current));
      out.writeText("\"");
      if (disabled) out.writeText(" disabled");
      out.writeText(">");
      break;
    }
    case ui::SettingKind::Text:
      break;  // returned above
  }

  out.writeText("<p class=\"hint\">");
  portalWriteEscaped(out, setting.description);
  if (setting.unit) {
    out.writeText(" (");
    portalWriteEscaped(out, setting.unit);
    out.writeText(")");
  }
  if (disabled) out.writeText(" &mdash; not readable right now, so it cannot be set here.");
  out.writeText("</p></div>");
}

void PortalForm::renderSettingsForm(PortalSink& out) const {
  out.writeText("<form method=\"post\" action=\"");
  portalWriteEscaped(out, kFormAction);
  out.writeText("\">");

  // The portal login comes FIRST. R7.9a wants the login to land on the change-password form rather
  // than on a status page, and these two fields have no catalogue descriptor to generate from — see
  // kPortalUserField in the header.
  out.writeText("<h2>Portal login</h2>");
  char portalUser[kTextBytes] = {};
  net_.get(NetField::PortalUser, portalUser, sizeof(portalUser));
  renderTextRow(out, kPortalUserField, kPortalUserField, "Login name for this page", portalUser,
                netFieldCapacity(NetField::PortalUser), false, false);
  renderTextRow(out, kPortalPasswordField, kPortalPasswordField, "Login password for this page", "",
                netFieldCapacity(NetField::PortalPassword), true, false);

  char currentGroup[48] = {};
  for (std::size_t i = 0; i < ui::settingCount(); ++i) {
    const ui::SettingDescriptor* setting = ui::settingAt(i);
    if (!setting) continue;

    char group[48] = {};
    groupOf(setting->bindingId, group, sizeof(group));
    if (std::strcmp(group, currentGroup) != 0) {
      std::snprintf(currentGroup, sizeof(currentGroup), "%s", group);
      out.writeText("<h2>");
      portalWriteEscaped(out, currentGroup);
      out.writeText("</h2>");
    }

    if (setting->perSensor) {
      for (std::size_t sensor = 0; sensor < sensorCount_; ++sensor) {
        renderRow(out, *setting, static_cast<uint8_t>(sensor), true);
      }
    } else {
      renderRow(out, *setting, 0, false);
    }
  }

  out.writeText("<button type=\"submit\">Save</button></form>");
}

void PortalForm::renderResult(PortalSink& out, const PortalSubmitResult& result) const {
  if (result.ok()) {
    out.writeText("<p class=\"ok\">");
    // Both sinks, not just NetSettings: a submission carrying only a link or calibration setting
    // commits nothing to the network block and has still changed the device.
    if (result.storedSomething()) {
      out.writeText("Saved ");
      writeNumber(out, static_cast<long>(result.fieldsAccepted));
      out.writeText(" field(s).");
    } else {
      out.writeText("Nothing changed &mdash; every submitted value was already stored.");
    }
    out.writeText("</p>");
    return;
  }

  out.writeText("<div class=\"err\"><p>");
  if (result.partiallyApplied) {
    // Stated rather than smoothed over: the operator has to know the device is in a state neither
    // the old nor the new configuration describes. Deliberately does not name WHICH values landed —
    // that depends on which sink took them, and a wrong specific claim is worse than a vague true one.
    out.writeText("<strong>Partly saved.</strong> Some values were stored, then the device "
                  "refused these:");
  } else {
    out.writeText("<strong>Nothing was saved.</strong> Fix these and submit again:");
  }
  out.writeText("</p><ul>");
  for (std::size_t i = 0; i < result.errorCount; ++i) {
    out.writeText("<li><code>");
    // The field name is whatever was submitted, so for an unknown field it is attacker-controlled
    // and this is a reflected sink. Escaped for exactly that reason.
    portalWriteEscaped(out, result.errors[i].field);
    out.writeText("</code> &mdash; ");
    portalWriteEscaped(out, portalFieldErrorText(result.errors[i].error));
    out.writeText("</li>");
  }
  out.writeText("</ul>");
  if (result.moreErrors) out.writeText("<p>&hellip; and more.</p>");
  out.writeText("</div>");
}

void PortalForm::renderSettingsPage(PortalSink& out) const {
  renderDocumentStart(out, "Water Flow Meter setup");
  renderSettingsForm(out);
  renderDocumentEnd(out);
}

void PortalForm::renderSubmitPage(PortalSink& out, const PortalSubmitResult& result) const {
  renderDocumentStart(out, "Water Flow Meter setup");
  renderResult(out, result);
  renderSettingsForm(out);
  renderDocumentEnd(out);
}

// ── Parsing, validation, commit ─────────────────────────────────────────────────────

void PortalForm::addError(PortalSubmitResult& result, const char* field, PortalFieldError error) {
  if (result.errorCount >= PortalSubmitResult::kMaxErrors) {
    result.moreErrors = true;
    return;
  }
  PortalSubmitResult::FieldError& slot = result.errors[result.errorCount++];
  std::snprintf(slot.field, sizeof(slot.field), "%s", field ? field : "");
  slot.error = error;
}

bool PortalForm::resolve(const char* name, FieldRef& ref) const {
  if (!name || name[0] == '\0') return false;

  if (std::strcmp(name, kPortalUserField) == 0) {
    ref.isText = true;
    ref.textField = NetField::PortalUser;
    ref.textCapacity = netFieldCapacity(NetField::PortalUser);
    return true;
  }
  if (std::strcmp(name, kPortalPasswordField) == 0) {
    ref.isText = true;
    ref.textField = NetField::PortalPassword;
    ref.textCapacity = netFieldCapacity(NetField::PortalPassword);
    ref.writeOnly = true;  // so an empty submission keeps the current login rather than clearing it
    return true;
  }

  char base[kNameBytes] = {};
  const char* at = std::strchr(name, '@');
  uint8_t sensorIndex = 0;
  if (at) {
    const std::size_t length = static_cast<std::size_t>(at - name);
    if (length == 0 || length >= sizeof(base)) return false;
    std::memcpy(base, name, length);
    int64_t parsed = 0;
    if (!parseInt64(at + 1, parsed)) return false;
    if (parsed < 0 || static_cast<std::size_t>(parsed) >= sensorCount_) return false;
    sensorIndex = static_cast<uint8_t>(parsed);
  } else {
    std::snprintf(base, sizeof(base), "%s", name);
  }

  const ui::SettingDescriptor* setting = ui::findSetting(base);
  if (!setting) return false;
  // A per-sensor setting must carry an index and a global one must not. Without this,
  // `config.sensor.multiplier` with no suffix would silently write sensor 0 — a calibration applied
  // to the wrong sensor is worse than a rejected form.
  if (setting->perSensor != (at != nullptr)) return false;

  const Storage storage = storageFor(setting->target);
  ref.setting = setting;
  ref.sensorIndex = sensorIndex;
  ref.writeOnly = setting->writeOnly;
  ref.isText = setting->kind == ui::SettingKind::Text;
  ref.external = storage.kind == Storage::Kind::External;
  ref.textField = storage.kind == Storage::Kind::NetText ? storage.textField : NetField::Count;
  if (ref.isText) {
    // The tighter of the two capacities. The catalogue's maxLength and NetSettings' capacity are
    // two declarations of one fact; the host test asserts they agree, and until they do this
    // refuses what the store would have truncated.
    ref.textCapacity = setting->maxLength;
    if (ref.textField != NetField::Count) {
      const std::size_t stored = netFieldCapacity(ref.textField);
      if (stored < ref.textCapacity) ref.textCapacity = stored;
    }
  }
  return true;
}

void PortalForm::handleField(const char* name,
                            const char* value,
                            Pass pass,
                            PortalSubmitResult& result) {
  FieldRef ref;
  if (!resolve(name, ref)) {
    if (pass == Pass::Validate) addError(result, name, PortalFieldError::UnknownField);
    return;
  }

  if (ref.isText) {
    // §8.1, and the reason this distinction is a required test: an EMPTY writeOnly field means
    // "leave unchanged", never "set to empty". The form cannot render the stored secret, so the
    // field is blank on every load — treating blank as a value would wipe the passphrase every time
    // anybody saved any other setting, and §6.3 left no way to type it back in at the panel.
    if (ref.writeOnly && value[0] == '\0') return;

    if (ref.textField == NetField::Count) {
      // No text setting is stored outside NetSettings today. If one is added, it lands here as a
      // reported refusal rather than as a value that vanishes.
      if (pass == Pass::Validate) addError(result, name, PortalFieldError::Refused);
      return;
    }
    if (std::strlen(value) > ref.textCapacity) {
      if (pass == Pass::Validate) addError(result, name, PortalFieldError::TooLong);
      return;
    }
    if (hasControlCharacter(value)) {
      if (pass == Pass::Validate) addError(result, name, PortalFieldError::BadEncoding);
      return;
    }
    switch (pass) {
      case Pass::Validate:
        ++result.fieldsAccepted;
        return;
      case Pass::StageNetwork:
        if (!net_.stage(ref.textField, value)) {
          addError(result, name, PortalFieldError::Refused);
        } else {
          ++result.networkFieldsStaged;
        }
        return;
      case Pass::WriteExternal:
        return;
    }
    return;
  }

  const ui::SettingDescriptor& setting = *ref.setting;
  int32_t resolved = 0;
  switch (setting.kind) {
    case ui::SettingKind::Boolean: {
      bool on = false;
      if (!parseBoolean(value, on)) {
        if (pass == Pass::Validate) addError(result, name, PortalFieldError::NotANumber);
        return;
      }
      resolved = on ? 1 : 0;
      break;
    }
    case ui::SettingKind::Numeric:
    case ui::SettingKind::Enum: {
      int64_t parsed = 0;
      if (!parseInt64(value, parsed)) {
        if (pass == Pass::Validate) addError(result, name, PortalFieldError::NotANumber);
        return;
      }
      if (parsed < setting.min || parsed > setting.max) {
        if (pass == Pass::Validate) addError(result, name, PortalFieldError::OutOfRange);
        return;
      }
      resolved = static_cast<int32_t>(parsed);
      // Range is not membership: config.ledPulseVolume runs 1..100 but offers only 1, 10 and 100,
      // so 50 passes the bounds and is still not a choice this firmware has.
      if (setting.kind == ui::SettingKind::Enum && !isOption(setting, resolved)) {
        if (pass == Pass::Validate) addError(result, name, PortalFieldError::UnknownOption);
        return;
      }
      break;
    }
    case ui::SettingKind::Text:
      return;  // handled above
  }

  switch (pass) {
    case Pass::Validate:
      ++result.fieldsAccepted;
      return;
    case Pass::StageNetwork:
      if (!ref.external) {
        if (!stageNetScalar(net_, setting.target, resolved)) {
          addError(result, name, PortalFieldError::Refused);
        } else {
          ++result.networkFieldsStaged;
        }
      }
      return;
    case Pass::WriteExternal:
      if (!ref.external) return;
      if (store_ != nullptr && store_->writeValue(setting, ref.sensorIndex, resolved)) {
        ++result.externalWrites;
      } else {
        addError(result, name, PortalFieldError::Refused);
      }
      return;
  }
}

void PortalForm::runPass(const char* body, Pass pass, PortalSubmitResult& result) {
  const char* p = body;
  while (*p != '\0') {
    const char* pairEnd = p;
    while (*pairEnd != '\0' && *pairEnd != '&') ++pairEnd;

    if (pairEnd != p) {
      // The FIRST '=' separates; later ones belong to the value. A base64 secret pasted into a
      // password field ends in '=' padding, and splitting on the last one would eat it.
      const char* equals = p;
      while (equals != pairEnd && *equals != '=') ++equals;

      char name[kNameBytes] = {};
      char value[kMaxValueBytes] = {};
      const DecodeStatus nameStatus = urlDecode(p, equals, name, sizeof(name));
      const char* valueBegin = equals == pairEnd ? pairEnd : equals + 1;
      const DecodeStatus valueStatus = urlDecode(valueBegin, pairEnd, value, sizeof(value));

      if (nameStatus != DecodeStatus::Ok) {
        // No usable name to report, so the error carries none rather than echoing raw bytes.
        if (pass == Pass::Validate) {
          addError(result, "",
                   nameStatus == DecodeStatus::Overflow ? PortalFieldError::UnknownField
                                                        : PortalFieldError::BadEncoding);
        }
      } else if (valueStatus != DecodeStatus::Ok) {
        if (pass == Pass::Validate) {
          addError(result, name,
                   valueStatus == DecodeStatus::Overflow ? PortalFieldError::TooLong
                                                         : PortalFieldError::BadEncoding);
        }
      } else {
        handleField(name, value, pass, result);
      }
    }

    p = *pairEnd == '\0' ? pairEnd : pairEnd + 1;
  }
}

PortalSubmitResult PortalForm::submit(const char* body) {
  PortalSubmitResult result;
  if (!body) {
    addError(result, "", PortalFieldError::BadEncoding);
    return result;
  }

  // Three passes over the body rather than one, so no per-field value has to be buffered: with 54
  // controls on the page a staging array would be the largest allocation in the firmware, on the
  // stack of a network handler. Decoding is pure, so re-running it is free of consequence.
  //
  // Pass 1 writes NOTHING. R7.11: a form POST must not leave half a configuration live, so one bad
  // field refuses the whole submission.
  runPass(body, Pass::Validate, result);
  if (!result.ok()) return result;

  runPass(body, Pass::StageNetwork, result);
  // One apply for the whole submission, so the radio and the MQTT client see a single transition
  // rather than one per field — but ONLY if this submission actually staged a network field.
  //
  // This used to be unconditional, and that was a real defect rather than a redundant call. R5.5a
  // accepts that a deliberate apply promotes whatever a Modbus master has staged but not yet
  // committed: there is one apply path and the last apply wins. What it does not excuse is applying
  // when the portal staged NOTHING — a POST of only store-backed fields, or of no recognised field
  // at all, would still promote a master's half-written multi-register block. That destroys somebody
  // else's in-flight configuration for no reason, produces no change to report, and is invisible
  // from both ends.
  if (result.networkFieldsStaged > 0) {
    result.committed = net_.apply();
  }

  runPass(body, Pass::WriteExternal, result);
  if (!result.ok() && result.storedSomething()) {
    // Only reachable from a store refusal — the Nyquist rule, which needs the live polling rate and
    // therefore cannot be checked in pass 1. Reported, not hidden. Conditional on something having
    // actually landed: a submission of one store-only field that the store refuses applied NOTHING,
    // and calling that "partly saved" would send the operator looking for a change that is not there.
    result.partiallyApplied = true;
  }
  return result;
}

}  // namespace plc
