// Host tests for Home Assistant MQTT discovery (WiFi_MQTT_Connectivity.md §4.4).
//
// Everything §4.4 gets wrong fails the same way: the entity never appears in Home Assistant and
// nothing is logged. There is no error to read on the device, on the broker or in HA — so a bench
// tells you strictly less than this file does. The centrepiece is R4.4.8: the worst-case payload is
// serialised for real and measured against the SAME constant the client's `out_buffer_size` uses.
//
// Written before src/net/ha_discovery.cpp, deliberately, for the reason R4.4.8 exists: a size test
// written afterwards asserts the buffer that was already chosen instead of the length actually
// needed.
#include "net/ha_discovery.h"
#include "net/mqtt_command_router.h"

// The ONE base-topic rule (owner decision 5A). Named explicitly rather than leaned on through
// ha_discovery.h, because the agreement checks below call it directly and must not depend on
// another header's include list to keep compiling.
#include "net/net_settings.h"

#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-74s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

void checkStr(const char* actual, const char* expected, const char* what) {
  const bool same = std::strcmp(actual, expected) == 0;
  ++checks;
  std::printf("  %-74s %s\n", what, same ? "ok" : "FAIL");
  if (!same) {
    std::printf("      expected \"%s\"\n      actual   \"%s\"\n", expected, actual);
    ++failures;
  }
}

using plc::HaBuildResult;
using plc::HaDeviceIdentity;
using plc::HaDiscovery;
using plc::HaEntity;
using plc::HaEntityRef;
using plc::HaRepublishPolicy;
using plc::HaRepublishReason;

// ── The exact unit bytes, spelled out one byte at a time ──────────────────────────────
//
// NOT written as "m³" here. A literal in this file would be re-encoded by whatever mangled the
// source, so it would still compare equal to an equally-mangled literal in ha_discovery.cpp and the
// test would pass while Home Assistant rejected the unit. Byte arrays cannot be silently
// re-encoded, so this is the one place the expectation is stated in bytes.
const char kUnitCubicMetres[] = {'m', '\xC2', '\xB3', '\0'};
const char kUnitDegreesCelsius[] = {'\xC2', '\xB0', 'C', '\0'};
const char kUnitCubicFeetPerMin[] = {'f', 't', '\xC2', '\xB3', '/', 'm', 'i', 'n', '\0'};

bool contains(const char* haystack, const char* needle) {
  return std::strstr(haystack, needle) != nullptr;
}

int countOccurrences(const char* haystack, const char* needle) {
  int found = 0;
  const std::size_t step = std::strlen(needle);
  for (const char* at = std::strstr(haystack, needle); at != nullptr;
       at = std::strstr(at + step, needle)) {
    ++found;
  }
  return found;
}

/**
 * A deliberately picky JSON check.
 *
 * Not a parser — it does not need to be. What it catches is exactly what a hand-rolled serialiser
 * gets wrong: an unbalanced brace, an unterminated string, a trailing comma, a raw control byte
 * inside a string (invalid JSON, and reachable from any settings field), and an escape sequence
 * that is not one of JSON's.
 */
bool jsonWellFormed(const char* s) {
  const std::size_t len = std::strlen(s);
  if (len < 2 || s[0] != '{' || s[len - 1] != '}') return false;
  int curly = 0;
  int square = 0;
  bool inString = false;
  for (std::size_t i = 0; i < len; ++i) {
    const unsigned char c = static_cast<unsigned char>(s[i]);
    if (inString) {
      if (c == '\\') {
        const char next = s[i + 1];
        if (next == '"' || next == '\\' || next == '/' || next == 'b' || next == 'f' ||
            next == 'n' || next == 'r' || next == 't') {
          ++i;
          continue;
        }
        if (next == 'u') {
          for (int k = 1; k <= 4; ++k) {
            if (std::isxdigit(static_cast<unsigned char>(s[i + 1 + k])) == 0) return false;
          }
          i += 5;
          continue;
        }
        return false;
      }
      if (c == '"') {
        inString = false;
      } else if (c < 0x20) {
        return false;
      }
      continue;
    }
    if (c == '"') {
      inString = true;
    } else if (c == '{') {
      ++curly;
    } else if (c == '}') {
      if (--curly < 0) return false;
    } else if (c == '[') {
      ++square;
    } else if (c == ']') {
      if (--square < 0) return false;
    } else if (c == ',') {
      if (s[i + 1] == '}' || s[i + 1] == ']' || s[i + 1] == ',') return false;
    } else if (c == ':') {
      if (s[i + 1] == ',' || s[i + 1] == '}') return false;
    }
  }
  return curly == 0 && square == 0 && !inString;
}

/** The configuration a device ships with: the §4.4 defaults plus a MAC-derived id. */
HaDiscovery defaultDiscovery() {
  HaDeviceIdentity device;
  device.nodeId = "wfm_a1b2c3";
  device.name = "Water Flow Meter";
  device.swVersion = "1.4.0";
  device.configurationUrl = "http://192.168.1.50/";
  HaDiscovery ha;
  ha.configure(device, "homeassistant", "watermeter/a1b2c3");
  return ha;
}

std::string payloadOf(const HaDiscovery& ha, HaEntityRef ref) {
  char buffer[plc::kMqttOutBufferSize] = {};
  const HaBuildResult result = ha.discoveryPayload(ref, buffer, sizeof(buffer));
  if (!result.complete) return std::string();
  return std::string(buffer);
}

std::string topicOf(const HaDiscovery& ha, HaEntityRef ref) {
  char buffer[plc::kHaDiscoveryTopicBytes] = {};
  const HaBuildResult result = ha.discoveryTopic(ref, buffer, sizeof(buffer));
  if (!result.complete) return std::string();
  return std::string(buffer);
}

std::string stateTopicOf(const HaDiscovery& ha, HaEntityRef ref) {
  char buffer[plc::kHaStateTopicBytes] = {};
  const HaBuildResult result = ha.stateTopic(ref, buffer, sizeof(buffer));
  if (!result.complete) return std::string();
  return std::string(buffer);
}

HaEntityRef sensorRef(HaEntity kind, uint8_t sensor) {
  HaEntityRef ref;
  ref.kind = kind;
  ref.sensor = sensor;
  return ref;
}

HaEntityRef globalRef(HaEntity kind) { return sensorRef(kind, 0); }

