#!/usr/bin/env node
/**
 * The gate that makes I2 more than prose: the value and action catalogue is APPEND-ONLY.
 *
 * Renaming or repurposing a catalogue entry breaks every menu pack authored against it, and breaks it
 * silently — the id still resolves, to something else — on a device with a card in it, weeks later,
 * with nothing logged. That is why `Loadable_UI_Menu_Packs.md` Q4 made append-only a project rule.
 *
 * ── WHY THE MANIFEST DIFF IN CI IS NOT THIS GATE ─────────────────────────────────────────
 *
 * `test/host/run.sh` already fails if `actionManifest.json` differs from a fresh generation, and that
 * looks like the same check. It is not. It enforces FRESHNESS: rename a catalogue id, regenerate, and
 * the two agree again — the rename sails through with a green build. A rule about history needs a
 * record of history, which is `ledger.json`: it only ever grows, so a line that CHANGES is a rename or
 * a repurpose, and a line that DISAPPEARS is a removal.
 *
 * ── WHAT IS PINNED, AND WHY THE REST IS NOT ──────────────────────────────────────────────
 *
 * For a value: `category`, `type`, `unit`, `readOnly`. Those four are the contract a pack binds
 * against — whether it must carry an editor, what kind, and what the number means.
 *
 * Deliberately NOT pinned, because a gate that cries wolf gets bypassed within a month:
 *
 *  - `description`. A wording improvement must never fail CI.
 *  - `min`, `max`, `step`. The firmware owns the domain; widening a range does not invalidate a pack's
 *    editor, and this project has widened one already.
 *  - `register`. A Modbus concern, and `gen-registers.mjs` reconciles it against the headers in both
 *    directions — a second home for that check here would just be a second thing to update.
 *
 * For an action: existence only. `label` is display text and may be improved. What this CANNOT see is
 * an action keeping its id and quietly doing something else; nothing machine-readable captures that,
 * and pretending otherwise would be worse than saying so here.
 *
 * ── THE ABI BUMP ─────────────────────────────────────────────────────────────────────────
 *
 * Owner's decision, 2026-08-21: every ADDITION bumps `kUiCatalogueAbi`, so a pack's stamped ABI says
 * which vocabulary it was built against and the exporter can tell "this pack predates that setting"
 * from "this pack is missing an editor it should have". So a new id whose `catalogueAbi` is not higher
 * than every `sinceAbi` already recorded is a FAILURE with the fix named: bump the constant.
 *
 * Every id existing on 2026-08-21 is recorded at `sinceAbi: 1`, which is the ABI they were actually
 * authored under. Back-dating them across the WiFi/MQTT and N-c rounds was considered and rejected: no
 * pack has ever been stamped with anything but 1, so those numbers would be invented history.
 *
 * Usage:  node tools/catalogue/check-ledger.mjs [--write]
 */
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "..", "..");
const MANIFEST = path.join(root, "web", "mockup", "src", "data", "actionManifest.json");
const LEDGER = path.join(here, "ledger.json");

/** The value fields that define meaning. Order fixed so a written line is stable. */
const PINNED = ["category", "type", "unit", "readOnly"];

const write = process.argv.includes("--write");
const manifest = JSON.parse(fs.readFileSync(MANIFEST, "utf8"));

if (typeof manifest.catalogueAbi !== "number") {
  console.error("ERROR: actionManifest.json carries no numeric catalogueAbi.");
  console.error("       Regenerate it: Water-Flow-Meter-PlatformIO/tools/manifest_gen/run.sh");
  process.exit(1);
}
const abi = manifest.catalogueAbi;

const ledger = fs.existsSync(LEDGER)
  ? JSON.parse(fs.readFileSync(LEDGER, "utf8"))
  : { catalogueAbi: abi, values: [], actions: [] };

/** Only the pinned keys, with `null` for absent — so "no unit" and "unit removed" read alike. */
const meaningOf = (value) => {
  const out = {};
  for (const key of PINNED) out[key] = value[key] ?? null;
  return out;
};

const failures = [];
const additions = { values: [], actions: [] };

