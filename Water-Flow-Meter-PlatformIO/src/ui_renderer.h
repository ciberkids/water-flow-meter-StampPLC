#pragma once

#include <cstddef>
#include <cstdint>

#include <array>
#include <cstdint>

#include "ui_controller.h"

class UiRenderer {
 public:
  void begin();

  void update(uint32_t nowMs, const UiRenderContext& context);

 private:
  void drawInfoScreen(const UiRenderContext& context);
  void drawConfigurationScreen(const UiRenderContext& context);
  void drawPropeller(uint8_t frame, int16_t centerX, int16_t centerY, bool active);
  void drawLegend(const UiRenderContext& context);
  void drawCountdownOverlay(const UiRenderContext& context);
  void drawWarningBanner(const UiRenderContext& context);

  uint32_t lastRenderMs_ = 0;
  static constexpr uint32_t kRefreshIntervalMs = 1000;
};
