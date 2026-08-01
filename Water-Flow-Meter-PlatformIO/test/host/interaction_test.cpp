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
#include "modbus/register_map.h"
#include "modbus/sensor_types.h"
#include "ui/core/ui_bindings.h"
#include "ui/core/ui_controller.h"
#include "ui/core/ui_actions.h"
#include "ui/core/ui_module.h"
#include "ui/core/ui_pages.h"
#include "ui/core/ui_renderer.h"
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
  plc::WriteOrigin origin;
};
std::vector<HoldingWrite> writes;

// The link half of ModbusManager::applyHoldingWrite is what defect 7 lives in, so the
// harness has to reproduce it rather than swallow it: with the writes merely recorded, a
// test of the rollback arming would pass no matter what the firmware did. These two
// globals stand in for the deps_ wiring that modbus_manager.cpp would provide — a plain
// pointer rather than a designated initialiser on ModbusDependencies, which is C++20.
plc::LinkSettingsManager* link = nullptr;
uint32_t nowMs = 0;

// firmware.cpp's performFactoryReset() wipes NVS but schedules no reboot — the reboot is a
// separate `restartScheduled` -> esp_restart() step. A harness that passes nullptr here
// cannot tell "wiped and rebooted" from "wiped and left running on stale RS485 settings",
// which is the whole of §4.3's "reboot, no toast".
std::size_t factoryResets = 0;
void countFactoryReset() { ++factoryResets; }
}  // namespace harness

ModbusManager::ModbusManager(const ModbusDependencies& deps) : deps_(deps) {}

