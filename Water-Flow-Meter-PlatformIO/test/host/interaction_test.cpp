// Host tests for the interaction layer — the device harness.
//
// This links the REAL UiController, UiNavigator, UiScreenRouter, InteractionHandler,
// ButtonInputManager, ui_actions and ui_settings against the REAL 48-screen generated table,
// with only Preferences, M5StamPLC and ModbusMessage stubbed. It is not a reimplementation
// of the gesture contract; it is the firmware, driven by a fake button source.
//
// Before this existed, interaction_handler.cpp, ui_controller.cpp and ui_actions.cpp were
// compiled by NO test. Nine validation gates, 88 host checks and a clean `pio run` all
// passed while the display-off gesture failed to do the one thing its own comment says it
// does. That is the gap this file closes.
#include <Preferences.h>
#include <M5StamPLC.h>

#include <cstdio>
#include <cstring>
#include <vector>

#include "input/interaction_handler.h"
#include "led/led_controller.h"
#include "modbus/modbus_manager.h"
#include "modbus/sensor_types.h"
#include "ui/core/ui_controller.h"
#include "ui/core/ui_actions.h"
#include "ui/core/ui_module.h"
#include "ui/core/ui_pages.h"
#include "ui/core/ui_screen_router.h"
#include "ui/core/ui_settings.h"

// ── The two symbols the interaction layer reaches into ModbusManager for ────────────
//
// Defined here rather than by compiling modbus_manager.cpp, which would drag in eModbus.
// Recording them is the point: "waking the display must not write a config register" is
// only checkable if we can see the writes.
namespace harness {
struct HoldingWrite {
  uint16_t address;
  uint16_t value;
};
std::vector<HoldingWrite> writes;
}  // namespace harness

ModbusManager::ModbusManager(const ModbusDependencies& deps) : deps_(deps) {}

bool ModbusManager::applyHoldingWrite(uint16_t address, uint16_t value) {
  harness::writes.push_back({address, value});
  return true;
}

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-64s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

/** A device, wired the way firmware.cpp wires it, driven by the stub board. */
struct Device {
  Preferences prefs;
  LedController leds;
  UiController controller;
  ButtonInputManager buttons;
  plc::InteractionHandler interactions;
  ui::UiAssets assets = ui::loadGeneratedAssets();
  plc::LinkSettingsManager link;
  ui::SettingsAccess settings;
  uint16_t connectedBitmap = 0xFF;
  ModbusManager modbus{ModbusDependencies{}};
  ui::UiScreenRouter router{assets};

  uint32_t now = 1000;

  void boot() {
    harness::writes.clear();
    m5stamplc_stub::board().releaseAll();
    controller.begin(now);
    // firmware.cpp:327 seeds the navigator from the router; without it current() is null
    // and every gesture is a no-op. The harness must wire the device the same way the
    // device does, or it tests something that does not exist.
    controller.navigator().reset(router.screenForMode(UiMode::Info, UiPage::GlobalStatus));
    buttons.begin(now);
    plc::InteractionHandler::Dependencies deps;
    deps.screenRouter = &router;
    deps.actions = &ui::defaultActionRegistry();
    deps.ledController = &leds;
    deps.preferences = &prefs;
    deps.modbus = &modbus;
    // Without a SettingsAccess, beginEdit can never fire and an editor never opens — the
    // harness would silently exercise a device with no settings at all.
    settings.link = &link;
    settings.leds = &leds;
    settings.modbus = &modbus;
    settings.configs = configs;
    settings.connectedBitmap = &connectedBitmap;
    settings.sensorCount = plc::kNumSensors;
    deps.settings = &settings;
    interactions.begin(now, nullptr, deps);
    tick(0);
  }

  SensorData sensors[plc::kNumSensors] = {};
  SensorCharacteristics configs[plc::kNumSensors] = {};

  /**
   * One pass of the main loop.
   *
   * controller.update() matters as much as the input calls: updateIdleState lives inside it,
   * so a harness that only drives the buttons never advances the idle timer and would report
   * the inactivity path as working no matter what it did.
   */
  void tick(uint32_t deltaMs) {
    now += deltaMs;
    buttons.update(now);
    const auto result = interactions.update(now, buttons, controller);
    controller.update(now, sensors, configs, 0, 0xFF, 0.0, 0.0, 0.0f, leds, result.countdown);
  }

  void press(ButtonInputManager::Button b, bool down) {
    auto& board = m5stamplc_stub::board();
    switch (b) {
      case ButtonInputManager::Button::Up:    board.BtnA.pressed = down; break;
      case ButtonInputManager::Button::Down:  board.BtnB.pressed = down; break;
      case ButtonInputManager::Button::Enter: board.BtnC.pressed = down; break;
    }
  }

