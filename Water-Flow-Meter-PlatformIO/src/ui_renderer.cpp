#include "ui_renderer.h"

#include <M5StamPLC.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace {
constexpr uint16_t kBackgroundColor = BLACK;
constexpr uint16_t kTextColor = WHITE;
constexpr uint16_t kHighlightColor = 0x07FF;  // Cyan
constexpr uint16_t kWarningColor = 0xF800;    // Red

const char* pageTitle(UiPage page) {
  switch (page) {
    case UiPage::InstantFlow: return "Instant Flow";
    case UiPage::CumulativeLiters: return "Cumulative Liters";
    case UiPage::CumulativeCubicMeters: return "Cumulative m^3";
    case UiPage::SessionLiters: return "Session Liters";
    case UiPage::SessionCubicMeters: return "Session m^3";
    case UiPage::MaxFlow: return "Max Flow";
    case UiPage::EnterConfiguration: return "Enter Configuration";
    default: return "";
  }
}
}

void UiRenderer::begin() {
  M5StamPLC.setBacklight(true);
  auto& display = M5StamPLC.Display;
  display.setRotation(1);
  display.fillScreen(kBackgroundColor);
  display.setTextColor(kTextColor, kBackgroundColor);
  display.setFont(&fonts::Font0);
}

void UiRenderer::update(uint32_t nowMs, const UiRenderContext& context) {
  if (context.mode == UiMode::Idle) {
    M5StamPLC.setBacklight(false);
    auto& display = M5StamPLC.Display;
    display.startWrite();
    display.fillScreen(BLACK);
    display.endWrite();
    lastRenderMs_ = nowMs;
    return;
  }

  if (nowMs - lastRenderMs_ < kRefreshIntervalMs && context.mode == UiMode::Info) {
    return;
  }
  lastRenderMs_ = nowMs;
  M5StamPLC.setBacklight(true);

  switch (context.mode) {
    case UiMode::Info:
      drawInfoScreen(context);
      break;
    case UiMode::Configuration:
      drawConfigurationScreen(context);
      break;
    default:
      break;
  }
}

void UiRenderer::drawInfoScreen(const UiRenderContext& context) {
  auto& display = M5StamPLC.Display;
  display.startWrite();
  display.fillScreen(kBackgroundColor);
  display.setTextSize(1);
  display.setCursor(0, 0);

  display.setTextColor(kHighlightColor, kBackgroundColor);
  display.printf("%s\n", pageTitle(context.page));
  display.setTextColor(kTextColor, kBackgroundColor);
  display.printf("Total: %.2f L\n", context.totalSessionLiters);
  display.printf("Flow: %.2f L/s\n", context.aggregateFlowLps);
  display.printf("LED Red Step: %uL  Period: %ums\n",
                 context.ledVolumeStep,
                 context.ledPulsePeriodMs);

  drawWarningBanner(context);
  drawPropeller(context.propellerFrame, 210, 48, context.propellerActive);

  display.setCursor(0, 56);
  display.setTextColor(kHighlightColor, kBackgroundColor);
  display.print("Sensors\n");
  display.setTextColor(kTextColor, kBackgroundColor);

  const int leftColumnX = 0;
  const int rightColumnX = 110;
  int row = 0;
  char buffer[64];
  for (std::size_t i = 0; i < plc::kNumSensors; ++i) {
    const auto& sensor = context.sensors[i];
    const bool warning = (context.warningFlags >> i) & 0x01;
    const bool enabled = sensor.enabled;
    const bool ready = sensor.ready;

    double value = 0.0;
    const char* unit = "";
    switch (context.page) {
      case UiPage::InstantFlow:
        value = sensor.instantFlow;
        unit = "L/s";
        break;
      case UiPage::CumulativeLiters:
        value = sensor.cumulativeLiters;
        unit = "L";
        break;
      case UiPage::CumulativeCubicMeters:
        value = sensor.cumulativeLiters / 1000.0;
        unit = "m^3";
        break;
      case UiPage::SessionLiters:
        value = sensor.sessionLiters;
        unit = "L";
        break;
      case UiPage::SessionCubicMeters:
        value = sensor.sessionLiters / 1000.0f;
        unit = "m^3";
        break;
      case UiPage::MaxFlow:
        value = sensor.maxFlow;
        unit = "L/s";
        break;
      case UiPage::EnterConfiguration:
        break;
      default:
        break;
    }

    int x = (i < 4) ? leftColumnX : rightColumnX;
    int y = 70 + (row * 12);

    if (i == 4) {
      row = 0;
    }

    if (!enabled) {
      std::snprintf(buffer, sizeof(buffer), "%u: --", static_cast<unsigned>(i + 1));
    } else if (!ready) {
      std::snprintf(buffer, sizeof(buffer), "%u: WAIT", static_cast<unsigned>(i + 1));
    } else if (context.page == UiPage::EnterConfiguration) {
      std::snprintf(buffer, sizeof(buffer), "%u: Hold ENTER", static_cast<unsigned>(i + 1));
    } else {
      std::snprintf(buffer, sizeof(buffer), "%u: %6.2f %s",
                    static_cast<unsigned>(i + 1), value, unit);
    }

    if (warning) {
      display.setTextColor(kWarningColor, kBackgroundColor);
      display.setCursor(x, y);
      display.print("!");
      display.setTextColor(kTextColor, kBackgroundColor);
      display.setCursor(x + 8, y);
      display.print(buffer);
    } else {
      display.setCursor(x, y);
      display.print(buffer);
    }

    if (i == 3) {
      row = 0;
    } else {
      ++row;
    }
  }

  drawLegend(context);

  if (context.countdownActive) {
    drawCountdownOverlay(context);
  }

  display.endWrite();
}

