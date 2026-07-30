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

  struct FactoryResetState {
    bool holdActive = false;
    bool overlayActive = false;
    uint32_t holdStartMs = 0;
    bool restartScheduled = false;
    uint32_t restartAtMs = 0;
  };

  void handleFactoryReset(uint32_t nowMs,
                          ButtonInputManager& buttonInput,
                          UiCountdownState* countdown);
  void scheduleFactoryReset(uint32_t nowMs);
  bool handleFlowEvent(uint32_t nowMs,
                       const ButtonInputManager::ButtonEvent& event,
                       UiController& uiController);
  const ui_exporter::Flow* matchFlow(const ui_exporter::Screen* screen,
                                     const ButtonInputManager::ButtonEvent& event) const;
  ui_exporter::FlowButton mapButton(ButtonInputManager::Button button) const;
  ui_exporter::FlowGesture mapGesture(const ButtonInputManager::ButtonEvent& event) const;

  FactoryResetState factoryResetState_{};
  FactoryResetFn factoryResetFn_ = nullptr;
  Dependencies deps_{};
};

}  // namespace plc
