// DF30 — Home Assistant discovery as a job across logic passes, run in the firmware's own order.
//
// WHY THIS TEST EXISTS. `publishDiscovery()` enqueued the whole discovery set — 3 per channel in
// service plus 7, so 31 with all eight — into a 16-slot queue with no pump in between, and the
// policy re-asked on every pass once a set had failed to fit. `ha_discovery_test` checked every
// payload and `mqtt_publisher_test` the queue's policy, each on its own, and both were green
// throughout, because the ORDER they were called in lived in firmware.cpp, which no host test links.
// So this binary links the real `mqtt_publisher.cpp`, `ha_discovery.cpp`, `mqtt_command_router.cpp`
// and `mqtt_reconnect.cpp`, drives `consumeMqttLinkLatches` and `serviceMqttConnectedPass` from
// net/ha_discovery_job.h exactly as firmware.cpp calls them, and then reads firmware.cpp as text to
// hold it to those calls.
//
// WHAT "REACHES THE SINK" MEANS here: handed to `esp_mqtt_client_enqueue`, which is what the sink
// stands in for. Not received by Home Assistant. esp-mqtt can still expire a message silently 30 s
// after the hand-over, and nothing on a host can see that; it is `DF31`.
//
// Built two ways. Normally against this tree. With -DDF30_TODAY, the firmware glue below is swapped
// for a transcription of firmware.cpp at 65b36f0e — the connect-first latch order, the duplicate
// availability message, `publishDiscovery()`, retained births latched, a teardown that clears
// nothing — and the binary is linked against that commit's sources, so the same checks can be shown
// to FAIL against the behaviour this replaced. That variant is a test of the test and is not in
// run.sh.
//
// Time is a host model: one pass is `kPassMs`, the logic task's floor (`vTaskDelay(1)` at 1 kHz).
// Pass counts are what the job's bounds are about; the 10 s period, the 60 s heartbeat and the 60 s
// retry are in milliseconds.
#ifndef DF30_TODAY
#include "net/ha_discovery_job.h"
#else
#include "net/ha_discovery.h"
#include "net/mqtt_publisher.h"
#include "net/mqtt_reconnect.h"
#endif

#include "firmware_source_text.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <set>
#include <string>
#include <vector>

namespace {

using plc::HaDeviceIdentity;
using plc::HaDiscovery;
using plc::HaEntity;
using plc::HaEntityRef;
using plc::HaRepublishPolicy;
using plc::HaRepublishReason;
using plc::MqttClass;
using plc::MqttCommandResult;
using plc::MqttPublisher;
using plc::MqttSnapshot;

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-86s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

/**
 * One NAMED check evaluated for every k = 0..8 and printed once, with the k that failed.
 *
 * Every route runs for every number of channels in service, and printing 9 lines per claim would bury
 * the claim; printing one line that hides which k broke would bury the failure.
 */
struct NamedCheck {
  std::string name;
  std::vector<int> failingK;
};
std::vector<NamedCheck> pendingChecks;

void expectK(const std::string& name, int k, bool ok) {
  for (NamedCheck& c : pendingChecks) {
    if (c.name == name) {
      // A route run at several offsets for one k fails as that k once.
      if (!ok && std::find(c.failingK.begin(), c.failingK.end(), k) == c.failingK.end()) {
        c.failingK.push_back(k);
      }
      return;
    }
  }
  pendingChecks.push_back({name, ok ? std::vector<int>{} : std::vector<int>{k}});
}

void flushChecks() {
  for (const NamedCheck& c : pendingChecks) {
    ++checks;
    const bool ok = c.failingK.empty();
    std::printf("  %-86s %s\n", c.name.c_str(), ok ? "ok" : "FAIL");
    if (!ok) {
      ++failures;
      std::printf("      failing for k =");
      for (int k : c.failingK) std::printf(" %d", k);
      std::printf("\n");
    }
  }
  pendingChecks.clear();
}

// ── The fixed facts the routes run against ─────────────────────────────────────────────

constexpr uint32_t kPassMs = 1;
constexpr const char* kBaseA = "watermeter/a1b2c3";
constexpr const char* kPrefixA = "homeassistant";
// The teardown route moves BOTH, so a message left over from the old client is recognisable.
constexpr const char* kBaseB = "plant/meter-two";
constexpr const char* kPrefixB = "ha-test";
constexpr uint8_t kMac[6] = {0x24, 0x6f, 0x28, 0xa1, 0xb2, 0xc3};

#ifndef DF30_TODAY
constexpr std::size_t kPerPass = MqttPublisher::kMessagesPerPass;
#else
constexpr std::size_t kPerPass = 4;  // firmware.cpp:1515 at 65b36f0e, `mqttPublisher.pump(4)`
#endif

/** Passes for a whole queue to drain at `kPerPass`, counting the pass it starts on: what the full
 *  set needs once it is queued behind at most a window of reliable messages and an availability. */
constexpr std::size_t kFullSetPasses = (MqttPublisher::kQueueCapacity + kPerPass - 1) / kPerPass;

/** Payload limit at which exactly two real configs no longer fit: *Board temperature* needs 463
 *  bytes and *Polling rate* 464, and every other one 460 or fewer (measured with this tree's builder,
 *  for the identity below). */
constexpr std::size_t kTruncatingPayloadLimit = 463;

uint16_t bitmapOf(int k) { return static_cast<uint16_t>((1u << k) - 1u); }

struct Capture {
  uint32_t pass = 0;
  std::string topic;
  std::string payload;
  int qos = 0;
  bool retain = false;
};

/**
 * The transport, as `MqttPublisher` sees it. Copied in spirit from mqtt_publisher_test.cpp's, which
 * is local to that file: records every hand-over, returns 0 at QoS 0 as esp-mqtt does, and can be
 * told to fail a topic (esp-mqtt's -1 is heap exhaustion) or to stall.
 *
 * `stalled` is the stuck-job seam: the device's own idea of the link stays up, so the job runs,
 * while `pump()` sees the sink down and drains nothing.
 */
class FakeSink : public plc::MqttSink {
 public:
  int publish(const char* topic, const char* payload, int qos, bool retain) override {
    calls.push_back({pass, topic, payload, qos, retain});
    ++callsThisPass;
    if (!failTopic.empty() && failTopic == topic && failTimes > 0) {
      --failTimes;
      return -1;
    }
    return qos > 0 ? ++lastId : 0;
  }
  bool connected() const override { return up && !stalled; }

