// Host tests for the MQTT publish policy (WiFi §4.2, §4.3, §4.5, R4.1.3–R4.1.6).
//
// Every rule in this slice is a rule about something that DID NOT happen: a second publish inside
// the rate-limit window, an availability message that was not evicted, a -1 that was not swallowed.
// None of those are observable on a bench without a broker log and a stopwatch, and all of them are
// one assertion here — which is the whole reason the policy is Arduino-free and takes an injected
// sink.
//
// The fake sink returns **0** for success, because that is what esp_mqtt_client_publish/_enqueue
// actually returns at QoS 0. A fake that returned 1 would let a `result > 0` success test pass here
// and classify every real steady-state publish on hardware as a failure.
#include "net/mqtt_publisher.h"

#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <string>
#include <vector>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-78s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

void checkStr(const char* actual, const char* expected, const char* what) {
  const bool same = std::strcmp(actual, expected) == 0;
  ++checks;
  std::printf("  %-78s %s\n", what, same ? "ok" : "FAIL");
  if (!same) {
    std::printf("      expected \"%s\"\n      actual   \"%s\"\n", expected, actual);
    ++failures;
  }
}

using plc::MqttClass;
using plc::MqttPublisher;
using plc::MqttSnapshot;

struct Capture {
  std::string topic;
  std::string payload;
  int qos = 0;
  bool retain = false;
};

/** Captures every publish, and can be told to be down or to fail. */
class FakeSink : public plc::MqttSink {
 public:
  int publish(const char* topic, const char* payload, int qos, bool retain) override {
    calls.push_back({topic, payload, qos, retain});
    if (failNext > 0) {
      --failNext;
      return -1;
    }
    return successReturn;
  }

  bool connected() const override { return up; }

  bool up = true;
  /** 0, not 1 — the real QoS-0 message id. */
  int successReturn = 0;
  int failNext = 0;
  std::vector<Capture> calls;
};

/**
 * Bounds-checked access to a captured publish.
 *
 * Not defensiveness for its own sake: several checks below assert "the Nth message is X", and when
 * a regression makes that message absent, `calls.front()` on an empty vector aborts the whole
 * binary. A suite that crashes on the first regression hides every check after it, so a missing
 * message has to read as an empty topic and a clean FAIL.
 */
Capture at(const FakeSink& sink, std::size_t index) {
  if (index >= sink.calls.size()) return Capture{};
  return sink.calls[index];
}

std::size_t countTopic(const FakeSink& sink, const char* topic) {
  std::size_t n = 0;
  for (const Capture& c : sink.calls) {
    if (c.topic == topic) ++n;
  }
  return n;
}

std::string lastPayloadFor(const FakeSink& sink, const char* topic) {
  std::string found;
  for (const Capture& c : sink.calls) {
    if (c.topic == topic) found = c.payload;
  }
  return found;
}

bool anyPayloadContains(const FakeSink& sink, const char* needle) {
  for (const Capture& c : sink.calls) {
    if (c.payload.find(needle) != std::string::npos) return true;
  }
  return false;
}

std::string topicOf(const MqttPublisher& p, std::size_t sensorIndex) {
  char buffer[MqttPublisher::kMaxTopicBytes] = {};
  p.sensorStateTopic(sensorIndex, buffer, sizeof(buffer));
  return buffer;
}

MqttSnapshot oneSensorSnapshot() {
  MqttSnapshot snap;
  snap.sensors[0].present = true;
  snap.sensors[0].flowLPerMin = 1.5f;
  return snap;
}

// ── §4.2 topic layout ────────────────────────────────────────────────────────────────

