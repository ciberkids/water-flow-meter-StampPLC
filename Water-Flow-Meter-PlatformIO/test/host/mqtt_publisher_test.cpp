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

// The ONE base-topic rule (owner decision 5A). Named explicitly rather than leaned on through
// mqtt_publisher.h, because the agreement checks below call it directly and must not depend on
// another header's include list to keep compiling.
#include "net/net_settings.h"

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

/**
 * The QoS the sink saw for `topic`, or **-1** when no publish on that topic happened at all.
 *
 * -1 rather than 0, and by topic rather than by index, both for the same reason: a QoS check whose
 * "not found" answer is 0 silently passes when the message it names VANISHES, and QoS 0 is exactly
 * the value the telemetry checks below assert. `at(sink, i)` returns a default `Capture{}` with
 * `qos = 0` past the end, so using it here would turn "the sensor never published" into "the sensor
 * published at QoS 0, as expected".
 */
int qosOf(const FakeSink& sink, const char* topic) {
  for (const Capture& c : sink.calls) {
    if (c.topic == topic) return c.qos;
  }
  return -1;
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

  // ── The trailing slash: this block used to assert the opposite ─────────────────────
  //
  // Until owner decision 5A was carried through, these three checks asserted that a trailing '/'
  // was ACCEPTED and STRIPPED. That normalisation is the defect, not a convenience: HaDiscovery
  // refused the identical string, so an operator who typed "watermeter/a1b2c3/" got telemetry on
  // "watermeter/a1b2c3/..." and NOT ONE Home Assistant entity, with the MQTT state register reading
  // connected and nothing logged at either end (§4.4.7). Refusal is reportable — ui_settings.cpp
  // fails the edit and portal_form.cpp reports Refused — and it is what NetSettings::stage() has
  // done with the same string all along.
  MqttPublisher slashed(sink);
  check(!slashed.configure("watermeter/a1b2c3/", 10, 0),
        "a trailing slash is REFUSED, not stripped — the strip is what produced zero HA entities");
  checkStr(slashed.availabilityTopic(), "",
           "and the refusal leaves no availability topic for the transport to use as LWT");
  check(!slashed.configured(), "so the publisher stays unconfigured and silent");

  MqttPublisher manySlashes(sink);
  check(!manySlashes.configure("watermeter/a1b2c3///", 10, 0), "several trailing slashes too");

  // Refusal, not truncation. A truncated topic publishes successfully to the wrong place, which is
  // strictly worse than not publishing: the broker accepts it and nothing anywhere reports a fault.
  char tiny[8] = {};
  check(!p.sensorStateTopic(0, tiny, sizeof(tiny)),
        "a topic that will not fit the caller's buffer is REFUSED");
  check(tiny[0] == '\0', "and the buffer is left empty, never half-written");

  // The length boundary is now the FIELD's capacity, not this buffer's. It moved when configure()
  // started deferring to NetSettings::isValidBaseTopic: 49..109 bytes used to be accepted here and
  // refused by discovery, and no real source can produce them anyway (the field holds 48,
  // mqttDefaultBaseTopic writes 17). Stated as the field capacity rather than as 48 so the pair
  // tracks a change to it instead of pinning today's number.
  const std::size_t cap = plc::netFieldCapacity(plc::NetField::MqttBaseTopic);
  MqttPublisher atCap(sink);
  check(atCap.configure(std::string(cap, 'x').c_str(), 10, 0),
        "a base topic exactly filling the MqttBaseTopic field is accepted");
  MqttPublisher overCap(sink);
  check(!overCap.configure(std::string(cap + 1, 'x').c_str(), 10, 0),
        "and one byte more is refused — the cap is the field's, not the topic buffer's");
  const std::string overlong(MqttPublisher::kMaxTopicBytes, 'x');
  MqttPublisher tooLong(sink);
  check(!tooLong.configure(overlong.c_str(), 10, 0),
        "a base topic the length of the whole topic buffer is refused at configure time");
}

