/**
 * Screen geometry audit — does each authored screen physically fit the device's panel?
 *
 * Grounding, not opinion: every number here is the FIRMWARE's, not the mockup's.
 *
 *   glyph width   6 px for text/badge/scrollbar, 7 px for a value  (ui_renderer.cpp:16-17)
 *   glyph height  8 px                                            (ui_renderer.cpp:292)
 *   badge box     text width + 3 px padding each side, 8 + 2*2 tall (ui_renderer.cpp:285-293)
 *   panel         240 x 135                                        (utils/layout.ts)
 *
 * Strings come from the same resolvers the simulator uses, so what is measured is what renders.
 *
 * Why this exists: P1's `s1-value` sits at x=14 and its status badge at x=60, while the value's own
 * format — `%u: %6.2f %s` (ui_bindings.cpp:306) — is 13 characters, i.e. 91 px. The value runs to x=105,
 * straight through the badge AND the right-hand column. The mockup could not show it (it sizes badges to
 * content) and the layout diagnostics could not either: they test out-of-bounds, never collisions. This
 * audit tests collisions, which is the check that was missing.
 */

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { createSensorTable, resolveSensorBinding } from "../../src/utils/sensorConfig";
import { sampleValueFor } from "../../src/utils/sampleValues";
import type { FirmwareValueDefinition } from "../../src/types/firmwareActions";

const here = path.dirname(fileURLToPath(import.meta.url));
const mockupRoot = path.resolve(here, "..", "..");

const kPanelWidth = 240;
const kPanelHeight = 135;
const kGlyphWidthBase = 6;
const kGlyphWidthValue = 7;
const kGlyphHeight = 8;
const kBadgePadX = 3;
const kBadgePadY = 2;

/**
 * §2c's warning-banner band — the same two numbers `tools/audit/screen-spec.ts` holds as
 * `kBannerTop` / `kBannerBottom`, read off `UiRenderer::drawWarningBanner` (`bannerY = 116`,
 * `bannerH = 18`, so the band is y 116…133 and 134 is the first row below it).
 *
 * WHY IT IS HERE TOO, AND WHY THAT IS NOT DUPLICATION FOR ITS OWN SAKE. `screen-spec.ts` runs per-file
 * over the 61 proposals in `docs/Requirements/feature addition/screens/` and reports all 61 clean. This
 * tool is the only one that sees the 80 SHIPPED screens, including the 19 that have no spec file at all
 * (`nyquist-warning`, `state-idle`, the six `confirm-*`, the six `confirm-*-back`, the five `toast-*`) —
 * and it had no banner check whatsoever. So the band that §2c's whole decision rests on was verified on
 * the proposals and on nothing that ships, which is how a text four pixels inside it sat in the dataset
 * while the audit printed "0 findings".
 *
 * The banner paints AFTER the screen (`ui_renderer.cpp`: `drawScreen` then `drawWarningBanner`), so an
 * element in this band is covered rather than punching through it. Being covered is the whole point for
 * the footer hint and a defect for anything else.
 */
const kBannerTop = 116;
const kBannerBottom = 134;

interface Element {
  id: string;
  kind: string;
  x: number;
  y: number;
  width?: number;
  height?: number;
  content?: string;
  binding?: string;
  emphasis?: string;
}

interface Screen {
  id: string;
  name: string;
  description?: string;
  elements: Element[];
}

const dataset = JSON.parse(
  fs.readFileSync(path.join(mockupRoot, "src", "data", "screens.json"), "utf-8")
) as { screens: Screen[] };
const manifest = JSON.parse(
  fs.readFileSync(path.join(mockupRoot, "src", "data", "actionManifest.json"), "utf-8")
) as { values: FirmwareValueDefinition[] };

const valueById = new Map(manifest.values.map((value) => [value.id, value]));
const table = createSensorTable();

/** The string the device would draw. Sensor 8 is used so the widest index is measured. */
function renderedText(element: Element): string {
  if (!element.binding) {
    return element.content ?? "";
  }
  const fromMemory = resolveSensorBinding(element.binding, table, 8);
  if (fromMemory !== undefined) {
    return fromMemory;
  }
  return sampleValueFor(element.binding, valueById.get(element.binding), "sample");
}

interface Box {
  element: Element;
  text: string;
  left: number;
  top: number;
  right: number;
  bottom: number;
}

function boxFor(element: Element): Box {
  const text = renderedText(element);
  const glyph = element.kind === "value" ? kGlyphWidthValue : kGlyphWidthBase;

  let width: number;
  let height: number;
  if (element.kind === "box" || element.kind === "icon" || element.kind === "scrollbar") {
    // Geometry-only elements: the dataset's own width/height, with drawBoxElement's 40x12 fallback
    // (ui_renderer.cpp:314-315).
    width = element.width && element.width > 0 ? element.width : 40;
    height = element.height && element.height > 0 ? element.height : 12;
  } else if (element.kind === "badge") {
    width = element.width && element.width > 0 ? element.width : text.length * glyph + kBadgePadX * 2;
    height = element.height && element.height > 0 ? element.height : kGlyphHeight + kBadgePadY * 2;
  } else {
    width = text.length * glyph;
    height = kGlyphHeight;
  }

  return {
    element,
    text,
    left: element.x,
    top: element.y,
    right: element.x + width,
    bottom: element.y + height
  };
}

