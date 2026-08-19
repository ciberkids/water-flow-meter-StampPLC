/**
 * The firmware-drawn warning banner (§2c), as data — the SECOND page the dataset cannot describe.
 *
 * `packSelector.ts` owns the Select Menu for that reason and this owns the banner for a different one.
 * The Select Menu is a whole page the firmware short-circuits to; the banner is an 18 px band
 * `UiRenderer::drawWarningBanner` paints edge to edge over whatever screen is showing, from a string
 * `UiController::update` composed. No element in `src/data/screens.json` declares it, no binding reaches
 * it, and the exporter never sees it — so if the simulator does not own it, the simulator does not have it.
 *
 * It did not have it. Until this module existed the mockup drew NO banner at all while
 * `Display_Per_Screen_Spec.md` §2c documented its pixels and `tools/audit/screen-spec.ts` measured
 * elements against its band. That is the more dangerous half of a parity gap than a wrong number: a wrong
 * number is visible on the panel, a missing layer looks exactly like a screen with nothing wrong.
 *
 * Every number and rule below is read off the firmware, and `__tests__/warningBanner.test.ts` pins them.
 */

import {
  uncalibratedSensorNumbers,
  warningSensorNumbers,
  warningSummaryText,
  type SimulatedSensor
} from "./sensorConfig";

/**
 * Geometry and copy, read off `UiRenderer::drawWarningBanner`.
 *
 * The band is y 116…133 — `bannerY = 116`, `bannerH = 18` — so 134 is the first row below it, which is
 * why `tools/audit/screen-spec.ts` and `tools/audit/screen-geometry.ts` both spell the bottom 134. It is
 * the panel's LAST band: the firmware carries a `static_assert(bannerY + bannerH <= kPanelHeight)` so
 * "does it still end on the panel" is a build failure rather than arithmetic somebody re-checks.
 */
export const warningBannerLayout = {
  y: 116,
  height: 18,
  /** Two separate draws at two x positions, both at `bannerY + 4`. */
  markerX: 4,
  marker: "!",
  textX: 16,
  textDy: 4,
  panelWidth: 240,
  glyphWidth: 6,
  /**
   * What the summary has to fit: the footer's 40 columns at 6 px, less the two the marker at x=4 and the
   * gap before x=16 spend. `UiController::update` composes against this number, and its widest branch is
   * 33 — four columns spare.
   */
  columns: 37,
  /**
   * `warningColor_`, and it is the theme's **badgeBorder** token, NOT a warning token.
   * `ui_renderer.cpp` maps it with `toRgb565(palette.color("badgeBorder", warningColor_))`. The mockup's
   * `ThemeColorTokens` has no `warning` key and must not gain one to serve this band — inventing a token
   * here would put a colour on the mockup's panel that no palette on the device can produce.
   */
  fillToken: "badgeBorder",
  /** `setTextColor(WHITE, warningColor_)` — a literal, not a token. */
  textColor: "#FFFFFF"
} as const;

