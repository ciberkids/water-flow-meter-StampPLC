#pragma once

#include <cstdint>
#include "input/button_input.h"
#include "ui/core/ui_controller.h"
// Needed for the ui_exporter::Flow/Screen/FlowButton/FlowGesture types used in
// the private member declarations below.
#include "ui/generated/GeneratedUi.h"

class Preferences;
class ModbusManager;
class LedController;

namespace ui {
class UiScreenRouter;
class UiActionRegistry;
}  // namespace ui

namespace plc {

struct InteractionResult {
  UiCountdownState countdown{};
  bool ledsSuspended = false;
  bool restartScheduled = false;
  uint32_t restartAtMs = 0;
};

class InteractionHandler {
 public:
  using FactoryResetFn = void (*)();

  InteractionHandler() = default;

  struct Dependencies {
    const ui::UiScreenRouter* screenRouter = nullptr;
    const ui::UiActionRegistry* actions = nullptr;
    ModbusManager* modbus = nullptr;
    LedController* ledController = nullptr;
    Preferences* preferences = nullptr;
  };

  void begin(uint32_t nowMs, FactoryResetFn resetFn, const Dependencies& deps);

  InteractionResult update(uint32_t nowMs,
                           ButtonInputManager& buttonInput,
                           UiController& uiController);

 private:
  static constexpr uint32_t kFactoryResetHoldMs = 30000;
  static constexpr uint32_t kFactoryResetOverlayDelayMs = 3000;
  static constexpr uint32_t kFactoryResetRestartDelayMs = 1000;
  static constexpr uint32_t kEnterIdleHoldMs = 3000;
  /**
   * How long ENTER must be held before a guarded-action countdown arms. Matches
   * ButtonInputManager::kLongPressThresholdMs so a short press still goes down
   * the ordinary discrete-event path.
   */
  static constexpr uint32_t kHoldCountdownArmMs = 1500;

  struct FactoryResetState {
    bool holdActive = false;
    bool overlayActive = false;
    uint32_t holdStartMs = 0;
    bool restartScheduled = false;
    uint32_t restartAtMs = 0;
  };

  /**
   * A guarded action armed by holding ENTER (Display_UI_Requirements §4.3:
   * "release ENTER before zero aborts").
   *
   * The dataset expresses this as two flows: an ENTER/long flow on the page
   * whose targetScreenId names a countdown screen, and a Timeout flow on that
   * countdown screen carrying the duration and the actionId to fire at zero.
   */
  struct HoldCountdownState {
    bool active = false;
    uint32_t startMs = 0;
    uint32_t durationMs = 0;
    const char* overlayScreenId = nullptr;
    const char* actionId = nullptr;
    const ui_exporter::Flow* timeoutFlow = nullptr;
  };

  void handleFactoryReset(uint32_t nowMs,
                          ButtonInputManager& buttonInput,
                          UiCountdownState* countdown);
  void scheduleFactoryReset(uint32_t nowMs);
  void handleHoldCountdown(uint32_t nowMs,
                           ButtonInputManager& buttonInput,
                           UiController& uiController,
                           UiCountdownState* countdown);
  bool armHoldCountdown(uint32_t nowMs, const ui_exporter::Screen* screen);
  void dispatchFlowAction(uint32_t nowMs,
                          UiController& uiController,
                          const ui_exporter::Flow& flow);
  bool handleFlowEvent(uint32_t nowMs,
                       const ButtonInputManager::ButtonEvent& event,
                       UiController& uiController);
  const ui_exporter::Flow* matchFlow(const ui_exporter::Screen* screen,
                                     const ButtonInputManager::ButtonEvent& event) const;
  ui_exporter::FlowButton mapButton(ButtonInputManager::Button button) const;
  ui_exporter::FlowGesture mapGesture(const ButtonInputManager::ButtonEvent& event) const;

  FactoryResetState factoryResetState_{};
  HoldCountdownState holdCountdown_{};
  FactoryResetFn factoryResetFn_ = nullptr;
  Dependencies deps_{};
};

}  // namespace plc