// ═══════════════════════════════════════════════════════════════════════════════════════
// R4.4.8 — the buffer is a silent failure, so it gets a test (§4.4.7)
// ═══════════════════════════════════════════════════════════════════════════════════════
//
// What would break this test: adding a field to the payload, raising any length cap in
// ha_discovery.h, lowering kMqttOutBufferSize, or raising the base-topic / discovery-prefix
// capacities in net_settings.h. All four are the moment somebody needs to know.
//
// The upper bound alone would be satisfied by a builder that emitted "{}" or bailed after the
// device block, so it is paired with a completeness check and with occurrence counts on the
// variable-length inputs. An upper bound with no floor is not a test.
void worstCaseBufferTests() {
  std::printf("[R4.4.8 — the worst-case payload against the client's out_buffer_size]\n");

  // Every variable field at its documented maximum. The strings are distinctive so the occurrence
  // counts below cannot be satisfied by accident.
  const std::string prefix(plc::kHaMaxPrefixBytes, 'P');
  const std::string base = std::string(plc::kHaMaxBaseTopicBytes - 5, 'B') + "/topi";
  const std::string nodeId(plc::kHaMaxNodeIdBytes, 'N');
  const std::string deviceName(plc::kHaMaxDeviceNameBytes, 'D');
  const std::string swVersion(plc::kHaMaxSwVersionBytes, 'V');
  const std::string configUrl = "http://" + std::string(plc::kHaMaxConfigUrlBytes - 8, 'U') + "/";

  HaDeviceIdentity device;
  device.nodeId = nodeId.c_str();
  device.name = deviceName.c_str();
  device.swVersion = swVersion.c_str();
  device.configurationUrl = configUrl.c_str();

  HaDiscovery ha;
  check(ha.configure(device, prefix.c_str(), base.c_str()),
        "the longest legal prefix, base topic, node id, name, version and URL all configure");
  check(configUrl.size() == plc::kHaMaxConfigUrlBytes && base.size() == plc::kHaMaxBaseTopicBytes,
        "and the worst-case inputs really are at their caps");

  // Measured over EVERY entity rather than over a guess at which is longest, with every sensor
  // connected so the highest sensor index is included.
  HaEntityRef entities[plc::kHaMaxEntities];
  const std::size_t count = ha.enumerateEntities(0xFFFF, entities, plc::kHaMaxEntities);
  check(count == plc::kHaMaxEntities, "all 28 entities enumerate with every sensor connected");

  std::size_t worstPayload = 0;
  std::size_t worstTopic = 0;
  std::size_t worstIndex = 0;
  bool allComplete = true;
  for (std::size_t i = 0; i < count; ++i) {
    char payload[plc::kMqttOutBufferSize] = {};
    const HaBuildResult p = ha.discoveryPayload(entities[i], payload, sizeof(payload));
    allComplete = allComplete && p.complete && jsonWellFormed(payload);
    if (p.required > worstPayload) {
      worstPayload = p.required;
      worstIndex = i;
    }
    char topic[plc::kHaDiscoveryTopicBytes] = {};
    const HaBuildResult t = ha.discoveryTopic(entities[i], topic, sizeof(topic));
    allComplete = allComplete && t.complete;
    if (t.required > worstTopic) worstTopic = t.required;
  }
  check(allComplete, "every one of them serialises whole and well-formed at worst-case lengths");

  std::printf("      worst-case payload %zu bytes (%s), bound %zu, out_buffer_size %zu\n",
              worstPayload, plc::haEntityObjectIdSuffix(entities[worstIndex].kind),
              plc::kMaxDiscoveryPayloadBytes, plc::kMqttOutBufferSize);
  std::printf("      worst-case discovery topic %zu bytes, buffer %zu\n", worstTopic,
              plc::kHaDiscoveryTopicBytes);

  check(worstPayload <= plc::kMaxDiscoveryPayloadBytes,
        "R4.4.8: the worst case fits the bound the buffer constant sets");
  check(worstPayload + plc::kDiscoveryPayloadHeadroom <= plc::kMqttOutBufferSize,
        "which is out_buffer_size less the headroom, stated as the arithmetic it is");
  check(worstTopic < plc::kHaDiscoveryTopicBytes,
        "and kHaDiscoveryTopicBytes really does hold the longest topic");

  // The floor. Derived from the inputs rather than a magic number: every variable field has to
  // appear in the payload, the base topic twice (state + availability) and the node id twice
  // (identifiers + unique_id). A builder that dropped the device block would fail here.
  char worst[plc::kMqttOutBufferSize] = {};
  ha.discoveryPayload(entities[worstIndex], worst, sizeof(worst));
  check(countOccurrences(worst, base.c_str()) == 2,
        "the base topic appears exactly twice: state_topic and availability_topic");
  check(countOccurrences(worst, nodeId.c_str()) == 2,
        "and the node id exactly twice: device identifiers and unique_id");
  const std::size_t floorBytes =
      2 * base.size() + 2 * nodeId.size() + deviceName.size() + swVersion.size() + configUrl.size();
  check(worstPayload > floorBytes,
        "the measured length exceeds the variable content it must carry (a real floor)");

  // device_class and unit_of_measurement are deliberately NOT in this list: §4.4.a gives the
  // diagnostics neither, and the longest payload is one of them. They are checked per entity in
  // exactStringTests, and on the richest payload just below.
  static const char* const kRequiredKeys[] = {"\"device\"",
                                              "\"identifiers\"",
                                              "\"name\"",
                                              "\"manufacturer\"",
                                              "\"model\"",
                                              "\"sw_version\"",
                                              "\"configuration_url\"",
                                              "\"unique_id\"",
                                              "\"state_topic\"",
                                              "\"value_template\"",
                                              "\"availability_topic\"",
                                              "\"state_class\"",
                                              "\"suggested_display_precision\""};
  bool everyKey = true;
  for (const char* key : kRequiredKeys) everyKey = everyKey && contains(worst, key);
  check(everyKey, "and it carries every key §4.4.b requires of it — a COMPLETE payload was measured");

  // The richest payload — a per-sensor water entity, which carries every key any entity can — is
  // measured too, so the bound covers the entity with the most to say and not merely the longest
  // name.
  char richest[plc::kMqttOutBufferSize] = {};
  const HaBuildResult rich =
      ha.discoveryPayload(sensorRef(HaEntity::SensorLifetimeVolume, plc::kNumSensors - 1), richest,
                          sizeof(richest));
  bool richKeys = rich.complete && contains(richest, "\"device_class\"") &&
                  contains(richest, "\"unit_of_measurement\"");
  for (const char* key : kRequiredKeys) richKeys = richKeys && contains(richest, key);
  check(richKeys && rich.required <= plc::kMaxDiscoveryPayloadBytes,
        "and the entity with the MOST keys fits the same bound, device_class and unit included");
}

// ═══════════════════════════════════════════════════════════════════════════════════════
// Truncation is detected, not emitted
// ═══════════════════════════════════════════════════════════════════════════════════════
//
// What would break this test: a builder that writes as much as fits and terminates. That publishes
// half a JSON object, which the broker accepts and Home Assistant discards in silence — strictly
// worse than publishing nothing, because the device then believes the entity exists.
void truncationTests() {
  std::printf("\n[truncation — reported, and never left in the buffer]\n");

  const HaDiscovery ha = defaultDiscovery();
  const HaEntityRef ref = sensorRef(HaEntity::SensorFlow, 0);

  char full[plc::kMqttOutBufferSize] = {};
  const HaBuildResult big = ha.discoveryPayload(ref, full, sizeof(full));
  check(big.complete && big.required == std::strlen(full),
        "a whole payload reports its own length");

  char small[64];
  std::memset(small, 'x', sizeof(small));
  const HaBuildResult tight = ha.discoveryPayload(ref, small, sizeof(small));
  check(!tight.complete, "a 64-byte buffer is refused rather than filled");
  check(small[0] == '\0', "and the buffer is left EMPTY, not holding a prefix of the JSON");
  check(tight.required == big.required,
        "the required length is reported anyway, so the failure is diagnosable");

  // A truncated topic is just as bad: it publishes a valid config to the wrong topic.
  char shortTopic[8];
  std::memset(shortTopic, 'x', sizeof(shortTopic));
  const HaBuildResult topic = ha.discoveryTopic(ref, shortTopic, sizeof(shortTopic));
  check(!topic.complete && shortTopic[0] == '\0',
        "a topic that does not fit is emptied too — never published to a truncated topic");

  const HaBuildResult none = ha.discoveryPayload(ref, nullptr, 0);
  check(!none.complete && none.required == big.required,
        "a null buffer measures the payload without writing anything");

  char boundary[plc::kMqttOutBufferSize] = {};
  const HaBuildResult exact = ha.discoveryPayload(ref, boundary, big.required + 1);
  check(exact.complete && std::strlen(boundary) == big.required,
        "a buffer of exactly required+1 bytes fits — the off-by-one goes the safe way");
  char justUnder[plc::kMqttOutBufferSize] = {};
  const HaBuildResult tooTight = ha.discoveryPayload(ref, justUnder, big.required);
  check(!tooTight.complete && justUnder[0] == '\0',
        "and one byte less is a refusal, because the terminator has to go somewhere");
}

