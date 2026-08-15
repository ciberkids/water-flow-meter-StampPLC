import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs/promises";
import os from "node:os";
import path from "node:path";
import {
  checkFirmwareBindingCoverage,
  checkManifestResolvable,
  scrapeFirmwareBindings
} from "../firmwareActions.js";
import type { FirmwareBindingScrape } from "../firmwareActions.js";
import type { FirmwareManifest } from "../../../shared/schemaDefinitions.js";

/**
 * The catalogue-entry-without-a-resolver-arm gates, tested from the failing side.
 *
 * These two checks are the only thing standing between "the mockup renders it" and "the device
 * paints a blank row", and until this file existed nothing exercised them at all: every other
 * exporter test supplies a dataset and a manifest that agree, so both gates were only ever
 * observed passing. A gate that has never been seen to fail is not known to be a gate — the
 * sibling `manifest-screen-coverage` check earned its own negative test the same way, after a
 * renamed screen shipped past a clean report.
 *
 * The scrape is asserted separately from the two checks that consume it, because they fail for
 * different reasons: an unresolvable id is a WARNING (the dataset is allowed to run ahead of the
 * resolver), while a scrape that could not find the resolver's shape at all is a FAIL. Conflating
 * them is how a broken scraper turns into a vacuous pass.
 */

/** A scrape standing in for a resolver that handles one id and knows one sensor metric. */
function makeScrape(): FirmwareBindingScrape {
  return { ok: true, exact: ["telemetry.status"], sensorMetrics: ["flow"] };
}

function makeManifest(values: string[]): FirmwareManifest {
  return {
    version: "1",
    actions: [],
    values: values.map((id) => ({ id, type: "string" as const })),
    screens: []
  };
}

test("checkManifestResolvable warns about an advertised value with no resolver arm", () => {
  // The exact shape of the documented failure: the catalogue entry exists, so the designer's
  // value picker offers it, and nothing else in the pipeline objects.
  const check = checkManifestResolvable(
    makeManifest(["telemetry.status", "telemetry.sessionStart"]),
    makeScrape()
  );
  assert.equal(check.status, "warning");
  assert.match(check.recommendation ?? "", /telemetry\.sessionStart/);
  // The resolvable one must not be blamed alongside it.
  assert.doesNotMatch(check.recommendation ?? "", /telemetry\.status/);
});

test("checkManifestResolvable passes when every advertised value has an arm", () => {
  const check = checkManifestResolvable(makeManifest(["telemetry.status"]), makeScrape());
  assert.equal(check.status, "pass");
});

test("checkFirmwareBindingCoverage warns about a bound value with no resolver arm", () => {
  const used = new Map([
    ["telemetry.status", ["badge-status"]],
    ["telemetry.sessionStart", ["p3-session-start", "p3-session-start-shadow"]]
  ]);
  const check = checkFirmwareBindingCoverage(used, makeScrape());
  assert.equal(check.status, "warning");
  assert.match(check.recommendation ?? "", /telemetry\.sessionStart/);
  // The element count is the operator-facing number: two elements go blank, not one binding.
  assert.match(check.message, /2 element\(s\)/);
});

test("a sensor binding resolves through the metric list, not the exact list", () => {
  // sensor.N.<metric> is resolved by one arm serving every channel, so the scrape lists the
  // metric and never the per-channel id. A gate that only consulted `exact` would condemn
  // every sensor row on the device.
  const scrape = makeScrape();
  assert.equal(checkManifestResolvable(makeManifest(["sensor.0.flow"]), scrape).status, "pass");
  assert.equal(
    checkManifestResolvable(makeManifest(["sensor.3.nosuchmetric"]), scrape).status,
    "warning"
  );
});

test("both gates FAIL, not warn, when the scrape could not read the resolver", () => {
  // A scraper defeated by a refactor must stop the export rather than report that all zero
  // known arms cover everything. This is the vacuous-pass case.
  const broken: FirmwareBindingScrape = {
    ok: false,
    exact: [],
    sensorMetrics: [],
    error: "the resolver's shape may have changed"
  };
  const manifestCheck = checkManifestResolvable(makeManifest(["telemetry.status"]), broken);
  assert.equal(manifestCheck.status, "fail");
  assert.match(manifestCheck.recommendation ?? "", /shape may have changed/);

  const coverageCheck = checkFirmwareBindingCoverage(new Map([["telemetry.status", ["e"]]]), broken);
  assert.equal(coverageCheck.status, "fail");
});

