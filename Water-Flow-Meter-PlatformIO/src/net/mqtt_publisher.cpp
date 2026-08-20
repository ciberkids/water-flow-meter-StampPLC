#include "net/mqtt_publisher.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace plc {
namespace {

/** Lowercase hex of the last three MAC bytes — the "<mac-suffix>" §4.2 and §4.4.b both use. */
bool writeSuffix(const uint8_t mac[6], char* out, std::size_t size) {
  if (out == nullptr || size < 7) return false;
  std::snprintf(out, size, "%02x%02x%02x", mac[3], mac[4], mac[5]);
  return true;
}

// There is deliberately NO per-byte topic rule in this file. It used to hold `topicByteIsLegal`
// while ha_discovery.cpp held `topicCharsValid`, and the two disagreed. See `configure` below.

}  // namespace

const char* mqttClassName(MqttClass cls) {
  switch (cls) {
    case MqttClass::Availability: return "availability";
    case MqttClass::Discovery:    return "discovery";
    case MqttClass::Telemetry:    return "telemetry";
  }
  return "unknown";
}

bool mqttClassIsDroppable(MqttClass cls) {
  switch (cls) {
    // R4.1.3 names these two as never dropped, and the asymmetry is about what the loss costs.
    // A dropped reading is superseded by the next one within a publish period; a dropped discovery
    // message is an entity that never appears in Home Assistant, and a dropped availability
    // message leaves HA showing values from a device that is gone.
    case MqttClass::Availability:
    case MqttClass::Discovery:
      return false;
    case MqttClass::Telemetry:
      return true;
  }
  return false;
}

uint8_t mqttClassQos(MqttClass cls, uint8_t configuredQos) {
  switch (cls) {
    // Owner decision 8A, and it is the SAME asymmetry R4.1.3 draws for the queue rather than a
    // second unrelated rule. The queue refuses to evict these two because losing one is not
    // self-correcting; the wire refuses to send them best-effort for exactly that reason. Having
    // won the argument at the queue's door and then handed the message to a QoS-0 publish would
    // leave R4.1.3 protecting a message from us and not from the network.
    case MqttClass::Availability:
    case MqttClass::Discovery:
      // QoS 1 ALWAYS, regardless of the setting — `configuredQos` is deliberately not read on this
      // path, in either direction. Making it a floor (`max(configuredQos, 1)`) instead would be a
      // quieter version of the same defect: it would leave these two classes' QoS a function of a
      // setting that speaks only for telemetry, so a future change to that setting's range would
      // silently move discovery too.
      return kMqttReliableQos;

    // A reading lost in flight is superseded by the next one within one publishPeriod (R4.3.1), and
    // by R4.3.2's heartbeat within 60 s at the outside — so best-effort really is correct here, and
    // §6.1 lets the operator say so. This is the one class the setting speaks for.
    case MqttClass::Telemetry:
      return configuredQos;
  }
  return configuredQos;
}

bool mqttMacSuffix(const uint8_t mac[6], char* out, std::size_t size) {
  return writeSuffix(mac, out, size);
}

bool mqttClientId(const uint8_t mac[6], char* out, std::size_t size) {
  char suffix[7] = {};
  if (!writeSuffix(mac, suffix, sizeof(suffix))) return false;
  if (out == nullptr || size < 11) return false;
  std::snprintf(out, size, "wfm-%s", suffix);
  return true;
}

bool mqttDefaultBaseTopic(const uint8_t mac[6], char* out, std::size_t size) {
  char suffix[7] = {};
  if (!writeSuffix(mac, suffix, sizeof(suffix))) return false;
  if (out == nullptr || size < 18) return false;
  std::snprintf(out, size, "watermeter/%s", suffix);
  return true;
}

bool MqttPublisher::elapsed(uint32_t nowMs, uint32_t sinceMs, uint32_t intervalMs) {
  return static_cast<uint32_t>(nowMs - sinceMs) >= intervalMs;
}

