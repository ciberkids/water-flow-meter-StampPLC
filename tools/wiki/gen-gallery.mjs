#!/usr/bin/env node
/**
 * Generates the wiki's Screen Gallery: a picture of every screen in the default menu, grouped by level.
 *
 *   node tools/wiki/gen-gallery.mjs --out <file>    (tools/wiki/sync.sh does this)
 *
 * The pictures are `graphics/screens/<id>.png`, captured from the web designer by
 * `web/mockup/tools/screenshots/capture.mjs` — in CI, on every run — and sync.sh publishes them into the
 * wiki repository under `screens/`. This only arranges them. Levels and their order come from
 * gen-diagrams.mjs's walk, so the gallery and the Screen Navigation tree cannot disagree.
 *
 * It FAILS rather than publish a partial gallery: a screen with no picture means the captures are older
 * than the dataset, and a picture with no screen means a screen was removed and not re-captured. Either
 * way the fix is to re-run the capture, and a gallery quietly missing a screen would hide that.
 */
import fs from "node:fs";
import path from "node:path";
import process from "node:process";
import { byId, dataset, descendOf, ringTitle, rings } from "./gen-diagrams.mjs";

const repoRoot = path.join(import.meta.dirname, "..", "..");
const shotsDir = path.join(repoRoot, "graphics", "screens");
// The WIKI repository's copy, which sync.sh writes next to the pages. raw.githubusercontent.com/wiki/...
// is where GitHub serves a wiki repository's files; the page and its pictures are pushed in one commit,
// so they always describe the same menu.
const imageBase = "https://raw.githubusercontent.com/wiki/ciberkids/water-flow-meter-StampPLC/screens";

// Absent on a fresh clone (gitignored), which is the same answer as empty: capture first.
const shotFiles = fs.existsSync(shotsDir) ? fs.readdirSync(shotsDir) : [];
const shots = new Set(shotFiles.filter((f) => f.endsWith(".png")).map((f) => f.slice(0, -4)));
const missing = dataset.screens.filter((s) => !shots.has(s.id)).map((s) => s.id);
const orphans = [...shots].filter((id) => !byId.has(id));
if (missing.length || orphans.length) {
  if (missing.length) console.error(`no picture for: ${missing.join(", ")}`);
  if (orphans.length) console.error(`picture for a screen that no longer exists: ${orphans.join(", ")}`);
  console.error("capture first: cd web/mockup && npm run capture:screens");
  process.exit(1);
}

const name = (id) => byId.get(id).name.replace(/—/g, "-");
const img = (id) => `<img src="${imageBase}/${id}.png" width="320" alt="${name(id).replace(/"/g, "&quot;")}">`;
const editorOf = (id) => {
  const target = descendOf(byId.get(id));
  return target && target.endsWith("-edit") ? target : undefined;
};

const shown = new Set();
const out = [];
out.push("# Screen Gallery");
out.push("");
out.push("Every screen of the **default menu**, as the web designer draws it. The values are the designer's");
out.push("sample values, not readings from a device.");
out.push("");
out.push("> **Generated on every publish.** CI captures the pictures with `web/mockup/tools/screenshots/capture.mjs`");
out.push("> and `tools/wiki/gen-gallery.mjs` lays them out in the same levels and order as [[Screen Navigation]],");
out.push("> so a menu change appears here on its next merge to `main`. Nothing to re-run by hand.");
out.push("");
out.push("Screens are numbered by their position in their level: **DOWN** goes to the next number and **UP**");
out.push("to the previous one. A setting with a value editor shows its editor next to it; ENTER opens it.");

rings.forEach((ring) => {
  out.push("");
  out.push(`## ${ringTitle(ring, "Root ring (info pages)")}`);
  out.push("");
  const editors = ring.some((id) => editorOf(id));
  out.push(editors ? "| # | Screen | Its editor |" : "| # | Screen |");
  out.push(editors ? "| --- | --- | --- |" : "| --- | --- |");
  ring.forEach((id, position) => {
    shown.add(id);
    const cells = [`**#${position + 1}**`, `${img(id)}<br>**${name(id)}** · \`${id}\``];
    if (editors) {
      const editor = editorOf(id);
      if (editor) shown.add(editor);
      cells.push(editor ? `${img(editor)}<br>\`${editor}\`` : "");
    }
    out.push(`| ${cells.join(" | ")} |`);
  });
  if (ring.includes("info-p0-global-status")) {
    out.push("");
    out.push(`**#${ring.length + 1} · SELECT MENU** is appended to this level by the firmware and opens the Select`);
    out.push("Menu (pack selector). Both are drawn by the firmware, not from the dataset, so the designer cannot");
    out.push("show them and they have no picture here.");
  }
});

// Whatever no level holds: the acknowledgement toasts a confirm screen lands on, and the idle state.
const rest = dataset.screens.filter((s) => !shown.has(s.id));
if (rest.length) {
  out.push("");
  out.push("## Outside the levels");
  out.push("");
  out.push("Screens no UP/DOWN ring holds: the message shown after a confirmed reset, and the idle state.");
  out.push("");
  out.push("| Screen | |");
  out.push("| --- | --- |");
  for (const screen of rest) {
    out.push(`| ${img(screen.id)} | **${name(screen.id)}** · \`${screen.id}\` |`);
  }
}

const page = out.join("\n") + "\n";
const target = process.argv[process.argv.indexOf("--out") + 1];
if (process.argv.includes("--out") && target) {
  fs.writeFileSync(target, page);
} else {
  process.stdout.write(page);
}