// ═══════════════════════════════════════════════════════════════════════════════════════
// §4.4.a — the exact strings
// ═══════════════════════════════════════════════════════════════════════════════════════
//
// The expectations are restated here from §4.4.a rather than read back from the module, so this
// fails if either side changes. What would break it: any edit to a device class, unit or state
// class; a diagnostics entity losing its entity_category; a unit re-encoded out of UTF-8.
void exactStringTests() {
  std::printf("\n[§4.4.a — device_class, unit_of_measurement, state_class, entity_category]\n");

  struct Expected {
    HaEntity entity;
    const char* deviceClass;  // nullptr where §4.4.a specifies none
    const char* unit;         // nullptr for undersampling, which is a bitmap
    const char* stateClass;
    bool diagnostic;
    const char* valueKey;
    uint8_t precision;
  };
  // The value keys are restated from the PUBLISHER's payload builders (MqttPublisher::formatSensor
  // and formatDiagnostics), not from this module, because that is the end of the contract that has
  // to match. A rename on either side breaks this table, which is the point: the alternative is an
  // entity that appears in HA and reads `unknown` forever with nothing logged anywhere.
  const Expected kExpected[] = {
      {HaEntity::SensorFlow, "volume_flow_rate", "L/min", "measurement", false, "flow", 2},
      {HaEntity::SensorSessionVolume, "water", "L", "total_increasing", false, "session", 1},
      {HaEntity::SensorLifetimeVolume, "water", kUnitCubicMetres, "total_increasing", false,
       "total", 3},
      {HaEntity::BoardTemperature, "temperature", kUnitDegreesCelsius, "measurement", false,
       "tempC", 1},
      {HaEntity::PollingRate, nullptr, "kHz", "measurement", true, "pollingRateKhz", 2},
      {HaEntity::Undersampling, nullptr, nullptr, "measurement", true, "undersampling", 0},
      {HaEntity::WifiRssi, nullptr, "dBm", "measurement", true, "rssi", 0},
  };

  const HaDiscovery ha = defaultDiscovery();
  for (const Expected& e : kExpected) {
    const HaEntityRef ref =
        plc::haEntityIsPerSensor(e.entity) ? sensorRef(e.entity, 2) : globalRef(e.entity);
    const std::string payload = payloadOf(ha, ref);
    const char* json = payload.c_str();
    std::printf("    %s\n", plc::haEntityObjectIdSuffix(e.entity));

    checkStr(plc::haEntityStateClass(e.entity), e.stateClass, "state_class");
    checkStr(plc::haEntityValueKey(e.entity), e.valueKey, "value_json key");

    if (e.deviceClass == nullptr) {
      check(plc::haEntityDeviceClass(e.entity) == nullptr && !contains(json, "device_class"),
            "no device_class, and the key is OMITTED rather than emitted empty");
    } else {
      checkStr(plc::haEntityDeviceClass(e.entity), e.deviceClass, "device_class");
      std::string key = "\"device_class\":\"";
      key += e.deviceClass;
      key += "\"";
      check(contains(json, key.c_str()), "and it reaches the payload verbatim");
    }

    if (e.unit == nullptr) {
      check(plc::haEntityUnit(e.entity) == nullptr && !contains(json, "unit_of_measurement"),
            "no unit, and the key is omitted");
    } else {
      checkStr(plc::haEntityUnit(e.entity), e.unit, "unit_of_measurement");
      std::string key = "\"unit_of_measurement\":\"";
      key += e.unit;
      key += "\"";
      check(contains(json, key.c_str()), "and it reaches the payload verbatim");
    }

    std::string stateKey = "\"state_class\":\"";
    stateKey += e.stateClass;
    stateKey += "\"";
    check(contains(json, stateKey.c_str()), "state_class reaches the payload");

    std::string tmpl = "\"value_template\":\"{{ value_json.";
    tmpl += e.valueKey;
    tmpl += " }}\"";
    check(contains(json, tmpl.c_str()),
          "the template reads exactly the key haEntityValueKey advertises");

    check(plc::haEntityIsDiagnostic(e.entity) == e.diagnostic, "diagnostic classification");
    check(contains(json, "\"entity_category\":\"diagnostic\"") == e.diagnostic,
          "entity_category is present only for diagnostics — the sole legal value (§4.4.a)");
    // The VALUE, not merely the key. Emitting 0 for a litres-per-minute reading is exactly the
    // behaviour §4.4 records as the thing suggested_display_precision exists to prevent, so a key
    // present with the wrong number is no better than an absent one.
    char precisionKey[64] = {};
    std::snprintf(precisionKey, sizeof(precisionKey), "\"suggested_display_precision\":%u",
                  static_cast<unsigned>(e.precision));
    check(plc::haEntityPrecision(e.entity) == e.precision && contains(json, precisionKey),
          "suggested_display_precision carries the right number of decimals, not just the key");
  }

  std::printf("    the unit bytes themselves\n");
  const char* cubic = plc::haEntityUnit(HaEntity::SensorLifetimeVolume);
  check(std::strlen(cubic) == 3 && static_cast<unsigned char>(cubic[0]) == 'm' &&
            static_cast<unsigned char>(cubic[1]) == 0xC2 &&
            static_cast<unsigned char>(cubic[2]) == 0xB3,
        "m3 is the three bytes 'm' C2 B3 — a Latin-1 re-encode would leave a bare B3");
  const char* degrees = plc::haEntityUnit(HaEntity::BoardTemperature);
  check(std::strlen(degrees) == 3 && static_cast<unsigned char>(degrees[0]) == 0xC2 &&
            static_cast<unsigned char>(degrees[1]) == 0xB0 &&
            static_cast<unsigned char>(degrees[2]) == 'C',
        "degC is the three bytes C2 B0 'C'");
  const std::string lifetime = payloadOf(ha, sensorRef(HaEntity::SensorLifetimeVolume, 7));
  check(contains(lifetime.c_str(), kUnitCubicMetres),
        "and those exact bytes survive into the payload, not just into the accessor");
}

