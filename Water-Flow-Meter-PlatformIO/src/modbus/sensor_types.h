#pragma once

#include <cstdint>

#include <algorithm>

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
 * The rules: q_max must be non-zero, or no frequency maps to a flow at all; the multiplier must be
 * POSITIVE, for the reason below; and the offset must be non-negative and may not exceed the frequency
 * the channel can actually reach, or the correction would dominate the measurement.
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
  /**
   * POSITIVE, not merely non-zero. This tested `!= 0`, and a negative multiplier is the one value a
   * Modbus master could install that the panel's editor cannot reach.
   *
   * The multiplier is a DIVISOR: the engine computes `flow = (F - adjust) / f_multiplier`
   * (sensor_state_engine.cpp:47), so a negative one maps rising frequency to falling flow and the
   * `flowRateLpm < 0` clamp below it then pins the result at zero — a channel that reads 0.00 no matter
   * how much water passes it, while reporting itself READY with no warning flag.
   *
   * THE DECISION WAS ALREADY RECORDED, in two places, and only this predicate had not been told:
   * ui_settings_types.cpp bounds the panel's multiplier editor at 1..32767 and its comment names this
   * exact consequence, and tools/wiki/gen-registers.mjs publishes "the legal range is 1..32767" for
   * OFF_CFG_F_MULT. Neither the panel's commit path nor the register arm re-checked it — the bound lived
   * only in the stepper that clamps while editing — so the range the integrator's documentation promises
   * was enforced nowhere. It is enforced here because this is the one choke point all four surfaces
   * share.
   */
  if (cfg.f_multiplier < 1) {
    return false;
  }
  const int32_t multiplier = static_cast<int32_t>(cfg.f_multiplier);
  const int32_t adjust = static_cast<int32_t>(cfg.adjust);
  /**
   * NON-NEGATIVE, and no larger than the reachable frequency. Both halves were decided in DF14 after
   * this predicate and its own comment had disagreed by a factor of ten for as long as either existed.
   *
   * WHY NEGATIVE IS REFUSED, which is the half that repairs something. The engine computes
   * `flow = (F - adjust) / f_multiplier` (sensor_state_engine.cpp:47) and clamps only a NEGATIVE result
   * to zero. A negative offset therefore produces a POSITIVE reading at zero frequency, which sails past
   * that clamp: `m = 10, q_max = 150, adjust = -1400` reads **140 L/min on a dry pipe**, accumulates
   * volume into the session and lifetime totals every cycle, and reports itself READY with register 30
   * clear. For a pulse meter the rule is not a matter of taste — zero pulses is zero flow, and a
   * calibration that says otherwise is describing something the sensor cannot measure.
   *
   * There is deliberately NO OVERRIDE. §5.5's write-twice handshake covers candidates that are valid but
   * outrun the sampler (`meetsNyquistLimit`); `prepareConfigUpdate` clears the override state and returns
   * early for an invalid one, so this is a hard refusal on every route. A datasheet whose least-squares
   * intercept really is negative is entered as PULSES PER LITRE, or with the intercept at 0 and the small
   * low-flow error that implies. The panel's editor is bounded 0..32767 to match, exactly as the
   * multiplier's is 1..32767 for the same class of reason.
   *
   * The upper bound is now what the comment always claimed: `q_max * multiplier`, the frequency the
   * channel reaches at its nominal maximum. Widest legal product is 65535 * 32767 = 2,147,385,345, which
   * fits int32 with 98,302 to spare — so no promotion is needed here, but do not widen either field
   * without revisiting this line.
   */
  if (adjust < 0) {
    return false;
  }
  return adjust <= static_cast<int32_t>(cfg.q_max) * multiplier;
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
