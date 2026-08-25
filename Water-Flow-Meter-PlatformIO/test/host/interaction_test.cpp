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
#include <string>
#include <vector>

#include "input/interaction_handler.h"
#include "led/led_controller.h"
#include "modbus/modbus_manager.h"
#include "modbus/register_map.h"
#include "net/mqtt_command_router.h"
#include "net/net_register_map.h"
#include "modbus/sensor_types.h"
#include "ui/core/ui_bindings.h"
#include "ui/core/ui_value_catalogue.h"
#include "ui/core/ui_controller.h"
#include "ui/core/ui_actions.h"
#include "ui/core/ui_module.h"
#include "ui/core/ui_pages.h"
#include "ui/core/ui_renderer.h"
#include "ui/core/ui_root_tail.h"
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

/**
 * Whether a setting can be changed AT THE PANEL — the completeness rule's subject.
 *
 * Two exemptions, both decided by a STATIC property so the rule stays decidable, which is what
 * disqualified runtime-guarded editors (R7.3):
 *
 *  - TEXT, because there is no on-device text entry. A three-button character wheel is not a way to
 *    type a 63-character passphrase (§6.3).
 *  - WIFI and MQTT, by the owner's ruling that the panel READS them and does not set them. It is the
 *    text exemption widened to its natural edge: the web portal, the RS485 block and the SD
 *    credential file are where a broker gets configured, and a panel offering to edit half a broker's
 *    settings is worse than one offering none.
 *
 * Mirrors `assertCoversEverySetting` in web/mockup/tools/skeleton/generate.mjs, which enforces the
 * same rule on the dataset before it is generated. Two homes for one rule is not ideal, but they
 * check different artefacts — the generator refuses to EMIT an incomplete menu, this refuses to
 * ACCEPT one — and a rule enforced at only one of those two points is a rule with a hole.
 */
bool panelEditable(const ui::SettingDescriptor& setting) {
  if (setting.kind == ui::SettingKind::Text) return false;
  const char* id = setting.bindingId;
  if (!id) return false;
  return std::strncmp(id, "config.wifi.", 12) != 0 && std::strncmp(id, "config.mqtt.", 12) != 0;
}

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
  /**
   * The network snapshot the display sees.
   *
   * Test-settable, which is the whole point: the real one is built from a live WifiManager, and a
   * harness that could only ever show "disabled" would leave every net.* binding unexercised.
   */
  plc::NetStatusSnapshot netStatus{};
  /**
   * The device clock the controller publishes onto the render context.
   *
   * A real DeviceClock, not a stub: its whole subject is which states are reachable, so a test that
   * invented its own trust flags could reach combinations the class forbids and would then assert P3
   * renders something for a device that cannot exist. Left in its boot state — unset — by default,
   * which is what a harness that says nothing about time should get.
   */
  plc::DeviceClock clock;
  ui::SettingsAccess settings;
  uint16_t connectedBitmap = 0xFF;
  /**
   * `REG_UNDERSAMPLING_FLAGS`, as the display sees it. Test-settable for the same reason `netStatus`
   * above is: `tick()` passed a hard-coded 0 to `UiController::update`, so no test could put the device
   * in a warned state at all and every warning surface — the summary line, the banner text, the row
   * colour — was unreachable from this harness while looking covered by it.
   */
  uint16_t undersamplingFlags = 0;
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
    // The two bitmaps come from the FIELDS, not from literals. `0xFF` was passed straight through here
    // while `settings.connectedBitmap` pointed at the member, so a harness that switched a channel off
    // through the settings still showed the display eight connected channels.
    controller.update(now, sensors, configs, undersamplingFlags, connectedBitmap, 0.0, 0.0, 0.0f, leds,
                      result.countdown, netStatus, clock);
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
    if (screen && std::strcmp(screen->id, "config-sensors") == 0) break;
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
  //
  // 23, not 20: the root level is now ten members, and 20 steps is exactly two laps — a symmetry
  // check that lands back where it started for free would pass with UP broken. 23 is coprime with
  // both 9 and 10, so the check keeps its teeth whichever the ring turns out to be.
  bool everNull = false;
  for (int i = 0; i < 23; ++i) {
    dev.tap(ButtonInputManager::Button::Down);
    if (dev.controller.navigator().current() == nullptr) everNull = true;
  }
  check(!everNull, "cycling the root ring never lands on a null screen");
  check(dev.controller.navigator().depth() == 0, "cycling siblings does not change depth");

  // Back to the start by going the other way the same number of steps.
  for (int i = 0; i < 23; ++i) dev.tap(ButtonInputManager::Button::Up);
  check(dev.controller.navigator().current() == start,
        "the root ring is symmetric: 23 down then 23 up returns to the starting screen");
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

  // ── Idle must go dark ONCE, not a thousand times a second ────────────────────
  //
  // The idle branch runs before the throttle, which is correct — going dark must not wait for an
  // interval boundary — but it had no completion flag. LogicTask yields with vTaskDelay(1) at
  // CONFIG_FREERTOS_HZ=1000, so the device was issuing ~1000 setBacklight(false) writes per second
  // on the AW9523's I²C bus (the one the pulse sampler reads) plus ~1000 full fillScreens, for as
  // long as the display stayed off. Idle is where the device spends most of its life, and the panel
  // is dark, so there was nothing to see.
  //
  // Same defect already fixed once for the status LED after it was measured writing the sensor bus
  // 1000x/s. This is the check that stops the third instance.
  Device idle;
  idle.boot();
  idle.controller.enterIdle(idle.now);
  idle.tick(10);
  check(idle.controller.mode() == UiMode::Idle, "the device is idle");
  auto& idlePanel = m5stamplc_stub::board();
  idle.resetFrames();
  const uint32_t backlightBefore = idlePanel.setBacklightCalls;
  // A full second of idle, sampled the way LogicTask actually runs it.
  for (int i = 0; i < 100; ++i) idle.tick(10);
  const uint32_t idleFrames = idlePanel.Display.fillScreens;
  const uint32_t backlightWrites = idlePanel.setBacklightCalls - backlightBefore;
  std::printf("      1 s of idle: %u fillScreen, %u setBacklight\n", idleFrames, backlightWrites);
  check(idleFrames == 0,
        "a second of idle paints NOTHING further — the blank frame was already drawn");
  check(backlightWrites == 0,
        "and writes the backlight expander zero times, keeping the sampler's I2C bus quiet");

  // Waking must re-arm it, or a second sleep would leave the panel lit.
  idle.tap(ButtonInputManager::Button::Down);
  check(idle.controller.mode() != UiMode::Idle, "a tap wakes the device");
  idle.controller.enterIdle(idle.now);
  idle.resetFrames();
  const uint32_t wakeBacklight = idlePanel.setBacklightCalls;
  idle.tick(10);
  check(idlePanel.Display.fillScreens >= 1, "going idle again DOES paint the blank frame");
  check(idlePanel.setBacklightCalls > wakeBacklight, "and does turn the backlight off");

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
/**
 * Walks to the first MODBUS setting, which is now two descents down rather than one.
 *
 * Configuration became three groups — Modbus, Display, Sensors — so P5 lands on the Modbus group
 * entry and the settings live one level inside it. It used to land directly on a leaf setting because
 * the root was a flat list of everything.
 */
bool walkToModbusSettings(Device& dev) {
  for (int i = 0; i < 16 && dev.controller.page() != UiPage::EnterConfiguration; ++i) {
    dev.tap(ButtonInputManager::Button::Down);
  }
  if (dev.controller.page() != UiPage::EnterConfiguration) {
    return false;
  }
  dev.tap(ButtonInputManager::Button::Enter);   // -> config-modbus, the group entry
  if (!onScreen(dev, "config-modbus")) {
    return false;
  }
  dev.tap(ButtonInputManager::Button::Enter);   // -> config-modbus-slave-id, inside it
  return onScreen(dev, "config-modbus-slave-id");
}

void configListPagingTests() {
  std::printf("\n[config list pages are not editors — §5.1, §5.4]\n");

  Device dev;
  dev.boot();
  check(walkToModbusSettings(dev), "two descents from P5 reach the first Modbus setting");
  check(dev.controller.navigator().depth() == 2,
        "M1 sits two levels below the info ring: the root holds groups, not settings");
  check(!dev.controller.editor().active,
        "descending onto the M1 LIST page does not open a value editor");

  // §5.1: "UP/DOWN move within the current level." A 330 ms press is past the 250 ms first
  // acceleration interval, so a live editor eats it; a navigation screen must page.
  // The Modbus level is its own ring: four settings then BACK. The LED settings are NOT here any
  // more — they live under Display, which is the point of the grouping.
  static constexpr const char* kRing[] = {"config-modbus-baud-rate", "config-modbus-parity",
                                          "config-modbus-stop-bits", "config-modbus-back"};
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
  check(paged, "a 330 ms DOWN pages M1 -> M2 -> M3 -> M4 -> BACK");
  check(!everEditing, "no editor is live anywhere along the Modbus ring");
  check(dev.controller.navigator().depth() == 2, "paging siblings did not change depth");
  check(harness::writes.empty(), "paging the Modbus ring wrote no Modbus register");
}