  bool up = false;
  bool stalled = false;
  uint32_t pass = 0;
  std::size_t callsThisPass = 0;
  int lastId = 0;
  std::string failTopic;
  int failTimes = 0;
  std::vector<Capture> calls;
};

/** The number after `"key":` in a JSON payload, or -1 — how a state message names its session. */
long tagOf(const std::string& payload, const char* key) {
  const std::string needle = std::string("\"") + key + "\":";
  const std::size_t at = payload.find(needle);
  if (at == std::string::npos) return -1;
  return std::strtol(payload.c_str() + at + needle.size(), nullptr, 10);
}

/**
 * One device: the real publisher, discovery builder, policy, ladder and job, plus the firmware's
 * latches and the order firmware.cpp runs them in.
 */
struct Device {
  FakeSink sink;
  MqttPublisher pub{sink};
  HaDiscovery ha;
  HaRepublishPolicy policy;
  plc::MqttReconnect ladder{kMac};
#ifndef DF30_TODAY
  plc::HaDiscoveryJob job;
#else
  std::size_t payloadLimit = MqttPublisher::kMaxPayloadBytes;
#endif

  std::string base = kBaseA;
  std::string prefix = kPrefixA;
  uint16_t bitmap = 0;
  bool discoveryEnabled = true;

  // firmware.cpp's latches — `volatile bool`s esp-mqtt's task sets.
  bool connectLatched = false;
  bool disconnectLatched = false;
  bool birthLatched = false;
  bool commandNextPass = false;  // one accepted `cmd/republish`
  bool commandEveryPass = false;
  /** The race seam: esp-mqtt's CONNECTED lands AFTER this pass read the latches and BEFORE the
   *  connected block's gate, so the transport says connected while the latch is still unconsumed. */
  bool connectArrivesBeforeGate = false;
  MqttCommandResult lastCommand = MqttCommandResult::Idle;

  uint32_t now = 0;
  uint32_t pass = 0;
  uint32_t generation = 0;  // connects consumed; tags every state message with its session

  // What the routes assert on.
  uint32_t starts = 0;
  uint32_t sensorsChangedAnswers = 0;
  uint32_t holdLimitHits = 0;
  std::size_t maxBuildsPerPass = 0;
  std::size_t maxSinkCallsPerPass = 0;
  std::size_t longestHold = 0;
  std::size_t currentHold = 0;
  struct Connect {
    uint32_t pass = 0;
    uint32_t generation = 0;
    std::size_t firstCall = 0;
    uint32_t releasePass = 0;  // first pass tick() ran after this connect
    bool released = false;
  };
  std::vector<Connect> connects;

  explicit Device(int k, std::size_t payloadLimitBytes = MqttPublisher::kMaxPayloadBytes)
#ifndef DF30_TODAY
      : job(payloadLimitBytes)
#endif
  {
#ifdef DF30_TODAY
    payloadLimit = payloadLimitBytes;
#endif
    bitmap = bitmapOf(k);
    configureClient(kBaseA, kPrefixA);
  }

  /** What firmware.cpp does when it creates a client: configure the publisher, then discovery from the
   *  SAME base topic. */
  void configureClient(const char* newBase, const char* newPrefix) {
    base = newBase;
    prefix = newPrefix;
    pub.configure(newBase, MqttPublisher::kDefaultPublishPeriodS, 0);
    HaDeviceIdentity id;
    id.nodeId = "wfm_a1b2c3";
    id.name = "Water Flow Meter";
    id.swVersion = "0.3-dev";
    ha.configure(id, newPrefix, newBase);
  }

  // ── esp-mqtt's task ───────────────────────────────────────────────────────────────────
  void brokerConnects() {
    sink.up = true;
    connectLatched = true;
  }
  void brokerDrops() {
    sink.up = false;
    disconnectLatched = true;
  }
  /** `onMqttData` for `<prefix>/status online`. */
  void haBirth(bool retained) {
    const std::string topic = prefix + "/status";
#ifndef DF30_TODAY
    if (policy.onStatusMessage(ha, topic.c_str(), "online", retained) != HaRepublishReason::None) {
      birthLatched = true;
    }
#else
    (void)retained;  // firmware.cpp:451 at 65b36f0e looked at the topic and payload only
    if (policy.onStatusMessage(ha, topic.c_str(), "online") != HaRepublishReason::None) {
      birthLatched = true;
    }
#endif
  }

  MqttSnapshot snapshot() const {
    MqttSnapshot s;
    for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
      s.sensors[i].present = ((bitmap >> i) & 1u) != 0;
      s.sensors[i].pulses = generation;  // the session tag, read back by tagOf()
    }
    s.total.activeSensors = static_cast<uint8_t>(generation);
    s.diagnostics.uncalibratedFlags = static_cast<uint16_t>(generation);
    s.diagnostics.uptimeSeconds = now / 1000;  // changes each second, as firmware.cpp's does
    s.diagnostics.lastCommandResult = lastCommand;
    return s;
  }

  void noteConnectConsumed() {
    ++generation;
    connects.push_back({pass, generation, sink.calls.size(), 0, false});
    // A hold is per connect: one the link dropped in the middle of is over, and the job starts the
    // new connect's budget from the top.
    currentHold = 0;
  }
  void noteTicked() {
    if (!connects.empty() && !connects.back().released) {
      connects.back().released = true;
      connects.back().releasePass = pass;
    }
  }

  /** serviceMqttCommand (firmware.cpp) for an accepted `cmd/republish`: R4.4.2d's full set for every
   *  result, then the birth latch. Dropped unless the link is up (R4.4.5). */
  void connectRace() {
    if (!connectArrivesBeforeGate) return;
    connectArrivesBeforeGate = false;
    sink.up = true;
    connectLatched = true;
  }

  void serviceCommand() {
    if (!(commandNextPass || commandEveryPass)) return;
    commandNextPass = false;
    if (!sink.up) return;
    lastCommand = MqttCommandResult::Accepted;
    pub.requestFullPublish();
    birthLatched = true;
  }

#ifndef DF30_TODAY
  // ── The firmware's order, this tree ───────────────────────────────────────────────────
  void runPass() {
    sink.pass = pass;
    sink.callsThisPass = 0;
    const uint16_t sensorsBitmap = bitmap;  // firmware.cpp reads the global once per pass
    const bool d = disconnectLatched;
    if (d) disconnectLatched = false;
    const bool c = connectLatched;
    if (c) connectLatched = false;
    const plc::MqttLinkLatchReport link = plc::consumeMqttLinkLatches(
        d, c, discoveryEnabled, sensorsBitmap, ha, ladder, pub, policy, job);
    if (link.connected) noteConnectConsumed();
    if (link.discoveryStarted) ++starts;
    serviceCommand();
    connectRace();
    if (sink.up && !connectLatched) {  // `if (mqttTransport.connected() && !mqttConnectLatched)`
      const bool birth = birthLatched;
      if (birth) birthLatched = false;
      const plc::MqttConnectedPassReport r = plc::serviceMqttConnectedPass(
          job, ha, pub, policy, birth, discoveryEnabled, sink.up, sensorsBitmap, now, snapshot());
      if (r.requestStarted) ++starts;
      if (r.step.started != plc::HaDiscoveryStart::None) ++starts;
      if (r.sensorsChanged) ++sensorsChangedAnswers;
      if (r.step.holdLimitReached) ++holdLimitHits;
      maxBuildsPerPass = std::max<std::size_t>(maxBuildsPerPass, r.step.built);
      if (r.telemetryHeld) {
        longestHold = std::max(longestHold, ++currentHold);
      } else {
        currentHold = 0;
        noteTicked();
      }
    }
    maxSinkCallsPerPass = std::max(maxSinkCallsPerPass, sink.callsThisPass);
    now += kPassMs;
    ++pass;
  }

