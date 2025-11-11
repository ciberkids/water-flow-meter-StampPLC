#include "ui/core/ui_renderer.h"

#include <M5StamPLC.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <cstdlib>

#include "ui/core/ui_bindings.h"
#include "ui/core/ui_screen_router.h"

namespace {

constexpr int kGlyphWidthBase = 6;
constexpr int kGlyphWidthValue = 7;
constexpr uint16_t kCountdownOverlayColor = 0x39E7;

bool bindingStartsWith(const ui_exporter::Element& element, const char* prefix) {
  if (!element.bindingId || !prefix) {
    return false;
  }
  return std::strncmp(element.bindingId, prefix, std::strlen(prefix)) == 0;
}

int16_t applyAlignment(int16_t origin, int16_t width, ui_exporter::TextAlign align) {
  switch (align) {
    case ui_exporter::TextAlign::Center:
      return origin - (width / 2);
    case ui_exporter::TextAlign::Right:
      return origin - width;
    default:
      return origin;
  }
}

}  // namespace

void UiRenderer::begin() {
  M5StamPLC.setBacklight(true);
  auto& display = M5StamPLC.Display;
  display.setRotation(1);
  display.fillScreen(backgroundColor_);
  display.setTextColor(textColor_, backgroundColor_);
  display.setFont(&fonts::Font0);
}

void UiRenderer::applyTheme(const ui::ThemePalette& palette) {
  backgroundColor_ = toRgb565(palette.color("displayBackground", backgroundColor_));
  textColor_ = toRgb565(palette.color("textPrimary", textColor_));
  highlightColor_ = toRgb565(palette.color("value", highlightColor_));
  warningColor_ = toRgb565(palette.color("badgeBorder", warningColor_));
  badgeBackgroundColor_ = toRgb565(palette.color("badgeBackground", badgeBackgroundColor_));
  badgeBorderColor_ = toRgb565(palette.color("badgeBorder", badgeBorderColor_));
  legendColor_ = toRgb565(palette.color("legend", legendColor_));
}

void UiRenderer::bindScreenRouter(const ui::UiScreenRouter* router) {
  screenRouter_ = router;
}

void UiRenderer::bindBindingResolver(const ui::UiBindingResolver* resolver) {
  bindingResolver_ = resolver;
}

void UiRenderer::update(uint32_t nowMs, const UiRenderContext& context) {
  if (!screenRouter_) {
    return;
  }

  if (context.mode == UiMode::Idle) {
    M5StamPLC.setBacklight(false);
    auto& display = M5StamPLC.Display;
    display.startWrite();
    display.fillScreen(backgroundColor_);
    display.endWrite();
    lastRenderMs_ = nowMs;
    return;
  }

  if (nowMs - lastRenderMs_ < kRefreshIntervalMs && context.mode == UiMode::Info) {
    return;
  }
  lastRenderMs_ = nowMs;
  M5StamPLC.setBacklight(true);

  const ui_exporter::Screen* screen = screenRouter_->screenForMode(context.mode);
  if (!screen) {
    return;
  }

  auto& display = M5StamPLC.Display;
  display.startWrite();
  display.fillScreen(backgroundColor_);
  drawWarningBanner(context);
  drawScreen(*screen, context, false);
  if (context.countdownActive) {
    if (const auto* overlay = screenRouter_->overlayForCountdown()) {
      drawScreen(*overlay, context, true);
    }
  }
  display.endWrite();
}

void UiRenderer::drawScreen(const ui_exporter::Screen& screen,
                            const UiRenderContext& context,
                            bool overlay) {
  for (std::size_t i = 0; i < screen.elementCount; ++i) {
    drawElement(screen.elements[i], context, overlay);
  }
}