/**
 * A HELD UP/DOWN navigates NOWHERE, and this is where that is written down.
 *
 * §3.1 said a held UP/DOWN repeats the navigation step every 250 ms, until §3.1.1 withdrew that step on
 * 2026-08-17. `button_input.cpp` still emits those repeats from the 1.5 s threshold, because §5.4's ramp
 * needs a held button, and nothing on the flow-table path answers them. `mapGesture` maps a repeat to
 * `FlowGesture::Hold`, `matchFlow` requires an exact gesture match with no fallback, and the generated
 * table declares no hold flow on any screen — so every repeat is popped, matched against nothing, and
 * dropped. That emitted-but-unanswered repeat is now the SPECIFIED behaviour, so what follows pins a
 * requirement rather than recording a gap: declaring a hold flow to "close" it would fail this test.
 *
 * It cost a real bug report. The web simulator invented the missing half — it answered a repeat with the
 * screen's SHORT flow — so holding UP on a setting page carried the operator several settings away from
 * the one they were reading, and the panel's own button legend documented that invention as fact. The
 * preview was corrected to match `matchFlow`; this test is what stops the firmware drifting the other way
 * without anyone noticing, and what tells the next person that adding a hold flow changes these answers.
 */
void heldRepeatScopeTests() {
  std::printf("\n[a held UP/DOWN navigates nowhere — §3.1.1, amended 2026-08-17]\n");

  // The premise first, read off the table the firmware actually runs. Every expectation below follows
  // from it, so if this ever changes the rest of the suite should be re-read rather than patched.
  std::size_t holdFlows = 0;
  std::size_t buttonFlows = 0;
  for (std::size_t i = 0; i < ui_exporter::kGeneratedScreenCount; ++i) {
    const auto& screen = ui_exporter::kGeneratedScreens[i];
    for (std::size_t f = 0; f < screen.flowCount; ++f) {
      const auto& flow = screen.flows[f];
      if (flow.trigger != ui_exporter::FlowTrigger::Button) continue;
      ++buttonFlows;
      if (flow.gesture == ui_exporter::FlowGesture::Hold) ++holdFlows;
    }
  }
  check(buttonFlows > 100, "the table has button flows at all, so a zero below means something");
  if (holdFlows != 0) {
    std::printf("    %zu hold flow(s) now exist — a held UP/DOWN can navigate again\n", holdFlows);
  }
  check(holdFlows == 0, "no screen declares a hold flow, so no repeat can ever match");

  // ── On the info ring ───────────────────────────────────────────────────────────────────
  Device ring;
  ring.boot();
  const std::string ringStart = currentId(ring);
  // 2.1 s: past the 1.5 s long-press threshold, with two 250 ms repeats behind it.
  ring.hold(ButtonInputManager::Button::Down, 2100);
  check(currentId(ring) == ringStart,
        "a 2.1 s DOWN on the info ring moves nowhere: the long press and both repeats match nothing");
  check(ring.controller.navigator().depth() == 0, "and it did not descend either");
  ring.tap(ButtonInputManager::Button::Down);
  check(currentId(ring) != ringStart, "while a TAP pages the ring as it always has");

  // ── Inside a configuration level ───────────────────────────────────────────────────────
  Device config;
  config.boot();
  check(walkToModbusSettings(config), "two descents from P5 reach M1");
  const std::string settingStart = currentId(config);
  config.hold(ButtonInputManager::Button::Down, 3000);
  check(currentId(config) == settingStart,
        "a 3 s DOWN on a Modbus setting page moves nowhere — the reported 'it brings me to the flow unit'");
  check(config.controller.navigator().depth() == 2, "and the depth is untouched");
  check(!config.controller.editor().active, "the hold did not open an editor either");
  config.tap(ButtonInputManager::Button::Down);
  check(onScreen(config, "config-modbus-baud-rate"), "a tap still pages M1 -> M2");

  // ── The one place a hold DOES act, and it does not come through the queue ──────────────
  config.tap(ButtonInputManager::Button::Enter);
  check(config.controller.editor().active, "ENTER opens the baud-rate editor");
  const int32_t opened = config.controller.editor().pending;
  config.hold(ButtonInputManager::Button::Up, 3000);
  check(config.controller.editor().pending != opened,
        "holding UP inside the editor ramps: §5.4 reads the button levels, not the event queue");
}

