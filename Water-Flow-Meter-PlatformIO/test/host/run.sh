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

# -Werror is deliberate. The manifest generator relies on -Wswitch to make "added a
# SettingKind without teaching the generator about it" a build failure rather than a silently
# incomplete manifest. Without -Werror that guarantee degrades to a warning nobody reads,
# which is the exact failure mode this project keeps rediscovering.
CXXFLAGS=(-std=gnu++17 -Wall -Wextra -Werror -I src)

g++ "${CXXFLAGS[@]}" -o "$OUT/nav_test" \
  test/host/nav_test.cpp \
  src/ui/core/ui_navigator.cpp \
  src/ui/generated/GeneratedUi.cpp

g++ "${CXXFLAGS[@]}" -o "$OUT/accel_test" \
  test/host/accel_test.cpp

# The clock's trust model. Arduino-free on purpose: "does a device whose RTC lost power ever show a
# timestamp" is a question that would otherwise need a bench and a power switch.
g++ "${CXXFLAGS[@]}" -o "$OUT/device_clock_test" \
  test/host/device_clock_test.cpp \
  src/time/device_clock.cpp

# The clock's one WIRING, as opposed to its logic: does a session reset arriving through
# ModbusManager actually date it? `ModbusDependencies::clock` is nullable and was null in both host
# tests that construct the struct, so noteSessionStart() was exercised by nothing — a nullable
# dependency that is null everywhere is indistinguishable from a no-op.
#
# The only host test that touches ModbusManager today DEFINES applyHoldingWrite itself to avoid
# linking eModbus, so it cannot cover this: it would assert against the harness. This one links the
# REAL modbus_manager.cpp, which is why -I test/host/stubs appears here and why the ModbusMessage
# stub had to grow the members the three frame handlers call.
g++ "${CXXFLAGS[@]}" -I test/host/stubs -o "$OUT/modbus_manager_clock_test" \
  test/host/modbus_manager_clock_test.cpp \
  src/modbus/modbus_manager.cpp \
  src/time/device_clock.cpp \
  src/led/led_controller.cpp \
  src/modbus/link_settings.cpp \
  src/net/net_settings.cpp \
  src/net/net_register_map.cpp

# What OFF_CMD_RESET_CALIBRATION keeps. The whole claim is about fields a command does NOT touch, so it
# has to run the real register arm against a channel carrying real readings — interaction_test.cpp
# defines applyHoldingWrite itself and would assert against its own stand-in. Same link line as the
# clock test above, and for the same reason.
g++ "${CXXFLAGS[@]}" -I test/host/stubs -o "$OUT/modbus_reset_calibration_test" \
  test/host/modbus_reset_calibration_test.cpp \
  src/modbus/modbus_manager.cpp \
  src/time/device_clock.cpp \
  src/led/led_controller.cpp \
  src/modbus/link_settings.cpp \
  src/net/net_settings.cpp \
  src/net/net_register_map.cpp

g++ "${CXXFLAGS[@]}" -o "$OUT/led_test" \
  test/host/led_test.cpp

# The loadable-menu format. Reads the pack the TS emitter produced from the real dataset and
# compares it against the generated table, so the two implementations of one binary layout are
# reconciled by execution rather than by review.
g++ "${CXXFLAGS[@]}" -o "$OUT/pack_test" \
  test/host/pack_test.cpp \
  src/ui/pack/ui_pack.cpp \
  src/ui/generated/GeneratedUi.cpp

# The network settings store and its register packing. Driven the way a Modbus master drives it:
# registers in arbitrary order, block writes across read-only regions, applies that must be refused.
g++ "${CXXFLAGS[@]}" -o "$OUT/net_settings_test" \
  test/host/net_settings_test.cpp \
  src/net/net_settings.cpp \
  src/net/net_register_map.cpp

# The firmware-owned Select Menu page. Exists so a pack that omits a selector cannot trap the
# operator, so the cases that matter are the awkward ones: an empty card, more packs than the page
# holds, and a pointer naming a pack that is not there.
g++ "${CXXFLAGS[@]}" -o "$OUT/pack_selector_test" \
  test/host/pack_selector_test.cpp \
  src/ui/pack/ui_pack_selector.cpp

# The shared-SPI handover. The requirement is "no visible artifacts", which is stronger than
# "no corruption" — so what is asserted is that the card never takes the bus between a
# startWrite() and its endWrite(), under interleavings no bench reproduces on demand.
g++ "${CXXFLAGS[@]}" -o "$OUT/spi_arbiter_test" \
  test/host/spi_arbiter_test.cpp \
  src/bus/spi_arbiter.cpp

# The boot selection ladder. Storage and the NVS counter are interfaces, so every failure rung —
# no card, a dangling pointer, "../.." in the pointer file, a pack that validates and then crashes
# the renderer — is reachable here and on no bench.
g++ "${CXXFLAGS[@]}" -o "$OUT/pack_loader_test" \
  test/host/pack_loader_test.cpp \
  src/ui/pack/ui_pack_loader.cpp \
  src/ui/pack/ui_pack.cpp \
  src/ui/generated/GeneratedUi.cpp