const manifestValues = new Map(manifest.values.map((v) => [v.id, v]));
const manifestActions = new Map(manifest.actions.map((a) => [a.id, a]));

// ── Existing entries: present, and unchanged in meaning ────────────────────────────────
for (const recorded of ledger.values) {
  const live = manifestValues.get(recorded.id);
  if (!live) {
    failures.push(`value REMOVED: ${recorded.id} — I2 forbids it; every pack binding this id breaks silently.`);
    continue;
  }
  const now = meaningOf(live);
  for (const key of PINNED) {
    if (JSON.stringify(now[key]) !== JSON.stringify(recorded[key] ?? null)) {
      failures.push(
        `value REPURPOSED: ${recorded.id}.${key} was ${JSON.stringify(recorded[key] ?? null)}, ` +
        `is now ${JSON.stringify(now[key])} — a pack built against the old meaning still resolves.`
      );
    }
  }
}
for (const recorded of ledger.actions) {
  if (!manifestActions.has(recorded.id)) {
    failures.push(`action REMOVED: ${recorded.id} — I2 forbids it; a pack's flow still names this id.`);
  }
}

// ── New entries: appended, and only with a bump ─────────────────────────────────────────
const knownValues = new Set(ledger.values.map((v) => v.id));
const knownActions = new Set(ledger.actions.map((a) => a.id));
const highestSince = Math.max(
  0,
  ...ledger.values.map((v) => v.sinceAbi ?? 0),
  ...ledger.actions.map((a) => a.sinceAbi ?? 0)
);

for (const value of manifest.values) {
  if (knownValues.has(value.id)) continue;
  additions.values.push({ id: value.id, sinceAbi: abi, ...meaningOf(value) });
}
for (const action of manifest.actions) {
  if (knownActions.has(action.id)) continue;
  additions.actions.push({ id: action.id, sinceAbi: abi });
}

const newCount = additions.values.length + additions.actions.length;
if (newCount > 0 && ledger.values.length + ledger.actions.length > 0 && abi <= highestSince) {
  failures.push(
    `${newCount} new catalogue ${newCount === 1 ? "entry" : "entries"} at catalogueAbi ${abi}, but ` +
    `${highestSince} is already recorded. Every addition bumps the ABI (owner's decision 2026-08-21): ` +
    `raise kUiCatalogueAbi in src/ui/core/ui_value_catalogue.h, regenerate the manifest, and re-run.`
  );
}

if (failures.length > 0) {
  console.error("The catalogue ledger refuses this change (I2 — append-only):\n");
  for (const f of failures) console.error(`  - ${f}`);
  console.error(
    "\nIf an entry genuinely must change meaning, that is a NEW id and the old one stays.\n" +
    "The ledger is tools/catalogue/ledger.json; nothing in it is ever edited or reordered by hand."
  );
  process.exit(1);
}

if (newCount === 0) {
  console.log(
    `catalogue ledger is up to date — ${ledger.values.length} values, ${ledger.actions.length} actions, ` +
    `catalogueAbi ${abi}`
  );
  process.exit(0);
}

if (!write) {
  console.error(`${newCount} catalogue ${newCount === 1 ? "entry is" : "entries are"} not in the ledger:\n`);
  for (const v of additions.values) console.error(`  + value  ${v.id}`);
  for (const a of additions.actions) console.error(`  + action ${a.id}`);
  console.error("\nAppend them: node tools/catalogue/check-ledger.mjs --write, then commit the ledger.");
  process.exit(1);
}

// APPENDED, never inserted or sorted: the point of the file is that a diff shows only new lines, so a
// reordering — however tidy — would hide exactly what this gate exists to surface.
ledger.values.push(...additions.values);
ledger.actions.push(...additions.actions);
ledger.catalogueAbi = Math.max(ledger.catalogueAbi ?? 0, abi);
fs.writeFileSync(LEDGER, `${JSON.stringify(ledger, null, 2)}\n`);
console.log(`appended ${additions.values.length} value(s) and ${additions.actions.length} action(s) at abi ${abi}`);
