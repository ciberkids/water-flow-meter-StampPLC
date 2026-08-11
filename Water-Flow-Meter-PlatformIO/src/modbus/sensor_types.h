#pragma once

#include <cstdint>

#include <algorithm>
#include <cstdlib>

/**
 * How a channel's meter is specified — the two forms real datasheets use.
 *
 * `Formula` is `F = f_multiplier * Q + adjust`, the linear fit printed on turbine meters.
 * `PulsesPerLitre` is a single K figure, which is the more common form and which the formula fields
 * cannot represent: K = 450 needs a multiplier of 7.5 and `f_multiplier` is an integer.
 */
enum class CalibrationType : uint16_t {
  Formula = 0,
  PulsesPerLitre = 1
};

struct SensorCharacteristics {
  uint16_t q_max = 0;
  int16_t f_multiplier = 0;
  int16_t adjust = 0;
  CalibrationType calibration = CalibrationType::Formula;
  /** Pulses per litre, used only when `calibration` is PulsesPerLitre. Exact, not scaled. */
  uint16_t pulses_per_litre = 0;

  bool operator==(const SensorCharacteristics& other) const {
    return q_max == other.q_max && f_multiplier == other.f_multiplier && adjust == other.adjust &&
           calibration == other.calibration && pulses_per_litre == other.pulses_per_litre;
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
  // Common to both forms: without a nominal maximum there is nothing to clamp a reading to, and the
  // Nyquist check has no ceiling frequency to test.
  if (cfg.q_max == 0) {
    return false;
  }
  // The pulses-per-litre form needs only K. There is no offset to sanity-check, because a
  // pulses-per-litre spec has none — that is the whole reason it is a separate form and not a
  // formula with adjust set to zero.
  if (cfg.calibration == CalibrationType::PulsesPerLitre) {
    return cfg.pulses_per_litre != 0;
  }
  if (cfg.f_multiplier == 0) {
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
  /**
   * Instantaneous flow in LITRES PER MINUTE (§2a).
   *
   * Was `instantFlow_L_s`. L/min is how water flow is specified everywhere it matters here — the
   * datasheet's Q, `q_max`, the Nyquist limit, MQTT, Home Assistant and the panel — so storing L/s
   * meant a division on the way in and a multiplication on every one of those ways out. §2a's point
   * is that the change REMOVES conversions rather than adding them.
   */
  float instantFlow_L_min = 0.0f;
  double cumulativeLiters = 0.0;
  float sessionLiters = 0.0f;
  /** Peak `instantFlow_L_min` since the last session reset, in the same unit. */
  float maxFlowSinceReset = 0.0f;
};
