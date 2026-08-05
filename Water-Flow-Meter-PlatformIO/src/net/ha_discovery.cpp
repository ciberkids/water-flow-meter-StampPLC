#include "net/ha_discovery.h"

#include <cstring>

namespace plc {

namespace {

// The buffer arithmetic R4.4.8 depends on, checked at compile time so a careless edit to either
// constant cannot leave the host test asserting a bound that is larger than the buffer.
static_assert(kMaxDiscoveryPayloadBytes < kMqttOutBufferSize,
              "the payload bound must leave room inside out_buffer_size (R4.1.6)");
static_assert(kDiscoveryPayloadHeadroom > 0, "a bound with no headroom cannot fail usefully");

// ── The units, spelled one byte at a time ────────────────────────────────────────────
//
// Home Assistant accepts only its own spelling of `m3` and `degC`, and both are multi-byte UTF-8.
// Written as byte escapes rather than as the characters themselves because a raw multi-byte
// character depends on this FILE surviving every editor, patch and pipeline step between here and
// the compiler — and a unit re-encoded to Latin-1 leaves a lone B3, which HA rejects with no entity
// and no log line (§4.4.7's failure shape, reached from another direction). The static_asserts make
// the byte sequence a guarantee rather than an assumption: they fail the BUILD, not the
// integration. `"\xC2\xB0" "C"` is split because `"\xC2\xB0C"` would parse B0C as one hex escape.
constexpr char kUnitCubicMetres[] = "m\xC2\xB3";
constexpr char kUnitDegreesCelsius[] = "\xC2\xB0" "C";

static_assert(sizeof(kUnitCubicMetres) == 4 && kUnitCubicMetres[0] == 'm' &&
                  kUnitCubicMetres[1] == '\xC2' && kUnitCubicMetres[2] == '\xB3',
              "m3 must be the UTF-8 bytes 'm' C2 B3");
static_assert(sizeof(kUnitDegreesCelsius) == 4 && kUnitDegreesCelsius[0] == '\xC2' &&
                  kUnitDegreesCelsius[1] == '\xB0' && kUnitDegreesCelsius[2] == 'C',
              "degC must be the UTF-8 bytes C2 B0 'C'");

/** Whether a string is being written as a raw topic or as the contents of a JSON string. */
enum class Emit : uint8_t { Raw, Json };

/**
 * A bounded appender with `snprintf` semantics.
 *
 * Two properties earn its existence. It counts what the value NEEDS even when the buffer cannot
 * hold it, which is what lets the R4.4.8 test measure the worst case without a 2 kB buffer; and on
 * overflow it leaves the caller's buffer EMPTY rather than holding a prefix. The second is the
 * important one: a half-written JSON object is accepted by the broker and discarded by Home
 * Assistant, so the device would believe it had published an entity that does not exist.
 */
class Writer {
 public:
  Writer(char* out, std::size_t size) : out_(out), size_(size) {
    if (out_ != nullptr && size_ > 0) out_[0] = '\0';
  }

  void raw(const char* text) {
    if (text == nullptr) return;
    for (const char* p = text; *p != '\0'; ++p) put(*p);
  }

  /** Appends `text` escaped for a JSON string. Operator-supplied fields all come through here. */
  void escaped(const char* text) {
    if (text == nullptr) return;
    for (const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p != '\0'; ++p) {
      const unsigned char c = *p;
      if (c == '"' || c == '\\') {
        put('\\');
        put(static_cast<char>(c));
      } else if (c == '\n') {
        put('\\');
        put('n');
      } else if (c == '\r') {
        put('\\');
        put('r');
      } else if (c == '\t') {
        put('\\');
        put('t');
      } else if (c < 0x20) {
        // Any other control byte. Reachable: a Modbus master can stage arbitrary bytes into a text
        // field two at a time (net_settings.h stageByte), and a raw control byte inside a JSON
        // string is invalid JSON.
        put('\\');
        put('u');
        put('0');
        put('0');
        put(hexDigit(static_cast<unsigned>(c) >> 4));
        put(hexDigit(static_cast<unsigned>(c) & 0x0Fu));
      } else {
        // Everything from 0x20 up, including UTF-8 continuation bytes, is passed through: the units
        // of §4.4.a are multi-byte and must reach Home Assistant unaltered.
        put(static_cast<char>(c));
      }
    }
  }

