#pragma once

#include <cstddef>
#include <cstdint>

#include "modbus/register_map.h"

namespace plc {

/**
 * The MQTT publish POLICY — topics, cadence, and the queue's drop order (WiFi §4.2, §4.3, §4.5).
 *
 * Arduino-free and esp-mqtt-free on purpose. Everything here is a decision with a checkable rule
 * behind it, and none of those rules are convenient to observe on hardware: "did the rate limit
 * suppress that second publish", "did the 60 s heartbeat fire with nothing changing", "which
 * message did the queue evict when it filled". Each is a one-line assertion on a host and a
 * multi-hour broker-log expedition on a bench, so the policy takes an injected `MqttSink` and the
 * transport half lives in `mqtt_transport_esp.h`.
 *
 * ── What is deliberately NOT here ──────────────────────────────────────────────────────
 *
 * Unit conversion. The snapshot fields are named with the units §4.4.a requires HA to see
 * (`flowLPerMin`, `totalCubicMeters`), while the firmware holds L/s and litres. The adapter that
 * fills the snapshot converts; doing it here would hide an HA-facing contract inside a serialiser.
 *
 * Discovery payloads (§4.4, slice N6). Discovery arrives through `enqueue()` as an opaque
 * `MqttClass::Discovery` message so that it inherits the never-dropped guarantee of R4.1.3 without
 * this file having to know what a `device` block looks like.
 */

/**
 * What a queued message is for, which is the same thing as what may happen to it when the queue
 * fills. R4.1.3 draws exactly this line: telemetry is expendable, the other two are not.
 */
enum class MqttClass : uint8_t {
  Availability = 0,  /**< `<base>/status` — R4.5.1. Losing this leaves HA showing stale values. */
  Discovery,         /**< `homeassistant/...` config topics — R4.4.5. Losing one loses an entity. */
  Telemetry          /**< A state topic. A superseded reading has no value (R4.1.3). */
};

const char* mqttClassName(MqttClass cls);

/** True only for the class R4.1.3 permits the queue to evict. */
bool mqttClassIsDroppable(MqttClass cls);

/**
 * Where published messages go.
 *
 * `publish` mirrors `esp_mqtt_client_publish`/`_enqueue`: **a non-negative return is success** and
 * -1 is failure. QoS 0 always yields message id 0, so a `> 0` success test would classify every
 * steady-state publish at the default QoS as a failure — which is why the contract is stated here
 * rather than left to the call site to infer.
 */
class MqttSink {
 public:
  virtual ~MqttSink() = default;

  /** Non-negative message id on success, -1 on failure (see §4.4.7 — failure is silent). */
  virtual int publish(const char* topic, const char* payload, int qos, bool retain) = 0;

  /** False whenever the broker session is down. The publisher holds messages rather than losing
   *  them, so this must not optimistically report true while reconnecting. */
  virtual bool connected() const = 0;
};

// ── MAC-derived identity (R4.1.4, §4.2) ────────────────────────────────────────────────
//
// Free functions taking the six MAC bytes, so the host test can drive them with a fixed MAC and
// the Arduino side is a single `WiFi.macAddress(mac)` call. All three write lowercase hex of the
// last three bytes, which is the "<mac-suffix>" the requirement keeps referring to.

/** The bare suffix, e.g. `a1b2c3`. Needs 7 bytes. */
bool mqttMacSuffix(const uint8_t mac[6], char* out, std::size_t size);

/**
 * R4.1.4 — the stable client id, e.g. `wfm-a1b2c3`. Needs 11 bytes.
 *
 * Stable across reboots because it is derived from hardware rather than generated: two devices
 * sharing a client id kick each other off the broker in a loop, and from the device end that looks
 * like an unreliable network rather than like a collision.
 */
bool mqttClientId(const uint8_t mac[6], char* out, std::size_t size);

/** §4.2's default base topic, e.g. `watermeter/a1b2c3`. Needs 18 bytes. */
bool mqttDefaultBaseTopic(const uint8_t mac[6], char* out, std::size_t size);

// ── The snapshot the publisher serialises ──────────────────────────────────────────────

/** One sensor's published state. Units are the ones HA is told to expect (§4.4.a). */
struct MqttSensorTelemetry {
  /** False for a sensor not in use — it gets no topic at all, rather than a topic full of zeros. */
  bool present = false;
  float flowLPerMin = 0.0f;
  float sessionLiters = 0.0f;
  double totalCubicMeters = 0.0;
  float maxFlowLPerMin = 0.0f;
  uint32_t pulses = 0;
};

/** The aggregate on `<base>/total/state`. */
struct MqttTotalTelemetry {
  float flowLPerMin = 0.0f;
  float sessionLiters = 0.0f;
  double totalCubicMeters = 0.0;
  uint8_t activeSensors = 0;
};

/** `<base>/diagnostics/state`. Carries the R2.1.2 pair so a polling regression is visible in HA. */
struct MqttDiagnosticsTelemetry {
  float pollingRateKhz = 0.0f;
  float baselineRateKhz = 0.0f;
  uint16_t undersamplingFlags = 0;
  float boardTemperatureC = 0.0f;
  uint32_t uptimeSeconds = 0;
  int8_t wifiRssiDbm = 0;
};

struct MqttSnapshot {
  MqttSensorTelemetry sensors[kNumSensors]{};
  MqttTotalTelemetry total{};
  MqttDiagnosticsTelemetry diagnostics{};
};

class MqttPublisher {
 public:
  // ── Sizes. Every one of these is an arithmetic argument, not a round number ──────────

