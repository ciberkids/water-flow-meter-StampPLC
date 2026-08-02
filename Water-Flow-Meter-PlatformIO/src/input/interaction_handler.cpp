#include "input/interaction_handler.h"

#include <Preferences.h>

#include <cstring>

#include "led/led_controller.h"
#include "modbus/modbus_manager.h"
#include "ui/core/ui_actions.h"
#include "ui/core/ui_screen_router.h"
#include "ui/generated/GeneratedUi.h"

namespace plc {

namespace {

constexpr const char* kFactoryResetCountdownScreenId = "confirm-factory-reset";

/**
 * The one action a completed hold must NOT be dispatched as an ordinary flow action.
 *
 * `ui_actions.cpp`'s handler only calls `ctx.factoryReset()`, i.e. firmware.cpp's
 * `performFactoryReset()`: it wipes NVS and resets `linkSettings` to defaults in RAM but
 * schedules no reboot. The reboot lives in `factoryResetState_.restartScheduled`
 * (firmware.cpp:512 `esp_restart()`), which only `scheduleFactoryReset()` sets — and the
 * same flag gates `noteResetAccepted()`'s solid-white acknowledgement (firmware.cpp:466)
 * and `restartRs485()`. Dispatching the flow would leave the device running on the old
 * RS485 binding with wiped NVS for ever, against §4.3's "Factory reset? | P8 | 30 s |
 * core.action.factory-reset | reboot, no toast".
 */
constexpr const char* kFactoryResetActionId = "core.action.factory-reset";

/**
 * Finds the hold-countdown flow a confirm screen declares on itself.
 *
 * NF-20260730-01 §3.8 gives the single `timeout` trigger a discriminator, and
 * cppEmitter.ts carries it in the flow's *button* field: `holdButton: "enter"`
 * emits `FlowButton::Enter` (hold countdown, aborts on release),
 * `holdButton: null` emits `FlowButton::None` (auto timeout, drives the toasts).
 * `gesture` is meaningless on a non-button trigger — the emitter fills in
 * `FlowGesture::Short` — so it must never be part of the predicate.
 *
 * The Enter check is what keeps a toast's `Timeout/None/2000` flow from being
 * treated as something the operator has to hold ENTER through.
 */
const ui_exporter::Flow* findHoldCountdownFlow(const ui_exporter::Screen* screen) {
  if (!screen || !screen->flows) {
    return nullptr;
  }
  for (std::size_t i = 0; i < screen->flowCount; ++i) {
    const auto& flow = screen->flows[i];
    if (flow.trigger != ui_exporter::FlowTrigger::Timeout) continue;
    if (flow.button != ui_exporter::FlowButton::Enter) continue;
    if (flow.timeoutMs == 0) continue;
    return &flow;
  }
  return nullptr;
}

/** Ceiling division to whole seconds, so the display never shows 0 while held. */
uint32_t secondsRemaining(uint32_t elapsedMs, uint32_t durationMs) {
  const uint32_t remainingMs = (elapsedMs >= durationMs) ? 0 : (durationMs - elapsedMs);
  return (remainingMs + 999) / 1000;
}

}  // namespace

void InteractionHandler::begin(uint32_t nowMs, FactoryResetFn resetFn, const Dependencies& deps) {
  factoryResetState_ = FactoryResetState{};
  holdCountdown_ = HoldCountdownState{};
  factoryResetFn_ = resetFn;
  deps_ = deps;
  (void)nowMs;
}

InteractionResult InteractionHandler::update(uint32_t nowMs,
                                             ButtonInputManager& buttonInput,
                                             UiController& uiController) {
  InteractionResult result{};
  handleDisplayOffCombo(nowMs, buttonInput, uiController);
  handleEditorRepeat(nowMs, buttonInput, uiController);
  handleFactoryReset(nowMs, &result.countdown);
  handleEntryTimer(nowMs, uiController);

  const bool factoryResetBusy =
      factoryResetState_.holdActive || factoryResetState_.restartScheduled;

  if (!factoryResetBusy) {
    // The UP+DOWN factory-reset combo outranks a single-button hold, so only run
    // the guarded-action countdown when the combo is idle.
    handleHoldCountdown(nowMs, buttonInput, uiController, &result);
  }

  if (!factoryResetBusy && !holdCountdown_.active) {
    ButtonInputManager::ButtonEvent event;
    while (buttonInput.popEvent(&event)) {
      handleFlowEvent(nowMs, event, uiController);
    }
  } else {
    // While a countdown owns ENTER, discrete press events must not also fire —
    // otherwise the long-press event and the countdown would both act.
    buttonInput.clearEvents();
  }

  if (factoryResetState_.overlayActive || factoryResetState_.restartScheduled) {
    result.countdown.screenId = kFactoryResetCountdownScreenId;
  }

  result.ledsSuspended = factoryResetState_.holdActive || factoryResetState_.restartScheduled;
  result.restartScheduled = factoryResetState_.restartScheduled;
  result.restartAtMs = factoryResetState_.restartAtMs;
  return result;
}

/**
 * UP+DOWN short press: display off, and reset navigation to P0
 * (Display_UI_Requirements.md §3.1).
 *
 * This replaces the retired UP+DOWN 30 s factory-reset combo. The dataset's footer
 * hints already advertised the gesture, but nothing implemented it — factory reset
 * moved to page P8 and this gesture was specified and then never built.
 *
 * It fires on *release* before the long-press threshold, so it cannot be confused
 * with a deliberate hold, and it clears the navigation stack so the display always
 * wakes on P0 rather than wherever the operator happened to be.
 */
/**
 * Drives the §5.4 acceleration ramp while UP or DOWN is held in an editor.
 *
 * This cannot come from ButtonInputManager's repeat events: those only begin after the
 * 1.5 s long-press threshold, whereas the ramp's first tier steps every 250 ms from the
 * moment the button goes down. So the held path owns UP/DOWN whenever an editor is
 * open, and discards that button's queued events once it has stepped — otherwise the
 * release short-press would add an extra step on top of the ramp.
 *
 * A genuine tap never reaches the first interval, so it produces no ramp step and its
 * release short-press is left alone: "a short press is always exactly +/-1".
 */
void InteractionHandler::handleEditorRepeat(uint32_t nowMs,
                                            ButtonInputManager& buttonInput,
                                            UiController& uiController) {
  const auto& editor = uiController.editor();
  if (!editor.active || !editor.setting) {
    editorRepeat_ = EditorRepeatState{};
    return;
  }

  const bool up = buttonInput.isPressed(ButtonInputManager::Button::Up);
  const bool down = buttonInput.isPressed(ButtonInputManager::Button::Down);
  if (up == down) {
    // Neither, or both: neither is an adjustment.
    if (editorRepeat_.active && editorRepeat_.stepped) {
      buttonInput.discardEvents(editorRepeat_.button);
    }
    editorRepeat_ = EditorRepeatState{};
    return;
  }

  const auto button = up ? ButtonInputManager::Button::Up : ButtonInputManager::Button::Down;
  if (!editorRepeat_.active || editorRepeat_.button != button) {
    editorRepeat_ = EditorRepeatState{};
    editorRepeat_.active = true;
    editorRepeat_.button = button;
    editorRepeat_.lastStepMs = nowMs;
    return;
  }

  const uint32_t heldMs = buttonInput.pressedDuration(button, nowMs);
  const ui::AccelTier tier = ui::accelerationTier(heldMs);
  if (nowMs - editorRepeat_.lastStepMs < tier.intervalMs) {
    return;
  }
  editorRepeat_.lastStepMs = nowMs;
  editorRepeat_.stepped = true;

  const int32_t base = editor.setting->step;
  const int32_t magnitude = base * tier.multiplier;
  uiController.adjustEdit(up ? magnitude : -magnitude, nowMs);
  // Swallow anything this button has queued, including the release short-press.
  buttonInput.discardEvents(button);
}

void InteractionHandler::handleDisplayOffCombo(uint32_t nowMs,
                                               ButtonInputManager& buttonInput,
                                               UiController& uiController) {
  const bool up = buttonInput.isPressed(ButtonInputManager::Button::Up);
  const bool down = buttonInput.isPressed(ButtonInputManager::Button::Down);
  const bool enter = buttonInput.isPressed(ButtonInputManager::Button::Enter);

  if (up && down && !enter) {
    if (!comboState_.active) {
      comboState_.active = true;
      comboState_.startMs = nowMs;
    }
    return;
  }

  if (comboState_.active) {
    const bool wasShort = (nowMs - comboState_.startMs) < kDisplayOffComboMaxMs;
    comboState_ = ComboState{};
    if (wasShort && !enter) {
      uiController.setPage(UiPage::GlobalStatus, nowMs);
      uiController.enterIdle(nowMs);
      buttonInput.clearEvents();
    }
  }
}

void InteractionHandler::handleFactoryReset(uint32_t nowMs, UiCountdownState* countdown) {
  (void)nowMs;
  if (!countdown) {
    return;
  }

  // The blind UP+DOWN 30 s arming combo is GONE. Display_UI_Requirements §3.3 retired it
  // ("a destructive action must be visible"), Project_document §5.3.4 and RGB_LED_Behavior §5
  // both record it as removed, and it moved to page P8 with a confirm screen.
  //
  // It was still live, and it was a hazard rather than merely dead code. §3.1 instructs the
  // operator to press UP+DOWN to blank the display; kDisplayOffComboMaxMs is 1000 ms, while
  // this combo armed on the same two buttons and raised a factory-reset countdown at
  // kFactoryResetOverlayDelayMs = 3000 ms. Holding a moment longer to check whether the
  // screen really went dark — the natural thing to do — started a wipe. Thirty seconds of
  // that and NVS was erased.
  //
  // It could only be removed once the replacement path actually worked. Until the
  // hold-countdown arming was fixed, this combo was the ONLY route that reboots the device,
  // so deleting it earlier would have left no factory reset at all.
  //
  // What remains here is the post-schedule display: the confirm path calls
  // scheduleFactoryReset(), and this paints the acknowledgement until firmware.cpp restarts.
  if (factoryResetState_.restartScheduled) {
    countdown->active = true;
    countdown->secondsRemaining = 0;
    countdown->label = "Factory reset complete";
  }
}

/** The unattended Timeout flow on a screen, or nullptr. FlowButton::None is the marker. */
const ui_exporter::Flow* findEntryTimeoutFlow(const ui_exporter::Screen* screen) {
  if (!screen || !screen->flows) {
    return nullptr;
  }
  for (std::size_t i = 0; i < screen->flowCount; ++i) {
    const auto& flow = screen->flows[i];
    if (flow.trigger != ui_exporter::FlowTrigger::Timeout) continue;
    // The discriminator of NF-20260730-01 §3.8: a Timeout flow carrying ENTER is a hold
    // countdown and belongs to armHoldCountdown; one carrying None runs unattended.
    if (flow.button != ui_exporter::FlowButton::None) continue;
    if (flow.timeoutMs == 0) continue;
    return &flow;
  }
  return nullptr;
}

void InteractionHandler::handleEntryTimer(uint32_t nowMs, UiController& uiController) {
  const auto* screen = uiController.navigator().current();

  // Re-arm whenever the screen changes. Comparing the SCREEN POINTER rather than a bool is
  // what makes navigating away cancel the timer: the operator leaving a toast early must not
  // have its action fire behind them a second later.
  if (screen != entryTimer_.screen) {
    entryTimer_ = EntryTimerState{};
    entryTimer_.screen = screen;
    entryTimer_.flow = findEntryTimeoutFlow(screen);
    entryTimer_.startMs = nowMs;
    return;
  }

  if (!entryTimer_.flow) {
    return;
  }
  if (nowMs - entryTimer_.startMs < entryTimer_.flow->timeoutMs) {
    return;
  }

  // Fire once. Clearing the flow first means a handler that does not navigate cannot re-fire
  // on the next pass.
  const auto* flow = entryTimer_.flow;
  entryTimer_.flow = nullptr;
  dispatchFlowAction(nowMs, uiController, *flow);
}

void InteractionHandler::scheduleFactoryReset(uint32_t nowMs) {
  factoryResetState_.holdActive = false;
  factoryResetState_.overlayActive = false;
  factoryResetState_.restartScheduled = true;
  factoryResetState_.restartAtMs = nowMs + kFactoryResetRestartDelayMs;
  if (factoryResetFn_) {
    factoryResetFn_();
  }
}

bool InteractionHandler::handleFlowEvent(uint32_t nowMs,
                                         const ButtonInputManager::ButtonEvent& event,
                                         UiController& uiController) {
  if (!deps_.screenRouter || !deps_.actions || !deps_.modbus || !deps_.ledController ||
      !deps_.preferences) {
    uiController.notifyInteraction(nowMs);
    return false;
  }

  // The navigator is authoritative for where we are. It only falls back to the
  // router when it has not been seeded yet, which cannot happen after begin().
  const ui_exporter::Screen* screen = nullptr;
  if (factoryResetState_.overlayActive) {
    screen = deps_.screenRouter->overlayForCountdown();
  } else {
    screen = uiController.navigator().current();
    if (!screen) {
      screen = deps_.screenRouter->screenForMode(uiController.mode(), uiController.page());
    }
  }

  if (!screen || !screen->flows || screen->flowCount == 0) {
    uiController.notifyInteraction(nowMs);
    return false;
  }

  const ui_exporter::Flow* flow = matchFlow(screen, event);
  if (!flow || !flow->actionId) {
    uiController.notifyInteraction(nowMs);
    return false;
  }

  ui::UiActionContext actionContext{
      .controller = uiController,
      .modbus = *deps_.modbus,
      .leds = *deps_.ledController,
      .preferences = *deps_.preferences,
      .nowMs = nowMs,
      .factoryReset = factoryResetFn_,
      .settings = deps_.settings,
      .resolvedTarget = flow->targetScreenId
                            ? deps_.screenRouter->screenById(flow->targetScreenId)
                            : nullptr};

  if (deps_.actions->dispatch(flow->actionId, actionContext, *flow)) {
    return true;
  }

  uiController.notifyInteraction(nowMs);
  return false;
}

void InteractionHandler::dispatchFlowAction(uint32_t nowMs,
                                            UiController& uiController,
                                            const ui_exporter::Flow& flow) {
  if (!flow.actionId || !deps_.actions || !deps_.modbus || !deps_.ledController ||
      !deps_.preferences) {
    return;
  }
  ui::UiActionContext actionContext{
      .controller = uiController,
      .modbus = *deps_.modbus,
      .leds = *deps_.ledController,
      .preferences = *deps_.preferences,
      .nowMs = nowMs,
      .factoryReset = factoryResetFn_,
      .settings = deps_.settings,
      .resolvedTarget = flow.targetScreenId && deps_.screenRouter
                            ? deps_.screenRouter->screenById(flow.targetScreenId)
                            : nullptr};
  deps_.actions->dispatch(flow.actionId, actionContext, flow);
}

/**
 * Arms the countdown the screen the operator is standing on declares.
 *
 * A confirm screen owns its own guard: the duration and the action both sit on its
 * Timeout/Enter flow, and the screen the countdown draws is the confirm screen
 * itself, not a separate overlay.
 */
bool InteractionHandler::armHoldCountdown(uint32_t nowMs, const ui_exporter::Screen* screen) {
  const auto* holdFlow = findHoldCountdownFlow(screen);
  if (!holdFlow) {
    return false;
  }

  holdCountdown_.active = true;
  // Anchored to this pass, not to the press instant: a countdown may then be a few
  // milliseconds longer than the hold, never shorter.
  holdCountdown_.startMs = nowMs;
  holdCountdown_.durationMs = holdFlow->timeoutMs;
  holdCountdown_.overlayScreenId = screen->id;
  holdCountdown_.actionId = holdFlow->actionId;
  holdCountdown_.timeoutFlow = holdFlow;
  return true;
}

void InteractionHandler::handleHoldCountdown(uint32_t nowMs,
                                             ButtonInputManager& buttonInput,
                                             UiController& uiController,
                                             InteractionResult* result) {
  UiCountdownState* countdown = &result->countdown;
  if (!countdown || !deps_.screenRouter) {
    return;
  }

  const bool enterHeld = buttonInput.isPressed(ButtonInputManager::Button::Enter);
  const bool otherHeld = buttonInput.isPressed(ButtonInputManager::Button::Up) ||
                         buttonInput.isPressed(ButtonInputManager::Button::Down);

  if (!holdCountdown_.active) {
    // Arm as soon as ENTER goes down, not at the 1.5 s long-press threshold: the
    // duration is a countdown, not a gesture boundary (§3, "durations longer than
    // 1.5 s are always countdowns, never gesture thresholds"). Arming at the
    // threshold would make the 1.5 s `Reset session?` guard zero-length and would
    // show only the second half of the 3 s `Reset totals?` one.
    //
    // A short press is still an exit: the countdown disarms on release and the
    // release short-press then reaches the screen's own ENTER-short flow.
    if (enterHeld && !otherHeld) {
      const auto* screen = uiController.navigator().current();
      if (!screen) {
        screen = deps_.screenRouter->screenForMode(uiController.mode(), uiController.page());
      }
      armHoldCountdown(nowMs, screen);
    }
    return;
  }

  // §4.3 note 1: releasing ENTER before zero aborts — and only that. §4.3 note 2
  // says UP/DOWN have "no effect" during a countdown, so they must not cancel it
  // either. UP+DOWN cannot arm a factory reset here regardless, because that
  // combo requires ENTER to be up.
  if (!enterHeld) {
    holdCountdown_ = HoldCountdownState{};
    uiController.notifyInteraction(nowMs);
    return;
  }

  // A hold is interaction. Without this a 30 s factory-reset hold begun late in the
  // inactivity window lets updateIdleState fire mid-countdown: navigation resets to P0
  // and the display goes dark while the countdown stays armed underneath.
  uiController.notifyInteraction(nowMs);

  const uint32_t elapsedMs = nowMs - holdCountdown_.startMs;
  // Only durations *above* the gesture boundary get an on-screen countdown. §4.3's table
  // specifies `Reset session?` as "ENTER long (1.5 s), no countdown", and §3 makes 1.5 s
  // the gesture boundary rather than a countdown, so a 1.5 s guard is a plain long press
  // that happens to be expressed as a duration.
  if (holdCountdown_.durationMs > kGestureLongPressMs) {
    countdown->active = true;
    countdown->secondsRemaining = secondsRemaining(elapsedMs, holdCountdown_.durationMs);
    countdown->remainingMs =
        (elapsedMs >= holdCountdown_.durationMs) ? 0 : (holdCountdown_.durationMs - elapsedMs);
    countdown->totalMs = holdCountdown_.durationMs;
    countdown->screenId = holdCountdown_.overlayScreenId;
    countdown->label.clear();
  }

  if (elapsedMs >= holdCountdown_.durationMs) {
    // Factory reset goes down the UP+DOWN combo's own completion path instead of being
    // dispatched as a flow action, because that path is the only one that arms the reboot
    // (see kFactoryResetActionId). scheduleFactoryReset() *replaces* the dispatch rather
    // than following it: it already calls factoryResetFn_(), so doing both would wipe
    // twice. From here the presentation is byte-identical to the combo's — update()'s
    // `restartScheduled` branch re-pins countdown.screenId to the confirm screen on this
    // very pass, and the next pass's handleFactoryReset paints "Factory reset complete".
    const bool isFactoryReset =
        holdCountdown_.actionId &&
        std::strcmp(holdCountdown_.actionId, kFactoryResetActionId) == 0;
    if (isFactoryReset) {
      scheduleFactoryReset(nowMs);
    } else if (const auto* flow = holdCountdown_.timeoutFlow) {
      dispatchFlowAction(nowMs, uiController, *flow);
    }
    // Captured BEFORE the state is cleared. Reading it afterwards silently yielded nullptr and
    // fell through to the plain ascend, so the toast never appeared — found by the test below,
    // not by reading the code.
    const ui_exporter::Flow* completedFlow = holdCountdown_.timeoutFlow;

    // §3.5: solid white on acceptance, for every reset confirm screen. Set here rather than in
    // firmware.cpp so it covers whichever action the confirm screen carries, including any
    // guarded action added later.
    result->resetAccepted = true;

    holdCountdown_ = HoldCountdownState{};
    buttonInput.clearEvents();
    countdown->active = false;
    countdown->screenId = nullptr;

    // Ascend: the confirm screen is a modal the countdown consumes, so whoever
    // descended into it must be unwound when it completes. §4.3 has the toast
    // "return automatically to the page the operator started from" — ascending to the
    // parent is that endpoint minus the 2 s dwell, so it lands the operator where the
    // finished feature will.
    //
    // The flow's targetScreenId (the toast) is deliberately NOT honoured yet: a toast
    // needs a screen-entry timer to run its own Timeout/None flow, and without one the
    // operator would be parked on "TOTALS RESET / Returning..." for good. When the
    // entry timer and a replace-at-depth navigator primitive exist, completion should
    // instead move to the resolved target at the confirm screen's depth and let the
    // toast's `Timeout/None/2000 -> ui.action.nav.back` do this ascend.
    //
    // Factory reset is the exception: the device reboots in a second and the combo path's
    // "Factory reset complete" overlay is the acknowledgement, so unwinding to P8 first
    // would only flicker. A judgment call, not a correctness constraint.
    if (!isFactoryReset) {
      // Prefer the flow's declared target — the acknowledgement toast — REPLACING the confirm
      // screen rather than descending onto it. The toast dismisses itself with
      // ui.action.nav.back, so pushing it would ascend right back into "RESET TOTALS?"; a
      // replacement at the same depth makes that ascend land on the originating page, which is
      // what §4.3.1 asks for.
      const auto* toast = completedFlow && completedFlow->targetScreenId
                              ? deps_.screenRouter->screenById(completedFlow->targetScreenId)
                              : nullptr;
      if (toast) {
        uiController.navigator().replaceCurrent(toast);
        uiController.syncPageFromScreen(uiController.navigator().current(), nowMs);
      } else if (uiController.navigator().ascend()) {
        uiController.syncPageFromScreen(uiController.navigator().current(), nowMs);
      }
    }
  }
}

const ui_exporter::Flow* InteractionHandler::matchFlow(const ui_exporter::Screen* screen,
                                                       const ButtonInputManager::ButtonEvent& event) const {
  if (!screen || !screen->flows) {
    return nullptr;
  }
  const auto gesture = mapGesture(event);
  const auto button = mapButton(event.button);
  for (std::size_t i = 0; i < screen->flowCount; ++i) {
    const auto& flow = screen->flows[i];
    if (flow.trigger != ui_exporter::FlowTrigger::Button) {
      continue;
    }
    if (flow.button != button) {
      continue;
    }
    if (flow.gesture != gesture) {
      continue;
    }
    return &flow;
  }
  return nullptr;
}

ui_exporter::FlowButton InteractionHandler::mapButton(ButtonInputManager::Button button) const {
  switch (button) {
    case ButtonInputManager::Button::Up:
      return ui_exporter::FlowButton::Up;
    case ButtonInputManager::Button::Down:
      return ui_exporter::FlowButton::Down;
    case ButtonInputManager::Button::Enter:
    default:
      return ui_exporter::FlowButton::Enter;
  }
}

ui_exporter::FlowGesture InteractionHandler::mapGesture(
    const ButtonInputManager::ButtonEvent& event) const {
  if (!event.isLongPress) {
    return ui_exporter::FlowGesture::Short;
  }
  return event.isRepeat ? ui_exporter::FlowGesture::Hold : ui_exporter::FlowGesture::Long;
}

}  // namespace plc
