#include "input/interaction_handler.h"

#include <Preferences.h>

#include "led/led_controller.h"
#include "modbus/modbus_manager.h"
#include "ui/core/ui_actions.h"
#include "ui/core/ui_screen_router.h"
#include "ui/generated/GeneratedUi.h"

namespace plc {

namespace {

constexpr const char* kFactoryResetCountdownScreenId = "countdown-factory-reset";

/**
 * Finds the Timeout flow on a countdown screen. That flow carries the duration
 * and the actionId to fire when the countdown reaches zero.
 */
const ui_exporter::Flow* findTimeoutFlow(const ui_exporter::Screen* screen) {
  if (!screen || !screen->flows) {
    return nullptr;
  }
  for (std::size_t i = 0; i < screen->flowCount; ++i) {
    const auto& flow = screen->flows[i];
    if (flow.trigger == ui_exporter::FlowTrigger::Timeout && flow.timeoutMs > 0) {
      return &flow;
    }
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
  handleFactoryReset(nowMs, buttonInput, &result.countdown);

  const bool factoryResetBusy =
      factoryResetState_.holdActive || factoryResetState_.restartScheduled;

  if (!factoryResetBusy) {
    // The UP+DOWN factory-reset combo outranks a single-button hold, so only run
    // the guarded-action countdown when the combo is idle.
    handleHoldCountdown(nowMs, buttonInput, uiController, &result.countdown);
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

void InteractionHandler::handleFactoryReset(uint32_t nowMs,
                                            ButtonInputManager& buttonInput,
                                            UiCountdownState* countdown) {
  if (!countdown) {
    return;
  }

  const bool upPressed = buttonInput.isPressed(ButtonInputManager::Button::Up);
  const bool downPressed = buttonInput.isPressed(ButtonInputManager::Button::Down);
  const bool enterPressed = buttonInput.isPressed(ButtonInputManager::Button::Enter);

  if (!factoryResetState_.restartScheduled) {
    if (!factoryResetState_.holdActive) {
      if (upPressed && downPressed && !enterPressed) {
        factoryResetState_.holdActive = true;
        factoryResetState_.overlayActive = false;
        factoryResetState_.holdStartMs = nowMs;
        buttonInput.clearEvents();
      }
    } else {
      if (!(upPressed && downPressed) || enterPressed) {
        factoryResetState_.holdActive = false;
        factoryResetState_.overlayActive = false;
        buttonInput.clearEvents();
      } else {
        const uint32_t elapsedMs = nowMs - factoryResetState_.holdStartMs;
        if (!factoryResetState_.overlayActive && elapsedMs >= kFactoryResetOverlayDelayMs) {
          factoryResetState_.overlayActive = true;
        }
        if (factoryResetState_.overlayActive) {
          const uint32_t remainingMs =
              (elapsedMs >= kFactoryResetHoldMs) ? 0 : (kFactoryResetHoldMs - elapsedMs);
          countdown->active = true;
          countdown->secondsRemaining = (remainingMs + 999) / 1000;
          countdown->label = "Hold UP+DOWN to factory reset (30->0)";
        }
        if (elapsedMs >= kFactoryResetHoldMs) {
          scheduleFactoryReset(nowMs);
          buttonInput.clearEvents();
        } else {
          buttonInput.clearEvents();
        }
      }
    }
  }

  if (factoryResetState_.restartScheduled) {
    countdown->active = true;
    countdown->secondsRemaining = 0;
    countdown->label = "Factory reset complete";
  }
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

  const ui_exporter::Screen* screen = nullptr;
  if (factoryResetState_.overlayActive) {
    screen = deps_.screenRouter->overlayForCountdown();
  } else {
    screen = deps_.screenRouter->screenForMode(uiController.mode(), uiController.page());
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
      .factoryReset = factoryResetFn_};

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
      .factoryReset = factoryResetFn_};
  deps_.actions->dispatch(flow.actionId, actionContext, flow);
}

bool InteractionHandler::armHoldCountdown(uint32_t nowMs, const ui_exporter::Screen* screen) {
  if (!screen || !screen->flows || !deps_.screenRouter) {
    return false;
  }

  for (std::size_t i = 0; i < screen->flowCount; ++i) {
    const auto& flow = screen->flows[i];
    // An ENTER/long flow with a target screen and no action of its own is a
    // countdown-arming flow; the target screen owns the duration and action.
    if (flow.trigger != ui_exporter::FlowTrigger::Button) continue;
    if (flow.button != ui_exporter::FlowButton::Enter) continue;
    if (flow.gesture != ui_exporter::FlowGesture::Long) continue;
    if (flow.actionId || !flow.targetScreenId) continue;

    const auto* overlay = deps_.screenRouter->screenById(flow.targetScreenId);
    const auto* timeoutFlow = findTimeoutFlow(overlay);
    if (!timeoutFlow) continue;

    holdCountdown_.active = true;
    holdCountdown_.startMs = nowMs;
    holdCountdown_.durationMs = timeoutFlow->timeoutMs;
    holdCountdown_.overlayScreenId = flow.targetScreenId;
    holdCountdown_.actionId = timeoutFlow->actionId;
    holdCountdown_.timeoutFlow = timeoutFlow;
    return true;
  }
  return false;
}

void InteractionHandler::handleHoldCountdown(uint32_t nowMs,
                                             ButtonInputManager& buttonInput,
                                             UiController& uiController,
                                             UiCountdownState* countdown) {
  if (!countdown || !deps_.screenRouter) {
    return;
  }

  const bool enterHeld = buttonInput.isPressed(ButtonInputManager::Button::Enter);
  const bool otherHeld = buttonInput.isPressed(ButtonInputManager::Button::Up) ||
                         buttonInput.isPressed(ButtonInputManager::Button::Down);

  if (!holdCountdown_.active) {
    // Arm only once ENTER has been held past the long-press threshold, so a
    // short press still reaches the normal discrete-event path.
    if (enterHeld && !otherHeld) {
      const uint32_t heldMs =
          buttonInput.pressedDuration(ButtonInputManager::Button::Enter, nowMs);
      if (heldMs >= kHoldCountdownArmMs) {
        const auto* screen =
            deps_.screenRouter->screenForMode(uiController.mode(), uiController.page());
        if (armHoldCountdown(nowMs, screen)) {
          buttonInput.clearEvents();
        }
      }
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

  const uint32_t elapsedMs = nowMs - holdCountdown_.startMs;
  countdown->active = true;
  countdown->secondsRemaining = secondsRemaining(elapsedMs, holdCountdown_.durationMs);
  countdown->screenId = holdCountdown_.overlayScreenId;
  countdown->label.clear();

  if (elapsedMs >= holdCountdown_.durationMs) {
    if (const auto* flow = holdCountdown_.timeoutFlow) {
      dispatchFlowAction(nowMs, uiController, *flow);
    }
    holdCountdown_ = HoldCountdownState{};
    buttonInput.clearEvents();
    countdown->active = false;
    countdown->screenId = nullptr;
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
