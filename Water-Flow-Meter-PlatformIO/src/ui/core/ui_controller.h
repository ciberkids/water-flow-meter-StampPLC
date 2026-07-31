#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "led/led_controller.h"
#include "ui/core/ui_navigator.h"
#include "modbus/register_map.h"
#include "modbus/sensor_types.h"

enum class UiMode { Idle, Info, Configuration };

// Order MUST match kInfoScreenIds in ui_screen_router.cpp (P0..P7).
enum class UiPage {
  GlobalStatus = 0,
  InstantFlow,
  CumulativeLiters,
  CumulativeCubicMeters,
  SessionLiters,
  SessionCubicMeters,
  MaxFlow,
  EnterConfiguration,
  FactoryReset,
  Count
};

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
  bool hasWarnings = false;
  uint8_t warningCount = 0;
  std::string warningSummary;
  std::array<SensorSnapshot, plc::kNumSensors> sensors{};
};

struct UiCountdownState {
  bool active = false;
  uint32_t secondsRemaining = 0;
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

 private:
  static constexpr uint32_t kIdleTimeoutMs = 120000;  // 2 minutes

  void updateIdleState(uint32_t nowMs);

  UiMode mode_ = UiMode::Info;
  UiPage page_ = UiPage::GlobalStatus;
  uint32_t lastInteractionMs_ = 0;

  ui::UiNavigator navigator_;
  UiRenderContext context_;
};