test("checkManifestResolvable fails when no manifest was loaded", () => {
  assert.equal(checkManifestResolvable(null, makeScrape()).status, "fail");
});

/** Lays out the two source files the scrape reads, under a throwaway project root. */
async function writeResolverTree(resolver: string, settings = ""): Promise<string> {
  const root = await fs.mkdtemp(path.join(os.tmpdir(), "fw-bindings-"));
  const dir = path.join(root, "Water-Flow-Meter-PlatformIO", "src", "ui", "core");
  await fs.mkdir(dir, { recursive: true });
  await fs.writeFile(path.join(dir, "ui_bindings.cpp"), resolver, "utf-8");
  if (settings) {
    await fs.writeFile(path.join(dir, "ui_settings_types.cpp"), settings, "utf-8");
  }
  return root;
}

test("scrapeFirmwareBindings recognises the forms the real resolver is written in", async (t) => {
  // Pinned against actual resolver syntax rather than a simplified stand-in, because the
  // regexes are the weak link: the arms use `==` or `!=` depending on how the branch reads,
  // and settings ids come from a table in a different translation unit.
  const root = await writeResolverTree(
    `
    bool UiBindingResolver::resolveTelemetryBinding(...) {
      if (binding == "telemetry.sessionStart") { return true; }
      if (binding != "telemetry.status") { return false; }
      if (metric == "flow") { return true; }
    }
    `,
    `
    const SettingDescriptor kSettings[] = {
      {"config.modbus.baud", SettingTarget::Modbus, 0},
    };
    `
  );
  t.after(() => fs.rm(root, { recursive: true, force: true }));

  const scrape = await scrapeFirmwareBindings(root);
  assert.equal(scrape.ok, true);
  assert.ok(scrape.exact.includes("telemetry.sessionStart"));
  assert.ok(scrape.exact.includes("telemetry.status"), "the != form must count as an arm");
  assert.ok(scrape.exact.includes("config.modbus.baud"), "settings ids come from the table");
  assert.deepEqual(scrape.sensorMetrics, ["flow"]);
});

test("scrapeFirmwareBindings reports a resolver it no longer understands", async (t) => {
  // No binding comparison anywhere: the honest answer is "I cannot tell", which the checks
  // above turn into a fail. Silently returning an empty list would pass everything.
  const root = await writeResolverTree("int main() { return 0; }\n");
  t.after(() => fs.rm(root, { recursive: true, force: true }));

  const scrape = await scrapeFirmwareBindings(root);
  assert.equal(scrape.ok, false);
  assert.match(scrape.error ?? "", /resolver's shape may have changed/);
});

test("scrapeFirmwareBindings reports an unreadable resolver rather than an empty one", async (t) => {
  const root = await fs.mkdtemp(path.join(os.tmpdir(), "fw-bindings-absent-"));
  t.after(() => fs.rm(root, { recursive: true, force: true }));

  const scrape = await scrapeFirmwareBindings(root);
  assert.equal(scrape.ok, false);
  assert.match(scrape.error ?? "", /Cannot read/);
});

test("the shipped resolver satisfies its own scrape", async () => {
  // An end-to-end anchor: the fixtures above prove the regexes match the C++ that was written
  // for them, which is not the same as matching the file that actually ships. This test runs
  // from dist-exporter/tools/exporter/__tests__, so the repo root is six levels up.
  const root = path.resolve(import.meta.dirname, "..", "..", "..", "..", "..", "..");
  const scrape = await scrapeFirmwareBindings(root);
  assert.equal(scrape.ok, true, "the real ui_bindings.cpp must still be scrapable");
  assert.ok(scrape.exact.length > 20, "a handful of arms would mean the regex half-matched");
  assert.ok(scrape.sensorMetrics.length > 0, "the sensor arm must still be found");
});