  /** stopMqttClient() (firmware.cpp): end(), then the session and all four latches go. */
  void teardown(const char* newBase, const char* newPrefix) {
    sink.up = false;  // EspMqttTransport::end() clears connected_ and dispatches nothing
    plc::endMqttSession(job, pub, policy);
    connectLatched = false;
    disconnectLatched = false;
    birthLatched = false;
    commandNextPass = false;
    ladder.reset();
    configureClient(newBase, newPrefix);
  }

  bool jobIdle() const { return !job.active(); }
#else
  // ── firmware.cpp at 65b36f0e, transcribed ─────────────────────────────────────────────
  /** :577-614 */
  void publishDiscovery() {
    if (!discoveryEnabled || !ha.configured()) return;
    ++starts;
    HaEntityRef refs[plc::kHaMaxEntities] = {};
    const std::size_t count = ha.enumerateEntities(bitmap, refs, plc::kHaMaxEntities);
    std::size_t accepted = 0;
    for (std::size_t i = 0; i < count; ++i) {
      char topic[MqttPublisher::kMaxTopicBytes] = {};
      char payload[MqttPublisher::kMaxPayloadBytes] = {};
      const auto t = ha.discoveryTopic(refs[i], topic, sizeof(topic));
      const auto p = ha.discoveryPayload(refs[i], payload, payloadLimit);
      if (!t.complete || !p.complete) continue;
      if (pub.enqueue(MqttClass::Discovery, topic, payload, true)) ++accepted;
    }
    buildsThisPass += count;
    if (accepted == count && count > 0) policy.notePublished(bitmap);
  }
  std::size_t buildsThisPass = 0;

  void runPass() {
    sink.pass = pass;
    sink.callsThisPass = 0;
    buildsThisPass = 0;
    // :1288-1323, the CONNECTED consumed first
    if (connectLatched) {
      connectLatched = false;
      ladder.noteConnected();
      pub.onConnected();
      // :1315-1316, the second availability message
      pub.enqueue(MqttClass::Availability, pub.availabilityTopic(), "online", true);
      noteConnectConsumed();
      if (policy.onConnected() != HaRepublishReason::None) publishDiscovery();
    }
    // :1325-1334
    if (disconnectLatched) {
      disconnectLatched = false;
      ladder.noteDisconnected();
      pub.onDisconnected();
      policy.noteDisconnected();
    }
    serviceCommand();  // :1351
    connectRace();
    if (sink.up) {     // :1444
      if (birthLatched) {
        birthLatched = false;
        publishDiscovery();
      }
      if (policy.onSensorBitmap(bitmap) != HaRepublishReason::None) {
        ++sensorsChangedAnswers;
        publishDiscovery();
      }
      pub.tick(now, snapshot());  // :1511
      noteTicked();
      pub.pump(4);  // :1515
    }
    maxBuildsPerPass = std::max(maxBuildsPerPass, buildsThisPass);
    maxSinkCallsPerPass = std::max(maxSinkCallsPerPass, sink.callsThisPass);
    now += kPassMs;
    ++pass;
  }

  /** :1357-1371 — end(), and nothing else of the session. */
  void teardown(const char* newBase, const char* newPrefix) {
    sink.up = false;
    ladder.reset();
    configureClient(newBase, newPrefix);
  }

  bool jobIdle() const { return true; }
#endif

  void run(uint32_t passes) {
    for (uint32_t i = 0; i < passes; ++i) runPass();
  }

  // ── Reading the sink ──────────────────────────────────────────────────────────────────

  /** The config topics the builder makes for `forBitmap` under the current prefix. */
  std::vector<std::string> expectedConfigs(uint16_t forBitmap) const {
    std::vector<std::string> out;
    HaEntityRef refs[plc::kHaMaxEntities] = {};
    const std::size_t n = ha.enumerateEntities(forBitmap, refs, plc::kHaMaxEntities);
    for (std::size_t i = 0; i < n; ++i) {
      char topic[MqttPublisher::kMaxTopicBytes] = {};
      ha.discoveryTopic(refs[i], topic, sizeof(topic));
      out.push_back(topic);
    }
    return out;
  }

  std::set<std::string> configsSince(std::size_t firstCall) const {
    std::set<std::string> out;
    const std::string under = prefix + "/";
    for (std::size_t i = firstCall; i < sink.calls.size(); ++i) {
      if (sink.calls[i].topic.rfind(under, 0) == 0) out.insert(sink.calls[i].topic);
    }
    return out;
  }

  /** Every config of `forBitmap` handed to the sink at or after `firstCall`. */
  bool everyConfigSince(std::size_t firstCall, uint16_t forBitmap) const {
    const std::set<std::string> got = configsSince(firstCall);
    for (const std::string& t : expectedConfigs(forBitmap)) {
      if (got.count(t) == 0) return false;
    }
    return true;
  }

  std::size_t discoveryCallsSince(std::size_t firstCall) const {
    std::size_t n = 0;
    const std::string under = prefix + "/";
    for (std::size_t i = firstCall; i < sink.calls.size(); ++i) {
      if (sink.calls[i].topic.rfind(under, 0) == 0) ++n;
    }
    return n;
  }

  std::size_t firstCallAt(const std::string& topic, std::size_t from, const char* tagKey,
                          long tag) const {
    for (std::size_t i = from; i < sink.calls.size(); ++i) {
      if (sink.calls[i].topic == topic && (tagKey == nullptr || tagOf(sink.calls[i].payload, tagKey) == tag)) {
        return i;
      }
    }
    return std::string::npos;
  }