const overlaps = (a: Box, b: Box) =>
  a.left < b.right && b.left < a.right && a.top < b.bottom && b.top < a.bottom;

/**
 * Can the banner ever be on the panel at the same time as this screen?
 *
 * `state-idle` is a non-issue BY CONSTRUCTION rather than by exemption: `UiRenderer::update` takes the
 * `UiMode::Idle` branch and RETURNS after `fillScreen` — backlight off, nothing drawn — long before
 * `drawWarningBanner` is reached, so no element on the idle screen can be covered by anything. Its
 * `idle-placeholder` text sits at y=124, squarely in the band, and reporting it would be a false
 * positive on the one screen the firmware provably never paints an element onto at all.
 *
 * Nothing else earns a screen-level skip. In particular the Select Menu short-circuit above the Idle
 * branch (`drawPackSelector` then return) suppresses the banner too, but that page is firmware-drawn and
 * has no dataset screen, so it never reaches this loop.
 */
const bannerBandApplies = (screen: Screen) => screen.id !== "state-idle";

interface Report {
  screen: string;
  name: string;
  overflows: string[];
  collisions: string[];
  unbound: string[];
  bannerBand: string[];
  /** §2c: a `level-position` scrollbar must stop clear of the banner band (DF19). */
  scrollbarBand: string[];
}

const reports: Report[] = [];

for (const screen of dataset.screens) {
  const boxes = (screen.elements ?? []).map(boxFor);
  const overflows: string[] = [];
  const collisions: string[] = [];
  const unbound: string[] = [];
  const bannerBand: string[] = [];
  const scrollbarBand: string[] = [];

  for (const box of boxes) {
    if (box.right > kPanelWidth || box.bottom > kPanelHeight) {
      overflows.push(
        `${box.element.id} (${box.element.kind}) "${box.text}" spans x ${box.left}..${box.right}` +
          `, y ${box.top}..${box.bottom} — past ${kPanelWidth}x${kPanelHeight} by ` +
          `${Math.max(0, box.right - kPanelWidth)}x${Math.max(0, box.bottom - kPanelHeight)}px`
      );
    }
    if (box.element.binding && !valueById.has(box.element.binding)) {
      unbound.push(`${box.element.id} binds unknown value "${box.element.binding}"`);
    }
    if (bannerBandApplies(screen) && box.top < kBannerBottom && box.bottom > kBannerTop) {
      /**
       * THE FOOTER HINT IS THE ROW §2c SPENT, and it is matched BY ID rather than by the spec's
       * `bannerReplaces` flag because the flag cannot get here: `tools/skeleton/generate.mjs` lists it in
       * `SPEC_ONLY_KEYS`, so it appears zero times in `src/data/screens.json`. Two allowances for one rule,
       * because one of them cannot cross the generator — and this one is the weaker contract: a future
       * screen that puts something other than a gesture reminder at y=124 and names it `footer-hint` is
       * excused by an id, not by a declaration.
       *
       * A FULL-PANEL BOX IS A BACKDROP and carries no glyphs, so 18 px of it under the band costs nothing.
       * The condition is deliberately the geometry (`x===0 && y===0 && 240x135`) and NOT `kind === "box"`:
       * `kind === "box"` would also excuse the next decorative divider drawn across the band, which is the
       * defect §5b.2 already records once. Note the reason is NOT "the firmware repaints over the banner" —
       * that is only true for the countdown overlay pass on the six `confirm-*` screens during a hold. Now
       * that the banner paints after `drawScreen`, the banner shows THROUGH on the other eleven full-panel
       * screens (the five `toast-*` and the six `confirm-*` before their hold begins).
       *
       * A SCROLLBAR IS A RIGHT-EDGE FIXTURE, exempt from the GLYPH argument for the same reason the
       * collision loop below treats it as decorative: 5 px wide, position carried by a thumb across its
       * whole length, and no glyph to hide. It is NOT exempt from §2c's band, which is a separate rule and
       * now has its own report below — a scrollbar that reaches the band is still painted over, and its
       * bottom is exactly what §2c legislates.
       *
       * That distinction was drawn the hard way (DF19). This exemption used to swallow the band overlap on
       * the six `confirm-*-back` screens, whose `level-position` shipped y=14 height=104 — bottom 118, two
       * pixels inside the band — while the other 55 came out at 100 from their spec files. The generator's
       * own `L.scrollbar.height` was the only thing holding the six there: one fact with two homes, and the
       * wrong home was the live one. Fixed by that single number plus `--write`, which turned out to change
       * exactly six lines of the dataset rather than the wholesale regeneration the deferral feared.
       */
      const isFooterHint = box.element.id === "footer-hint";
      const isFullPanelBackdrop =
        box.element.kind === "box" &&
        box.element.x === 0 &&
        box.element.y === 0 &&
        (box.element.width ?? 0) >= kPanelWidth &&
        (box.element.height ?? 0) >= kPanelHeight;
      const isScrollbar = box.element.kind === "scrollbar";
      if (isScrollbar) {
        // §2c's actual rule for this element: stop clear of the band. Reported separately so it cannot be
        // confused with a glyph being hidden, and so the six screens DF19 fixed cannot regress unseen.
        scrollbarBand.push(
          `${box.element.id} spans y ${box.top}..${box.bottom}, reaching the banner band at y ${kBannerTop}` +
            ` — §2c requires it to stop clear, at height 100 from y 14. Fix the generator's` +
            ` L.scrollbar.height or this screen's spec file, then re-run --write`
        );
      }
      if (!isFooterHint && !isFullPanelBackdrop && !isScrollbar) {
        bannerBand.push(
          `${box.element.id} (${box.element.kind}) "${box.text}" sits at y ${box.top}..${box.bottom}` +
            `, inside the warning banner's band y ${kBannerTop}..${kBannerBottom} — the banner paints` +
            ` edge to edge over it while a warning is live`
        );
      }
    }
  }

  for (let i = 0; i < boxes.length; i += 1) {
    for (let j = i + 1; j < boxes.length; j += 1) {
      const a = boxes[i];
      const b = boxes[j];
      // A scrollbar is a deliberate right-edge fixture and a box is a deliberate backdrop, so neither is
      // reported as colliding with text drawn over it.
      //
      // An ICON IS NOT DECORATIVE. `drawFlowDots` paints eight animated dots across its whole 55x55 box
      // (ui_renderer.cpp:drawFlowDots), so text crossing it is as broken as two labels overlapping —
      // excluding it here is what hid P0's own defect from the first run of this audit.
      const decorative = (kind: string) => kind === "box" || kind === "scrollbar";
      if (decorative(a.element.kind) || decorative(b.element.kind)) {
        continue;
      }
      if (overlaps(a, b)) {
        const width = Math.min(a.right, b.right) - Math.max(a.left, b.left);
        collisions.push(
          `${a.element.id} "${a.text}" (x ${a.left}..${a.right}) overlaps ` +
            `${b.element.id} "${b.text}" (x ${b.left}..${b.right}) by ${width}px`
        );
      }
    }
  }

  if (overflows.length || collisions.length || unbound.length || bannerBand.length || scrollbarBand.length) {
    reports.push({
      screen: screen.id,
      name: screen.name,
      overflows,
      collisions,
      unbound,
      bannerBand,
      scrollbarBand
    });
  }
}