void topicTests() {
  std::printf("[topics — §4.2, built from a configurable base]\n");

  FakeSink sink;
  MqttPublisher p(sink);
  check(!p.configured(), "a fresh publisher is unconfigured and therefore silent");
  checkStr(p.availabilityTopic(), "", "with no availability topic for the transport to use as LWT");

  check(!p.configure(nullptr, 10, 0), "a null base topic is refused");
  check(!p.configure("", 10, 0), "so is an empty one");
  check(!p.configure("///", 10, 0), "and one made only of slashes");
  check(!p.configure("watermeter/+/a", 10, 0),
        "a '+' is refused — it is a wildcard and illegal in a PUBLISH topic");
  check(!p.configure("watermeter/#", 10, 0), "and so is a '#'");
  check(!p.configured(), "every refusal leaves the publisher unconfigured rather than half-set");

  check(p.configure("watermeter/a1b2c3", 10, 0), "a plain base topic is accepted");
  checkStr(p.availabilityTopic(), "watermeter/a1b2c3/status", "<base>/status is the LWT topic");
  checkStr(topicOf(p, 0).c_str(), "watermeter/a1b2c3/sensor/1/state",
           "sensor 0 publishes on .../sensor/1/state — 1-based on the wire (§4.4.b)");
  checkStr(topicOf(p, 7).c_str(), "watermeter/a1b2c3/sensor/8/state",
           "and the eighth sensor is .../sensor/8/state, not /7/");

  char buffer[MqttPublisher::kMaxTopicBytes] = {};
  check(!p.sensorStateTopic(plc::kNumSensors, buffer, sizeof(buffer)),
        "an index past the last sensor is refused");
  check(p.totalStateTopic(buffer, sizeof(buffer)) &&
            std::strcmp(buffer, "watermeter/a1b2c3/total/state") == 0,
        "<base>/total/state");
  check(p.diagnosticsStateTopic(buffer, sizeof(buffer)) &&
            std::strcmp(buffer, "watermeter/a1b2c3/diagnostics/state") == 0,
        "<base>/diagnostics/state");

  // The mandated trailing-slash case. Without the strip, "<base>/status" becomes
  // "watermeter/a1b2c3//status" — a legal topic with an empty level that matches nothing the
  // operator configured in Home Assistant, and no error anywhere.
  MqttPublisher slashed(sink);
  check(slashed.configure("watermeter/a1b2c3/", 10, 0), "a trailing slash is accepted");
  checkStr(slashed.availabilityTopic(), "watermeter/a1b2c3/status",
           "and stripped, so a trailing slash gives the SAME status topic");
  checkStr(topicOf(slashed, 0).c_str(), "watermeter/a1b2c3/sensor/1/state",
           "and the same sensor topic — no doubled separator");

  MqttPublisher manySlashes(sink);
  check(manySlashes.configure("watermeter/a1b2c3///", 10, 0), "several trailing slashes too");
  checkStr(manySlashes.availabilityTopic(), "watermeter/a1b2c3/status", "all of them stripped");

  // Refusal, not truncation. A truncated topic publishes successfully to the wrong place, which is
  // strictly worse than not publishing: the broker accepts it and nothing anywhere reports a fault.
  char tiny[8] = {};
  check(!p.sensorStateTopic(0, tiny, sizeof(tiny)),
        "a topic that will not fit the caller's buffer is REFUSED");
  check(tiny[0] == '\0', "and the buffer is left empty, never half-written");

  const std::string overlong(MqttPublisher::kMaxTopicBytes, 'x');
  MqttPublisher tooLong(sink);
  check(!tooLong.configure(overlong.c_str(), 10, 0),
        "a base topic with no room for '/diagnostics/state' is refused at configure time");
}

// ── R4.1.4 identity ─────────────────────────────────────────────────────────────────

