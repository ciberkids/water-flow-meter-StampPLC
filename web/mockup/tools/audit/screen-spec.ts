/**
 * Emit a screen's spec section — ASCII plus element table — as markdown, from the proposal JSON.
 *
 * `npx tsx tools/audit/screen-spec.ts "../../docs/Requirements/feature addition/screens/info-p0-global-status.json"`
 *
 * WHY THIS EXISTS. The spec document's first draft carried hand-written ASCII and a hand-written element
 * table alongside the proposal JSON, and an audit found them stating three mutually exclusive geometries for
 * one element: the JSON said 120x40 at (60,30), the ASCII annotation said 96x32 at (72,28), and the table's
 * derived radius followed neither. Three homes for one fact — the same defect this project keeps finding in
 * its code, reproduced in its documentation. Generating both from the JSON makes drift impossible.
 *
 * Widths come from the PHYSICAL bound, not from the format string. `%7.2f` is a MINIMUM field width, so
 * quoting it as a worst case is a floor pretending to be a ceiling: a channel clamped to q_max = 65535 L/min
 * renders `65535.00`, which is eight characters, not seven.
 */

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const mockupRoot = path.resolve(here, "..", "..");

const kCols = 40;
const kRows = 17;
const kPanelW = 240;
const kPanelH = 135;
const kGlyphBase = 6;
const kGlyphValue = 7;
const kGlyphHeight = 8;
const kBadgePadX = 3;
const kBadgePadY = 2;

/**
 * Where the firmware paints its warning banner while the banner's gate is open —
 * `UiRenderer::drawWarningBanner` in `ui_renderer.cpp` (`bannerY = 116` and `bannerH = 18`, currently
 * :549-550; cited by function name because the line numbers here were already stale once, having pointed
 * at :427-428 for a function that had moved).
 *
 * RELOCATED by decision §2c, AND THE FIRMWARE HAS LANDED THE MOVE — this is no longer a proposal these
 * constants anticipate. `bannerY` was 34, which put 18 px of full-width overlay mid-panel on all 80
 * screens, the one place every screen keeps its content. It now covers the FOOTER row, the least valuable
 * row anywhere: a gesture reminder read once. A warning replacing it costs nothing while a warning is live,
 * and no screen has to reserve a band or nominate an element to sacrifice.
 *
 * Two things landed with the coordinate, and both matter to what "sits under the banner" means here.
 * `drawWarningBanner` is now called AFTER `drawScreen`, so an element in this band is COVERED rather than
 * punching a background-coloured hole through the band; and the gate widened from `hasWarnings` to
 * `bannerActive()` (`hasWarnings || (uncalibratedCount > 0 && !editorActive)`), so the band is live for a
 * commissioning gap too — which is far more of the time than a sampling fault, and is why an element
 * reported here is worth moving rather than tolerating.
 *
 * `tools/audit/screen-geometry.ts` holds these same two numbers for the 80 SHIPPED screens; this tool only
 * ever sees the 61 proposals in `screens/`. Both are needed: `bannerReplaces` below is spec-only metadata
 * the generator strips, so the shipped-dataset check cannot use it and allows the footer by element id.
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
  metadata?: { assetId?: string };
  /** The worst case string this element can render, stated per element and justified in `bound`. */
  worst?: string;
  /** Why that is the worst case — a physical limit, an enum, or a fixed literal. */
  bound?: string;
  /**
   * Set on the ONE element the banner is designed to replace — the footer hint.
   *
   * After the §2c relocation the banner covers the footer row, so the footer being hidden while a warning is
   * live is the intent rather than a collision. Every OTHER element in that band is still a defect, which is
   * what keeps this from becoming a blanket exemption.
   */
  bannerReplaces?: boolean;
}

interface Proposal {
  id: string;
  name: string;
  description?: string;
  elements: Element[];
}

const proposalPath = process.argv[2];
if (!proposalPath) {
  console.error("usage: screen-spec.ts <proposal.json>");
  process.exit(1);
}
const proposal = JSON.parse(fs.readFileSync(path.resolve(proposalPath), "utf-8")) as Proposal;

const isGeometryOnly = (kind: string) => kind === "box" || kind === "icon" || kind === "scrollbar";

function textOf(element: Element): string {
  if (isGeometryOnly(element.kind)) return "";
  if (element.worst !== undefined) return element.worst;
  return element.content ?? `?? ${element.binding ?? element.id}`;
}

function boxOf(element: Element) {
  const text = textOf(element);
  const glyph = element.kind === "value" ? kGlyphValue : kGlyphBase;
  let w: number;
  let h: number;
  if (isGeometryOnly(element.kind)) {
    w = element.width && element.width > 0 ? element.width : 40;
    h = element.height && element.height > 0 ? element.height : 12;
  } else if (element.kind === "badge") {
    w = element.width && element.width > 0 ? element.width : text.length * glyph + kBadgePadX * 2;
    h = element.height && element.height > 0 ? element.height : kGlyphHeight + kBadgePadY * 2;
  } else {
    w = text.length * glyph;
    h = kGlyphHeight;
  }
  return { text, left: element.x, top: element.y, right: element.x + w, bottom: element.y + h };
}

const boxes = proposal.elements.map((element) => ({ element, ...boxOf(element) }));

/* ── the ASCII, on the same grid the device uses ─────────────────────────────────────────────── */

const grid: string[][] = Array.from({ length: kRows }, () => Array.from({ length: kCols }, () => " "));
let clashes = 0;
const put = (r: number, c: number, ch: string) => {
  if (r < 0 || r >= kRows || c < 0 || c >= kCols) return;
  if (grid[r][c] !== " " && grid[r][c] !== ch) {
    grid[r][c] = "·";
    clashes += 1;
    return;
  }
  grid[r][c] = ch;
};

