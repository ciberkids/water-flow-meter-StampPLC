/**
 * Build a single HTML page showing every specified screen as SVG, at the device's own metrics.
 *
 * `npx tsx tools/audit/screen-gallery.mts <out.html>`
 *
 * Groups the screens the way an operator meets them — the info ring, then the network levels, then
 * configuration — because the review this feeds is about whether each screen READS correctly in
 * context, which a flat alphabetical list hides.
 */

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { renderScreen } from "./screen-svg";

const here = path.dirname(fileURLToPath(import.meta.url));
const specDir = path.resolve(here, "..", "..", "..", "..", "docs", "Requirements", "feature addition", "screens");

const out = process.argv[2];
if (!out) {
  console.error("usage: screen-gallery.mts <out.html>");
  process.exit(1);
}

/**
 * Every screen in the DATASET, drawn from its requirement file where it has one.
 *
 * It used to read the spec directory alone, so the nine screens with no requirement file — the
 * confirms, the toasts, the Nyquist warning, the idle state — were simply absent from a gallery whose
 * whole job is showing what the panel does. Worse, they were absent silently: the count looked
 * plausible and nothing said anything was missing.
 *
 * The spec file wins where one exists, because that is the reviewed geometry. The dataset fills in
 * otherwise, and those cards are marked so the gap is visible rather than invisible.
 */
const specFiles = fs.readdirSync(specDir).filter((f) => f.endsWith(".json"));
const specById = new Map(
  specFiles
    .map((f) => JSON.parse(fs.readFileSync(path.join(specDir, f), "utf-8")))
    .map((s) => [s.id, s])
);
const dataset = JSON.parse(
  fs.readFileSync(path.resolve(here, "..", "..", "src", "data", "screens.json"), "utf-8")
) as { screens: any[] };

const screens = dataset.screens.map((screen) => specById.get(screen.id) ?? screen);
const byId = new Map(screens.map((s: any) => [s.id, s]));
const unspecified = new Set(
  dataset.screens.filter((screen) => !specById.has(screen.id)).map((screen) => screen.id)
);

const RING = [
  "info-p0-global-status", "info-p1-instant-flow", "info-p2-cumulative-m3", "info-p3-session-m3",
  "info-p4-max-flow", "info-p5-enter-config", "info-p6-factory-reset", "net-wifi-root", "net-mqtt-root"
];
const groups: { title: string; blurb: string; ids: string[] }[] = [
  { title: "The info ring", blurb: "What UP/DOWN cycles through at the root. Nine entries.", ids: RING },
  {
    title: "Network — read only",
    blurb: "The panel reads WiFi and MQTT; it does not set them. No row offers ENTER edit, and none carries a range hint for a value it cannot change.",
    ids: screens.filter((s) => s.id.startsWith("net-") && !RING.includes(s.id)).map((s) => s.id).sort()
  },
  {
    title: "Configuration",
    blurb: "Every setting page derives its range from its own descriptor, and every editor shows New against Saved.",
    ids: screens.filter((s) => s.id.startsWith("config-")).map((s) => s.id).sort()
  },
  {
    title: "Confirmations, warnings and transient states",
    blurb: "Each confirm is now a two-entry level: the confirm itself and a < BACK row. ENTER-short on a confirm is unattached and therefore ignored — long-ENTER is the only gesture it claims, and holding it runs the countdown.",
    ids: screens
      .filter((s) => !s.id.startsWith("net-") && !s.id.startsWith("config-") && !s.id.startsWith("info-"))
      .map((s) => s.id).sort()
  }
];

const seen = new Set<string>();
const card = (id: string, worst: boolean) => {
  const s = byId.get(id);
  if (!s) return "";
  const gap = unspecified.has(id)
    ? `<span class="card__gap" title="Drawn from the dataset — this screen has no requirement file yet">no spec</span>`
    : "";
  return `<figure class="card">
  <div class="panel">${renderScreen(s, worst)}</div>
  <figcaption><b>${s.name ?? id}</b>${gap}<code>${id}</code>${s.description ? `<p>${s.description}</p>` : ""}</figcaption>
</figure>`;
};

const sections = groups
  .map((g) => {
    const ids = g.ids.filter((id) => byId.has(id) && !seen.has(id));
    ids.forEach((id) => seen.add(id));
    if (ids.length === 0) return "";
    return `<section><h2>${g.title} <span class="count">${ids.length}</span></h2>
<p class="blurb">${g.blurb}</p>
<div class="grid">${ids.map((id) => card(id, false)).join("\n")}</div></section>`;
  })
  .join("\n");