// ═══════════════════════════════════════════════════════════════════════════════════════
// R4.4.4 — the Water dashboard, which is the highest-value detail in the feature
// ═══════════════════════════════════════════════════════════════════════════════════════
//
// Asserted as SET MEMBERSHIP against §4.4.a's permitted list, not against the literal we happen to
// emit. A later change of unit to something outside the `water` list then fails here rather than
// passing and quietly dropping the entity out of long-term statistics.
void waterDashboardTests() {
  std::printf("\n[R4.4.4 — device_class water AND total_increasing AND a water unit, together]\n");

  const char* const kWaterUnits[] = {"L", "gal", kUnitCubicMetres, kUnitCubicFeetPerMin,
                                     "CCF", "MCF"};
  const HaEntity kCumulative[] = {HaEntity::SensorSessionVolume, HaEntity::SensorLifetimeVolume};
  const HaDiscovery ha = defaultDiscovery();

  for (HaEntity entity : kCumulative) {
    const std::string payload = payloadOf(ha, sensorRef(entity, 0));
    const char* json = payload.c_str();
    const char* unit = plc::haEntityUnit(entity);

    bool unitPermitted = false;
    for (const char* permitted : kWaterUnits) {
      // ft3/min is in the volume_flow_rate list, not the water list; it is present above only so a
      // cubed unit from the WRONG list cannot pass. Compare against the water entries only.
      if (std::strcmp(permitted, kUnitCubicFeetPerMin) == 0) continue;
      if (std::strcmp(unit, permitted) == 0) unitPermitted = true;
    }
    std::printf("    %s\n", plc::haEntityObjectIdSuffix(entity));
    check(std::strcmp(plc::haEntityDeviceClass(entity), "water") == 0, "device_class is water");
    check(std::strcmp(plc::haEntityStateClass(entity), "total_increasing") == 0,
          "state_class is total_increasing — monotonic, and HA handles the reset itself");
    check(unitPermitted, "and the unit is one HA permits for device_class water");
    check(contains(json, "\"device_class\":\"water\"") &&
              contains(json, "\"state_class\":\"total_increasing\""),
          "all three travel in the SAME payload, which is what R4.4.4 actually requires");
  }

  // The flow entity must NOT look like a dashboard total: L/min with total_increasing would make HA
  // accumulate a rate.
  check(std::strcmp(plc::haEntityStateClass(HaEntity::SensorFlow), "measurement") == 0,
        "instantaneous flow stays a measurement, so nothing accumulates a rate");
}

// ═══════════════════════════════════════════════════════════════════════════════════════
// §4.4.b / §4.2 — topics
// ═══════════════════════════════════════════════════════════════════════════════════════
void topicTests() {
  std::printf("\n[§4.4.b — one topic per entity; §4.2 — one state topic per sensor]\n");

  const HaDiscovery ha = defaultDiscovery();

  checkStr(topicOf(ha, sensorRef(HaEntity::SensorFlow, 0)).c_str(),
           "homeassistant/sensor/wfm_a1b2c3/s1_flow/config",
           "the discovery topic is <prefix>/sensor/<node_id>/<object_id>/config");
  checkStr(topicOf(ha, globalRef(HaEntity::PollingRate)).c_str(),
           "homeassistant/sensor/wfm_a1b2c3/polling_rate/config",
           "and a per-device entity uses the same shape");
  checkStr(stateTopicOf(ha, sensorRef(HaEntity::SensorFlow, 0)).c_str(),
           "watermeter/a1b2c3/sensor/1/state",
           "the state topic is 1-based, as §4.2 writes it");
  checkStr(stateTopicOf(ha, globalRef(HaEntity::Undersampling)).c_str(),
           "watermeter/a1b2c3/diagnostics/state", "diagnostics share one state topic");

  char availability[plc::kHaStateTopicBytes] = {};
  ha.availabilityTopic(availability, sizeof(availability));
  checkStr(availability, "watermeter/a1b2c3/status", "availability is <base>/status (R4.5.1)");

  char status[plc::kHaStateTopicBytes] = {};
  ha.haStatusTopic(status, sizeof(status));
  checkStr(status, "homeassistant/status",
           "and the topic to SUBSCRIBE to follows the discovery prefix (R4.4.7)");

  // §4.2's whole argument: 8 publishes per cycle rather than 40. That is only true if the three
  // per-sensor entities really do share one topic and separate themselves by template.
  const HaEntity kPerSensor[] = {HaEntity::SensorFlow, HaEntity::SensorSessionVolume,
                                 HaEntity::SensorLifetimeVolume};
  const std::string shared = stateTopicOf(ha, sensorRef(HaEntity::SensorFlow, 4));
  bool sameTopic = true;
  bool distinctTemplates = true;
  std::string seen[3];
  for (std::size_t i = 0; i < 3; ++i) {
    const HaEntityRef ref = sensorRef(kPerSensor[i], 4);
    sameTopic = sameTopic && stateTopicOf(ha, ref) == shared;
    const std::string payload = payloadOf(ha, ref);
    std::string embedded = "\"state_topic\":\"";
    embedded += shared;
    embedded += "\"";
    sameTopic = sameTopic && contains(payload.c_str(), embedded.c_str());
    seen[i] = plc::haEntityValueKey(kPerSensor[i]);
  }
  for (std::size_t i = 0; i < 3; ++i) {
    for (std::size_t j = i + 1; j < 3; ++j) {
      if (seen[i] == seen[j]) distinctTemplates = false;
    }
  }
  checkStr(shared.c_str(), "watermeter/a1b2c3/sensor/5/state", "sensor 5's shared state topic");
  check(sameTopic,
        "all three of its entities advertise that ONE topic — §4.2's 8 publishes, not 40");
  check(distinctTemplates, "and pick their value out of it with three distinct templates");

  // objectId() and uniqueId() exist so the MQTT client can name a command topic or a log line
  // without rebuilding the string. They must agree with what the topic and the payload carry, or
  // there are two spellings of one identity and a renamed entity in HA would come back.
  HaEntityRef longest = sensorRef(HaEntity::SensorLifetimeVolume, 7);
  char oid[plc::kHaMaxObjectIdBytes + 1] = {};
  ha.objectId(longest, oid, sizeof(oid));
  checkStr(oid, "s8_lifetime", "objectId() is the fragment the discovery topic carries");
  check(contains(topicOf(ha, longest).c_str(), "/s8_lifetime/config"),
        "and the topic really does carry it");
  char uid[plc::kHaMaxNodeIdBytes + plc::kHaMaxObjectIdBytes + 2] = {};
  ha.uniqueId(longest, uid, sizeof(uid));
  std::string embeddedId = "\"unique_id\":\"";
  embeddedId += uid;
  embeddedId += "\"";
  check(contains(payloadOf(ha, longest).c_str(), embeddedId.c_str()),
        "and uniqueId() is exactly the unique_id the payload carries, not a second spelling");

  // kHaDiscoveryTopicBytes is computed from kHaMaxObjectIdBytes, so that cap has to be true for
  // every entity — otherwise the topic buffer is a guess and a long object id truncates silently.
  HaEntityRef all[plc::kHaMaxEntities];
  const std::size_t total = ha.enumerateEntities(0xFFFF, all, plc::kHaMaxEntities);
  bool everyObjectIdFits = total == plc::kHaMaxEntities;
  for (std::size_t i = 0; i < total; ++i) {
    char buffer[plc::kHaMaxObjectIdBytes + 1] = {};
    everyObjectIdFits = everyObjectIdFits && ha.objectId(all[i], buffer, sizeof(buffer)).complete;
  }
  check(everyObjectIdFits, "every entity's object_id fits kHaMaxObjectIdBytes, as the topic buffer assumes");

  const std::string flow = payloadOf(ha, sensorRef(HaEntity::SensorFlow, 0));
  check(contains(flow.c_str(), "\"unique_id\":\"wfm_a1b2c3_s1_flow\""),
        "unique_id is <node_id>_<object_id>, so an HA rename survives a restart (R4.4.3)");
  check(contains(flow.c_str(), "\"identifiers\":[\"wfm_a1b2c3\"]") &&
            contains(flow.c_str(), "\"manufacturer\":\"M5Stack\"") &&
            contains(flow.c_str(), "\"model\":\"StampPLC\""),
        "and the device block repeats in every payload, which is what groups them (R4.4.2)");

  // A caller looping kinds x sensors would otherwise emit eight identical polling_rate entities
  // that collide on one object_id and silently overwrite each other.
  char scratch[plc::kMqttOutBufferSize] = {};
  check(!ha.discoveryPayload(sensorRef(HaEntity::PollingRate, 3), scratch, sizeof(scratch)).complete,
        "a per-device entity with a sensor index is REFUSED, not silently duplicated");
  check(!ha.discoveryPayload(sensorRef(HaEntity::SensorFlow, plc::kNumSensors), scratch,
                             sizeof(scratch))
             .complete,
        "and so is a sensor index past the eighth sensor");
}

