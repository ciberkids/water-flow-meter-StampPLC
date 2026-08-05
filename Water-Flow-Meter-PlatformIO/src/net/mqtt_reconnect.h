#pragma once

#include <cstddef>
#include <cstdint>

namespace plc {

/**
 * The MQTT reconnection policy (WiFi_MQTT_Connectivity R4.1.2 — "reconnection uses the same
 * backoff policy as §3.1.2").
 *
 * ── Why this class exists at all ─────────────────────────────────────────────────
 *
 * §4.1.1 of the requirement claims the §3.1.2 ladder comes "for free" from esp-mqtt's
 * `reconnect_timeout_ms`. It does not. The installed header
 * (`tools/sdk/esp32s3/include/mqtt/esp-mqtt/include/mqtt_client.h`) describes that field as
 * "reconnect to the broker after this value in milliseconds if auto reconnect is not disabled
 * (defaults to 10s)" — a FIXED interval. A fixed 10 s retry does satisfy the part of R4.1.2 that
 * matters most (no tight-loop storm against an unreachable broker), but it is NOT the exponential
 * 1 s → 5 min ramp with jitter that §3.1.2 specifies, so R4.1.2 was unimplemented for as long as
 * the transport relied on it. This class is that ramp, and the transport must therefore set
 * `disable_auto_reconnect = true` (§4.1's code block, owner decision 3B) — otherwise the library's
 * fixed retry races this policy and wins, and the ladder becomes a number we print rather than a
 * rate the broker observes. That is the same hazard `WifiManager::failBackoff()` calls
 * `radio_.disconnect()` to close.
 *
 * ── "Never connected" is a rung, not a resting place ─────────────────────────────
 *
 * An earlier version of this class armed the ladder from a CONNECTED → DISCONNECTED *edge*, and
 * sat in an `Idle` phase until one arrived. That contract could not survive the one case R4.1.2 is
 * actually written for. A broker that is unreachable from boot never produces that edge — the
 * transport's `connected_` flag starts false and stays false — so the ladder never armed; and
 * because `disable_auto_reconnect = true` had already switched the library's own retry off,
 * NOTHING retried. MQTT would have been permanently down on precisely the devices the requirement
 * exists for, with the ladder reporting that it was at rest.
 *
 * So the policy now owns every attempt, including the first:
 *
 *   - `Phase::Cold` — the state a fresh policy and a `reset()` policy are in — asks for its
 *     attempt on the FIRST `update()`, with NO delay. A device booting next to a working broker
 *     connects as fast as the loop can call it, not one rung later;
 *   - a failed attempt (reported, or timed out — see `kAttemptTimeoutMs`) climbs one rung and
 *     schedules the next attempt. Backing off from "never connected" is therefore the ordinary
 *     path through this class rather than a special case;
 *   - a successful connection forgets the climb.
 *
 * Nothing here reads the library's retry state, and nothing waits for the library to retry on its
 * own: every attempt this device makes is one `update()` asked for.
 *
 * ── The integration contract the firmware must honour ────────────────────────────
 *
 * Ticking, and the one task that may do it (see the ownership note below):
 *
 *   1. call `update(millis())` once per pass of the priority-1 logic task, but ONLY while WiFi is
 *      associated and MQTT is enabled. Call `reset()` when either goes away — the policy has no
 *      view of the radio, and asking a client to connect with no IP just climbs the ladder for a
 *      failure that was never the broker's;
 *   2. if `shouldAttemptNow()` is true, make ONE attempt and nothing else. With no client yet
 *      (boot, or after `reset()` tore one down) that means `EspMqttTransport::begin()`; with a
 *      client parked after a failure it means `esp_mqtt_client_reconnect()`. The firmware must not
 *      start the client anywhere else: a `begin()` on the side is an attempt the ladder did not
 *      ration, and R4.1.2 is a claim about the rate the broker sees;
 *   3. `esp_mqtt_client_reconnect()` returns ESP_FAIL unless the client is parked waiting to
 *      reconnect ("ESP_FAIL if client is in invalid state", installed header), and a refusal must
 *      NOT simply be reported as a failed attempt — that would ration a call that can never
 *      succeed and reproduce this class's own blocker one level up, in the wiring. Split it by
 *      whether the client has reported a DISCONNECTED since it was last started:
 *        - it has → the client is not parked where `reconnect()` can act (on some esp-mqtt
 *          versions the task ends instead of parking when auto-reconnect is off), so RECREATE it:
 *          `end()` then `begin()`. That is this attempt;
 *        - it has not → a connect is genuinely still in flight. Do nothing at all and let the
 *          `kAttemptTimeoutMs` deadline below decide;
 *      and if the attempt could not be issued for a reason that will not change — no URI
 *      configured, `begin()` refused — call `noteAttemptFailed()` at once. It costs nothing and it
 *      saves the 20 s the deadline would otherwise take to reach the same conclusion;
 *   4. if the transport already reports `connected()` when a due edge arrives (a CONNECTED that
 *      crossed the tick boundary), answer it with `noteConnected()` and make no attempt. Never
 *      `noteAttemptFailed()`: tearing down a working session because the policy asked first is a
 *      worse outcome than the one rung it would cost to ignore.
 *
 * The event mapping, which is where the bug above was born, so it is spelled out:
 *
 *   - MQTT_EVENT_CONNECTED    → `noteConnected()`.
 *   - MQTT_EVENT_DISCONNECTED → `noteDisconnected()`. This is THE failure notification: esp-mqtt
 *     dispatches it from `esp_mqtt_abort_connection()`, which is what the client's INIT state calls
 *     when the transport connect or the CONNECT itself fails — so a first connect that never
 *     succeeded reports the same event as a live session that was torn down, and this policy needs
 *     no other.
 *
 *     PROVENANCE, because it decides the shape of the wiring: that reading is from
 *     `~/.platformio/packages/framework-espidf/components/mqtt/esp-mqtt/mqtt_client.c` at **IDF
 *     5.3.1**, which is NOT the IDF 4.4.7 esp-mqtt this firmware links — 4.4.7 ships as a
 *     precompiled `libmqtt.a` with no source on disk. The consistency evidence is that the archive
 *     names `MQTT_STATE_WAIT_RECONNECT` (and no `MQTT_STATE_WAIT_TIMEOUT`), i.e. the same state
 *     machine as the source read. It is an inference, not a verified fact, which is exactly why
 *     step 3 above recreates the client rather than trusting `reconnect()` to be accepted.
 *
 *     Every DISCONNECTED must be delivered: a firmware that instead compares
 *     `connected()` against its previous value sees no change at boot and drops it, which is the
 *     blocker this header used to prescribe. The transport must LATCH the event (a `volatile bool`
 *     the callback sets and the logic task reads and clears) and the logic task must pass it on.
 *   - MQTT_EVENT_ERROR        → nothing is required, and treating it as a disconnect on its own is
 *     wrong: esp-mqtt raises it for a transport fault and for a refused CONNACK alike, and in both
 *     cases DISCONNECTED follows. A handler that decodes `connect_return_code` (§9 wants a refused
 *     credential distinguishable) may call `noteAttemptFailed()` from it; the ladder cannot
 *     double-count, because the DISCONNECTED that follows arrives while the rung is already being
 *     waited out and is ignored.
 *   - everything else (SUBSCRIBED / UNSUBSCRIBED / PUBLISHED / DATA / BEFORE_CONNECT / DELETED) is
 *     not this policy's business.
 *
 * At construction the client must NOT be running: `Phase::Cold` means "nobody is connecting", and
 * the first due edge is what creates and starts it. Construct with the factory MAC (see below).
 *
 * ── Arduino-free and esp-mqtt-free, on purpose ───────────────────────────────────
 *
 * The whole content of R4.1.2 is a sequence of DURATIONS. Verifying it on hardware means sitting
 * in front of a bench with an unplugged broker for the better part of an hour per run, and the
 * interesting rung — the 5 min ceiling — is the one nobody ever waits for. So the clock is a
 * parameter, there is no dependency on `mqtt_client.h` or on `MqttSink`, and the ladder is
 * measured on a host in milliseconds of simulated hours. `EspMqttTransport` is left holding only
 * the two things a host test could lie about: the config struct and the event callback.
 *
 * ── Ownership: not thread-safe, and one caller is not enough to say so ───────────
 *
 * Nothing here locks and nothing here is atomic. `update()` and the notifications must all be
 * called from the ONE task that owns the client — the priority-1 logic task (R4.1.5).
 *
 * This is sharper than the equivalent note on `WifiManager`, because that class raises its own
 * edges internally whereas ours arrive from a different task by construction: esp-mqtt's event
 * callback runs in esp-mqtt's own task. The callback must therefore NOT call `noteConnected()` /
 * `noteDisconnected()` directly. It latches, the logic task drains the latch once per tick, and
 * fans each notification out to both `MqttPublisher::onConnected()/onDisconnected()` and to this
 * policy. One notification, one task, two consumers.
 */
class MqttReconnect {
 public:
  /**
   * The ladder, R4.1.2 by way of §3.1.2: 1 s doubling to a 5 min ceiling, with jitter.
   *
   * MIRRORED from `WifiManager::kInitialBackoffMs` / `kMaxBackoffMs` / `kBackoffJitterDivisor`
   * rather than shared, and the choice is deliberate in both directions:
   *
   *   - shared would be better if it were free, but the only honest way to share is a third
   *     header both modules include, and a backoff-constants header is not a file this slice
   *     owns. Including `wifi_manager.h` from HERE is the wrong price: it would drag `NetSettings`
   *     and the `WifiRadio` interface into every translation unit that only wants to know how long
   *     to wait before the next CONNECT;
   *   - so the values are duplicated, and the duplication is made answerable instead of trusted.
   *     `mqtt_reconnect.cpp` static_asserts each one against `WifiManager`'s, which makes a change
   *     to either side a compile error in the firmware build rather than a drift nobody notices
   *     until two subsystems retry at two different rates. It sits in the .cpp precisely so the
   *     .h stays dependency-free, and the host test additionally pins §3.1.2's LITERAL figures —
   *     a check that derives its expectation from these constants cannot fail when they change,
   *     which is the exact defect that had to be fixed in `wifi_manager_test` this session.
   */
  static constexpr uint32_t kInitialBackoffMs = 1000;
  static constexpr uint32_t kMaxBackoffMs = 300000;