  /**
   * Topic buffer, and the limit `enqueue()` refuses above. Overlong topics are REFUSED, never
   * truncated — a truncated topic publishes *successfully* to the wrong place, so the broker
   * accepts it and nothing anywhere reports a fault.
   *
   * Sized by DISCOVERY, not by our own state topics. `<base>/diagnostics/state` is at most
   * 48 + 18 = 66 bytes, but a §4.4.b discovery topic is
   * `<prefix>/sensor/<node_id>/<object_id>/config` — 32 + 32 + 24 plus separators, which
   * `ha_discovery.h` bounds at 107. Discovery travels through THIS queue so that it inherits
   * R4.1.3's never-dropped guarantee, so this cap has to clear that bound or the entity that
   * happens to have the longest name silently never appears.
   */
  static constexpr std::size_t kMaxTopicBytes = 128;

  /**
   * Queue slot payload size, and the limit `enqueue()` refuses above.
   *
   * Also set by discovery: `ha_discovery.h` bounds its worst-case payload — longest sensor name,
   * longest base topic, longest sw_version, with the `device` block repeated per §4.4.b — at 1024
   * bytes. Anything at or below that must fit here, or R4.1.3's "discovery is never dropped" would
   * be defeated at the queue's door rather than at the client's buffer.
   */
  static constexpr std::size_t kMaxPayloadBytes = 1152;

  /** Telemetry payloads are ~110 bytes; this bounds the per-topic change-detection baseline, of
   *  which there are `kNumSensors + 2`. Kept separate from `kMaxPayloadBytes` so the baselines do
   *  not pay for discovery's size. */
  static constexpr std::size_t kMaxTelemetryPayloadBytes = 256;

  /** A full set is `kNumSensors + 2` = 10 messages, plus the availability publish on connect. 16
   *  therefore holds a complete set with headroom while still being small enough that the eviction
   *  path of R4.1.3 is reachable in practice rather than theoretical.
   *
   *  Cost: 16 x (128 + 1152 + a few) is ~20.5 KB of static RAM, and the discovery bound above is
   *  what most of it buys. Paid deliberately and statically — the alternative is a heap allocation
   *  on a device whose §2.1 budget is the measurement itself. A discovery burst is larger than any
   *  sane capacity (kNumSensors x 3 + 4 entities), so the caller pumps as it enqueues rather than
   *  this number growing to hold one. */
  static constexpr std::size_t kQueueCapacity = 16;

  /** R4.1.6 — what the transport sets `out_buffer_size` to. Lives here, in the host-compiled half,
   *  so the R4.4.8 length test and the transport share ONE constant rather than two that drift.
   *  §4.4.7: a payload over this is dropped by esp-mqtt with no log and no entity in HA. */
  static constexpr int kOutBufferBytes = 2048;

  /** A payload this queue would accept but the client could never send is a silent loss by
   *  construction (§4.4.7), so the two bounds are reconciled at compile time rather than by review. */
  static_assert(kMaxPayloadBytes <= static_cast<std::size_t>(kOutBufferBytes),
                "queue slots must fit inside out_buffer_size");
  static_assert(kMaxTelemetryPayloadBytes <= kMaxPayloadBytes,
                "a telemetry payload must fit a queue slot");