// ═══════════════════════════════════════════════════════════════════════════════════════
// Every payload is valid JSON, including when a setting contains something hostile
// ═══════════════════════════════════════════════════════════════════════════════════════
//
// What would break this test: removing the string escaping. A base topic is operator-supplied text
// arriving from a web form or a Modbus block write, so a quote or a backslash in it is reachable,
// and an unescaped one turns the payload into something HA drops without a word.
void jsonValidityTests() {
  std::printf("\n[valid JSON — balanced, escaped, and hostile input included]\n");

  const HaDiscovery ha = defaultDiscovery();
  HaEntityRef entities[plc::kHaMaxEntities];
  const std::size_t count = ha.enumerateEntities(0xFFFF, entities, plc::kHaMaxEntities);
  bool allValid = true;
  for (std::size_t i = 0; i < count; ++i) {
    const std::string payload = payloadOf(ha, entities[i]);
    if (payload.empty() || !jsonWellFormed(payload.c_str())) allValid = false;
  }
  check(count == plc::kHaMaxEntities && allValid,
        "all 28 payloads are well-formed: braces, brackets and quotes all balance");

  check(!jsonWellFormed("{\"a\":\"b}"), "and the checker itself rejects an unterminated string");
  check(!jsonWellFormed("{\"a\":1,}"), "and a trailing comma");

  HaDeviceIdentity hostile;
  hostile.nodeId = "wfm_a1b2c3";
  hostile.name = "He said \"no\" \\ then left";
  hostile.swVersion = "1.0\t-dev";
  hostile.configurationUrl = "http://host/?q=\"1\"";
  HaDiscovery rough;
  check(rough.configure(hostile, "homeassistant", "water\"meter/a1"),
        "a quote and a backslash in operator-supplied text are accepted, not rejected");
  const std::string payload = payloadOf(rough, sensorRef(HaEntity::SensorFlow, 0));
  check(!payload.empty() && jsonWellFormed(payload.c_str()),
        "and the payload is STILL valid JSON — the strings are escaped");
  check(contains(payload.c_str(), "\\\"no\\\"") && contains(payload.c_str(), "\\\\"),
        "with the quote and the backslash escaped rather than dropped");
  check(contains(payload.c_str(), "\\t") || contains(payload.c_str(), "\\u0009"),
        "and a control byte escaped rather than emitted raw");
}

// ═══════════════════════════════════════════════════════════════════════════════════════
// Optional keys are omitted, never emitted empty
// ═══════════════════════════════════════════════════════════════════════════════════════
//
// What would break this test: emitting "configuration_url":"" before WiFi has an address, or
// "sw_version":"". A payload HA rejects produces no entity and no log — the same failure class as
// the buffer overflow, reached from a different direction.
void optionalKeyTests() {
  std::printf("\n[optional keys — omitted when unknown, so HA never sees an empty value]\n");

  HaDeviceIdentity bare;
  bare.nodeId = "wfm_a1b2c3";
  HaDiscovery ha;
  check(ha.configure(bare, "homeassistant", "watermeter/a1b2c3"),
        "a device with no version, URL or name configures — that is the pre-WiFi state");
  const std::string payload = payloadOf(ha, sensorRef(HaEntity::SensorFlow, 0));
  const char* json = payload.c_str();
  check(!payload.empty() && jsonWellFormed(json), "its payload is still valid JSON");
  check(!contains(json, "sw_version"), "sw_version is absent rather than empty");
  check(!contains(json, "configuration_url"), "configuration_url is absent rather than empty");
  check(!contains(json, "\"\""), "and no key anywhere carries an empty string");
  check(contains(json, "\"name\":\"Water Flow Meter\""),
        "the device still has a name, because a nameless device block is unusable");
}

// ═══════════════════════════════════════════════════════════════════════════════════════
// R4.4.7 — the birth message
// ═══════════════════════════════════════════════════════════════════════════════════════
//
// What would break this test: a prefix match instead of an exact one, a case-insensitive payload
// compare, or accepting our own <base>/status. The last is the dangerous one: our LWT publishes
// "online" on that topic (R4.5.1), so treating it as a birth would make each republish trigger the
// next one.
void birthMessageTests() {
  std::printf("\n[R4.4.7 — republish on HA's birth message, and on nothing that resembles it]\n");

  const HaDiscovery ha = defaultDiscovery();
  check(ha.isHaBirth("homeassistant/status", "online"),
        "online on <prefix>/status is the birth message");
  check(!ha.isHaBirth("homeassistant/status", "offline"), "offline is not");
  check(!ha.isHaBirth("homeassistant/status", "ONLINE"),
        "and neither is a different spelling — HA sends exactly \"online\"");
  check(!ha.isHaBirth("homeassistant/status", ""), "nor an empty payload");
  check(!ha.isHaBirth("watermeter/a1b2c3/status", "online"),
        "our OWN availability topic is not a birth message, or every republish causes another");
  check(!ha.isHaBirth("homeassistant/status/extra", "online"),
        "a longer topic is not a birth message either — the compare is exact");
  check(!ha.isHaBirth("homeassistant/sensor/wfm_a1b2c3/s1_flow/config", "online"),
        "and neither is one of our own retained discovery topics coming back to us");
  check(!ha.isHaBirth(nullptr, "online") && !ha.isHaBirth("homeassistant/status", nullptr),
        "a null topic or payload is not a birth message and does not except");

  HaDeviceIdentity device;
  device.nodeId = "wfm_a1b2c3";
  HaDiscovery custom;
  custom.configure(device, "ha-test", "watermeter/a1b2c3");
  check(custom.isHaBirth("ha-test/status", "online") &&
            !custom.isHaBirth("homeassistant/status", "online"),
        "the status topic follows the CONFIGURED prefix, not the default one");
}

