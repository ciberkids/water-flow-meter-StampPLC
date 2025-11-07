#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "led_controller.h"
#include "register_map.h"
#include "sensor_types.h"

enum class UiMode { Idle, Info, Configuration };

enum class UiPage {
  InstantFlow = 0,
  CumulativeLiters,
  CumulativeCubicMeters,
  SessionLiters,
  SessionCubicMeters,
  MaxFlow,
  EnterConfiguration,
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
  UiPage page = UiPage::InstantFlow;
  bool propellerActive = false;
  uint8_t propellerFrame = 0;
  uint16_t warningFlags = 0;
  uint16_t connectedBitmap = 0;
  double totalSessionLiters = 0.0;
  double aggregateFlowLps = 0.0;
  uint16_t ledVolumeStep = 1;
  uint16_t ledPulsePeriodMs = 500;
  bool countdownActive = false;
  uint32_t countdownSeconds = 0;
  std::string countdownLabel;
  bool hasWarnings = false;
  uint8_t warningCount = 0;
  std::string warningSummary;
  std::array<SensorSnapshot, plc::kNumSensors> sensors{};
};

struct UiCountdownState {
  bool active = false;
  uint32_t secondsRemaining = 0;
  std::string label;
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

 private:
  static constexpr uint32_t kIdleTimeoutMs = 120000;  // 2 minutes
  static constexpr uint32_t kPropellerFrameIntervalMs = 166;  // ~6fps

  void updateIdleState(uint32_t nowMs);

  UiMode mode_ = UiMode::Info;
  UiPage page_ = UiPage::InstantFlow;
  uint32_t lastInteractionMs_ = 0;
  uint32_t lastPropellerUpdateMs_ = 0;
  uint8_t propellerFrame_ = 0;

  UiRenderContext context_;
};
