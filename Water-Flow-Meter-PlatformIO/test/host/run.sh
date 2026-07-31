#!/usr/bin/env bash
# Host-side firmware tests. No hardware, no PlatformIO, no container.
#
# The parts of the UI with exact, checkable requirements — the navigation graph and the
# acceleration tiers — are deliberately kept free of Arduino dependencies so they can be
# exercised with g++ in under a second. The alternative is flashing hardware to find out
# whether ENTER descends to the right screen, or whether a 700 ms hold steps by 5.
#
# This is also the seed of a fuller device emulator: everything here already runs the
# real generated screen table and the real navigator, so what is missing is a fake
# display and a fake button source rather than a reimplementation.
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

CXXFLAGS=(-std=gnu++17 -Wall -Wextra -I src)

g++ "${CXXFLAGS[@]}" -o "$OUT/nav_test" \
  test/host/nav_test.cpp \
  src/ui/core/ui_navigator.cpp \
  src/ui/generated/GeneratedUi.cpp

g++ "${CXXFLAGS[@]}" -o "$OUT/accel_test" \
  test/host/accel_test.cpp

g++ "${CXXFLAGS[@]}" -o "$OUT/led_test" \
  test/host/led_test.cpp

"$OUT/nav_test"
echo
"$OUT/accel_test"
echo
"$OUT/led_test"
