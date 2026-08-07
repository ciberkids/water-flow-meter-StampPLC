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

interface Report {
  screen: string;
  name: string;
  overflows: string[];
  collisions: string[];
  unbound: string[];
}

const reports: Report[] = [];

for (const screen of dataset.screens) {
  const boxes = (screen.elements ?? []).map(boxFor);
  const overflows: string[] = [];
  const collisions: string[] = [];
  const unbound: string[] = [];

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

  if (overflows.length || collisions.length || unbound.length) {
    reports.push({ screen: screen.id, name: screen.name, overflows, collisions, unbound });
  }
}

const totalCollisions = reports.reduce((sum, r) => sum + r.collisions.length, 0);
const totalOverflows = reports.reduce((sum, r) => sum + r.overflows.length, 0);

console.log(`Screens audited: ${dataset.screens.length}`);
console.log(`Screens with findings: ${reports.length}`);
console.log(`Text collisions: ${totalCollisions}   Panel overflows: ${totalOverflows}\n`);

for (const report of reports) {
  console.log(`## ${report.screen} — ${report.name}`);
  for (const line of report.collisions) console.log(`   COLLISION  ${line}`);
  for (const line of report.overflows) console.log(`   OVERFLOW   ${line}`);
  for (const line of report.unbound) console.log(`   UNBOUND    ${line}`);
  console.log("");
}