void identityTests() {
  std::printf("\n[identity — R4.1.4, stable and unique from the MAC]\n");

  const uint8_t mac[6] = {0x24, 0x6f, 0x28, 0xa1, 0xb2, 0xc3};
  char suffix[8] = {};
  char clientId[16] = {};
  char base[32] = {};

  check(plc::mqttMacSuffix(mac, suffix, sizeof(suffix)), "the MAC suffix is derivable");
  checkStr(suffix, "a1b2c3", "lowercase hex of the last three bytes");
  check(plc::mqttClientId(mac, clientId, sizeof(clientId)), "so is the client id");
  checkStr(clientId, "wfm-a1b2c3", "which is wfm-<mac-suffix>");
  check(plc::mqttDefaultBaseTopic(mac, base, sizeof(base)), "and the default base topic");
  checkStr(base, "watermeter/a1b2c3", "§4.2's watermeter/<mac-suffix>");

  // Uniqueness is the requirement's actual concern: two devices sharing a client id disconnect
  // each other in a loop, and from the device end that looks like an unreliable network. A
  // derivation that dropped the last byte would pass every check above and fail this one.
  const uint8_t neighbour[6] = {0x24, 0x6f, 0x28, 0xa1, 0xb2, 0xc4};
  char otherId[16] = {};
  plc::mqttClientId(neighbour, otherId, sizeof(otherId));
  check(std::strcmp(clientId, otherId) != 0, "a MAC differing in its LAST byte yields another id");

  const uint8_t upstream[6] = {0x24, 0x6f, 0x29, 0xa1, 0xb2, 0xc3};
  char sameId[16] = {};
  plc::mqttClientId(upstream, sameId, sizeof(sameId));
  checkStr(sameId, clientId, "while the OUI bytes are deliberately not part of it");

  char cramped[4] = {};
  check(!plc::mqttClientId(mac, cramped, sizeof(cramped)),
        "a buffer too small to hold the id is refused, not truncated into a collision");
}

// ── R4.3.1 rate limit ───────────────────────────────────────────────────────────────

void rateLimitTests() {
  std::printf("\n[cadence — R4.3.1, publish on change, rate-limited]\n");

  FakeSink sink;
  MqttPublisher p(sink);
  check(p.configure("wm", 10, 0), "a 10 s publish period");

  MqttSnapshot snap = oneSensorSnapshot();
  p.tick(1000, snap);
  p.pump();
  check(sink.calls.size() == 3,
        "the first tick publishes a full set: the one present sensor, total, diagnostics");

  sink.calls.clear();
  snap.sensors[0].flowLPerMin = 2.5f;
  p.tick(6000, snap);
  p.pump();
  // THE mandated check. Delete the rate-limit gate from tick() and this publishes immediately.
  check(sink.calls.empty(), "a change 5 s into a 10 s window publishes NOTHING (R4.3.1)");

  p.tick(11000, snap);
  p.pump();
  check(sink.calls.size() == 1, "the same change publishes once the window has passed");
  checkStr(at(sink, 0).topic.c_str(), "wm/sensor/1/state",
           "and only the topic that moved — total and diagnostics are unchanged");

  sink.calls.clear();
  p.tick(30000, snap);
  p.pump();
  check(sink.calls.empty(),
        "an unchanged snapshot publishes nothing even long after the window passed");

  // A tick that published nothing must NOT restart the window, or a quiet device would push its own
  // deadline forward and the next real change would wait a further full period to be seen.
  snap.sensors[0].flowLPerMin = 3.5f;
  p.tick(31000, snap);
  p.pump();
  check(sink.calls.size() == 1,
        "so a change 1 s after a silent tick still publishes — the silent tick did not rearm it");

  MqttPublisher clamps(sink);
  clamps.configure("wm", 0, 0);
  check(clamps.publishPeriodS() == MqttPublisher::kMinPublishPeriodS,
        "a period of 0 clamps up to R4.3.1's 1 s floor");
  clamps.configure("wm", 60000, 0);
  check(clamps.publishPeriodS() == MqttPublisher::kMaxPublishPeriodS,
        "and 60000 clamps down to the 3600 s ceiling");
}

// ── R4.3.2 heartbeat ────────────────────────────────────────────────────────────────

