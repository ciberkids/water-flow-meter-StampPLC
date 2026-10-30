#pragma once

#include <cstddef>
#include <cstdint>

#include "net/ha_discovery.h"
#include "net/mqtt_publisher.h"
#include "net/mqtt_reconnect.h"

namespace plc {

/**
 * Home Assistant discovery as a job that runs across logic passes, and the MQTT pass it runs in
 * (`DF30`).
 *
 * ── What this replaced ─────────────────────────────────────────────────────────────────
 *
 * `publishDiscovery()` in firmware.cpp enqueued every config for the connected sensors in one loop:
 * 3 per channel in service plus 7, so 31 with all eight, into a publisher queue of 16 slots with no
 * pump in between. On a connect, two availability messages were already queued, so 14 fitted and
 * the rest were refused. The policy recorded a bitmap only when a whole set had been accepted, and
 * compared every pass against that recorded bitmap, so once a channel change had made the set too big
 * to fit, the whole set was rebuilt on EVERY connected pass and telemetry stopped until a reboot.
 * Commissioning one channel at a time while connected reached that state at the fourth channel.
 * None of it was visible to a host test, because it lived in firmware.cpp, which no host test links.
 *
 * ── What it is now ─────────────────────────────────────────────────────────────────────
 *
 *  - A JOB. It captures the connected-sensors bitmap once, when it starts, and keeps the index of
 *    the next entity. Each topic and payload is built at the moment it is enqueued, so nothing is
 *    stored but the cursor, and the bitmap the Modbus task can rewrite at any time is read once per
 *    pass by the caller rather than twice per set.
 *  - ADMITTED, one message at a time. Before every enqueue there must be a free slot, and the
 *    messages the queue never evicts (availability and discovery) must be fewer than
 *    `kAdmissionWindow`, which leaves a full telemetry set and a connect's availability message
 *    their room. `enqueue()`'s true cannot be the test: a full queue evicts telemetry and still
 *    returns true.
 *  - BOUNDED per pass, like `pump()`: at most `kMaxBuildsPerPass` builds, counted as attempts.
 *  - FIRST on a connect (owner decision B, 2026-10-30). The connect's telemetry set is held back
 *    until the job that connect started has queued its last entity, so every config is ahead of
 *    every state in the queue. The hold covers that one job only and ends after
 *    `kConnectHoldPassLimit` passes whatever happens.
 *  - RECORDED only when complete (owner decision A): every entity handed over, none of them still
 *    queued, and no Discovery-class failure since the set started. Handed to esp-mqtt, NOT
 *    delivered — see `step()`.
 *  - RETRIED on a timer when not complete, never on every pass.
 *  - COALESCED: requests that arrive while a set is being queued produce one more set, not one each.
 *
 * Arduino-free, so `test/host/ha_discovery_job_test.cpp` runs the firmware's own entry points —
 * `consumeMqttLinkLatches`, `serviceMqttConnectedPass`, `endMqttSession` — against the real
 * publisher, discovery builder and policy, and reads firmware.cpp as text to hold it to them.
 */

/** How a set started. Reported so the console can say why discovery is being published. */
enum class HaDiscoveryStart : uint8_t {
  None = 0,
  Connect,  /**< `consumeMqttLinkLatches`, on a CONNECTED: the only start that holds telemetry. */
  Request,  /**< A birth, `cmd/republish` or bitmap change that found no set being queued. */
  Rerun,    /**< The one extra set that requests arriving mid-set were coalesced into. */
  Retry     /**< `kRetryIntervalMs` after a set that did not complete. */
};

/** `ceil(a / b)`, for the job's derived bounds. Outside the class so it can be used in them. */
constexpr std::size_t haDiscoveryDivideRoundingUp(std::size_t a, std::size_t b) {
  return (a + b - 1) / b;
}

/** What one `step()` did, for firmware.cpp's console lines and for the host test. */
struct HaDiscoveryStepReport {
  /** Rerun or Retry when this step started a set; Connect and Request start outside `step()`. */
  HaDiscoveryStart started = HaDiscoveryStart::None;
  uint8_t built = 0;      /**< Topic and payload pairs built, each one an attempt. */
  uint8_t enqueued = 0;   /**< Of those, accepted by the publisher. */
  uint8_t truncated = 0;  /**< Of those, too long for their buffer: reported, never sent. */
  /** The first truncation in this step, so the console names what did not fit and by how much. */
  uint8_t truncatedIndex = 0;
  uint16_t truncatedTopicBytes = 0;
  uint16_t truncatedPayloadBytes = 0;
  /**
   * Refused by `enqueue()` despite the window. Unreachable today: the free-slot and window checks
   * before every enqueue, and the builder's own bounds on topic and payload, keep every config
   * inside what `enqueue()` accepts.
   *
   * It is NOT protected against. If a change made it reachable, `step()` would stop at that entity
   * and try the SAME one again next pass, and every pass after: the set never reaches its verdict, so
   * it never gets `kRetryIntervalMs`'s backoff either, and on a connect only `kConnectHoldPassLimit`
   * would release telemetry. What this count buys is that the spin is SEEN: firmware.cpp's console
   * report prints it on every pass it happens.
   */
  uint8_t refused = 0;
  bool allEnqueued = false;       /**< The cursor reached the end of its set in this step. */
  bool holdLimitReached = false;  /**< The connect's hold was ended by its bound, not by the job. */
  bool cancelled = false;         /**< Discovery is off or unconfigured, so the job was dropped. */
  bool finished = false;          /**< A set reached its verdict in this step. */
  bool complete = false;          /**< ... and was recorded with `notePublished`. */
  uint8_t announced = 0;          /**< Entities in the finished set. */
  uint8_t skipped = 0;            /**< Of which never sent, because they would have truncated. */
  uint32_t failures = 0;          /**< Discovery-class sink failures while the set ran. */
};

class HaDiscoveryJob {
 public:
  /**
   * Availability messages one connect queues: `MqttPublisher::onConnected()`'s retained `online`.
   *
   * It was two until `DF30`. firmware.cpp's connect latch enqueued a second one, byte for byte the
   * same message: the topic `availabilityTopic()`, the payload `kOnlinePayload`, retained, at
   * `qosFor(MqttClass::Availability)`. The second one cost a slot of sixteen and said nothing new.
   */
  static constexpr std::size_t kAvailabilityPerConnect = 1;

