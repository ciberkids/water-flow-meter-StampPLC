#pragma once

#include <cstddef>
#include <cstdint>

#include <array>

#include "ui/core/ui_controller.h"
#include "ui/theme/theme_palette.h"

namespace ui {
class UiBindingResolver;
class UiScreenRouter;
}  // namespace ui

class UiRenderer {
 public:
  void begin();
  void applyTheme(const ui::ThemePalette& palette);
  void bindScreenRouter(const ui::UiScreenRouter* router);
  void bindBindingResolver(const ui::UiBindingResolver* resolver);

  void update(uint32_t nowMs, const UiRenderContext& context);

 private:
  void drawScreen(const ui_exporter::Screen& screen,
                  const UiRenderContext& context,
                  bool overlay);
  void drawElement(const ui_exporter::Element& element,
                   const UiRenderContext& context,
                   bool overlay);
  void drawTextElement(const ui_exporter::Element& element,
                       const UiRenderContext& context);
  void drawBoxElement(const ui_exporter::Element& element, uint16_t fillColor, uint16_t borderColor);
  void drawIconElement(const ui_exporter::Element& element, const UiRenderContext& context);
  void drawFlowDots(const ui_exporter::Element& element, const UiRenderContext& context);
  void drawWarningBanner(const UiRenderContext& context);
  void drawAssetError(const UiRenderContext& context);
  int16_t measureTextWidth(const ui_exporter::Element& element, const char* text) const;
  int16_t glyphWidthFor(const ui_exporter::Element& element) const;
  uint16_t colorForText(const ui_exporter::Element& element,
                        const UiRenderContext& context,
                        const char* bindingId) const;
  bool resolveElementText(const ui_exporter::Element& element,
                          const UiRenderContext& context,
                          char* buffer,
                          std::size_t bufferSize,
                          const char** textOut) const;

  static uint16_t toRgb565(std::uint32_t argb);

  uint32_t lastRenderMs_ = 0;
  static constexpr uint32_t kRefreshIntervalMs = 1000;
  uint16_t backgroundColor_ = 0x0000;
  uint16_t textColor_ = 0xFFFF;
  uint16_t highlightColor_ = 0x07FF;
  uint16_t warningColor_ = 0xF800;
  uint16_t badgeBackgroundColor_ = 0x0000;
  uint16_t badgeBorderColor_ = 0xFFFF;
  uint16_t legendColor_ = 0xFFFF;
  const ui::UiScreenRouter* screenRouter_ = nullptr;
  const ui::UiBindingResolver* bindingResolver_ = nullptr;
};
