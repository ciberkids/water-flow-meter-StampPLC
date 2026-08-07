#pragma once

#include <cstdint>

#include <algorithm>
#include <cstdlib>

struct SensorCharacteristics {
  uint16_t q_max = 0;
  int16_t f_multiplier = 0;
  int16_t adjust = 0;

  bool operator==(const SensorCharacteristics& other) const {
    return q_max == other.q_max && f_multiplier == other.f_multiplier && adjust == other.adjust;
  }

  bool operator!=(const SensorCharacteristics& other) const {
    return !(*this == other);
  }
};

/**
 * Whether this configuration can produce a flow reading — the whole of what "ready" ever meant.
 *
 * It lives HERE, beside the struct it tests, because it used to live in an anonymous namespace inside
 * modbus_manager.cpp. Being unreachable from any other translation unit is what created the bug it now
 * replaces: the answer was cached into `SensorData::isReady` purely so it could cross a module boundary,
 * and boot then cleared the cache without recomputing it. A calibrated channel therefore came back from
 * every reboot reporting not-ready — pulses counted and discarded, and the lifetime total published to
 * Modbus as 0.0. Nothing was wrong with the predicate; it was in the wrong place.
 *
 * The rules, unchanged from the original: both q_max and the multiplier must be non-zero, or no frequency
 * maps to a flow at all; and the offset may not exceed the frequency the channel can actually reach, or
 * the correction would dominate the measurement.
 */
inline bool configIsValid(const SensorCharacteristics& cfg) {
  if (cfg.q_max == 0 || cfg.f_multiplier == 0) {
    return false;
  }
  const int32_t multiplier = std::max<int32_t>(std::abs(static_cast<int32_t>(cfg.f_multiplier)), 1);
  const int32_t limit = static_cast<int32_t>(cfg.q_max) * multiplier * 10;
  const int32_t adjust = static_cast<int32_t>(cfg.adjust);
  return std::abs(adjust) <= limit;
}

/**
 * A channel's runtime state.
 *
 * There is deliberately no `isReady` here. Readiness is `configIsValid(configs[n])` — a question the
 * configuration already answers, so storing the answer could only ever go stale, and did.
 */
struct SensorData {
  bool inUse = false;
  volatile uint32_t pulseCount = 0;
  float instantFlow_L_s = 0.0f;
  double cumulativeLiters = 0.0;
  float sessionLiters = 0.0f;
  float maxFlowSinceReset = 0.0f;
};