const totalCollisions = reports.reduce((sum, r) => sum + r.collisions.length, 0);
const totalOverflows = reports.reduce((sum, r) => sum + r.overflows.length, 0);
const totalBannerBand = reports.reduce((sum, r) => sum + r.bannerBand.length, 0);
const totalScrollbarBand = reports.reduce((sum, r) => sum + r.scrollbarBand.length, 0);

console.log(`Screens audited: ${dataset.screens.length}`);
console.log(`Screens with findings: ${reports.length}`);
console.log(`Text collisions: ${totalCollisions}   Panel overflows: ${totalOverflows}`);
// ZERO is the correct state of this line since 2026-08-18, and it got there by being FIXED, not by the
// check being weakened (DF18). It reported one element — `nyquist-warning` / `option-down` at y 112..120,
// four pixels inside the band — because that screen had no spec file and the generator's footer re-stack
// pinned its geometry. `screens/nyquist-warning.json` now exists: the lower half moved up one 12 px row so
// the last content row ends at 112, and the footer declares `bannerReplaces`, which makes this the screen
// that RESERVES the band rather than competing with it — apt, since it is the one screen where a live
// warning and its own prompt are guaranteed on the panel together.
//
// If this ever reads non-zero again, the finding is real. Do not silence it by deleting the check, and do
// not "fix" it by deleting a screen: authoring or correcting the spec file is the route.
console.log(`Banner-band elements: ${totalBannerBand}`);
// ZERO is the correct state of this line, unlike the one above it. DF19 brought the six `confirm-*-back`
// scrollbars from 104 to 100, so every `level-position` now stops clear of the band — and this check is what
// says so the next time the generator's layout table or a spec file moves. It reported six before the fix.
console.log(`Scrollbars reaching the band: ${totalScrollbarBand}\n`);

for (const report of reports) {
  console.log(`## ${report.screen} — ${report.name}`);
  for (const line of report.collisions) console.log(`   COLLISION  ${line}`);
  for (const line of report.overflows) console.log(`   OVERFLOW   ${line}`);
  for (const line of report.unbound) console.log(`   UNBOUND    ${line}`);
  for (const line of report.bannerBand) console.log(`   BANNER     ${line}`);
  for (const line of report.scrollbarBand) console.log(`   SCROLLBAR  ${line}`);
  console.log("");
}