for (const box of boxes) {
  const { element } = box;
  if (isGeometryOnly(element.kind)) {
    const c0 = Math.round(box.left / kGlyphBase);
    const c1 = Math.round(box.right / kGlyphBase);
    const r0 = Math.round(box.top / kGlyphHeight);
    const r1 = Math.round(box.bottom / kGlyphHeight);
    for (let r = r0; r < r1; r += 1) {
      for (let c = c0; c < c1; c += 1) {
        if (r === r0 || r === r1 - 1 || c === c0 || c === c1 - 1) {
          put(r, c, element.kind === "scrollbar" ? "|" : "+");
        }
      }
    }
    continue;
  }
  const row = Math.round(box.top / kGlyphHeight);
  const col = Math.round(box.left / kGlyphBase);
  for (let i = 0; i < box.text.length; i += 1) put(row, col + i, box.text[i]);
  if (element.kind === "value") {
    const overhang = Math.round((box.text.length * (kGlyphValue - kGlyphBase)) / kGlyphBase);
    for (let i = 0; i < overhang; i += 1) put(row, col + box.text.length + i, ">");
  }
}

/* ── checks ──────────────────────────────────────────────────────────────────────────────────── */

const problems: string[] = [];
const accepted: string[] = [];
for (const box of boxes) {
  if (box.right > kPanelW || box.bottom > kPanelH) {
    problems.push(
      `${box.element.id} spans x ${box.left}..${box.right}, y ${box.top}..${box.bottom} — past ${kPanelW}x${kPanelH}`
    );
  }
  if (box.element.kind === "icon" && !box.element.metadata?.assetId) {
    problems.push(
      `${box.element.id} is an icon with NO assetId — ui_renderer.cpp:324 dispatches on ` +
        `strcmp(assetId, "flow-dots"), so it would draw nothing at all`
    );
  }
  if (box.top < kBannerBottom && box.bottom > kBannerTop) {
    const line =
      `${box.element.id} (y ${box.top}..${box.bottom}) sits under the warning banner's band ` +
      `y ${kBannerTop}..${kBannerBottom}, painted edge to edge while a warning is live`;
    if (box.element.bannerReplaces) {
      accepted.push(`${box.element.id} is the row the banner replaces by design (§2c)`);
    } else {
      problems.push(line);
    }
  }
}
for (let i = 0; i < boxes.length; i += 1) {
  for (let j = i + 1; j < boxes.length; j += 1) {
    const a = boxes[i];
    const b = boxes[j];
    if (a.element.kind === "box" || b.element.kind === "box") continue;
    if (a.element.kind === "scrollbar" || b.element.kind === "scrollbar") continue;
    if (a.left < b.right && b.left < a.right && a.top < b.bottom && b.top < a.bottom) {
      problems.push(`${a.element.id} overlaps ${b.element.id}`);
    }
  }
}

// Rows actually inked, not a sum of heights — the scrollbar is 104px tall and would dominate a sum.
const inkedRows = new Set<number>();
for (const b of boxes) {
  for (let y = b.top; y < b.bottom; y += 1) inkedRows.add(Math.floor(y / kGlyphHeight));
}
const minSpare = Math.min(...boxes.filter((b) => !isGeometryOnly(b.element.kind)).map((b) => kPanelW - b.right));

/* ── output ──────────────────────────────────────────────────────────────────────────────────── */

console.log(`<!-- generated by tools/audit/screen-spec.ts ${path.basename(proposalPath)} — do not hand-edit -->`);
console.log(`\nWorst case on every row, from the physical bound of each value rather than its format string:\n`);
console.log("```");
console.log(`     +${"-".repeat(kCols)}+   ${kPanelW} x ${kPanelH} px = ${kCols} cols x ${kRows} rows`);
grid.forEach((cells, r) => console.log(`y${String(r * kGlyphHeight).padStart(4)} |${cells.join("")}|`));
console.log(`     +${"-".repeat(kCols)}+`);
console.log("```");
console.log(`\n\`>\` marks a value's 7 px glyphs overhanging the 6 px grid. \`·\` marks two elements in one cell.`);
console.log(`\n| Element | Kind | x, y | Binding | Worst case | Bound |`);
console.log(`| --- | --- | --- | --- | --- | --- |`);
for (const box of boxes) {
  const e = box.element;
  const geom = isGeometryOnly(e.kind);
  const worst = geom
    ? `${box.right - box.left} × ${box.bottom - box.top} px`
    : `${box.text.length} ch = ${box.right - box.left} px, x ${box.left}..${box.right}`;
  console.log(
    `| \`${e.id}\` | ${e.kind} | ${e.x}, ${e.y} | ${e.binding ? `\`${e.binding}\`` : "—"} | ${worst} | ${e.bound ?? (geom ? "geometry" : "fixed literal")} |`
  );
}
console.log(`\nRows inked ${inkedRows.size} of ${kRows}. Narrowest right margin ${minSpare} px.`);
for (const line of accepted) console.log(`\n> Accepted overlap: ${line}`);
if (problems.length === 0 && clashes === 0) {
  console.log(
    `\n**No collisions, no overflow, every icon addressable` +
      `${accepted.length ? `, and ${accepted.length} banner overlap(s) declared below` : ", nothing in the banner band"}.**`
  );
} else {
  console.log(`\n**${problems.length + (clashes ? 1 : 0)} PROBLEM(S):**`);
  for (const problem of problems) console.log(`- ${problem}`);
  if (clashes) console.log(`- ${clashes} ASCII cell(s) claimed by two elements`);
}
