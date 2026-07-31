#!/usr/bin/env bash
# Host-side firmware tests. No hardware, no PlatformIO, no container.
#
# UiNavigator and the generated screen table have no Arduino dependencies, so the
# real navigation graph the device will walk can be exercised with g++ in under a
# second. That matters because the alternative is flashing hardware to find out
# whether ENTER descends to the right screen.
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT="$(mktemp -d)"; trap 'rm -rf "$OUT"' EXIT

g++ -std=gnu++17 -Wall -Wextra -I src -o "$OUT/nav_test" \
  test/host/nav_test.cpp \
  src/ui/core/ui_navigator.cpp \
  src/ui/generated/GeneratedUi.cpp

"$OUT/nav_test"
