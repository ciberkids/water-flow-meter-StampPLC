#pragma once

#include <cstdint>

namespace plc {

/**
 * LED override patterns — RGB_LED_Behavior.md §3.4 and §3.5.
 *
 * Pure arithmetic in a dependency-free header so the ramp formula and the boot
 * sequence can be host-tested. LedController itself talks to M5StamPLC, so anything
 * left inside it is only checkable on hardware.
 */

/** Which override, if any, is displacing the normal channel semantics. */
enum class LedOverride : uint8_t {
  None = 0,
  /** §3.4 — cycling R/G/B while the controller initialises. */
  Boot,
  /** §3.4 — initialisation has taken too long; a red blink instead. */
  BootStalled,
  /** §3.5 — accelerating white flash during a destructive countdown. */
  ResetRamp,
  /** §3.5 — solid white, the signal that a reset was accepted. */
  ResetAccepted
};

struct LedState {
  bool red = false;
  bool green = false;
  bool blue = false;
};

// §3.4
constexpr uint32_t kBootStepMs = 150;
constexpr uint32_t kBootStallMs = 10000;
constexpr uint32_t kBootStallBlinkMs = 500;

// §3.5
constexpr uint32_t kRampSlowestMs = 600;
constexpr uint32_t kRampFastestMs = 60;

/**
 * Blink period for a reset countdown, derived from the FRACTION remaining.
 *
 * Using the fraction rather than the absolute time is what lets one formula serve both
 * a 3 s and a 30 s countdown without tuning: both start at kRampSlowestMs and arrive at
 * kRampFastestMs together, so the LED and the on-screen digits always tell the same
 * story.
 */
constexpr uint32_t resetRampPeriodMs(uint32_t remainingMs, uint32_t totalMs) {
  if (totalMs == 0) {
    return kRampFastestMs;
  }
  const uint32_t clamped = remainingMs > totalMs ? totalMs : remainingMs;
  const uint32_t span = kRampSlowestMs - kRampFastestMs;
  // 64-bit intermediate: 30000 * 540 overflows a uint32_t.
  const uint32_t scaled =
      static_cast<uint32_t>((static_cast<uint64_t>(span) * clamped) / totalMs);
  return kRampFastestMs + scaled;
}

/** R -> G -> B, one channel at a time. */
constexpr LedState bootSnakeState(uint32_t elapsedMs) {
  const uint32_t step = (elapsedMs / kBootStepMs) % 3;
  return {step == 0, step == 1, step == 2};
}

constexpr LedState bootStalledState(uint32_t elapsedMs) {
  return {((elapsedMs / kBootStallBlinkMs) % 2) == 0, false, false};
}

/** All three in unison, so the flash reads as white rather than a colour. */
constexpr LedState resetRampState(uint32_t elapsedMs, uint32_t remainingMs, uint32_t totalMs) {
  const uint32_t period = resetRampPeriodMs(remainingMs, totalMs);
  const bool on = (elapsedMs % period) < (period / 2);
  return {on, on, on};
}

constexpr LedState resetAcceptedState() { return {true, true, true}; }

}  // namespace plc
