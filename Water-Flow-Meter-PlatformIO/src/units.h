#pragma once

/**
 * Unit conversions, each with exactly one implementation.
 *
 * WHY THIS FILE EXISTS. `Display_Per_Screen_Spec.md` §2a.1 requires every wire surface to carry both
 * litres and cubic metres, which doubles the number of places a division by 1000 happens. That
 * division had four homes, each a bare literal: `firmware.cpp` (into the MQTT payload),
 * `ui_bindings.cpp` twice (cumulative and session, for the panel), and `modbus_manager.cpp` twice
 * (once into a double, once into a float). §2a.1's instruction is that "a single
 * litresToCubicMeters() comes first" — before the surfaces multiply.
 *
 * All of those now call through here. That mattered to say out loud, because introducing the helper
 * and leaving the literals in place would have made it a FIFTH home rather than the only one, and
 * this comment would have described something untrue.
 *
 * A bare `/ 1000.0` is not wrong on its own; four of them are, because nothing makes them agree and
 * the compiler cannot tell you when one is missed.
 */

#include <cstdint>

namespace units {

/** Litres to cubic metres. 1 m3 = 1000 L, exactly. */
inline constexpr double litresToCubicMeters(double litres) {
  return litres / 1000.0;
}

/** Litres to cubic metres, for the float accumulators. */
inline constexpr float litresToCubicMeters(float litres) {
  return litres / 1000.0f;
}

/**
 * There is deliberately NO flow conversion here any more.
 *
 * `flowLpsToLpm` lived here through the transition and its own comment said what would end it: §2a
 * moves the stored unit to L/min, at which point flow is already in the unit the panel, `q_max`, the
 * Nyquist limit, MQTT and Home Assistant all use, and the function has no callers. That move has now
 * happened, so it is deleted rather than left as a tempting no-op.
 *
 * The one place a per-second figure is still produced is `telemetry.totalFlowLps`, which divides at
 * the point of use because it is the only consumer that wants seconds.
 */

/**
 * The panel's flow unit — a DISPLAY choice, applied at render and nowhere else.
 *
 * Storage is L/min (§2a) and every wire surface keeps it, so this converts on the way to the glyphs
 * and never on the way to a register. One function, because otherwise each of the four flow readings
 * would carry its own factor and they would drift the way the `/ 1000` volume conversions did.
 */
enum class FlowUnit : uint16_t { LitresPerMinute = 0, LitresPerSecond = 1, CubicMetresPerHour = 2 };

inline double flowFromLpm(double litresPerMinute, FlowUnit unit) {
  switch (unit) {
    case FlowUnit::LitresPerSecond:
      return litresPerMinute / 60.0;
    case FlowUnit::CubicMetresPerHour:
      // 1 m3/h is 1000 L per 60 min, so L/min * 60 / 1000.
      return litresPerMinute * 0.06;
    case FlowUnit::LitresPerMinute:
    default:
      return litresPerMinute;
  }
}

/** What the header prints. Matches kFlowUnitOptions' labels, which are the same strings. */
inline const char* flowUnitLabel(FlowUnit unit) {
  switch (unit) {
    case FlowUnit::LitresPerSecond:      return "L/s";
    case FlowUnit::CubicMetresPerHour:   return "m3/h";
    case FlowUnit::LitresPerMinute:
    default:                             return "L/m";
  }
}

}  // namespace units