  /**
   * The jitter band, as a divisor of the current step: the delay is `base - [0, base/8]`.
   *
   * SUBTRACTIVE, for the reason `WifiManager` documents and which applies with more force here:
   * additive jitter on a step already clamped to `kMaxBackoffMs` would either exceed §3.1.2's
   * ceiling or be clamped away exactly AT the ceiling — where it is needed most, because a whole
   * site's meters, all rebooted by one power cut and all pointed at one broker, would otherwise
   * reconnect in lockstep forever. R4.1.2 names that case: "MQTT reconnect storms against an
   * unreachable broker are a common way to make a device unusable."
   */
  static constexpr uint32_t kBackoffJitterDivisor = 8;

  /**
   * How long an attempt may stay outstanding before it counts as failed. A DESIGN CHOICE, not a
   * requirement figure — no clause in the document names it.
   *
   * Three failures it exists to prevent, in order of severity:
   *
   *   - the notification that never comes. With `disable_auto_reconnect = true` the library will
   *     not retry on its own, so an attempt whose verdict is never delivered — a firmware that
   *     wires CONNECTED but forgets DISCONNECTED, a client that accepts the TCP connection and
   *     then black-holes the CONNECT — would leave MQTT down forever. This deadline is what makes
   *     the ladder run on a device whose event plumbing is broken: every attempt has an end;
   *   - the dropped edge. `shouldAttemptNow()` is true for one tick (see below). A caller that
   *     misses it would otherwise wait forever; with this, the missed attempt is counted as a
   *     failure and the ladder keeps running, one rung slower. Degrading toward LONGER delays is
   *     the safe direction for R4.1.2;
   *   - the attempt that outlives its rung, once the ladder is near the bottom.
   *
   * 20 s is double the transport's `network_timeout_ms` (10 s), so the library's own DISCONNECTED
   * normally arrives first and this only fires when nothing was reported at all. It also matches
   * `WifiManager::kConnectTimeoutMs`, which covers the same "the driver never answered" case.
   */
  static constexpr uint32_t kAttemptTimeoutMs = 20000;

