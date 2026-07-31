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

  // Throttle every mode, not just Info. Configuration mode was exempt, so it
  // redrew on every logic-loop iteration (~1 ms) — a full 240x135x16bpp SPI blast
  // each time. Interactive modes get a faster cadence so edits feel immediate
  // without saturating the bus.
  const uint32_t interval =
      (context.mode == UiMode::Info) ? kRefreshIntervalMs : kInteractiveRefreshIntervalMs;
  if (nowMs - lastRenderMs_ < interval) {
    return;
  }
  lastRenderMs_ = nowMs;
  M5StamPLC.setBacklight(true);

  const ui_exporter::Screen* screen = screenRouter_->screenForMode(context.mode, context.page);
  if (!screen) {
    // Previously a silent `return`, which rendered an empty screen and gave no
    // hint that the exported dataset was missing the ID the router asked for.
    // A dataset/firmware screen-ID mismatch must be visible on the device.
    auto& display = M5StamPLC.Display;
    display.startWrite();
    display.fillScreen(backgroundColor_);
    drawAssetError(context);
    display.endWrite();
    return;
  }

  auto& display = M5StamPLC.Display;
  display.startWrite();
  display.fillScreen(backgroundColor_);
  drawWarningBanner(context);
  drawScreen(*screen, context, false);
  if (context.countdownActive) {
    if (const auto* overlay = screenRouter_->overlayForCountdown(context.countdownScreenId)) {
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
    case ui_exporter::ElementType::Scrollbar:
      drawScrollbarElement(element, context);
      break;
  }
}

void UiRenderer::drawScrollbarElement(const ui_exporter::Element& element,
                                      const UiRenderContext& context) {
  // Carries no binding: the step count and current step come from the active
  // navigation level, so one authored element works on every page of that level.
  const int steps = (context.mode == UiMode::Info)
                        ? static_cast<int>(UiPage::Count)
                        : 1;
  const int index = (context.mode == UiMode::Info)
                        ? static_cast<int>(context.page)
                        : 0;
  if (steps <= 0) {
    return;
  }

  auto& display = M5StamPLC.Display;
  const int16_t width = element.width > 0 ? element.width : 4;
  const int16_t height = element.height > 0 ? element.height : 60;

  display.drawRect(element.x, element.y, width, height, badgeBorderColor_);

  // One segment per step, with a 1 px gap so adjacent segments stay legible.
  const int16_t segment = static_cast<int16_t>(height / steps);
  if (segment <= 0) {
    return;
  }
  const int16_t thumbY = static_cast<int16_t>(element.y + segment * index);
  const int16_t thumbH = static_cast<int16_t>(segment > 2 ? segment - 1 : segment);
  display.fillRect(static_cast<int16_t>(element.x + 1),
                   thumbY,
                   static_cast<int16_t>(width - 2 > 0 ? width - 2 : 1),
                   thumbH,
                   highlightColor_);
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
    // Size the badge box to its rendered text plus padding. drawBoxElement's
    // fallback is an opaque 40x12 fillRect, and no badge in the dataset carries an
    // explicit width — so a status badge at x=60 painted over the next column's
    // label at x=69. The web mockup sizes badges to content, so the 40px default
    // also meant the mockup could not show the collision.
    constexpr int16_t kBadgePadX = 3;
    constexpr int16_t kBadgePadY = 2;
    ui_exporter::Element sized = element;
    if (sized.width <= 0) {
      sized.width = static_cast<int16_t>(measureTextWidth(element, text) + kBadgePadX * 2);
    }
    if (sized.height <= 0) {
      sized.height = static_cast<int16_t>(8 + kBadgePadY * 2);
    }
    drawBoxElement(sized, badgeBackgroundColor_, badgeBorderColor_);
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
  if (element.assetId && std::strcmp(element.assetId, "flow-dots") == 0) {
    drawFlowDots(element, context);
  }
}

void UiRenderer::drawFlowDots(const ui_exporter::Element& element,
                              const UiRenderContext& context) {
  auto& display = M5StamPLC.Display;
  const bool active = context.aggregateFlowLps > 0.001;
  const int16_t centerY = element.y + (element.height / 2);
  const int16_t radius = std::min(element.width, element.height) / 4;
  const int16_t leftX = element.x + radius;
  const int16_t rightX = element.x + element.width - radius;

  if (!active) {
    // Single red dot when no flow
    display.fillCircle(leftX, centerY, radius, 0xF800); // Red
    display.fillCircle(rightX, centerY, radius, badgeBackgroundColor_); // Clear second dot
  } else {
    // Alternating blue dots based on flow rate
    const uint32_t nowMs = millis();
    float flow = context.aggregateFlowLps;
    if (flow < 0.1f) flow = 0.1f;
    if (flow > 10.0f) flow = 10.0f;
    
    const uint32_t periodMs = static_cast<uint32_t>(1000.0f / flow);
    const bool phase = (nowMs % periodMs) < (periodMs / 2);
    
    uint16_t blueColor = 0x001F; // Blue in RGB565
    if (phase) {
      display.fillCircle(leftX, centerY, radius, blueColor);
      display.fillCircle(rightX, centerY, radius, badgeBackgroundColor_);
    } else {
      display.fillCircle(leftX, centerY, radius, badgeBackgroundColor_);
      display.fillCircle(rightX, centerY, radius, blueColor);
    }
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

void UiRenderer::drawAssetError(const UiRenderContext& context) {
  auto& display = M5StamPLC.Display;
  display.setTextSize(1);
  display.setTextColor(warningColor_, backgroundColor_);
  display.setCursor(4, 4);
  display.print("UI ASSET ERROR");
  display.setTextColor(textColor_, backgroundColor_);
  display.setCursor(4, 20);
  display.print("No screen for");
  char detail[40];
  std::snprintf(detail,
                sizeof(detail),
                "mode %u page %u",
                static_cast<unsigned>(context.mode),
                static_cast<unsigned>(context.page));
  display.setCursor(4, 32);
  display.print(detail);
  display.setCursor(4, 52);
  display.print("Re-run the UI");
  display.setCursor(4, 64);
  display.print("exporter.");
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
