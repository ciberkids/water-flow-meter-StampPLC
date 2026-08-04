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
#include "net/net_register_map.h"
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

/**
 * How the next sensor-config write should be refused, so §5.5's two outcomes can be told
 * apart.
 *
 * Accept        — the normal path.
 * NyquistRefuse — refused AND parked awaiting an override, which is the only failure an
 *                 override can resolve and so the only one that may offer "Save anyway".
 * PlainRefuse   — refused for any of the five other reasons writeSetting can fail.
 */
enum class WriteMode { Accept, NyquistRefuse, PlainRefuse };
WriteMode writeMode = WriteMode::Accept;
}  // namespace harness

ModbusManager::ModbusManager(const ModbusDependencies& deps) : deps_(deps) {}

bool ModbusManager::applyHoldingWrite(uint16_t address,
                                      uint16_t value,
                                      plc::WriteOrigin origin) {
  harness::writes.push_back({address, value, origin});

  // A sensor-block write can be refused two ways. Setting overridePending_ here is legitimate
  // and not a back door: this function IS ModbusManager::applyHoldingWrite, so it has the same
  // access the production definition does, and it is what the real one does when the Nyquist
  // check parks a write.
  if (address >= plc::SENSOR_1_BASE_ADDR && harness::writeMode != harness::WriteMode::Accept) {
    const std::size_t index =
        static_cast<std::size_t>((address - plc::SENSOR_1_BASE_ADDR) / plc::SENSOR_BLOCK_SIZE);
    if (index < plc::kNumSensors) {
      overridePending_[index] = harness::writeMode == harness::WriteMode::NyquistRefuse;
    }
    return false;
  }
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
  plc::SpiArbiter spiArbiter;
  ui::UiAssets assets = ui::loadGeneratedAssets();
  plc::LinkSettingsManager link;
  plc::NetSettings net;
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
    // Same reasoning as the line above about SettingsAccess itself: without this the fourteen
    // network settings have no storage, every text edit silently refuses, and the tests below
    // would pass against a device that cannot hold an SSID.
    settings.net = &net;
    deps.settings = &settings;
    harness::factoryResets = 0;
    interactions.begin(now, harness::countFactoryReset, deps);
    // firmware.cpp wires the renderer with the theme, the router and a binding resolver that
    // can see the settings and the controller; anything less and every config binding falls
    // back to its placeholder.
    bindings.bindSettings(&settings, &controller);
    renderer.bindSpiArbiter(&spiArbiter);
    renderer.applyTheme(assets.palette);
    renderer.bindScreenRouter(&router);
    renderer.bindBindingResolver(&bindings);
    renderer.begin();
    tick(0);
  }

  /** Passes on which a reset was reported accepted — the §3.5 white-latch signal. */
  std::size_t resetAcceptedPasses = 0;
  /**
   * Times the selector was asked to open.
   *
   * Counted rather than read from lastResult, which is a snapshot of the LAST pass: a gesture
   * that fires mid-hold and then keeps ticking would look like it never fired at all. That is
   * exactly how the first version of these tests failed.
   */
  std::size_t selectorOpens = 0;
  std::size_t selectionCommits = 0;

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
    // §3.5's white acceptance latch fires on this, for every reset — not only the one that
    // reboots. Counted rather than latched so a stuck flag is distinguishable from a
    // correctly re-triggered one.
    if (result.resetAccepted) {
      resetAcceptedPasses += 1;
    }
    if (result.openPackSelector) {
      selectorOpens += 1;
    }
    if (result.packSelectionCommitted) {
      selectionCommits += 1;
    }
    // firmware.cpp:495-501 drives the LED controller separately from the UI controller. Without
    // this the harness never ran LedController::update at all, so any assertion about LED output
    // — including how much I2C traffic it generates — passed for the wrong reason.
    leds.setSuspended(result.ledsSuspended);
    leds.update(now, 0.0, 0.0, true, false);
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

  /** All three buttons, held. §3.4.1's recovery gesture needs 3 s. */
  void holdAllThree(uint32_t forMs) {
    press(ButtonInputManager::Button::Up, true);
    press(ButtonInputManager::Button::Down, true);
    press(ButtonInputManager::Button::Enter, true);
    for (uint32_t elapsed = 0; elapsed < forMs; elapsed += 50) tick(50);
  }

  void releaseAllThree() {
    press(ButtonInputManager::Button::Up, false);
    press(ButtonInputManager::Button::Down, false);
    press(ButtonInputManager::Button::Enter, false);
    tick(50);
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

/**
 * Walks to a PER-SENSOR value editor — one whose editor.sensorIndex is non-zero.
 *
 * The Nyquist path only exists for sensor calibration, so a device-wide editor
 * (config.modbusSlaveId and friends, sensorIndex 0) cannot exercise it. Route:
 * P7 -> config root -> C7 Sensors -> Sensor 1 -> its settings ring -> an editor.
 */
bool descendToASensorEditor(Device& dev) {
  for (int i = 0; i < 16; ++i) {
    if (dev.controller.page() == UiPage::EnterConfiguration) break;
    dev.tap(ButtonInputManager::Button::Down);
  }
  dev.tap(ButtonInputManager::Button::Enter);  // into the config root ring

  // Cycle the config ring to the Sensors entry, which descends rather than editing.
  for (int i = 0; i < 12; ++i) {
    const auto* screen = dev.controller.navigator().current();
    if (screen && std::strcmp(screen->id, "config-c7-sensor-select") == 0) break;
    dev.tap(ButtonInputManager::Button::Down);
  }
  dev.tap(ButtonInputManager::Button::Enter);  // sensor list
  dev.tap(ButtonInputManager::Button::Enter);  // sensor 1's settings ring

  // Must be a CALIBRATION setting, not config.sensor.connected. Connected writes to the
  // bitmap at register 10, outside any sensor block, so the Nyquist check never sees it —
  // the first version of this helper stopped there and the tests failed for that reason
  // rather than for the behaviour they were checking.
  const auto isCalibration = [](const UiEditorState& e) {
    return e.active && e.sensorIndex > 0 && e.setting != nullptr &&
           e.setting->registerOffset != ui::kNoRegister;
  };

  for (int guard = 0; guard < 10; ++guard) {
    if (isCalibration(dev.controller.editor())) return true;
    if (dev.controller.editor().active) {
      // Sitting in the wrong editor: discard out of it and move to the next sibling.
      dev.press(ButtonInputManager::Button::Enter, true);
      dev.tick(1600);
      dev.press(ButtonInputManager::Button::Enter, false);
      dev.tick(30);
      dev.tap(ButtonInputManager::Button::Down);
    }
    dev.tap(ButtonInputManager::Button::Enter);
  }
  return isCalibration(dev.controller.editor());
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

  // ── §7's number, stated rather than implied ──────────────────────────────────
  //
  // Everything above asserts the SHAPE of the cadence (fast while interactive, slow while not).
  // None of it asserts §7's actual figure: a button press must be acknowledged on screen within
  // 100 ms. The paging check came closest, but only because tap() happens to advance 90 ms — so it
  // would have kept passing if the deadline slipped to 90.
  //
  // Measured from the RELEASE, because that is when a short gesture completes and the action fires.
  Device latency;
  latency.boot();
  latency.press(ButtonInputManager::Button::Down, true);
  latency.tick(30);
  latency.tick(30);
  const auto* screenBeforeRelease = latency.controller.navigator().current();
  latency.press(ButtonInputManager::Button::Down, false);
  latency.resetFrames();
  uint32_t elapsed = 0;
  uint32_t paintedAt = 0;
  for (int i = 0; i < 30 && paintedAt == 0; ++i) {  // a 300 ms window, sampled every 10 ms
    latency.tick(10);
    elapsed += 10;
    if (latency.frames() >= 1) paintedAt = elapsed;
  }
  std::printf("      first repaint %u ms after the release\n", paintedAt);
  check(latency.controller.navigator().current() != screenBeforeRelease,
        "the release completed the gesture and moved the ring");
  check(paintedAt > 0 && paintedAt <= 100,
        "a button press is acknowledged on screen within 100 ms of release (§7)");

  // ── Per-frame WORK, which is the half a host cannot time ─────────────────────
  //
  // What this suite genuinely cannot check: whether the work in one repaint fits inside 100 ms on
  // an ESP32-S3 driving a real SPI panel. Host wall-clock on x86 is not a proxy for that, and
  // measuring it here would look like validating a device timing requirement with irrelevant
  // hardware — worse than not measuring it, because it would read as covered. That measurement
  // belongs to N9, on the board.
  //
  // What IS checkable is the work itself: a frame draws a bounded number of primitives. If a future
  // screen or binding change multiplies the per-frame draw count, the timing risk grows
  // proportionally, and this notices even though it cannot time anything.
  Device work;
  work.boot();
  check(descendToAnEditor(work), "an editor opened for the per-frame work count");
  work.resetFrames();
  work.tick(100);  // long enough for exactly one interactive repaint
  auto& panel = m5stamplc_stub::board().Display;
  const uint32_t framesDrawn = panel.fillScreens;
  const uint32_t primitives = panel.drawStrings + panel.fillRects + panel.drawRects +
                              panel.fillCircles + panel.prints;
  std::printf("      %u frame(s), %u draw primitives\n", framesDrawn, primitives);
  check(framesDrawn >= 1, "the editor painted at least one frame in 100 ms");
  // A generous ceiling. It is not a performance target — it is a tripwire for an order-of-magnitude
  // change, which is the kind that turns a comfortable frame budget into a missed deadline.
  check(primitives > 0 && primitives < 200,
        "and did so with a bounded number of draw primitives, not an unbounded sweep");
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
  std::size_t textRows = 0;

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

    // A row showing a TEXT setting must NOT descend to an editor — there is no on-device text
    // entry (§6.3). Everything else must, or the setting is unreachable at the panel.
    bool carriesTextSetting = false;
    for (std::size_t e = 0; e < screen.elementCount; ++e) {
      const auto* found = ui::findSetting(screen.elements[e].bindingId);
      if (found && found->kind == ui::SettingKind::Text) {
        carriesTextSetting = true;
        break;
      }
    }
    const auto* descend = flowWithAction(screen, "ui.action.nav.descend");
    const auto* target = descend ? screenById(descend->targetScreenId) : nullptr;
    const bool hasEditorBelow = target && declaresBinding(*target, "config.editor.pending");
    if (carriesTextSetting) {
      ++textRows;
      if (hasEditorBelow) {
        everyListPageHasItsEditor = false;
        std::printf("    %s is a text row but descends to an editor\n", screen.id);
      }
      continue;
    }
    if (!hasEditorBelow) {
      everyListPageHasItsEditor = false;
      std::printf("    %s has no editor screen below it\n", screen.id);
    }
  }

  // Derived from the catalogue rather than hard-coded. The invariant is "one editor per setting"
  // — the completeness rule — and a literal 10 only restated how many settings there happened to
  // be, so adding the fourteen network settings broke a test that was not actually wrong about
  // anything. Counting from settingCount() states the rule the export gate enforces.
  // Derived from the catalogue, and now split by kind: the completeness rule covers every setting
  // an operator can CHANGE at the panel, which excludes text (§6.3). Text still gets a row, so the
  // row count covers everything.
  std::size_t editableSettings = 0;
  for (std::size_t i = 0; i < ui::settingCount(); ++i) {
    const auto* setting = ui::settingAt(i);
    if (setting && setting->kind != ui::SettingKind::Text) ++editableSettings;
  }
  std::printf("      %zu editors / %zu rows (%zu text) / %zu settings, %zu editable\n",
              static_cast<std::size_t>(editors), static_cast<std::size_t>(settingListPages),
              static_cast<std::size_t>(textRows), ui::settingCount(), editableSettings);
  check(static_cast<std::size_t>(editors) == editableSettings,
        "every NON-TEXT setting has exactly one editor screen (the completeness rule)");
  check(pendingMatchesCommit,
        "a screen shows a pending value if and only if it has a commit flow");
  check(static_cast<std::size_t>(settingListPages) == ui::settingCount(),
        "and every setting has a row, text included, so all of them are at least readable");
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
  // (d) the reset is worthless if the operator is left staring at "RESET TOTALS?" with no way
  // to tell it happened. This originally ascended straight to the originating page, which was
  // a documented interim shortcut taken because the toast had no driver. It does now, so the
  // journey is confirm -> toast -> originating page, and the toast is the acknowledgement
  // §4.3.1 asks for. Asserted end to end rather than only at the destination.
  check(onScreen(dev, "toast-totals-reset"), "and the acknowledgement toast is shown");
  for (int i = 0; i < 24; ++i) dev.tick(100);  // past the toast's declared 2000 ms
  check(dev.controller.navigator().depth() == 0 && onScreen(dev, "info-p2-cumulative-liters"),
        "which then dismisses itself back to the page it was opened from");
  check(!dev.controller.context().countdownActive, "the overlay is gone once the action fired");

  // Still holding ENTER after the action fired: releasing must not re-open the confirm
  // screen or write a second time.
  dev.press(ButtonInputManager::Button::Enter, false);
  dev.tick(30);
  dev.tick(30);
  check(wroteOnce(plc::REG_MASTER_RESET_ALL_MEASURED),
        "releasing after the action does not write again");
  check(!onScreen(dev, "confirm-reset-totals"), "nor does it re-descend into the confirm screen");
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
  check(onScreen(dev, "toast-session-reset"), "and its acknowledgement toast is shown");
  for (int i = 0; i < 24; ++i) dev.tick(100);
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


void nyquistPromptTests() {
  std::printf("\n[Nyquist override — Display_UI_Requirements §5.5]\n");

  // A refusal the operator CAN override.
  {
    Device dev;
    dev.boot();
    harness::writeMode = harness::WriteMode::NyquistRefuse;
    const bool editing = descendToASensorEditor(dev);
    check(editing, "reached a per-sensor value editor");

    dev.tap(ButtonInputManager::Button::Enter);  // commit -> refused
    check(dev.controller.editor().nyquistPrompt,
          "a Nyquist refusal raises the override prompt");
    check(!dev.controller.editor().commitFailed,
          "and is not reported as a generic failure");
    check(dev.controller.editor().active, "the editor stays open so the value is not lost");

    // §5.5: DOWN = save anyway. This used to adjust the value by -1 while the prompt on
    // screen said "DOWN=Save anyway".
    const std::size_t before = harness::writes.size();
    harness::writeMode = harness::WriteMode::Accept;
    dev.tap(ButtonInputManager::Button::Down);
    check(harness::writes.size() > before, "DOWN with the prompt showing commits the override");
    check(!dev.controller.editor().nyquistPrompt, "and clears the prompt");
  }

  // A refusal the operator CANNOT override must not offer to.
  {
    Device dev;
    dev.boot();
    harness::writeMode = harness::WriteMode::PlainRefuse;
    descendToASensorEditor(dev);
    dev.tap(ButtonInputManager::Button::Enter);

    check(!dev.controller.editor().nyquistPrompt,
          "a non-Nyquist refusal does NOT claim the sampling rate is the problem");
    check(dev.controller.editor().commitFailed, "it reports a generic write failure instead");

    // Offering "Save anyway" for a failure an override cannot fix would be worse than useless.
    const std::size_t before = harness::writes.size();
    dev.tap(ButtonInputManager::Button::Down);
    check(harness::writes.size() == before,
          "DOWN does not attempt an override that cannot succeed");
    check(!dev.controller.editor().commitFailed, "it dismisses the message instead");
  }

  // UP always means "edit again", for either outcome.
  {
    Device dev;
    dev.boot();
    harness::writeMode = harness::WriteMode::NyquistRefuse;
    descendToASensorEditor(dev);
    dev.tap(ButtonInputManager::Button::Enter);
    check(dev.controller.editor().nyquistPrompt, "prompt is up");
    dev.tap(ButtonInputManager::Button::Up);
    check(!dev.controller.editor().nyquistPrompt, "UP dismisses the prompt");
    check(dev.controller.editor().active, "and returns to editing rather than ascending");
  }

  harness::writeMode = harness::WriteMode::Accept;
}


/** Walks to P2 and holds ENTER long enough to complete its reset-totals confirm screen. */
bool completeResetTotals(Device& dev) {
  for (int i = 0; i < 16; ++i) {
    if (dev.controller.page() == UiPage::CumulativeLiters) break;
    dev.tap(ButtonInputManager::Button::Down);
  }
  dev.tap(ButtonInputManager::Button::Enter);  // descend to the confirm screen
  const auto* screen = dev.controller.navigator().current();
  if (!screen || std::strcmp(screen->id, "confirm-reset-totals") != 0) {
    return false;
  }
  // Hold past the screen's declared 3000 ms.
  dev.press(ButtonInputManager::Button::Enter, true);
  for (int i = 0; i < 40; ++i) dev.tick(100);
  dev.press(ButtonInputManager::Button::Enter, false);
  dev.tick(30);
  return true;
}

void toastTests() {
  std::printf("\n[acknowledgement toast — §4.3.1, NF-20260730-01 §3.8]\n");

  Device dev;
  dev.boot();
  check(completeResetTotals(dev), "reached and completed confirm-reset-totals");

  const auto* afterCommit = dev.controller.navigator().current();
  check(afterCommit && std::strcmp(afterCommit->id, "toast-totals-reset") == 0,
        "completing the reset shows the acknowledgement toast, not a bare screen change");
  check(harness::writes.size() > 0, "and the reset itself was dispatched");

  // The toast must not need a button. NF-20260730-01 §3.8: an auto timeout runs unattended.
  const uint8_t depthOnToast = dev.controller.navigator().depth();
  dev.tick(500);
  check(dev.controller.navigator().current() == afterCommit,
        "half a second in, the toast is still showing");

  // Past its declared 2000 ms.
  for (int i = 0; i < 20; ++i) dev.tick(100);
  const auto* afterToast = dev.controller.navigator().current();
  check(afterToast != afterCommit, "the toast dismisses itself after its declared 2 s");

  // The point of replaceCurrent: dismissing must land on the ORIGINATING page, not back on
  // the confirm screen. If the toast had been pushed, nav.back would return to it and the
  // operator would be asked again whether to do what they just did.
  check(afterToast && std::strcmp(afterToast->id, "confirm-reset-totals") != 0,
        "and does NOT return to the confirm screen");
  check(afterToast && std::strcmp(afterToast->id, "info-p2-cumulative-liters") == 0,
        "it returns to the page the operator started from");
  check(dev.controller.navigator().depth() < depthOnToast,
        "one level shallower than the toast, so the modal is fully unwound");
}

void toastCancellationTests() {
  std::printf("\n[an abandoned toast must not fire behind the operator]\n");

  Device dev;
  dev.boot();
  check(completeResetTotals(dev), "on the toast");

  // Leave early, the way an impatient operator would: the display-off combo works from any
  // screen at any depth (§3.1).
  dev.tapUpDown();
  const auto* afterEscape = dev.controller.navigator().current();
  check(dev.controller.navigator().depth() == 0, "UP+DOWN unwound to the root");

  // Now let the toast's timer expire. Its action must NOT fire: it would ascend from wherever
  // the operator now is, which is not where the timer was armed. Comparing the screen POINTER
  // rather than a boolean is what makes this work.
  for (int i = 0; i < 30; ++i) dev.tick(100);
  check(dev.controller.navigator().current() == afterEscape,
        "the abandoned toast's timer does not navigate behind the operator");
  check(dev.controller.navigator().depth() == 0, "and does not pop below the root");
}


void resetAcceptanceLedTests() {
  std::printf("\n[white acceptance latch — RGB_LED_Behavior §3.5]\n");

  // §3.5: "Applies to every reset confirm screen: factory reset, reset totals and reset
  // session." firmware.cpp used to drive the latch off restartScheduled, which only the
  // factory path sets — so the two resets an operator actually uses gave no panel signal.
  Device dev;
  dev.boot();
  check(dev.resetAcceptedPasses == 0, "no acceptance reported before anything is confirmed");

  check(completeResetTotals(dev), "completed confirm-reset-totals");
  check(dev.resetAcceptedPasses >= 1,
        "reset-totals reports acceptance, so the panel latches solid white");
  check(!dev.lastResult.restartScheduled,
        "and does so WITHOUT scheduling a reboot — which is why the old gate missed it");

  // Exactly one pass, so the latch is re-triggered rather than held on by a stuck flag.
  const std::size_t after = dev.resetAcceptedPasses;
  for (int i = 0; i < 10; ++i) dev.tick(100);
  check(dev.resetAcceptedPasses == after,
        "acceptance is reported once, on the completing pass only");
}


void spiHandoverRenderTests() {
  std::printf("\n[the renderer honours the SPI handover — §4.10]\n");

  Device dev;
  dev.boot();
  for (int i = 0; i < 4; ++i) dev.tick(100);
  // frames() counts full repaints, which the renderer only performs on a screen CHANGE — a
  // steady-state tick legitimately paints nothing, so the change has to be provoked.
  dev.resetFrames();
  dev.tap(ButtonInputManager::Button::Down);
  check(dev.frames() > 0, "changing screen repaints while the renderer owns the bus");

  // Hand the bus to the card between frames, as the selector's directory scan does.
  check(dev.spiArbiter.requestCard(dev.now), "the card takes the bus between frames");
  dev.resetFrames();
  for (int i = 0; i < 5; ++i) {
    dev.tap(ButtonInputManager::Button::Down);  // keep changing screen, so a repaint is due
  }
  check(dev.frames() == 0,
        "and the renderer paints NOTHING while it is held, even with a repaint due");

  // Give it back: one full repaint is owed, because the panel now shows pre-handover state.
  dev.spiArbiter.releaseCard(dev.now);
  dev.resetFrames();
  dev.tick(100);
  check(dev.frames() > 0,
        "handing it back forces a FULL repaint even with no further screen change, because the "
        "panel shows pre-handover state");

  // And the frame really was closed each pass, so the card is not blocked by a stale open frame.
  check(dev.spiArbiter.mayBeginFrame(), "the frame is closed after each pass, not left open");
  check(dev.spiArbiter.timeouts() == 0,
        "no handover ever needed the wedged-renderer timeout, so frames close cleanly");
}


void recoveryGestureTests() {
  std::printf("\n[UP+DOWN+ENTER recovery gesture — §3.4.1]\n");

  {
    Device dev;
    dev.boot();
    dev.holdAllThree(2900);
    check(dev.selectorOpens == 0, "just under 3 s does not open the selector");
    dev.tick(200);
    check(dev.selectorOpens == 1, "past 3 s it does");

    // Fires once, not on every subsequent pass — otherwise the selector would be re-entered
    // continuously for as long as the operator kept holding.
    for (int i = 0; i < 10; ++i) dev.tick(50);
    check(dev.selectorOpens == 1, "and does not re-fire while the buttons stay held");
    dev.releaseAllThree();
  }

  // THE COLLISION THAT MATTERS. §3 gives UP+DOWN to display-off, with a 1000 ms window. The
  // recovery gesture is the same two buttons plus ENTER, held three times as long.
  {
    Device dev;
    dev.boot();
    const auto* before = dev.controller.navigator().current();
    dev.holdAllThree(3100);
    check(dev.selectorOpens == 1, "the three-button hold opened the selector");
    check(dev.controller.mode() != UiMode::Idle,
          "and did NOT also trip display-off, which is the same two buttons plus ENTER");
    dev.releaseAllThree();
    check(dev.controller.navigator().current() == before,
          "releasing the three does not also page or descend on the way out");
  }

  // The reverse direction: the plain display-off combo must still work untouched.
  {
    Device dev;
    dev.boot();
    dev.tapUpDown();
    check(dev.controller.mode() == UiMode::Idle, "UP+DOWN alone still turns the display off");
    check(dev.selectorOpens == 0, "and does not open the selector");
  }

  // Available from any depth, because a pack that renders nothing illegible could have left the
  // operator anywhere.
  {
    Device dev;
    dev.boot();
    descendToAnEditor(dev);
    check(dev.controller.navigator().depth() > 0, "nested somewhere");
    dev.holdAllThree(3100);
    check(dev.selectorOpens == 1, "the gesture works from a nested level too");
  }
}


void selectorPageTests() {
  std::printf("\n[the firmware-drawn Select Menu page — §3.4]\n");

  char names[3][ui::PackLoader::kMaxNameBytes] = {};
  std::snprintf(names[0], sizeof(names[0]), "production.uipack");
  std::snprintf(names[1], sizeof(names[1]), "service.uipack");

  Device dev;
  dev.boot();
  dev.controller.openPackSelector(names, 2, "production.uipack", dev.now);
  check(dev.controller.selectorActive(), "the page opens");
  check(!dev.controller.editor().active,
        "and discards any pending edit, since it is reachable from inside an editor");

  // The page is painted by the FIRMWARE, not from the screen table — that is what makes it work
  // when a pack draws nothing. Asserted on the strings that actually reached the panel.
  dev.resetFrames();
  dev.tick(100);
  const std::string& painted = m5stamplc_stub::board().Display.strings;
  check(painted.find("SELECT MENU") != std::string::npos, "it paints its own title");
  check(painted.find("Built-in") != std::string::npos, "entry 0 is always the built-in default");
  check(painted.find("production.uipack") != std::string::npos, "the card's packs are listed");
  check(painted.find("*") != std::string::npos, "and the running menu is marked");

  // UP/DOWN move the cursor rather than paging the screen behind.
  const std::size_t before = dev.controller.packSelector().cursor();
  dev.tap(ButtonInputManager::Button::Down);
  check(dev.controller.packSelector().cursor() != before, "DOWN moves the cursor");
  check(dev.controller.selectorActive(), "and does not leave the page");

  // ENTER-long leaves without selecting, as it does on every other screen.
  //
  // Two ticks, not one: the first registers the press and the second is where the hold duration
  // is evaluated. A single long tick only ever produces a SHORT press on release, which is how
  // the first version of this check accidentally committed a selection instead of leaving.
  dev.press(ButtonInputManager::Button::Enter, true);
  dev.tick(50);
  dev.tick(1600);
  dev.press(ButtonInputManager::Button::Enter, false);
  dev.tick(50);
  check(!dev.controller.selectorActive(), "ENTER-long leaves without selecting");
  check(dev.selectionCommits == 0, "and commits nothing");
}

void selectorCommitTests() {
  std::printf("\n[committing a selection]\n");

  char names[2][ui::PackLoader::kMaxNameBytes] = {};
  std::snprintf(names[0], sizeof(names[0]), "production.uipack");
  std::snprintf(names[1], sizeof(names[1]), "service.uipack");

  {
    Device dev;
    dev.boot();
    dev.controller.openPackSelector(names, 2, "production.uipack", dev.now);
    // Move off the running entry, then commit.
    while (dev.controller.packSelector().commitAction() == ui::PackSelector::Commit::Nothing) {
      dev.controller.packSelector().moveCursor(1);
    }
    dev.tap(ButtonInputManager::Button::Enter);
    check(dev.selectionCommits == 1, "ENTER-short on a different entry commits");
    check(dev.controller.packSelector().commitAction() != ui::PackSelector::Commit::Nothing,
          "and the caller can still read what to write");
  }
  {
    // Choosing what is already running: leaving is the honest response. Rebooting into an
    // identical UI would look like the press was ignored.
    Device dev;
    dev.boot();
    dev.controller.openPackSelector(names, 2, "production.uipack", dev.now);
    while (dev.controller.packSelector().commitAction() != ui::PackSelector::Commit::Nothing) {
      dev.controller.packSelector().moveCursor(1);
    }
    dev.tap(ButtonInputManager::Button::Enter);
    check(dev.selectionCommits == 0, "ENTER on the already-running entry commits nothing");
    check(!dev.controller.selectorActive(), "and leaves the page rather than doing nothing at all");
  }
}


void ledI2cTrafficTests() {
  std::printf("\n[the LED must not compete with the sensor bus — R2.1.6]\n");

  // The expander the LEDs sit on (PI4IOE5V6408, 0x43) shares its I2C bus with the one
  // readPlcInput reads the sensors through (AW9523B, 0x59): SCL 15, SDA 13, one driver mutex. So
  // an LED write is not free — it is contention with the measurement.
  Device dev;
  dev.boot();
  auto& board = m5stamplc_stub::board();

  // Settle, then measure the steady state: nothing changing, LEDs already correct.
  for (int i = 0; i < 20; ++i) dev.tick(1);
  const uint32_t settled = board.setStatusLightCalls;
  for (int i = 0; i < 1000; ++i) dev.tick(1);
  const uint32_t steady = board.setStatusLightCalls - settled;

  std::printf("      %u expander writes across 1000 idle passes\n", steady);
  // Before the dirty check this was one write per pass — 1000 on this exact loop. The bound is
  // deliberately generous: what matters is the ORDER of magnitude, not an exact count that would
  // break on any unrelated timing change.
  check(steady < 50,
        "an idle second does not write the shared bus a thousand times");

  // And the patterns that genuinely change state must still be driven, or the fix would have
  // bought accuracy by breaking the LED requirement.
  const uint32_t beforeRamp = board.setStatusLightCalls;
  dev.leds.setCardBusy(dev.now);
  for (int i = 0; i < 20; ++i) dev.tick(100);  // spans several 400 ms card-busy phases
  const uint32_t duringRamp = board.setStatusLightCalls - beforeRamp;
  std::printf("      %u writes across 2 s of the card-busy pattern\n", duringRamp);
  check(duringRamp >= 3,
        "a pattern that alternates is still driven onto the pins, not suppressed");
  dev.leds.clearCardBusy();
}

/**
 * Text settings are DISPLAY-ONLY at the panel (§6.3).
 *
 * There is no on-device text entry: a three-button character wheel is not a usable way to type a
 * 63-character WPA2 passphrase, so text arrives via the web portal, RS485 or the SD credential
 * file. What has to hold here is that the display never pretends otherwise — no editor opens, no
 * screen offers one, and the value still renders (masked, when it is a secret) so an operator can
 * see which network and which broker the device is configured for.
 */
void textIsDisplayOnlyTests() {
  std::printf("\n[text settings are display-only — no on-device entry]\n");
  Device dev;
  dev.boot();

  const auto* ssid = ui::findSetting("config.wifi.ssid");
  const auto* psk = ui::findSetting("config.wifi.psk");
  check(ssid != nullptr && psk != nullptr, "the catalogue still declares the WiFi text settings");
  if (!ssid || !psk) return;

  // ── No text setting has an editor screen ──────────────────────────────────────
  //
  // The invariant that replaced "every setting has an editor". Checked over the whole generated
  // table, so a dataset that grew one back fails here rather than shipping a screen whose UP/DOWN
  // do nothing.
  std::size_t textSettings = 0, textWithEditor = 0, textRows = 0;
  for (std::size_t i = 0; i < ui::settingCount(); ++i) {
    const auto* setting = ui::settingAt(i);
    if (!setting || setting->kind != ui::SettingKind::Text) continue;
    ++textSettings;
    check(setting->maxLength > 0, "a text setting declares a capacity");
    check(!setting->perSensor, "and is not per-sensor, which readSettingText cannot express");

    for (std::size_t sc = 0; sc < ui_exporter::kGeneratedScreenCount; ++sc) {
      const auto& screen = ui_exporter::kGeneratedScreens[sc];
      bool showsThis = false, isEditor = false;
      for (std::size_t e = 0; e < screen.elementCount; ++e) {
        const char* b = screen.elements[e].bindingId;
        if (!b) continue;
        if (std::strcmp(b, setting->bindingId) == 0) showsThis = true;
        if (std::strcmp(b, "config.editor.pending") == 0) isEditor = true;
      }
      if (!showsThis) continue;
      if (isEditor) {
        ++textWithEditor;
        std::printf("      %s has an EDITOR screen (%s) — text is not editable at the panel\n",
                    setting->bindingId, screen.id);
      } else {
        ++textRows;
      }
    }
  }
  std::printf("      %zu text settings, %zu display rows, %zu editors\n",
              textSettings, textRows, textWithEditor);
  check(textSettings == 7, "seven text settings (§6.1)");
  check(textWithEditor == 0, "NONE of them has an editor screen");
  check(textRows >= textSettings, "but each is displayed somewhere, so it can be read off the panel");

  // ── Descending onto a text row must not open an editor ────────────────────────
  //
  // Belt and braces: even if a pack shipped a screen that looks like a text editor, the descend
  // handler refuses to open one. A pack is customer-supplied, so the firmware cannot assume the
  // dataset is well-formed.
  dev.controller.beginEdit(ssid, 0, 0);
  check(dev.controller.editor().active,
        "beginEdit on a text setting still sets state (it is the numeric path, unaware of kind)");
  dev.controller.endEdit();
  check(!dev.controller.editor().active, "endEdit clears it");

  // ── The value renders, and secrets mask ───────────────────────────────────────
  auto resolveBinding = [&](const char* bindingId, char* out, std::size_t size) {
    ui_exporter::Element el{};
    el.id = "probe";
    el.type = ui_exporter::ElementType::Value;
    el.bindingId = bindingId;
    return dev.bindings.resolveText(dev.controller.context(), el, out, size);
  };

  char line[80] = {};
  resolveBinding("config.wifi.ssid", line, sizeof(line));
  check(std::strcmp(line, "(not set)") == 0, "an unconfigured SSID says so rather than showing 0");

  dev.net.stage(plc::NetField::WifiSsid, "PlantFloor");
  dev.net.stage(plc::NetField::WifiPsk, "hunter2");
  dev.net.apply();

  resolveBinding("config.wifi.ssid", line, sizeof(line));
  check(std::strcmp(line, "PlantFloor") == 0,
        "a configured SSID renders as text (it read as \"0\" before the display arm existed)");
  resolveBinding("config.wifi.psk", line, sizeof(line));
  check(std::strcmp(line, "********") == 0,
        "the passphrase masks at a fixed width, hiding its length as well as its value");

  // ── The write path still exists, for the portal and RS485 to use ──────────────
  check(ui::writeSettingText(*ssid, "OtherNet", dev.settings),
        "writeSettingText still works — the portal and the SD loader are its callers");
  char stored[80] = {};
  dev.net.get(plc::NetField::WifiSsid, stored, sizeof(stored));
  check(std::strcmp(stored, "OtherNet") == 0, "and it reaches the live settings");

  // ── The shared flags register composes with a master's staged bits ─────────────
  const auto* qos = ui::findSetting("config.mqtt.qos");
  if (qos) {
    check(dev.net.mqttHaDiscovery(), "HA discovery defaults on");
    plc::NetRegisterMap::stageWrite(dev.net, plc::net_reg::kMqttFlags, 0);
    check(dev.net.mqttHaDiscovery(), "a master's flags write is staged only");
    check(ui::writeSetting(*qos, 0, 1, dev.settings), "the display commits QoS 1");
    check(dev.net.mqttQos() == 1, "QoS 1 is live");
    check(!dev.net.mqttHaDiscovery(),
          "and the master's staged bit survived — reading live would have re-enabled it");
  }
}

/**
 * R8.2a — the MENU half of portal-login recovery.
 *
 * net_settings_test covers the store and the RS485 command. What is only testable here is that the
 * panel can actually reach it: the action is registered, it lands on NetSettings, and the dataset
 * puts it behind a hold-to-confirm rather than a single press.
 */
void portalLoginResetMenuTests() {
  std::printf("\n[R8.2a — resetting the portal login from the menu]\n");
  Device dev;
  dev.boot();

  // Establish a device with a changed login and configuration worth keeping.
  dev.net.stage(plc::NetField::PortalPassword, "correct-horse");
  dev.net.stage(plc::NetField::WifiSsid, "PlantFloor");
  dev.net.stageMqttPort(8883);
  dev.net.apply();
  check(!dev.net.portalPasswordIsDefault(), "the login starts changed");

  // ── The action is registered and reaches the settings ─────────────────────────
  ui::UiActionContext ctx{dev.controller, dev.modbus, dev.leds, dev.prefs};
  ctx.nowMs = dev.now;
  ctx.settings = &dev.settings;
  check(ui::defaultActionRegistry().dispatch("core.action.reset-portal-login", ctx,
                                            ui_exporter::Flow{}),
        "core.action.reset-portal-login is registered");
  check(dev.net.portalPasswordIsDefault(), "and it restored the login");
  char buf[80] = {};
  dev.net.get(plc::NetField::WifiSsid, buf, sizeof(buf));
  check(std::strcmp(buf, "PlantFloor") == 0, "leaving the SSID alone");
  check(dev.net.mqttPort() == 8883, "and the broker port");

  // ── With no settings wired it must not crash ──────────────────────────────────
  ui::UiActionContext bare{dev.controller, dev.modbus, dev.leds, dev.prefs};
  bare.nowMs = dev.now;
  check(ui::defaultActionRegistry().dispatch("core.action.reset-portal-login", bare,
                                            ui_exporter::Flow{}),
        "dispatching with no SettingsAccess is a no-op, not a null dereference");

  // ── The dataset guards it behind a hold, and says what the new login is ───────
  const ui_exporter::Screen* confirm = nullptr;
  const ui_exporter::Screen* toast = nullptr;
  const ui_exporter::Screen* entry = nullptr;
  for (std::size_t i = 0; i < ui_exporter::kGeneratedScreenCount; ++i) {
    const auto& sc = ui_exporter::kGeneratedScreens[i];
    if (!sc.id) continue;
    if (std::strcmp(sc.id, "confirm-reset-portal-login") == 0) confirm = &sc;
    if (std::strcmp(sc.id, "toast-portal-login-reset") == 0) toast = &sc;
    if (std::strcmp(sc.id, "net-wifi-portal-reset") == 0) entry = &sc;
  }
  check(confirm != nullptr, "the confirm screen is in the generated table");
  check(toast != nullptr, "so is the acknowledgement toast");
  check(entry != nullptr, "and the WiFi-level entry that reaches it");

  if (confirm) {
    const ui_exporter::Flow* hold = nullptr;
    for (std::size_t i = 0; i < confirm->flowCount; ++i) {
      const auto& f = confirm->flows[i];
      if (f.actionId && std::strcmp(f.actionId, "core.action.reset-portal-login") == 0) hold = &f;
    }
    check(hold != nullptr, "the confirm screen carries the reset action");
    if (hold) {
      // A single press must not drop the device to a published default. A hold is encoded as
      // Timeout + Enter with timeoutMs — NOT as a gesture; `gesture` is unused on a timeout
      // trigger. That is the same pair armHoldCountdown matches on, so asserting it here is
      // asserting the contract the interaction handler actually reads.
      check(hold->trigger == ui_exporter::FlowTrigger::Timeout,
            "and it is a HELD flow (Timeout + timeoutMs), not a single press");
      check(hold->button == ui_exporter::FlowButton::Enter,
            "held on ENTER, which is what armHoldCountdown looks for");
      std::printf("      hold is %u ms\n", static_cast<unsigned>(hold->timeoutMs));
      check(hold->timeoutMs >= 3000, "held for at least 3 s");
    }
  }
  if (entry) {
    const ui_exporter::Flow* descend = nullptr;
    for (std::size_t i = 0; i < entry->flowCount; ++i) {
      const auto& f = entry->flows[i];
      if (f.targetScreenId &&
          std::strcmp(f.targetScreenId, "confirm-reset-portal-login") == 0) descend = &f;
    }
    check(descend != nullptr, "the WiFi entry descends onto the confirm screen");
  }
  if (toast) {
    bool namesTheCredential = false;
    for (std::size_t i = 0; i < toast->elementCount; ++i) {
      const auto* text = toast->elements[i].text;
      if (text && text->text && std::strstr(text->text, "admin")) namesTheCredential = true;
    }
    check(namesTheCredential,
          "the toast names the new login, so the operator is not left guessing what changed");
  }
}

}  // namespace

int main() {
  std::printf("interaction layer — real firmware, fake buttons\n\n");
  idleContractTests();
  idleTimeoutTests();
  navigationRingTests();
  retiredComboTests();
  nyquistPromptTests();
  toastTests();
  toastCancellationTests();
  resetAcceptanceLedTests();
  spiHandoverRenderTests();
  recoveryGestureTests();
  selectorPageTests();
  selectorCommitTests();
  ledI2cTrafficTests();
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
  textIsDisplayOnlyTests();
  portalLoginResetMenuTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