  /**
   * Seeded from the factory MAC, which is what makes a fleet diverge without an entropy source.
   *
   * The MAC is already in the caller's hand for R4.1.4's client id, so this costs nothing at the
   * call site. A different hash basis from `WifiManager`'s keeps the two ladders on ONE device from
   * stepping together as well — they are independent failures and should not share a jitter phase.
   */
  explicit MqttReconnect(const uint8_t mac[6]);

  /**
   * One tick. The ONLY place a deadline is evaluated, the only place an attempt is issued, and the
   * only place `shouldAttemptNow()` changes.
   *
   * Call it once per pass of the owning loop and test `shouldAttemptNow()` immediately after:
   * the edge is cleared at the top of the NEXT `update()`, so a caller that ticks twice before
   * looking will drop it (recoverably — see `kAttemptTimeoutMs`).
   *
   * The FIRST call on a cold policy always asks for an attempt, which is why a caller must not
   * tick this while WiFi is down (see the contract above).
   *
   * `nowMs` is a free-running millisecond counter and is allowed to wrap: every deadline is
   * compared as a signed difference, so the 49.7-day rollover is a non-event rather than a
   * 49.7-day hang.
   */
  void update(uint32_t nowMs);

  /** The session is up. Forgets the ladder — R4.1.2's ramp measures CONSECUTIVE failures. */
  void noteConnected();

