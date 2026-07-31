// Host-side checks for the LED override patterns, RGB_LED_Behavior.md §3.4-§3.5.
#include "led/led_patterns.h"
#include <cstdio>
#include <string>

using namespace plc;
static int failures = 0;
static void check(bool ok, const std::string& what) {
  std::printf("  %-60s %s\n", what.c_str(), ok ? "ok" : "FAIL");
  if (!ok) failures++;
}

int main() {
  std::printf("[3.4 boot snake]\n");
  check(bootSnakeState(0).red && !bootSnakeState(0).green && !bootSnakeState(0).blue,
        "step 0 is red only");
  check(!bootSnakeState(150).red && bootSnakeState(150).green, "step 1 is green only");
  check(!bootSnakeState(300).green && bootSnakeState(300).blue, "step 2 is blue only");
  check(bootSnakeState(450).red, "step 3 wraps back to red");
  bool exactlyOne = true;
  for (uint32_t t = 0; t < 3000; t += 7) {
    const auto s = bootSnakeState(t);
    if ((int)s.red + (int)s.green + (int)s.blue != 1) exactlyOne = false;
  }
  check(exactlyOne, "exactly one channel is lit at any instant (a rotation, not a blend)");

  std::printf("\n[3.4 stalled boot]\n");
  check(bootStalledState(0).red && !bootStalledState(0).green && !bootStalledState(0).blue,
        "stalled is red only, never green or blue");
  check(!bootStalledState(500).red && bootStalledState(1000).red,
        "stalled blinks at 500 ms, so it is distinguishable from the snake");

  std::printf("\n[3.5 reset ramp period]\n");
  check(resetRampPeriodMs(3000, 3000) == kRampSlowestMs, "3 s countdown starts at 600 ms");
  check(resetRampPeriodMs(30000, 30000) == kRampSlowestMs, "30 s countdown starts at 600 ms");
  check(resetRampPeriodMs(0, 3000) == kRampFastestMs, "3 s countdown ends at 60 ms");
  check(resetRampPeriodMs(0, 30000) == kRampFastestMs, "30 s countdown ends at 60 ms");

  // The property the fraction-based formula exists for: identical progress gives an
  // identical period regardless of the countdown's length.
  bool sameShape = true;
  for (int pct = 0; pct <= 100; pct += 5) {
    const uint32_t shortMs = resetRampPeriodMs(3000u * pct / 100, 3000);
    const uint32_t longMs = resetRampPeriodMs(30000u * pct / 100, 30000);
    if (shortMs != longMs) { sameShape = false;
      std::printf("      %3d%%: 3s=%u 30s=%u\n", pct, shortMs, longMs); }
  }
  check(sameShape, "3 s and 30 s ramps have the same period at the same fraction");

  bool monotonic = true;
  uint32_t prev = kRampSlowestMs + 1;
  for (int32_t rem = 30000; rem >= 0; rem -= 250) {
    const uint32_t p = resetRampPeriodMs((uint32_t)rem, 30000);
    if (p > prev) monotonic = false;
    prev = p;
  }
  check(monotonic, "the period only ever shortens as the countdown runs down");

  bool inRange = true;
  for (uint32_t total : {1500u, 3000u, 30000u}) {
    for (uint32_t rem = 0; rem <= total; rem += 37) {
      const uint32_t p = resetRampPeriodMs(rem, total);
      if (p < kRampFastestMs || p > kRampSlowestMs) inRange = false;
    }
  }
  check(inRange, "the period stays within [60, 600] ms for 1.5 s, 3 s and 30 s");
  check(resetRampPeriodMs(99999, 3000) == kRampSlowestMs, "remaining > total clamps, no overflow");
  check(resetRampPeriodMs(1000, 0) == kRampFastestMs, "a zero total does not divide by zero");

  std::printf("\n[3.5 states]\n");
  const auto accepted = resetAcceptedState();
  check(accepted.red && accepted.green && accepted.blue, "acceptance is solid white, all three on");
  bool unison = true;
  for (uint32_t t = 0; t < 5000; t += 11) {
    const auto s = resetRampState(t, 1500, 3000);
    if (!(s.red == s.green && s.green == s.blue)) unison = false;
  }
  check(unison, "the ramp flashes all three in unison, so it reads white not coloured");

  std::printf("\n%s (%d failures)\n", failures ? "FAILED" : "ALL PASSED", failures);
  return failures ? 1 : 0;
}
