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
 * `disable_auto_reconnect = true` — otherwise the library's fixed retry races this policy and wins,
 * and the ladder becomes a number we print rather than a rate the broker observes. That is the same
 * hazard `WifiManager::failBackoff()` calls `radio_.disconnect()` to close.
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
 * Nothing here locks and nothing here is atomic. `update()` and the three notifications must all
 * be called from the ONE task that owns the client — the priority-1 logic task (R4.1.5).
 *
 * This is sharper than the equivalent note on `WifiManager`, because that class raises its own
 * edges internally whereas ours arrive from a different task by construction: esp-mqtt's event
 * callback runs in esp-mqtt's own task. The callback must therefore NOT call `noteConnected()` /
 * `noteDisconnected()` directly. It sets a flag (the transport's `connected_` is already
 * `volatile` for exactly this), the logic task derives the edge from it once per tick, and fans
 * that one edge out to both `MqttPublisher::onConnected()/onDisconnected()` and to this policy.
 * One edge, one task, two consumers.
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
   * Two failures it exists to prevent, in order of likelihood:
   *
   *   - the client that answers nothing. `esp_mqtt_client_reconnect()` returns immediately and the
   *     verdict arrives later as an event. A broker that accepts the TCP connection and then
   *     black-holes the CONNECT produces no event at all until some timeout fires somewhere, and
   *     without a deadline of our own the ladder stops dead — an MQTT link that is silently never
   *     retried again, which is worse than a storm because nothing reports it;
   *   - the dropped edge. `shouldAttemptNow()` is true for one tick (see below). A caller that
   *     misses it would otherwise wait forever; with this, the missed attempt is counted as a
   *     failure and the ladder keeps running, one rung slower. Degrading toward LONGER delays is
   *     the safe direction for R4.1.2.
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
   * One tick. The ONLY place a deadline is evaluated, and the only place `shouldAttemptNow()`
   * changes.
   *
   * Call it once per pass of the owning loop and test `shouldAttemptNow()` immediately after:
   * the edge is cleared at the top of the NEXT `update()`, so a caller that ticks twice before
   * looking will drop it (recoverably — see `kAttemptTimeoutMs`).
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
   * This is the ARMING edge, and with `disable_auto_reconnect = true` it is the one production
   * actually uses — the library reports both "the initial connect failed" and "the live session
   * was torn down" this way, and it is the only notification the transport can raise honestly.
   *
   * Ignored while already waiting out a rung. That guard is not defensive tidiness: the event
   * arrives from esp-mqtt's task, ERROR-then-DISCONNECTED pairs and repeats are normal, and a
   * policy that advanced a rung per event would double its delay per duplicate and leave the
   * device at the 5 min ceiling after one flaky minute.
   *
   * A disconnect from a LIVE session therefore always starts again at 1 s, because
   * `noteConnected()` has already zeroed the counter. That is deliberate and it mirrors R3.1.3's
   * treatment of a dropped WiFi link: a broker that demonstrably worked a moment ago must not
   * inherit the ceiling some earlier misconfiguration climbed to.
   */
  void noteDisconnected();

  /**
   * The attempt this policy asked for did not connect. The CLIMBING edge.
   *
   * Acted on only while an attempt is outstanding — "the attempt failed" is meaningless when no
   * attempt was made, and honouring it from `Connected` would let a caller invent a disconnect
   * that the publisher's own view of the session does not share.
   *
   * Distinct from `noteDisconnected()` so a caller that CAN tell a refused CONNACK (bad
   * credentials — a fault that will not fix itself, and the one §9 wants distinguishable) from a
   * session teardown has somewhere to say so without also asserting that the session was ever up.
   * esp-mqtt does not give the transport that distinction today, so on the device the failure path
   * runs through `noteDisconnected()`; this is exercised by the host tests and by any future
   * caller that reads MQTT_EVENT_ERROR's `connect_return_code`.
   */
  void noteAttemptFailed();

  /**
   * Forget everything: no ladder, nothing due, waiting for the next disconnect.
   *
   * For the case that would otherwise be a five-minute hole in the availability of a device that
   * is working perfectly: WiFi drops, MQTT cannot reach the broker and climbs to the ceiling, WiFi
   * comes back — and the client, restarted by the firmware, sits out a rung it earned for a
   * failure that was never the broker's. The alternative is a firmware that simply stops ticking
   * this policy while WiFi is down, which gets an immediate attempt on the first tick after the
   * link returns; that works too, but it hides the intent, and it breaks the moment somebody adds
   * an unconditional `update()` to the loop. Call this from the WiFi association edge
   * (`WifiManager::consumeJustConnected()`), next to the client's own `begin()`.
   *
   * Leaves the jitter stream where it is, so a device that resets repeatedly does not replay one
   * jitter sequence and re-synchronise with its neighbours.
   */
  void reset();

  /**
   * True on the ONE tick when a reconnect is due. The caller answers it by calling
   * `esp_mqtt_client_reconnect()`.
   *
   * One tick rather than "true until satisfied", because `esp_mqtt_client_reconnect()` is exactly
   * the call R4.1.2 is rationing: a sticky flag would have the owner issue it on every pass of the
   * loop for as long as the attempt took to fail, which is the storm this class exists to prevent
   * wearing the costume of a fix for it.
   */
  bool shouldAttemptNow() const { return attemptDue_; }

  /**
   * The delay chosen for the wait now in progress, in ms; 0 when not waiting.
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
    Idle,       /**< At rest. The client's own first connect is not this policy's business. */
    Waiting,    /**< Sitting out a rung. */
    Attempting, /**< A reconnect has been asked for and no verdict has arrived. */
    Connected   /**< The session is up; no ladder runs. */
  };

  /** Signed comparison, so a wrapped `nowMs` does not park a deadline 49 days into the future. */
  bool reached(uint32_t deadlineMs) const {
    return static_cast<int32_t>(nowMs_ - deadlineMs) >= 0;
  }

  void beginAttempt();
  void failOnce();
  uint32_t nextRandom();

  Phase phase_ = Phase::Idle;

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