  /**
   * The most availability-plus-discovery messages the job lets the queue hold: 16 − 10 − 1 = 5.
   *
   * Derived from what must still fit beside them, so it moves with them: a full telemetry set,
   * which is what a reconnect, a heartbeat and a command's acknowledgement each queue at once, and the
   * availability message the NEXT connect queues before this job's leftovers have drained. With at
   * most this many reliable messages queued, a set plus that availability message is exactly the
   * capacity, so the set is never refused and never evicts one of its own members.
   *
   * Counted on the reliable messages only, not on everything queued: the design study measured the
   * alternative under `cmd/republish` on every pass and the job never finished, starved by telemetry.
   */
  static constexpr std::size_t kAdmissionWindow = MqttPublisher::kQueueCapacity -
                                                  MqttPublisher::kFullTelemetrySetMessages -
                                                  kAvailabilityPerConnect;
  static_assert(MqttPublisher::kFullTelemetrySetMessages + kAvailabilityPerConnect <
                    MqttPublisher::kQueueCapacity,
                "the admission window must leave discovery at least one slot, or no set can finish");

  /** Builds per pass, counted as attempts — a truncated entity costs one too — like `pump()`. */
  static constexpr std::size_t kMaxBuildsPerPass = MqttPublisher::kMessagesPerPass;

  /**
   * How long a set that did not complete waits before it is tried again: 60 s, R4.3.2's heartbeat.
   *
   * The bound this firmware already sets on how stale a subscriber's picture may be, applied to the
   * entities themselves. Not every pass: that was the storm. Not "the next reconnect" either: on a
   * stable link that can be days, and the policy no longer re-asks by itself (it compares against
   * the REQUESTED bitmap, see `HaRepublishPolicy`). A cause that does not go away — a config too long
   * for its buffer — costs one set a minute.
   */
  static constexpr uint32_t kRetryIntervalMs = MqttPublisher::kHeartbeatMs;

