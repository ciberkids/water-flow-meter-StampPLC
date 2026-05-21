#include "input/interaction_handler.h"

#include <Preferences.h>

#include "led/led_controller.h"
#include "modbus/modbus_manager.h"
#include "ui/core/ui_actions.h"
#include "ui/core/ui_screen_router.h"
#include "ui/generated/GeneratedUi.h"

namespace plc {

void InteractionHandler::begin(uint32_t nowMs, FactoryResetFn resetFn, const Dependencies& deps) {
  factoryResetState_ = FactoryResetState{};
  factoryResetFn_ = resetFn;
  deps_ = deps;
  (void)nowMs;
}

InteractionResult InteractionHandler::update(uint32_t nowMs,
                                             ButtonInputManager& buttonInput,
                                             UiController& uiController) {
  InteractionResult result{};
  handleFactoryReset(nowMs, buttonInput, &result.countdown);

  if (!factoryResetState_.holdActive && !factoryResetState_.restartScheduled) {
    ButtonInputManager::ButtonEvent event;
    while (buttonInput.popEvent(&event)) {
      handleFlowEvent(nowMs, event, uiController);
    }
  } else {
    buttonInput.clearEvents();
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
    screen = deps_.screenRouter->screenForMode(uiController.mode());
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
      .nowMs = nowMs};

  if (deps_.actions->dispatch(flow->actionId, actionContext, *flow)) {
    return true;
  }

  uiController.notifyInteraction(nowMs);
  return false;
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