// ═══════════════════════════════════════════════════════════════════════════════════════
// R4.4.6 — when discovery is republished
// ═══════════════════════════════════════════════════════════════════════════════════════
//
// The bitmap checks assert a CHANGE rather than a value: the same bitmap twice must yield None the
// second time. A policy that always said SensorsChanged would pass a value assertion and republish
// 28 retained messages on every telemetry cycle.
void republishPolicyTests() {
  std::printf("\n[R4.4.6 — first connect, reconnect, birth, and a bitmap that moved]\n");

  const HaDiscovery ha = defaultDiscovery();
  HaRepublishPolicy policy;

  check(policy.onSensorBitmap(0x01) == HaRepublishReason::None,
        "a bitmap change before the first connect asks for nothing — the connect will publish");
  check(policy.onConnected() == HaRepublishReason::FirstConnect, "the first connect publishes");
  check(policy.onSensorBitmap(0x05) == HaRepublishReason::None,
        "a change between that connect and its publish adds nothing — the publish owes it");
  policy.notePublished(0x01);
  check(policy.everPublished() && policy.publishedBitmap() == 0x01,
        "and the published bitmap is recorded only once the publish happened");

  check(policy.onSensorBitmap(0x01) == HaRepublishReason::None,
        "the same bitmap again asks for nothing");
  check(policy.onSensorBitmap(0x03) == HaRepublishReason::SensorsChanged,
        "a sensor appearing asks for a republish, so its entity appears (R4.4.6)");
  check(policy.onSensorBitmap(0x03) == HaRepublishReason::SensorsChanged,
        "and keeps asking until the publish is noted — a -1 publish must not be forgotten");
  policy.notePublished(0x03);
  check(policy.onSensorBitmap(0x03) == HaRepublishReason::None, "after which it goes quiet");

  check(policy.onStatusMessage(ha, "homeassistant/status", "online") == HaRepublishReason::HaBirth,
        "HA restarting republishes everything (R4.4.7)");
  check(policy.onStatusMessage(ha, "watermeter/a1b2c3/status", "online") ==
            HaRepublishReason::None,
        "our own availability publish does not");

  policy.noteDisconnected();
  check(policy.onSensorBitmap(0xFF) == HaRepublishReason::None,
        "a bitmap change while disconnected asks for nothing — there is nowhere to publish");
  check(policy.onConnected() == HaRepublishReason::Reconnect,
        "and the reconnect republishes regardless, which is R4.4.6's real guarantee");

  checkStr(plc::haRepublishReasonText(HaRepublishReason::SensorsChanged), "sensor bitmap changed",
           "every reason has text, so a storm is visible in the log");
}