  /**
   * The session is not up: esp-mqtt raised MQTT_EVENT_DISCONNECTED.
   *
   * The library reports both "the connect attempt failed" and "the live session was torn down"
   * this way, and with `disable_auto_reconnect = true` it is the only failure notification the
   * transport can raise honestly. Both climb one rung.
   *
   * Ignored in two phases, for two different reasons:
   *
   *   - while already waiting out a rung. That guard is not defensive tidiness: the event arrives
   *     from esp-mqtt's task, ERROR-then-DISCONNECTED pairs and repeats are normal, and a policy
   *     that advanced a rung per event would double its delay per duplicate and leave the device
   *     at the 5 min ceiling after one flaky minute;
   *   - while cold. Before the first tick there is no attempt of ours outstanding and no session
   *     of ours to lose, so the event describes a client this policy did not ask for — a late
   *     DISCONNECTED from the session `reset()` just abandoned, typically. Honouring it would put
   *     the fresh first attempt one rung late, and `reset()` exists to do the opposite. Nothing is
   *     lost by ignoring it: the attempt follows on the very next tick anyway.
   *
   * A disconnect from a LIVE session therefore always starts again at 1 s, because
   * `noteConnected()` has already zeroed the counter. That is deliberate and it mirrors R3.1.3's
   * treatment of a dropped WiFi link: a broker that demonstrably worked a moment ago must not
   * inherit the ceiling some earlier misconfiguration climbed to.
   */
  void noteDisconnected();

  /**
   * The attempt this policy asked for did not connect. The same climbing edge, from a caller that
   * knows the attempt failed without the library having said DISCONNECTED yet.
   *
   * Two callers: a transport whose `begin()` / `esp_mqtt_client_reconnect()` refused outright
   * (contract step 3 above), and a handler that reads MQTT_EVENT_ERROR's `connect_return_code` and
   * can tell a refused CONNACK — bad credentials, a fault that will not fix itself and the one §9
   * wants distinguishable — from a transport hiccup.
   *
   * Acted on only while an attempt is outstanding: "the attempt failed" is meaningless when no
   * attempt was made, and honouring it from `Connected` would let a caller invent a disconnect
   * that the publisher's own view of the session does not share.
   */
  void noteAttemptFailed();