bool MqttPublisher::configure(const char* baseTopic, uint16_t publishPeriodS, uint8_t qos) {
  // Clamp first: even a refused base topic should leave the cadence sane, because the operator's
  // next act is to fix the topic and they should not have to re-enter the period too.
  publishPeriodS_ = publishPeriodS < kMinPublishPeriodS
                        ? kMinPublishPeriodS
                        : (publishPeriodS > kMaxPublishPeriodS ? kMaxPublishPeriodS
                                                               : publishPeriodS);
  qos_ = qos > 2 ? 2 : qos;

  baseTopic_[0] = '\0';
  availabilityTopic_[0] = '\0';

  // ── Owner decision 5A: ONE base-topic rule, and this module defers to it ─────────────
  //
  // Everything this used to do itself is gone: the null check, the trailing-slash strip, the length
  // bound and the per-byte `topicByteIsLegal` scan. `NetSettings::isValidBaseTopic` decides, and
  // nothing is layered on top of it — two validators that agree today are two validators that can
  // disagree tomorrow, which is exactly what happened here.
  //
  // What the disagreement was, measured on the pre-change tree with `HaDiscovery::configure` given
  // the same string: this accepted a trailing `/` (and stripped it), a leading `/`, a space, and any
  // length up to 109 bytes; discovery refused all four. Discovery accepted 0x7f and every byte from
  // 0x80 up; this refused both. Neither direction says anything to the operator — §4.4.7's failure
  // mode is silence — and the first direction is the one that shipped: `watermeter/plant-3/` was
  // accepted here, published telemetry happily, and produced NO Home Assistant entities at all,
  // because discovery had refused the identical string. The MQTT state register still read connected.
  //
  // Three behaviour changes follow, and each replaces a silent outcome with a reportable one:
  //
  //  - a trailing `/` is now REFUSED rather than repaired. Repair was the defect: it made this
  //    module's accepted set differ from discovery's while looking helpful. A refusal reaches the
  //    operator — ui_settings.cpp fails the edit, portal_form.cpp reports Refused — and
  //    `NetSettings::stage()` has refused the same string at the field's own door since 5A landed,
  //    so nothing an operator could previously store becomes unusable here;
  //  - a leading `/`, a space, and an interior `//` are refused. Each leaves an empty topic level or
  //    a byte §4.6 cannot render on the one screen that could explain the fault;
  //  - the length cap is now the FIELD's 48 bytes rather than this buffer's 109. A base topic can
  //    only arrive from `NetField::MqttBaseTopic` (48) or `mqttDefaultBaseTopic` (17), so 49..109 was
  //    unreachable from either real source and was refused by discovery anyway.
  //
  // The interior `//` case is the one where BOTH modules tighten: both used to accept it.
  if (!NetSettings::isValidBaseTopic(baseTopic)) return false;

  // No length branch of its own: the validator caps this at netFieldCapacity(MqttBaseTopic), and the
  // static_assert in the header proves that cap plus "/diagnostics/state" fits kMaxTopicBytes. That
  // moved the old `length + 19 > kMaxTopicBytes` refusal from a branch that can no longer be taken
  // to a build failure that fires if either constant moves.
  const std::size_t length = std::strlen(baseTopic);
  std::memcpy(baseTopic_, baseTopic, length);
  baseTopic_[length] = '\0';

  // memcpy rather than snprintf: the bound above already proves this fits, but -Werror's
  // format-truncation analysis cannot see that through a member array, and silencing it with a
  // pragma would also silence a future real truncation here.
  static const char kStatusSuffix[] = "/status";
  std::memcpy(availabilityTopic_, baseTopic_, length);
  std::memcpy(availabilityTopic_ + length, kStatusSuffix, sizeof(kStatusSuffix));

  // A changed base topic invalidates every baseline: the old ones describe topics nobody is
  // subscribed to any more, so keeping them would suppress the first publish on the new topics.
  for (std::size_t slot = 0; slot < kSlotCount; ++slot) {
    baselineValid_[slot] = false;
    baseline_[slot][0] = '\0';
  }
  forceFull_ = true;
  everPublished_ = false;
  return true;
}