  /** What a healthy job queues per pass once the queue is clear: the least of its build cap, its
   *  window and what `pump()` frees. */
  static constexpr std::size_t kSteadyAdmissionsPerPass =
      kMaxBuildsPerPass < kAdmissionWindow
          ? (kMaxBuildsPerPass < MqttPublisher::kMessagesPerPass ? kMaxBuildsPerPass
                                                                 : MqttPublisher::kMessagesPerPass)
          : (kAdmissionWindow < MqttPublisher::kMessagesPerPass ? kAdmissionWindow
                                                                : MqttPublisher::kMessagesPerPass);

  /**
   * The most passes a connect may hold its telemetry back: 4 + 8 + 1 = 13.
   *
   * The longest a healthy connect job can take to queue its last entity, so the bound is never what
   * ends a working hold and always what ends a stuck one:
   *  - `ceil(kQueueCapacity / kMessagesPerPass)` = 4 passes for `pump()` to drain a queue left full
   *    by the previous session, during which nothing new can be admitted ahead of it;
   *  - `ceil(kHaMaxEntities / kSteadyAdmissionsPerPass)` = 8 passes to queue the largest set;
   *  - 1 for the connect pass itself.
   *
   * In passes, not milliseconds, because the job advances once per pass however long a pass takes:
   * a bound in time would release a slow pass's hold early and a fast pass's late. What 13 passes
   * are in milliseconds is not known: a pass has never been timed on hardware (`DF31` lists the
   * longest one among its board measurements), and the host test's 1 ms pass is the logic task's
   * floor, `vTaskDelay(1)`, not a measurement. The host test holds both halves of the claim: no route
   * reaches the bound, and a job stalled on purpose is released exactly at it.
   */
  static constexpr std::size_t kConnectHoldPassLimit =
      haDiscoveryDivideRoundingUp(MqttPublisher::kQueueCapacity, MqttPublisher::kMessagesPerPass) +
      haDiscoveryDivideRoundingUp(kHaMaxEntities, kSteadyAdmissionsPerPass) + 1;

  static_assert(kHaMaxEntities <= 255, "the cursor and the counts are uint8_t");
  static_assert(kConnectHoldPassLimit <= 255, "the hold's pass budget is a uint8_t");

  enum class Phase : uint8_t {
    Idle,             /**< Nothing to do. */
    Enqueuing,        /**< The cursor is walking the set. */
    AwaitingVerdict,  /**< All queued; waiting for the last of it to leave the publisher queue. */
    AwaitingRetry     /**< Not complete; waiting `kRetryIntervalMs` to start again. */
  };

  /**
   * `payloadLimitBytes` is the size handed to `discoveryPayload`, clamped to the stack buffer. The
   * firmware takes the default, which is that buffer. It exists for the host test, which has no
   * other way to make a payload truncate: R4.4.8's bound keeps every real one far inside the buffer,
   * which is the point of the bound, and a check of the truncation path needs one that does not fit.
   */
  explicit HaDiscoveryJob(std::size_t payloadLimitBytes = MqttPublisher::kMaxPayloadBytes)
      : payloadLimit_(static_cast<uint16_t>(payloadLimitBytes < MqttPublisher::kMaxPayloadBytes
                                                ? payloadLimitBytes
                                                : MqttPublisher::kMaxPayloadBytes)) {}

  /**
   * A CONNECTED: start again from the first entity, and hold the connect's telemetry until this set
   * is queued.
   *
   * Always from entity 0, even when a set was half-queued before the drop: esp-mqtt's outbox may
   * already have expired what was handed over, and a reconnect republishes every config anyway
   * (`HaRepublishPolicy::onConnected` always gives a reason). What the previous session left in the
   * publisher queue still goes out, first, which costs at most `kAdmissionWindow` duplicates.
   */
  void startForConnect(uint16_t bitmap, const MqttPublisher& publisher) {
    begin(bitmap, publisher);
    holding_ = true;
    holdPassesLeft_ = static_cast<uint8_t>(kConnectHoldPassLimit);
  }