// ── Owner decision 5A: ONE definition of a base topic ────────────────────────────────
//
// The divergence this exists for, measured on the pre-change tree by handing the same string to
// MqttPublisher::configure and HaDiscovery::configure. Six classes disagreed:
//
//   input                     MqttPublisher   HaDiscovery
//   "watermeter/a1b2c3/"      accept (strip)  reject
//   "watermeter/a1b2c3///"    accept (strip)  reject
//   "/watermeter/a1b2c3"      accept          reject
//   "water meter/a1"          accept          reject
//   49..109 bytes             accept          reject
//   0x7f, or any byte >= 0x80 reject          accept
//
// plus "watermeter//a1", which both accepted and the canonical rule refuses.
//
// The first five yield ZERO Home Assistant entities while telemetry flows and the MQTT state
// register reads connected; the sixth advertises entities nothing ever publishes to. Neither is
// logged (§4.4.7), and both look like a broker fault.
//
// The assertion is agreement with the NAMED canonical rule rather than with the other module: the
// expectation is obtained by CALLING plc::NetSettings::isValidBaseTopic, never by restating a bool,
// so a rule change moves both sides together and cannot be satisfied by a stale literal. The
// discovery half of the same corpus is in ha_discovery_test.cpp — agreement with one canonical rule
// on both sides is transitive, and it needs no cross-link between two leaf tests.
//
// What would break this test: reinstating any private rule in MqttPublisher::configure — a strip, a
// per-byte scan, its own length bound.
void baseTopicAgreementTests() {
  std::printf("\n[owner decision 5A — configure() accepts exactly isValidBaseTopic's set]\n");

  struct Case {
    const char* topic;
    const char* what;
  };

  const std::string atCap(plc::netFieldCapacity(plc::NetField::MqttBaseTopic), 'x');
  const std::string overCap(plc::netFieldCapacity(plc::NetField::MqttBaseTopic) + 1, 'x');
  const std::string bufferLength(MqttPublisher::kMaxTopicBytes - 19, 'x');

  const Case cases[] = {
      {"watermeter/a1b2c3", "a plain topic"},
      {"watermeter/a1b2c3/", "DIVERGED: trailing '/' — accepted and stripped here, refused there"},
      {"watermeter/a1b2c3///", "DIVERGED: three trailing '/'"},
      {"/watermeter/a1b2c3", "DIVERGED: leading '/' — accepted here, refused there"},
      {"water meter/a1", "DIVERGED: a space — accepted here, refused there"},
      {"watermeter//a1", "an interior '//' — both modules used to accept it"},
      {"watermeter/a1\x7f", "DIVERGED: DEL 0x7f — refused here, accepted there"},
      {"watermeter/caf\xC3\xA9", "DIVERGED: a UTF-8 byte >= 0x80 — refused here, accepted there"},
      {"watermeter/a1\x01", "a control byte, which both always refused"},
      {"watermeter/+/a", "a '+' wildcard"},
      {"watermeter/#", "a '#' wildcard"},
      {"water\"meter/a1", "a double quote, which is legal and must stay accepted"},
      {"", "the empty string"},
      {"///", "a topic made only of slashes"},
      {nullptr, "a null pointer"},
      {atCap.c_str(), "a topic exactly filling the field"},
      {overCap.c_str(), "DIVERGED: one byte over the field — accepted here, refused there"},
      {bufferLength.c_str(), "DIVERGED: a topic sized to this buffer rather than to the field"},
  };

  FakeSink sink;
  for (const Case& c : cases) {
    MqttPublisher p(sink);
    const bool accepted = p.configure(c.topic, 10, 0);
    const bool canonical = plc::NetSettings::isValidBaseTopic(c.topic);
    check(accepted == canonical, c.what);
    // Acceptance must be VERBATIM. A module that agreed about the verdict and then repaired the
    // string would publish to a topic the operator did not type, which is the trailing-slash defect
    // wearing agreement as a disguise.
    if (accepted) {
      check(std::string(p.baseTopic()) == c.topic, "  and the accepted topic is stored verbatim");
      check(std::string(p.availabilityTopic()) == std::string(c.topic) + "/status",
            "  with <base>/status built from it unrepaired");
    } else {
      check(!p.configured() && p.baseTopic()[0] == '\0',
            "  and a refusal leaves the publisher unconfigured");
    }
  }

  // The floor under the agreement loop. If isValidBaseTopic degenerated to "return true", every row
  // above would still pass as long as configure() degenerated with it, so the verdicts that motivated
  // 5A are also pinned absolutely here.
  MqttPublisher p(sink);
  check(!p.configure("watermeter/a1b2c3/", 10, 0) && !p.configure("/watermeter", 10, 0) &&
            !p.configure("water meter/a1", 10, 0) && !p.configure("watermeter//a1", 10, 0),
        "absolutely: trailing '/', leading '/', a space and an interior '//' are all refused");
  check(p.configure("watermeter/a1b2c3", 10, 0) && p.configure("water\"meter/a1", 10, 0),
        "and a plain topic — or one with a quote in it — is still accepted");

  // The consequence a refusal must have, since §4.4.7's failure mode is silence: nothing is
  // published at all, rather than published somewhere nobody is subscribed.
  MqttPublisher refused(sink);
  refused.configure("watermeter/a1b2c3/", 10, 0);
  refused.onConnected();
  refused.tick(0, oneSensorSnapshot());
  check(refused.queued() == 0, "a refused base topic queues nothing — not even '/status'");
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
  // A DIFFERENT bitmap from the undersampling one, deliberately: the two mean opposite things — a
  // reading that is wrong versus no reading at all — and an assertion where both read 3 would pass
  // just as happily with the two fields swapped.
  snap.diagnostics.uncalibratedFlags = 0x84;
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
  // `uncalibrated` is the only route to the commissioning gap over MQTT: a sensor payload is five
  // readings with no status field, so an uncalibrated channel publishes honest zeros that a subscriber
  // cannot tell from no water. Beside `undersampling` because they are the same shape and the two
  // opposite explanations for a flow of 0.
  checkStr(lastPayloadFor(sink, "wm/diagnostics/state").c_str(),
           "{\"pollingRateKhz\":4.500,\"baselineKhz\":4.750,\"undersampling\":3,"
           "\"uncalibrated\":132,\"tempC\":31.5,\"uptimeS\":86400,\"rssi\":-67}",
           "and the R2.1.2 pair travels together, so a regression is visible in HA");

  check(countTopic(sink, "wm/sensor/8/state") == 1, "the eighth sensor publishes when present");
  check(countTopic(sink, "wm/sensor/2/state") == 0,
        "an absent sensor gets NO topic — zeros would be indistinguishable from no flow");
  // Qualified deliberately. Since owner decision 8A the setting speaks for TELEMETRY only, and an
  // unqualified "the configured QoS reaches the sink" would read as a claim about all three classes
  // — which is now false, and is checked in qosByClassTests().
  check(qosOf(sink, "wm/sensor/1/state") == 2, "the configured QoS reaches the sink for TELEMETRY");

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

// ── Owner decision 8A — per-message-class QoS ───────────────────────────────────────
//
// The whole point of this block is a DISTINCTION, so it is asserted as one: a single publisher, a
// single queue holding all three classes, drained in a single pump, with `config.mqtt.qos` at
// §6.1's default of 0. Two separate tests each run at their own configured QoS would not
// discriminate — one would pass against a publisher that returned 0 for everything and the other
// against one that returned 1 for everything.
//
// And "telemetry is 0 when the setting is 0" is a check at rest on its own: it passes if the split
// is deleted and `slot.qos = qos_` comes back. So it is paired with the same publisher reconfigured
// to 1, where telemetry must MOVE and discovery must NOT.
void qosByClassTests() {
  std::printf("\n[QoS — owner decision 8A, the class decides, not the caller and not one setting]\n");

  FakeSink sink;
  sink.up = false;  // pump is a no-op while down, so all three classes queue up together
  MqttPublisher p(sink);
  check(p.configure("wm", 10, 0), "configured at §6.1's default config.mqtt.qos = 0");
  check(p.qos() == 0, "and the setting really is 0 — this is the value the other two must NOT take");

  const char* discoveryTopic = "homeassistant/sensor/wfm_a1b2c3/s1_flow/config";
  p.onConnected();
  check(p.enqueue(MqttClass::Discovery, discoveryTopic, "{\"unique_id\":\"wfm_a1b2c3_s1_flow\"}",
                  true),
        "an availability and a discovery message are queued alongside telemetry");
  const MqttSnapshot snap = oneSensorSnapshot();
  p.tick(0, snap);

  sink.up = true;
  p.pump();

  // THE three checks, from one drain of one queue.
  check(qosOf(sink, "wm/sensor/1/state") == 0,
        "telemetry goes out at the operator's QoS: 0, because a superseded reading is worthless");
  check(qosOf(sink, discoveryTopic) == 1,
        "discovery goes out at QoS 1 ANYWAY — a lost one is an entity that never appears");
  check(qosOf(sink, "wm/status") == 1,
        "and so does availability — a lost one leaves HA trusting a dead device's last reading");

  // The half that cannot be faked by a constant. Same publisher, setting raised: telemetry has to
  // follow it and the other two have to stay put.
  sink.calls.clear();
  check(p.configure("wm", 10, 1), "the operator raises config.mqtt.qos to 1");
  p.onConnected();
  check(p.enqueue(MqttClass::Discovery, discoveryTopic, "{\"unique_id\":\"wfm_a1b2c3_s1_flow\"}",
                  true),
        "and the same three classes are queued again");
  p.tick(20000, snap);
  p.pump();

  check(qosOf(sink, "wm/sensor/1/state") == 1,
        "telemetry FOLLOWS the setting — it is 1 now, so the 0 above was not a hardcoded 0");
  check(qosOf(sink, discoveryTopic) == 1, "discovery is still 1, having never depended on the setting");
  check(qosOf(sink, "wm/status") == 1, "as is availability");

  // The policy on its own, so a regression names the class rather than the topic. Read as a table:
  // one row moves with the setting, four do not.
  std::printf("\n[QoS — the class-to-QoS table itself]\n");
  check(plc::mqttClassQos(MqttClass::Telemetry, 0) == 0, "telemetry at setting 0 is 0");
  check(plc::mqttClassQos(MqttClass::Telemetry, 1) == 1, "telemetry at setting 1 is 1");
  check(plc::mqttClassQos(MqttClass::Discovery, 0) == 1, "discovery at setting 0 is still 1");
  check(plc::mqttClassQos(MqttClass::Discovery, 1) == 1, "and at setting 1 is 1");
  check(plc::mqttClassQos(MqttClass::Availability, 0) == 1, "availability at setting 0 is still 1");
  check(plc::mqttClassQos(MqttClass::Availability, 1) == 1, "and at setting 1 is 1");

  // "QoS 1 ALWAYS, regardless of the setting" means regardless in BOTH directions, so the setting is
  // not a floor either. Reachable only because MqttPublisher::configure clamps at 2 while §6.1's
  // enum and NetSettings::stageMqttQos both stop at 1 — but a `max(configuredQos, 1)` would pass
  // every other check in this block, so the case that distinguishes it is asserted rather than left
  // to whoever next widens the setting's range.
  check(plc::mqttClassQos(MqttClass::Discovery, 2) == 1,
        "a setting of 2 does not raise discovery either — the setting has no say here at all");
  check(plc::mqttClassQos(MqttClass::Availability, 2) == 1, "nor availability");
  check(plc::kMqttReliableQos == 1, "and the QoS they take is 1 — at-least-once, which is the point");

  // Through the publisher too, because the table being right does not prove enqueue() consults it.
  FakeSink viaMember;
  MqttPublisher m(viaMember);
  m.configure("wm", 10, 0);
  check(m.qosFor(MqttClass::Telemetry) == 0 && m.qosFor(MqttClass::Discovery) == 1 &&
            m.qosFor(MqttClass::Availability) == 1,
        "qosFor() reports the per-class figure, so a log line need not repeat the setting and lie");

  // ── The two halves of R4.1.3 and 8A, asserted together ─────────────────────────────
  //
  // They are one argument: a message the queue refuses to evict is a message the wire must not send
  // best-effort. Protecting discovery from our own overflow and then handing it to a QoS-0 publish
  // would be R4.1.3 defending it from us and not from the network.
  std::printf("\n[QoS — the never-dropped classes are also the never-best-effort ones]\n");
  FakeSink flooded;
  flooded.up = false;
  MqttPublisher q(flooded);
  q.configure("wm", 1, 0);
  q.onConnected();
  q.enqueue(MqttClass::Discovery, discoveryTopic, "{\"unique_id\":\"x\"}", true);

  // The snapshot must MOVE every tick or change detection suppresses it and the queue never fills.
  MqttSnapshot moving = oneSensorSnapshot();
  for (int i = 0; i < 40; ++i) {
    moving.sensors[0].pulses = static_cast<uint32_t>(i + 1);
    q.tick(static_cast<uint32_t>(1000 * (i + 1)), moving);
  }
  check(q.droppedTelemetry() > 0, "the queue really did overflow and evict telemetry (R4.1.3)");

  flooded.up = true;
  q.pump(MqttPublisher::kQueueCapacity);
  check(qosOf(flooded, "wm/status") == 1,
        "the availability message survived the flood AND went out at QoS 1");
  check(qosOf(flooded, discoveryTopic) == 1, "so did the discovery message behind it");
  check(qosOf(flooded, "wm/sensor/1/state") == 0,
        "while the telemetry that survived is still best-effort, as configured");
}

// ── R4.1.6 / §4.4.7 — what has to fit out_buffer_size is the PACKET ─────────────────
//
// esp-mqtt assembles the whole PUBLISH into one buffer sized by `out_buffer_size`
// (esp-mqtt/lib/mqtt_msg.c: `set_message_header_size` reserves MQTT_MAX_FIXED_HEADER_SIZE = 5
// unconditionally, `append_string` writes a 2-byte length prefix and refuses at
// `length + len + 2 > buffer_length`, `append_message_id` adds 2 more when `qos > 0`). Bounding the
// payload alone — which mqtt_publisher.h used to do — leaves 135 bytes of topic and framing
// uncounted, and going over is §4.4.7's silent failure.
//
// The compile-time guarantee is the widened static_assert in mqtt_publisher.h. Restating its
// arithmetic here would be worthless — the assert fires before the binary exists — and a first
// attempt at this test did exactly that in a way that looked like measurement: it sized the packet
// from the SAME `kPublish*Bytes` constants the header derives its bound from, so zeroing
// `kPublishFixedHeaderBytes` moved both sides equally and the suite stayed green. Recorded because it
// is the failure mode this file keeps rediscovering.
//
// So the framing numbers below are LITERALS, read out of esp-mqtt's serialiser rather than out of
// our own header, and the header's constants are checked against them. That is the only version that
// can be red while the compile is green.
void packetBoundTests() {
  std::printf("\n[buffer — the worst-case PUBLISH packet against out_buffer_size (R4.1.6)]\n");

  // Straight from esp-mqtt/lib/mqtt_msg.c (IDF 4.4 layout, unchanged in 5.x). If a future IDF
  // changes any of these three, that is the moment somebody needs to know — and it is a silent
  // truncation at the client if nobody does (§4.4.7).
  const std::size_t kIdfFixedHeader = 5;   // MQTT_MAX_FIXED_HEADER_SIZE, mqtt_msg.c:37; reserved
                                           // unconditionally by set_message_header_size()
  const std::size_t kIdfTopicPrefix = 2;   // append_string(): two length bytes, refuses at
                                           // `length + len + 2 > buffer_length`
  const std::size_t kIdfPacketId = 2;      // append_message_id(): two bytes, and only when qos > 0
  check(MqttPublisher::kPublishFixedHeaderBytes == kIdfFixedHeader,
        "the header's fixed-header constant is esp-mqtt's MQTT_MAX_FIXED_HEADER_SIZE of 5");
  check(MqttPublisher::kPublishTopicLengthBytes == kIdfTopicPrefix,
        "and its topic prefix is append_string()'s two length bytes");
  check(MqttPublisher::kPublishPacketIdBytes == kIdfPacketId,
        "and its packet identifier is append_message_id()'s two");

  // One byte under each cap: enqueue() refuses at `strlen >= cap`, so these are the longest strings
  // that can reach the sink at all.
  const std::string maxTopic(MqttPublisher::kMaxTopicBytes - 1, 't');
  const std::string maxPayload(MqttPublisher::kMaxPayloadBytes - 1, 'p');

  FakeSink sink;
  MqttPublisher p(sink);
  p.configure("wm", 10, 0);  // the operator's telemetry QoS at its default, deliberately
  check(p.enqueue(MqttClass::Discovery, maxTopic.c_str(), maxPayload.c_str(), true),
        "a message one byte under each cap is accepted — the caps are reachable, not decorative");
  check(p.pump() == 1, "and reaches the sink");

  const Capture largest = at(sink, 0);
  check(largest.topic.size() == MqttPublisher::kMaxTopicBytes - 1 &&
            largest.payload.size() == MqttPublisher::kMaxPayloadBytes - 1,
        "whole, at both caps — nothing was truncated on the way through the queue");

  // 8A is what makes the packet identifier unconditional on this path: the largest message this
  // device sends is a discovery payload, and discovery is now always QoS >= 1. At the old
  // `slot.qos = qos_` a device left at the default published it at QoS 0 and paid no packet id, so
  // these two bytes are part of the worst case only BECAUSE of the decision above.
  check(largest.qos >= 1, "the largest message is discovery, so 8A puts it at QoS 1");

  // Sized from the LITERALS above and from what the sink actually received — no `kPublish*Bytes` on
  // this side of the comparison, which is the whole point.
  const std::size_t packetIdBytes = largest.qos > 0 ? kIdfPacketId : 0;
  const std::size_t measured =
      kIdfFixedHeader + kIdfTopicPrefix + largest.topic.size() + packetIdBytes +
      largest.payload.size();

  std::printf("      measured worst-case packet %zu bytes = 5 + 2 + %zu topic + %zu id + %zu payload"
              "; header's bound %zu, out_buffer_size %d\n",
              measured, largest.topic.size(), packetIdBytes, largest.payload.size(),
              MqttPublisher::kWorstCasePublishBytes, MqttPublisher::kOutBufferBytes);
  // Printed, not asserted, and deliberately: "the packet is bigger than the payload" is arithmetic
  // that cannot be false, and a check that cannot fail is worse than a number nobody asserted.
  std::printf("      the payload-only bound it replaces counted %zu of those %zu bytes\n",
              MqttPublisher::kMaxPayloadBytes, measured);

  // The direction that matters, and the one the static_assert cannot cover: UNDERSTATING
  // kWorstCasePublishBytes — dropping a term while "simplifying" the formula — makes the
  // compile-time bound more permissive, so the assert stays green while the constant stops
  // describing the packet. Only a measurement taken independently of that constant catches it.
  check(measured <= MqttPublisher::kWorstCasePublishBytes,
        "the header's constant really is an upper bound on the real packet, not an understatement");

  // This one CANNOT be red while the compile is green — the static_assert fires first. Kept because
  // it is the sentence the requirement is written in, and it costs one comparison to state it.
  check(measured <= static_cast<std::size_t>(MqttPublisher::kOutBufferBytes),
        "the largest message the queue accepts, serialised as esp-mqtt serialises it, fits");

  check(MqttPublisher::kOutBufferBytes == static_cast<int>(plc::kMqttOutBufferSize),
        "measured against mqtt_limits.h's ONE definition of out_buffer_size (R4.4.8)");
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
  baseTopicAgreementTests();
  identityTests();
  rateLimitTests();
  heartbeatTests();
  availabilityTests();
  dropPolicyTests();
  publishFailureTests();
  payloadTests();
  classTests();
  qosByClassTests();
  packetBoundTests();
  unconfiguredTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