void heartbeatTests() {
  std::printf("\n[heartbeat — R4.3.2, a full set every 60 s regardless of change]\n");

  FakeSink sink;
  MqttPublisher p(sink);
  p.configure("wm", 10, 0);

  // The snapshot is filled once and then NEVER TOUCHED again. A test that nudged a value to make
  // the heartbeat fire would pass whether or not the bypass exists, which is the exact shape of
  // test this project has been burned by.
  const MqttSnapshot snap = oneSensorSnapshot();
  p.tick(0, snap);
  p.pump();
  const std::string firstPayload = lastPayloadFor(sink, "wm/sensor/1/state");
  check(!firstPayload.empty(), "the first tick publishes the sensor");

  sink.calls.clear();
  for (uint32_t t = 10000; t <= 50000; t += 10000) {
    p.tick(t, snap);
    p.pump();
  }
  check(sink.calls.empty(), "five ticks across the minute publish nothing, because nothing changed");

  p.tick(60000, snap);
  p.pump();
  check(sink.calls.size() == 3, "at 60 s a FULL set goes out anyway (R4.3.2)");
  checkStr(lastPayloadFor(sink, "wm/sensor/1/state").c_str(), firstPayload.c_str(),
           "republishing the IDENTICAL payload — the heartbeat bypasses change detection");

  // R4.3.2 is a bound on staleness, so it has to outrank R4.3.1's rate limit rather than be
  // subject to it. Gate the heartbeat behind the rate limit and this device goes an hour silent.
  FakeSink slow;
  MqttPublisher hourly(slow);
  hourly.configure("wm", 3600, 0);
  hourly.tick(0, snap);
  hourly.pump();
  slow.calls.clear();
  hourly.tick(60000, snap);
  hourly.pump();
  check(slow.calls.size() == 3,
        "a 3600 s publish period still heartbeats at 60 s — R4.3.2 outranks the rate limit");

  // millis() wraps every 49.7 days; this device is meant to run for years.
  FakeSink wrapping;
  MqttPublisher late(wrapping);
  late.configure("wm", 10, 0);
  const uint32_t nearOverflow = 0xFFFFF000u;
  late.tick(nearOverflow, snap);
  late.pump();
  wrapping.calls.clear();
  MqttSnapshot moved = snap;
  moved.sensors[0].flowLPerMin = 9.5f;
  late.tick(static_cast<uint32_t>(nearOverflow + 11000u), moved);
  late.pump();
  check(wrapping.calls.size() == 1,
        "a change across the millis() wrap still publishes — the elapsed test is wrap-safe");
}

// ── R4.5.1 availability ─────────────────────────────────────────────────────────────

void availabilityTests() {
  std::printf("\n[availability — R4.5.1, retained online, LWT for offline]\n");

  FakeSink sink;
  MqttPublisher p(sink);
  p.configure("wm", 10, 0);

  const MqttSnapshot snap = oneSensorSnapshot();
  p.tick(1000, snap);
  p.pump();
  sink.calls.clear();

  p.onConnected();
  p.pump();
  check(sink.calls.size() == 1, "connecting queues exactly one message");
  checkStr(at(sink, 0).topic.c_str(), "wm/status", "on <base>/status");
  checkStr(at(sink, 0).payload.c_str(), "online", "saying online");
  check(at(sink, 0).retain, "retained, so a late subscriber sees it (R4.5.1)");

  // Nothing changed and only 1 ms elapsed, so both the rate limit and change detection would
  // normally suppress everything. A reconnect is exactly when a subscriber's picture is emptiest.
  sink.calls.clear();
  p.tick(1001, snap);
  p.pump();
  check(sink.calls.size() == 3,
        "and the next tick republishes in full, bypassing both the window and change detection");
  check(!at(sink, 0).retain,
        "state topics are NOT retained — availability, not a stale reading, reports a dead device");

  sink.calls.clear();
  p.onDisconnected();
  check(p.queued() == 0,
        "disconnecting publishes no 'offline' of its own: the broker's will covers power loss too");
  p.pump();
  check(sink.calls.empty(), "so nothing reaches the sink");
}

// ── R4.1.3 queue policy ─────────────────────────────────────────────────────────────

