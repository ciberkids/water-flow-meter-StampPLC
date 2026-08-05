#pragma once

#include <cstddef>
#include <cstdint>

#include "modbus/register_map.h"  // kNumSensors — one source for "how many sensors exist"
#include "net/mqtt_limits.h"     // kMqttOutBufferSize — the ONE definition (R4.4.8)
#include "net/net_settings.h"     // netFieldCapacity — the topic length caps already live there

namespace plc {

/**
 * Home Assistant MQTT discovery payloads (WiFi_MQTT_Connectivity.md §4.4).
 *
 * Arduino-free and allocation-free on purpose. Two separate reasons, both from the requirement:
 *
 *  - §4.4 is *string* work whose failure mode is invisible on hardware. A wrong `device_class`, a
 *    mis-spelled unit or a payload one byte over the client buffer all produce the same symptom:
 *    the entity never appears in Home Assistant and nothing is logged (§4.4.7). The only place that
 *    can be checked cheaply and repeatedly is a host test, so everything that decides *what the
 *    bytes are* lives here and the MQTT client is left with nothing but transport;
 *  - the rest of the firmware is allocation-free by policy, and this code runs from the MQTT task
 *    at priority 1 (R4.1.5) while core 0 is polling. Fixed buffers, caller-owned.
 *
 * ── Why the caller supplies the buffer ────────────────────────────────────────────────
 * A discovery payload is built, published and forgotten. Handing the buffer in means the MQTT
 * task can reuse one scratch buffer for all ~28 entities, and it means this module never has to
 * guess a size — see `kMqttOutBufferSize` below, which is the one number R4.4.8 is about.
 */

// ── The buffer constant R4.4.8 requires to be shared ─────────────────────────────────
//
// `kMqttOutBufferSize` now comes from net/mqtt_limits.h, which is its ONLY definition. It used to be
// declared here as well as in mqtt_publisher.h — two independent 2048s, each with a comment claiming
// to be the single source of truth. The transport read the publisher's copy; the worst-case test
// below measured the copy declared here. Halving one would have left this test green over a client
// that silently dropped every discovery payload.

/**
 * Slack the worst-case discovery payload must leave inside the buffer.
 *
 * Deliberately half the buffer, which pins the real bound at 1024 bytes — the client's *default*
 * buffer size. So the assertion the test makes is stronger than "it fits": it is "it would still
 * fit if somebody removed the explicit `out_buffer_size`", which is exactly the mistake §4.4.7
 * describes. It is also tight enough to actually fail when a field is added, and a bound that
 * cannot fail is not a test.
 */
inline constexpr std::size_t kDiscoveryPayloadHeadroom = 1024;

/** The bound the R4.4.8 host test asserts against. */
inline constexpr std::size_t kMaxDiscoveryPayloadBytes = kMqttOutBufferSize - kDiscoveryPayloadHeadroom;

// ── Length caps on everything that varies ────────────────────────────────────────────
//
// The two topic caps are taken from the settings store rather than repeated, because they are the
// same strings: a base topic longer than the field can hold cannot occur, and if that field ever
// grows the worst-case payload measurement grows with it automatically.

inline constexpr std::size_t kHaMaxPrefixBytes = netFieldCapacity(NetField::MqttDiscoveryPrefix);
inline constexpr std::size_t kHaMaxBaseTopicBytes = netFieldCapacity(NetField::MqttBaseTopic);

/** `node_id` — `wfm_<mac-suffix>` in §4.4.b, but the caller owns the derivation (R4.1.4). */
inline constexpr std::size_t kHaMaxNodeIdBytes = 32;
inline constexpr std::size_t kHaMaxDeviceNameBytes = 32;
inline constexpr std::size_t kHaMaxSwVersionBytes = 24;
inline constexpr std::size_t kHaMaxConfigUrlBytes = 40;

/**
 * Longest `object_id` this module emits, e.g. `s8_lifetime` / `board_temperature`.
 *
 * Load-bearing rather than documentation: the discovery-topic buffer size below is computed from
 * it, and the host test proves every entity's object id fits, so the two cannot drift.
 */
inline constexpr std::size_t kHaMaxObjectIdBytes = 24;

/** A buffer of this size always holds any discovery topic this module builds. */
inline constexpr std::size_t kHaDiscoveryTopicBytes =
    kHaMaxPrefixBytes + sizeof("/sensor/") + kHaMaxNodeIdBytes + 1 + kHaMaxObjectIdBytes +
    sizeof("/config") + 1;

/** A buffer of this size always holds any state, availability or status topic. */
inline constexpr std::size_t kHaStateTopicBytes =
    kHaMaxBaseTopicBytes + sizeof("/diagnostics/state") + 8;

/** Fixed device-block strings (§4.4.b). Not configurable — this is what the hardware is. */
inline constexpr const char* kHaManufacturer = "M5Stack";
inline constexpr const char* kHaModel = "StampPLC";

/** Used when the caller passes no device name, so the device block is never nameless. */
inline constexpr const char* kHaDefaultDeviceName = "Water Flow Meter";

/** What Home Assistant publishes its birth and will messages on, relative to the prefix (R4.4.7). */
inline constexpr const char* kHaStatusTopicSuffix = "status";
inline constexpr const char* kHaBirthPayload = "online";

/**
 * The entities of §4.4.a.
 *
 * The first three are per-sensor and share one state topic (§4.2 — 8 publishes per cycle, not 40);
 * the rest are per-device. `WifiRssi` is here because R4.4.6 requires RSSI as a diagnostic entity.
 */
enum class HaEntity : uint8_t {
  SensorFlow = 0,        /**< volume_flow_rate, L/min, measurement */
  SensorSessionVolume,   /**< water, L, total_increasing */
  SensorLifetimeVolume,  /**< water, m3, total_increasing — this is the Water dashboard one (R4.4.4) */
  BoardTemperature,      /**< temperature, degC, measurement */
  PollingRate,           /**< diagnostic, kHz, measurement */
  Undersampling,         /**< diagnostic, no unit, measurement */
  WifiRssi,              /**< diagnostic, dBm, measurement (R4.4.6) */
  Count
};

/** An entity to build for. `sensor` is a 0-based index and only meaningful for per-sensor kinds. */
struct HaEntityRef {
  HaEntity kind = HaEntity::Count;
  uint8_t sensor = 0;
};

/** Every per-sensor entity for every sensor, plus the per-device ones. */
inline constexpr std::size_t kHaMaxEntities = kNumSensors * 3 + 4;

/**
 * The result of building a string into a caller's buffer, with `snprintf` semantics.
 *
 * `required` is the length the value NEEDS, whether or not it fitted — which is what makes the
 * R4.4.8 measurement possible without a 2 kB buffer in the test, and what makes a truncation
 * report actionable ("needed 1103, had 1024") rather than merely negative.
 *
 * When `complete` is false the output buffer is left as an EMPTY STRING, never as a prefix of the
 * value. §4.4.7's failure mode is a silently dropped publish; a half-written JSON object that the
 * broker happily accepts and Home Assistant silently discards would be strictly worse, because the
 * device would believe it had published. A caller that ignores `complete` therefore publishes
 * nothing, which is a bug that shows up as a missing entity rather than as a corrupt one.
 */
struct HaBuildResult {
  std::size_t required = 0;
  bool complete = false;
};

// ── Per-entity metadata: the exact strings of §4.4.a ─────────────────────────────────
//
// Exposed rather than buried in the payload builder for one specific reason: the MQTT client of
// slice N5 has to build the STATE payloads whose keys these templates read. If it generates them
// from `haEntityValueKey` then a renamed key moves the template with it. A key that exists in one
// and not the other yields an entity stuck at `unknown` forever, with nothing logged — the same
// class of silent failure as R4.4.8's.

/** The JSON key inside the shared state payload, i.e. the `value_json.<key>` of §4.4.b. */
const char* haEntityValueKey(HaEntity entity);

/** `device_class`, or nullptr where §4.4.a specifies none (the diagnostics). */
const char* haEntityDeviceClass(HaEntity entity);

/**
 * `unit_of_measurement`, or nullptr for undersampling which is a bitmap.
 *
 * NOTE the units are UTF-8, not ASCII: `m³` is `m` 0xC2 0xB3 and `°C` is 0xC2 0xB0 `C`. §4.6's
 * ASCII-only rule applies to text bound for the M5GFX Font0 display, NOT to MQTT — Home Assistant
 * accepts only its own spelling of these units, so the payload has to carry the real codepoints.
 */
const char* haEntityUnit(HaEntity entity);

/** `state_class` — `measurement` or `total_increasing` (§4.4.a). */
const char* haEntityStateClass(HaEntity entity);

/**
 * `suggested_display_precision`.
 *
 * Present on every entity because without it Home Assistant infers 0 decimals and renders a
 * litres-per-minute reading as a bare integer (§4.4). The values are an engineering choice, not a
 * specified one; 3 for m³ is litre resolution.
 */
uint8_t haEntityPrecision(HaEntity entity);

/** True for the entities that carry `entity_category: diagnostic` — the only legal value (§4.4.a). */
bool haEntityIsDiagnostic(HaEntity entity);

/** True for the three entities that exist once per sensor. */
bool haEntityIsPerSensor(HaEntity entity);

/** The `object_id` fragment: `flow`, `lifetime`, `polling_rate`, ... */
const char* haEntityObjectIdSuffix(HaEntity entity);

/** Why discovery is being published. Reported so a republish storm is visible rather than inferred. */
enum class HaRepublishReason : uint8_t {
  None = 0,
  FirstConnect,    /**< The first publish after boot. */
  Reconnect,       /**< R4.4.6 — every reconnect republishes. */
  HaBirth,         /**< R4.4.7 — `online` arrived on the HA status topic. */
  SensorsChanged   /**< R4.4.6 — the connected-sensor bitmap moved, so an entity must appear. */
};

const char* haRepublishReasonText(HaRepublishReason reason);

/** The parts of the device block the caller has to supply (§4.4.b). */
struct HaDeviceIdentity {
  /**
   * `node_id` and the device `identifiers` entry, e.g. `wfm_a1b2c3`. MAC-derived by the caller
   * (R4.1.4), and restricted to `[A-Za-z0-9_-]` — see `HaDiscovery::configure`.
   */
  const char* nodeId = nullptr;
  const char* name = nullptr;             /**< Empty → `kHaDefaultDeviceName`. */
  const char* swVersion = nullptr;        /**< Empty → the key is omitted entirely. */
  const char* configurationUrl = nullptr; /**< Empty → the key is omitted entirely. */
};

class HaDiscovery {
 public:
  /**
   * Copies the configuration in and validates it. False leaves the object UNCONFIGURED, and an
   * unconfigured object builds nothing at all.
   *
   * Copies rather than borrows because the sources are `NetSettings` live values that an apply
   * from a Modbus master or the web form can change between two publishes in the same loop
   * (net_settings.h staged/apply) — half the entities under the old base topic and half under the
   * new one is not a state worth being able to reach.
   *
   * Rejects rather than truncates. A truncated base topic is still a *valid* topic, so the device
   * would publish happily to the wrong place and the operator would see nothing in Home Assistant
   * and nothing wrong on the panel. A refusal is reportable.
   *
   * The base topic AND the discovery prefix are judged by `NetSettings::isValidBaseTopic` — the one
   * canonical rule (owner decision 5A) — and by no local rule of this module's own. `MqttPublisher`
   * defers to the same function, so a topic this accepts is a topic that publishes. That was not
   * true before: see the deferral comment in `configure`'s body for the six classes of input the two
   * modules used to judge differently, and for why every one of them failed silently.
   *
   * The prefix additionally has to fit its own 32-byte field, which is a tighter bound than the
   * validator's and therefore still checked here.
   */
  bool configure(const HaDeviceIdentity& device, const char* discoveryPrefix, const char* baseTopic);

