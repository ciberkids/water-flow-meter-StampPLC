#pragma once

#include <cstdint>
#include "input/button_input.h"
#include "ui/core/ui_controller.h"
#include "ui/core/ui_accel.h"
#include "ui/core/ui_settings.h"
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
    const ui::SettingsAccess* settings = nullptr;
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
   * The gesture boundary (Display_UI_Requirements §3): mirrors
   * ButtonInputManager::kLongPressThresholdMs, which is private.
   *
   * A guarded action arms on ENTER *down*, not here — but a guard no longer than this
   * is a plain long press and draws no countdown, because "durations longer than that
   * are always countdowns shown on screen, never gesture thresholds".
   */
  static constexpr uint32_t kGestureLongPressMs = 1500;
  /** UP+DOWN released within this window is a display-off request, not a hold. */
  static constexpr uint32_t kDisplayOffComboMaxMs = 1000;

  struct FactoryResetState {
    bool holdActive = false;
    bool overlayActive = false;
    uint32_t holdStartMs = 0;
    bool restartScheduled = false;
    uint32_t restartAtMs = 0;
  };

  /**
   * A guarded action armed by holding ENTER (Display_UI_Requirements §4.3:
   * "releasing ENTER before zero aborts and returns to the confirm screen").
   *
   * The dataset expresses this with ONE flow, on the confirm screen itself: a
   * Timeout flow whose `button` is Enter (NF-20260730-01 §3.8's discriminator,
   * emitted from `holdButton`), carrying both the duration and the actionId to
   * fire at zero. `overlayScreenId` is therefore the confirm screen's own id.
   *
   * It is not two flows with the page's ENTER-long pointing at a separate countdown
   * screen. That is what this code used to look for, and no screen in the dataset has
   * ever matched it, so the confirm screens could not be confirmed at all.
   */
  struct HoldCountdownState {
    bool active = false;
    uint32_t startMs = 0;
    uint32_t durationMs = 0;
    const char* overlayScreenId = nullptr;
    const char* actionId = nullptr;
    const ui_exporter::Flow* timeoutFlow = nullptr;
  };

  struct ComboState {
    bool active = false;
    uint32_t startMs = 0;
  };

  /** Tracks a held UP/DOWN while an editor is open, for the §5.4 acceleration ramp. */
  struct EditorRepeatState {
    bool active = false;
    ButtonInputManager::Button button = ButtonInputManager::Button::Up;
    uint32_t lastStepMs = 0;
    bool stepped = false;
  };

  void handleEditorRepeat(uint32_t nowMs,
                          ButtonInputManager& buttonInput,
                          UiController& uiController);
  void handleDisplayOffCombo(uint32_t nowMs,
                             ButtonInputManager& buttonInput,
                             UiController& uiController);
  void handleFactoryReset(uint32_t nowMs, UiCountdownState* countdown);
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
  ComboState comboState_{};
  EditorRepeatState editorRepeat_{};
  FactoryResetFn factoryResetFn_ = nullptr;
  Dependencies deps_{};
};

}  // namespace plc
