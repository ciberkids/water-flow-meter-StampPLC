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
import { clipToPanel } from "../../src/utils/layout";

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
const truncations: string[] = [];
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

    // 5. the resolved string must still fit the panel — AFTER the clip the renderer applies, and
    //    reported when the clip actually bites, because a row that can only ever show a truncated
    //    hostname is a layout decision somebody should have made on purpose.
    const advance = element.kind === "value" ? kGlyphValue : kGlyphText;
    const clipped = clipToPanel(rendered, advance, element.x);
    if (clipped !== rendered) {
      truncations.push(
        `${screen.id}/${element.id} (${element.binding}) needs ${rendered.length} ch but the row holds ` +
          `${clipped.length}; the panel shows "${clipped}"`
      );
    }
    const right = element.x + clipped.length * advance;
    if (right > kPanelW) {
      problems.push(
        `${screen.id}/${element.id} renders ${clipped.length} ch reaching x=${right}, past the ${kPanelW} px panel`
      );
    }

    if (showAll || element.binding.startsWith("config.")) {
      rows.push([screen.id, element.id, element.binding, JSON.stringify(rendered)]);
    }
  }

  /**
   * 3. an editor's pending value must RENDER, and must be one the setting could hold.
   *
   * This used to assert New differs from Saved, which encoded a false requirement: the device sets
   * pending = saved when an editor opens (`beginEdit`), so they agree until something is pressed.
   * The gallery still draws them differently on purpose — a static render showing one number twice
   * cannot demonstrate what the screen is for — but "they differ" is not a property of the device and
   * had no business being checked as one. What DID need checking is the reported defect: `New 19200`
   * on Modbus ID, a value 78 times its ceiling.
   */
  const pending = screen.elements.find((e) => e.binding === "config.editor.pending");
  if (pending && setting && setting.type !== "string") {
    const raw = pendingRawFor(setting);
    const rendered = formatSetting(setting, raw);
    if (!rendered) {
      problems.push(`${screen.id} has a pending-value element that renders nothing`);
    }
    if (setting.options) {
      if (!setting.options.some((option) => option.value === raw)) {
        problems.push(`${screen.id} offers New ${rendered}, which is not one of ${setting.id}'s options`);
      }
    } else if (
      (setting.min !== undefined && raw < setting.min) ||
      (setting.max !== undefined && raw > setting.max)
    ) {
      problems.push(
        `${screen.id} offers New ${raw} for ${setting.id}, outside its domain ${setting.min}..${setting.max}`
      );
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
if (truncations.length > 0) {
  console.log(`\n${truncations.length} row(s) the panel must TRUNCATE (not a failure — see clipToPanel):`);
  for (const t of truncations) console.log(`- ${t}`);
}
if (problems.length === 0) {
  console.log("No unresolved placeholders, no value outside its descriptor, no dead editor, nothing past the panel.");
} else {
  console.log(`\n${problems.length} PROBLEM(S):`);
  for (const p of problems) console.log(`- ${p}`);
  process.exit(1);
}
