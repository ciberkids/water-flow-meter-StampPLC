#pragma once

#include "bus/spi_arbiter.h"

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

  /**
   * Binds the shared-SPI arbiter (§4.10). Optional: with none bound the renderer assumes it
   * owns the bus, which is correct on a build with no card support.
   *
   * When bound, update() asks `mayBeginFrame()` BEFORE opening a frame and never during one, and
   * honours `consumeFullRepaintRequest()` after a handover. Checking mid-frame would reintroduce
   * exactly the torn frame the arbiter exists to prevent.
   */
  void bindSpiArbiter(plc::SpiArbiter* arbiter);

 private:
  /**
   * Paints the Select Menu page directly, without consulting the screen table.
   *
   * §3.4.1: the gesture that opens this page "works even if the active pack draws nothing at
   * all", which is only true if the firmware draws the page itself. So this deliberately shares
   * no code with the generated-table path — a pack cannot influence what it looks like, and a
   * pack that breaks the table cannot break this.
   */
  void drawPackSelector(const UiRenderContext& context);

 public:
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
  void drawScrollbarElement(const ui_exporter::Element& element, const UiRenderContext& context);
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
  /** Info mode is telemetry at 1 Hz; nothing changes faster than that. */
  static constexpr uint32_t kRefreshIntervalMs = 1000;
  /**
   * Editors and countdowns must feel responsive (§7: acknowledge within 100 ms) without
   * redrawing on every logic-loop tick. Selected by UiRenderContext::interactive; it used to
   * be selected by a UiMode value that no code path sets, which made it dead.
   */
  static constexpr uint32_t kInteractiveRefreshIntervalMs = 80;
  uint16_t backgroundColor_ = 0x0000;
  uint16_t textColor_ = 0xFFFF;
  uint16_t highlightColor_ = 0x07FF;
  uint16_t warningColor_ = 0xF800;
  uint16_t badgeBackgroundColor_ = 0x0000;
  uint16_t badgeBorderColor_ = 0xFFFF;
  uint16_t legendColor_ = 0xFFFF;
  /**
   * The screen currently on the panel, so a navigation step can be painted immediately
   * instead of waiting out the interval. Without this, tapping DOWN on the info ring could
   * take a full second to show the next page — the same §7 breach as the dead fast cadence,
   * on a path no `interactive` flag covers because nothing is animating.
   */
  const ui_exporter::Screen* lastScreen_ = nullptr;
  /**
   * Whether the last painted frame carried a countdown overlay.
   *
   * The overlay is drawn ON TOP of the base screen, so `lastScreen_` cannot see it come or
   * go. Releasing a hold-to-confirm early clears `interactive` on the same pass, which drops
   * the interval back to 1 s — without this the abandoned overlay would stay on the panel for
   * up to a second, which reads as "the countdown is still running".
   */
  bool lastCountdownActive_ = false;
  const ui::UiScreenRouter* screenRouter_ = nullptr;
  const ui::UiBindingResolver* bindingResolver_ = nullptr;
  plc::SpiArbiter* spiArbiter_ = nullptr;
};