  /**
   * A birth, `cmd/republish` or bitmap change. True when it started a set; false when it was folded
   * into the one being queued.
   *
   *  - Nothing being queued (idle, waiting for a verdict, or waiting to retry): start now. A set
   *    waiting for its verdict is superseded and never recorded; the new one covers it.
   *  - A set being queued that has handed nothing over yet: RETARGET it at `bitmap`. Nothing of the
   *    old target is out, so this is the same set, and the connect's hold, if any, is not extended.
   *  - A set part-way through: let it finish, then run ONE more, for the bitmap of the pass it
   *    finishes on. Restarting instead was measured in the design study: `cmd/republish` on every
   *    pass (it has no rate limit) and a bitmap toggling every few passes each kept the set from
   *    ever finishing. Any number of requests before it finishes produce that one extra set.
   */
  bool request(uint16_t bitmap, const MqttPublisher& publisher) {
    if (phase_ != Phase::Enqueuing) {
      begin(bitmap, publisher);
      return true;
    }
    if (next_ == 0) {
      bitmap_ = bitmap;
    } else {
      rerunPending_ = true;
    }
    return false;
  }

  /** Teardown, or discovery switched off: drop everything, including a pending retry and the hold. */
  void cancel() {
    phase_ = Phase::Idle;
    rerunPending_ = false;
    holding_ = false;
  }

  /**
   * One pass's work. The caller runs it BEFORE `tick()`, and then ticks only if
   * `holdingTelemetry()` is false — `serviceMqttConnectedPass` is that order.
   *
   * `canPublish` is discovery switched on AND configured; when false the job is cancelled. `linkUp`
   * false PAUSES it, and spends none of the hold's budget: a disconnect does not cancel a set, and the
   * reconnect's `startForConnect` begins it again from entity 0.
   *
   * `bitmap` is this pass's single read of the connected sensors. It is used only to start a rerun or
   * a retry; a set in progress keeps the bitmap it started with, so an index into it stays meaningful.
   *
   * WHAT "COMPLETE" MEANS (owner decision A). All three of:
   *  1. every entity of the set was accepted by the publisher — none skipped as too long;
   *  2. no Discovery-class message remains in the publisher queue, so every one was offered to the
   *     sink;
   *  3. `MqttPublisher::discoveryPublishFailures()` has not moved since the set started. `pump()` pops
   *     a message even when the sink returns -1, so that counter is the only evidence one was lost.
   * Only then `notePublished`. That means HANDED TO esp-mqtt, NOT DELIVERED. A non-negative return
   * from `esp_mqtt_client_enqueue` puts the message in esp-mqtt's outbox and no further: the outbox
   * expires every item 30 s after it was enqueued, silently (esp-mqtt bb9c8af lib/mqtt_outbox.c:193-207,
   * called from mqtt_client.c:1509, with no `MQTT_EVENT_DELETED` because REPORT_DELETED is not
   * configured), and `EspMqttTransport` discards `MQTT_EVENT_PUBLISHED`, the only delivery signal there
   * is. A set lost there is usually recovered by the next reconnect, birth or `cmd/republish` — not
   * where the cause repeats (`DF31`), which is the open item about what esp-mqtt does with messages
   * after the hand-over.
   *
   * NOT INLINED, on purpose, and the attribute on the definition below is load-bearing. This
   * function keeps an entity table and the two build buffers — `refs[kHaMaxEntities]`,
   * `topic[kMaxTopicBytes]` and `payload[kMaxPayloadBytes]`, about 1.3 KB — on the stack.
   * `publishDiscovery()`, which it replaced, was an ordinary out-of-line function, so those bytes were
   * on LogicTask's stack only while it ran. Defined `inline` in a header and called once, from
   * `serviceMqttConnectedPass`, which is itself called once from `logicTaskCode`, GCC folded both
   * into `logicTaskCode` at -Os, and the buffers became part of that function's own frame for the
   * WHOLE pass: measured with -fstack-usage, `logicTaskCode`'s frame went from 1,328 bytes at
   * 65b36f0e to 2,464, so every other deep chain of the pass — the UI renderer, the portal's POST
   * handler, WiFi — started about 1.1 KB deeper, in a 10,000-byte task whose high-water mark nobody
   * has measured. `noinline` puts them back in a frame of their own that exists only during the
   * call. A GCC attribute, which the host build's g++ accepts too; on the definition and not on this
   * declaration, because GCC warns (-Wattributes) when an `inline` definition follows a `noinline`
   * declaration.
   */
  HaDiscoveryStepReport step(bool canPublish, bool linkUp, uint32_t nowMs, uint16_t bitmap,
                             const HaDiscovery& discovery, MqttPublisher& publisher,
                             HaRepublishPolicy& policy);

