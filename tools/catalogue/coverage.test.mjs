/**
 * The completeness policy, and N-b's fail/warn split.
 *
 * The warn path CANNOT fire against the live catalogue: every id is recorded at `sinceAbi: 1` and the
 * firmware is at ABI 1, so nothing predates anything. That is exactly why it is tested here with a
 * fixture instead — a gate whose interesting branch has never executed is a gate nobody should trust,
 * and the alternative was back-dating the ledger to invent a history no pack ever saw.
 */
import assert from "node:assert/strict";
import test from "node:test";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";

import { classifyCoverage, requiredPanelSettings } from "./coverage.mjs";

const here = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(here, "..", "..");

const VALUES = [
  { id: "config.modbusSlaveId", category: "setting", type: "number" },
  { id: "config.flowUnit", category: "setting", type: "enum" },
  { id: "config.newThing", category: "setting", type: "number" },
  { id: "config.mqtt.host", category: "setting", type: "string" },
  { id: "config.mqtt.port", category: "setting", type: "number" },
  { id: "net.wifi.psk", category: "setting", type: "string" },
  { id: "telemetry.instantFlow", category: "reading", type: "number" }
];
const LEDGER = {
  values: [
    { id: "config.modbusSlaveId", sinceAbi: 1 },
    { id: "config.flowUnit", sinceAbi: 1 },
    { id: "config.newThing", sinceAbi: 4 },
    { id: "config.mqtt.host", sinceAbi: 2 },
    { id: "config.mqtt.port", sinceAbi: 2 },
    { id: "net.wifi.psk", sinceAbi: 2 },
    { id: "telemetry.instantFlow", sinceAbi: 1 }
  ]
};

test("the required set is settings only, minus text and network", () => {
  const required = requiredPanelSettings(VALUES).map((v) => v.id);
  assert.deepEqual(required, ["config.modbusSlaveId", "config.flowUnit", "config.newThing"]);
  // A reading needs no editor; text has no on-device entry (§6.3); network is portal/RS485 only.
  assert.ok(!required.includes("telemetry.instantFlow"), "a reading is not a setting");
  assert.ok(!required.includes("net.wifi.psk"), "a passphrase cannot be typed on three buttons");
  assert.ok(!required.includes("config.mqtt.port"), "network is exempt even when it is a NUMBER");
});

test("a pack stamped at the current ABI must carry every editor", () => {
  const { missing, predating } = classifyCoverage({
    values: VALUES,
    ledger: LEDGER,
    covered: new Set(["config.modbusSlaveId", "config.flowUnit"]),
    packAbi: 4
  });
  assert.deepEqual(missing, ["config.newThing"], "the gap is a failure at the ABI that knew about it");
  assert.deepEqual(predating, [], "and nothing predates a pack built against the current catalogue");
});

test("an OLDER pack is warned about, not failed — the whole point of N-b", () => {
  const { missing, predating } = classifyCoverage({
    values: VALUES,
    ledger: LEDGER,
    covered: new Set(["config.modbusSlaveId", "config.flowUnit"]),
    packAbi: 1
  });
  assert.deepEqual(missing, [], "a pack from before the setting existed is not broken");
  assert.deepEqual(predating, [{ id: "config.newThing", sinceAbi: 4 }]);
});

test("the boundary is >, not >=: a pack stamped AT the ABI knew about it", () => {
  const at = classifyCoverage({ values: VALUES, ledger: LEDGER, covered: new Set(), packAbi: 4 });
  assert.ok(at.missing.includes("config.newThing"), "abi 4 knew about a sinceAbi-4 setting");
  const before = classifyCoverage({ values: VALUES, ledger: LEDGER, covered: new Set(), packAbi: 3 });
  assert.ok(!before.missing.includes("config.newThing"), "abi 3 did not");
  assert.ok(before.predating.some((p) => p.id === "config.newThing"));
});

test("a covered setting is in neither list, and full coverage is silent", () => {
  const { missing, predating } = classifyCoverage({
    values: VALUES,
    ledger: LEDGER,
    covered: new Set(["config.modbusSlaveId", "config.flowUnit", "config.newThing"]),
    packAbi: 1
  });
  assert.deepEqual(missing, []);
  assert.deepEqual(predating, []);
});

test("a setting absent from the ledger is treated as always having existed", () => {
  // Fails STRICTER, never looser: a missing ledger line cannot turn a real gap into a warning. The
  // ledger gate refuses that state anyway, so this is the safe direction for a belt-and-braces case.
  const { missing, predating } = classifyCoverage({
    values: VALUES,
    ledger: { values: [] },
    covered: new Set(),
    packAbi: 1
  });
  assert.ok(missing.includes("config.newThing"));
  assert.deepEqual(predating, []);
});

test("against the REAL catalogue, the shipped skeleton covers everything", () => {
  // The live check, so this file fails if the repository's own dataset ever stops satisfying §3.0.1 —
  // and so the fixtures above cannot drift into describing a policy the project no longer applies.
  const manifest = JSON.parse(
    fs.readFileSync(path.join(root, "web", "mockup", "src", "data", "actionManifest.json"), "utf8")
  );
  const ledger = JSON.parse(fs.readFileSync(path.join(here, "ledger.json"), "utf8"));
  const dataset = JSON.parse(
    fs.readFileSync(path.join(root, "web", "mockup", "src", "data", "screens.json"), "utf8")
  );
  const covered = new Set();
  for (const screen of dataset.screens) {
    for (const element of screen.elements ?? []) {
      if (element.binding) covered.add(element.binding);
    }
  }
  const { missing, predating } = classifyCoverage({
    values: manifest.values,
    ledger,
    covered,
    packAbi: manifest.catalogueAbi
  });
  assert.deepEqual(missing, [], "the shipped dataset satisfies the completeness rule");
  assert.deepEqual(predating, [], "and the ledger agrees with the manifest about what exists");
});
