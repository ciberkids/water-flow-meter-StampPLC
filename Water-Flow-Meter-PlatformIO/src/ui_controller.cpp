#include "ui_controller.h"

#include <algorithm>

void UiController::begin(uint32_t nowMs) {
  mode_ = UiMode::Info;
  page_ = UiPage::InstantFlow;
  lastInteractionMs_ = nowMs;
  lastPropellerUpdateMs_ = nowMs;
  propellerFrame_ = 0;
  context_ = UiRenderContext{};
  context_.mode = mode_;
  context_.page = page_;
}

void UiController::notifyInteraction(uint32_t nowMs) {
  lastInteractionMs_ = nowMs;
  if (mode_ == UiMode::Idle) {
    mode_ = UiMode::Info;
  }
}

void UiController::setMode(UiMode mode, uint32_t nowMs) {
  mode_ = mode;
  if (mode_ == UiMode::Idle) {
    lastInteractionMs_ = nowMs;
  } else {
    notifyInteraction(nowMs);
  }
}

void UiController::enterIdle(uint32_t nowMs) {
  mode_ = UiMode::Idle;
  lastInteractionMs_ = nowMs;
}

void UiController::nextPage(uint32_t nowMs) {
  notifyInteraction(nowMs);
  int pageIndex = static_cast<int>(page_);
  pageIndex = (pageIndex + 1) % static_cast<int>(UiPage::Count);
  page_ = static_cast<UiPage>(pageIndex);
}

void UiController::previousPage(uint32_t nowMs) {
  notifyInteraction(nowMs);
  int pageIndex = static_cast<int>(page_);
  pageIndex = (pageIndex - 1 + static_cast<int>(UiPage::Count)) % static_cast<int>(UiPage::Count);
  page_ = static_cast<UiPage>(pageIndex);
}

void UiController::setPage(UiPage page, uint32_t nowMs) {
  notifyInteraction(nowMs);
  page_ = page;
}

void UiController::update(uint32_t nowMs,
                          const SensorData* sensors,
                          const SensorCharacteristics* configs,
                          uint16_t warningFlags,
                          uint16_t connectedBitmap,
                          double totalSessionLiters,
                          double aggregateFlowLps,
                          const LedController& ledController,
                          const UiCountdownState& countdown) {
  updateIdleState(nowMs);

  context_.mode = mode_;
  context_.page = page_;
  context_.warningFlags = warningFlags;
  context_.connectedBitmap = connectedBitmap;
  context_.totalSessionLiters = totalSessionLiters;
  context_.aggregateFlowLps = aggregateFlowLps;
  context_.ledVolumeStep = ledController.volumeStepLiters();
  context_.ledPulsePeriodMs = ledController.pulsePeriodMs();
  context_.countdownActive = countdown.active;
  context_.countdownSeconds = countdown.secondsRemaining;
  context_.countdownLabel = countdown.label;
  context_.hasWarnings = warningFlags != 0;
  context_.warningCount = 0;
  context_.warningSummary.clear();

  // Propeller animation
  const bool active = aggregateFlowLps > 0.001;
  context_.propellerActive = active;
  if (active) {
    if (nowMs - lastPropellerUpdateMs_ >= kPropellerFrameIntervalMs) {
      propellerFrame_ = static_cast<uint8_t>((propellerFrame_ + 1) % 8);
      lastPropellerUpdateMs_ = nowMs;
    }
  } else {
    propellerFrame_ = 0;
    lastPropellerUpdateMs_ = nowMs;
  }
  context_.propellerFrame = propellerFrame_;

  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    auto& dst = context_.sensors[i];
    const auto& src = sensors[i];
    dst.enabled = (connectedBitmap >> i) & 0x01;
    dst.ready = src.isReady;
    dst.instantFlow = src.instantFlow_L_s;
    dst.cumulativeLiters = src.cumulativeLiters;
    dst.sessionLiters = src.sessionLiters;
    dst.maxFlow = src.maxFlowSinceReset;

    if ((warningFlags >> i) & 0x01) {
      if (context_.warningCount == 0) {
        context_.warningSummary = "Sampling warning on sensors ";
      } else {
        context_.warningSummary += ", ";
      }
      context_.warningSummary += std::to_string(i + 1);
      context_.warningCount++;
    }
  }

  if (!context_.hasWarnings) {
    context_.warningSummary = "All sensors nominal";
  }
}

void UiController::updateIdleState(uint32_t nowMs) {
  if (mode_ != UiMode::Idle) {
    if (nowMs - lastInteractionMs_ >= kIdleTimeoutMs) {
      mode_ = UiMode::Idle;
    }
  }
  if (mode_ == UiMode::Idle) {
    context_.propellerActive = false;
    propellerFrame_ = 0;
  }
}
