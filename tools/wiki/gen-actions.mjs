#!/usr/bin/env node
/**
 * Generates the action table in `UI_Firmware_Interface.md` FROM the firmware's own catalogue.
 *
 *   node tools/wiki/gen-actions.mjs           print the table to stdout
 *   node tools/wiki/gen-actions.mjs --write   replace the marked region in the document
 *
 * WHY THIS IS GENERATED AND NOT WRITTEN.
 *
 * The hand-written table listed FOUR actions where `kActionCatalogue` declares nineteen, and two of
 * the four — `ui.action.mode.configuration` and `ui.action.mode.info` — do not exist at all: they were
 * replaced by `ui.action.nav.descend` / `.back` / `.escape` and the table was never told. That is the
 * dangerous direction of a stale document. Omitting fifteen actions makes them look unavailable;
 * advertising two that no handler implements sends an implementer to write a flow the firmware will
 * refuse, and `MEMORY.md` called this "the most dangerous live document" for exactly that reason.
 *
 * The catalogue is the only home for the ids, their picker labels and their descriptions. It is
 * already reconciled against the handler table by `static_assert` in `ui_actions.cpp`, so an action
 * cannot be advertised without a handler behind it. This script carries that same guarantee into the
 * document instead of asking a human to copy nineteen rows and remember to come back.
 *
 * The gate is a diff: CI runs `--write` and fails if anything changed, which is how the diagrams are
 * kept honest (`.github/workflows/ci.yml`, "Generated docs"). A parse that finds no actions, or a
 * document missing its markers, is an ERROR rather than an empty table — an empty section is exactly
 * what a silent failure would look like.
 */
import fs from "node:fs";
import path from "node:path";
import process from "node:process";

const repoRoot = path.join(import.meta.dirname, "..", "..");
const CATALOGUE = path.join(
  repoRoot,
  "Water-Flow-Meter-PlatformIO",
  "src",
  "ui",
  "core",
  "ui_action_catalogue.h"
);
const DOCUMENT = path.join(
  repoRoot,
  "docs",
  "Requirements",
  "feature addition",
  "UI_Firmware_Interface.md"
);

const BEGIN = "<!-- BEGIN GENERATED ACTION TABLE — node tools/wiki/gen-actions.mjs --write -->";
const END = "<!-- END GENERATED ACTION TABLE -->";

function fail(message) {
  console.error(`gen-actions: ${message}`);
  process.exit(1);
}

/**
 * The three string fields of every `{id, label, description}` entry in `kActionCatalogue`.
 *
 * Written as a scanner rather than a regex because two of the entries the table has to carry defeat
 * one: a description split across ADJACENT string literals (C++ concatenates them, a regex sees two
 * fields), and a `//` comment sitting between entries, which `reset-calibration` has above it
 * explaining why it is not called "reset sensor values".
 */
function readCatalogue(source) {
  const start = source.indexOf("kActionCatalogue[] = {");
  if (start === -1) {
    fail(`could not find kActionCatalogue in ${CATALOGUE}`);
  }
  let i = source.indexOf("{", start + "kActionCatalogue[] =".length);
  const entries = [];
  let depth = 0;
  let fields = null; // string literals of the entry being read, grouped per comma-separated field
  let literal = null; // the literal currently being consumed, or null

  while (i < source.length) {
    const c = source[i];

    if (literal !== null) {
      if (c === "\\") {
        literal += source.slice(i, i + 2);
        i += 2;
        continue;
      }
      if (c === '"') {
        // Adjacent literals belong to the SAME field, so append rather than start a new one.
        const decoded = JSON.parse(`"${literal}"`);
        fields[fields.length - 1] += decoded;
        literal = null;
        i += 1;
        continue;
      }
      literal += c;
      i += 1;
      continue;
    }

    if (c === "/" && source[i + 1] === "/") {
      i = source.indexOf("\n", i);
      if (i === -1) break;
      continue;
    }
    if (c === "/" && source[i + 1] === "*") {
      const close = source.indexOf("*/", i);
      i = close === -1 ? source.length : close + 2;
      continue;
    }
    if (c === "{") {
      depth += 1;
      if (depth === 2) fields = [""];
      i += 1;
      continue;
    }
    if (c === "}") {
      depth -= 1;
      if (depth === 1 && fields) {
        entries.push(fields);
        fields = null;
      }
      if (depth === 0) break;
      i += 1;
      continue;
    }
    if (c === "," && depth === 2) {
      fields.push("");
      i += 1;
      continue;
    }
    if (c === '"') {
      literal = "";
      i += 1;
      continue;
    }
    i += 1;
  }

  const actions = entries
    .map((fieldList) => fieldList.map((field) => field.trim()).filter(Boolean))
    .filter((fieldList) => fieldList.length >= 2)
    .map(([id, label, description]) => ({ id, label, description: description ?? "" }));

  if (actions.length === 0) {
    fail("parsed zero actions — refusing to write an empty table");
  }
  return actions;
}

function renderTable(actions) {
  const lines = [
    `*${actions.length} actions, generated from \`src/ui/core/ui_action_catalogue.h\`. Do not edit by hand:`,
    "CI regenerates this table and fails on any difference.*",
    "",
    "| Action ID | Designer label | What it does |",
    "| --- | --- | --- |"
  ];
  for (const action of actions) {
    const description = action.description.replace(/\s+/g, " ").replace(/\|/g, "\\|");
    lines.push(`| \`${action.id}\` | ${action.label} | ${description} |`);
  }
  return lines.join("\n");
}

const actions = readCatalogue(fs.readFileSync(CATALOGUE, "utf-8"));
const table = renderTable(actions);

if (!process.argv.includes("--write")) {
  process.stdout.write(`${table}\n`);
  console.error(`gen-actions: ${actions.length} actions`);
} else {
  const document = fs.readFileSync(DOCUMENT, "utf-8");
  const begin = document.indexOf(BEGIN);
  const end = document.indexOf(END);
  if (begin === -1 || end === -1 || end < begin) {
    fail(`markers missing from ${DOCUMENT} — expected ${BEGIN} ... ${END}`);
  }
  const updated =
    document.slice(0, begin + BEGIN.length) + "\n\n" + table + "\n\n" + document.slice(end);
  fs.writeFileSync(DOCUMENT, updated);
  console.error(`gen-actions: wrote ${actions.length} actions into ${path.basename(DOCUMENT)}`);
}
