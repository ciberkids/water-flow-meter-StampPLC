/**
 * Render a screen as ASCII, at the device's real metrics, for review in a terminal or a document.
 *
 * `npx tsx tools/audit/screen-ascii.ts info-p0-global-status`
 *
 * The grid is 40 columns x 16 rows because that is what the panel IS: 240 / 6 px per glyph, 135 / 8 px
 * per row (ui_renderer.cpp:16-17, :292). A value element draws at 7 px, so its text is placed at its true
 * pixel position and marked `>` where it extends past the 6 px grid — that discrepancy is exactly how the
 * two-column sensor pages came to collide, so it is shown rather than smoothed away.
 *
 * Strings come from the same resolvers the simulator uses, so this is what renders, not what was authored.
 */

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { createSensorTable, resolveSensorBinding } from "../../src/utils/sensorConfig";
import { sampleValueFor } from "../../src/utils/sampleValues";
import type { FirmwareValueDefinition } from "../../src/types/firmwareActions";

const here = path.dirname(fileURLToPath(import.meta.url));
const mockupRoot = path.resolve(here, "..", "..");

const kCols = 40;
// 135 / 8 = 16.875, so the last row EXISTS and is clipped by 3px. At 16 the footer at y=124 fell off the
// grid entirely and the render silently omitted it — the renderer has to show the row the device shows.
const kRows = 17;
const kGlyphBase = 6;
const kGlyphValue = 7;

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

const dataset = JSON.parse(
  fs.readFileSync(path.join(mockupRoot, "src", "data", "screens.json"), "utf-8")
) as { screens: Array<{ id: string; name: string; description?: string; elements: Element[] }> };
const manifest = JSON.parse(
  fs.readFileSync(path.join(mockupRoot, "src", "data", "actionManifest.json"), "utf-8")
) as { values: FirmwareValueDefinition[] };
const valueById = new Map(manifest.values.map((value) => [value.id, value]));
const table = createSensorTable();

const screenId = process.argv[2];
/**
 * `--proposal <file>` renders a PROPOSED layout instead of the authored one, through this same code.
 *
 * So a proposal in Display_Per_Screen_Spec.md is measured by the tool that measures the device, not drawn
 * by hand and hoped to be right. The file holds `{ id, name, description?, elements: [...] }`.
 */
const proposalFlag = process.argv.indexOf("--proposal");
const proposalPath = proposalFlag > -1 ? process.argv[proposalFlag + 1] : undefined;

const screen = proposalPath
  ? (JSON.parse(fs.readFileSync(path.resolve(proposalPath), "utf-8")) as (typeof dataset)["screens"][number])
  : dataset.screens.find((candidate) => candidate.id === screenId);
if (!screen) {
  console.error(`No such screen: ${screenId}`);
  console.error(`Try one of: ${dataset.screens.slice(0, 6).map((s) => s.id).join(", ")} …`);
  process.exit(1);
}

/**
 * Formats PROPOSED here but not yet in the firmware catalogue.
 *
 * Worst cases, so the render shows the widest thing the row can ever draw:
 *   telemetry.totalFlowLps      %7.2f                      (proposed: was %.2f, unbounded)
 *   telemetry.totalVolumeLiters "Since reset: %7.2f L"
 *   telemetry.maxFlowLpm        "Max Flow: %7.2f L/m (S%u)" (proposed: new value, L/m per the unit decision)
 *   legend.status               "WiFi %s  MQTT %s  LED 1p/%uL", wifiStateText capped at 5 chars
 */
const kProposedFormats: Record<string, string> = {
  "telemetry.totalFlowLps": "9999.99",
  "telemetry.totalVolumeLiters": "Since reset: 9999.99 L",
  "telemetry.maxFlowLpm": "Max Flow: 9999.99 L/m (S8)",
  "legend.status": "WiFi AuthF  MQTT OK  LED 1p/100L"
};

function renderedText(element: Element): string {
  if (!element.binding) {
    return element.content ?? "";
  }
  if (kProposedFormats[element.binding]) {
    return kProposedFormats[element.binding];
  }
  return (
    resolveSensorBinding(element.binding, table, 8) ??
    sampleValueFor(element.binding, valueById.get(element.binding), "sample")
  );
}

const grid: string[][] = Array.from({ length: kRows }, () => Array.from({ length: kCols }, () => " "));
const collisionsAt = new Set<string>();

function put(row: number, col: number, char: string) {
  if (row < 0 || row >= kRows || col < 0 || col >= kCols) {
    return;
  }
  if (grid[row][col] !== " " && grid[row][col] !== char) {
    collisionsAt.add(`${row},${col}`);
    grid[row][col] = "·"; // a middle dot marks two elements claiming one cell
    return;
  }
  grid[row][col] = char;
}

const notes: string[] = [];

for (const element of screen.elements ?? []) {
  const row = Math.round(element.y / 8);
  const text = renderedText(element);
  const glyph = element.kind === "value" ? kGlyphValue : kGlyphBase;

  if (element.kind === "box" || element.kind === "icon" || element.kind === "scrollbar") {
    const width = element.width && element.width > 0 ? element.width : 40;
    const height = element.height && element.height > 0 ? element.height : 12;
    const c0 = Math.round(element.x / kGlyphBase);
    const c1 = Math.round((element.x + width) / kGlyphBase);
    const r0 = Math.round(element.y / 8);
    const r1 = Math.round((element.y + height) / 8);
    for (let r = r0; r < r1; r += 1) {
      for (let c = c0; c < c1; c += 1) {
        const edge = r === r0 || r === r1 - 1 || c === c0 || c === c1 - 1;
        if (edge) put(r, c, element.kind === "scrollbar" ? "|" : "+");
      }
    }
    notes.push(
      `${element.id} (${element.kind}) box x ${element.x}..${element.x + width}, y ${element.y}..${element.y + height}`
    );
    continue;
  }

  const startCol = Math.round(element.x / kGlyphBase);
  const pxRight = element.x + text.length * glyph;
  for (let i = 0; i < text.length; i += 1) {
    put(row, startCol + i, text[i] === " " ? " " : text[i]);
  }
  const overhang = element.kind === "value" ? text.length * (kGlyphValue - kGlyphBase) : 0;
  if (overhang > 0) {
    for (let i = 0; i < Math.round(overhang / kGlyphBase); i += 1) {
      put(row, startCol + text.length + i, ">");
    }
  }
  notes.push(
    `${element.id} (${element.kind}${element.binding ? ` ← ${element.binding}` : ""})` +
      ` x ${element.x}..${pxRight}${pxRight > 240 ? "  ** PAST 240 **" : ""}` +
      `, y ${element.y}  "${text}"`
  );
}

console.log(`${screen.id} — ${screen.name}${proposalPath ? "   [PROPOSAL]" : "   [as authored today]"}`);
if (screen.description) console.log(`${screen.description}`);
console.log(`\n     +${"-".repeat(kCols)}+   240 x 135 px = ${kCols} cols x ${kRows} rows (last row clipped 3px)`);
grid.forEach((cells, row) => {
  console.log(`y${String(row * 8).padStart(4)} |${cells.join("")}|`);
});
console.log(`     +${"-".repeat(kCols)}+`);
console.log(`\n\`>\` = a value's 7px glyphs overhanging the 6px grid; \`·\` = two elements in one cell`);
if (collisionsAt.size > 0) {
  console.log(`OVERLAPPING CELLS: ${collisionsAt.size}`);
}
console.log("\nElements:");
for (const note of notes) console.log(`  ${note}`);