void dropPolicyTests() {
  std::printf("\n[queue — R4.1.3, oldest telemetry evicted, availability and discovery never]\n");

  FakeSink sink;
  sink.up = false;  // pump is a no-op while down, so the queue genuinely fills
  MqttPublisher p(sink);
  p.configure("wm", 1, 0);

  p.onConnected();
  const char* discoveryTopic = "homeassistant/sensor/wfm_a1b2c3/s1_flow/config";
  check(p.enqueue(MqttClass::Discovery, discoveryTopic, "{\"unique_id\":\"wfm_a1b2c3_s1_flow\"}",
                  true),
        "an availability and a discovery message go in first");
  check(p.queued() == 2, "and both are queued");

  // Flood with telemetry. The snapshot has to MOVE every tick, or change detection would suppress
  // it and the queue would never overrun — a drop test that never drops.
  MqttSnapshot snap = oneSensorSnapshot();
  const int floodTicks = 40;
  for (int i = 0; i < floodTicks; ++i) {
    snap.sensors[0].pulses = static_cast<uint32_t>(i + 1);
    p.tick(static_cast<uint32_t>(1000 * (i + 1)), snap);
  }

  check(p.queued() == MqttPublisher::kQueueCapacity, "the queue is full, not overflowing");
  check(p.droppedTelemetry() > 0, "and it evicted telemetry to stay bounded (R4.1.3)");
  check(p.rejectedMessages() == 0, "nothing was refused outright — there was always telemetry to go");

  sink.up = true;
  const std::size_t drained = p.pump(MqttPublisher::kQueueCapacity);
  check(drained == MqttPublisher::kQueueCapacity, "and the whole queue drains on reconnect");

  checkStr(at(sink, 0).topic.c_str(), "wm/status",
           "the OLDEST message survived — it is the availability one, never droppable");
  checkStr(at(sink, 1).topic.c_str(), discoveryTopic,
           "and so did the discovery message right behind it");
  check(anyPayloadContains(sink, "\"pulses\":40}"),
        "the NEWEST reading survived — stale readings are what get dropped");
  check(!anyPayloadContains(sink, "\"pulses\":1}"), "the oldest reading did not");
  check(countTopic(sink, "wm/total/state") == 0,
        "and the early total was evicted too: age decides within telemetry, not topic");

  // The companion case. With no telemetry left to evict, an incoming message must be REJECTED
  // rather than allowed to displace a discovery message — losing an entity permanently to save one
  // reading is the wrong trade, and R4.1.3 says so.
  std::printf("\n[queue — the hard edge: nothing droppable left]\n");
  FakeSink full;
  full.up = false;
  MqttPublisher q(full);
  q.configure("wm", 1, 0);
  for (std::size_t i = 0; i < MqttPublisher::kQueueCapacity; ++i) {
    char topic[64] = {};
    std::snprintf(topic, sizeof(topic), "homeassistant/sensor/wfm/e%u/config",
                  static_cast<unsigned>(i));
    q.enqueue(MqttClass::Discovery, topic, "{}", true);
  }
  check(q.queued() == MqttPublisher::kQueueCapacity, "a queue full of discovery messages");

  MqttSnapshot fresh = oneSensorSnapshot();
  q.tick(1000, fresh);
  check(q.queued() == MqttPublisher::kQueueCapacity, "telemetry cannot displace any of them");
  check(q.droppedTelemetry() == 0, "nothing was dropped");
  check(q.rejectedMessages() > 0, "the newcomer was rejected instead, and counted");

  full.up = true;
  q.pump(MqttPublisher::kQueueCapacity);
  check(q.publishedMessages() == MqttPublisher::kQueueCapacity,
        "and every discovery message survived to be published");
  check(countTopic(full, "wm/sensor/1/state") == 0, "with no telemetry among them");

  const std::string hugeTopic(MqttPublisher::kMaxTopicBytes + 4, 'x');
  const std::string hugePayload(MqttPublisher::kMaxPayloadBytes + 4, 'y');
  FakeSink small;
  MqttPublisher r(small);
  r.configure("wm", 10, 0);
  check(!r.enqueue(MqttClass::Discovery, hugeTopic.c_str(), "{}", true),
        "an overlong topic is refused rather than silently cut");
  check(!r.enqueue(MqttClass::Discovery, "homeassistant/x/config", hugePayload.c_str(), true),
        "and so is an overlong payload");
  check(r.rejectedMessages() == 2, "both rejections are counted, because §4.4.7 is about silence");
}