/**
 * `UiRenderContext::bannerActive()`, mirrored — the banner's gate, and its one home on this side too.
 *
 * THREE TERMS, not two: `hasWarnings || (uncalibratedCount > 0 && !editorActive)`.
 *
 * (1) A SAMPLING FAULT ALWAYS RAISES IT. That is `hasWarnings`, i.e. `REG_UNDERSAMPLING_FLAGS != 0`,
 *     which is `warningSensorNumbers` here.
 *
 * (2) A COMMISSIONING GAP RAISES IT TOO, which is the half §2c widened. Before that the gate was
 *     `hasWarnings` alone, so the two uncalibrated phrasings the firmware composed every pass painted
 *     nowhere. It is only SAFE because the band moved to the footer row first: at `bannerY = 34` a
 *     factory-fresh device would have worn a permanent banner across the very config rows an operator
 *     reads while calibrating — the objection `UiRenderContext` used to carry, which was correct at 34
 *     and is retired at 116, not refuted.
 *
 * (3) EXCEPT WHILE A VALUE EDITOR IS OPEN, where the uncalibrated half is suppressed. Every one of the
 *     thirteen `config-*-edit` screens carries a footer hint ending in `hold=cancel` at y=124 — the only
 *     place the abort gesture is written down anywhere — and the band is exactly that row. On a
 *     factory-fresh device `uncalibratedCount` is 8, so without this term the banner would hide
 *     `hold=cancel` permanently, on the screens whose use is the thing that clears the condition.
 *
 * A sampling fault is EXEMPT from (3), on purpose: `hasWarnings` says a reading is wrong — the number on
 * the panel is not the flow — and that outranks a gesture reminder on every screen. "Setup unfinished"
 * does not.
 *
 * `editorOpen` is a PARAMETER rather than something derived here, because this module has no access to the
 * navigation state and the app already computes the answer: the firmware opens the editor on DESCENT onto
 * a screen whose bindings name a non-text setting (`ui_actions.cpp` `settingEditedByScreen`, then
 * `beginEdit`), which is `settingOfScreen(...)` plus the `type !== "string"` test on this side. Deriving
 * it from `App`'s `editorState` instead would be wrong in exactly the case that matters — that state only
 * becomes non-null after the first UP/DOWN, so landing on an edit screen with a commissioning gap live
 * would show the banner over `hold=cancel` before the first press.
 */
export function warningBannerVisible(
  table: readonly SimulatedSensor[],
  editorOpen = false
): boolean {
  const hasWarnings = warningSensorNumbers(table).length > 0;
  const uncalibratedCount = uncalibratedSensorNumbers(table).length;
  return hasWarnings || (uncalibratedCount > 0 && !editorOpen);
}

/**
 * The banner's text, or `null` when the gate is closed.
 *
 * The string is `warningSummaryText` unchanged, which is the point: the firmware composes `warningSummary`
 * ONCE and both the banner and the `legend.warning` row print it, so a band and a row on the same panel
 * cannot describe the state two ways.
 *
 * THE GATE FILTERS, THE WORDING DOES NOT. Two consequences worth stating because both look like bugs:
 *
 * - "All sensors nominal" and "No channels in use" are unreachable THROUGH THE BANNER by construction —
 *   the gate is true only when one of the counts is non-zero, so the band can only ever carry a fault.
 *   Both strings remain reachable through `telemetry.status` and `legend.warning`, which no dataset
 *   element binds, which is why the exporter's `diagnostics-banner` check still emits its standing
 *   WARNING on every run. That is the ruling, not an oversight.
 * - While an editor is open AND a sampling fault is live, the text is still the COMBINED line naming both
 *   kinds ("1 not calibrated, 2 undersampling"). `editorOpen` suppresses the uncalibrated half of the
 *   GATE, not the uncalibrated half of the SENTENCE — the firmware composes `warningSummary` with no
 *   knowledge of the editor at all, and mirroring that faithfully matters more than the tidier reading.
 */
export function warningBannerText(
  table: readonly SimulatedSensor[],
  editorOpen = false
): string | null {
  return warningBannerVisible(table, editorOpen) ? warningSummaryText(table) : null;
}

/**
 * Where a summary's right edge lands, so the fit is measurable rather than asserted in prose.
 *
 * §2c claimed 37 columns "holds the widest summary the device can produce" against
 * `! S1,2,3,4,5,6,7,8` — a format the firmware had stopped composing. The sampling-only branch then read
 * `Sampling warning on sensors %s` with the full channel list: a 28-character prefix plus a 3k-2 list,
 * which passed 37 at FOUR flagged channels and reached 50 (right edge x=316, 76 px off a 240 px panel) at
 * eight. It overflowed SILENTLY, because `drawWarningBanner` prints straight to the panel and gets none of
 * `drawTextElement`'s `~` truncation. Every branch is a count now and the widest is the combined one at
 * 33 (right edge 214), so there are four columns of slack — but the renderer still has no clipping, so the
 * bound belongs on the composition and this function is how a test holds it.
 */
export function warningBannerRightEdge(text: string): number {
  return warningBannerLayout.textX + text.length * warningBannerLayout.glyphWidth;
}
