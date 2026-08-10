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
 * Litres per second to litres per minute.
 *
 * TRANSITIONAL. Spec §2a moves the stored unit to L/min, at which point flow is already in the
 * unit the panel, the q_max setting and the Nyquist limit all use, and this function has no
 * callers left. It exists so that the display bindings which must show L/min today do the
 * conversion in ONE place rather than each carrying its own `* 60`, and so that the storage move
 * is a deletion rather than a hunt.
 */
inline constexpr float flowLpsToLpm(float litresPerSecond) {
  return litresPerSecond * 60.0f;
}

/** Litres per second to litres per minute, for the double-valued aggregate. */
inline constexpr double flowLpsToLpm(double litresPerSecond) {
  return litresPerSecond * 60.0;
}

}  // namespace units