const worstIds = ["info-p0-global-status", "info-p1-instant-flow", "config-c2-baud-rate-edit", "config-s4-max-flow"];
const worstSection = `<section><h2>Worst case <span class="count">${worstIds.length}</span></h2>
<p class="blurb">The same screens drawn with every value at its declared physical limit — what the geometry audit checks. Nothing may cross the right edge.</p>
<div class="grid">${worstIds.map((id) => card(id, true)).join("\n")}</div></section>`;

const html = `<title>StampPLC panel — every screen</title>
<style>
  :root {
    --ink: #14181f; --ink-soft: #4d5566; --rule: #d9dee8; --ground: #f5f6f9;
    --card: #ffffff; --accent: #1d6fa5;
  }
  :root:not([data-theme="light"]) { }
  @media (prefers-color-scheme: dark) {
    :root:not([data-theme="light"]) {
      --ink: #e6eaf2; --ink-soft: #98a2b6; --rule: #2a3140; --ground: #10131a;
      --card: #171b24; --accent: #6fb6e8;
    }
  }
  :root[data-theme="dark"] {
    --ink: #e6eaf2; --ink-soft: #98a2b6; --rule: #2a3140; --ground: #10131a;
    --card: #171b24; --accent: #6fb6e8;
  }
  body {
    background: var(--ground); color: var(--ink); margin: 0; padding: 2.5rem 1.5rem 4rem;
    font: 15px/1.55 ui-sans-serif, system-ui, -apple-system, "Segoe UI", sans-serif;
  }
  .wrap { max-width: 1180px; margin: 0 auto; }
  h1 { font-size: 1.7rem; margin: 0 0 .3rem; letter-spacing: -.01em; text-wrap: balance; }
  .lede { color: var(--ink-soft); max-width: 62ch; margin: 0 0 2.5rem; }
  h2 { font-size: 1.05rem; margin: 2.5rem 0 .25rem; display: flex; align-items: center; gap: .6rem;
       padding-bottom: .5rem; border-bottom: 1px solid var(--rule); }
  .count { font: 600 11px/1 ui-monospace, monospace; color: var(--accent);
           border: 1px solid var(--rule); border-radius: 99px; padding: .25rem .5rem; }
  .blurb { color: var(--ink-soft); margin: .6rem 0 1.4rem; max-width: 70ch; font-size: .92rem; }
  .grid { display: grid; gap: 1.4rem; grid-template-columns: repeat(auto-fill, minmax(272px, 1fr)); }
  .card { margin: 0; background: var(--card); border: 1px solid var(--rule); border-radius: 10px;
          overflow: hidden; display: flex; flex-direction: column; }
  .panel { display: block; line-height: 0; background: #071018; }
  .panel svg { width: 100%; height: auto; display: block; image-rendering: pixelated; }
  figcaption { padding: .7rem .85rem .9rem; font-size: .84rem; display: flex; flex-direction: column; gap: .25rem; }
  figcaption b { font-size: .9rem; }
  figcaption code { font: 11px/1.4 ui-monospace, monospace; color: var(--accent); }
  .card__gap { align-self: flex-start; font: 600 9px/1 ui-sans-serif, system-ui, sans-serif;
               letter-spacing: .06em; text-transform: uppercase; padding: .2rem .35rem;
               border: 1px solid var(--rule); border-radius: 3px; color: var(--ink-soft); }
  figcaption p { margin: .15rem 0 0; color: var(--ink-soft); font-size: .8rem; }
</style>
<div class="wrap">
<h1>StampPLC panel — every screen</h1>
<p class="lede">Rendered at the device's own glyph advances — 6 px for text, 7 px for a value — so what you see is what the 240 x 135 panel draws. Geometry comes from each screen's requirement file in <code>docs/Requirements/feature addition/screens/</code>; the few marked <b>no spec</b> are drawn from the generated dataset because no requirement file exists for them yet. Values resolve through the firmware manifest descriptor and the simulated sensor table — the same sources the simulator uses, so this cannot disagree with it.</p>
${sections}
${worstSection}
</div>`;

fs.writeFileSync(out, html);
console.log(`wrote ${out}: ${seen.size} screens in ${groups.filter((g) => g.ids.length).length} groups`);