  /** A deliberate tap: down, a few polls, up. Comfortably under the 1.5 s long threshold. */
  void tap(ButtonInputManager::Button b) {
    press(b, true);
    tick(30);
    tick(30);
    press(b, false);
    tick(30);
  }

  /** Both UP and DOWN together, briefly — the display-off combo of §3. */
  void tapUpDown() {
    press(ButtonInputManager::Button::Up, true);
    press(ButtonInputManager::Button::Down, true);
    tick(30);
    tick(30);
    press(ButtonInputManager::Button::Up, false);
    press(ButtonInputManager::Button::Down, false);
    tick(30);
  }
};

/**
 * Walks to the config tree and descends until an editor opens.
 *
 * ENTER on P0 does not descend — P7 is the configuration entry (UiPage::EnterConfiguration),
 * so the ring has to be cycled there first. Tapping ENTER from P0 and expecting to arrive in
 * an editor is exactly the sort of wrong assumption a harness exists to expose.
 */
bool descendToAnEditor(Device& dev) {
  for (int i = 0; i < 16; ++i) {
    if (dev.controller.page() == UiPage::EnterConfiguration) break;
    dev.tap(ButtonInputManager::Button::Down);
  }
  for (int guard = 0; guard < 8 && !dev.controller.editor().active; ++guard) {
    dev.tap(ButtonInputManager::Button::Enter);
  }
  return dev.controller.editor().active;
}

void idleContractTests() {
  std::printf("[idle contract — Display_UI_Requirements §3]\n");

  Device dev;
  dev.boot();
  check(dev.controller.navigator().current() != nullptr,
        "the harness boots onto a real screen from the generated table");

  // Get somewhere deep, with an editor open if the tree allows it.
  const bool editing = descendToAnEditor(dev);
  const uint8_t depthBefore = dev.controller.navigator().depth();
  check(depthBefore > 0, "navigation reached a nested level");

  // §3: "UP + DOWN short — Display off, and reset navigation to P0. Works from any screen
  // at any depth." The handler's own comment says it "clears the navigation stack so the
  // display always wakes on P0 rather than wherever the operator happened to be."
  dev.tapUpDown();

  check(dev.controller.mode() == UiMode::Idle, "the combo enters idle");
  check(dev.controller.navigator().depth() == 0,
        "the combo resets navigation to the root, as §3 and the code comment both promise");
  check(dev.controller.page() == UiPage::GlobalStatus, "the combo returns to P0");
  if (editing) {
    check(!dev.controller.editor().active,
          "the combo abandons any open editor rather than leaving it live behind a dark screen");
  }

  // The severe consequence: with a live editor surviving idle, the first ENTER after waking
  // runs the commit handler and writes a config register the operator never confirmed.
  harness::writes.clear();
  dev.tap(ButtonInputManager::Button::Enter);
  check(harness::writes.empty(),
        "waking the display does not commit a Modbus write the operator never confirmed");
}

void idleTimeoutTests() {
  std::printf("\n[idle timeout takes the same path as the combo]\n");

  Device dev;
  dev.boot();
  descendToAnEditor(dev);
  check(dev.controller.navigator().depth() > 0, "nested before the timeout");

  // Two minutes of nothing. updateIdleState used to set mode_ directly, bypassing every
  // reset enterIdle performs — so the two ways of going idle behaved differently.
  for (int i = 0; i < 130; ++i) dev.tick(1000);

  check(dev.controller.mode() == UiMode::Idle, "the inactivity timeout enters idle");
  check(dev.controller.navigator().depth() == 0,
        "the timeout resets navigation too — one idle path, not two");
  check(!dev.controller.editor().active, "the timeout abandons any open editor");

  harness::writes.clear();
  dev.tap(ButtonInputManager::Button::Enter);
  check(harness::writes.empty(), "waking from a timeout does not commit a write either");
}

void navigationRingTests() {
  std::printf("\n[navigation over the real generated table]\n");

  Device dev;
  dev.boot();
  const auto* start = dev.controller.navigator().current();

  // Cycling the root ring must return to where it began, and must never leave the table.
  bool everNull = false;
  for (int i = 0; i < 20; ++i) {
    dev.tap(ButtonInputManager::Button::Down);
    if (dev.controller.navigator().current() == nullptr) everNull = true;
  }
  check(!everNull, "cycling the root ring never lands on a null screen");
  check(dev.controller.navigator().depth() == 0, "cycling siblings does not change depth");

  // Back to the start by going the other way the same number of steps.
  for (int i = 0; i < 20; ++i) dev.tap(ButtonInputManager::Button::Up);
  check(dev.controller.navigator().current() == start,
        "the root ring is symmetric: 20 down then 20 up returns to the starting screen");
}

}  // namespace

int main() {
  std::printf("interaction layer — real firmware, fake buttons\n\n");
  idleContractTests();
  idleTimeoutTests();
  navigationRingTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