void UiRenderer::drawConfigurationScreen(const UiRenderContext& context) {
  auto& display = M5StamPLC.Display;
  display.startWrite();
  display.fillScreen(kBackgroundColor);
  display.setTextColor(kHighlightColor, kBackgroundColor);
  display.setCursor(0, 0);
  display.print("Configuration Mode\n");
  display.setTextColor(kTextColor, kBackgroundColor);
  display.println("Use navigation buttons to edit settings.");
  drawWarningBanner(context);
  drawLegend(context);
  if (context.countdownActive) {
    drawCountdownOverlay(context);
  }
  display.endWrite();
}

void UiRenderer::drawPropeller(uint8_t frame, int16_t centerX, int16_t centerY, bool active) {
  auto& display = M5StamPLC.Display;
  const uint16_t circleColor = active ? kHighlightColor : kTextColor;
  display.drawCircle(centerX, centerY, 22, circleColor);
  display.drawCircle(centerX, centerY, 4, circleColor);

  constexpr float kPi = 3.14159265f;
  const float baseAngleRad = static_cast<float>(frame) * (kPi / 4.0f);
  const float bladeWidth = 12.0f;
  const float bladeLength = 20.0f;

  for (int i = 0; i < 4; ++i) {
    const float angle = baseAngleRad + static_cast<float>(i) * (kPi / 2.0f);
    const float cosA = std::cos(angle);
    const float sinA = std::sin(angle);

    const int16_t tipX = static_cast<int16_t>(centerX + cosA * bladeLength);
    const int16_t tipY = static_cast<int16_t>(centerY + sinA * bladeLength);
    const int16_t leftX = static_cast<int16_t>(centerX + cosA * 6.0f - sinA * bladeWidth);
    const int16_t leftY = static_cast<int16_t>(centerY + sinA * 6.0f + cosA * bladeWidth);
    const int16_t rightX = static_cast<int16_t>(centerX + cosA * 6.0f + sinA * bladeWidth);
    const int16_t rightY = static_cast<int16_t>(centerY + sinA * 6.0f - cosA * bladeWidth);

    display.fillTriangle(tipX, tipY, leftX, leftY, rightX, rightY, circleColor);
  }
}

void UiRenderer::drawLegend(const UiRenderContext& context) {
  auto& display = M5StamPLC.Display;
  display.setTextColor(kTextColor, kBackgroundColor);
  display.setCursor(0, 138);
  display.print("LED: Red pulses every ");
  display.print(context.ledVolumeStep);
  display.print("L | Green=Ready | Blue=Flow");
  display.setCursor(0, 150);
  display.print("Warn: ");
  display.print(context.warningSummary.c_str());
}

void UiRenderer::drawCountdownOverlay(const UiRenderContext& context) {
  auto& display = M5StamPLC.Display;
  const int16_t boxX = 40;
  const int16_t boxY = 60;
  const int16_t boxW = 160;
  const int16_t boxH = 60;
  display.fillRect(boxX, boxY, boxW, boxH, 0x39E7);  // Pale blue overlay
  display.drawRect(boxX, boxY, boxW, boxH, kHighlightColor);
  display.setCursor(boxX + 8, boxY + 20);
  display.setTextColor(kHighlightColor, 0x39E7);
  if (!context.countdownLabel.empty()) {
    display.print(context.countdownLabel.c_str());
  }
  display.setCursor(boxX + 8, boxY + 38);
  display.setTextColor(kTextColor, 0x39E7);
  display.printf("%u s", static_cast<unsigned>(context.countdownSeconds));
}

void UiRenderer::drawWarningBanner(const UiRenderContext& context) {
  if (!context.hasWarnings) {
    return;
  }
  auto& display = M5StamPLC.Display;
  const int16_t bannerY = 34;
  const int16_t bannerH = 18;
  display.fillRect(0, bannerY, 240, bannerH, kWarningColor);
  display.setTextColor(WHITE, kWarningColor);
  display.setCursor(4, bannerY + 4);
  display.print("!");
  display.setCursor(16, bannerY + 4);
  display.print(context.warningSummary.c_str());
  display.setTextColor(kTextColor, kBackgroundColor);
}