bool MqttPublisher::buildTopic(const char* suffix, std::size_t index, bool indexed, char* out,
                               std::size_t size) const {
  if (out == nullptr || size == 0) return false;
  out[0] = '\0';
  if (!configured()) return false;

  char scratch[kMaxTopicBytes * 2] = {};
  int written = 0;
  if (indexed) {
    // 1-based on the wire, matching §4.4.b's "<base>/sensor/1/state".
    written = std::snprintf(scratch, sizeof(scratch), "%s/sensor/%u/state", baseTopic_,
                            static_cast<unsigned>(index + 1));
  } else {
    written = std::snprintf(scratch, sizeof(scratch), "%s%s", baseTopic_, suffix);
  }
  if (written <= 0) return false;
  if (static_cast<std::size_t>(written) >= size) return false;  // refuse, never truncate
  std::memcpy(out, scratch, static_cast<std::size_t>(written) + 1);
  return true;
}

bool MqttPublisher::sensorStateTopic(std::size_t index, char* out, std::size_t size) const {
  if (index >= kNumSensors) {
    if (out != nullptr && size > 0) out[0] = '\0';
    return false;
  }
  return buildTopic(nullptr, index, true, out, size);
}

bool MqttPublisher::totalStateTopic(char* out, std::size_t size) const {
  return buildTopic("/total/state", 0, false, out, size);
}

bool MqttPublisher::diagnosticsStateTopic(char* out, std::size_t size) const {
  return buildTopic("/diagnostics/state", 0, false, out, size);
}

void MqttPublisher::jsonNumber(char* out, std::size_t size, double value, int decimals) {
  if (out == nullptr || size == 0) return;
  if (!std::isfinite(value)) {
    std::snprintf(out, size, "null");
    return;
  }
  std::snprintf(out, size, "%.*f", decimals, value);
}

void MqttPublisher::formatSensor(const MqttSensorTelemetry& s, char* out, std::size_t size) const {
  char flow[24] = {};
  char session[24] = {};
  char total[32] = {};
  char peak[24] = {};
  jsonNumber(flow, sizeof(flow), s.flowLPerMin, 3);
  jsonNumber(session, sizeof(session), s.sessionLiters, 2);
  jsonNumber(total, sizeof(total), s.totalCubicMeters, 6);
  jsonNumber(peak, sizeof(peak), s.maxFlowLPerMin, 3);
  // One payload per sensor, not per value (§4.2): eight publishes per cycle instead of forty, which
  // is radio airtime §2.1 cares about directly. HA reaches each field with a value_template.
  std::snprintf(out, size, "{\"flow\":%s,\"session\":%s,\"total\":%s,\"max\":%s,\"pulses\":%lu}",
                flow, session, total, peak, static_cast<unsigned long>(s.pulses));
}

void MqttPublisher::formatTotal(const MqttTotalTelemetry& t, char* out, std::size_t size) const {
  char flow[24] = {};
  char session[24] = {};
  char total[32] = {};
  jsonNumber(flow, sizeof(flow), t.flowLPerMin, 3);
  jsonNumber(session, sizeof(session), t.sessionLiters, 2);
  jsonNumber(total, sizeof(total), t.totalCubicMeters, 6);
  std::snprintf(out, size, "{\"flow\":%s,\"session\":%s,\"total\":%s,\"sensors\":%u}", flow, session,
                total, static_cast<unsigned>(t.activeSensors));
}

