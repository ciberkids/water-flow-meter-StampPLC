#pragma once

#include <cstdint>

namespace ui {

/**
 * Coarse-adjust acceleration, Display_UI_Requirements §5.4.
 *
 * | Hold duration    | Step | Interval |
 * | 0-700 ms         | x1   | 250 ms   |
 * | 700 ms - 1.5 s   | x5   | 150 ms   |
 * | > 1.5 s          | x25  | 150 ms   |
 *
 * Deliberately a free function in a dependency-free header so the tier boundaries can
 * be host-tested. The alternative — burying the arithmetic inside InteractionHandler,
 * which pulls in Preferences and the whole Arduino core — would make the one part with
 * exact numeric requirements the one part that cannot be checked without hardware.
 */
struct AccelTier {
  int32_t multiplier;
  uint32_t intervalMs;
};

constexpr uint32_t kAccelTier2Ms = 700;
constexpr uint32_t kAccelTier3Ms = 1500;

constexpr AccelTier accelerationTier(uint32_t heldMs) {
  if (heldMs < kAccelTier2Ms) {
    return {1, 250};
  }
  if (heldMs < kAccelTier3Ms) {
    return {5, 150};
  }
  return {25, 150};
}

}  // namespace ui