# ── The four network modules (N4, N5, N6, N8a) ────────────────────────────────
#
# Each is split so the POLICY half is Arduino-free and lands here, while the SDK-facing adapter
# (mqtt_transport_esp.h, the WifiRadio implementation, the WebServer routing) is not host-compiled.
# Without these four blocks the suite compiled and ran 68 checks while ~580 sat inert on disk —
# which is the exact shape of "a check that isn't checking" this project keeps rediscovering, so
# they are wired the moment they exist rather than when the firmware wiring is finished.

# The measurement core: rising-edge detection. Previously untested — it lived in firmware.cpp, which
# is in no link set here, so the logic producing every litre this product reports was the only
# subsystem without coverage. Header-only, so the test needs no companion .cpp.
g++ "${CXXFLAGS[@]}" -o "$OUT/pulse_counter_test" \
  test/host/pulse_counter_test.cpp

# N4 — the WiFi state machine: backoff ladder, AP window, and the provisioning hand-off.
g++ "${CXXFLAGS[@]}" -o "$OUT/wifi_manager_test" \
  test/host/wifi_manager_test.cpp \
  src/net/wifi_manager.cpp \
  src/net/net_settings.cpp

# 3B — the MQTT reconnect ladder. Not gated when it landed, which is the same defect as last round:
# 44 checks sat inert on disk while the suite reported green.
g++ "${CXXFLAGS[@]}" -o "$OUT/mqtt_reconnect_test" \
  test/host/mqtt_reconnect_test.cpp \
  src/net/mqtt_reconnect.cpp

# The esp_http_server task-placement policy: priority 5 / tskNO_AFFINITY by default would outrank the
# priority-2 sampler on whichever core it landed on.
g++ "${CXXFLAGS[@]}" -o "$OUT/httpd_task_policy_test" \
  test/host/httpd_task_policy_test.cpp

# N5 — the MQTT publish policy: cadence, the drop order, topic construction.
g++ "${CXXFLAGS[@]}" -o "$OUT/mqtt_publisher_test" \
  test/host/mqtt_publisher_test.cpp \
  src/net/mqtt_publisher.cpp

# N6 — Home Assistant discovery payloads, including the R4.4.8 worst-case buffer bound.
g++ "${CXXFLAGS[@]}" -o "$OUT/ha_discovery_test" \
  test/host/ha_discovery_test.cpp \
  src/net/ha_discovery.cpp

# N8a — the configuration portal's form generation, parsing, validation and escaping.
g++ "${CXXFLAGS[@]}" -o "$OUT/portal_form_test" \
  test/host/portal_form_test.cpp \
  src/net/portal_form.cpp \
  src/net/net_settings.cpp \
  src/net/net_register_map.cpp \
  src/ui/core/ui_settings_types.cpp

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
  src/ui/core/ui_actions.cpp \
  src/ui/core/ui_bindings.cpp \
  src/ui/core/ui_settings.cpp \
  src/ui/core/ui_module.cpp \
  src/ui/core/ui_renderer.cpp \
  src/ui/theme/theme_palette.cpp \
  src/time/device_clock.cpp \
  src/led/led_controller.cpp \
  src/input/button_input.cpp \
  src/input/interaction_handler.cpp \
  src/modbus/link_settings.cpp \
  src/bus/spi_arbiter.cpp \
  src/ui/pack/ui_pack_selector.cpp \
  src/net/net_settings.cpp \
  src/net/net_register_map.cpp \
  src/net/wifi_manager.cpp \
  src/ui/generated/GeneratedUi.cpp

"$OUT/nav_test"
echo
"$OUT/accel_test"
"$OUT/device_clock_test"
echo
"$OUT/modbus_manager_clock_test"
echo
"$OUT/modbus_reset_calibration_test"
echo
"$OUT/led_test"
echo
"$OUT/interaction_test"
echo
"$OUT/sensor_state_test"
echo
"$OUT/pack_test"
echo
"$OUT/pack_loader_test"
echo
"$OUT/spi_arbiter_test"
echo
"$OUT/pack_selector_test"
echo
"$OUT/net_settings_test"
echo
"$OUT/pulse_counter_test"
echo
"$OUT/wifi_manager_test"
echo
"$OUT/mqtt_publisher_test"
echo
"$OUT/mqtt_reconnect_test"
echo
"$OUT/httpd_task_policy_test"
echo
"$OUT/ha_discovery_test"
echo
"$OUT/portal_form_test"
echo

# The manifest the design tool validates against is generated from the firmware's own
# catalogues. Checking it here means a catalogue change that nobody regenerated fails in
# under a second, rather than surfacing as a design that passes every gate and then renders
# blank on the device.
tools/manifest_gen/run.sh --check