  /** R4.3.2. */
  static constexpr uint32_t kHeartbeatMs = 60000;

  /** R4.3.1's configurable range. */
  static constexpr uint16_t kMinPublishPeriodS = 1;
  static constexpr uint16_t kMaxPublishPeriodS = 3600;
  static constexpr uint16_t kDefaultPublishPeriodS = 10;

  explicit MqttPublisher(MqttSink& sink) : sink_(sink) {}

  /**
   * Sets the base topic (§4.2) and the cadence (R4.3.1). Returns false — leaving the publisher
   * unconfigured and silent — when the base topic cannot be used.
   *
   * Refusal rather than repair, because a base topic is operator input that reaches a broker: `+`
   * and `#` are illegal in a PUBLISH topic per MQTT 3.1.1, and silently stripping them would
   * publish to a topic the operator did not type and cannot find.
   *
   * `publishPeriodS` and `qos` are CLAMPED rather than refused. They arrive from a Modbus register
   * where any 16-bit value is writable, and refusing the whole configuration because a master
   * wrote 60000 would take MQTT down over a typo in an unrelated field.
   */
  bool configure(const char* baseTopic, uint16_t publishPeriodS, uint8_t qos);

  bool configured() const { return baseTopic_[0] != '\0'; }
  const char* baseTopic() const { return baseTopic_; }
  uint16_t publishPeriodS() const { return publishPeriodS_; }
  uint8_t qos() const { return qos_; }

  /**
   * `<base>/status` (§4.2), owned as a member because the transport hands this pointer to
   * `esp_mqtt_client_config_t.lwt_topic`. Storing it here rather than in a caller's temporary
   * sidesteps the question of whether IDF 4.4 copies the string: the publisher outlives the client.
   *
   * Empty string while unconfigured.
   */
  const char* availabilityTopic() const { return availabilityTopic_; }

  /** The two retained LWT payloads of R4.5.1. */
  static constexpr const char* kOnlinePayload = "online";
  static constexpr const char* kOfflinePayload = "offline";

  /** §4.2 state topics. False — with `out` left empty — if unconfigured or the topic would not fit.
   *  `index` is 0-based; the topic is 1-based, matching §4.4.b's `<base>/sensor/1/state`. */
  bool sensorStateTopic(std::size_t index, char* out, std::size_t size) const;
  bool totalStateTopic(char* out, std::size_t size) const;
  bool diagnosticsStateTopic(char* out, std::size_t size) const;

  /**
   * R4.5.1 — queues the retained `online` publish, and arms a full set for the next `tick()`.
   *
   * The full set is armed rather than published here because a reconnect is exactly when a
   * subscriber's picture is emptiest, and the change detection would otherwise suppress every
   * value that happens to be unchanged since before the drop.
   *
   * There is no matching `offline` publish: R4.5.1's will message is configured on the client, so
   * the broker sends it — including in the case that actually matters, a device that lost power
   * and has no opportunity to send anything.
   */
  void onConnected();

  /** Resets the cadence state so the reconnect republishes in full. Does not publish. */
  void onDisconnected();

  /**
   * Queues a message. Used directly by N6 for discovery; telemetry goes through `tick()`.
   *
   * R4.1.3, in full: when the queue is full the OLDEST TELEMETRY is evicted to make room,
   * whatever the incoming class. If the queue holds no telemetry to evict — all availability and
   * discovery — the incoming message is REJECTED instead, because evicting a discovery message
   * would lose an entity permanently while dropping a reading loses one sample.
   *
   * False on rejection, an overlong topic or payload, or an unconfigured publisher.
   */
  bool enqueue(MqttClass cls, const char* topic, const char* payload, bool retain);

  /**
   * The cadence decision (§4.3). Cheap and non-blocking: builds payloads, compares, queues.
   * Safe to call every pass of the logic loop.
   *
   * Change detection compares the SERIALISED PAYLOAD against the last one published on that topic.
   * That is deliberate: it needs no per-field epsilon, and it makes "unchanged" mean exactly what
   * a subscriber would see, so a value that moves by less than its printed precision correctly
   * counts as no change.
   *
   * The R4.3.2 heartbeat BYPASSES both the change detection and the rate limit. A consequence
   * worth naming: with `publishPeriodS` above 60 the heartbeat becomes the effective cadence,
   * because R4.3.2 is a bound on staleness rather than a preference.
   */
  void tick(uint32_t nowMs, const MqttSnapshot& snapshot);