  /**
   * Owner decision B: on the connect `c`, every config reaches the sink before the first state
   * message of THAT session that its entity reads.
   *
   * "Of that session" by its tag, because a state message queued before a drop is still in the queue
   * after it (the queue survives a disconnect, on purpose) and goes out first. It was queued after its
   * configs in the earlier session, in a first-in-first-out queue, which `noStateBeforeAnyConfig`
   * below holds across the whole run.
   */
  bool configsBeforeStates(const Connect& c) const {
    HaEntityRef refs[plc::kHaMaxEntities] = {};
    const std::size_t n = ha.enumerateEntities(bitmap, refs, plc::kHaMaxEntities);
    for (std::size_t i = 0; i < n; ++i) {
      if (plc::haEntityIsButton(refs[i].kind)) continue;  // a button reads no state
      char config[MqttPublisher::kMaxTopicBytes] = {};
      ha.discoveryTopic(refs[i], config, sizeof(config));
      char state[MqttPublisher::kMaxTopicBytes] = {};
      const char* key = "uncalibrated";
      if (plc::haEntityIsPerSensor(refs[i].kind)) {
        pub.sensorStateTopic(refs[i].sensor, state, sizeof(state));
        key = "pulses";
      } else {
        pub.diagnosticsStateTopic(state, sizeof(state));
      }
      const std::size_t firstState = firstCallAt(state, c.firstCall, key, static_cast<long>(c.generation));
      const std::size_t firstConfig = firstCallAt(config, c.firstCall, nullptr, 0);
      if (firstConfig == std::string::npos) return false;
      if (firstState != std::string::npos && firstState < firstConfig) return false;
    }
    return true;
  }

  /** No state message reaches the sink, ever, before some config for an entity that reads it: a
   *  channel's state before its *Flow* config, the diagnostics before *Board temperature*'s. */
  bool noStateBeforeAnyConfig() const {
    std::set<std::string> seenConfigs;
    const std::string sensorPrefix = base + "/sensor/";
    const std::string diagnostics = base + "/diagnostics/state";
    const std::string boardTemperature = prefix + "/sensor/wfm_a1b2c3/board_temperature/config";
    for (const Capture& call : sink.calls) {
      if (call.topic.find("/config") != std::string::npos) {
        seenConfigs.insert(call.topic);
        continue;
      }
      if (call.topic.rfind(sensorPrefix, 0) == 0) {
        const int n = std::atoi(call.topic.c_str() + sensorPrefix.size());
        const std::string flow = prefix + "/sensor/wfm_a1b2c3/s" + std::to_string(n) + "_flow/config";
        if (seenConfigs.count(flow) == 0) return false;
      }
      if (call.topic == diagnostics && seenConfigs.count(boardTemperature) == 0) return false;
    }
    return true;
  }

  /** The full telemetry set of connect `c`'s session, within `kFullSetPasses` of its release. */
  bool fullSetSoonAfterRelease(const Connect& c) const {
    if (!c.released) return false;
    const uint32_t lastPass = c.releasePass + static_cast<uint32_t>(kFullSetPasses) - 1;
    std::vector<std::pair<std::string, const char*>> want;
    for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
      if (((bitmap >> i) & 1u) == 0) continue;
      char t[MqttPublisher::kMaxTopicBytes] = {};
      pub.sensorStateTopic(i, t, sizeof(t));
      want.push_back({t, "pulses"});
    }
    char total[MqttPublisher::kMaxTopicBytes] = {};
    pub.totalStateTopic(total, sizeof(total));
    want.push_back({total, "sensors"});
    char diagnostics[MqttPublisher::kMaxTopicBytes] = {};
    pub.diagnosticsStateTopic(diagnostics, sizeof(diagnostics));
    want.push_back({diagnostics, "uncalibrated"});
    for (const auto& w : want) {
      const std::size_t at = firstCallAt(w.first, c.firstCall, w.second, static_cast<long>(c.generation));
      if (at == std::string::npos || sink.calls[at].pass > lastPass) return false;
    }
    return true;
  }

  bool recorded() const { return policy.everPublished() && policy.publishedBitmap() == bitmap; }
};

/** The claims every connect must meet, for every k. */
void expectConnect(const std::string& route, int k, const Device& d, const Device::Connect& c) {
  expectK(route + ": every one of the 3k+7 configs reaches the sink", k,
          d.everyConfigSince(c.firstCall, d.bitmap));
  expectK(route + ": each config reaches the sink before its entity's first state", k,
          d.configsBeforeStates(c));
  expectK(route + ": no channel's state ever reaches the sink before a config for it", k,
          d.noStateBeforeAnyConfig());
  expectK(route + ": the full telemetry set reaches the sink within 4 passes of the hold", k,
          d.fullSetSoonAfterRelease(c));
  expectK(route + ": the set is recorded, publishedBitmap() == the bitmap", k, d.recorded());
}

void expectBounded(const std::string& route, int k, const Device& d) {
  expectK(route + ": at most 4 builds and 4 sink calls in any pass", k,
          d.maxBuildsPerPass <= kPerPass && d.maxSinkCallsPerPass <= kPerPass);
}

// ═══════════════════════════════════════════════════════════════════════════════════════
// Routes, each for k = 0..8
// ═══════════════════════════════════════════════════════════════════════════════════════

void connectRoutes() {
  std::printf("\n[DF30 — a connect, a reconnect, and a drop at several points mid-set]\n");
  for (int k = 0; k <= 8; ++k) {
    {
      Device d(k);
      d.brokerConnects();
      d.run(300);
      expectConnect("first connect", k, d, d.connects.at(0));
      expectK("first connect: nothing refused, no telemetry evicted", k,
              d.pub.rejectedMessages() == 0 && d.pub.droppedTelemetry() == 0);
      expectK("first connect: one set started", k, d.starts == 1);
      expectBounded("first connect", k, d);
      const uint32_t startsBefore = d.starts;
      d.run(5000);
      expectK("first connect: no set starts in the 5 s after it (no storm)", k, d.starts == startsBefore);
    }
    {
      // esp-mqtt's own auto-reconnect: the client is not recreated, so configure() does not run and
      // the change-detection baselines survive.
      Device d(k);
      d.brokerConnects();
      d.run(2000);
      d.brokerDrops();
      d.run(10);
      const uint32_t r0 = d.pub.rejectedMessages();
      const uint32_t e0 = d.pub.droppedTelemetry();
      d.brokerConnects();
      d.run(300);
      expectConnect("reconnect, baselines kept", k, d, d.connects.at(1));
      expectK("reconnect, baselines kept: nothing refused, no telemetry evicted", k,
              d.pub.rejectedMessages() == r0 && d.pub.droppedTelemetry() == e0);
      expectBounded("reconnect, baselines kept", k, d);
    }
    for (uint32_t offset : {1u, 2u, 3u, 5u, 8u, 12u}) {
      Device d(k);
      d.brokerConnects();
      d.run(offset);
      d.brokerDrops();
      d.run(10);
      const uint32_t r0 = d.pub.rejectedMessages();
      const uint32_t e0 = d.pub.droppedTelemetry();
      d.brokerConnects();
      d.run(300);
      const std::string route = "drop 1-12 passes into the set, then reconnect";
      expectConnect(route, k, d, d.connects.at(1));
      expectK(route + ": nothing refused, no telemetry evicted", k,
              d.pub.rejectedMessages() == r0 && d.pub.droppedTelemetry() == e0);
      expectBounded(route, k, d);
    }
    {
      // The first connect after boot, with esp-mqtt's CONNECTED landing between the latch read and the
      // connected block's gate: the transport says connected for a session this pass has not started.
      Device d(k);
      d.connectArrivesBeforeGate = true;
      d.run(300);
      expectConnect("a CONNECTED landing between the latch read and the gate", k, d, d.connects.at(0));
    }
    {
      // Three connects in a row that drop before their first pump(), each leaving its retained
      // `online` queued, then one that stays. The design study measured this pushing a telemetry-first
      // design's full set out to the 60 s heartbeat from the third such connect; the hold keeps the set
      // back until the leftovers have drained.
      Device d(k);
      d.brokerConnects();
      d.run(2000);
      for (int flap = 0; flap < 3; ++flap) {
        d.brokerDrops();
        d.run(5);
        d.brokerConnects();
        d.brokerDrops();  // both latched before the pass: it consumes them with the link down
        d.run(5);
      }
      const uint32_t r0 = d.pub.rejectedMessages();
      const uint32_t e0 = d.pub.droppedTelemetry();
      d.brokerConnects();
      d.run(300);
      const std::string route = "three connects dropped before their first pump, then one that stays";
      expectConnect(route, k, d, d.connects.back());
      expectK(route + ": nothing refused, no telemetry evicted", k,
              d.pub.rejectedMessages() == r0 && d.pub.droppedTelemetry() == e0);
    }
    {
      // A drop right after a forced full set, so the reconnect finds telemetry from the old session
      // queued ahead of everything: the worst leftover the hold's bound is derived for.
      Device d(k);
      d.brokerConnects();
      d.run(2000);
      d.pub.requestFullPublish();
      d.run(1);
      d.brokerDrops();
      d.run(5);
      d.brokerConnects();
      d.run(300);
      expectConnect("drop with a full set still queued, then reconnect", k, d, d.connects.at(1));
    }
  }
  flushChecks();
}