  bool configured() const { return configured_; }

  /** `<prefix>/sensor/<node_id>/<object_id>/config` — retained, one per entity (§4.4.b, R4.4.5). */
  HaBuildResult discoveryTopic(HaEntityRef ref, char* out, std::size_t size) const;

  /** The full discovery payload, device block included (§4.4.b). */
  HaBuildResult discoveryPayload(HaEntityRef ref, char* out, std::size_t size) const;

  /**
   * The state topic the entity reads: `<base>/sensor/<n>/state` or `<base>/diagnostics/state`.
   *
   * Public so slice N5 publishes to the topic discovery advertises, from one implementation.
   */
  HaBuildResult stateTopic(HaEntityRef ref, char* out, std::size_t size) const;

  /** `<base>/status` — the LWT and `availability_topic` of R4.5.1. */
  HaBuildResult availabilityTopic(char* out, std::size_t size) const;

  /** `<prefix>/status` — what the device must SUBSCRIBE to (R4.4.7). */
  HaBuildResult haStatusTopic(char* out, std::size_t size) const;

  /** `object_id`: `s3_flow`, `polling_rate`, ... */
  HaBuildResult objectId(HaEntityRef ref, char* out, std::size_t size) const;

  /** `unique_id`: `<node_id>_<object_id>` (R4.4.3). */
  HaBuildResult uniqueId(HaEntityRef ref, char* out, std::size_t size) const;

