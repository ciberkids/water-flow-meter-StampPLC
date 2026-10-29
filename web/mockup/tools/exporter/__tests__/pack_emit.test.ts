import test from "node:test";
import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { emitPack, packCrc32 } from "../packEmitter.js";
import type { ScreenDataset } from "../../../src/types.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const projectRoot = path.resolve(here, "..", "..", "..", "..");
const cli = path.resolve(here, "..", "cli.js");
const fixture = path.join(projectRoot, "tests", "fixtures", "default.uipack");

function shippedInputs() {
  const dataset = JSON.parse(
    fs.readFileSync(path.join(projectRoot, "src", "data", "screens.json"), "utf-8")
  ) as ScreenDataset;
  // The ABI comes from the MANIFEST, which the firmware generates from `ui::kUiCatalogueAbi` — not
  // from a literal here. It was a hard-coded `1` until 2026-08-21, which meant the number a pack was
  // stamped with and the number the firmware compares it against were two unrelated facts that
  // happened to agree; the day someone bumped the constant, this fixture would have gone on claiming
  // the old vocabulary and the C++ round-trip would have kept passing.
  const manifest = JSON.parse(
    fs.readFileSync(path.join(projectRoot, "src", "data", "actionManifest.json"), "utf-8")
  ) as { catalogueAbi: number };
  return { dataset, manifest };
}

test("emits a pack for the shipped dataset", () => {
  const { dataset, manifest } = shippedInputs();
  assert.equal(typeof manifest.catalogueAbi, "number", "the manifest carries the firmware's ABI");
  const result = emitPack(dataset, { label: "default", catalogueAbi: manifest.catalogueAbi });
  assert.equal(result.bytes.readUInt16LE(8), manifest.catalogueAbi,
               "and it reaches the header at offset 8, where MenuPack::validate reads it");
  assert.equal(result.bytes.subarray(0, 5).toString("latin1"), "WFMUI");
  assert.equal(result.screenCount, dataset.screens.length);
  assert.ok(result.bytes.length > 64, "has a payload");
  const payload = result.bytes.subarray(64);
  assert.equal(result.bytes.readUInt32LE(10), payload.length, "payloadBytes matches");
  assert.equal(result.bytes.readUInt32LE(14), packCrc32(payload), "crc matches");
  assert.deepEqual(result.unresolvedTargets, [], "every flow target in the shipped menu resolves");
  console.log(`  pack: ${result.bytes.length} bytes, ${result.screenCount} screens, ` +
              `${result.stringBytes} string bytes, ${result.dedupSaved} saved by dedup`);
});

/**
 * THE FIXTURE IS WRITTEN BY THE COMMAND, not by `emitPack` directly (N-g).
 *
 * `tests/fixtures/default.uipack` is what the firmware's host `pack_test` validates and walks field by
 * field against the compiled-in table, and what `pack_loader_test` boots. Until N-g this test wrote it
 * with `emitPack`, so the host suite proved the LIBRARY's output loads while the command an author
 * actually runs was never exercised by anything. Now the CLI writes it, with the same flags
 * `npm run export:pack` passes bar the path, so a gate that wrongly refused the shipped menu fails here,
 * and the bytes the C++ reads are the bytes a card would hold.
 *
 * It is also the ONLY writer of the fixture. `node --test` runs files in parallel processes, so a second
 * test reading or writing this file could see it half-written.
 *
 * And it is compared with the library's output in the same run, so "the command" and "emitPack" cannot
 * quietly come to mean two different packs. CI then diffs the committed fixture against this run's, so a
 * dataset change committed without its pack fails there rather than reaching the host job stale — which
 * is the DF19 shape: six scrollbar heights changed and the committed pack never followed.
 */
test("npm run export:pack's command writes the fixture the firmware's host tests read", () => {
  const { dataset, manifest } = shippedInputs();
  const run = spawnSync(process.execPath, [cli, "--emit-pack", fixture], { encoding: "utf-8" });
  assert.equal(run.status, 0, `the shipped menu is refused:\n${run.stdout}\n${run.stderr}`);

  const report = JSON.parse(run.stdout) as {
    status: string;
    summary: { pack: string; label: string; bytes: number; copyTo: string };
    compilation: { status: string };
  };
  assert.match(report.status, /^ok/);
  assert.equal(report.summary.pack, fixture);
  assert.equal(report.summary.label, "default", "the label defaults to the file name minus .uipack");
  assert.equal(report.compilation.status, "not-applicable",
               "and the compile gate is reported as not applying, rather than passed or skipped silently");

  const written = fs.readFileSync(fixture);
  assert.equal(report.summary.bytes, written.length);
  const library = emitPack(dataset, { label: "default", catalogueAbi: manifest.catalogueAbi });
  assert.ok(written.equals(library.bytes), "the command's pack is byte for byte the emitter's");
});