  /**
   * Drains up to `maxMessages` to the sink. No-op while the sink is disconnected, which is what
   * lets the queue — and therefore R4.1.3 — do its job.
   *
   * Returns the number of successful publishes. A message is removed from the queue whether it
   * succeeded or failed: a -1 from an oversize payload is permanent, so retrying it would wedge
   * the queue behind a message that can never go out. Failures are counted and the topic recorded
   * (R4.1.6) instead. What heals a lost message is R4.3.2's heartbeat for telemetry and R4.4.6's
   * reconnect republish for discovery.
   */
  std::size_t pump(std::size_t maxMessages = kQueueCapacity);

  // ── Observability. All of it exists because §4.4.7's failure mode is silence ─────────

  std::size_t queued() const { return count_; }
  uint32_t droppedTelemetry() const { return droppedTelemetry_; }

  /** Messages refused outright: overlong, or the queue held nothing droppable. Never zero-cost —
   *  a non-zero value here means an entity or an availability state was lost. */
  uint32_t rejectedMessages() const { return rejectedMessages_; }

  /** R4.1.6 — publishes the sink reported as -1. */
  uint32_t publishFailures() const { return publishFailures_; }

  /** Topic of the most recent failure, empty if there has been none. */
  const char* lastFailureTopic() const { return lastFailureTopic_; }

  uint32_t publishedMessages() const { return publishedMessages_; }

 private:
  struct Message {
    MqttClass cls = MqttClass::Telemetry;
    bool retain = false;
    uint8_t qos = 0;
    char topic[kMaxTopicBytes] = {};
    char payload[kMaxPayloadBytes] = {};
  };

  /** One change-detection baseline per state topic: the sensors, then total, then diagnostics. */
  static constexpr std::size_t kSlotCount = kNumSensors + 2;
  static constexpr std::size_t kTotalSlot = kNumSensors;
  static constexpr std::size_t kDiagnosticsSlot = kNumSensors + 1;

  /** Wrap-safe elapsed time. `millis()` wraps every 49.7 days and this device is meant to run for
   *  years, so a naive `now - last >= period` on signed or reordered values would stall the
   *  publisher for one full period at the wrap. */
  static bool elapsed(uint32_t nowMs, uint32_t sinceMs, uint32_t intervalMs);

  bool buildTopic(const char* suffix, std::size_t index, bool indexed, char* out,
                  std::size_t size) const;

  /** Emits `value` with `decimals` fixed places, or `null` when it is not finite. snprintf would
   *  otherwise write `nan`/`inf`, which is not valid JSON — HA drops the whole message and the
   *  entity simply stops updating with nothing logged. */
  static void jsonNumber(char* out, std::size_t size, double value, int decimals);

  void formatSensor(const MqttSensorTelemetry& s, char* out, std::size_t size) const;
  void formatTotal(const MqttTotalTelemetry& t, char* out, std::size_t size) const;
  void formatDiagnostics(const MqttDiagnosticsTelemetry& d, char* out, std::size_t size) const;

  /** Queues `payload` for `slot` when the heartbeat forces it or the payload changed. */
  void offer(std::size_t slot, const char* topic, const char* payload, bool force);

  /** R4.1.3's eviction. False when the queue holds nothing the policy allows dropping. */
  bool evictOldestTelemetry();

  MqttSink& sink_;

  char baseTopic_[kMaxTopicBytes] = {};
  char availabilityTopic_[kMaxTopicBytes] = {};
  uint16_t publishPeriodS_ = kDefaultPublishPeriodS;
  uint8_t qos_ = 0;

  Message queue_[kQueueCapacity]{};
  std::size_t head_ = 0;
  std::size_t count_ = 0;

  char baseline_[kSlotCount][kMaxTelemetryPayloadBytes] = {};
  bool baselineValid_[kSlotCount] = {};

  uint32_t lastPublishMs_ = 0;
  uint32_t lastFullMs_ = 0;
  bool everPublished_ = false;
  bool forceFull_ = true;

  uint32_t droppedTelemetry_ = 0;
  uint32_t rejectedMessages_ = 0;
  uint32_t publishFailures_ = 0;
  uint32_t publishedMessages_ = 0;
  char lastFailureTopic_[kMaxTopicBytes] = {};
};

}  // namespace plc
