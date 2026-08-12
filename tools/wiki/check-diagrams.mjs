#!/usr/bin/env node
/**
 * Parses every `docs/diagrams/*.mermaid` with mermaid itself, and fails on any that will not render.
 *
 *   npm ci --prefix tools/wiki && node tools/wiki/check-diagrams.mjs
 *
 * WHY THIS EXISTS.
 *
 * Four diagrams were committed — two of them GENERATED, in the same change that added a CI gate for
 * diagram freshness — and not one of them rendered. Every one carried a bare `%%` line as a separator
 * inside its comment header, and mermaid only treats `%%` as a comment when something follows it: a
 * bare one is passed to the parser, glues onto the next line as `%%flowchart TB`, and takes the whole
 * diagram down. GitHub showed an error where the picture should be.
 *
 * The freshness gate could not have caught it. It checks that the committed file matches what the
 * generator produces, which was true — the generator was producing something invalid. "Current" and
 * "correct" are different properties and each needs its own check, which is the general lesson: a gate
 * that passes on broken output is the failure mode this repository keeps finding.
 *
 * `mermaid.parse` is the same validation a renderer runs before drawing, so a file that passes here is
 * one GitHub will draw. Verified by rendering all four to SVG with a real browser once, by hand; that
 * is too heavy for CI, and parse is where the failures actually are.
 */
import fs from "node:fs";
import path from "node:path";
import process from "node:process";
import { JSDOM } from "jsdom";

const repoRoot = path.join(import.meta.dirname, "..", "..");
const diagramDir = path.join(repoRoot, "docs", "diagrams");

// mermaid needs a DOM even to parse. jsdom is enough — no browser, no download.
const dom = new JSDOM("<!doctype html><body></body>", { pretendToBeVisual: true });
global.window = dom.window;
global.document = dom.window.document;
// `navigator` is getter-only on newer Node globals, so it has to be defined rather than assigned.
Object.defineProperty(global, "navigator", { value: dom.window.navigator, configurable: true });

const mermaid = (await import("mermaid")).default;
mermaid.initialize({ startOnLoad: false, securityLevel: "loose" });

const files = fs
  .readdirSync(diagramDir)
  .filter((name) => name.endsWith(".mermaid"))
  .sort();

if (files.length === 0) {
  console.error("no diagrams found — check-diagrams.mjs is looking at the wrong directory");
  process.exit(1);
}

let failed = 0;
for (const name of files) {
  const source = fs.readFileSync(path.join(diagramDir, name), "utf-8");

  /**
   * The specific trap, checked BEFORE the parser sees it.
   *
   * mermaid's own error for this points at line 1 and says "Expecting 'GRAPH', got 'NODE_STRING'",
   * which sends the reader to the `flowchart` declaration — the one line that is not the problem. So
   * this names it instead, because the parser cannot.
   */
  const bare = source
    .split("\n")
    .map((line, index) => ({ line, number: index + 1 }))
    .filter(({ line }) => /^%%\r?$/.test(line));
  if (bare.length > 0) {
    failed++;
    console.error(
      `FAIL  ${name}\n      bare "%%" on line(s) ${bare.map((b) => b.number).join(", ")} — ` +
        `mermaid only treats %% as a comment when something follows it, so this glues onto the next\n` +
        `      line and breaks the diagram. Use a BLANK line as the separator — "%% " works too, but\n` +
        `      its trailing space is load-bearing and any editor that trims whitespace reintroduces this.`
    );
    continue;
  }

  try {
    await mermaid.parse(source);
    console.log(`OK    ${name}`);
  } catch (error) {
    failed++;
    const message = String(error?.message ?? error)
      .split("\n")
      .slice(0, 6)
      .join("\n      ");
    console.error(`FAIL  ${name}\n      ${message}`);
  }
}

if (failed > 0) {
  console.error(`\n${failed} of ${files.length} diagrams will not render.`);
  process.exit(1);
}
console.log(`\n${files.length} diagrams parse.`);