void UiRenderer::drawElement(const ui_exporter::Element& element,
                             const UiRenderContext& context,
                             bool overlay) {
  switch (element.type) {
    case ui_exporter::ElementType::Text:
    case ui_exporter::ElementType::Value:
    case ui_exporter::ElementType::Badge:
      drawTextElement(element, context);
      break;
    case ui_exporter::ElementType::Box:
      drawBoxElement(element,
                     overlay ? kCountdownOverlayColor : badgeBackgroundColor_,
                     badgeBorderColor_);
      break;
    case ui_exporter::ElementType::Icon:
      drawIconElement(element, context);
      break;
  }
}

bool UiRenderer::resolveElementText(const ui_exporter::Element& element,
                                    const UiRenderContext& context,
                                    char* buffer,
                                    std::size_t bufferSize,
                                    const char** textOut) const {
  if (!textOut) {
    return false;
  }
  const char* fallback = (element.text && element.text->text) ? element.text->text : "";
  *textOut = fallback;
  if (!bindingResolver_ || !element.bindingId) {
    return !std::strlen(fallback) ? false : true;
  }
  if (bindingResolver_->resolveText(context, element, buffer, bufferSize)) {
    *textOut = buffer;
    return true;
  }
  return !std::strlen(fallback) ? false : true;
}

void UiRenderer::drawTextElement(const ui_exporter::Element& element,
                                 const UiRenderContext& context) {
  auto& display = M5StamPLC.Display;
  const ui_exporter::TextPayload* payload = element.text;
  const ui_exporter::TextAlign align = payload ? payload->align : ui_exporter::TextAlign::Left;
  const bool isBadge = element.type == ui_exporter::ElementType::Badge;

  char buffer[96];
  const char* text = nullptr;
  if (!resolveElementText(element, context, buffer, sizeof(buffer), &text)) {
    return;
  }

  if (isBadge) {
    drawBoxElement(element, badgeBackgroundColor_, badgeBorderColor_);
  }

  const uint16_t color = colorForText(element, context, element.bindingId);
  const uint16_t bg = isBadge ? badgeBackgroundColor_ : backgroundColor_;

  display.setTextSize(1);
  display.setTextColor(color, bg);

  const int16_t textWidth = measureTextWidth(element, text);
  const int16_t cursorX = applyAlignment(element.x, textWidth, align);
  const int16_t cursorY = element.y;
  display.setCursor(cursorX, cursorY);
  display.print(text);
}

void UiRenderer::drawBoxElement(const ui_exporter::Element& element,
                                uint16_t fillColor,
                                uint16_t borderColor) {
  auto& display = M5StamPLC.Display;
  const int16_t width = element.width > 0 ? element.width : 40;
  const int16_t height = element.height > 0 ? element.height : 12;
  display.fillRect(element.x, element.y, width, height, fillColor);
  if (borderColor != fillColor) {
    display.drawRect(element.x, element.y, width, height, borderColor);
  }
}

void UiRenderer::drawIconElement(const ui_exporter::Element& element,
                                 const UiRenderContext& context) {
  if (element.assetId && std::strcmp(element.assetId, "propeller") == 0) {
    drawPropeller(element, context);
  }
}

void UiRenderer::drawPropeller(const ui_exporter::Element& element,
                               const UiRenderContext& context) {
  auto& display = M5StamPLC.Display;
  const bool active = context.propellerActive;
  const uint16_t circleColor = active ? highlightColor_ : textColor_;
  const int16_t radius = std::min(element.width, element.height) / 2;
  const int16_t centerX = element.x + radius;
  const int16_t centerY = element.y + radius;
  display.drawCircle(centerX, centerY, radius, circleColor);
  display.drawCircle(centerX, centerY, radius / 5, circleColor);

  constexpr float kPi = 3.14159265f;
  const float baseAngleRad = static_cast<float>(context.propellerFrame) * (kPi / 4.0f);
  const float bladeWidth = static_cast<float>(radius) * 0.55f;
  const float bladeLength = static_cast<float>(radius) * 0.9f;

  for (int i = 0; i < 4; ++i) {
    const float angle = baseAngleRad + static_cast<float>(i) * (kPi / 2.0f);
    const float cosA = std::cos(angle);
    const float sinA = std::sin(angle);

    const int16_t tipX = static_cast<int16_t>(centerX + cosA * bladeLength);
    const int16_t tipY = static_cast<int16_t>(centerY + sinA * bladeLength);
    const int16_t leftX = static_cast<int16_t>(centerX + cosA * (radius / 3.0f) - sinA * bladeWidth);
    const int16_t leftY = static_cast<int16_t>(centerY + sinA * (radius / 3.0f) + cosA * bladeWidth);
    const int16_t rightX = static_cast<int16_t>(centerX + cosA * (radius / 3.0f) + sinA * bladeWidth);
    const int16_t rightY = static_cast<int16_t>(centerY + sinA * (radius / 3.0f) - cosA * bladeWidth);

    display.fillTriangle(tipX, tipY, leftX, leftY, rightX, rightY, circleColor);
  }
}