bool ModbusManager::applyHoldingWrite(uint16_t address,
                                      uint16_t value,
                                      plc::WriteOrigin origin) {
  harness::writes.push_back({address, value, origin});
  if (!harness::link) {
    return true;
  }
  // Mirrors modbus_manager.cpp's 40-43 stage / 44 commit block, minus the register-bank
  // publish the harness has no bank for.
  if (address == plc::REG_LINK_SLAVE_ID || address == plc::REG_LINK_BAUD_INDEX ||
      address == plc::REG_LINK_PARITY || address == plc::REG_LINK_STOP_BITS) {
    return harness::link->stage(address, value, origin);
  }
  if (address == plc::REG_LINK_APPLY) {
    if (value != plc::LinkSettingsManager::kApplyMagic) {
      return false;
    }
    harness::link->apply(harness::nowMs, origin);
    return true;
  }
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
  /** What the last tick()'s InteractionHandler::update() returned. */
  plc::InteractionResult lastResult{};
  ModbusManager modbus{ModbusDependencies{}};
  ui::UiScreenRouter router{assets};
  // The renderer is part of the device, so it belongs in the device harness. Leaving it out
  // is what let a repaint cadence gated on an unreachable mode survive every gate.
  UiRenderer renderer;
  ui::UiBindingResolver bindings;

  uint32_t now = 1000;

  void boot() {
    harness::writes.clear();
    harness::link = &link;
    harness::nowMs = now;
    m5stamplc_stub::board().releaseAll();
    m5stamplc_stub::board().Display.reset();
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
    harness::factoryResets = 0;
    interactions.begin(now, harness::countFactoryReset, deps);
    // firmware.cpp wires the renderer with the theme, the router and a binding resolver that
    // can see the settings and the controller; anything less and every config binding falls
    // back to its placeholder.
    bindings.bindSettings(&settings, &controller);
    renderer.applyTheme(assets.palette);
    renderer.bindScreenRouter(&router);
    renderer.bindBindingResolver(&bindings);
    renderer.begin();
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
    harness::nowMs = now;
    // millis() must advance with the simulated clock: drawFlowDots() reads it directly.
    m5stamplc_stub::clockMs() = now;
    buttons.update(now);
    const auto result = interactions.update(now, buttons, controller);
    // firmware.cpp:512 reboots on this flag and firmware.cpp:466 latches the solid-white
    // acknowledgement on it, so a test of a destructive action has to see it. Discarding
    // the whole result is what let the un-rebooted factory reset look like a success.
    lastResult = result;
    controller.update(now, sensors, configs, 0, 0xFF, 0.0, 0.0, 0.0f, leds, result.countdown);
    // firmware.cpp:446 — the renderer runs on every pass of the logic loop and decides for
    // itself whether to paint. That decision is what the cadence tests below measure.
    renderer.update(now, controller.context());
  }

  uint32_t frames() const { return m5stamplc_stub::board().Display.fillScreens; }
  void resetFrames() { m5stamplc_stub::board().Display.reset(); }

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

  /**
   * A deliberate hold: down for `ms`, then release.
   *
   * Distinct from tap() because the two are read differently. 330 ms is past the 250 ms
   * first acceleration interval of §5.4 but well under the 1.5 s long-press threshold, so
   * a navigation screen must still see one ordinary short press on release. That window is
   * exactly where an editor that should not be open steals the event.
   */
  void hold(ButtonInputManager::Button b, uint32_t ms) {
    press(b, true);
    for (uint32_t elapsed = 0; elapsed < ms; elapsed += 30) {
      tick(30);
    }
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

/**
 * Repaint cadence — Display_UI_Requirements §7: "Button events must be acknowledged within
 * 100 ms to feel immediate."
 *
 * UiRenderer picks its interval from `context.mode`: 1000 ms for Info, 80 ms otherwise. The
 * only other awake mode is Configuration, and the 0.2 header note (line 12) records that it
 * "was never implemented" — nothing calls setMode with it (ui_actions.cpp:117 passes Info,
 * the only call site). So the fast branch is dead code and every awake screen, editor and
 * countdown included, is painted once a second.
 *
 * These checks are deliberately about frames per unit of simulated time, not pixels: the
 * cadence is the part with a requirement attached.
 */
void repaintCadenceTests() {
  std::printf("\n[repaint cadence — Display_UI_Requirements §7]\n");

  // A telemetry page with nobody touching the buttons genuinely has nothing to say faster
  // than 1 Hz, and a full-panel clear it does not need is 64,800 bytes of SPI traffic.
  Device idlePage;
  idlePage.boot();
  idlePage.resetFrames();
  for (int i = 0; i < 40; ++i) idlePage.tick(25);  // one second of hands-off time on P0
  check(idlePage.frames() <= 2, "a static info page stays on the 1 Hz telemetry cadence");

  // An open value editor is the case the operator feels. §5.4's acceleration ramp steps the
  // pending value every 150 ms at the top tier; at one repaint per second the operator sees
  // roughly every seventh step, so a held button looks like it dropped most of its input.
  Device editing;
  editing.boot();
  check(descendToAnEditor(editing), "an editor opened, so there is something to repaint");
  editing.resetFrames();
  for (int i = 0; i < 40; ++i) editing.tick(25);
  // 80 ms sampled on a 25 ms tick gives 10 frames per second here; the 1 Hz path gives 1.
  check(editing.frames() >= 8, "an open editor repaints many times a second, not once");

  // Depth 0 is not exempt: tapping DOWN on the info ring changes the screen, and the new
  // page has to appear now rather than at the next 1 Hz boundary.
  Device paging;
  paging.boot();
  const auto* before = paging.controller.navigator().current();
  paging.resetFrames();
  paging.tap(ButtonInputManager::Button::Down);  // 90 ms of loop passes inside tap()
  check(paging.controller.navigator().current() != before, "the tap moved the ring");
  check(paging.frames() >= 1,
        "a page change is painted within the tap, not at the next 1 Hz boundary");

  // ...and having paged, the device must fall back to the slow cadence. A permanently fast
  // repaint would be a full-screen clear at 12.5 Hz for a screen that is not changing.
  paging.resetFrames();
  for (int i = 0; i < 40; ++i) paging.tick(25);
  check(paging.frames() <= 2, "after paging it returns to 1 Hz rather than staying fast");
}

// ── DEFECT 2: the value editor opened on the config LIST pages ─────────────────────────
//
// §5.1 makes Configuration a tree whose every level is a ring of sibling pages: UP/DOWN move
// within the level, ENTER-short descends. §5.4 then gives every editable setting "its own
// screen, showing the label, the pending value (highlighted), the unit, and the currently
// saved value". So a list page (C1..C6, S1..S4) shows only the value in force and descends;
// the screen one level below it is the editor.
//
// settingOnScreen() matched any element whose bindingId is in the setting catalogue, and a
// list page displays its own saved value — so descending onto the LIST called beginEdit().
// The damage does not stop at that page: goToSibling() never touches editor state, so the
// spurious editor leaks along the whole sibling ring, and handleEditorRepeat owns UP/DOWN for
// as long as it lives. Any press past the 250 ms tier-1 interval of §5.4 is consumed as an
// adjust step and its release short-press discarded, so paging simply stops.

const char* currentId(const Device& dev) {
  const auto* screen = dev.controller.navigator().current();
  return (screen && screen->id) ? screen->id : "<null>";
}

bool onScreen(const Device& dev, const char* id) {
  return std::strcmp(currentId(dev), id) == 0;
}

/** Pages the info ring to P7 and descends once, landing on the config root ring at C1. */
bool walkToConfigRoot(Device& dev) {
  for (int i = 0; i < 16 && dev.controller.page() != UiPage::EnterConfiguration; ++i) {
    dev.tap(ButtonInputManager::Button::Down);
  }
  if (dev.controller.page() != UiPage::EnterConfiguration) {
    return false;
  }
  dev.tap(ButtonInputManager::Button::Enter);
  return onScreen(dev, "config-c1-modbus-id");
}

void configListPagingTests() {
  std::printf("\n[config list pages are not editors — §5.1, §5.4]\n");

  Device dev;
  dev.boot();
  check(walkToConfigRoot(dev), "ENTER-short on P7 descends onto the config root ring at C1");
  check(dev.controller.navigator().depth() == 1, "C1 sits one level below the info ring");
  check(!dev.controller.editor().active,
        "descending onto the C1 LIST page does not open a value editor");

  // §5.1: "UP/DOWN move within the current level." A 330 ms press is past the 250 ms first
  // acceleration interval, so a live editor eats it; a navigation screen must page.
  static constexpr const char* kRing[] = {"config-c2-baud-rate", "config-c3-parity",
                                          "config-c4-stop-bits", "config-c5-led-pulse-vol",
                                          "config-c6-led-pulse-period"};
  bool paged = true;
  bool everEditing = false;
  for (const char* expected : kRing) {
    dev.hold(ButtonInputManager::Button::Down, 330);
    if (!onScreen(dev, expected)) {
      paged = false;
      std::printf("    expected %s, got %s\n", expected, currentId(dev));
    }
    if (dev.controller.editor().active) everEditing = true;
  }
  check(paged, "a 330 ms DOWN pages C1 -> C2 -> C3 -> C4 -> C5 -> C6");
  check(!everEditing, "no editor is live anywhere along the config root ring");
  check(dev.controller.navigator().depth() == 1, "paging siblings did not change depth");
  check(harness::writes.empty(), "paging the config ring wrote no Modbus register");
}

void configEditorDescentTests() {
  std::printf("\n[one more level down IS the editor — §5.4, §5.7]\n");

  Device dev;
  dev.boot();
  check(walkToConfigRoot(dev), "at C1");
  dev.tap(ButtonInputManager::Button::Enter);
  check(onScreen(dev, "config-c1-modbus-id-edit"),
        "ENTER-short on C1 descends onto its derived editor screen (§5.7)");
  check(dev.controller.navigator().depth() == 2, "the editor is one level below its list page");
  const auto& editor = dev.controller.editor();
  check(editor.active, "descending onto C1.V DOES open the value editor");
  check(editor.setting != nullptr &&
            std::strcmp(editor.setting->bindingId, "config.modbusSlaveId") == 0,
        "the editor is bound to the setting its screen declares");

  // §5.4: a 330 ms hold is one tier-1 step, and here that is the whole point of the screen.
  const int32_t before = editor.pending;
  dev.hold(ButtonInputManager::Button::Up, 330);
  check(editor.pending != before, "on the editor screen a 330 ms UP does adjust the value");
}

void sensorEditorDescentTests() {
  std::printf("\n[the sensor branch: L2 list, L3 settings, L4 editor — §5.1]\n");

  Device dev;
  dev.boot();
  check(walkToConfigRoot(dev), "at C1");
  for (int i = 0; i < 8 && !onScreen(dev, "config-c7-sensor-select"); ++i) {
    dev.tap(ButtonInputManager::Button::Down);
  }
  check(onScreen(dev, "config-c7-sensor-select"), "paged along the config root ring to C7");

  dev.tap(ButtonInputManager::Button::Enter);
  check(onScreen(dev, "config-sensor-1"), "C7 descends onto the sensor list");
  check(!dev.controller.editor().active,
        "C7 carries no value, so the sensor list opens no editor (§5.1)");

  dev.tap(ButtonInputManager::Button::Enter);
  check(onScreen(dev, "config-s1-connected"), "the sensor list descends onto the S ring");
  check(!dev.controller.editor().active,
        "descending onto the S1 LIST page does not open a value editor either");

  // The S ring pages the same way the C ring does, and for the same reason.
  dev.hold(ButtonInputManager::Button::Down, 330);
  check(onScreen(dev, "config-s2-multiplier"), "a 330 ms DOWN pages S1 -> S2");
  dev.hold(ButtonInputManager::Button::Up, 330);
  check(onScreen(dev, "config-s1-connected"), "a 330 ms UP pages S2 -> S1 again");

  dev.tap(ButtonInputManager::Button::Enter);
  check(onScreen(dev, "config-s1-connected-edit"), "ENTER-short on S1 descends onto S1.V");
  const auto& editor = dev.controller.editor();
  check(editor.active, "descending onto S1.V DOES open the value editor");
  check(editor.setting != nullptr &&
            std::strcmp(editor.setting->bindingId, "config.sensor.connected") == 0,
        "the editor is bound to config.sensor.connected");
  // §5.1: the sensor a setting applies to is the L2 page the operator descended from.
  check(dev.controller.navigator().sensorIndex() == 1,
        "the editor is scoped to sensor 1, the list page it descended from");
  check(editor.sensorIndex == 1, "and the editor records that same sensor index");
}

bool declaresBinding(const ui_exporter::Screen& screen, const char* bindingId) {
  for (std::size_t i = 0; i < screen.elementCount; ++i) {
    const char* binding = screen.elements[i].bindingId;
    if (binding && std::strcmp(binding, bindingId) == 0) {
      return true;
    }
  }
  return false;
}

const ui_exporter::Flow* flowWithAction(const ui_exporter::Screen& screen, const char* actionId) {
  for (std::size_t i = 0; i < screen.flowCount; ++i) {
    const auto& flow = screen.flows[i];
    if (flow.actionId && std::strcmp(flow.actionId, actionId) == 0) {
      return &flow;
    }
  }
  return nullptr;
}

const ui_exporter::Screen* screenById(const char* id) {
  if (!id) return nullptr;
  for (std::size_t i = 0; i < ui_exporter::kGeneratedScreenCount; ++i) {
    const auto& screen = ui_exporter::kGeneratedScreens[i];
    if (screen.id && std::strcmp(screen.id, id) == 0) {
      return &screen;
    }
  }
  return nullptr;
}

/**
 * The dataset fact the fix reads, asserted rather than trusted.
 *
 * "This screen is a value editor" is decided by "it declares a config.editor.pending
 * element" — §5.4's highlighted pending value, which only an editor shows. If a designer
 * deleted that element from an editor screen, editing would silently stop working and every
 * export gate would still pass. §5.7 says the exporter can mechanically verify that every
 * setting page has its editor; this is that check, run against the emitted table rather than
 * the JSON, because the emitter is what the firmware actually sees.
 */
void editorDatasetInvariantTests() {
  std::printf("\n[the dataset fact the fix reads — §5.4, §5.7]\n");

  std::size_t editors = 0;
  std::size_t settingListPages = 0;
  bool pendingMatchesCommit = true;
  bool everyListPageHasItsEditor = true;

  for (std::size_t i = 0; i < ui_exporter::kGeneratedScreenCount; ++i) {
    const auto& screen = ui_exporter::kGeneratedScreens[i];
    const bool pending = declaresBinding(screen, "config.editor.pending");
    const bool commits = flowWithAction(screen, "config.action.value.commit") != nullptr;
    if (pending != commits) {
      pendingMatchesCommit = false;
      std::printf("    %s: pending=%d commit-flow=%d\n", screen.id, pending ? 1 : 0,
                  commits ? 1 : 0);
    }
    if (pending) {
      ++editors;
      continue;
    }

    // A catalogue setting displayed by a screen that is not itself an editor is a list page:
    // its ENTER-short must descend onto a screen that IS one.
    bool carriesSetting = false;
    for (std::size_t e = 0; e < screen.elementCount; ++e) {
      if (ui::findSetting(screen.elements[e].bindingId)) {
        carriesSetting = true;
        break;
      }
    }
    if (!carriesSetting) continue;
    ++settingListPages;
    const auto* descend = flowWithAction(screen, "ui.action.nav.descend");
    const auto* target = descend ? screenById(descend->targetScreenId) : nullptr;
    if (!target || !declaresBinding(*target, "config.editor.pending")) {
      everyListPageHasItsEditor = false;
      std::printf("    %s has no editor screen below it\n", screen.id);
    }
  }

  check(editors == 10, "exactly ten screens declare config.editor.pending — the ten editors");
  check(pendingMatchesCommit,
        "a screen shows a pending value if and only if it has a commit flow");
  check(settingListPages == 10, "ten list pages display a saved setting without being editors");
  check(everyListPageHasItsEditor,
        "every setting list page's ENTER-short descends onto a real editor (§5.7)");
}

/**
 * Defect 7 — the link apply/rollback protocol, Project_document.md §4.1.1.
 *
 * Two things were wrong. A display-originated change armed the 60 s rollback even though
 * rollback's only confirmation signal is an incoming Modbus frame, so on a unit with no
 * master attached every link change made at the display silently reverted a minute later.
 * And `noteValidFrame` confirmed on ANY frame, so once a slave-ID change had been applied,
 * whatever still answered on the OLD id kept confirming a link the master had not
 * followed — making rollback unable to fire in precisely the case it exists for.
 */
void linkApplyProtocolTests() {
  std::printf("\n[link apply protocol — Project_document.md §4.1.1]\n");

  // ── The origin guard, driven through the real display path ────────────────────────
  Device dev;
  dev.boot();
  const auto* slaveId = ui::findSetting("config.modbusSlaveId");
  check(slaveId != nullptr, "the settings catalogue exposes config.modbusSlaveId");
  if (!slaveId) return;

  const uint8_t bootId = dev.link.live().slaveId;
  check(ui::writeSetting(*slaveId, 0, bootId + 6, dev.settings),
        "the display can commit a slave id");
  check(dev.link.live().slaveId == bootId + 6,
        "a display commit reaches the LIVE settings, not just the staged ones");
  check(dev.link.revision() == 1, "a display commit still bumps the revision (§4.1.1 step 3)");
  check(!dev.link.awaitingConfirmation(),
        "a display-originated link change does NOT arm the 60 s rollback");

  // The severe consequence of the old behaviour: no master, so no frame, so the value the
  // operator watched take effect reverts on its own after a minute.
  for (int i = 0; i < 70; ++i) dev.tick(1000);
  check(!dev.link.rollbackDue(dev.now),
        "and it is still not due 70 s later — a local change is not silently reverted");
  check(dev.link.live().slaveId == bootId + 6, "the display's value survives the window");

  check(harness::writes.size() >= 2, "the display path went through applyHoldingWrite");
  if (harness::writes.size() >= 2) {
    const auto& apply = harness::writes.back();
    check(apply.address == plc::REG_LINK_APPLY &&
              apply.value == plc::LinkSettingsManager::kApplyMagic,
          "it still commits via register 44 and 0x5AA5, not a UI-only side door");
    check(apply.origin == plc::WriteOrigin::Display,
          "and it tags the write WriteOrigin::Display");
  }

  // ── A bus-originated change keeps the full protocol ───────────────────────────────
  Device bus;
  bus.boot();
  const uint8_t oldId = bus.link.live().slaveId;
  const uint8_t newId = static_cast<uint8_t>(oldId + 5);
  check(bus.link.stage(plc::REG_LINK_SLAVE_ID, newId), "a bus write stages register 40");
  check(bus.link.live().slaveId == oldId, "staging leaves the live link untouched (§4.1.1 step 1)");
  check(bus.link.apply(bus.now), "0x5AA5 commits");
  check(bus.link.awaitingConfirmation(), "a bus apply DOES arm rollback (§4.1.1 step 4)");

  // The heart of the defect: a frame on the pre-apply id proves nothing.
  bus.link.noteValidFrame(bus.now + 1000, oldId);
  check(bus.link.awaitingConfirmation(),
        "a frame on the OLD slave id must not confirm — that is the failure, not the proof");
  for (int i = 0; i < 61; ++i) {
    bus.tick(1000);
    bus.link.noteValidFrame(bus.now, oldId);  // a master still polling the stale id
  }
  check(bus.link.rollbackDue(bus.now),
        "so rollback becomes due after 60 s despite continuous traffic on the old id");
  check(bus.link.rollback(), "and the rollback restores something");
  check(bus.link.live().slaveId == oldId, "the pre-apply slave id is restored");
  check(!bus.link.awaitingConfirmation(), "the window closes after a rollback");

  // And a frame on the NEW id does confirm, so a working reconfiguration is not reverted.
  Device ok;
  ok.boot();
  const uint8_t okNewId = static_cast<uint8_t>(ok.link.live().slaveId + 3);
  ok.link.stage(plc::REG_LINK_SLAVE_ID, okNewId);
  ok.link.apply(ok.now);
  ok.link.noteValidFrame(ok.now + 500, okNewId);
  check(!ok.link.awaitingConfirmation(), "a frame on the NEW slave id confirms the apply");
  for (int i = 0; i < 61; ++i) ok.tick(1000);
  check(!ok.link.rollbackDue(ok.now), "a confirmed apply is never rolled back");
  check(ok.link.live().slaveId == okNewId, "the confirmed slave id stands");

  // A display change on top of an armed bus window must DISARM it, not inherit it.
  Device mixed;
  mixed.boot();
  mixed.link.stage(plc::REG_LINK_SLAVE_ID, 40);
  mixed.link.apply(mixed.now);
  check(mixed.link.awaitingConfirmation(), "armed by the bus apply");
  check(ui::writeSetting(*slaveId, 0, 41, mixed.settings), "the operator then changes it locally");
  check(!mixed.link.awaitingConfirmation(),
        "a display change clears an already-armed window rather than reverting to a value "
        "no longer on screen");

  // ── A pending bus stage must not ride along on the display's unprotected commit ────
  //
  // The nastier version of the case above: the master staged a baud change and deliberately
  // did NOT commit it. If the display's commit carried it along, a bus-originated change
  // would take effect with no rollback window — orphaning the bus with the safety net that
  // exists for exactly that change disarmed.
  Device pending;
  pending.boot();
  const uint8_t liveBaud = pending.link.live().baudIndex;
  const uint8_t otherBaud = static_cast<uint8_t>(liveBaud == 7 ? 0 : 7);
  check(pending.link.stage(plc::REG_LINK_BAUD_INDEX, otherBaud),
        "a master stages a baud change");
  check(pending.link.staged().baudIndex == otherBaud, "and it is staged");
  check(pending.link.live().baudIndex == liveBaud, "but not live — no 0x5AA5 was sent");
  check(ui::writeSetting(*slaveId, 0, 44, pending.settings),
        "the operator then commits an unrelated slave id from the display");
  check(pending.link.live().slaveId == 44, "the display's own field takes effect");
  check(pending.link.live().baudIndex == liveBaud,
        "the master's uncommitted baud does NOT ride along on the unprotected local commit");
  check(pending.link.staged().baudIndex == liveBaud,
        "and the pending stage is dropped, so a master reading 41 back sees it (§4.1.1 step 1)");
}

// ── DEFECT 1: confirm screens could never be confirmed ─────────────────────────────────
//
// §4.3 gives P2/P3 a `Reset totals?` confirm screen held for 3 s and P4/P5/P6 a
// `Reset session?` one held for 1.5 s. The dataset states each guard as ONE flow on the
// confirm screen itself: a Timeout flow whose emitted `button` is Enter — cppEmitter.ts
// carries `holdButton` there (NF-20260730-01 §3.8), and `gesture` is filler on a
// non-button trigger. See GeneratedUi.cpp:1198/1219/1240.
//
// armHoldCountdown looked for something else entirely: a Button+Enter+Long flow with a
// targetScreenId and no actionId, reading the duration off the TARGET screen. No screen in
// the 48 has that shape, so no countdown ever armed and neither reset could be performed
// on the device at all. These tests hold ENTER and require the Modbus command.

/** Pages the info ring to `page`, then reports whether it got there. */
bool walkToInfoPage(Device& dev, UiPage page) {
  for (int i = 0; i < 16 && dev.controller.page() != page; ++i) {
    dev.tap(ButtonInputManager::Button::Down);
  }
  return dev.controller.page() == page;
}

bool wroteOnce(uint16_t address) {
  return harness::writes.size() == 1 && harness::writes[0].address == address &&
         harness::writes[0].value == 1;
}

void confirmCountdownTests() {
  std::printf("\n[confirm screens can be confirmed — §4.3]\n");

  Device dev;
  dev.boot();
  check(walkToInfoPage(dev, UiPage::CumulativeLiters), "paged the info ring to P2");
  dev.tap(ButtonInputManager::Button::Enter);
  check(onScreen(dev, "confirm-reset-totals"), "ENTER-short on P2 opens the confirm screen");
  check(dev.controller.navigator().depth() == 1, "the confirm screen is one level down");

  // Hold ENTER through the 3 s guard, watching the overlay on the way.
  harness::writes.clear();
  dev.press(ButtonInputManager::Button::Enter, true);
  for (int i = 0; i < 10; ++i) dev.tick(50);  // ~500 ms in
  // The discriminating assertion: arming at the 1.5 s long-press threshold could never
  // show a countdown here, and could never show 3 at all — only half of 3->0.
  check(dev.controller.context().countdownActive,
        "the countdown is on screen from the moment ENTER goes down, not at 1.5 s");
  check(dev.controller.context().countdownSeconds == 3,
        "and it starts at the full 3 s the dataset declares");
  check(dev.controller.context().countdownScreenId != nullptr &&
            std::strcmp(dev.controller.context().countdownScreenId, "confirm-reset-totals") == 0,
        "the countdown draws the confirm screen itself, not a separate overlay");
  check(harness::writes.empty(), "nothing is written while the countdown is still running");

  for (int i = 0; i < 20; ++i) dev.tick(50);  // ~1500 ms in
  check(dev.controller.context().countdownSeconds == 2 ||
            dev.controller.context().countdownSeconds == 1,
        "the countdown is still counting down at the 1.5 s gesture boundary");

  for (int i = 0; i < 40; ++i) dev.tick(50);  // past 3000 ms
  check(wroteOnce(plc::REG_MASTER_RESET_ALL_MEASURED),
        "holding ENTER for 3 s issues the reset-all-measured command exactly once (§4.3 note 3)");
  // (d) ascend-after-dispatch: the reset is worthless if the operator is left staring at
  // "RESET TOTALS?" with no way to tell it happened.
  check(dev.controller.navigator().depth() == 0 && onScreen(dev, "info-p2-cumulative-liters"),
        "and the confirm screen is unwound back to the page it was opened from");
  check(!dev.controller.context().countdownActive, "the overlay is gone once the action fired");

  // Still holding ENTER after the action fired: releasing must not re-open the confirm
  // screen or write a second time.
  dev.press(ButtonInputManager::Button::Enter, false);
  dev.tick(30);
  dev.tick(30);
  check(wroteOnce(plc::REG_MASTER_RESET_ALL_MEASURED),
        "releasing after the action does not write again");
  check(onScreen(dev, "info-p2-cumulative-liters"),
        "nor does it re-descend into the confirm screen");
}

void confirmSessionCountdownTests() {
  std::printf("\n[a 1.5 s guard is a long press, not a countdown — §4.3, §3]\n");

  Device dev;
  dev.boot();
  check(walkToInfoPage(dev, UiPage::SessionLiters), "paged the info ring to P4");
  dev.tap(ButtonInputManager::Button::Enter);
  check(onScreen(dev, "confirm-reset-session"), "ENTER-short on P4 opens the confirm screen");

  harness::writes.clear();
  dev.press(ButtonInputManager::Button::Enter, true);
  for (int i = 0; i < 12; ++i) dev.tick(50);  // ~600 ms in
  // §4.3's table: "Reset session? — ENTER long (1.5 s), no countdown", and §3 makes 1.5 s
  // the gesture boundary rather than a countdown.
  check(!dev.controller.context().countdownActive,
        "a 1.5 s guard draws no countdown — it is the long-press gesture itself");

  for (int i = 0; i < 24; ++i) dev.tick(50);  // past 1500 ms
  // Arming at the 1.5 s threshold instead of on press would make this guard zero-length in
  // one direction and 3 s long in the other; 1.7 s of hold must be exactly enough.
  check(wroteOnce(plc::REG_MASTER_RESET_ALL_SESSION),
        "holding ENTER for 1.5 s issues the reset-session command");
  check(dev.controller.navigator().depth() == 0 && onScreen(dev, "info-p4-session-liters"),
        "and returns to the page the operator started from");
  dev.press(ButtonInputManager::Button::Enter, false);
  dev.tick(30);
}

void confirmAbortTests() {
  std::printf("\n[releasing ENTER before zero aborts — §4.3 note 1]\n");

  Device dev;
  dev.boot();
  check(walkToInfoPage(dev, UiPage::CumulativeLiters), "paged the info ring to P2");
  dev.tap(ButtonInputManager::Button::Enter);
  check(onScreen(dev, "confirm-reset-totals"), "on the 3 s confirm screen");

  // Release at ~2 s: past the 1.5 s threshold — so ButtonInputManager has already emitted
  // the long press and will emit nothing on release — but well before zero.
  harness::writes.clear();
  dev.press(ButtonInputManager::Button::Enter, true);
  for (int i = 0; i < 40; ++i) dev.tick(50);
  check(dev.controller.context().countdownActive, "the countdown was running when released");
  check(dev.controller.context().countdownSeconds >= 1 &&
            dev.controller.context().countdownSeconds <= 2,
        "with a second or so still left to go");
  dev.press(ButtonInputManager::Button::Enter, false);
  dev.tick(30);
  dev.tick(30);

  check(harness::writes.empty(), "an aborted countdown fires no action");
  check(!dev.controller.context().countdownActive, "and takes its overlay down");
  // §4.3 note 1: aborting "returns to the confirm screen" — it is not also an exit. The
  // release came after the 1.5 s threshold, so ButtonInputManager emits no short press and
  // the screen's ENTER-short f-exit correctly does not fire.
  check(onScreen(dev, "confirm-reset-totals") && dev.controller.navigator().depth() == 1,
        "releasing past 1.5 s returns to the confirm screen rather than exiting it");

  // A plain tap is still the way out, and must not be swallowed by the arming path.
  dev.tap(ButtonInputManager::Button::Enter);
  check(harness::writes.empty(), "a tap on the confirm screen writes nothing");
  check(dev.controller.navigator().depth() == 0 && onScreen(dev, "info-p2-cumulative-liters"),
        "a tap exits via the screen's own ENTER-short f-exit flow");

  // §4.3 note 2: "during a countdown, UP/DOWN have no effect" — including not cancelling it.
  Device other;
  other.boot();
  check(walkToInfoPage(other, UiPage::CumulativeLiters), "paged a second device to P2");
  other.tap(ButtonInputManager::Button::Enter);
  harness::writes.clear();
  other.press(ButtonInputManager::Button::Enter, true);
  for (int i = 0; i < 10; ++i) other.tick(50);
  other.press(ButtonInputManager::Button::Up, true);
  for (int i = 0; i < 10; ++i) other.tick(50);
  other.press(ButtonInputManager::Button::Up, false);
  for (int i = 0; i < 50; ++i) other.tick(50);
  check(wroteOnce(plc::REG_MASTER_RESET_ALL_MEASURED),
        "UP pressed mid-countdown neither cancels it nor stops the action");
  other.press(ButtonInputManager::Button::Enter, false);
  other.tick(30);
}

// ── The ENTER-hold route to factory reset must reboot ───────────────────────────────────
//
// Making Timeout/Enter flows fire turns `core.action.factory-reset` from an undispatchable
// action into a live one: the dataset declares a 30 s hold on `confirm-factory-reset`
// (GeneratedUi.cpp:1240) and P8 descends into it by ordinary paging (GeneratedUi.cpp:1176).
//
// §4.3's table says the outcome is "reboot, no toast". `ui_actions.cpp`'s handler only calls
// ctx.factoryReset() — firmware.cpp's performFactoryReset() — which wipes NVS and resets
// linkSettings to defaults in RAM but arms no reboot. The reboot, the solid-white
// acknowledgement (firmware.cpp:466) and restartRs485() all hang off
// `InteractionResult::restartScheduled`, which only scheduleFactoryReset() sets. Dispatching
// the flow instead would leave the device serving on its old slave ID/baud with wiped NVS
// indefinitely — a half-completed destructive action, worse than one that cannot fire.
void factoryResetHoldTests() {
  std::printf("\n[the P8 30 s ENTER hold reboots — §4.3 \"reboot, no toast\"]\n");

  Device dev;
  dev.boot();
  check(walkToInfoPage(dev, UiPage::FactoryReset), "paged the info ring to P8");
  dev.tap(ButtonInputManager::Button::Enter);
  check(onScreen(dev, "confirm-factory-reset"), "ENTER-short on P8 opens the confirm screen");

  harness::writes.clear();
  dev.press(ButtonInputManager::Button::Enter, true);
  for (int i = 0; i < 40; ++i) dev.tick(50);  // ~2 s in
  check(dev.controller.context().countdownActive && dev.controller.context().countdownSeconds >= 28,
        "a 30 s guard does draw a countdown, unlike the 1.5 s one");
  check(harness::factoryResets == 0, "and has not wiped anything two seconds in");

  for (int i = 0; i < 580; ++i) dev.tick(50);  // past 30 000 ms
  check(harness::factoryResets == 1, "holding ENTER for 30 s wipes exactly once");
  // The discriminating assertions: dispatching the flow action satisfies the wipe above and
  // nothing below it.
  check(dev.lastResult.restartScheduled,
        "and arms the reboot — a wipe with no reboot leaves stale RS485 settings live");
  check(dev.lastResult.restartAtMs > dev.now,
        "with the 1 s delay the combo path uses, not immediately");
  check(dev.lastResult.ledsSuspended,
        "and suspends the LEDs, so §3.5's acceptance latch owns the panel");

  dev.tick(50);
  check(dev.controller.context().countdownScreenId != nullptr &&
            std::strcmp(dev.controller.context().countdownScreenId, "confirm-factory-reset") == 0,
        "the confirm screen stays up until the reboot rather than unwinding to P8");
  check(harness::factoryResets == 1, "and the wipe is not repeated on later passes");

  dev.press(ButtonInputManager::Button::Enter, false);
  dev.tick(30);
  check(harness::factoryResets == 1, "releasing after the wipe does not wipe again");
}


void retiredComboTests() {
  std::printf("\n[the retired blind UP+DOWN combo must be gone — safety]\n");

  // Display_UI_Requirements §3.1 instructs the operator to press UP+DOWN to blank the
  // display. That same pair used to arm a factory reset, showing a countdown at 3 s and
  // erasing NVS at 30 s. An operator holding a moment longer to check the screen really went
  // dark — the natural thing to do — started a wipe. §3.3 retired the combo; it was still live.
  Device dev;
  dev.boot();

  dev.press(ButtonInputManager::Button::Up, true);
  dev.press(ButtonInputManager::Button::Down, true);

  // Past the 3 s point where the old overlay appeared.
  for (int i = 0; i < 40; ++i) dev.tick(100);
  check(harness::factoryResets == 0, "four seconds of UP+DOWN does not arm anything");

  // Past the 30 s point where the old combo wiped NVS.
  for (int i = 0; i < 320; ++i) dev.tick(100);
  check(harness::factoryResets == 0,
        "thirty-six seconds of UP+DOWN does not wipe NVS — the combo is genuinely removed");

  dev.press(ButtonInputManager::Button::Up, false);
  dev.press(ButtonInputManager::Button::Down, false);
  dev.tick(30);
  check(harness::factoryResets == 0, "and nothing fires on release either");

  // The replacement path must still work, or removing the combo would leave no factory reset
  // at all — which is why this could not be deleted before the countdown arming was fixed.
  check(dev.controller.navigator().current() != nullptr,
        "the device is still navigable afterwards");
}

}  // namespace

int main() {
  std::printf("interaction layer — real firmware, fake buttons\n\n");
  idleContractTests();
  idleTimeoutTests();
  navigationRingTests();
  retiredComboTests();
  configListPagingTests();
  configEditorDescentTests();
  sensorEditorDescentTests();
  editorDatasetInvariantTests();
  repaintCadenceTests();
  confirmCountdownTests();
  confirmSessionCountdownTests();
  confirmAbortTests();
  factoryResetHoldTests();
  linkApplyProtocolTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