void MqttPublisher::formatDiagnostics(const MqttDiagnosticsTelemetry& d, char* out,
                                      std::size_t size) const {
  char rate[24] = {};
  char baseline[24] = {};
  char temperature[24] = {};
  jsonNumber(rate, sizeof(rate), d.pollingRateKhz, 3);
  jsonNumber(baseline, sizeof(baseline), d.baselineRateKhz, 3);
  jsonNumber(temperature, sizeof(temperature), d.boardTemperatureC, 1);
  // The R2.1.2 pair travels together on purpose: the live rate alone cannot show a regression, and
  // a regression that is only visible in a lab is the failure §2.1 is written to prevent.
  // `uncalibrated` sits beside `undersampling` because they are the same shape and the opposite
  // problem: one says a reading is wrong, the other says there is no reading to be wrong. A
  // subscriber seeing 0 flow needs both to tell which.
  std::snprintf(out, size,
                "{\"pollingRateKhz\":%s,\"baselineKhz\":%s,\"undersampling\":%u,"
                "\"uncalibrated\":%u,\"tempC\":%s,\"uptimeS\":%lu,\"rssi\":%d,"
                "\"lastCmd\":\"%s\"}",
                rate, baseline, static_cast<unsigned>(d.undersamplingFlags),
                static_cast<unsigned>(d.uncalibratedFlags), temperature,
                static_cast<unsigned long>(d.uptimeSeconds), static_cast<int>(d.wifiRssiDbm),
                mqttCommandResultText(d.lastCommandResult));
}

bool MqttPublisher::evictOldestTelemetry() {
  for (std::size_t position = 0; position < count_; ++position) {
    const std::size_t at = (head_ + position) % kQueueCapacity;
    if (!mqttClassIsDroppable(queue_[at].cls)) continue;

    // Compact the ring by moving every later message down one place. O(n) with n = 16 and only on
    // the overflow path, which is the trade taken to keep the queue a plain array of messages
    // rather than an array of indices into a pool.
    for (std::size_t shift = position; shift + 1 < count_; ++shift) {
      const std::size_t dst = (head_ + shift) % kQueueCapacity;
      const std::size_t src = (head_ + shift + 1) % kQueueCapacity;
      queue_[dst] = queue_[src];
    }
    --count_;
    ++droppedTelemetry_;
    return true;
  }
  return false;
}

bool MqttPublisher::enqueue(MqttClass cls, const char* topic, const char* payload, bool retain) {
  if (topic == nullptr || payload == nullptr) {
    ++rejectedMessages_;
    return false;
  }
  if (std::strlen(topic) == 0 || std::strlen(topic) >= kMaxTopicBytes ||
      std::strlen(payload) >= kMaxPayloadBytes) {
    ++rejectedMessages_;
    return false;
  }

  if (count_ == kQueueCapacity && !evictOldestTelemetry()) {
    // R4.1.3's hard edge: the queue is full of messages the policy forbids evicting. Refusing the
    // newcomer is the only remaining option, and it is counted loudly because reaching here means
    // something was lost that the requirement says must not be.
    ++rejectedMessages_;
    return false;
  }

  const std::size_t at = (head_ + count_) % kQueueCapacity;
  Message& slot = queue_[at];
  slot.cls = cls;
  slot.retain = retain;
  // Owner decision 8A: the class decides, not `qos_` and not the caller. This used to be a flat
  // `slot.qos = qos_`, which published discovery at whatever the operator had chosen for readings —
  // so §6.1's default of 0 meant a discovery message lost on a lossy link was an entity that never
  // appeared, with nothing logged (§4.4.7).
  slot.qos = qosFor(cls);
  std::snprintf(slot.topic, sizeof(slot.topic), "%s", topic);
  std::snprintf(slot.payload, sizeof(slot.payload), "%s", payload);
  ++count_;
  return true;
}

void MqttPublisher::onConnected() {
  if (!configured()) return;
  enqueue(MqttClass::Availability, availabilityTopic_, kOnlinePayload, true);
  // Arm rather than publish. A reconnect is when a subscriber's picture is emptiest, so the next
  // tick must send everything — including the values that did not happen to change while the link
  // was down, which change detection would otherwise suppress.
  forceFull_ = true;
  everPublished_ = false;
}

void MqttPublisher::onDisconnected() {
  // No "offline" publish: R4.5.1's will message is configured on the client, so the BROKER sends it.
  // That covers the case a self-published offline never could — power removed mid-reading — and
  // publishing it ourselves here would additionally race the LWT.
  forceFull_ = true;
  everPublished_ = false;
}

