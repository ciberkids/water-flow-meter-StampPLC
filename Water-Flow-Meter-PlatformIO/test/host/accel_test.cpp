// Host-side checks for the parts of the editor with exact numeric requirements:
// the §5.4 acceleration tiers, and the clamp/cycle rules of §5.4.
#include "ui/core/ui_accel.h"
#include <cstdio>
#include <cstdint>
#include <string>

static int failures = 0;
static void check(bool ok, const std::string& what) {
  std::printf("  %-58s %s\n", what.c_str(), ok ? "ok" : "FAIL");
  if (!ok) failures++;
}

int main() {
  using ui::accelerationTier;

  std::printf("[acceleration tiers, Display_UI_Requirements 5.4]\n");
  // Tier 1: 0-700 ms -> x1 every 250 ms
  check(accelerationTier(0).multiplier == 1 && accelerationTier(0).intervalMs == 250,
        "0 ms          -> x1 / 250 ms");
  check(accelerationTier(699).multiplier == 1 && accelerationTier(699).intervalMs == 250,
        "699 ms        -> x1 / 250 ms (last of tier 1)");
  // Tier 2: 700 ms - 1.5 s -> x5 every 150 ms
  check(accelerationTier(700).multiplier == 5 && accelerationTier(700).intervalMs == 150,
        "700 ms        -> x5 / 150 ms (tier 2 boundary is inclusive)");
  check(accelerationTier(1499).multiplier == 5, "1499 ms       -> x5 (last of tier 2)");
  // Tier 3: > 1.5 s -> x25 every 150 ms
  check(accelerationTier(1500).multiplier == 25 && accelerationTier(1500).intervalMs == 150,
        "1500 ms       -> x25 / 150 ms");
  check(accelerationTier(60000).multiplier == 25, "60 s          -> x25 (no further tiers)");

  std::printf("\n[monotonicity]\n");
  bool monotonic = true;
  int32_t last = 0;
  for (uint32_t held = 0; held <= 3000; held += 10) {
    const auto tier = accelerationTier(held);
    if (tier.multiplier < last) monotonic = false;
    last = tier.multiplier;
  }
  check(monotonic, "the multiplier never decreases as the hold lengthens");

  bool intervalsSane = true;
  for (uint32_t held = 0; held <= 3000; held += 10) {
    const auto tier = accelerationTier(held);
    if (tier.intervalMs == 0 || tier.intervalMs > 250) intervalsSane = false;
  }
  check(intervalsSane, "every interval is in (0, 250] ms, so holding always advances");

  std::printf("\n%s (%d failures)\n", failures ? "FAILED" : "ALL PASSED", failures);
  return failures ? 1 : 0;
}