void republishRoutes() {
  std::printf("\n[DF30 — Home Assistant's birth, a retained birth, and cmd/republish]\n");
  for (int k = 0; k <= 8; ++k) {
    {
      Device d(k);
      d.brokerConnects();
      d.run(2000);
      const std::size_t first = d.sink.calls.size();
      const uint32_t r0 = d.pub.rejectedMessages();
      const uint32_t e0 = d.pub.droppedTelemetry();
      d.haBirth(false);
      d.run(300);
      expectK("HA birth: every config reaches the sink again", k, d.everyConfigSince(first, d.bitmap));
      expectK("HA birth: recorded, nothing refused, no telemetry evicted", k,
              d.recorded() && d.pub.rejectedMessages() == r0 && d.pub.droppedTelemetry() == e0);
      expectK("HA birth: one more set started", k, d.starts == 2);
      expectBounded("HA birth", k, d);
    }
    for (uint32_t roundTrip : {0u, 2u}) {
      // A broker holding a retained `homeassistant/status online` replays it at our subscribe. MQTT
      // 3.1.1 §3.3.1.3 sets the RETAIN flag on exactly that delivery. It arrives a round trip after the
      // subscribe, so a few passes into the connect's set; arriving in the connect pass itself, the set
      // has handed nothing over yet and would absorb it as a retarget either way.
      Device d(k);
      d.brokerConnects();
      d.run(roundTrip);
      d.haBirth(true);
      d.run(300);
      expectK("retained birth at the subscribe: one set, not two", k, d.starts == 1);
      expectK("retained birth at the subscribe: each config handed over once", k,
              d.discoveryCallsSince(0) == 3u * static_cast<std::size_t>(k) + 7u);
      expectConnect("retained birth at the subscribe", k, d, d.connects.at(0));
    }
    {
      Device d(k);
      d.brokerConnects();
      d.run(2000);
      const std::size_t first = d.sink.calls.size();
      const uint32_t r0 = d.pub.rejectedMessages();
      d.commandNextPass = true;
      d.run(300);
      expectK("cmd/republish: every config reaches the sink again, recorded, nothing refused", k,
              d.everyConfigSince(first, d.bitmap) && d.recorded() && d.pub.rejectedMessages() == r0);
      char diagnostics[MqttPublisher::kMaxTopicBytes] = {};
      d.pub.diagnosticsStateTopic(diagnostics, sizeof(diagnostics));
      std::size_t ack = std::string::npos;
      for (std::size_t i = first; i < d.sink.calls.size(); ++i) {
        if (d.sink.calls[i].topic == diagnostics &&
            d.sink.calls[i].payload.find("\"lastCmd\":\"accepted\"") != std::string::npos) {
          ack = i;
          break;
        }
      }
      // R4.4.2d / R4.4.4: the acknowledgement is the telemetry, so it cannot wait for a period.
      expectK("cmd/republish: the acknowledging diagnostics reach the sink within 4 passes", k,
              ack != std::string::npos && d.sink.calls[ack].pass < d.sink.calls[first].pass + kFullSetPasses);
      expectBounded("cmd/republish", k, d);
    }
    {
      // No rate limit on republish (mqtt_command_router.h) and a full set forced for every result, so
      // this is the heaviest legal request stream. Coalescing must let the set finish anyway.
      Device d(k);
      d.brokerConnects();
      d.run(500);
      const std::size_t first = d.sink.calls.size();
      const uint32_t r0 = d.pub.rejectedMessages();
      d.commandEveryPass = true;
      d.run(2000);
      const bool reachedDuringStorm = d.everyConfigSince(first, d.bitmap);
      d.commandEveryPass = false;
      d.run(500);
      expectK("cmd/republish every pass for 2000 passes: every config still reaches the sink during it", k,
              reachedDuringStorm);
      expectK("cmd/republish every pass for 2000 passes: ends recorded and idle, nothing refused", k,
              d.recorded() && d.jobIdle() && d.pub.rejectedMessages() == r0);
      expectBounded("cmd/republish every pass for 2000 passes", k, d);
    }
  }
  flushChecks();
}