uint16_t UiRenderer::colorForText(const ui_exporter::Element& element,
                                  const UiRenderContext& context,
                                  const char* bindingId) const {
  uint16_t color = textColor_;
  if (element.type == ui_exporter::ElementType::Value) {
    color = highlightColor_;
  }
  if (element.text) {
    switch (element.text->emphasis) {
      case ui_exporter::TextEmphasis::Strong:
        color = highlightColor_;
        break;
      case ui_exporter::TextEmphasis::Muted:
        color = legendColor_;
        break;
      default:
        break;
    }
  }
  if (bindingId && std::strncmp(bindingId, "sensor.", 7) == 0) {
    const char* start = bindingId + 7;
    char* end = nullptr;
    const unsigned long value = std::strtoul(start, &end, 10);
    if (end != start && value > 0 && value <= context.sensors.size()) {
      const std::size_t index = static_cast<std::size_t>(value - 1);
      if ((context.warningFlags >> index) & 0x01) {
        color = warningColor_;
      }
    }
  }
  if (bindingId && std::strcmp(bindingId, "legend.warning") == 0) {
    color = context.hasWarnings ? warningColor_ : legendColor_;
  }
  return color;
}

void UiRenderer::drawWarningBanner(const UiRenderContext& context) {
  if (!context.hasWarnings) {
    return;
  }
  auto& display = M5StamPLC.Display;
  const int16_t bannerY = 34;
  const int16_t bannerH = 18;
  display.fillRect(0, bannerY, 240, bannerH, warningColor_);
  display.setTextColor(WHITE, warningColor_);
  display.setCursor(4, bannerY + 4);
  display.print("!");
  display.setCursor(16, bannerY + 4);
  display.print(context.warningSummary.c_str());
  display.setTextColor(textColor_, backgroundColor_);
}

int16_t UiRenderer::measureTextWidth(const ui_exporter::Element& element, const char* text) const {
  if (!text) {
    return 0;
  }
  const std::size_t length = std::strlen(text);
  return static_cast<int16_t>(length) * glyphWidthFor(element);
}

int16_t UiRenderer::glyphWidthFor(const ui_exporter::Element& element) const {
  return (element.type == ui_exporter::ElementType::Value) ? kGlyphWidthValue : kGlyphWidthBase;
}

uint16_t UiRenderer::toRgb565(std::uint32_t argb) {
  const std::uint8_t r = static_cast<std::uint8_t>((argb >> 16) & 0xFF);
  const std::uint8_t g = static_cast<std::uint8_t>((argb >> 8) & 0xFF);
  const std::uint8_t b = static_cast<std::uint8_t>(argb & 0xFF);
  const std::uint16_t r5 = static_cast<std::uint16_t>(r >> 3);
  const std::uint16_t g6 = static_cast<std::uint16_t>(g >> 2);
  const std::uint16_t b5 = static_cast<std::uint16_t>(b >> 3);
  return static_cast<std::uint16_t>((r5 << 11) | (g6 << 5) | b5);
}