  /**
   * True only for Home Assistant's birth message: payload `online` on `<prefix>/status` (R4.4.7).
   *
   * The topic is compared exactly, which is load-bearing rather than pedantic: the device's OWN
   * availability topic `<base>/status` carries the payload `online` too (R4.5.1), and treating that
   * as a birth would make every one of our own availability publishes trigger a full 28-entity
   * republish — a self-sustaining storm on the radio §2.1 is trying to protect.
   */
  bool isHaBirth(const char* topic, const char* payload) const;

  /**
   * Fills `out` with the entities to publish for `connectedBitmap` (bit n = sensor n+1).
   *
   * Only connected sensors get entities, which is what makes R4.4.6's "enabling a sensor makes its
   * entity appear" true. Returns the number written, never more than `capacity`.
   */
  std::size_t enumerateEntities(uint16_t connectedBitmap, HaEntityRef* out,
                                std::size_t capacity) const;

 private:
  bool configured_ = false;
  char prefix_[kHaMaxPrefixBytes + 1] = {};
  char base_[kHaMaxBaseTopicBytes + 1] = {};
  char nodeId_[kHaMaxNodeIdBytes + 1] = {};
  char deviceName_[kHaMaxDeviceNameBytes + 1] = {};
  char swVersion_[kHaMaxSwVersionBytes + 1] = {};
  char configUrl_[kHaMaxConfigUrlBytes + 1] = {};
};

/**
 * When discovery must be (re)published — R4.4.6 and R4.4.7 as a state machine rather than as
 * conditions scattered through the MQTT event handler.
 *
 * `notePublished` is what closes the loop: the bitmap is recorded only once the publish has
 * actually happened, so a failed publish (§4.4.7 returns -1) leaves the policy still asking for a
 * republish instead of believing the entities exist.
 */
class HaRepublishPolicy {
 public:
  /** Always a reason — R4.4.6 republishes on every reconnect. */
  HaRepublishReason onConnected();

  /** R4.4.7. Delegates the topic and payload test to `HaDiscovery::isHaBirth`. */
  HaRepublishReason onStatusMessage(const HaDiscovery& discovery, const char* topic,
                                    const char* payload) const;

  /** R4.4.6. `SensorsChanged` only when the bitmap actually differs from the published one. */
  HaRepublishReason onSensorBitmap(uint16_t connectedBitmap) const;

  /** Call after the whole set has been published successfully. */
  void notePublished(uint16_t connectedBitmap);

  void noteDisconnected();

  bool everPublished() const { return published_; }
  uint16_t publishedBitmap() const { return bitmap_; }

 private:
  bool connected_ = false;
  bool published_ = false;
  uint16_t bitmap_ = 0;
};

}  // namespace plc