void bitmapRoutes() {
  std::printf("\n[DF30 — the connected sensors change: one set per change, never a storm]\n");
  {
    // Commissioning one channel at a time while connected. Today this storms at the fourth channel.
    Device d(0);
    d.brokerConnects();
    d.run(500);
    for (int k = 1; k <= 8; ++k) {
      const uint32_t s0 = d.starts;
      const uint32_t a0 = d.sensorsChangedAnswers;
      const uint32_t r0 = d.pub.rejectedMessages();
      const std::size_t first = d.sink.calls.size();
      d.bitmap = bitmapOf(k);
      d.run(500);
      expectK("one channel at a time 0 -> 8: exactly one set and one R4.4.6 answer per change", k,
              d.starts - s0 == 1 && d.sensorsChangedAnswers - a0 == 1);
      expectK("one channel at a time 0 -> 8: every config of the new set reaches the sink", k,
              d.everyConfigSince(first, d.bitmap));
      expectK("one channel at a time 0 -> 8: recorded, nothing refused", k,
              d.recorded() && d.pub.rejectedMessages() == r0);
    }
    expectBounded("one channel at a time 0 -> 8", 8, d);
  }
  for (int k = 0; k <= 8; ++k) {
    // Every jump from 0 to k while connected, which covers 0 -> 4 and 0 -> 8; then 2 -> k across a
    // drop; then 8 -> k, channels going out of service.
    {
      Device d(0);
      d.brokerConnects();
      d.run(500);
      const uint32_t s0 = d.starts;
      const std::size_t first = d.sink.calls.size();
      d.bitmap = bitmapOf(k);
      d.run(500);
      const bool changed = d.bitmap != 0;
      expectK("jump 0 -> k while connected: one set for a change, none without", k,
              d.starts - s0 == (changed ? 1u : 0u));
      expectK("jump 0 -> k while connected: every config reaches the sink, recorded", k,
              d.everyConfigSince(changed ? first : 0, d.bitmap) && d.recorded());
      const uint32_t s1 = d.starts;
      const uint32_t a1 = d.sensorsChangedAnswers;
      d.run(5000);
      expectK("jump 0 -> k while connected: no storm in the 5 s after", k,
              d.starts == s1 && d.sensorsChangedAnswers == a1);
    }
    {
      Device d(2);
      d.brokerConnects();
      d.run(500);
      d.brokerDrops();
      d.bitmap = bitmapOf(k);
      d.run(10);
      d.brokerConnects();
      d.run(500);
      expectConnect("2 -> k while disconnected, then reconnect", k, d, d.connects.at(1));
      expectK("2 -> k while disconnected, then reconnect: two sets in all, the connects'", k,
              d.starts == 2);
      const uint32_t s1 = d.starts;
      d.run(5000);
      expectK("2 -> k while disconnected, then reconnect: no storm in the 5 s after", k, d.starts == s1);
    }
    {
      Device d(8);
      d.brokerConnects();
      d.run(500);
      const uint32_t s0 = d.starts;
      const std::size_t first = d.sink.calls.size();
      d.bitmap = bitmapOf(k);
      d.run(500);
      const bool changed = d.bitmap != 0xFF;
      expectK("8 -> k while connected: one set for a change, every config, recorded", k,
              d.starts - s0 == (changed ? 1u : 0u) && d.recorded() &&
                  d.everyConfigSince(changed ? first : 0, d.bitmap));
      const uint32_t s1 = d.starts;
      d.run(5000);
      expectK("8 -> k while connected: no storm in the 5 s after", k, d.starts == s1);
    }
    {
      // A change in the middle of a set: let it finish, then run once more for the latest bitmap.
      Device d(8);
      d.brokerConnects();
      d.run(3);
      d.bitmap = bitmapOf(k);
      d.run(1000);
      expectK("change 3 passes into the connect's set: every config of the new bitmap, recorded", k,
              d.everyConfigSince(0, d.bitmap) && d.recorded());
      expectK("change 3 passes into the connect's set: at most one extra set", k, d.starts <= 2);
    }
  }
  flushChecks();
}

void switchedOffAndTeardownRoutes() {
  std::printf("\n[DF30 — discovery switched off, and a teardown, mid-set]\n");
  for (int k = 0; k <= 8; ++k) {
    {
      // The firmware path: every settings apply bumps the revision and firmware.cpp tears the client
      // down. Fully published at 2 channels first, which is when today's policy stormed.
      Device d(2);
      d.brokerConnects();
      d.run(500);
      d.discoveryEnabled = false;
      d.teardown(kBaseA, kPrefixA);
      d.brokerConnects();
      d.run(3);
      const std::size_t first = d.sink.calls.size();
      const uint32_t a0 = d.sensorsChangedAnswers;
      d.bitmap = bitmapOf(k);
      d.run(1000);
      expectK("discovery switched off: no config is handed over while it is off", k,
              d.discoveryCallsSince(first) == 0);
      expectK("discovery switched off: a bitmap change is answered at most once, not every pass", k,
              d.sensorsChangedAnswers - a0 <= 1);
      d.discoveryEnabled = true;
      d.teardown(kBaseA, kPrefixA);
      d.brokerConnects();
      d.run(500);
      expectConnect("discovery switched back on", k, d, d.connects.back());
    }
#ifndef DF30_TODAY
    {
      // The in-place switch, which firmware.cpp never makes (it always tears down): the job cancels
      // and releases a connect's hold at once, so telemetry is not held for a set nobody will finish.
      Device d(k);
      d.brokerConnects();
      d.run(1);
      d.discoveryEnabled = false;
      d.run(1);
      expectK("switched off in place mid-set: the job cancels and the hold ends", k,
              !d.job.active() && !d.job.holdingTelemetry());
    }
#endif
    {
      // A settings change three passes into the set that moves BOTH the base topic and the prefix,
      // with a CONNECTED latched just before end() for good measure.
      Device d(k);
      d.brokerConnects();
      d.run(3);
      d.connectLatched = true;
      const std::size_t first = d.sink.calls.size();
      d.teardown(kBaseB, kPrefixB);
      d.run(5);
      d.brokerConnects();
      d.run(500);
      bool oldTopicAfter = false;
      for (std::size_t i = first; i < d.sink.calls.size(); ++i) {
        const std::string& t = d.sink.calls[i].topic;
        if (t.rfind(std::string(kBaseA) + "/", 0) == 0 || t.rfind(std::string(kPrefixA) + "/", 0) == 0) {
          oldTopicAfter = true;
        }
      }
      expectK("teardown mid-set: nothing under the old base topic or prefix reaches the sink after it", k,
              !oldTopicAfter);
      expectConnect("teardown mid-set, new base topic and prefix", k, d, d.connects.back());
    }
  }
  flushChecks();
}

void latchOrderRoutes() {
  std::printf("\n[DF30 — both link latches set in one pass, in each order they can really happen]\n");
  for (int k = 0; k <= 8; ++k) {
    {
      // Really disconnect then connect: a pass long enough for esp-mqtt's 10 s reconnect wait.
      Device d(k);
      d.brokerConnects();
      d.run(500);
      d.brokerDrops();
      d.brokerConnects();
      d.run(500);
      expectConnect("drop and reconnect inside one pass", k, d, d.connects.back());
      expectK("drop and reconnect inside one pass: the ladder ends connected", k,
              d.ladder.retryDelayMs() == 0 && d.ladder.attempts() == 0);
      const uint32_t s0 = d.starts;
      const std::size_t first = d.sink.calls.size();
      d.bitmap = static_cast<uint16_t>(bitmapOf(k) ^ 0x80u);
      d.run(500);
      expectK("drop and reconnect inside one pass: a later bitmap change still republishes (R4.4.6)", k,
              d.starts - s0 == 1 && d.everyConfigSince(first, d.bitmap));
    }
    {
      // Really connect then disconnect: the link is DOWN when the pass reaches the latches.
      Device d(k);
      d.brokerConnects();
      d.run(500);
      d.brokerDrops();
      d.run(10);
      d.brokerConnects();
      d.brokerDrops();
      const std::size_t first = d.sink.calls.size();
      d.run(200);
      expectK("connect then drop inside one pass: nothing reaches the sink while the link is down", k,
              d.sink.calls.size() == first);
      d.brokerConnects();
      d.run(500);
      expectConnect("connect then drop inside one pass, then a real reconnect", k, d, d.connects.back());
    }
  }
  flushChecks();
}