// ── R4.1.6 the return value ─────────────────────────────────────────────────────────

void publishFailureTests() {
  std::printf("\n[failures — R4.1.6 / §4.4.7, a -1 must not be swallowed]\n");

  FakeSink sink;
  MqttPublisher p(sink);
  p.configure("wm", 10, 0);
  const MqttSnapshot snap = oneSensorSnapshot();
  p.tick(0, snap);
  check(p.queued() == 3, "three messages are queued");

  sink.failNext = 3;  // every one of them comes back -1
  const std::size_t sent = p.pump();
  check(sent == 0, "pump reports that nothing was sent");
  check(p.publishFailures() == 3, "every -1 was counted (R4.1.6)");
  checkStr(p.lastFailureTopic(), "wm/diagnostics/state",
           "and the failing topic is recorded, so the symptom is not silence");
  check(p.publishedMessages() == 0, "nothing is claimed as published");
  check(p.queued() == 0,
        "the queue is not wedged behind a message that can never go out (oversize is permanent)");

  // The discriminator between `result >= 0` and `result > 0`. QoS 0 always yields message id 0, so a
  // `> 0` test would count EVERY steady-state publish on hardware as a failure while a fake that
  // returned 1 kept this suite green.
  FakeSink zeroIds;
  check(zeroIds.successReturn == 0, "the fake returns the message id QoS 0 really produces: 0");
  MqttPublisher q(zeroIds);
  q.configure("wm", 10, 0);
  q.tick(0, snap);
  check(q.pump() == 3, "a message id of 0 is SUCCESS, not failure");
  check(q.publishFailures() == 0, "so nothing is miscounted");

  // Nothing is published while the sink is down, which is what makes the queue load-bearing.
  FakeSink down;
  down.up = false;
  MqttPublisher r(down);
  r.configure("wm", 10, 0);
  r.tick(0, snap);
  check(r.pump() == 0 && down.calls.empty(), "a disconnected sink is never called");
  check(r.queued() == 3, "and the messages are held, not lost");
  check(r.publishFailures() == 0, "a down link is a normal condition, not a publish failure");
}

// ── §4.2 payload shape ──────────────────────────────────────────────────────────────

