#include "ui/core/ui_controller.h"

#include <algorithm>
#include <cstring>

void UiController::begin(uint32_t nowMs) {
  mode_ = UiMode::Info;
  page_ = UiPage::GlobalStatus;
  lastInteractionMs_ = nowMs;
  context_ = UiRenderContext{};
  context_.mode = mode_;
  context_.page = page_;
}

void UiController::syncPageFromScreen(const ui_exporter::Screen* screen, uint32_t nowMs) {
  if (!screen || !screen->id) {
    return;
  }
  // Only info-level screens map onto a UiPage; deeper levels leave it untouched so
  // page.title keeps naming the info page the operator came from.
  static constexpr const char* kInfoIds[] = {
      "info-p0-global-status", "info-p1-instant-flow", "info-p2-cumulative-liters",
      "info-p3-cumulative-m3", "info-p4-session-liters", "info-p5-session-m3",
      "info-p6-max-flow", "info-p7-enter-config", "info-p8-factory-reset"};
  static_assert(sizeof(kInfoIds) / sizeof(kInfoIds[0]) ==
                    static_cast<std::size_t>(UiPage::Count),
                "kInfoIds must have one entry per UiPage");
  for (std::size_t i = 0; i < static_cast<std::size_t>(UiPage::Count); ++i) {
    if (std::strcmp(screen->id, kInfoIds[i]) == 0) {
      setPage(static_cast<UiPage>(i), nowMs);
      return;
    }
  }
  notifyInteraction(nowMs);
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
  context_.countdownScreenId = countdown.screenId;
  context_.currentScreen = navigator_.current();
  uint8_t ringIndex = 0;
  uint8_t ringCount = 0;
  if (navigator_.ringPosition(&ringIndex, &ringCount)) {
    context_.ringIndex = ringIndex;
    context_.ringCount = ringCount;
  } else {
    context_.ringIndex = 0;
    context_.ringCount = 0;
  }
  context_.hasWarnings = warningFlags != 0;
  context_.warningCount = 0;
  context_.warningSummary.clear();

  // The P0 flow indicator is driven straight from aggregateFlowLps by
  // UiRenderer::drawFlowDots(); no frame counter is kept here.

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
}
