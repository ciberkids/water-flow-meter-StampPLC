#pragma once

#include <cstdint>

namespace plc {

/**
 * WHEN to ask the network for the time — and nothing about how (R7.13).
 *
 * The clock's third route, after the Modbus block at 50-52 and the portal page (R7.9d). Split from the
 * asking for the same reason `PortalClockWriter` is split from `PortalForm`: SNTP needs Arduino, the
 * decisions need testing, and mixing them means the decisions get tested by hand on a bench or not at all.
 *
 * ── WHY A POLICY AND NOT AN `if` AT THE CALL SITE ────────────────────────────────────────
 *
 * "Sync on connect" sounds like one line. It is four decisions:
 *
 *  1. **Only while associated.** SNTP against a powered-down radio spends the retry ladder on a dead
 *     interface — the mistake `mqtt_reconnect.h` documents for MQTT, in the same loop, for the same reason.
 *  2. **Once per association, not once per pass.** The logic task runs this hundreds of times a second.
 *  3. **Retry a failure, but not immediately and not forever-often.** A device on a LAN with no route to
 *     the internet must not spend its life on DNS lookups that will not answer.
 *  4. **Re-sync eventually.** The RX8130CE drifts, and a device that syncs once at commissioning and never
 *     again is a device whose timestamps quietly diverge for years.
 *
 * ── WHAT IT DELIBERATELY DOES NOT DECIDE ─────────────────────────────────────────────────
 *
 * Whether the answer is plausible. `DeviceClock::setTime` owns that floor (2020…2100) and refuses without
 * changing anything; a second copy of that rule here is the "one fact, two homes" failure this codebase
 * keeps finding. Nor does it decide whether NTP outranks an operator: it reports success and the clock
 * records the new source, because a network time is strictly better than a typed one and the panel says
 * which it has.
 */
class NtpPolicy {
 public:
  /**
   * First attempt after association, then the gap between one success and the next request.
   *
   * Six hours: the RX8130CE's datasheet drift is ±3 ppm at room temperature, about 0.8 s/day, so six hours
   * bounds the accumulated error to well under a second while asking a public pool four times a day rather
   * than continuously. A device that cannot reach a server never gets this far — see `kRetryMs`.
   */
  static constexpr uint32_t kResyncIntervalMs = 6u * 60u * 60u * 1000u;

  /**
   * Gap after a failed attempt.
   *
   * Two minutes, not the seconds an association retry uses: a failure here means DNS or the pool did not
   * answer, which is a condition that resolves on its own timescale or not at all. The device has a clock
   * it can already report as unset; hammering the network to improve that is the wrong trade.
   */
  static constexpr uint32_t kRetryMs = 2u * 60u * 1000u;

  /**
   * How long an unanswered request stays in flight before it counts as failed.
   *
   * SNTP is asynchronous: `configTime` returns immediately and the answer arrives on the network stack's
   * own schedule, so somebody has to decide when silence means failure. It is here rather than in the
   * adapter because it is a timing rule, and timing rules in an Arduino file are timing rules nothing tests.
   *
   * Fifteen seconds covers a DNS lookup plus a round trip on a slow link, and is short enough that a device
   * with no route to the internet reaches its two-minute retry cadence promptly instead of sitting in flight
   * forever — which would also mean `noteSucceeded` never runs and the resync never schedules.
   */
  static constexpr uint32_t kRequestTimeoutMs = 15u * 1000u;

  /** The WiFi association edge — `WifiManager::consumeJustConnected()`. */
  void noteAssociated(uint32_t nowMs) {
    associated_ = true;
    // Ask immediately on a fresh association: this is the moment a device that booted with a dead RTC
    // first has any chance of a real time.
    dueAtMs_ = nowMs;
    due_ = true;
  }

  /** Association lost. Any pending request is abandoned rather than answered against a dead radio. */
  void noteDisassociated() {
    associated_ = false;
    due_ = false;
    inFlight_ = false;
  }

  /**
   * True once, when a request should be issued now.
   *
   * Consuming rather than peeking, so the caller cannot issue two requests for one decision — the same
   * shape as `consumeJustConnected()`.
   */
  bool consumeDueRequest(uint32_t nowMs) {
    if (!associated_ || inFlight_ || !due_) {
      return false;
    }
    // Wrap-safe: `millis()` rolls every 49.7 days and this device is meant to run for years.
    if (static_cast<int32_t>(nowMs - dueAtMs_) < 0) {
      return false;
    }
    due_ = false;
    inFlight_ = true;
    requestedAtMs_ = nowMs;
    return true;
  }

  /**
   * True once, when an in-flight request has been silent too long.
   *
   * Consuming, and it fails the request itself rather than expecting the caller to follow up with
   * `noteFailed`: two calls to express one decision is how a caller ends up expressing half of it.
   */
  bool consumeTimeout(uint32_t nowMs) {
    if (!inFlight_) {
      return false;
    }
    if (static_cast<int32_t>(nowMs - (requestedAtMs_ + kRequestTimeoutMs)) < 0) {
      return false;
    }
    noteFailed(nowMs);
    return true;
  }

  /** The request answered with a time the clock accepted. */
  void noteSucceeded(uint32_t nowMs) {
    inFlight_ = false;
    succeededAtMs_ = nowMs;
    everSucceeded_ = true;
    dueAtMs_ = nowMs + kResyncIntervalMs;
    due_ = true;
  }

  /** The request did not answer, or answered something the clock refused. */
  void noteFailed(uint32_t nowMs) {
    inFlight_ = false;
    dueAtMs_ = nowMs + kRetryMs;
    due_ = true;
  }

  bool everSucceeded() const { return everSucceeded_; }
  bool requestInFlight() const { return inFlight_; }
  uint32_t lastSuccessMs() const { return succeededAtMs_; }

 private:
  bool associated_ = false;
  bool due_ = false;
  bool inFlight_ = false;
  bool everSucceeded_ = false;
  uint32_t dueAtMs_ = 0;
  uint32_t requestedAtMs_ = 0;
  uint32_t succeededAtMs_ = 0;
};

}  // namespace plc