void configEditorDescentTests() {
  std::printf("\n[one more level down IS the editor — §5.4, §5.7]\n");

  Device dev;
  dev.boot();
  check(walkToModbusSettings(dev), "at M1");
  dev.tap(ButtonInputManager::Button::Enter);
  check(onScreen(dev, "config-modbus-slave-id-edit"),
        "ENTER-short on M1 descends onto its derived editor screen (§5.7)");
  // Three, not two: root -> Modbus -> M1 -> editor. The grouping added a level above the settings.
  check(dev.controller.navigator().depth() == 3, "the editor is one level below its list page");
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
  // Reaching the Sensors group means walking the CONFIG ROOT, not the Modbus level — the two are
  // different rings now. So descend to the root's first entry and page there.
  for (int i = 0; i < 16 && dev.controller.page() != UiPage::EnterConfiguration; ++i) {
    dev.tap(ButtonInputManager::Button::Down);
  }
  dev.tap(ButtonInputManager::Button::Enter);
  check(onScreen(dev, "config-modbus"), "P5 lands on the config root's first group");
  for (int i = 0; i < 6 && !onScreen(dev, "config-sensors"); ++i) {
    dev.tap(ButtonInputManager::Button::Down);
  }
  check(onScreen(dev, "config-sensors"), "paged along the config root to the Sensors group");

  dev.tap(ButtonInputManager::Button::Enter);
  check(onScreen(dev, "config-sensor-1"), "the Sensors group descends onto the sensor list");
  check(!dev.controller.editor().active,
        "the group carries no value, so the sensor list opens no editor (§5.1)");

  dev.tap(ButtonInputManager::Button::Enter);
  check(onScreen(dev, "config-s1-connected"), "the sensor list descends onto the S ring");
  check(!dev.controller.editor().active,
        "descending onto the S1 LIST page does not open a value editor either");

  // The S ring pages the same way the C ring does, and for the same reason.
  dev.hold(ButtonInputManager::Button::Down, 330);
  check(onScreen(dev, "config-s2-calibration"), "a 330 ms DOWN pages S1 -> S2");
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

    // A row the PANEL CANNOT EDIT must not descend to an editor; everything else must, or the
    // setting is unreachable at the panel. Two kinds cannot be edited here, and the second is new:
    // text, because there is no on-device text entry (§6.3), and every WiFi/MQTT setting, because the
    // owner ruled that the panel only reads them — configuring a broker through three buttons is too
    // painful, and the portal, RS485 and SD file already do it better.
    bool carriesTextSetting = false;
    for (std::size_t e = 0; e < screen.elementCount; ++e) {
      const auto* found = ui::findSetting(screen.elements[e].bindingId);
      if (found && !panelEditable(*found)) {
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
    if (setting && panelEditable(*setting)) ++editableSettings;
  }
  std::printf("      %zu editors / %zu rows (%zu text) / %zu settings, %zu editable\n",
              static_cast<std::size_t>(editors), static_cast<std::size_t>(settingListPages),
              static_cast<std::size_t>(textRows), ui::settingCount(), editableSettings);
  check(static_cast<std::size_t>(editors) == editableSettings,
        "every NON-TEXT setting has exactly one editor screen (the completeness rule)");
  check(pendingMatchesCommit,
        "a screen shows a pending value if and only if it has a commit flow");
  // DISTINCT settings with a row, not a row count. The two were the same number until the §7.1
  // information pages landed: net-mqtt-info displays config.mqtt.host, .port and .baseTopic read-only
  // alongside the editors those settings already have, so a setting can legitimately appear on two
  // screens. "Exactly one row each" was never the requirement — it was an artifact of the generator
  // emitting one row per setting, and asserting it made a correct addition fail.
  //
  // What R7.9c actually needs is COVERAGE: every setting readable somewhere.
  std::size_t settingsWithARow = 0;
  for (std::size_t i = 0; i < ui::settingCount(); ++i) {
    const auto* setting = ui::settingAt(i);
    if (!setting) continue;
    bool shown = false;
    for (std::size_t sc = 0; sc < ui_exporter::kGeneratedScreenCount && !shown; ++sc) {
      const auto& screen = ui_exporter::kGeneratedScreens[sc];
      bool isEditor = false;
      bool carries = false;
      for (std::size_t e = 0; e < screen.elementCount; ++e) {
        const char* b = screen.elements[e].bindingId;
        if (!b) continue;
        if (std::strcmp(b, "config.editor.pending") == 0) isEditor = true;
        if (std::strcmp(b, setting->bindingId) == 0) carries = true;
      }
      if (carries && !isEditor) shown = true;
    }
    if (shown) ++settingsWithARow;
    else std::printf("      %s appears on no non-editor screen\n", setting->bindingId);
  }
  check(settingsWithARow == ui::settingCount(),
        "and every setting is readable on at least one non-editor screen, text included");
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

/**
 * Walks from a fresh boot down to the sensor-settings ring of a chosen 1-BASED channel.
 *
 * Four descents, and the third is the one that matters: `UiNavigator::descend` fixes `sensorIndex_`
 * from the id of the screen being LEFT, so it is paging the sensor LIST to `config-sensor-<n>` before
 * pressing ENTER that decides which channel everything below applies to. A helper that always left from
 * `config-sensor-1` would make every per-sensor test agree with a broken index.
 */
bool walkToSensorSettings(Device& dev, unsigned sensorNumber) {
  for (int i = 0; i < 16 && dev.controller.page() != UiPage::EnterConfiguration; ++i) {
    dev.tap(ButtonInputManager::Button::Down);
  }
  dev.tap(ButtonInputManager::Button::Enter);
  for (int i = 0; i < 8 && !onScreen(dev, "config-sensors"); ++i) {
    dev.tap(ButtonInputManager::Button::Down);
  }
  if (!onScreen(dev, "config-sensors")) return false;
  dev.tap(ButtonInputManager::Button::Enter);

  char wanted[24] = {};
  std::snprintf(wanted, sizeof(wanted), "config-sensor-%u", sensorNumber);
  for (int i = 0; i < 16 && !onScreen(dev, wanted); ++i) {
    dev.tap(ButtonInputManager::Button::Down);
  }
  if (!onScreen(dev, wanted)) return false;
  dev.tap(ButtonInputManager::Button::Enter);
  return dev.controller.navigator().sensorIndex() == sensorNumber;
}

/** Pages the sensor-settings ring to a screen id, stepping over whatever the calibration form hides. */
bool pageSensorRingTo(Device& dev, const char* id) {
  for (int i = 0; i < 12 && !onScreen(dev, id); ++i) {
    dev.tap(ButtonInputManager::Button::Down);
  }
  return onScreen(dev, id);
}

void confirmCountdownTests() {
  std::printf("\n[confirm screens can be confirmed — §4.3]\n");

  Device dev;
  dev.boot();
  check(walkToInfoPage(dev, UiPage::CumulativeCubicMeters), "paged the info ring to P2");
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
  check(dev.controller.navigator().depth() == 0 && onScreen(dev, "info-p2-cumulative-m3"),
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
  check(walkToInfoPage(dev, UiPage::SessionCubicMeters), "paged the info ring to P4");
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
  check(dev.controller.navigator().depth() == 0 && onScreen(dev, "info-p3-session-m3"),
        "and returns to the page the operator started from");
  dev.press(ButtonInputManager::Button::Enter, false);
  dev.tick(30);
}

/**
 * P4's peak reset — the cheapest guard in the system, and a reversal.
 *
 * P4 deliberately claimed no gesture: it had descended into `confirm-reset-session`, which was removed
 * because an operator looking at max flow got a screen warning about session totals, and because the peak
 * is volatile so nothing persistent was there to reset. The second half of that was backwards — volatile
 * is what makes a reset CHEAP, and a channel that spiked once went on showing the spike until the next
 * power cycle unless somebody gave up real data to clear it.
 */
void maxFlowResetTests() {
  std::printf("\n[P4 resets the peak, and only the peak]\n");

  Device dev;
  dev.boot();
  check(walkToInfoPage(dev, UiPage::MaxFlow), "paged the info ring to P4");
  dev.tap(ButtonInputManager::Button::Enter);
  check(onScreen(dev, "confirm-reset-max-flow"),
        "ENTER-short on P4 opens ITS OWN confirm, not the session one");

  harness::writes.clear();
  dev.press(ButtonInputManager::Button::Enter, true);
  for (int i = 0; i < 12; ++i) dev.tick(50);   // ~600 ms in
  check(!wroteOnce(plc::REG_MASTER_RESET_ALL_MAX), "600 ms is not enough — nothing issued yet");

  for (int i = 0; i < 24; ++i) dev.tick(50);   // past 1500 ms
  check(wroteOnce(plc::REG_MASTER_RESET_ALL_MAX),
        "holding ENTER for 1.5 s issues the reset-max-flow command");
  // The distinction that justifies a command of its own: neither of the destructive resets is issued.
  check(!wroteOnce(plc::REG_MASTER_RESET_ALL_MEASURED),
        "and NOT the measured reset, which would take the lifetime total");
  check(!wroteOnce(plc::REG_MASTER_RESET_ALL_SESSION),
        "and NOT the session reset, which would take the session volume");
  check(onScreen(dev, "toast-max-flow-reset"), "its acknowledgement toast is shown");
  for (int i = 0; i < 24; ++i) dev.tick(100);
  check(dev.controller.navigator().depth() == 0 && onScreen(dev, "info-p4-max-flow"),
        "and it returns to P4, the page the operator started from");
  dev.press(ButtonInputManager::Button::Enter, false);
  dev.tick(30);
}

/**
 * S.RESET reaches the reset-calibration command, ON THE CHANNEL THE OPERATOR IS LOOKING AT.
 *
 * What this file can assert and what it deliberately leaves to modbus_reset_calibration_test.cpp: here
 * `ModbusManager::applyHoldingWrite` is the harness stand-in that records the address and returns, so
 * every claim about what the command DOES — the config cleared, the totals kept, the override dropped —
 * belongs to the file that links the real one. What lives here is the half that file cannot see: the
 * gesture, the hold threshold, the navigation, and THE ADDRESS.
 *
 * The address IS the wrong-channel test. `handleResetCalibration` turns a 1-based navigator index into a
 * 0-based register slot, and on channel 1 the off-by-one and the right answer are the same address — so
 * this walks to channel 3 and pins 180 + 25, then states the two addresses a slip in either direction
 * would have produced. Channel 3 rather than channel 1 for exactly that reason.
 */
void resetCalibrationEntryTests() {
  std::printf("\n[S.RESET issues the per-channel calibration reset, on the selected channel]\n");

  Device dev;
  dev.boot();
  check(walkToSensorSettings(dev, 3), "walked to sensor 3's settings ring, leaving from config-sensor-3");
  check(dev.controller.navigator().sensorIndex() == 3, "and the navigator holds channel 3");

  check(pageSensorRingTo(dev, "config-sensor-settings-reset-cal"),
        "paging the S ring reaches the S.RESET entry");
  // The entry joined the ring rather than the settings table, so it must NOT have brought an editor
  // with it: every S1..S6 row has a `-edit` child and this one has nothing to edit.
  check(!dev.controller.editor().active, "which opens no editor, having no value to edit");
  /**
   * EIGHT here, and 6 or 7 on the device — the difference is the harness, not the ring.
   *
   * `UiNavigator::screenVisible` returns true for a gated screen when `visibility_` is unbound, which is
   * the deliberate safe default (hiding a row because the question could not be asked would make a
   * setting unreachable). Only firmware.cpp binds that callback, so every declared member counts here,
   * including the pulses-per-litre row the Formula form hides. On the device the visible ring is 6 with
   * Pulses/L and 7 with Formula.
   *
   * Asserted anyway, because 8 is the number the GEOMETRY depends on: `nav.position` renders "L%u %u/%u"
   * into a 7-character gap, its declared worst case is `L3 8/8` at six, and this entry is what took the
   * ring from 7 to 8. A visible ring of 10 makes that string 8 characters and it collides with the
   * sensor number beside it — which the geometry audit has already caught this field doing once. It is
   * also well under the unguarded `kMaxRing` of 16, whose overflow truncates a ring silently.
   */
  check(dev.controller.context().ringCount == 8,
        "the declared ring is 8 - one more than before, and still six characters of nav.position");
  check(dev.controller.context().ringIndex == 6, "with S.RESET second-to-last, just above < BACK");

  dev.tap(ButtonInputManager::Button::Enter);
  check(onScreen(dev, "confirm-reset-calibration"), "ENTER-short on S.RESET opens its confirm");
  check(dev.controller.navigator().depth() == 4, "which sits at depth 4, alongside the value editors");
  check(dev.controller.navigator().sensorIndex() == 3,
        "and the channel SURVIVES the descent onto a screen whose id names no sensor");

  // 1500 ms is the tier this deliberately does NOT use: the calibration is persisted, unlike the peak.
  harness::writes.clear();
  dev.press(ButtonInputManager::Button::Enter, true);
  for (int i = 0; i < 34; ++i) dev.tick(50);  // ~1700 ms in, past the peak reset's threshold
  check(harness::writes.empty(),
        "1700 ms issues nothing - this is a 3 s hold, not the peak reset's 1.5 s");

  for (int i = 0; i < 30; ++i) dev.tick(50);  // past 3000 ms
  const uint16_t expected =
      static_cast<uint16_t>(plc::sensorBaseAddress(2) + plc::OFF_CMD_RESET_CALIBRATION);
  check(expected == 205, "channel 3's command register is 180 + 25 = 205");
  check(wroteOnce(expected),
        "holding ENTER for 3 s writes 1 to CHANNEL 3's reset-calibration register, and nothing else");

  // The negatives. Each names a specific way of getting this wrong, which is what makes them worth
  // asserting rather than a restatement of the line above.
  check(!wroteOnce(static_cast<uint16_t>(plc::sensorBaseAddress(0) +
                                        plc::OFF_CMD_RESET_CALIBRATION)),
        "not channel 1's - the 1-based index was not used as a 0-based slot");
  check(!wroteOnce(static_cast<uint16_t>(plc::sensorBaseAddress(3) +
                                        plc::OFF_CMD_RESET_CALIBRATION)),
        "nor channel 4's - nor the other way round");
  check(!wroteOnce(static_cast<uint16_t>(plc::sensorBaseAddress(2) + plc::OFF_CMD_RESET_CONFIG)),
        "and NOT offset 19, which would have destroyed the totals it exists to keep");
  check(!wroteOnce(plc::REG_MASTER_RESET_ALL_MEASURED),
        "nor the device-wide measured reset, on eight channels instead of one");
  check(!wroteOnce(plc::REG_MASTER_RESET_ALL_SESSION), "nor the device-wide session reset");

  check(onScreen(dev, "toast-calibration-reset"), "its acknowledgement toast is shown");
  for (int i = 0; i < 24; ++i) dev.tick(100);
  /**
   * It returns to the S.RESET ROW, not to P0.
   *
   * The other resets are reached from a level-0 info page and unwind to depth 0, which is right for
   * them and would be wrong here: the operator's next move is to page back up this very ring and enter
   * the replacement meter's figures, and being thrown out to the status page would mean walking four
   * levels back down first. The toast replaces the confirm at depth 4, so its own `nav.back` lands here.
   */
  check(dev.controller.navigator().depth() == 3 &&
            onScreen(dev, "config-sensor-settings-reset-cal"),
        "and it returns to the S.RESET row, where the new figures are four presses away");
  check(dev.controller.navigator().sensorIndex() == 3, "still on channel 3");
  dev.press(ButtonInputManager::Button::Enter, false);
  dev.tick(30);
}

void confirmAbortTests() {
  std::printf("\n[releasing ENTER before zero aborts — §4.3 note 1]\n");

  Device dev;
  dev.boot();
  check(walkToInfoPage(dev, UiPage::CumulativeCubicMeters), "paged the info ring to P2");
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

  /**
   * A plain tap on a confirm screen is IGNORED — it is not attached to anything.
   *
   * This asserted the opposite until the gesture contract was harmonised: ENTER-short was an `f-exit`
   * meaning "leave without acting", which put the escape one slip away from the hold that acts. Now
   * long-ENTER is the only gesture the screen claims, and leaving is the same motion as leaving any
   * other level — page to `< BACK` and press ENTER.
   *
   * The important half is that the press does NOTHING: `findFlow` returns null for an unclaimed
   * gesture and the handler makes no default, which is what lets long-ENTER stop being a hidden
   * global escape everywhere else.
   */
  dev.tap(ButtonInputManager::Button::Enter);
  check(harness::writes.empty(), "a tap on the confirm screen writes nothing");
  check(onScreen(dev, "confirm-reset-totals") && dev.controller.navigator().depth() == 1,
        "and does not move: ENTER-short is unattached on a confirm, so it is ignored");

  // Leaving is by the level's own BACK entry, reached with DOWN like any other sibling.
  dev.tap(ButtonInputManager::Button::Down);
  check(onScreen(dev, "confirm-reset-totals-back"), "DOWN pages to the confirm's < BACK entry");
  dev.tap(ButtonInputManager::Button::Enter);
  check(dev.controller.navigator().depth() == 0 && onScreen(dev, "info-p2-cumulative-m3"),
        "and ENTER there ascends, landing back on the page the operator came from");

  // §4.3 note 2: "during a countdown, UP/DOWN have no effect" — including not cancelling it.
  Device other;
  other.boot();
  check(walkToInfoPage(other, UiPage::CumulativeCubicMeters), "paged a second device to P2");
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
    if (dev.controller.page() == UiPage::CumulativeCubicMeters) break;
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
  check(afterToast && std::strcmp(afterToast->id, "info-p2-cumulative-m3") == 0,
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


/**
 * The §3.4 root-level entry — the DISCOVERABLE route into the Select Menu.
 *
 * Driven through `Device::tap` throughout, so the whole stack runs: button input, matchFlow against
 * the tail's own flows, the action registry, the navigator's splice and the renderer. That matters
 * more here than anywhere else in this file, because the entry is a firmware-owned
 * `ui_exporter::Screen` that is in NO screen table — every API-level assertion about it would pass
 * against a navigator whose paging never consults the splice at all.
 */
void rootEntryTests() {
  std::printf("\n[the appended Select Menu root entry — §3.4]\n");

  // ── (a) reachable by paging, and leavable in both directions ────────────────────────────────
  {
    Device dev;
    dev.boot();
    auto& nav = dev.controller.navigator();

    int taps = 0;
    while (taps < 12 &&
           std::strcmp(nav.current()->id, ui::kSelectMenuScreenId) != 0) {
      dev.tap(ButtonInputManager::Button::Down);
      taps += 1;
      if (nav.depth() != 0) break;
    }
    check(std::strcmp(nav.current()->id, ui::kSelectMenuScreenId) == 0,
          "paging DOWN from P0 reaches the appended entry");
    check(taps == 9, "and it takes exactly 9 taps — it is at the END of the level, not in the middle");
    check(nav.depth() == 0, "paging to it never leaves depth 0");
    check(nav.current() == &ui::kSelectMenuScreen,
          "the screen really is the firmware-owned constant, not a look-alike");

    uint8_t ri = 0, rc = 0;
    // ringIndex is 0-based and the root anchors on the root, so P0 is 0 and the entry is 9 — the
    // scrollbar reads `L0 9/10`, seven characters, which is the widest nav.position the root can
    // produce. There is no `10/10`.
    check(nav.ringPosition(&ri, &rc) && rc == 10 && ri == 9,
          "it reports index 9 of 10: the last member of a ten-member level");

    dev.tap(ButtonInputManager::Button::Down);
    check(std::strcmp(nav.current()->id, "info-p0-global-status") == 0,
          "one more DOWN wraps to P0");

    // THE ASSERTION THE WHOLE ITEM TURNS ON. The tail's f-prev declares NO target, so a paging path
    // that only acts when `ctx.resolvedTarget` is set moves the UiPage ring and leaves the navigator
    // exactly where it was — every other wire looking connected while UP off the tail does nothing.
    dev.tap(ButtonInputManager::Button::Up);
    check(std::strcmp(nav.current()->id, ui::kSelectMenuScreenId) == 0,
          "a single UP from P0 lands on the appended entry again");
    dev.tap(ButtonInputManager::Button::Up);
    check(std::strcmp(nav.current()->id, "net-mqtt-root") == 0,
          "UP off the appended entry reaches MQTT — the step the navigator, not the flow, decides");
  }

  // ── (b) ENTER converges with the recovery gesture, on one flag ──────────────────────────────
  {
    Device dev;
    dev.boot();
    auto& nav = dev.controller.navigator();
    for (int i = 0; i < 9; ++i) dev.tap(ButtonInputManager::Button::Down);
    check(std::strcmp(nav.current()->id, ui::kSelectMenuScreenId) == 0, "standing on the entry");

    dev.tap(ButtonInputManager::Button::Enter);
    check(dev.selectorOpens == 1,
          "ENTER-short raises openPackSelector — the same counter the gesture moves");
    check(!dev.controller.consumePackSelectorRequest(),
          "and the one-shot was consumed, not left latched");
    for (int i = 0; i < 10; ++i) dev.tick(50);
    check(dev.selectorOpens == 1, "it does not re-fire on subsequent passes");
  }
  {
    // The other route, in its own device: both must land on the one flag rather than two paths.
    Device dev;
    dev.boot();
    dev.holdAllThree(3100);
    check(dev.selectorOpens == 1, "the three-button hold still raises the same flag");
    dev.releaseAllThree();
  }

  // ── (c) it paints, and its geometry is sound ────────────────────────────────────────────────
  //
  // THE ONLY GATE THE ENTRY'S GEOMETRY WILL EVER HAVE. screen-spec.ts and screen-geometry.ts read
  // web/mockup/src/data/screens.json, and this screen is deliberately not in it.
  {
    Device dev;
    dev.boot();
    for (int i = 0; i < 9; ++i) dev.tap(ButtonInputManager::Button::Down);
    dev.resetFrames();
    // Past the 1 s telemetry interval, not 100 ms: the entry is a static menu page, so
    // `context.interactive` is false and the last tap already consumed this screen's
    // change-driven repaint. A 100 ms tick would record no frame and prove nothing.
    dev.tick(1100);
    const std::string& painted = m5stamplc_stub::board().Display.strings;
    check(painted.find("SELECT MENU") != std::string::npos, "the entry paints its title");
    check(painted.find("UP/DN pages  ENTER open") != std::string::npos,
          "and its footer hint, through the ordinary table-driven path");
    check(painted.find("UI ASSET ERROR") == std::string::npos,
          "and never falls into drawAssetError, which a screen outside the table could");

    bool geometryOk = true;
    bool footerBandClear = true;
    for (std::size_t i = 0; i < ui::kSelectMenuElementCount; ++i) {
      const auto& element = ui::kSelectMenuElements[i];
      if (element.x < 0 || element.y < 0) geometryOk = false;
      if (element.type == ui_exporter::ElementType::Text && element.text && element.text->text) {
        const int chars = static_cast<int>(std::strlen(element.text->text));
        if (element.x + chars * 6 > 240) geometryOk = false;
        if (element.y + 8 > 135) geometryOk = false;
      }
      // §2c reserves y=116..134 for the warning banner. The footer hint is the one thing allowed
      // there, and it is allowed knowing the banner covers it while a warning is live.
      if (element.y >= 116 && element.y <= 134 && std::strcmp(element.id, "footer-hint") != 0) {
        footerBandClear = false;
      }
    }
    check(geometryOk, "every element sits on the panel and inside the 40-column row budget");
    check(footerBandClear, "nothing but the footer hint sits in the banner's y=116..134 band");

    // nav.position is the one string on this screen whose width is not fixed by the source.
    char buffer[32] = {};
    const auto& navElement = ui::kSelectMenuElements[1];
    check(navElement.bindingId && std::strcmp(navElement.bindingId, "nav.position") == 0,
          "element 1 is the nav.position text, as the width check below assumes");
    const bool resolved =
        dev.bindings.resolveText(dev.controller.context(), navElement, buffer, sizeof(buffer));
    const int width = static_cast<int>(std::strlen(buffer));
    std::printf("      nav.position on the appended entry: \"%s\" (%d chars)\n", buffer, width);
    // The EXACT string, not just its width: `ringCount == 0` makes the resolver fall back to a bare
    // "L0", which would satisfy any width bound while proving nothing about the wide case.
    check(resolved && std::strcmp(buffer, "L0 9/10") == 0,
          "nav.position reads L0 9/10 — 0-based index, so the tenth member is 9");
    check(width <= 8, "which is inside the 8 characters the row has for it");
    check(navElement.x + width * 6 <= 232,
          "so it stops short of the level scrollbar at x=232");
  }

  // ── (d) no pack can shadow or remove it ────────────────────────────────────────────────────
  {
    Device dev;
    dev.boot();
    bool inDataset = false;
    for (std::size_t i = 0; i < ui_exporter::kGeneratedScreenCount; ++i) {
      const auto& screen = ui_exporter::kGeneratedScreens[i];
      if (screen.id && std::strcmp(screen.id, ui::kSelectMenuScreenId) == 0) inDataset = true;
    }
    check(!inDataset, "the entry is in NO dataset row, so no pack declares or omits it");
    check(dev.router.screenById(ui::kSelectMenuScreenId) == nullptr,
          "and the router cannot resolve it by name, which is what keeps it unshadowable");
    check(dev.controller.navigator().rootTail() == &ui::kSelectMenuScreen,
          "a freshly booted device has it bound with no bindRootTail() call anywhere in boot()");
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

/**
 * The display's network path (§3.4, §7.3).
 *
 * The owner's first request was "the display should be able to display whether or not the wifi is
 * connected (main display) and whether or not the mqtt is connected". Until now the render context
 * had no network fields at all, so none of it could be shown.
 */
void checkStr(const char* actual, const char* expected, const char* what) {
  const bool ok = actual != nullptr && std::strcmp(actual, expected) == 0;
  check(ok, what);
  if (!ok) std::printf("        got \"%s\" want \"%s\"\n", actual ? actual : "(null)", expected);
}

/**
 * Resolves one binding against a device's live render context, the way the renderer does.
 *
 * Free rather than a lambda inside each test because the session-start cases need more than one
 * Device: two of the four states are only reachable from a DIFFERENT boot, since DeviceClock has no
 * way back from trusted to unset — which is the point of the class.
 */
const char* renderBinding(Device& dev, const char* bindingId) {
  ui_exporter::Element el{};
  el.id = "probe";
  el.type = ui_exporter::ElementType::Value;
  el.bindingId = bindingId;
  static char out[80];
  out[0] = '\0';
  dev.bindings.resolveText(dev.controller.context(), el, out, sizeof(out));
  return out;
}

void sessionStartRenderTests() {
  std::printf("\n[P3 says WHEN the session began, or which of three reasons it cannot — all four]\n");

  /** 2026-08-12T14:32:07Z. `date -u -d @1786545127` -> Wed Aug 12 14:32:07 UTC 2026. */
  constexpr uint32_t kEpoch = 1786545127u;

  // ── State 1: no clock, and no reset waiting to be dated ───────────────────────
  //
  // The boot state of a device whose RX8130CE lost power. DeviceClock is left as the harness
  // constructs it, which is exactly that.
  Device dev;
  dev.boot();
  dev.tick(10);
  check(!dev.controller.context().clockSet, "the context reports no clock");
  check(dev.controller.context().sessionStartEpoch == 0, "and no session start");
  checkStr(renderBinding(dev, "telemetry.sessionStart"), "CLOCK UNSET",
           "no clock and nothing waiting renders CLOCK UNSET");

  // ── State 2: a reset happened with no clock to date it ────────────────────────
  //
  // Setting the clock WILL fill this in retroactively, which is the whole reason it is not the same
  // string as state 1 — the operator gets an action that works.
  dev.clock.noteSessionStart(dev.now);
  dev.tick(10);
  check(dev.controller.context().sessionStartAwaitingClock, "the context reports the wait");
  checkStr(renderBinding(dev, "telemetry.sessionStart"), "AWAITING CLOCK",
           "a reset with no clock renders AWAITING CLOCK, not the same string as state 1");

  // ── State 3: the operator supplies a time, and the waiting reset gets dated ────
  check(dev.clock.setTime(kEpoch, plc::ClockSource::Operator, dev.now), "the operator sets the time");
  dev.tick(10);
  checkStr(renderBinding(dev, "telemetry.sessionStart"), "2026-08-12 14:32 UTC",
           "which turns into the timestamp — and never a 1970 or a bare zero");
  check(!dev.controller.context().sessionStartAwaitingClock, "with nothing left waiting");

  // ── State 4: a trusted clock, but nothing ever recorded a start ────────────────
  //
  // A fresh boot, because a clock cannot become untrusted again. This is the case no further sync can
  // fix — only a reset produces a timestamp here — which is why it says UNKNOWN rather than CLOCK UNSET.
  Device trusted;
  trusted.boot();
  trusted.clock.noteBootTrust(false, kEpoch, trusted.now);
  trusted.tick(10);
  check(trusted.controller.context().clockSet, "a trusted RTC at boot");
  check(trusted.controller.context().sessionStartEpoch == 0, "with no session start recorded");
  check(!trusted.controller.context().sessionStartAwaitingClock, "and nothing awaiting a clock");
  checkStr(renderBinding(trusted, "telemetry.sessionStart"), "UNKNOWN",
           "renders UNKNOWN — distinct from CLOCK UNSET, because a sync would not help");

  // And the same clock, once a reset dates it, on the page that shows it.
  trusted.clock.noteSessionStart(trusted.now);
  trusted.tick(10);
  checkStr(renderBinding(trusted, "telemetry.sessionStart"), "2026-08-12 14:32 UTC",
           "a reset on a trusted clock dates it immediately");

  /**
   * And the part no resolver test can prove: that the string reaches the PANEL.
   *
   * A binding can resolve perfectly and still be invisible, because P3's geometry comes from its spec
   * file and nothing fails an export when a value is advertised but bound by no element — that is the
   * blank-on-device failure this pipeline keeps producing. So this navigates to P3 and reads what was
   * actually painted.
   */
  check(walkToInfoPage(trusted, UiPage::SessionCubicMeters), "paged the info ring to P3");
  trusted.resetFrames();
  // Past kRefreshIntervalMs, and the frame count is asserted before the text is: a static page repaints
  // only once a second, so a 100 ms tick leaves `strings` empty and both text assertions below would
  // fail for having painted NOTHING rather than for painting the wrong thing. That is how the first
  // version of this check failed, and reading the count first is what tells the two apart.
  trusted.tick(1100);
  check(trusted.frames() > 0, "P3 repainted, so there is a frame to inspect");
  const std::string& painted = m5stamplc_stub::board().Display.strings;
  check(painted.find("2026-08-12 14:32 UTC") != std::string::npos,
        "and P3 actually PAINTS the session start, not merely resolves it");
  check(painted.find("Since") != std::string::npos, "beside the label that says what it is");
}

void networkBindingTests() {
  std::printf("\n[the display's network bindings]\n");
  Device dev;
  dev.boot();

  // Delegates to the free helper above rather than keeping a second copy of the same six lines.
  auto render = [&](const char* bindingId) { return renderBinding(dev, bindingId); };

  // ── Disabled: everything says so, and nothing pretends ────────────────────────
  dev.tick(10);
  checkStr(render("net.wifi.state"), "OFF", "a disabled radio renders OFF");
  checkStr(render("net.wifi.ip"), "---",
           "and no address renders --- rather than 0.0.0.0, which would read as configured");
  checkStr(render("net.wifi.rssi"), "--", "RSSI is withheld when not associated, not shown as 0");
  checkStr(render("net.mqtt.state"), "OFF", "MQTT off says OFF");
  checkStr(render("net.status"), "WiFi OFF  MQTT OFF", "the main-screen summary carries both halves");

  // ── Associated ────────────────────────────────────────────────────────────────
  dev.netStatus.wifiState = plc::WifiState::Connected;
  dev.netStatus.ipAddress = (192u << 24) | (168u << 16) | (1u << 8) | 50u;
  dev.netStatus.rssiDbm = -57;
  std::snprintf(dev.netStatus.ssid, sizeof(dev.netStatus.ssid), "%s", "PlantFloor");
  dev.tick(10);
  checkStr(render("net.wifi.ssid"), "PlantFloor", "the SSID renders");
  // The octet order is the hazard: a cast of IPAddress' raw word would print 50.1.168.192.
  checkStr(render("net.wifi.ip"), "192.168.1.50", "and the address in the right octet order");
  checkStr(render("net.wifi.rssi"), "-57", "RSSI appears once associated");

  // ── MQTT: three distinguishable states, not two ────────────────────────────────
  dev.netStatus.mqttEnabled = true;
  dev.tick(10);
  checkStr(render("net.mqtt.state"), "UNSET",
           "enabled with no broker configured says UNSET — 'finish setting it up'");
  dev.netStatus.mqttConfigured = true;
  dev.tick(10);
  checkStr(render("net.mqtt.state"), "DOWN",
           "configured but not connected says DOWN — 'go look at the broker'");
  dev.netStatus.mqttConnected = true;
  dev.tick(10);
  checkStr(render("net.mqtt.state"), "OK", "and connected says OK");
  checkStr(render("net.status"), "WiFi OK  MQTT OK", "the summary tracks both");

  // ── The same four states, as register 561 ──────────────────────────────────────
  //
  // 561 was declared read-only, documented in the register wiki as "broker connection state" and
  // written by NOTHING: `NetRegisterMap::publish` packs the settings and zeroes the rest of its
  // 233-register block, so a Modbus master polling MQTT state read 0 — "disabled" — on a device
  // happily publishing telemetry. Giving it a value meant naming the states, and the panel above had
  // already named them, so both now come from `mqttLinkState`. These checks exist to keep it that
  // way: two implementations of one decision is how the panel and the bus end up describing
  // different devices.
  check(plc::mqttLinkState(false, true, true) == plc::MqttLinkState::Off,
        "disabled outranks everything — a switched-off client cannot be connected");
  check(plc::mqttLinkState(true, false, false) == plc::MqttLinkState::Unset, "561 = 1 when unset");
  check(plc::mqttLinkState(true, true, false) == plc::MqttLinkState::Down, "561 = 2 when down");
  check(plc::mqttLinkState(true, true, true) == plc::MqttLinkState::Ok, "561 = 3 when connected");
  // The wire values themselves, because an integrator's template reads the numbers and I2 makes them
  // append-only. A reordering that kept the names would pass every check above.
  check(static_cast<uint16_t>(plc::MqttLinkState::Off) == 0 &&
            static_cast<uint16_t>(plc::MqttLinkState::Unset) == 1 &&
            static_cast<uint16_t>(plc::MqttLinkState::Down) == 2 &&
            static_cast<uint16_t>(plc::MqttLinkState::Ok) == 3,
        "and the four wire values are 0..3, in that order (I2)");
  // The agreement itself: whatever the panel renders is what `mqttLinkStateText` returns for the
  // state the register carries. Checked through the live snapshot, not by restating the mapping.
  checkStr(render("net.mqtt.state"),
           plc::mqttLinkStateText(plc::mqttLinkState(dev.netStatus.mqttEnabled,
                                                    dev.netStatus.mqttConfigured,
                                                    dev.netStatus.mqttConnected)),
           "the panel renders exactly what the register's state spells");

  // ── R4.4.2d — the command result, on the panel ────────────────────────────────
  checkStr(render("net.mqtt.lastCommandResult"), "idle",
           "a device that has had no command says idle, not a success it never had");
  dev.netStatus.mqttLastCommandResult = static_cast<uint8_t>(plc::MqttCommandResult::RetainedIgnored);
  dev.tick(10);
  checkStr(render("net.mqtt.lastCommandResult"), "retained-ignored",
           "and a retained command that was discarded says so — R4.4.2d's whole point");
  check(std::strlen(render("net.mqtt.lastCommandResult")) <= 26,
        "the longest result fits the 26 characters the value column has at x=84");
  dev.netStatus.mqttLastCommandResult = static_cast<uint8_t>(plc::MqttCommandResult::RateLimited);
  dev.tick(10);
  checkStr(render("net.mqtt.lastCommandResult"), "rate-limited",
           "and a refused repeat reads rate-limited rather than looking like nothing happened");

  // ── The AP, and R5.3's deliberate asymmetry ───────────────────────────────────
  dev.netStatus.apIpAddress = (192u << 24) | (168u << 16) | (4u << 8) | 1u;
  std::snprintf(dev.netStatus.apSsid, sizeof(dev.netStatus.apSsid), "%s", "water_flow_meter_309245");
  std::snprintf(dev.netStatus.apPassword, sizeof(dev.netStatus.apPassword), "%s", "KU67QJ4DRPDP");
  dev.netStatus.portalRemainingS = 540;
  dev.tick(10);
  checkStr(render("net.ap.ssid"), "water_flow_meter_309245", "the AP name follows R7.5a's shape");
  // NOT masked, and that is the requirement rather than an oversight: the device is broadcasting this
  // network, so anyone in range already sees it, and an operator at the panel must read the key off.
  checkStr(render("net.ap.password"), "KU67QJ4DRPDP",
           "the AP key renders in CLEAR — R5.3, unlike the operator's own passphrase");
  checkStr(render("net.ap.ip"), "192.168.4.1", "and the address to browse to");
  checkStr(render("net.portal.remaining"), "540", "with the R7.6 window counting down");

  // ── Every declared net.* value must actually resolve ──────────────────────────
  //
  // The catalogue header used to claim -Werror=switch made an unresolvable value a build failure. It
  // does not: nothing switches on ValueSource, and the resolver dispatches on the binding string. The
  // export gate catches it, but that runs at export time — this catches it in the suite.
  std::size_t netValues = 0;
  bool allResolve = true;
  for (std::size_t i = 0; i < ui::simpleValueCount(); ++i) {
    const auto* v = ui::simpleValueAt(i);
    if (!v || v->source != ui::ValueSource::Network) continue;
    ++netValues;
    char out[80] = {};
    ui_exporter::Element el{};
    el.id = "probe";
    el.type = ui_exporter::ElementType::Value;
    el.bindingId = v->id;
    if (!dev.bindings.resolveText(dev.controller.context(), el, out, sizeof(out))) {
      allResolve = false;
      std::printf("      UNRESOLVED %s\n", v->id);
    }
  }
  std::printf("      %zu network values declared\n", netValues);
  check(netValues == 11, "eleven network values are declared");
  check(allResolve, "and every one of them resolves — no declared value renders blank");
}

/**
 * EVERY element binding on EVERY screen of the real table, resolved through the real resolver.
 *
 * This closes the highest-consequence hole in the UI pipeline. A catalogue entry with no
 * `ui_bindings.cpp` arm only WARNS in the exporter — `firmware-manifest-resolvable` emits
 * `status: "warning"` on purpose, because a dataset may legitimately be authored ahead of the firmware
 * — so it compiles, ships, and renders BLANK on hardware while looking perfect in the mockup, which
 * has its own sample values. Nothing downstream of that warning failed. Now something does.
 *
 * `networkBindingTests` above sweeps the eleven `ValueSource::Network` catalogue entries. This sweeps
 * the 255 bound ELEMENTS actually placed on screens, which is the set an operator can see.
 *
 * ── TWO KINDS OF NOT-A-STRING, AND ONLY ONE IS A BUG ─────────────────────────────────────
 *
 * `resolveText` returning FALSE means no arm claimed the binding: the element draws nothing, ever.
 * That is always a bug — except for bindings whose value only EXISTS in a particular state, and there
 * are exactly two of those. They are listed with reasons, the list is asserted to be exact in both
 * directions, and each is then proved to resolve once its state exists — otherwise "state-dependent"
 * would be an excuse rather than an explanation.
 *
 * Returning TRUE with an empty string is different: the arm exists and deliberately renders nothing.
 * Two bindings do that, also listed, for the same reason and with the same exactness check.
 */
void everyScreenBindingResolvesTests() {
  std::printf("\n[every binding on every screen resolves — the blank-on-hardware hole]\n");

  struct Exempt { const char* binding; const char* why; };

  // Resolve to FALSE on a cold device because their value does not exist yet.
  static constexpr Exempt kStateDependent[] = {
      {"config.editor.pending",
       "there is no pending value until an editor is open; proved below by opening one"},
      {"countdown.value",
       "there is no countdown until ENTER is held on a confirm screen; proved below by holding it"}};

  // Resolve to TRUE and render nothing, deliberately.
  static constexpr Exempt kLegitimatelyEmpty[] = {
      {"config.editor.range",
       "a reading page shows no domain; ui_bindings resolves it per-SCREEN and draws nothing off an "
       "editor, which is why it is empty here rather than absent"},
      {"config.sensor.nyquistWarning",
       "§5.5's prompt text, empty unless a commit is parked awaiting an override — the element keeps "
       "whatever static text it was authored with"}};

  const auto isExempt = [](const Exempt* list, std::size_t count, const char* binding) {
    for (std::size_t i = 0; i < count; ++i) {
      if (std::strcmp(list[i].binding, binding) == 0) return true;
    }
    return false;
  };

  Device dev;
  dev.boot();
  dev.tick(10);

  std::size_t bound = 0;
  bool everyFailureIsExempt = true;
  bool everyEmptyIsExempt = true;
  std::vector<std::string> failedCold;   // distinct bindings that resolved false
  std::vector<std::string> emptyCold;    // distinct bindings that resolved to ""
  const auto note = [](std::vector<std::string>& into, const char* id) {
    for (const auto& x : into) if (x == id) return;
    into.push_back(id);
  };

  for (std::size_t i = 0; i < ui_exporter::kGeneratedScreenCount; ++i) {
    const auto& screen = ui_exporter::kGeneratedScreens[i];
    for (std::size_t e = 0; e < screen.elementCount; ++e) {
      const auto& element = screen.elements[e];
      if (!element.bindingId) continue;
      ++bound;
      char out[96] = {};
      if (!dev.bindings.resolveText(dev.controller.context(), element, out, sizeof(out))) {
        note(failedCold, element.bindingId);
        if (!isExempt(kStateDependent, sizeof(kStateDependent) / sizeof(kStateDependent[0]),
                      element.bindingId)) {
          std::printf("      UNRESOLVED %s on %s — nothing will ever draw here\n", element.bindingId,
                      screen.id);
          everyFailureIsExempt = false;
        }
      } else if (out[0] == '\0') {
        note(emptyCold, element.bindingId);
        if (!isExempt(kLegitimatelyEmpty,
                      sizeof(kLegitimatelyEmpty) / sizeof(kLegitimatelyEmpty[0]),
                      element.bindingId)) {
          std::printf("      EMPTY %s on %s — resolves, renders nothing\n", element.bindingId,
                      screen.id);
          everyEmptyIsExempt = false;
        }
      }
    }
  }

  std::printf("      %zu bound elements swept across %zu screens\n", bound,
              ui_exporter::kGeneratedScreenCount);
  check(bound > 200, "the sweep really covered the table rather than an empty subset");
  check(everyFailureIsExempt,
        "every binding that resolves to nothing is one of the two the list explains");
  check(everyEmptyIsExempt, "and every binding that renders empty is one of the two allowed to");

  // ── The exemption lists are EXACT, so neither can rot into a blanket excuse ──────────
  for (const auto& entry : kStateDependent) {
    bool actuallyFailed = false;
    for (const auto& id : failedCold) if (id == entry.binding) { actuallyFailed = true; break; }
    check(actuallyFailed,
          "a state-dependent exemption still describes a binding that really does fail cold");
  }
  for (const auto& entry : kLegitimatelyEmpty) {
    bool actuallyEmpty = false;
    for (const auto& id : emptyCold) if (id == entry.binding) { actuallyEmpty = true; break; }
    check(actuallyEmpty, "an empty-allowed exemption still describes a binding that really is empty");
  }

  // ── And the state-dependent two DO resolve once their state exists ───────────────────
  //
  // Without these, "state-dependent" would be indistinguishable from "has no resolver arm" — which is
  // exactly the failure this whole function exists to catch.
  {
    Device editing;
    editing.boot();
    check(descendToAnEditor(editing), "an editor opens, so config.editor.pending has a value to show");
    ui_exporter::Element probe{};
    probe.id = "probe";
    probe.type = ui_exporter::ElementType::Value;
    probe.bindingId = "config.editor.pending";
    char out[96] = {};
    const bool resolved =
        editing.bindings.resolveText(editing.controller.context(), probe, out, sizeof(out));
    check(resolved && out[0] != '\0',
          "config.editor.pending resolves to a real string once an editor is open");
  }
  {
    Device counting;
    counting.boot();
    check(walkToInfoPage(counting, UiPage::CumulativeCubicMeters), "paged to P2");
    counting.tap(ButtonInputManager::Button::Enter);
    counting.press(ButtonInputManager::Button::Enter, true);
    for (int i = 0; i < 6; ++i) counting.tick(50);
    check(counting.controller.context().countdownActive, "a countdown is running");
    ui_exporter::Element probe{};
    probe.id = "probe";
    probe.type = ui_exporter::ElementType::Value;
    probe.bindingId = "countdown.value";
    char out[96] = {};
    const bool resolved =
        counting.bindings.resolveText(counting.controller.context(), probe, out, sizeof(out));
    check(resolved && out[0] != '\0',
          "countdown.value resolves to a real string once a countdown is running");
  }
}

/**
 * A channel nobody has calibrated is a COMMISSIONING GAP, and the summary must say so.
 *
 * The gap these pin: `warningCount` came from `REG_UNDERSAMPLING_FLAGS` alone, and
 * `evaluateSensorDiagnostics` only ever flags a channel whose configuration is VALID
 * (`valid && !meetsNyquistLimit`) — so an uncalibrated channel could not raise a bit, and a device
 * whose eight channels all sat at `SET?` reported "All sensors ready" beside rows saying otherwise.
 *
 * A valid configuration here is q_max and f_multiplier both non-zero on the Formula form, which is
 * exactly what `configIsValid` asks for; the undersampling bits are set on the harness rather than
 * provoked through the Nyquist check, because what is under test is the SUMMARY's arithmetic, not
 * the diagnostic that produces the bits.
 */
void commissioningSummaryTests() {
  std::printf("\n[an uncalibrated channel is counted, and outranks a sampling warning]\n");

  SensorCharacteristics calibrated{};
  calibrated.q_max = 150;
  calibrated.f_multiplier = 10;

  Device dev;
  dev.boot();

  // ── Every channel in use, none calibrated: the harness's own boot state ────────
  //
  // `configs[8] = {}` is q_max = 0 throughout, which is what a device out of the box holds.
  dev.tick(10);
  check(dev.controller.context().uncalibratedCount == 8, "eight in-use channels, none calibrated");
  checkStr(renderBinding(dev, "telemetry.status"), "8 channels not calibrated",
           "the summary counts them rather than claiming readiness");
  checkStr(renderBinding(dev, "legend.warning"), "8 channels not calibrated",
           "and the legend row says the same thing, not \"All sensors nominal\"");

  // ── Calibrate all but three ───────────────────────────────────────────────────
  for (std::size_t i = 3; i < plc::kNumSensors; ++i) {
    dev.configs[i] = calibrated;
  }
  dev.tick(10);
  check(dev.controller.context().uncalibratedCount == 3, "three left uncalibrated");
  checkStr(renderBinding(dev, "telemetry.status"), "3 channels not calibrated",
           "N channels uncalibrated says N");

  // ── One left: the plural has to go ────────────────────────────────────────────
  dev.configs[1] = calibrated;
  dev.configs[2] = calibrated;
  dev.tick(10);
  checkStr(renderBinding(dev, "telemetry.status"), "1 channel not calibrated",
           "one channel is singular — the same %s the warning count already used");

  // ── PRECEDENCE: a sampling fault as well ──────────────────────────────────────
  //
  // Channels 5 and 6 undersampling (bits 4 and 5) while channel 1 is still uncalibrated. Both facts
  // are reported, uncalibrated FIRST, and the counts stay separate: "3 warnings" for one
  // uncalibrated channel and two under-sampling ones would tell the operator neither.
  dev.undersamplingFlags = 0x30;
  dev.tick(10);
  checkStr(renderBinding(dev, "telemetry.status"), "1 channel not calibrated | 2 warnings",
           "commissioning gap leads, sampling faults follow, neither absorbed into the other");
  checkStr(renderBinding(dev, "legend.warning"), "1 not calibrated, 2 undersampling",
           "the banner's string reports both too — and drops the noun \"channels\" to fit its 37");
  check(dev.controller.context().hasWarnings,
        "hasWarnings still means a SAMPLING fault — the banner's gate is now bannerActive(), "
        "and this field was NOT widened");

  // One of each: both singulars at once, which is the case a `%s` on the wrong count would survive.
  dev.undersamplingFlags = 0x10;
  dev.tick(10);
  checkStr(renderBinding(dev, "telemetry.status"), "1 channel not calibrated | 1 warning",
           "one of each is singular twice over");
  checkStr(renderBinding(dev, "legend.warning"), "1 not calibrated, 1 undersampling",
           "and the legend counts them without pluralising a count of one either");

  // ── Sampling only: a COUNT, not a channel list ────────────────────────────────
  //
  // This line used to name the flagged channels, and nothing bounded it: the prefix was 28 characters,
  // a k-channel list is 3k-2, so it passed the banner's 37 columns at FOUR flagged channels and reached
  // 50 at eight — silently, because drawWarningBanner has no `~` clipping. The count is 29 at worst.
  dev.configs[0] = calibrated;
  dev.undersamplingFlags = 0x30;
  dev.tick(10);
  check(dev.controller.context().uncalibratedCount == 0, "nothing uncalibrated now");
  checkStr(renderBinding(dev, "telemetry.status"), "2 warnings",
           "a sampling-only device reads exactly as it did before this change");
  checkStr(renderBinding(dev, "legend.warning"), "Sampling warning on 2 sensors",
           "and its summary counts them instead — identity stays on the warning-coloured rows");

  // One flagged channel: the same `%s` plural idiom the uncalibrated branch beside it already uses.
  dev.undersamplingFlags = 0x10;
  dev.tick(10);
  checkStr(renderBinding(dev, "legend.warning"), "Sampling warning on 1 sensor",
           "one flagged channel is singular, like the uncalibrated line above it");

  // ── Everything calibrated, nothing flagged: all-ready survives ────────────────
  dev.undersamplingFlags = 0;
  dev.tick(10);
  checkStr(renderBinding(dev, "telemetry.status"), "All sensors ready",
           "a fully commissioned, unflagged device still says so");
  checkStr(renderBinding(dev, "legend.warning"), "All sensors nominal",
           "and the legend row keeps its own wording for that state");

  // ── A DISCONNECTED channel is ABSENT, not uncalibrated ────────────────────────
  //
  // The distinction the whole count rests on. Channels 2-8 are switched out of the bitmap and their
  // configurations wiped: a device with one sensor wired and seven bare terminals has ONE channel to
  // commission, and reporting eight problems would bury the one that is real.
  dev.connectedBitmap = 0x01;
  for (std::size_t i = 1; i < plc::kNumSensors; ++i) {
    dev.configs[i] = SensorCharacteristics{};
  }
  dev.tick(10);
  check(dev.controller.context().uncalibratedCount == 0,
        "seven disconnected, uncalibrated channels count for nothing");
  checkStr(renderBinding(dev, "telemetry.status"), "All sensors ready",
           "so a one-sensor installation is ready when its one sensor is");
  checkStr(renderBinding(dev, "sensor.2.status"), "--",
           "and the row for one of them still says NOT IN SERVICE rather than SET?");

  // The one that IS wired, left uncalibrated: one problem, not eight.
  dev.configs[0] = SensorCharacteristics{};
  dev.tick(10);
  check(dev.controller.context().uncalibratedCount == 1, "only the wired channel is counted");
  checkStr(renderBinding(dev, "telemetry.status"), "1 channel not calibrated",
           "one wired channel needing setup reports one problem");
  checkStr(renderBinding(dev, "sensor.1.status"), "SET?",
           "and its own row agrees — the summary is the rows, counted");

  // ── Nothing in use at all: the factory-fresh device ───────────────────────────
  //
  // The connected bitmap comes out of NVS with a default of 0 (firmware.cpp), so this is the state a
  // device ships in. Both counts are legitimately zero, which is why "All sensors ready" survived the
  // count alone and needed a state of its own.
  dev.connectedBitmap = 0;
  dev.tick(10);
  check(dev.controller.context().uncalibratedCount == 0, "no channel is in use, so none is counted");
  checkStr(renderBinding(dev, "telemetry.status"), "No channels in use",
           "a device with nothing wired does not claim its sensors are ready");
  checkStr(renderBinding(dev, "legend.warning"), "No channels in use",
           "and neither does the legend row");

  // ── Every string fits the panel ───────────────────────────────────────────────
  //
  // 40 characters is one 6 px row; the banner starts at x=16 and holds 37. Asserted rather than
  // eyeballed because the worst case only appears with eight channels of each kind at once.
  dev.connectedBitmap = 0xFF;
  dev.undersamplingFlags = 0xFF;
  dev.tick(10);
  checkStr(renderBinding(dev, "telemetry.status"), "8 channels not calibrated | 8 warnings",
           "the widest line the summary can draw");
  checkStr(renderBinding(dev, "legend.warning"), "8 not calibrated, 8 undersampling",
           "and the widest the banner can — the one line that cannot afford the noun");
  const std::size_t statusLen = std::strlen(renderBinding(dev, "telemetry.status"));
  const std::size_t legendLen = std::strlen(renderBinding(dev, "legend.warning"));
  std::printf("      worst case: status %zu chars, legend %zu chars\n", statusLen, legendLen);
  check(statusLen <= 40, "the widest telemetry.status fits a 40-column row");
  check(legendLen <= 37, "the widest legend.warning fits the banner's 37 columns from x=16");

  // ── The SAMPLING-ONLY worst case, which no state above had ever reached ────────
  //
  // The width guard immediately above sets 0xFF/0xFF with every config WIPED, so it routes to the
  // COMBINED branch — which means `legendLen <= 37` has never once evaluated the sampling-only line, and
  // passed for the wrong reason while that line was 50 characters at eight channels. Calibrating all
  // eight while leaving every flag set is the only state that reaches it.
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    dev.configs[i] = calibrated;
  }
  dev.tick(10);
  check(dev.controller.context().uncalibratedCount == 0,
        "all eight calibrated, so this is provably the sampling-only branch");
  check(dev.controller.context().warningCount == 8, "and all eight are flagged");
  checkStr(renderBinding(dev, "legend.warning"), "Sampling warning on 8 sensors",
           "eight flagged channels report a count, which is the same length at any k");
  const std::size_t samplingLen = std::strlen(renderBinding(dev, "legend.warning"));
  std::printf("      sampling-only worst case: legend %zu chars\n", samplingLen);
  check(samplingLen <= 37, "and it fits the banner's 37 columns, which the channel list did not");
}

/** True if any fillRect this frame was the banner's band — x=0, y=116, full width, 18 px tall. */
bool bannerBandPainted() {
  for (const auto& rect : m5stamplc_stub::board().Display.rects) {
    if (rect.x == 0 && rect.y == 116 && rect.w == 240 && rect.h == 18) {
      return true;
    }
  }
  return false;
}

/** Where a string landed this frame, or nullptr if it was not painted. */
const m5stamplc_stub::DisplayRecorder::Placed* placedText(const char* text) {
  for (const auto& entry : m5stamplc_stub::board().Display.placed) {
    if (entry.text == text) {
      return &entry;
    }
  }
  return nullptr;
}

/**
 * The banner's PLACEMENT, on the panel — §2c's y=116, its draw order, and its widened gate.
 *
 * Every other warning assertion in this file goes through renderBinding(), which calls the binding
 * resolver directly and never touches UiRenderer at all. So nothing observed the banner's position, the
 * order it paints in, or what raises it — which is the whole of what this decision changes.
 */
void bannerPlacementTests() {
  std::printf("\n[the warning banner's band, draw order and gate — §2c]\n");

  SensorCharacteristics calibrated{};
  calibrated.q_max = 150;
  calibrated.f_multiplier = 10;

  // ── The banner paints AFTER the screen ────────────────────────────────────────
  //
  // It used to paint first, which made "the banner replaces the footer row" false rather than untidy:
  // drawTextElement prints with an OPAQUE background, so the footer-hint Text at y=124 punched a
  // background-coloured hole straight through a banner drawn beneath it.
  //
  // Readable from `strings` because that buffer is APPEND-ORDERED, and unambiguous because no element in
  // the generated table binds `legend.warning` — the summary text can only have come from the banner.
  Device dev;
  dev.boot();
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    dev.configs[i] = calibrated;
  }
  dev.undersamplingFlags = 0x01;
  dev.resetFrames();
  // Past the 1 Hz refresh interval, and the frame count is read FIRST: a 100 ms tick leaves every
  // recorder empty and each assertion below would fail for having painted nothing rather than the wrong
  // thing. That is how the P3 paint check first failed.
  dev.tick(1100);
  check(dev.frames() > 0, "P0 repainted, so there is a frame to inspect");
  // The branch is witnessed before the string is: an invalid `calibrated` would silently route the
  // summary to the combined line and the checks below would fail for the wrong reason.
  check(dev.controller.context().uncalibratedCount == 0, "all eight calibrated");
  check(dev.controller.context().warningCount == 1, "so this is the sampling-only branch, at k=1");
  const std::string& painted = m5stamplc_stub::board().Display.strings;
  const std::size_t footerAt = painted.find("UP/DN pages   UP+DN off");
  const std::size_t bannerAt = painted.find("Sampling warning on 1 sensor");
  check(footerAt != std::string::npos, "P0 painted its own footer hint");
  check(bannerAt != std::string::npos, "and the banner painted its summary over the same band");
  check(footerAt < bannerAt,
        "the SCREEN goes down first and the banner over it — otherwise the footer's opaque text "
        "background punches a hole through the band");

  // ── The band is y 116..133, not 34..51 ────────────────────────────────────────
  //
  // Same single frame. The fill and both setCursor calls are asserted together on purpose: the text y is
  // derived from bannerY + 4, so a partial edit that moved the rectangle and left the cursors at 38
  // fails here rather than shipping a banner with its caption outside it.
  check(bannerBandPainted(), "the banner fills x=0 y=116, 240 x 18 — the footer row, not mid-panel");
  const auto* summary = placedText("Sampling warning on 1 sensor");
  const auto* bang = placedText("!");
  check(summary != nullptr && summary->x == 16 && summary->y == 120,
        "the summary sits at x=16, y=bannerY+4");
  check(bang != nullptr && bang->x == 4 && bang->y == 120, "and the ! marker at x=4 on the same line");

  // ── A COMMISSIONING GAP ALONE RAISES IT ───────────────────────────────────────
  //
  // Fix 1b. Before this, drawWarningBanner returned early on `!hasWarnings` and a device whose channels
  // all sat at `SET?` wore no banner at all. Pairing it with the !hasWarnings assertion is what pins the
  // choice of widening the GATE rather than the field: `telemetry.status` reads that field as "how many
  // warnings", and a widened one would be true at a count of zero.
  Device fresh;
  fresh.boot();
  fresh.resetFrames();
  fresh.tick(1100);
  check(fresh.frames() > 0, "the fresh device repainted");
  check(fresh.controller.context().uncalibratedCount == 8, "eight in use, none calibrated");
  check(!fresh.controller.context().hasWarnings, "with no sampling fault, so the FIELD did not widen");
  check(m5stamplc_stub::board().Display.strings.find("8 channels not calibrated") !=
            std::string::npos,
        "and the banner announces the commissioning gap anyway");
  check(bannerBandPainted(), "in the band, on a screen that carries no warning row of its own");

  // Nothing wrong and nothing unfinished: the widening did not become "always on".
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    fresh.configs[i] = calibrated;
  }
  fresh.resetFrames();
  fresh.tick(1100);
  check(fresh.frames() > 0, "and repainted again once every channel was calibrated");
  check(!bannerBandPainted(), "no band on a commissioned, unflagged device");
  check(m5stamplc_stub::board().Display.strings.find("All sensors nominal") == std::string::npos,
        "and the reassurance is not painted as a warning either");

  // ── SUPPRESSED WHILE AN EDITOR IS OPEN — but only the uncalibrated half ───────
  //
  // All thirteen config-*-edit screens carry "UP/DN adjust  ENTER save  hold=cancel" at y=124, the only
  // place the abort gesture is documented, and that row IS the banner's band. A factory-fresh device has
  // eight uncalibrated channels, so an unsuppressed banner would hide `hold=cancel` on every editor,
  // permanently, while the operator is calibrating — the one activity that clears the condition.
  Device editing;
  editing.boot();
  check(walkToModbusSettings(editing), "walked to the first Modbus setting");
  editing.tap(ButtonInputManager::Button::Enter);
  check(editing.controller.editor().active, "and descended onto its editor");
  // AFTER the walk: every tap above repainted, and a band from one of those frames would satisfy the
  // absence check below by having been cleared rather than never drawn.
  editing.resetFrames();
  editing.tick(1100);
  check(editing.frames() > 0, "the editor screen repainted");
  check(editing.controller.context().editorActive, "the editor is published onto the render context");
  check(editing.controller.context().uncalibratedCount == 8,
        "with eight uncalibrated channels still outstanding");
  check(!editing.controller.context().hasWarnings, "and no sampling fault");
  check(!bannerBandPainted(), "so the commissioning banner gets out of the way of `hold=cancel`");
  check(m5stamplc_stub::board().Display.strings.find("hold=cancel") != std::string::npos,
        "and the abort gesture is on the panel where the operator can read it");

  // A SAMPLING fault is NOT suppressed: it says a reading is wrong, which is urgent on every screen,
  // including the one the operator is typing into. Calibrated here so the band can only come from the
  // flag — the uncalibrated term is false either way with the editor open.
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    editing.configs[i] = calibrated;
  }
  editing.undersamplingFlags = 0x01;
  editing.resetFrames();
  editing.tick(1100);
  check(editing.frames() > 0, "the editor screen repainted with the flag set");
  check(editing.controller.editor().active, "the editor is still open");
  check(editing.controller.context().uncalibratedCount == 0,
        "and nothing is uncalibrated, so the band can only come from the sampling flag");
  check(bannerBandPainted(), "a wrong READING outranks the gesture hint even inside an editor");
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
  rootEntryTests();
  selectorPageTests();
  selectorCommitTests();
  ledI2cTrafficTests();
  configListPagingTests();
  heldRepeatScopeTests();
  configEditorDescentTests();
  sensorEditorDescentTests();
  editorDatasetInvariantTests();
  repaintCadenceTests();
  confirmCountdownTests();
  confirmSessionCountdownTests();
  maxFlowResetTests();
  resetCalibrationEntryTests();
  confirmAbortTests();
  factoryResetHoldTests();
  linkApplyProtocolTests();
  textIsDisplayOnlyTests();
  portalLoginResetMenuTests();
  sessionStartRenderTests();
  networkBindingTests();
  everyScreenBindingResolvesTests();
  commissioningSummaryTests();
  bannerPlacementTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