void incompleteRoutes() {
  std::printf("\n[DF30 — a set that does not complete is not recorded, and is retried once a minute]\n");
  for (int k = 0; k <= 8; ++k) {
    {
      // Truncation through the payload-limit seam: two configs do not fit.
      Device d(k, kTruncatingPayloadLimit);
      d.brokerConnects();
      d.run(300);
      const std::string boardTemperature = d.prefix + "/sensor/wfm_a1b2c3/board_temperature/config";
      const std::string pollingRate = d.prefix + "/sensor/wfm_a1b2c3/polling_rate/config";
      const std::set<std::string> got = d.configsSince(0);
      expectK("truncation: the two configs that do not fit are never handed over", k,
              got.count(boardTemperature) == 0 && got.count(pollingRate) == 0);
      expectK("truncation: every other config is", k,
              got.size() == 3u * static_cast<std::size_t>(k) + 7u - 2u);
      expectK("truncation: the set is not recorded as published", k, !d.policy.everPublished());
      const uint32_t s0 = d.starts;
      // Three minutes. A retry per pass would be 180,000 sets; a retry a minute is three.
      d.run(180000);
      expectK("truncation: retried once a minute, not every pass and not never", k, d.starts - s0 == 3);
      expectK("truncation: telemetry still flows and nothing is refused", k,
              d.pub.rejectedMessages() == 0 && d.fullSetSoonAfterRelease(d.connects.at(0)));
      expectBounded("truncation", k, d);
    }
    // The sink refuses one config once — esp-mqtt's -1 is heap exhaustion. pump() pops it regardless,
    // so only the Discovery-class failure counter knows. Board temperature, the entity DF29's bench
    // check reads, and the LAST entity of the set, which is still queued when the cursor reaches the
    // end: a verdict that did not wait for the queue to drain would record the set before it failed.
    for (const char* refused : {"/sensor/wfm_a1b2c3/board_temperature/config",
                                "/button/wfm_a1b2c3/cmd_republish/config"}) {
      Device d(k);
      d.sink.failTopic = d.prefix + refused;
      d.sink.failTimes = 1;
      d.brokerConnects();
      d.run(300);
      expectK("a refused config, mid-set or last: the set is not recorded", k, !d.policy.everPublished());
      d.run(59000);
      expectK("a refused config, mid-set or last: not retried before the minute is up", k, d.starts == 1);
      const std::size_t first = d.sink.calls.size();
      d.run(2000);
      expectK("a refused config, mid-set or last: retried after it, every config again, then recorded", k,
              d.starts == 2 && d.everyConfigSince(first, d.bitmap) && d.recorded());
    }
  }
  flushChecks();
}

#ifndef DF30_TODAY
void holdRoutes() {
  std::printf("\n[DF30 — the connect's hold on telemetry is bounded]\n");
  {
    // A stuck job: the device believes the link is up, so the job runs and holds, but pump() sees
    // the sink down and drains nothing, so the job can never queue its set.
    Device d(8);
    d.sink.stalled = true;
    d.brokerConnects();
    d.run(static_cast<uint32_t>(plc::HaDiscoveryJob::kConnectHoldPassLimit) + 5);
    check(d.holdLimitHits == 1, "a stuck job's hold is ended by the bound, once");
    check(d.longestHold == plc::HaDiscoveryJob::kConnectHoldPassLimit,
          "and telemetry was held for exactly kConnectHoldPassLimit passes, no more");
    check(d.connects.at(0).released, "and tick() runs again after it");
  }
  {
    // Every route above, again, for the one number they all share: the longest hold seen.
    std::size_t longest = 0;
    uint32_t hits = 0;
    for (int k = 0; k <= 8; ++k) {
      for (uint32_t offset : {1u, 2u, 3u, 5u, 8u, 12u}) {
        Device d(k);
        d.brokerConnects();
        d.run(2000);
        d.pub.requestFullPublish();
        d.run(1);
        d.brokerDrops();
        d.run(offset);
        d.brokerConnects();
        d.run(offset);
        d.brokerDrops();
        d.run(5);
        d.brokerConnects();
        d.run(300);
        longest = std::max(longest, d.longestHold);
        hits += d.holdLimitHits;
      }
    }
    std::printf("  (longest hold seen on a healthy connect: %zu passes; the bound is %zu)\n", longest,
                plc::HaDiscoveryJob::kConnectHoldPassLimit);
    check(hits == 0 && longest < plc::HaDiscoveryJob::kConnectHoldPassLimit,
          "no healthy connect, with leftovers from two drops queued, reaches the hold's bound");
  }
  {
    std::printf("  (admission window %zu = %zu slots - %zu telemetry - %zu availability)\n",
                plc::HaDiscoveryJob::kAdmissionWindow, MqttPublisher::kQueueCapacity,
                MqttPublisher::kFullTelemetrySetMessages, plc::HaDiscoveryJob::kAvailabilityPerConnect);
    check(plc::HaDiscoveryJob::kAdmissionWindow == 5,
          "the window is 16 - 10 - 1 = 5, with the duplicate availability message gone");
    check(plc::HaDiscoveryJob::kConnectHoldPassLimit == 13, "the hold's bound is 4 + 8 + 1 = 13 passes");
    check(plc::HaDiscoveryJob::kRetryIntervalMs == MqttPublisher::kHeartbeatMs,
          "the retry interval is R4.3.2's heartbeat");
  }
}
#endif

// ═══════════════════════════════════════════════════════════════════════════════════════
// firmware.cpp is wired to the job, read as text
// ═══════════════════════════════════════════════════════════════════════════════════════
//
// Everything above runs the header's entry points. These hold firmware.cpp to calling them, and to
// calling nothing that would bypass them — the gap DF30 lived in. Whole statements are compared with
// whitespace removed (firmware_source_text.h's second lesson), so a changed argument fails.

std::size_t countOf(const std::string& code, const char* needle) {
  return firmware_source::offsetsOf(code, needle).size();
}

std::size_t onlyOffsetOf(const std::string& code, const char* needle) {
  const std::vector<std::size_t> at = firmware_source::offsetsOf(code, needle);
  return at.size() == 1 ? at.front() : std::string::npos;
}