// ═══════════════════════════════════════════════════════════════════════════════════════
// R4.4.6 — only connected sensors get entities
// ═══════════════════════════════════════════════════════════════════════════════════════
void enumerationTests() {
  std::printf("\n[R4.4.6 — the entity set follows the connected-sensor bitmap]\n");

  const HaDiscovery ha = defaultDiscovery();
  HaEntityRef entities[plc::kHaMaxEntities];

  // Four per-device readings plus §4.4.1's three command buttons. The buttons are deliberately NOT
  // conditioned on the bitmap: a device with nothing wired can still be told to republish, and
  // `reset-totals` is most wanted exactly when the channels have just been turned off.
  constexpr std::size_t kPerDevice = 4 + 3;
  const std::size_t none = ha.enumerateEntities(0x0000, entities, plc::kHaMaxEntities);
  check(none == kPerDevice,
        "with no sensors connected the per-device entities and the three buttons are published");
  std::size_t buttons = 0;
  for (std::size_t i = 0; i < none; ++i) {
    if (plc::haEntityIsButton(entities[i].kind)) ++buttons;
  }
  check(buttons == 3, "and all three buttons are there — §4.4.1 has exactly three topics");

  const std::size_t one = ha.enumerateEntities(0x0001, entities, plc::kHaMaxEntities);
  check(one == kPerDevice + 3, "one sensor adds exactly its three entities");
  bool onlySensorOne = true;
  int perSensor = 0;
  for (std::size_t i = 0; i < one; ++i) {
    if (!plc::haEntityIsPerSensor(entities[i].kind)) continue;
    ++perSensor;
    if (entities[i].sensor != 0) onlySensorOne = false;
  }
  check(perSensor == 3 && onlySensorOne, "and they are sensor 1's, not some other sensor's");

  // A gapped bitmap, because "count the bits" and "use the right bits" are different bugs.
  const std::size_t gapped = ha.enumerateEntities(0b1000'0010, entities, plc::kHaMaxEntities);
  bool sensorTwo = false;
  bool sensorEight = false;
  bool nothingElse = true;
  for (std::size_t i = 0; i < gapped; ++i) {
    if (!plc::haEntityIsPerSensor(entities[i].kind)) continue;
    if (entities[i].sensor == 1) sensorTwo = true;
    else if (entities[i].sensor == 7) sensorEight = true;
    else nothingElse = false;
  }
  check(gapped == kPerDevice + 6 && sensorTwo && sensorEight && nothingElse,
        "sensors 2 and 8 connected yields sensors 2 and 8 — not 1 and 2");

  // The caller's array must never be overrun, whatever it claims to be.
  HaEntityRef tiny[5];
  const std::size_t clipped = ha.enumerateEntities(0xFFFF, tiny, 4);
  check(clipped == 4, "a capacity smaller than the set is honoured rather than overrun");

  HaDiscovery unconfigured;
  check(unconfigured.enumerateEntities(0xFFFF, entities, plc::kHaMaxEntities) == 0,
        "and an unconfigured object enumerates nothing at all");
}

// ═══════════════════════════════════════════════════════════════════════════════════════
// configure() refuses what it cannot publish correctly
// ═══════════════════════════════════════════════════════════════════════════════════════
//
// What would break this test: truncating instead of rejecting. A truncated base topic is a VALID
// topic, so the device would publish cheerfully to the wrong place, and the operator would see an
// empty Home Assistant and a panel reporting MQTT connected.
void configValidationTests() {
  std::printf("\n[configure — rejects what would publish to the wrong topic]\n");

  HaDeviceIdentity device;
  device.nodeId = "wfm_a1b2c3";

  HaDiscovery unconfigured;
  char scratch[plc::kMqttOutBufferSize];
  std::memset(scratch, 'x', sizeof(scratch));
  const HaBuildResult built = unconfigured.discoveryPayload(sensorRef(HaEntity::SensorFlow, 0),
                                                            scratch, sizeof(scratch));
  check(!unconfigured.configured() && !built.complete && built.required == 0 && scratch[0] == '\0',
        "an unconfigured object builds nothing and leaves the buffer empty");

  const std::string longPrefix(plc::kHaMaxPrefixBytes + 1, 'p');
  const std::string longBase(plc::kHaMaxBaseTopicBytes + 1, 'b');
  const std::string longNode(plc::kHaMaxNodeIdBytes + 1, 'n');

  HaDiscovery ha;
  check(!ha.configure(device, longPrefix.c_str(), "watermeter/a1"),
        "a prefix one byte over its cap is REJECTED, not truncated");
  check(!ha.configure(device, "homeassistant", longBase.c_str()),
        "and so is an over-long base topic");
  HaDeviceIdentity longId = device;
  longId.nodeId = longNode.c_str();
  check(!ha.configure(longId, "homeassistant", "watermeter/a1"), "and an over-long node id");
  check(!ha.configured(), "and none of those left the object half-configured");

  check(!ha.configure(device, "", "watermeter/a1") &&
            !ha.configure(device, "homeassistant", ""),
        "an empty prefix or base topic is refused — it would publish to a malformed topic");
  HaDeviceIdentity noId = device;
  noId.nodeId = nullptr;
  check(!ha.configure(noId, "homeassistant", "watermeter/a1"),
        "and so is a missing node id, which is the whole unique_id (R4.4.3)");

  check(!ha.configure(device, "home+assistant", "watermeter/a1") &&
            !ha.configure(device, "homeassistant", "water#meter/a1"),
        "MQTT wildcards are refused: they are illegal in a topic a client publishes to");
  check(!ha.configure(device, "homeassistant", "watermeter/a1/") &&
            !ha.configure(device, "homeassistant", "/watermeter/a1"),
        "and a leading or trailing / is refused, which would leave an empty topic level");
  check(!ha.configure(device, "homeassistant", "water meter/a1"),
        "a space in a topic is refused — legal in MQTT, a trap everywhere else");

  HaDeviceIdentity slashed = device;
  slashed.nodeId = "wfm/a1b2c3";
  check(!ha.configure(slashed, "homeassistant", "watermeter/a1"),
        "a / in the node id is refused: it would silently restructure the discovery topic");
  HaDeviceIdentity spaced = device;
  spaced.nodeId = "wfm a1b2c3";
  check(!ha.configure(spaced, "homeassistant", "watermeter/a1"), "and so is a space in it");

  HaDeviceIdentity fine = device;
  fine.nodeId = "WFM_a1-b2_c3";
  check(ha.configure(fine, "homeassistant", "watermeter/a1b2c3") && ha.configured(),
        "letters, digits, underscore and dash are accepted, which is all a MAC suffix needs");
}

// ═══════════════════════════════════════════════════════════════════════════════════════
// Owner decision 5A — ONE definition of a base topic, and this module defers to it
// ═══════════════════════════════════════════════════════════════════════════════════════
//
// The divergence this exists for, measured on the pre-change tree by handing the same string to
// HaDiscovery::configure and MqttPublisher::configure. Six classes disagreed:
//
//   input                     HaDiscovery     MqttPublisher
//   "watermeter/a1b2c3/"      reject          accept (silently stripped)
//   "watermeter/a1b2c3///"    reject          accept (silently stripped)
//   "/watermeter/a1b2c3"      reject          accept
//   "water meter/a1"          reject          accept
//   49..109 bytes             reject          accept
//   0x7f, or any byte >= 0x80 accept          reject
//
// plus "watermeter//a1", which both accepted and the canonical rule refuses.
//
// The first five are the ones that shipped a symptom: the publisher took the topic, published
// telemetry to it, and this module refused the identical string — so NOT ONE entity was created
// while the MQTT state register read connected and nothing was logged at either end (§4.4.7). It
// presents as a broker fault. The sixth is the mirror image: entities advertised on a base topic
// nothing ever publishes to, so every one sits at `unknown` forever.
//
// The assertion is agreement with the NAMED canonical rule — plc::NetSettings::isValidBaseTopic —
// rather than with the other module, and the expectation is obtained by CALLING it, never by
// restating a bool. The publisher half of this corpus is in mqtt_publisher_test.cpp; agreement with
// one canonical rule on both sides is transitive and needs no cross-link between two leaf tests
// (run.sh keeps them dependency-free on purpose).
//
// What would break this test: reinstating a private topic rule in ha_discovery.cpp — the old
// `topicCharsValid` accepted three of the rows below that the canonical rule refuses.
void baseTopicAgreementTests() {
  std::printf("\n[owner decision 5A — configure() accepts exactly isValidBaseTopic's set]\n");

  struct Case {
    const char* topic;
    const char* what;
  };

  const std::string atCap(plc::kHaMaxBaseTopicBytes, 'x');
  const std::string overCap(plc::kHaMaxBaseTopicBytes + 1, 'x');

  const Case cases[] = {
      {"watermeter/a1b2c3", "a plain topic"},
      {"watermeter/a1b2c3/", "DIVERGED: trailing '/' — refused here, accepted and stripped there"},
      {"watermeter/a1b2c3///", "DIVERGED: three trailing '/'"},
      {"/watermeter/a1b2c3", "DIVERGED: leading '/' — refused here, accepted there"},
      {"water meter/a1", "DIVERGED: a space — refused here, accepted there"},
      {"watermeter//a1", "an interior '//' — both modules used to accept it"},
      {"watermeter/a1\x7f", "DIVERGED: DEL 0x7f — accepted here, refused there"},
      {"watermeter/caf\xC3\xA9", "DIVERGED: a UTF-8 byte >= 0x80 — accepted here, refused there"},
      {"watermeter/a1\x01", "a control byte, which both always refused"},
      {"watermeter/+/a", "a '+' wildcard"},
      {"watermeter/#", "a '#' wildcard"},
      {"water\"meter/a1", "a double quote, which is legal and must stay accepted"},
      {"", "the empty string"},
      {"///", "a topic made only of slashes"},
      {nullptr, "a null pointer"},
      {atCap.c_str(), "a topic exactly filling the field"},
      {overCap.c_str(), "DIVERGED: one byte over the field — refused here, accepted there"},
  };

  HaDeviceIdentity device;
  device.nodeId = "wfm_a1b2c3";

  for (const Case& c : cases) {
    HaDiscovery ha;
    const bool accepted = ha.configure(device, "homeassistant", c.topic);
    const bool canonical = plc::NetSettings::isValidBaseTopic(c.topic);
    check(accepted == canonical, c.what);
    char scratch[plc::kHaStateTopicBytes] = {};
    if (accepted) {
      // Accepted VERBATIM. A module that agreed about the verdict and then repaired the string would
      // advertise a topic the publisher does not publish to, which is the same defect wearing
      // agreement as a disguise.
      ha.availabilityTopic(scratch, sizeof(scratch));
      check(std::string(scratch) == std::string(c.topic) + "/status",
            "  and <base>/status is built from it unrepaired");
    } else {
      // The silent failure, stated as a mechanism: a refusal produces no entities at all, which is
      // exactly what the operator saw while MQTT reported connected.
      HaEntityRef entities[plc::kHaMaxEntities];
      check(!ha.configured() && ha.enumerateEntities(0xFFFF, entities, plc::kHaMaxEntities) == 0,
            "  and a refusal yields ZERO entities — the symptom 5A exists to remove");
    }
  }

  // ── The discovery prefix goes through the same rule, plus its OWN tighter cap ─────────
  //
  // 32 bytes (its register field) against the validator's 48, so the cap is reachable and is the one
  // check ha_discovery.cpp still makes for itself. The expectation is the conjunction of the two
  // rules, written as the conjunction rather than as a verdict per row.
  const std::string prefixAtCap(plc::kHaMaxPrefixBytes, 'P');
  const std::string prefixOverCap(plc::kHaMaxPrefixBytes + 1, 'P');
  const Case prefixCases[] = {
      {"homeassistant", "the default prefix"},
      {"ha-test", "a renamed prefix"},
      {"home//assistant", "an interior '//' — <prefix>//status is never HA's birth topic (R4.4.7)"},
      {"homeassistant/", "a trailing '/'"},
      {"/homeassistant", "a leading '/'"},
      {"home assistant", "a space"},
      // Split for the reason ha_discovery.cpp splits its degree sign: "\x7fa" would parse as one
      // out-of-range hex escape, which -Werror refuses.
      {"home\x7f" "assistant", "DEL 0x7f, which the old private rule let through"},
      {"homeassistant\xC3\xA9", "a UTF-8 byte >= 0x80, likewise"},
      {"home+assistant", "a '+' wildcard"},
      {"", "the empty string"},
      {nullptr, "a null pointer"},
      {prefixAtCap.c_str(), "a prefix exactly filling its 32-byte field"},
      {prefixOverCap.c_str(), "one byte over it — refused by the module's own cap, not the rule"},
  };

  for (const Case& c : prefixCases) {
    HaDiscovery ha;
    const bool accepted = ha.configure(device, c.topic, "watermeter/a1b2c3");
    const bool canonical = plc::NetSettings::isValidBaseTopic(c.topic) &&
                           std::strlen(c.topic == nullptr ? "" : c.topic) <= plc::kHaMaxPrefixBytes;
    check(accepted == canonical, c.what);
  }

  // The floor under both loops. If isValidBaseTopic degenerated to "return true", every row above
  // would still pass as long as configure() degenerated with it — so the three verdicts this module
  // actually CHANGED are pinned absolutely, and the ones it must not have changed with them.
  HaDiscovery ha;
  check(!ha.configure(device, "homeassistant", "watermeter/a1\x7f") &&
            !ha.configure(device, "homeassistant", "watermeter/caf\xC3\xA9") &&
            !ha.configure(device, "homeassistant", "watermeter//a1"),
        "absolutely: 0x7f, a byte >= 0x80 and an interior '//' are refused — all three USED to pass");
  check(ha.configure(device, "homeassistant", "water\"meter/a1"),
        "while a quote is still accepted, because the payload escapes it rather than refusing it");
  check(ha.configure(device, "homeassistant", std::string(plc::kHaMaxBaseTopicBytes, 'x').c_str()),
        "and a topic filling the whole field still configures — R4.4.8's worst case needs it");
}


// ═══════════════════════════════════════════════════════════════════════════════════════
// §4.4.1 — the command buttons
// ═══════════════════════════════════════════════════════════════════════════════════════
void commandButtonTests() {
  std::printf("\n[§4.4.1 — the three commands are `button` entities that can actually be pressed]\n");

  const HaDiscovery ha = defaultDiscovery();
  const HaEntityRef session{HaEntity::ButtonResetSession, 0};
  const HaEntityRef totals{HaEntity::ButtonResetTotals, 0};
  const HaEntityRef republish{HaEntity::ButtonRepublish, 0};

  // THE assertion of this file. Home Assistant's default press payload is `PRESS`; R4.4.1 accepts
  // only `RESET`. A button discovered without `payload_press` therefore ships broken — it appears,
  // it presses, and the device answers `bad-payload` where nobody is looking. Everything else here
  // is structure; this is the one that was going to be wrong.
  const std::string s = payloadOf(ha, session);
  const std::string t = payloadOf(ha, totals);
  check(contains(s.c_str(), "\"payload_press\":\"RESET\""),
        "reset-session carries payload_press RESET, not Home Assistant's default PRESS");
  check(contains(t.c_str(), "\"payload_press\":\"RESET\""), "and so does reset-totals");
  checkStr(plc::haEntityPressPayload(HaEntity::ButtonResetSession), plc::MqttCommandRouter::kResetPayload,
           "and that payload is character-for-character the one the router demands");

  // The topic the button publishes to has to be the topic the router matches. Compared through
  // `commandFor` rather than against a literal, so the two ends are checked against each other and
  // not both against my typing.
  const std::string cmdTopic = std::string("watermeter/a1b2c3") +
                               plc::haEntityCommandSuffix(HaEntity::ButtonResetSession);
  check(contains(s.c_str(), (std::string("\"command_topic\":\"") + cmdTopic + "\"").c_str()),
        "the command topic is <base>/cmd/reset-session");
  check(plc::MqttCommandRouter::commandFor(cmdTopic.c_str(), "watermeter/a1b2c3") ==
            plc::MqttCommand::ResetSession,
        "and the router resolves that exact string back to the command — both ends of §4.4.1 agree");
  check(plc::MqttCommandRouter::commandFor(
            (std::string("watermeter/a1b2c3") + plc::haEntityCommandSuffix(HaEntity::ButtonResetTotals))
                .c_str(),
            "watermeter/a1b2c3") == plc::MqttCommand::ResetTotals,
        "reset-totals likewise");
  check(plc::MqttCommandRouter::commandFor(
            (std::string("watermeter/a1b2c3") + plc::haEntityCommandSuffix(HaEntity::ButtonRepublish))
                .c_str(),
            "watermeter/a1b2c3") == plc::MqttCommand::Republish,
        "and republish likewise");

  // The discovery topic addresses the `button` component. Published under `sensor/` the entity would
  // be created as a sensor with a command topic it cannot use — or rejected outright.
  const std::string topic = topicOf(ha, session);
  checkStr(topic.c_str(), "homeassistant/button/wfm_a1b2c3/cmd_reset_session/config",
           "the discovery topic addresses button/, not sensor/");

  // The reading keys must be ABSENT. Home Assistant rejects a payload carrying keys the component
  // does not define, and a rejected payload is an entity that silently never appears.
  for (const std::string& p : {s, t, payloadOf(ha, republish)}) {
    check(!contains(p.c_str(), "state_topic"), "a button payload has no state_topic");
    check(!contains(p.c_str(), "value_template"), "nor a value_template");
    check(!contains(p.c_str(), "state_class"), "nor a state_class");
    check(!contains(p.c_str(), "suggested_display_precision"), "nor a display precision");
    check(!contains(p.c_str(), "unit_of_measurement"), "nor a unit — a press is not a quantity");
    check(!contains(p.c_str(), "entity_category"),
          "and no entity_category: a reset button belongs in the dashboard, not the diagnostics fold");
    // What it must have: identity and availability, like every other entity.
    check(contains(p.c_str(), "\"availability_topic\":\"watermeter/a1b2c3/status\""),
          "but it shares the availability topic, so an offline device greys the button out");
    check(contains(p.c_str(), "\"device\":{\"identifiers\":[\"wfm_a1b2c3\"]"),
          "and the device block, so it groups with the sensors (R4.4.2)");
  }

  // `stateTopic` must refuse a button rather than handing back the diagnostics topic, which is a
  // real topic carrying real JSON: publishing a button "state" there would corrupt the diagnostics
  // payload and report success.
  char buffer[plc::kHaStateTopicBytes] = {};
  check(!ha.stateTopic(session, buffer, sizeof(buffer)).complete,
        "stateTopic refuses a button instead of returning <base>/diagnostics/state");
  check(buffer[0] == '\0', "and leaves the buffer empty rather than half-written");

  // Three distinct unique_ids, since one duplicated id silently collapses two buttons into one.
  const std::string ids[] = {payloadOf(ha, session), payloadOf(ha, totals), payloadOf(ha, republish)};
  check(contains(ids[0].c_str(), "\"unique_id\":\"wfm_a1b2c3_cmd_reset_session\"") &&
            contains(ids[1].c_str(), "\"unique_id\":\"wfm_a1b2c3_cmd_reset_totals\"") &&
            contains(ids[2].c_str(), "\"unique_id\":\"wfm_a1b2c3_cmd_republish\""),
        "each button has its own unique_id (R4.4.3)");
}

}  // namespace

int main() {
  std::printf("plc::HaDiscovery — Home Assistant MQTT discovery (§4.4)\n\n");
  worstCaseBufferTests();
  truncationTests();
  exactStringTests();
  waterDashboardTests();
  topicTests();
  jsonValidityTests();
  optionalKeyTests();
  birthMessageTests();
  republishPolicyTests();
  enumerationTests();
  commandButtonTests();
  configValidationTests();
  baseTopicAgreementTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