void MqttPublisher::offer(std::size_t slot, const char* topic, const char* payload, bool force) {
  const bool changed = !baselineValid_[slot] || std::strcmp(baseline_[slot], payload) != 0;
  if (!force && !changed) return;
  if (!enqueue(MqttClass::Telemetry, topic, payload, false)) return;

  // The baseline advances on a successful ENQUEUE, not on a successful publish. Publishing is
  // fire-and-forget from the caller's point of view (R4.1.1), and what heals a message the sink
  // later rejects is R4.3.2's heartbeat rather than a per-message retry this class would have to
  // own — so a lost sample costs at most one heartbeat of staleness.
  std::snprintf(baseline_[slot], kMaxTelemetryPayloadBytes, "%s", payload);
  baselineValid_[slot] = true;
}

void MqttPublisher::tick(uint32_t nowMs, const MqttSnapshot& snapshot) {
  if (!configured()) return;

  const bool heartbeat =
      !everPublished_ || forceFull_ || elapsed(nowMs, lastFullMs_, kHeartbeatMs);
  const bool rateLimitCleared =
      !everPublished_ ||
      elapsed(nowMs, lastPublishMs_, static_cast<uint32_t>(publishPeriodS_) * 1000u);

  // R4.3.2 outranks R4.3.1's rate limit, not the other way round: the heartbeat is a bound on how
  // stale a late subscriber's view may be, so a long publishPeriod cannot push it out. The visible
  // consequence is that a publishPeriod above 60 s stops being the effective cadence.
  if (!heartbeat && !rateLimitCleared) return;

  const std::size_t before = count_ + droppedTelemetry_ + rejectedMessages_;

  char topic[kMaxTopicBytes] = {};
  char payload[kMaxTelemetryPayloadBytes] = {};

  for (std::size_t i = 0; i < kNumSensors; ++i) {
    // An absent sensor gets no topic at all. Publishing zeros for a sensor that is not fitted would
    // create an HA entity reading 0 L/min, which is indistinguishable from a fitted sensor with no
    // flow — the same confusion §4.5 exists to prevent.
    if (!snapshot.sensors[i].present) continue;
    if (!sensorStateTopic(i, topic, sizeof(topic))) continue;
    formatSensor(snapshot.sensors[i], payload, sizeof(payload));
    offer(i, topic, payload, heartbeat);
  }

  if (totalStateTopic(topic, sizeof(topic))) {
    formatTotal(snapshot.total, payload, sizeof(payload));
    offer(kTotalSlot, topic, payload, heartbeat);
  }

  if (diagnosticsStateTopic(topic, sizeof(topic))) {
    formatDiagnostics(snapshot.diagnostics, payload, sizeof(payload));
    offer(kDiagnosticsSlot, topic, payload, heartbeat);
  }

  const bool didSomething = (count_ + droppedTelemetry_ + rejectedMessages_) != before;

  if (heartbeat) {
    lastFullMs_ = nowMs;
    forceFull_ = false;
  }
  if (heartbeat || didSomething) {
    // Only a tick that actually produced messages restarts the rate-limit window. A tick where
    // nothing changed must leave it alone, or a quiet device would push its own window forward and
    // the next real change would wait a further full period to be seen.
    lastPublishMs_ = nowMs;
    everPublished_ = true;
  }
}

std::size_t MqttPublisher::pump(std::size_t maxMessages) {
  if (!sink_.connected()) return 0;

  std::size_t sent = 0;
  std::size_t attempts = 0;
  while (count_ > 0 && attempts < maxMessages) {
    Message& message = queue_[head_];
    const int result = sink_.publish(message.topic, message.payload,
                                     static_cast<int>(message.qos), message.retain);

    // Success is >= 0. QoS 0 always yields message id 0, so testing for > 0 would count every
    // publish at the default QoS as a failure and R4.1.6's counter would be pure noise.
    if (result < 0) {
      ++publishFailures_;
      std::snprintf(lastFailureTopic_, sizeof(lastFailureTopic_), "%s", message.topic);
    } else {
      ++publishedMessages_;
      ++sent;
    }

    head_ = (head_ + 1) % kQueueCapacity;
    --count_;
    ++attempts;
  }
  return sent;
}

}  // namespace plc
