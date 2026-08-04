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
    case Phase::Idle:
      // Nothing to do, forever, until somebody reports a disconnect. §7's rule for the radio —
      // nothing switches itself on — applies to the broker connection too: the client's first
      // connect belongs to whoever called begin().
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
  if (phase_ == Phase::Waiting) {
    // Already sitting out a rung. See the header: repeats and ERROR/DISCONNECTED pairs are normal,
    // and one rung per event would put a flaky minute at the five-minute ceiling.
    return;
  }
  failOnce();
}

void MqttReconnect::noteAttemptFailed() {
  if (phase_ != Phase::Attempting) {
    return;  // No attempt outstanding: there is nothing this could truthfully be about.
  }
  failOnce();
}

void MqttReconnect::reset() {
  phase_ = Phase::Idle;
  attempts_ = 0;
  retryDelayMs_ = 0;
  attemptDue_ = false;
  // random_ untouched — see the header.
}

}  // namespace plc