  Phase phase() const { return phase_; }
  /** True while anything is outstanding, including a scheduled retry. */
  bool active() const { return phase_ != Phase::Idle; }
  bool holdingTelemetry() const { return holding_; }
  bool rerunPending() const { return rerunPending_; }
  uint16_t bitmap() const { return bitmap_; }
  std::size_t nextEntity() const { return next_; }

 private:
  void begin(uint16_t bitmap, const MqttPublisher& publisher) {
    phase_ = Phase::Enqueuing;
    bitmap_ = bitmap;
    next_ = 0;
    skipped_ = 0;
    count_ = 0;
    rerunPending_ = false;
    failuresAtStart_ = publisher.discoveryPublishFailures();
  }

  Phase phase_ = Phase::Idle;
  bool rerunPending_ = false;
  bool holding_ = false;
  uint8_t holdPassesLeft_ = 0;
  uint8_t next_ = 0;
  uint8_t skipped_ = 0;
  uint8_t count_ = 0;
  uint16_t bitmap_ = 0;
  uint16_t payloadLimit_ = 0;
  uint32_t failuresAtStart_ = 0;
  uint32_t retryFromMs_ = 0;
};

// noinline: see the declaration. Its buffers must not live in the caller's frame.
__attribute__((noinline)) inline HaDiscoveryStepReport HaDiscoveryJob::step(
    bool canPublish, bool linkUp, uint32_t nowMs, uint16_t bitmap, const HaDiscovery& discovery,
    MqttPublisher& publisher, HaRepublishPolicy& policy) {
  HaDiscoveryStepReport report;
  if (phase_ == Phase::Idle) return report;
  if (!canPublish) {
    cancel();
    report.cancelled = true;
    return report;
  }
  if (!linkUp) return report;

  if (holding_) {
    if (holdPassesLeft_ == 0) {
      // A healthy connect never gets here (see kConnectHoldPassLimit). Reaching it means the job is
      // stuck, and a stuck job must not keep telemetry off the wire.
      holding_ = false;
      report.holdLimitReached = true;
    } else {
      --holdPassesLeft_;
    }
  }

  if (phase_ == Phase::AwaitingRetry) {
    // Wrap-safe: millis() wraps every 49.7 days and the interval is a minute.
    if (static_cast<uint32_t>(nowMs - retryFromMs_) < kRetryIntervalMs) return report;
    begin(bitmap, publisher);
    policy.noteRequested(bitmap);
    report.started = HaDiscoveryStart::Retry;
  }

  if (phase_ == Phase::Enqueuing) {
    HaEntityRef refs[kHaMaxEntities] = {};
    const std::size_t count = discovery.enumerateEntities(bitmap_, refs, kHaMaxEntities);
    count_ = static_cast<uint8_t>(count);
    while (next_ < count && report.built < kMaxBuildsPerPass &&
           publisher.queued() < MqttPublisher::kQueueCapacity &&
           publisher.queuedNeverEvicted() < kAdmissionWindow) {
      // The same two buffers publishDiscovery() kept on the logic task's stack, built only for the
      // entity about to be queued.
      char topic[MqttPublisher::kMaxTopicBytes] = {};
      char payload[MqttPublisher::kMaxPayloadBytes] = {};
      const HaBuildResult t = discovery.discoveryTopic(refs[next_], topic, sizeof(topic));
      const HaBuildResult p = discovery.discoveryPayload(refs[next_], payload, payloadLimit_);
      ++report.built;
      if (!t.complete || !p.complete) {
        // Truncation is REPORTED, never published. A truncated topic publishes successfully to the
        // wrong place and a truncated payload is malformed JSON — either way HA shows nothing and the
        // broker reports success (§4.4.7). R4.4.8's host test exists to make this unreachable; this
        // is the belt to its braces, and the set that skipped one is not recorded.
        if (report.truncated == 0) {
          report.truncatedIndex = next_;
          report.truncatedTopicBytes = static_cast<uint16_t>(t.required);
          report.truncatedPayloadBytes = static_cast<uint16_t>(p.required);
        }
        ++report.truncated;
        ++skipped_;
        ++next_;
        continue;
      }
      // Retained (R4.4.5), and MqttClass::Discovery so R4.1.3 never evicts it.
      if (!publisher.enqueue(MqttClass::Discovery, topic, payload, true)) {
        ++report.refused;
        break;  // the same entity again next pass
      }
      ++next_;
      ++report.enqueued;
    }
    if (next_ < count) return report;

    report.allEnqueued = true;
    // The connect's hold ends HERE: every config of the set is now ahead of the telemetry the next
    // tick() queues, in a queue that is first in, first out. A rerun does not extend it.
    holding_ = false;
    if (rerunPending_) {
      // The set just queued is superseded, so it is never judged; the rerun's verdict covers both.
      begin(bitmap, publisher);
      policy.noteRequested(bitmap);
      report.started = HaDiscoveryStart::Rerun;
      return report;
    }
    phase_ = Phase::AwaitingVerdict;
  }

  if (phase_ == Phase::AwaitingVerdict) {
    if (publisher.queuedOfClass(MqttClass::Discovery) > 0) return report;
    const uint32_t failures = publisher.discoveryPublishFailures() - failuresAtStart_;
    const bool complete = skipped_ == 0 && failures == 0 && count_ > 0;
    report.finished = true;
    report.complete = complete;
    report.announced = count_;
    report.skipped = skipped_;
    report.failures = failures;
    if (complete) {
      policy.notePublished(bitmap_);
      phase_ = Phase::Idle;
    } else {
      phase_ = Phase::AwaitingRetry;
      retryFromMs_ = nowMs;
    }
  }
  return report;
}

// ── The firmware's three call sites, here so a host test runs them in the firmware's order ──────

/** What consuming the two link latches did. */
struct MqttLinkLatchReport {
  bool disconnected = false;
  bool connected = false;
  /**
   * The CONNECTED was consumed while the transport still said connected, so this pass STARTED a
   * session: firmware.cpp subscribes on it, and `sessionStarted` was set. False for a CONNECTED
   * consumed after its link had already dropped again.
   */
  bool sessionStarted = false;
  HaRepublishReason connectReason = HaRepublishReason::None;
  bool discoveryStarted = false;
};

/**
 * The MQTT link's two latches, consumed in ONE order: the DISCONNECTED first, then the CONNECTED.
 *
 * Both can be set when the logic task reaches them, after a pass long enough for esp-mqtt to drop and
 * reconnect in between (its reconnect wait is 10 s). Which happened first is not recorded, so one
 * order has to be chosen, and only this one leaves a working session correct:
 *  - really disconnect then connect, the usual case: the session ends up connected, as it is. The
 *    order this replaced, connect first, left the ladder and the policy believing the link was down
 *    while it was up, so a later change of the connected sensors went unpublished for the rest of
 *    the session;
 *  - really connect then disconnect: the policy, the publisher and the ladder end up believing a
 *    down link is up. The policy and the job are harmless, because everything that publishes runs
 *    in the connected block, which firmware.cpp gates on the transport's own `connected()` AND on
 *    `sessionStarted`, which this consume leaves false. The report's `connected` is true here too,
 *    which is why firmware.cpp subscribes on `sessionStarted` rather than on `connected`: a
 *    subscribe takes esp-mqtt's lock before it checks the state, and a client that is reconnecting
 *    holds that lock through its whole connect attempt. The ladder makes no attempt while it
 *    believes it is connected, and that is harmless ONLY because esp-mqtt's own auto-reconnect is
 *    on: `EspMqttTransport` never sets `disable_auto_reconnect`, although `mqtt_reconnect.h`'s
 *    contract (owner decision 3B) says it should. The library's next attempt latches a fresh event
 *    either way. If 3B is ever honoured, this case needs revisiting.
 *
 * ── `sessionStarted`, the flag the connected block's gate needs (`DF30`'s review) ──────────
 *
 * Owned by the logic task: esp-mqtt's task never writes it, so it has none of the latches' races.
 * Cleared here when a DISCONNECTED is consumed, BEFORE the connect below is handled, and set when a
 * CONNECTED is consumed with `transportConnected` true; firmware.cpp's `stopMqttClient()` clears it
 * too. See `mqttConnectedPassMayRun` for why the gate cannot do without it.
 *
 * `transportConnected` must be read AFTER the connect latch was read and cleared. Read before, a
 * CONNECTED that lands in between is consumed against a stale `false`: no session starts, and on a
 * link that then stays up no later CONNECTED ever arrives to start one, so the connected block would
 * stay shut for good. Read after, a `false` means the link dropped again after that CONNECTED, and
 * the drop's own DISCONNECTED, and the next connect's CONNECTED, are still to come.
 *
 * The request is recorded on every connect, discovery on or off (see `noteRequested`).
 */
inline MqttLinkLatchReport consumeMqttLinkLatches(bool disconnectLatched, bool connectLatched,
                                                  bool transportConnected, bool discoveryEnabled,
                                                  uint16_t sensorsBitmap,
                                                  const HaDiscovery& discovery,
                                                  MqttReconnect& reconnect,
                                                  MqttPublisher& publisher,
                                                  HaRepublishPolicy& policy, HaDiscoveryJob& job,
                                                  bool& sessionStarted) {
  MqttLinkLatchReport report;
  if (disconnectLatched) {
    report.disconnected = true;
    // First, so the connect below decides the flag for the session that follows.
    sessionStarted = false;
    reconnect.noteDisconnected();
    publisher.onDisconnected();
    policy.noteDisconnected();
    // The job is NOT cancelled: step() pauses while the link is down, and the connect below, or the
    // next one, starts it again from entity 0.
  }
  if (connectLatched) {
    report.connected = true;
    if (transportConnected) {
      sessionStarted = true;
      report.sessionStarted = true;
    }
    reconnect.noteConnected();
    // R4.5.1's retained `online`, and the full telemetry set armed for the first tick that is not
    // held. The only availability message a connect queues (kAvailabilityPerConnect).
    publisher.onConnected();
    report.connectReason = policy.onConnected();
    policy.noteRequested(sensorsBitmap);
    if (discoveryEnabled && discovery.configured()) {
      job.startForConnect(sensorsBitmap, publisher);
      report.discoveryStarted = true;
    } else {
      job.cancel();
    }
  }
  return report;
}

/**
 * The connected block's gate: the transport says connected, this task has started that session,
 * and no CONNECTED is latched but unconsumed.
 *
 * WHY THE FLAG. `EspMqttTransport::onEvent` stores the transport's `connected_` and THEN calls the
 * listener that latches the CONNECTED, two stores from esp-mqtt's task with instructions and a call
 * between them; the logic task can read both in that window. A gate on `connected()` and the latch
 * alone then opens for a CONNECTED this pass has not consumed, and `tick()` hands the full set that
 * `configure()` or the last disconnect armed to esp-mqtt AHEAD of the retained `online` and of every
 * config: owner decision B broken on that connect, and on the first connect after boot Home Assistant
 * has no configs at all. Swapping the two stores in `onEvent` would close that window and open a worse
 * one: a latch seen before `connected_` makes the consume skip the subscribes, on a link that then
 * stays up with no later CONNECTED to subscribe it. So the gate also asks for `sessionStarted`, which
 * only this task sets, and only when it CONSUMES a CONNECTED — so never while that CONNECTED is still
 * half-stored.
 *
 * THE ORDER OF THE THREE READS DECIDES NOTHING, and must not be relied on: arguments are evaluated
 * in an unspecified order, and with only `connected()` and the latch, neither order closes the
 * window. The flag is what closes it, and it is not shared with esp-mqtt's task.
 *
 * WHAT IT DOES NOT CLOSE. The flag is the PREVIOUS session's until a DISCONNECTED is consumed. If a
 * drop and esp-mqtt's reconnect both land after this pass consumed the latches and before the gate —
 * the reconnect waits 10 s, so only on a pass that long — and the gate reads in the instructions
 * between the reconnect's `connected_` store and its latch, the stale flag opens it for one pass.
 * The DISCONNECTED latched by that drop is set by then; adding `!disconnectLatched` would close it
 * too, and is not done here.
 */
inline bool mqttConnectedPassMayRun(bool transportConnected, bool sessionStarted,
                                    bool connectLatched) {
  return transportConnected && sessionStarted && !connectLatched;
}

/** What one connected pass did. */
struct MqttConnectedPassReport {
  bool birthHandled = false;    /**< A birth or an accepted `cmd/republish` was consumed. */
  bool sensorsChanged = false;  /**< The connected sensors changed: answered once per change. */
  bool requestStarted = false;  /**< ... and one of them started a set rather than being folded in. */
  HaDiscoveryStepReport step;
  bool telemetryHeld = false;   /**< `tick()` was skipped: the connect's discovery is still queuing. */
};

/**
 * Everything the connected block does with MQTT, in the order that makes owner decision B true.
 *
 *  1. The two republish triggers, each recorded as a request even with discovery off.
 *  2. One step of the job — BEFORE `tick()`, so a slice queued this pass is ahead of this pass's
 *     telemetry and so the job can always find a free slot after the previous `pump()`.
 *  3. `tick()`, unless the connect's hold is on. The full set `onConnected()` armed stays armed while
 *     it is held, so the first tick after the hold sends it, behind every config.
 *  4. `pump(kMessagesPerPass)`.
 *
 * `birthLatched` is passed by value: firmware.cpp's latch is a `volatile bool` written by esp-mqtt's
 * task, read and cleared there before this call.
 */
inline MqttConnectedPassReport serviceMqttConnectedPass(
    HaDiscoveryJob& job, const HaDiscovery& discovery, MqttPublisher& publisher,
    HaRepublishPolicy& policy, bool birthLatched, bool discoveryEnabled, bool linkUp,
    uint16_t sensorsBitmap, uint32_t nowMs, const MqttSnapshot& snapshot) {
  MqttConnectedPassReport report;
  const bool canPublish = discoveryEnabled && discovery.configured();
  if (birthLatched) {
    report.birthHandled = true;
    policy.noteRequested(sensorsBitmap);
    if (canPublish && job.request(sensorsBitmap, publisher)) report.requestStarted = true;
  }
  // A change of the connected sensors, answered once per change: the request is recorded before the
  // next pass compares again.
  if (policy.onSensorBitmap(sensorsBitmap) != HaRepublishReason::None) {
    report.sensorsChanged = true;
    policy.noteRequested(sensorsBitmap);
    if (canPublish && job.request(sensorsBitmap, publisher)) report.requestStarted = true;
  }
  report.step =
      job.step(canPublish, linkUp, nowMs, sensorsBitmap, discovery, publisher, policy);
  report.telemetryHeld = job.holdingTelemetry();
  if (!report.telemetryHeld) {
    // Telemetry. tick() owns the cadence — publish-on-change rate-limited to publishPeriod, plus
    // §4.3.2's full set at least every 60 s.
    publisher.tick(nowMs, snapshot);
  }
  // Bounded per pass; see kMessagesPerPass.
  publisher.pump(MqttPublisher::kMessagesPerPass);
  return report;
}

/**
 * A client TEARDOWN — MQTT switched off, or a settings change that recreates the client.
 *
 * Everything queued was built for the client being stopped, under its base topic and discovery
 * prefix, so it goes: `configure()` keeps the queue, and the new client would otherwise publish
 * retained availability and discovery under topics nothing will ever clear. The job goes with it;
 * the next connect starts a new one. The policy and publisher are told here because nothing else
 * reliably tells them: the session is over whether or not esp-mqtt dispatched an event for it.
 *
 * esp-mqtt CAN still deliver a CONNECTED or a DISCONNECTED WHILE `end()` runs, never after it
 * returns (esp-mqtt bb9c8af). `esp_mqtt_client_stop` waits for the client's lock
 * (mqtt_client.c:1656), which esp-mqtt's task holds for a whole loop body (:1455), and a connect
 * attempt in progress finishes under it: it dispatches CONNECTED, or fails and dispatches
 * DISCONNECTED (:1472-1490). And a last iteration whose poll fails aborts the connection, dispatching
 * DISCONNECTED (:1563-1567), after stop() has cleared the run flag. Only once the loop has exited does the task set the bit
 * stop() waits for (:1571-1573), so nothing is dispatched after `end()` returns. That is why
 * firmware.cpp's `stopMqttClient()` clears the latches AFTER `end()` and this call, and why that
 * order is load-bearing: a latch set during `end()` is cleared, rather than consumed next pass
 * against a publisher not yet reconfigured.
 */
inline void endMqttSession(HaDiscoveryJob& job, MqttPublisher& publisher, HaRepublishPolicy& policy) {
  job.cancel();
  publisher.discardQueue();
  publisher.onDisconnected();
  policy.noteDisconnected();
}

}  // namespace plc
