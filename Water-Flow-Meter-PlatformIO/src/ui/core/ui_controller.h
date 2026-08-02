#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "led/led_controller.h"
#include "ui/core/ui_navigator.h"
#include "ui/core/ui_pages.h"
#include "ui/core/ui_settings.h"
#include "modbus/register_map.h"
#include "modbus/sensor_types.h"

enum class UiMode { Idle, Info, Configuration };

struct SensorSnapshot {
  bool enabled = false;
  bool ready = false;
  float instantFlow = 0.0f;
  double cumulativeLiters = 0.0;
  float sessionLiters = 0.0f;
  float maxFlow = 0.0f;
};

struct UiRenderContext {
  UiMode mode = UiMode::Info;
  UiPage page = UiPage::GlobalStatus;
  uint16_t warningFlags = 0;
  uint16_t connectedBitmap = 0;
  double totalSessionLiters = 0.0;
  double aggregateFlowLps = 0.0;
  /**
   * Core-0 sampling rate, as published in register 0.
   *
   * Surfaced on screen because the achievable rate depends on the wiring and the load,
   * so it has to be read off the device rather than assumed (open decision G1).
   */
  float pollingRateKhz = 0.0f;
  uint16_t ledVolumeStep = 1;
  uint16_t ledPulsePeriodMs = 500;
  bool countdownActive = false;
  uint32_t countdownSeconds = 0;
  std::string countdownLabel;
  /**
   * Exporter screen ID of the overlay to draw while a countdown runs, or nullptr
   * to fall back to the default. Each guarded action has its own countdown
   * screen (Display_UI_Requirements §4.3, §5.3), so the overlay cannot be a
   * single fixed screen.
   */
  const char* countdownScreenId = nullptr;
  /** Screen the navigator is on. Null only before begin() has run. */
  const ui_exporter::Screen* currentScreen = nullptr;
  /** Position within the current level's ring, for the scrollbar. 0 = unknown. */
  uint8_t ringIndex = 0;
  uint8_t ringCount = 0;
  /**
   * True while something on screen changes faster than the 1 Hz telemetry cadence: an open
   * value editor stepping its pending value (§5.4), or a running countdown (§3.3).
   *
   * UiRenderer used to infer this from `mode == UiMode::Configuration`, a mode the 0.2 header
   * note records as "never implemented" and which nothing sets — ui_actions.cpp is the only
   * setMode() caller and it passes Info. So the fast cadence was unreachable and editors and
   * countdowns redrew once a second. Publishing the condition as state rather than deriving
   * it from a retired mode keeps §7's 100 ms acknowledgement out of the mode enum entirely.
   *
   * Navigation between screens is deliberately NOT in here: a page change is a one-off event,
   * and the renderer handles it by repainting the instant the resolved screen differs from the
   * one on the panel. Holding a static menu page at 12.5 Hz would buy nothing and cost a
   * full-panel clear per frame.
   */
  bool interactive = false;
  bool hasWarnings = false;
  uint8_t warningCount = 0;
  std::string warningSummary;
  std::array<SensorSnapshot, plc::kNumSensors> sensors{};
};

/**
 * The value being edited (Display_UI_Requirements §5.4).
 *
 * `pending` is what the operator has dialled up; `saved` is what is in force. Both are
 * shown at once so the operator can see what they are about to commit against what is
 * already there.
 */
struct UiEditorState {
  bool active = false;
  const ui::SettingDescriptor* setting = nullptr;
  uint8_t sensorIndex = 0;
  int32_t pending = 0;
  int32_t saved = 0;
  /** Set when a commit failed its Nyquist check and DOWN can force it (§5.5). */
  bool nyquistPrompt = false;
  /**
   * The commit was refused for a reason that is NOT the Nyquist limit.
   *
   * Kept separate so the operator is told which problem they have. Offering
   * "DOWN = Save anyway" for a failure an override cannot fix would be worse than useless.
   */
  bool commitFailed = false;
  uint32_t lastStepMs = 0;
};

struct UiCountdownState {
  bool active = false;
  uint32_t secondsRemaining = 0;
  /**
   * Millisecond precision alongside the whole seconds shown on screen. The §3.5 LED
   * ramp reaches a 60 ms period, which a value rounded to seconds cannot express.
   */
  uint32_t remainingMs = 0;
  uint32_t totalMs = 0;
  std::string label;
  /** Exporter screen ID of the overlay for this countdown; may be nullptr. */
  const char* screenId = nullptr;
};

class UiController {
 public:
  void begin(uint32_t nowMs);

  void notifyInteraction(uint32_t nowMs);
  void setMode(UiMode mode, uint32_t nowMs);
  void enterIdle(uint32_t nowMs);
  void nextPage(uint32_t nowMs);
  void previousPage(uint32_t nowMs);
  void setPage(UiPage page, uint32_t nowMs);

  void update(uint32_t nowMs,
              const SensorData* sensors,
              const SensorCharacteristics* configs,
              uint16_t warningFlags,
              uint16_t connectedBitmap,
              double totalSessionLiters,
              double aggregateFlowLps,
              float pollingRateKhz,
              const LedController& ledController,
              const UiCountdownState& countdown);

  const UiRenderContext& context() const { return context_; }
  UiMode mode() const { return mode_; }
  UiPage page() const { return page_; }

  ui::UiNavigator& navigator() { return navigator_; }
  const ui::UiNavigator& navigator() const { return navigator_; }

  /**
   * Keeps UiPage in step with the navigator, one direction only: screen -> page.
   *
   * UiPage remains the source for the `page.title` binding, but the navigator is
   * authoritative for what is drawn. Deriving the page from the screen means the two
   * cannot drift; maintaining both independently is exactly the kind of duplicate
   * bookkeeping that produced the stale router defaults.
   */
  void syncPageFromScreen(const ui_exporter::Screen* screen, uint32_t nowMs);

  const UiEditorState& editor() const { return editor_; }
  void beginEdit(const ui::SettingDescriptor* setting, uint8_t sensorIndex, int32_t current);
  void endEdit();
  void adjustEdit(int32_t delta, uint32_t nowMs);
  void setNyquistPrompt(bool on) { editor_.nyquistPrompt = on; }
  void setCommitFailed(bool on) { editor_.commitFailed = on; }

 private:
  static constexpr uint32_t kIdleTimeoutMs = 120000;  // 2 minutes

  void updateIdleState(uint32_t nowMs);

  UiMode mode_ = UiMode::Info;
  UiPage page_ = UiPage::GlobalStatus;
  uint32_t lastInteractionMs_ = 0;

  ui::UiNavigator navigator_;
  UiEditorState editor_{};
  UiRenderContext context_;
};