  void text(const char* value, Emit mode) {
    if (mode == Emit::Raw) {
      raw(value);
    } else {
      escaped(value);
    }
  }

  void number(uint32_t value) {
    char digits[10];
    std::size_t count = 0;
    do {
      digits[count++] = static_cast<char>('0' + (value % 10u));
      value /= 10u;
    } while (value != 0u && count < sizeof(digits));
    while (count > 0) put(digits[--count]);
  }

  HaBuildResult finish() {
    const bool fits = out_ != nullptr && size_ > 0 && required_ + 1 <= size_;
    if (out_ != nullptr && size_ > 0) out_[fits ? required_ : 0] = '\0';
    return HaBuildResult{required_, fits};
  }

 private:
  static char hexDigit(unsigned nibble) {
    return static_cast<char>(nibble < 10 ? '0' + nibble : 'A' + (nibble - 10));
  }

  void put(char c) {
    // required_ + 1 < size_ keeps the last byte for the terminator, so a buffer of exactly
    // required + 1 bytes succeeds and one of exactly required bytes does not.
    if (out_ != nullptr && required_ + 1 < size_) out_[required_] = c;
    ++required_;
  }

  char* out_;
  std::size_t size_;
  std::size_t required_ = 0;
};

/**
 * True when the reference names an entity that exists.
 *
 * The `sensor == 0` demand on per-device entities is not pedantry. A caller looping kinds across
 * sensors — the obvious way to write the publish loop — would otherwise emit eight `polling_rate`
 * entities that all collide on one `object_id`, each retained message overwriting the last. That is
 * invisible in Home Assistant, so it is refused here instead.
 */
bool refValid(HaEntityRef ref) {
  if (ref.kind == HaEntity::Count) return false;
  if (haEntityIsPerSensor(ref.kind)) return static_cast<std::size_t>(ref.sensor) < kNumSensors;
  return ref.sensor == 0;
}

/** The per-entity `name`. Per-sensor entities are prefixed so eight of them are distinguishable. */
void writeEntityName(Writer& w, HaEntityRef ref) {
  if (haEntityIsPerSensor(ref.kind)) {
    w.raw("Sensor ");
    w.number(static_cast<uint32_t>(ref.sensor) + 1u);
    w.raw(" ");
  }
  switch (ref.kind) {
    case HaEntity::SensorFlow:           w.raw("flow"); break;
    case HaEntity::SensorSessionVolume:  w.raw("session volume"); break;
    case HaEntity::SensorLifetimeVolume: w.raw("lifetime volume"); break;
    case HaEntity::BoardTemperature:     w.raw("Board temperature"); break;
    case HaEntity::PollingRate:          w.raw("Polling rate"); break;
    case HaEntity::Undersampling:        w.raw("Undersampling"); break;
    case HaEntity::WifiRssi:             w.raw("WiFi signal"); break;
    case HaEntity::Count:                break;
  }
}

void writeObjectId(Writer& w, HaEntityRef ref) {
  if (haEntityIsPerSensor(ref.kind)) {
    w.raw("s");
    w.number(static_cast<uint32_t>(ref.sensor) + 1u);
    w.raw("_");
  }
  w.raw(haEntityObjectIdSuffix(ref.kind));
}

/** `<base>/sensor/<n>/state` or `<base>/diagnostics/state` (§4.2). */
void writeStateTopic(Writer& w, Emit mode, const char* base, HaEntityRef ref) {
  w.text(base, mode);
  if (haEntityIsPerSensor(ref.kind)) {
    w.text("/sensor/", mode);
    w.number(static_cast<uint32_t>(ref.sensor) + 1u);
    w.text("/state", mode);
  } else {
    w.text("/diagnostics/state", mode);
  }
}

bool fitsCap(const char* value, std::size_t capacity) {
  return value == nullptr || std::strlen(value) <= capacity;
}

void copyInto(char* dest, const char* value) {
  if (value == nullptr) {
    dest[0] = '\0';
    return;
  }
  const std::size_t length = std::strlen(value);
  std::memcpy(dest, value, length);
  dest[length] = '\0';
}

/**
 * `node_id` charset.
 *
 * Deliberately narrower than MQTT would allow. A `/` would silently restructure the discovery
 * topic — `homeassistant/sensor/a/b/s1_flow/config` is a topic Home Assistant does not read — and a
 * space or a `+` would be worse. Narrow is affordable because the value is MAC-derived (R4.1.4).
 */
bool idCharsValid(const char* value) {
  if (value == nullptr || value[0] == '\0') return false;
  for (const char* p = value; *p != '\0'; ++p) {
    const char c = *p;
    const bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') ||
                    c == '_' || c == '-';
    if (!ok) return false;
  }
  return true;
}

// There is deliberately NO topic rule in this file. It used to hold one — `topicCharsValid` — and
// `MqttPublisher::configure` held another, and the two disagreed about the same operator-supplied
// string. See `HaDiscovery::configure` below for what replaced it and what the disagreement cost.

}  // namespace

// ── §4.4.a, one accessor per column ──────────────────────────────────────────────────

const char* haEntityValueKey(HaEntity entity) {
  // The keys of the shared per-sensor and diagnostics state payloads. `flow` is spelled exactly as
  // §4.4.b's example template writes it; the rest are the field names the publisher actually emits
  // (`MqttPublisher::formatSensor` / `formatDiagnostics`), which is why `total` and `tempC` are
  // spelled the publisher's way rather than the way an entity name would suggest.
  //
  // A template reads what a payload writes, so these two spellings are ONE contract with two ends,
  // and a mismatch is invisible: the entity appears in Home Assistant and sits at `unknown`
  // forever, with nothing logged at either end. So the publisher must build its state JSON from
  // THIS function rather than from its own literals — that is what this accessor is for.
  switch (entity) {
    case HaEntity::SensorFlow:           return "flow";
    case HaEntity::SensorSessionVolume:  return "session";
    case HaEntity::SensorLifetimeVolume: return "total";  // m3, per MqttSensorTelemetry
    case HaEntity::BoardTemperature:     return "tempC";
    case HaEntity::PollingRate:          return "pollingRateKhz";
    case HaEntity::Undersampling:        return "undersampling";
    case HaEntity::WifiRssi:             return "rssi";
    case HaEntity::Count:                return nullptr;
  }
  return nullptr;
}

const char* haEntityDeviceClass(HaEntity entity) {
  switch (entity) {
    case HaEntity::SensorFlow:           return "volume_flow_rate";
    case HaEntity::SensorSessionVolume:  return "water";
    case HaEntity::SensorLifetimeVolume: return "water";
    case HaEntity::BoardTemperature:     return "temperature";
    // §4.4.a gives the diagnostics no device class. Nothing is invented here on purpose: an
    // unrecognised device_class is rejected by Home Assistant, and the symptom is a missing entity.
    // Omitting the key is always valid, so RSSI ships without one rather than with a guess.
    case HaEntity::PollingRate:          return nullptr;
    case HaEntity::Undersampling:        return nullptr;
    case HaEntity::WifiRssi:             return nullptr;
    case HaEntity::Count:                return nullptr;
  }
  return nullptr;
}

const char* haEntityUnit(HaEntity entity) {
  switch (entity) {
    case HaEntity::SensorFlow:           return "L/min";
    case HaEntity::SensorSessionVolume:  return "L";
    case HaEntity::SensorLifetimeVolume: return kUnitCubicMetres;
    case HaEntity::BoardTemperature:     return kUnitDegreesCelsius;
    case HaEntity::PollingRate:          return "kHz";
    case HaEntity::Undersampling:        return nullptr;  // a bitmap of sensors, not a quantity
    case HaEntity::WifiRssi:             return "dBm";
    case HaEntity::Count:                return nullptr;
  }
  return nullptr;
}

const char* haEntityStateClass(HaEntity entity) {
  switch (entity) {
    case HaEntity::SensorFlow:           return "measurement";
    // R4.4.4 — `total_increasing` together with device_class water and a water unit is what makes
    // HA's Water dashboard accumulate long-term statistics. Correct for a monotonic counter; HA
    // handles the reset-to-zero itself, which is why a session total can use it too.
    case HaEntity::SensorSessionVolume:  return "total_increasing";
    case HaEntity::SensorLifetimeVolume: return "total_increasing";
    case HaEntity::BoardTemperature:     return "measurement";
    case HaEntity::PollingRate:          return "measurement";
    case HaEntity::Undersampling:        return "measurement";
    case HaEntity::WifiRssi:             return "measurement";
    case HaEntity::Count:                return nullptr;
  }
  return nullptr;
}

uint8_t haEntityPrecision(HaEntity entity) {
  switch (entity) {
    case HaEntity::SensorFlow:           return 2;
    case HaEntity::SensorSessionVolume:  return 1;
    case HaEntity::SensorLifetimeVolume: return 3;  // m3 to three places is litre resolution
    case HaEntity::BoardTemperature:     return 1;
    case HaEntity::PollingRate:          return 2;
    case HaEntity::Undersampling:        return 0;
    case HaEntity::WifiRssi:             return 0;
    case HaEntity::Count:                return 0;
  }
  return 0;
}

bool haEntityIsDiagnostic(HaEntity entity) {
  switch (entity) {
    case HaEntity::PollingRate:
    case HaEntity::Undersampling:
    case HaEntity::WifiRssi:
      return true;
    // Board temperature is a reading of the environment, not of the firmware's health, so it stays
    // in the main view. It shares the diagnostics STATE topic only because that is where the value
    // is published (§4.2).
    case HaEntity::SensorFlow:
    case HaEntity::SensorSessionVolume:
    case HaEntity::SensorLifetimeVolume:
    case HaEntity::BoardTemperature:
    case HaEntity::Count:
      return false;
  }
  return false;
}

bool haEntityIsPerSensor(HaEntity entity) {
  switch (entity) {
    case HaEntity::SensorFlow:
    case HaEntity::SensorSessionVolume:
    case HaEntity::SensorLifetimeVolume:
      return true;
    case HaEntity::BoardTemperature:
    case HaEntity::PollingRate:
    case HaEntity::Undersampling:
    case HaEntity::WifiRssi:
    case HaEntity::Count:
      return false;
  }
  return false;
}

const char* haEntityObjectIdSuffix(HaEntity entity) {
  switch (entity) {
    case HaEntity::SensorFlow:           return "flow";
    case HaEntity::SensorSessionVolume:  return "session";
    case HaEntity::SensorLifetimeVolume: return "lifetime";
    case HaEntity::BoardTemperature:     return "board_temperature";
    case HaEntity::PollingRate:          return "polling_rate";
    case HaEntity::Undersampling:        return "undersampling";
    case HaEntity::WifiRssi:             return "wifi_rssi";
    case HaEntity::Count:                return "";
  }
  return "";
}

const char* haRepublishReasonText(HaRepublishReason reason) {
  switch (reason) {
    case HaRepublishReason::None:           return "none";
    case HaRepublishReason::FirstConnect:   return "first connect";
    case HaRepublishReason::Reconnect:      return "reconnect";
    case HaRepublishReason::HaBirth:        return "home assistant birth";
    case HaRepublishReason::SensorsChanged: return "sensor bitmap changed";
  }
  return "none";
}

// ── HaDiscovery ──────────────────────────────────────────────────────────────────────

bool HaDiscovery::configure(const HaDeviceIdentity& device, const char* discoveryPrefix,
                            const char* baseTopic) {
  configured_ = false;
  prefix_[0] = '\0';
  base_[0] = '\0';
  nodeId_[0] = '\0';
  deviceName_[0] = '\0';
  swVersion_[0] = '\0';
  configUrl_[0] = '\0';

  // Everything is validated before anything is copied, so a rejected configuration leaves the
  // object empty rather than partly populated with the fields that happened to be checked first.
  if (!idCharsValid(device.nodeId)) return false;

  // ── Owner decision 5A: ONE base-topic rule, and this module defers to it ─────────────
  //
  // This used to be a private `topicCharsValid` here and a different private rule in
  // `MqttPublisher::configure`, for the same field. Measured on the pre-change tree, they disagreed
  // about six classes of operator input: the publisher accepted a trailing `/` (silently stripping
  // it), a leading `/`, a space, and any length from 49 to 109 bytes, all of which this module
  // refused; this module accepted 0x7f and every byte from 0x80 up, which the publisher refused.
  //
  // Either direction is silent. A topic the publisher accepts and discovery refuses yields ZERO
  // Home Assistant entities while `<base>/...` telemetry flows and the MQTT state register reads
  // connected (§4.4.7 — nothing is logged at either end), so it presents as a broker fault. The
  // reverse leaves the entities advertised and nothing published to them, so every one sits at
  // `unknown` forever.
  //
  // So the rule is `NetSettings::isValidBaseTopic` and nothing else — not layered on top of a local
  // one, because two validators that agree today are two validators that can disagree tomorrow,
  // which is the defect being removed rather than a hypothetical.
  //
  // The discovery PREFIX goes through the same validator, for the same reason: it is operator input
  // that becomes a published topic (`<prefix>/sensor/...`) and a SUBSCRIBED one (`<prefix>/status`,
  // R4.4.7), so a private second rule for it would just be the same defect at a smaller scale. An
  // interior `//` is the case that matters — `homeassistant//status` is a legal topic that Home
  // Assistant's birth message never lands on, so R4.4.7's republish would silently never fire.
  if (!NetSettings::isValidBaseTopic(discoveryPrefix) ||
      !NetSettings::isValidBaseTopic(baseTopic)) {
    return false;
  }
  if (!fitsCap(device.nodeId, kHaMaxNodeIdBytes)) return false;
  // KEPT, and not a topic rule: the prefix has its own 32-byte field (§5's register block), which is
  // TIGHTER than the validator's 48-byte base-topic cap, so this refusal is reachable for prefixes
  // of 33..48 bytes and is what keeps `prefix_` from being overrun. Do not "simplify" it away on the
  // grounds that the validator already checks a length — it checks a different field's length.
  if (!fitsCap(discoveryPrefix, kHaMaxPrefixBytes)) return false;
  // There is deliberately no matching `fitsCap(baseTopic, kHaMaxBaseTopicBytes)`. That constant IS
  // `netFieldCapacity(NetField::MqttBaseTopic)` (ha_discovery.h), which is the cap the validator
  // itself enforces, so the branch could not be taken — and this project has already shipped checks
  // that could not fail. `base_` is sized from the same constant, so nothing is left unguarded.
  if (!fitsCap(device.swVersion, kHaMaxSwVersionBytes)) return false;
  if (!fitsCap(device.configurationUrl, kHaMaxConfigUrlBytes)) return false;

  const char* name = (device.name != nullptr && device.name[0] != '\0') ? device.name
                                                                       : kHaDefaultDeviceName;
  if (!fitsCap(name, kHaMaxDeviceNameBytes)) return false;

  copyInto(nodeId_, device.nodeId);
  copyInto(prefix_, discoveryPrefix);
  copyInto(base_, baseTopic);
  copyInto(deviceName_, name);
  copyInto(swVersion_, device.swVersion);
  copyInto(configUrl_, device.configurationUrl);
  configured_ = true;
  return true;
}

HaBuildResult HaDiscovery::discoveryTopic(HaEntityRef ref, char* out, std::size_t size) const {
  Writer w(out, size);
  if (!configured_ || !refValid(ref)) return HaBuildResult{};
  w.raw(prefix_);
  w.raw("/sensor/");
  w.raw(nodeId_);
  w.raw("/");
  writeObjectId(w, ref);
  w.raw("/config");
  return w.finish();
}

HaBuildResult HaDiscovery::objectId(HaEntityRef ref, char* out, std::size_t size) const {
  Writer w(out, size);
  if (!configured_ || !refValid(ref)) return HaBuildResult{};
  writeObjectId(w, ref);
  return w.finish();
}

HaBuildResult HaDiscovery::uniqueId(HaEntityRef ref, char* out, std::size_t size) const {
  Writer w(out, size);
  if (!configured_ || !refValid(ref)) return HaBuildResult{};
  w.raw(nodeId_);
  w.raw("_");
  writeObjectId(w, ref);
  return w.finish();
}

HaBuildResult HaDiscovery::stateTopic(HaEntityRef ref, char* out, std::size_t size) const {
  Writer w(out, size);
  if (!configured_ || !refValid(ref)) return HaBuildResult{};
  writeStateTopic(w, Emit::Raw, base_, ref);
  return w.finish();
}

HaBuildResult HaDiscovery::availabilityTopic(char* out, std::size_t size) const {
  Writer w(out, size);
  if (!configured_) return HaBuildResult{};
  w.raw(base_);
  w.raw("/");
  w.raw(kHaStatusTopicSuffix);
  return w.finish();
}

HaBuildResult HaDiscovery::haStatusTopic(char* out, std::size_t size) const {
  Writer w(out, size);
  if (!configured_) return HaBuildResult{};
  w.raw(prefix_);
  w.raw("/");
  w.raw(kHaStatusTopicSuffix);
  return w.finish();
}

HaBuildResult HaDiscovery::discoveryPayload(HaEntityRef ref, char* out, std::size_t size) const {
  Writer w(out, size);
  if (!configured_ || !refValid(ref)) return HaBuildResult{};

  // The device block, repeated in every payload — that repetition is what groups the entities under
  // one device (R4.4.2). Ordered as §4.4.b writes it so a diff against the requirement is readable.
  w.raw("{\"device\":{\"identifiers\":[\"");
  w.escaped(nodeId_);
  w.raw("\"],\"name\":\"");
  w.escaped(deviceName_);
  w.raw("\",\"manufacturer\":\"");
  w.raw(kHaManufacturer);
  w.raw("\",\"model\":\"");
  w.raw(kHaModel);
  w.raw("\"");
  // Omitted rather than emitted empty: `"configuration_url":""` is not a URL, and a payload Home
  // Assistant rejects produces no entity and no log — the same silent failure as R4.4.8's. Before
  // the first association there is no address to put here, so this case is the normal one.
  if (swVersion_[0] != '\0') {
    w.raw(",\"sw_version\":\"");
    w.escaped(swVersion_);
    w.raw("\"");
  }
  if (configUrl_[0] != '\0') {
    w.raw(",\"configuration_url\":\"");
    w.escaped(configUrl_);
    w.raw("\"");
  }
  w.raw("},\"name\":\"");
  writeEntityName(w, ref);

  w.raw("\",\"unique_id\":\"");
  w.escaped(nodeId_);
  w.raw("_");
  writeObjectId(w, ref);

  // One state topic shared by the three per-sensor entities, each picking its value out with a
  // template (§4.2 — eight publishes per cycle instead of forty).
  w.raw("\",\"state_topic\":\"");
  writeStateTopic(w, Emit::Json, base_, ref);
  w.raw("\",\"value_template\":\"{{ value_json.");
  w.raw(haEntityValueKey(ref.kind));
  w.raw(" }}\",\"availability_topic\":\"");
  w.escaped(base_);
  w.raw("/");
  w.raw(kHaStatusTopicSuffix);
  w.raw("\"");

  const char* deviceClass = haEntityDeviceClass(ref.kind);
  if (deviceClass != nullptr) {
    w.raw(",\"device_class\":\"");
    w.raw(deviceClass);
    w.raw("\"");
  }
  const char* unit = haEntityUnit(ref.kind);
  if (unit != nullptr) {
    w.raw(",\"unit_of_measurement\":\"");
    w.raw(unit);
    w.raw("\"");
  }
  w.raw(",\"state_class\":\"");
  w.raw(haEntityStateClass(ref.kind));
  // Always present: without it Home Assistant infers 0 decimals and renders litres per minute as a
  // bare integer (§4.4).
  w.raw("\",\"suggested_display_precision\":");
  w.number(haEntityPrecision(ref.kind));
  if (haEntityIsDiagnostic(ref.kind)) {
    // The only value this key may take on a sensor (§4.4.a).
    w.raw(",\"entity_category\":\"diagnostic\"");
  }
  w.raw("}");
  return w.finish();
}

bool HaDiscovery::isHaBirth(const char* topic, const char* payload) const {
  if (!configured_ || topic == nullptr || payload == nullptr) return false;
  char expected[kHaStateTopicBytes] = {};
  if (!haStatusTopic(expected, sizeof(expected)).complete) return false;
  return std::strcmp(topic, expected) == 0 && std::strcmp(payload, kHaBirthPayload) == 0;
}

std::size_t HaDiscovery::enumerateEntities(uint16_t connectedBitmap, HaEntityRef* out,
                                           std::size_t capacity) const {
  if (!configured_ || out == nullptr) return 0;

  static constexpr HaEntity kPerSensor[] = {HaEntity::SensorFlow, HaEntity::SensorSessionVolume,
                                            HaEntity::SensorLifetimeVolume};
  static constexpr HaEntity kPerDevice[] = {HaEntity::BoardTemperature, HaEntity::PollingRate,
                                            HaEntity::Undersampling, HaEntity::WifiRssi};
  std::size_t written = 0;
  for (std::size_t sensor = 0; sensor < kNumSensors; ++sensor) {
    // Only connected sensors get entities, which is what makes "enabling a sensor makes its entity
    // appear" true once the bitmap change triggers a republish (R4.4.6).
    if ((connectedBitmap & (1u << sensor)) == 0u) continue;
    for (HaEntity kind : kPerSensor) {
      if (written >= capacity) return written;
      out[written].kind = kind;
      out[written].sensor = static_cast<uint8_t>(sensor);
      ++written;
    }
  }
  for (HaEntity kind : kPerDevice) {
    if (written >= capacity) return written;
    out[written].kind = kind;
    out[written].sensor = 0;
    ++written;
  }
  return written;
}

// ── HaRepublishPolicy ────────────────────────────────────────────────────────────────

HaRepublishReason HaRepublishPolicy::onConnected() {
  connected_ = true;
  return published_ ? HaRepublishReason::Reconnect : HaRepublishReason::FirstConnect;
}

HaRepublishReason HaRepublishPolicy::onStatusMessage(const HaDiscovery& discovery,
                                                     const char* topic,
                                                     const char* payload) const {
  return discovery.isHaBirth(topic, payload) ? HaRepublishReason::HaBirth
                                             : HaRepublishReason::None;
}

HaRepublishReason HaRepublishPolicy::onSensorBitmap(uint16_t connectedBitmap) const {
  // Before the first publish there is nothing to re-publish: the connect path publishes the whole
  // set anyway, so asking here as well would double it.
  if (!connected_ || !published_) return HaRepublishReason::None;
  return connectedBitmap != bitmap_ ? HaRepublishReason::SensorsChanged : HaRepublishReason::None;
}

void HaRepublishPolicy::notePublished(uint16_t connectedBitmap) {
  published_ = true;
  bitmap_ = connectedBitmap;
}

void HaRepublishPolicy::noteDisconnected() {
  // `published_` and the bitmap are deliberately kept. The retained discovery messages of R4.4.5
  // survive the disconnect, and the reconnect republishes unconditionally anyway (R4.4.6), so
  // forgetting them here would only lose the ability to report WHY the republish happened.
  connected_ = false;
}

}  // namespace plc