void firmwareWiringTests() {
  std::printf("\n[DF30 — firmware.cpp calls the job, and nothing that bypasses it]\n");
  using firmware_source::functionBody;
  using firmware_source::withoutCommentsOrStrings;
  using firmware_source::withoutWhitespace;
  const std::string raw = firmware_source::loadFirmwareSource();
  check(!raw.empty(), "src/firmware.cpp found and read");
  const std::string code = withoutCommentsOrStrings(raw);
  const std::string flat = withoutWhitespace(code);

  check(countOf(code, "publishDiscovery") == 0, "publishDiscovery() is gone");
  check(countOf(code, "mqttPublisher.enqueue(") == 0,
        "firmware.cpp enqueues nothing itself: no discovery burst, no second availability message");
  check(countOf(code, "mqttPublisher.tick(") == 0 && countOf(code, "mqttPublisher.pump(") == 0,
        "tick() and pump() are called only through serviceMqttConnectedPass");
  check(countOf(code, ".notePublished(") == 0 && countOf(code, ".onSensorBitmap(") == 0 &&
            countOf(code, "haRepublish.onConnected(") == 0 &&
            countOf(code, "haRepublish.noteDisconnected(") == 0,
        "the policy is driven only through the job's entry points");
  check(countOf(flat, "plc::HaDiscoveryJobhaDiscoveryJob;") == 1,
        "one job, with the default payload limit (the seam is for the host test only)");

  const std::size_t bitmapRead = onlyOffsetOf(flat, "constuint16_tsensorsBitmap=connectedSensorsBitmap;");
  check(bitmapRead != std::string::npos, "the connected sensors are read once into sensorsBitmap");

  const std::size_t readDisconnect = onlyOffsetOf(flat, "constbooldisconnectLatched=mqttDisconnectLatched;");
  const std::size_t clearDisconnect =
      onlyOffsetOf(flat, "if(disconnectLatched)mqttDisconnectLatched=false;");
  const std::size_t readConnect = onlyOffsetOf(flat, "constboolconnectLatched=mqttConnectLatched;");
  const std::size_t clearConnect = onlyOffsetOf(flat, "if(connectLatched)mqttConnectLatched=false;");
  check(readDisconnect != std::string::npos && clearDisconnect != std::string::npos &&
            readConnect != std::string::npos && clearConnect != std::string::npos &&
            readDisconnect < clearDisconnect && clearDisconnect < readConnect &&
            readConnect < clearConnect,
        "each link latch is read, then cleared only if it was set, the disconnect first");

  const std::size_t consume = onlyOffsetOf(
      flat,
      "constplc::MqttLinkLatchReportlink=plc::consumeMqttLinkLatches(disconnectLatched,connectLatched,"
      "netSettings.mqttHaDiscovery(),sensorsBitmap,haDiscovery,mqttReconnect,mqttPublisher,"
      "haRepublish,haDiscoveryJob);");
  check(consume != std::string::npos && bitmapRead < consume && clearConnect < consume,
        "consumeMqttLinkLatches is called once, with this pass's latches and bitmap");
  const std::size_t command = onlyOffsetOf(flat, "serviceMqttCommand(now);");
  check(consume != std::string::npos && command != std::string::npos && consume < command,
        "and before serviceMqttCommand, so a command arriving with a connect sees the connect");

  const std::string connectedBlock =
      functionBody(code, "if (mqttTransport.connected() && !mqttConnectLatched)");
  const std::string block = withoutWhitespace(connectedBlock);
  check(!block.empty(),
        "the connected block is found, once, and waits while a CONNECTED is latched but unconsumed");
  const std::string subscribeBlock =
      functionBody(code, "if (link.connected && mqttTransport.connected())");
  check(countOf(code, "mqttTransport.subscribe(") == 2 &&
            countOf(subscribeBlock, "mqttTransport.subscribe(") == 2,
        "both subscribes run only on a consumed connect whose link is still up");
  const std::size_t birthRead = block.find("constboolbirthLatched=haBirthLatched;");
  const std::size_t birthClear = block.find("if(birthLatched)haBirthLatched=false;");
  const std::size_t fill = block.find("plc::fillMqttSnapshot(");
  const std::size_t service = block.find(
      "constplc::MqttConnectedPassReportdiscoveryPass=plc::serviceMqttConnectedPass(haDiscoveryJob,"
      "haDiscovery,mqttPublisher,haRepublish,birthLatched,netSettings.mqttHaDiscovery(),"
      "mqttTransport.connected(),sensorsBitmap,now,snapshot);");
  check(countOf(code, "plc::serviceMqttConnectedPass(") == 1 && service != std::string::npos,
        "serviceMqttConnectedPass is called once, inside the connected block, with these arguments");
  check(birthRead != std::string::npos && birthClear != std::string::npos && birthRead < birthClear &&
            birthClear < service,
        "the birth latch is read and cleared in the block, before the service call");
  check(fill != std::string::npos && fill < service,
        "after the snapshot is filled, so the telemetry it ticks is this pass's");
  check(service != std::string::npos &&
            block.substr(service + std::strlen("constplc::MqttConnectedPassReportdiscoveryPass=")) ==
                "plc::serviceMqttConnectedPass(haDiscoveryJob,haDiscovery,mqttPublisher,haRepublish,"
                "birthLatched,netSettings.mqttHaDiscovery(),mqttTransport.connected(),sensorsBitmap,"
                "now,snapshot);reportDiscoveryPass(discoveryPass,netSettings.mqttHaDiscovery());",
        "and it is the block's last MQTT work: only the console report follows it");

  check(countOf(code, "mqttTransport.end()") == 1, "mqttTransport.end() is called in one place");
  const std::string stopBody = withoutWhitespace(functionBody(code, "void stopMqttClient()"));
  check(stopBody ==
            "mqttTransport.end();plc::endMqttSession(haDiscoveryJob,mqttPublisher,haRepublish);"
            "mqttConnectLatched=false;mqttDisconnectLatched=false;haBirthLatched=false;"
            "mqttCommandLatched=false;mqttClientUp=false;mqttReconnect.reset();",
        "stopMqttClient: end(), then the job, the queue and all four latches of that session go");
  check(countOf(code, "stopMqttClient();") == 2,
        "and both teardowns call it: MQTT switched off, and a settings change");
  check(countOf(flat, "haRepublish.onStatusMessage(haDiscovery,t,d,retained)") == 1,
        "onMqttData hands the retain flag to the birth test");
}

}  // namespace

int main() {
#ifdef DF30_TODAY
  std::printf("DF30 job test — against firmware.cpp at 65b36f0e, transcribed (expected to FAIL)\n");
#else
  std::printf("DF30 job test — net/ha_discovery_job.h, driven in the firmware's order\n");
#endif
  connectRoutes();
  republishRoutes();
  bitmapRoutes();
  switchedOffAndTeardownRoutes();
  latchOrderRoutes();
  incompleteRoutes();
#ifndef DF30_TODAY
  holdRoutes();
#endif
  firmwareWiringTests();

  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILED", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