void payloadTests() {
  std::printf("\n[payloads — §4.2, one JSON object per sensor]\n");

  FakeSink sink;
  MqttPublisher p(sink);
  p.configure("wm", 10, 2);
  check(p.qos() == 2, "a QoS of 2 configures");

  MqttSnapshot snap;
  snap.sensors[0].present = true;
  snap.sensors[0].flowLPerMin = 1.5f;
  snap.sensors[0].sessionLiters = 250.25f;
  snap.sensors[0].totalCubicMeters = 1.5;
  snap.sensors[0].maxFlowLPerMin = 2.25f;
  snap.sensors[0].pulses = 4242;
  snap.sensors[7].present = true;
  snap.total.flowLPerMin = 1.5f;
  snap.total.sessionLiters = 250.25f;
  snap.total.totalCubicMeters = 1.5;
  snap.total.activeSensors = 2;
  snap.diagnostics.pollingRateKhz = 4.5f;
  snap.diagnostics.baselineRateKhz = 4.75f;
  snap.diagnostics.undersamplingFlags = 3;
  snap.diagnostics.boardTemperatureC = 31.5f;
  snap.diagnostics.uptimeSeconds = 86400;
  snap.diagnostics.wifiRssiDbm = -67;

  p.tick(0, snap);
  p.pump();

  // Exact strings. Every key here is the right-hand side of a value_template in a discovery
  // payload, so renaming one silently stops an HA entity updating with nothing logged anywhere.
  checkStr(lastPayloadFor(sink, "wm/sensor/1/state").c_str(),
           "{\"flow\":1.500,\"session\":250.25,\"total\":1.500000,\"max\":2.250,\"pulses\":4242}",
           "one object per sensor, five metrics — eight publishes per cycle, not forty");
  checkStr(lastPayloadFor(sink, "wm/total/state").c_str(),
           "{\"flow\":1.500,\"session\":250.25,\"total\":1.500000,\"sensors\":2}",
           "the aggregate on <base>/total/state");
  checkStr(lastPayloadFor(sink, "wm/diagnostics/state").c_str(),
           "{\"pollingRateKhz\":4.500,\"baselineKhz\":4.750,\"undersampling\":3,\"tempC\":31.5,"
           "\"uptimeS\":86400,\"rssi\":-67}",
           "and the R2.1.2 pair travels together, so a regression is visible in HA");

  check(countTopic(sink, "wm/sensor/8/state") == 1, "the eighth sensor publishes when present");
  check(countTopic(sink, "wm/sensor/2/state") == 0,
        "an absent sensor gets NO topic — zeros would be indistinguishable from no flow");
  check(at(sink, 0).qos == 2, "the configured QoS reaches the sink");

  const std::string sensorPayload = lastPayloadFor(sink, "wm/sensor/1/state");
  check(sensorPayload.size() < MqttPublisher::kMaxTelemetryPayloadBytes,
        "a telemetry payload fits its buffer with room to spare");
  check(static_cast<int>(sensorPayload.size()) < MqttPublisher::kOutBufferBytes,
        "and sits comfortably inside out_buffer_size (R4.1.6, §4.4.7)");

  // snprintf writes "nan"/"inf" for a non-finite double, which is not valid JSON: HA discards the
  // whole message and the entity stops updating, silently. A sensor divided by a zero elapsed time
  // is one arithmetic slip away from producing exactly that.
  FakeSink nanSink;
  MqttPublisher nanPub(nanSink);
  nanPub.configure("wm", 10, 0);
  MqttSnapshot broken = oneSensorSnapshot();
  broken.sensors[0].flowLPerMin = std::numeric_limits<float>::quiet_NaN();
  broken.sensors[0].totalCubicMeters = std::numeric_limits<double>::infinity();
  nanPub.tick(0, broken);
  nanPub.pump();
  const std::string brokenPayload = lastPayloadFor(nanSink, "wm/sensor/1/state");
  check(brokenPayload.find("\"flow\":null") != std::string::npos,
        "a non-finite value serialises as null, which is valid JSON");
  check(brokenPayload.find("nan") == std::string::npos &&
            brokenPayload.find("inf") == std::string::npos,
        "and never as nan/inf, which HA would reject whole");
}

void classTests() {
  std::printf("\n[classes — R4.1.3's line between expendable and not]\n");

  check(plc::mqttClassIsDroppable(MqttClass::Telemetry), "telemetry may be dropped");
  check(!plc::mqttClassIsDroppable(MqttClass::Availability),
        "availability may not — HA would keep showing values from a device that is gone");
  check(!plc::mqttClassIsDroppable(MqttClass::Discovery),
        "and discovery may not — a lost one is an entity that never appears");
  checkStr(plc::mqttClassName(MqttClass::Availability), "availability",
           "each class names itself for the log");
  checkStr(plc::mqttClassName(MqttClass::Discovery), "discovery", "including discovery");
  checkStr(plc::mqttClassName(MqttClass::Telemetry), "telemetry", "and telemetry");
}

void unconfiguredTests() {
  std::printf("\n[unconfigured — a device nobody set up must be silent]\n");

  FakeSink sink;
  MqttPublisher p(sink);
  const MqttSnapshot snap = oneSensorSnapshot();
  p.tick(0, snap);
  p.onConnected();
  check(p.queued() == 0, "no base topic means nothing is queued, not messages on '/status'");
  p.pump();
  check(sink.calls.empty(), "and nothing reaches the sink");
}

}  // namespace

int main() {
  std::printf("plc::MqttPublisher — MQTT publish policy (WiFi §4.1-§4.5)\n\n");
  topicTests();
  identityTests();
  rateLimitTests();
  heartbeatTests();
  availabilityTests();
  dropPolicyTests();
  publishFailureTests();
  payloadTests();
  classTests();
  unconfiguredTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
