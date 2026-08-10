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

/**
 * The panel, in pixels. 240 x 135 — the StampPLC's built-in display.
 *
 * Named because the width is now needed in two places (clipping overlong text, and the full-width
 * warning banner), and a second bare `240` is how the two would come to disagree.
 */
constexpr int kPanelWidth = 240;
constexpr uint16_t kCountdownOverlayColor = 0x39E7;

/**
 * Closes the arbiter's frame on every exit path from update().
 *
 * update() returns from several places — idle, no screen, an asset error. A frame left open
 * because one of those paths forgot to close it would block the card until the arbiter's 500 ms
 * timeout, on every single pass, which would look like the card being intermittently unreadable.
 * RAII removes the possibility rather than relying on each branch remembering.
 */
struct FrameGuard {
  plc::SpiArbiter* arbiter;
  uint32_t nowMs;
  ~FrameGuard() {
    if (arbiter) {
      arbiter->noteFrameEnded(nowMs);
    }
  }
};


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

void UiRenderer::bindSpiArbiter(plc::SpiArbiter* arbiter) { spiArbiter_ = arbiter; }

void UiRenderer::update(uint32_t nowMs, const UiRenderContext& context) {
  if (!screenRouter_) {
    return;
  }

  // §4.10. Asked once, here, before anything is drawn. While the card holds the shared bus the
  // renderer draws NOTHING rather than something partial: a skipped frame is invisible, half a
  // frame is an artifact. The LEDs report during the handover because the panel cannot.
  if (spiArbiter_ && !spiArbiter_->mayBeginFrame()) {
    return;
  }
  if (spiArbiter_ && spiArbiter_->consumeFullRepaintRequest()) {
    // Whatever is on the panel describes state from before the handover, by an unknown amount, so
    // an incremental update would leave it visible. Forcing the full path discards that.
    lastScreen_ = nullptr;
  }
  if (spiArbiter_) {
    spiArbiter_->noteFrameBegan(nowMs);
  }
  const FrameGuard frameGuard{spiArbiter_, nowMs};

  // Before every table-driven path, and sharing none of it (§3.4.1).
  if (context.selectorActive) {
    drawPackSelector(context);
    return;
  }

  if (context.mode == UiMode::Idle) {
    // Once. Blanking the panel is idempotent and nothing changes while the display is off, so
    // repeating it only burns the sampler's I²C bus and the SPI bus. See idlePainted_.
    if (idlePainted_) {
      return;
    }
    idlePainted_ = true;
    M5StamPLC.setBacklight(false);
    auto& display = M5StamPLC.Display;
    display.startWrite();
    display.fillScreen(backgroundColor_);
    display.endWrite();
    lastRenderMs_ = nowMs;
    // Nothing is on the panel any more, so waking must paint rather than assume the frame
    // it last drew is still there.
    lastScreen_ = nullptr;
    lastCountdownActive_ = false;
    return;
  }

  // The navigator owns the current position; the router is only the by-ID lookup and
  // the seed for the root. Asking the router for mode+page would ignore any descent.
  //
  // Resolved before the throttle, because whether the screen changed is half of the
  // decision about whether this pass has to paint.
  const ui_exporter::Screen* screen = context.currentScreen
                                          ? context.currentScreen
                                          : screenRouter_->screenForMode(context.mode, context.page);

  // Throttle every mode. The cadence used to be chosen by `mode != UiMode::Info`, i.e. by
  // UiMode::Configuration — which nothing sets, so every awake screen got the 1 Hz telemetry
  // interval and editors, countdowns and the §5.4 acceleration ramp all updated once a
  // second. The condition now comes from the context (see UiRenderContext::interactive),
  // plus an unconditional repaint when the screen itself changed so a navigation step is
  // acknowledged within §7's 100 ms instead of at the next interval boundary.
  const uint32_t interval = context.interactive ? kInteractiveRefreshIntervalMs : kRefreshIntervalMs;
  // The overlay is not part of `screen`, so its arrival and — the case that bites — its
  // teardown on an aborted hold have to be noticed separately.
  const bool contentChanged =
      screen != lastScreen_ || context.countdownActive != lastCountdownActive_;
  if (!contentChanged && nowMs - lastRenderMs_ < interval) {
    return;
  }
  lastRenderMs_ = nowMs;
  lastScreen_ = screen;
  lastCountdownActive_ = context.countdownActive;
  // Waking re-arms the idle path, so the next descent into Idle paints its blank frame again.
  idlePainted_ = false;
  M5StamPLC.setBacklight(true);

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
  // Ring position comes from the navigator, so the bar works at every level rather
  // than only on the info ring.
  const int steps = context.ringCount > 0 ? context.ringCount : 1;
  const int index = context.ringCount > 0 ? context.ringIndex : 0;
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

  /**
   * CLIP to what the panel can actually hold, marking the cut with a trailing `~`.
   *
   * The buffer is 96 characters and the panel is 40, so a long text setting rendered straight off
   * the end of the display: `config.mqtt.host` holds 64 bytes, which from x=80 draws 384 px on a
   * 240 px panel — over the scrollbar, over nothing, gone. The screen specs hid it by declaring
   * those rows' worst case as `?`, one character, so the geometry audit was checking a fiction.
   *
   * Truncation is the only honest option: 240 px at 6 px per glyph is 40 characters, so a 64-byte
   * broker host CANNOT be shown in full and no layout can change that. The `~` says so, rather than
   * letting a clipped hostname read as the whole one — which would send somebody debugging a broker
   * they are actually pointed at correctly.
   *
   * Badges are exempt: they size their own box to their content, so they clip nothing.
   */
  if (!isBadge) {
    const int advance = element.type == ui_exporter::ElementType::Value ? kGlyphWidthValue : kGlyphWidthBase;
    const int available = (element.width > 0 ? element.width : kPanelWidth - element.x);
    const int maxChars = available / advance;
    if (maxChars > 1 && static_cast<int>(std::strlen(text)) > maxChars) {
      if (text != buffer) {
        std::snprintf(buffer, sizeof(buffer), "%s", text);
      }
      buffer[maxChars - 1] = '~';
      buffer[maxChars] = '\0';
      text = buffer;
    }
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

/**
 * FOUR dots in a chase, at a rate set by the aggregate flow.
 *
 * It drew TWO, alternating left/right, while the agreed design (and the mockup, and the spec's
 * rendered gallery) had four advancing. The panel and the requirement had drifted, and the panel was
 * the one an operator would see.
 *
 * Geometry is derived from the element rather than from its ends: `spacing = width / 4` and
 * `radius = min(spacing, height) / 3`, so four dots fit whatever box the dataset gives them. The old
 * version placed dots at `x + radius` and `x + width - radius` with `radius = min(w,h)/4`, which only
 * ever described two.
 *
 * Colour comes from the palette, not from literals. The previous code hardcoded 0xF800 and 0x001F,
 * so a theme change moved every other element and left these two dots behind.
 */
void UiRenderer::drawFlowDots(const ui_exporter::Element& element,
                              const UiRenderContext& context) {
  auto& display = M5StamPLC.Display;
  constexpr int kDotCount = 4;
  const int spacing = (element.width > 0 ? element.width : 40) / kDotCount;
  const int height = element.height > 0 ? element.height : 12;
  const int radius = std::max(1, std::min(spacing, height) / 3);
  const int16_t centerY = static_cast<int16_t>(element.y + height / 2);
  const bool active = context.aggregateFlowLps > 0.001;

  /**
   * ONE STEP PER REPAINT, not a rate derived from the flow.
   *
   * The obvious version computes a step period from the flow — 25 ms at the 10 L/s clamp — and reads
   * `millis()` against it. That cannot work here: an info page repaints at 1 Hz
   * (kRefreshIntervalMs), so a 25 ms counter is sampled forty times slower than it advances and the
   * "chase" arrives as a different arbitrary dot each second. It looks like noise because it IS
   * noise — the phase is aliased past recognition.
   *
   * Advancing once per painted frame makes the motion mean what it appears to mean: each frame the
   * panel draws moves the chase one place. Flow decides WHETHER it moves, and the repaint rate
   * decides how fast — which is the only rate a viewer can actually perceive.
   */
  int litIndex = -1;
  if (active) {
    litIndex = static_cast<int>(flowDotPhase_ % kDotCount);
    flowDotPhase_ += 1;
  } else {
    // Reset so flow always restarts the chase at the first dot rather than wherever it stopped.
    flowDotPhase_ = 0;
  }

  for (int i = 0; i < kDotCount; ++i) {
    const int16_t cx = static_cast<int16_t>(element.x + spacing / 2 + i * spacing);
    if (i == litIndex) {
      display.fillCircle(cx, centerY, radius, highlightColor_);
    } else {
      // An OUTLINE for the unlit dots rather than a filled background circle. Filling them with the
      // badge background painted four opaque discs over whatever the screen had there, which on a
      // themed background read as four holes.
      display.drawCircle(cx, centerY, radius, legendColor_);
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
  display.fillRect(0, bannerY, kPanelWidth, bannerH, warningColor_);
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

void UiRenderer::drawPackSelector(const UiRenderContext& context) {
  auto& display = M5StamPLC.Display;
  M5StamPLC.setBacklight(true);

  // Repainted in full on every pass. The list is short, this page is open for seconds at a time,
  // and the alternative — tracking which row changed — would be complexity spent on the one
  // screen whose whole purpose is to work when other things do not.
  display.startWrite();
  display.fillScreen(backgroundColor_);
  display.setFont(&fonts::Font0);

  display.setTextColor(textColor_, backgroundColor_);
  display.drawString("SELECT MENU", 4, 4);

  const ui::PackSelector* selector = context.selector;
  if (!selector) {
    // selectorActive without a selector is a wiring bug, not an operator-visible state. Say so
    // rather than painting an empty page they cannot escape.
    display.setTextColor(warningColor_, backgroundColor_);
    display.drawString("selector unavailable", 4, 20);
    display.endWrite();
    lastScreen_ = nullptr;
    return;
  }

  for (std::size_t i = 0; i < selector->entryCount(); ++i) {
    const int32_t y = static_cast<int32_t>(20 + i * 12);
    const bool onCursor = i == selector->cursor();
    // The cursor is a leading '>' rather than an inverted row: Font0 has no bold, and inverting
    // would mean a fillRect per row — more bus traffic on the one screen that most needs to be
    // dependable.
    display.setTextColor(onCursor ? highlightColor_ : textColor_, backgroundColor_);
    display.drawString(onCursor ? ">" : " ", 4, y);
    display.drawString(selector->labelAt(i), 16, y);
    if (selector->isActive(i)) {
      // §3.4: the running menu is marked, so the operator can see what they are leaving.
      display.drawString("*", 228, y);
    }
  }

  display.setTextColor(textColor_, backgroundColor_);
  if (selector->truncated()) {
    // Said out loud rather than silently dropped — an operator whose pack is missing from the
    // list would otherwise conclude the card was faulty.
    display.drawString("...more on card, not shown", 4, 116);
  } else {
    display.drawString("UP/DN choose  ENTER select", 4, 116);
  }
  display.endWrite();

  // The table-driven path tracks the last screen to decide on incremental updates; this page is
  // not in the table, so leaving a stale pointer there would let the next ordinary frame skip its
  // repaint and show the selector underneath.
  lastScreen_ = nullptr;
}