  /**
   * Back to cold: no ladder, and the next tick attempts immediately.
   *
   * For the case that would otherwise be a five-minute hole in the availability of a device that
   * is working perfectly: WiFi drops, MQTT cannot reach the broker and climbs to the ceiling, WiFi
   * comes back — and the client sits out a rung it earned for a failure that was never the
   * broker's. Call this from the WiFi association edge (`WifiManager::consumeJustConnected()`) and
   * from the far side of the settings change that tore the client down; the first tick afterwards
   * asks for the attempt, so there is no `begin()` for the firmware to make on its own.
   *
   * PRECONDITION: no client is running — call `EspMqttTransport::end()` first if one is. `Cold`
   * asserts "nobody is connecting", and the tick after this one issues an attempt; issuing it at a
   * client that is connected or mid-connect is how a live session gets torn down by its own
   * reconnection policy. Losing WiFi already satisfies this, because the session is gone with it.
   *
   * Leaves the jitter stream where it is, so a device that resets repeatedly does not replay one
   * jitter sequence and re-synchronise with its neighbours.
   */
  void reset();

  /**
   * True on the ONE tick when an attempt is due. The caller answers it by starting the client, or
   * by calling `esp_mqtt_client_reconnect()` if one is already parked after a failure.
   *
   * One tick rather than "true until satisfied", because that call is exactly what R4.1.2 is
   * rationing: a sticky flag would have the owner issue it on every pass of the loop for as long
   * as the attempt took to fail, which is the storm this class exists to prevent wearing the
   * costume of a fix for it.
   */
  bool shouldAttemptNow() const { return attemptDue_; }

  /**
   * The delay chosen for the wait now in progress, in ms; 0 when not waiting — which includes the
   * cold state, where the wait before the first attempt is zero by design.
   *
   * Exposed — same reasoning as `WifiManager::retryDelayMs()` — so the ceiling can be asserted
   * exactly rather than inferred from a ticked measurement, whose resolution is the tick. The
   * tests assert BOTH: this value, and that the policy actually waits it out.
   */
  uint32_t retryDelayMs() const;

  /** Consecutive failed attempts since the last successful connection. Saturates rather than wraps. */
  uint16_t attempts() const { return attempts_; }

 private:
  /**
   * Where the policy is. Private because it is not a wire encoding and nothing outside needs it:
   * everything observable is reachable through `shouldAttemptNow()`, `retryDelayMs()` and
   * `attempts()`. Contrast `WifiState`, which IS register 501 and must never be renumbered.
   */
  enum class Phase : uint8_t {
    Cold,       /**< Nothing connecting and nothing scheduled. The next tick attempts, undelayed. */
    Waiting,    /**< Sitting out a rung. */
    Attempting, /**< An attempt has been asked for and no verdict has arrived. */
    Connected   /**< The session is up; no ladder runs. */
  };

  /** Signed comparison, so a wrapped `nowMs` does not park a deadline 49 days into the future. */
  bool reached(uint32_t deadlineMs) const {
    return static_cast<int32_t>(nowMs_ - deadlineMs) >= 0;
  }

  void beginAttempt();
  void failOnce();
  uint32_t nextRandom();

  Phase phase_ = Phase::Cold;

  uint32_t nowMs_ = 0;
  uint32_t retryDeadlineMs_ = 0;
  uint32_t retryDelayMs_ = 0;
  uint32_t attemptDeadlineMs_ = 0;

  uint16_t attempts_ = 0;
  bool attemptDue_ = false;

  /** Jitter stream, seeded from the MAC. See the constructor. */
  uint32_t random_ = 0;
};

}  // namespace plc
