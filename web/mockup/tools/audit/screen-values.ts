/**
 * Audit what every screen's bound elements actually RESOLVE TO, not just where they sit.
 *
 * `npx tsx tools/audit/screen-values.ts [--all]`
 *
 * WHY THIS EXISTS. `screen-geometry.ts` proves the layout holds and `screen-spec.ts` proves the
 * document matches the dataset, but neither looks at the strings. A review of the rendered mockups
 * found that eleven setting pages showed `1 to 247` — the Modbus ID range — because the range hint
 * was one static placeholder standing in for a per-screen fact; that every value editor offered
 * `New 19200`, including Modbus ID whose domain stops at 247; and that every numeric setting page
 * drew `42`, the sample table's bare type fallback, so the Baud Rate page showed a number that is
 * not a baud rate beside a range hint that said so.
 *
 * All three were invisible to a passing test suite because nothing checked the text. These are the
 * checks that would have caught them:
 *
 *   1. Nothing renders an unresolved `{{binding}}` placeholder.
 *   2. A setting page's value is one its descriptor permits — an option's label, or a number inside
 *      min..max.
 *   3. An editor's pending value differs from its saved one, or the screen shows nothing.
 *   4. A range hint belongs to the setting on ITS OWN screen.
 *   5. Every resolved string still fits the 240 px panel at the device's glyph advances.
 */

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import type { FirmwareValueDefinition } from "../../src/types/firmwareActions";
import { sampleValueFor } from "../../src/utils/sampleValues";
import { formatSetting, pendingRawFor, rangeHintFor, settingOfScreen } from "../../src/utils/settingHints";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "..", "..");

const kPanelW = 240;
const kGlyphText = 6;
const kGlyphValue = 7;

interface Element {
  id: string;
  kind: string;
  x: number;
  y: number;
  content?: string;
  binding?: string;
}
interface Screen {
  id: string;
  name: string;
  elements: Element[];
}

const dataset = JSON.parse(fs.readFileSync(path.join(root, "src", "data", "screens.json"), "utf-8")) as {
  screens: Screen[];
};
const manifest = JSON.parse(fs.readFileSync(path.join(root, "src", "data", "actionManifest.json"), "utf-8")) as {
  values: FirmwareValueDefinition[];
};
const byId = new Map(manifest.values.map((v) => [v.id, v]));

/** Everything a device-memory or aggregate binding resolves to is checked in the app, not here. */
const kMemoryOwned = (binding: string) =>
  binding.startsWith("sensor.") || binding.startsWith("telemetry.") || binding.startsWith("net.") ||
  binding === "legend.status";

const problems: string[] = [];
const rows: string[][] = [];
const showAll = process.argv.includes("--all");

for (const screen of dataset.screens) {
  const setting = settingOfScreen(screen, byId);

  for (const element of screen.elements) {
    if (!element.binding) continue;
    const definition = byId.get(element.binding);

    let rendered: string;
    if (element.binding === "config.editor.range") {
      rendered = setting ? rangeHintFor(setting) : "";
    } else if (element.binding === "config.editor.pending") {
      rendered = setting && setting.type !== "string" ? formatSetting(setting, pendingRawFor(setting)) : "";
    } else {
      rendered = sampleValueFor(element.binding, definition, "sample");
    }

    // 1. no unresolved placeholder
    if (rendered.includes("{{")) {
      problems.push(`${screen.id}/${element.id} renders the unresolved placeholder ${rendered}`);
    }

    // 2. a setting's value must be one its own descriptor permits
    if (definition?.category === "setting" && definition.type !== "string" && element.binding !== "config.editor.pending") {
      const legal = definition.options
        ? definition.options.map((o) => (definition.unit ? `${o.label} ${definition.unit}` : o.label))
        : null;
      if (legal && !legal.includes(rendered)) {
        problems.push(
          `${screen.id}/${element.id} renders "${rendered}" for ${element.binding}, which is not one of its options (${legal.join(", ")})`
        );
      }
      if (!legal) {
        const numeric = Number.parseInt(rendered, 10);
        const { min, max } = definition;
        if (Number.isFinite(numeric) && min !== undefined && max !== undefined && (numeric < min || numeric > max)) {
          problems.push(
            `${screen.id}/${element.id} renders ${numeric} for ${element.binding}, outside its domain ${min}..${max}`
          );
        }
      }
    }

    // 5. the resolved string must still fit the panel
    const advance = element.kind === "value" ? kGlyphValue : kGlyphText;
    const right = element.x + rendered.length * advance;
    if (right > kPanelW) {
      problems.push(
        `${screen.id}/${element.id} renders ${rendered.length} ch reaching x=${right}, past the ${kPanelW} px panel`
      );
    }

    if (showAll || element.binding.startsWith("config.")) {
      rows.push([screen.id, element.id, element.binding, JSON.stringify(rendered)]);
    }
  }

  // 3. an editor must show New against Saved, or it hides the only thing it is for
  const pending = screen.elements.find((e) => e.binding === "config.editor.pending");
  if (pending && setting && setting.type !== "string") {
    const newValue = formatSetting(setting, pendingRawFor(setting));
    const savedValue = sampleValueFor(setting.id, setting, "sample");
    if (newValue === savedValue) {
      problems.push(`${screen.id} shows the same value as New and Saved (${newValue}) — the editor demonstrates nothing`);
    }
  }

  // 4. a range hint with no setting on its own screen is an invitation the page cannot honour
  const hint = screen.elements.find((e) => e.binding === "config.editor.range");
  if (hint && !setting) {
    problems.push(`${screen.id} carries a range hint but shows no setting`);
  }
}

if (rows.length > 0) {
  const w = [0, 1, 2].map((i) => Math.max(...rows.map((r) => r[i].length)));
  console.log(`${"SCREEN".padEnd(w[0])}  ${"ELEMENT".padEnd(w[1])}  ${"BINDING".padEnd(w[2])}  RENDERS`);
  for (const r of rows) console.log(`${r[0].padEnd(w[0])}  ${r[1].padEnd(w[1])}  ${r[2].padEnd(w[2])}  ${r[3]}`);
}

console.log(`\n${dataset.screens.length} screens, ${rows.length} config binding(s) shown.`);
if (problems.length === 0) {
  console.log("No unresolved placeholders, no value outside its descriptor, no dead editor, nothing past the panel.");
} else {
  console.log(`\n${problems.length} PROBLEM(S):`);
  for (const p of problems) console.log(`- ${p}`);
  process.exit(1);
}
