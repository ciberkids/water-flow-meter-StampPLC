#include "net/mqtt_reconnect.h"

// Included for the static_asserts below and for NOTHING else — no symbol from it is referenced, so
// this costs a header parse and adds no link dependency (the constants are `static constexpr`, and
// reading one in a constant expression is not an odr-use).
//
// It is here rather than in mqtt_reconnect.h so that a consumer who wants only "how long until the
// next CONNECT" does not also compile NetSettings and the WifiRadio interface. The point of the
// asserts is that R4.1.2's "the same backoff policy as §3.1.2" is enforced by the compiler: the
// device has ONE backoff behaviour, and the two copies of its numbers cannot drift apart silently.
#include "net/wifi_manager.h"

namespace plc {
namespace {

static_assert(MqttReconnect::kInitialBackoffMs == WifiManager::kInitialBackoffMs,
              "R4.1.2 is 'the same backoff policy as §3.1.2' — the two ladders must start together");
static_assert(MqttReconnect::kMaxBackoffMs == WifiManager::kMaxBackoffMs,
              "R4.1.2 is 'the same backoff policy as §3.1.2' — and share one ceiling");
static_assert(MqttReconnect::kBackoffJitterDivisor == WifiManager::kBackoffJitterDivisor,
              "R4.1.2 is 'the same backoff policy as §3.1.2' — and one jitter band");

/** FNV-1a. Short, deterministic and dependency-free; not chosen for cryptography. */
uint32_t fnv1a(const uint8_t* data, std::size_t count, uint32_t basis) {
  uint32_t hash = basis;
  for (std::size_t i = 0; i < count; ++i) {
    hash ^= data[i];
    hash *= 16777619u;
  }
  return hash;
}

uint32_t xorshift(uint32_t& state) {
  uint32_t x = state;
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  state = x;
  return x;
}

}  // namespace

MqttReconnect::MqttReconnect(const uint8_t mac[6]) {
  // A different mask from WifiManager's 0xA5A5A5A5, so the WiFi ladder and the MQTT ladder of one
  // device do not draw the same jitter sequence. Same basis, so the fleet-divergence property comes
  // from the MAC in both.
  random_ = fnv1a(mac, 6, 2166136261u) ^ 0x5E7B1CE5u;
  if (random_ == 0) {
    random_ = 1;  // xorshift is absorbing at zero; a MAC hashing to 0 would give a constant delay.
  }
}

uint32_t MqttReconnect::nextRandom() { return xorshift(random_); }

uint32_t MqttReconnect::retryDelayMs() const {
  return phase_ == Phase::Waiting ? retryDelayMs_ : 0;
}

void MqttReconnect::update(uint32_t nowMs) {
  nowMs_ = nowMs;

  // The edge lives for one tick. Cleared here rather than by the reader so `shouldAttemptNow()`
  // stays a const query — a consuming getter reads like a question and behaves like a command, and
  // this one is asked from the same loop that decides whether to publish.
  attemptDue_ = false;

  switch (phase_) {
    case Phase::Cold:
      // The first attempt, undelayed, and the fix for the blocker the header describes: a broker
      // that is unreachable from boot never produces a CONNECTED → DISCONNECTED edge, so a policy
      // that waited here for one would never retry at all — and with `disable_auto_reconnect =
      // true` nothing else would either. Issuing it here also means the ladder never delays the
      // first CONNECT of a device sitting next to a working broker (R4.1.2 rations retries, not
      // the first try).
      beginAttempt();
      return;
    case Phase::Connected:
      return;
    case Phase::Waiting:
      if (reached(retryDeadlineMs_)) {
        beginAttempt();
      }
      return;
    case Phase::Attempting:
      if (reached(attemptDeadlineMs_)) {
        // Nothing was reported for kAttemptTimeoutMs. Counting it as a failure keeps the ladder
        // running; the alternative is a link that is never retried again and never says so.
        failOnce();
      }
      return;
  }
}

void MqttReconnect::beginAttempt() {
  phase_ = Phase::Attempting;
  attemptDeadlineMs_ = nowMs_ + kAttemptTimeoutMs;
  attemptDue_ = true;
}

void MqttReconnect::failOnce() {
  if (attempts_ < 0xFFFFu) {
    ++attempts_;
  }

  // R4.1.2 via §3.1.2 — double from 1 s, and stop doubling at the 5 min ceiling. Arithmetic
  // deliberately identical to WifiManager::failBackoff(), down to the `+ 1u` in the modulo, because
  // "the same backoff policy" is a claim about the delays a broker sees and not about the prose.
  uint32_t base = kInitialBackoffMs;
  for (uint16_t i = 1; i < attempts_ && base < kMaxBackoffMs; ++i) {
    base <<= 1;
  }
  if (base >= kMaxBackoffMs) {
    base = kMaxBackoffMs;
  }
  const uint32_t jitter = nextRandom() % (base / kBackoffJitterDivisor + 1u);
  retryDelayMs_ = base - jitter;
  retryDeadlineMs_ = nowMs_ + retryDelayMs_;

  phase_ = Phase::Waiting;
  attemptDue_ = false;  // A failure withdraws an outstanding edge the caller has not acted on.
}

void MqttReconnect::noteConnected() {
  phase_ = Phase::Connected;
  attempts_ = 0;
  retryDelayMs_ = 0;
  attemptDue_ = false;
}

void MqttReconnect::noteDisconnected() {
  // A switch rather than two ifs because the two phases that ignore this do so for different
  // reasons, and because -Werror then keeps the answer explicit if a phase is ever added.
  switch (phase_) {
    case Phase::Waiting:
      // Already sitting out a rung. Repeats and ERROR-then-DISCONNECTED pairs are normal from
      // esp-mqtt's task, and one rung per event would put a flaky minute at the 5 min ceiling.
      return;
    case Phase::Cold:
      // Not ours: no attempt outstanding and no session to lose. Typically a late DISCONNECTED
      // from the client reset() just abandoned. Climbing here would put the fresh first attempt a
      // rung late, which is the opposite of what reset() is for — and it costs nothing to ignore,
      // because the next tick attempts anyway.
      return;
    case Phase::Attempting:
    case Phase::Connected:
      // The two real failures: the attempt we asked for did not come up, and the live session went
      // away. Both are one rung.
      failOnce();
      return;
  }
}

void MqttReconnect::noteAttemptFailed() {
  if (phase_ != Phase::Attempting) {
    return;  // No attempt outstanding: there is nothing this could truthfully be about.
  }
  failOnce();
}

void MqttReconnect::reset() {
  // Cold, not "at rest": the next tick asks for an attempt. See the header — the alternative is a
  // client that comes back with WiFi and then sits out a rung it earned against a broker that was
  // never the problem.
  phase_ = Phase::Cold;
  attempts_ = 0;
  retryDelayMs_ = 0;
  attemptDue_ = false;
  // random_ untouched — see the header.
}

}  // namespace plc
