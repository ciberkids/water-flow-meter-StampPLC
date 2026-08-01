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

g++ "${CXXFLAGS[@]}" -o "$OUT/text_editor_test" \
  test/host/text_editor_test.cpp \
  src/ui/core/ui_text_editor.cpp

g++ "${CXXFLAGS[@]}" -I test/host/stubs -o "$OUT/sensor_state_test" \
  test/host/sensor_state_test.cpp \
  src/sensors/sensor_state_engine.cpp

# The device harness: the REAL interaction stack, driven by a fake button source.
# -I test/host/stubs applies only here, so the leaf tests above stay dependency-free.
g++ "${CXXFLAGS[@]}" -I test/host/stubs -o "$OUT/interaction_test" \
  test/host/interaction_test.cpp \
  src/ui/core/ui_controller.cpp \
  src/ui/core/ui_navigator.cpp \
  src/ui/core/ui_screen_router.cpp \
  src/ui/core/ui_settings_types.cpp \
  src/ui/core/ui_value_catalogue.cpp \
  src/ui/core/ui_text_editor.cpp \
  src/ui/core/ui_actions.cpp \
  src/ui/core/ui_bindings.cpp \
  src/ui/core/ui_settings.cpp \
  src/ui/core/ui_module.cpp \
  src/ui/theme/theme_palette.cpp \
  src/led/led_controller.cpp \
  src/input/button_input.cpp \
  src/input/interaction_handler.cpp \
  src/modbus/link_settings.cpp \
  src/ui/generated/GeneratedUi.cpp

"$OUT/nav_test"
echo
"$OUT/accel_test"
echo
"$OUT/led_test"
echo
"$OUT/text_editor_test"
echo
"$OUT/interaction_test"
echo
"$OUT/sensor_state_test"
echo

# The manifest the design tool validates against is generated from the firmware's own
# catalogues. Checking it here means a catalogue change that nobody regenerated fails in
# under a second, rather than surfacing as a design that passes every gate and then renders
# blank on the device.
tools/manifest_gen/run.sh --check
