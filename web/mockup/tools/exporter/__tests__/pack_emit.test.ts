import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { emitPack, packCrc32 } from "../packEmitter.js";
import type { ScreenDataset } from "../../../src/types.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const projectRoot = path.resolve(here, "..", "..", "..", "..");

test("emits a pack for the shipped dataset", () => {
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
  assert.equal(typeof manifest.catalogueAbi, "number", "the manifest carries the firmware's ABI");
  const result = emitPack(dataset, { label: "Default", catalogueAbi: manifest.catalogueAbi });
  assert.equal(result.bytes.readUInt16LE(8), manifest.catalogueAbi,
               "and it reaches the header at offset 8, where MenuPack::validate reads it");
  assert.equal(result.bytes.subarray(0, 5).toString("latin1"), "WFMUI");
  assert.equal(result.screenCount, dataset.screens.length);
  assert.ok(result.bytes.length > 64, "has a payload");
  const payload = result.bytes.subarray(64);
  assert.equal(result.bytes.readUInt32LE(10), payload.length, "payloadBytes matches");
  assert.equal(result.bytes.readUInt32LE(14), packCrc32(payload), "crc matches");
  console.log(`  pack: ${result.bytes.length} bytes, ${result.screenCount} screens, ` +
              `${result.stringBytes} string bytes, ${result.dedupSaved} saved by dedup`);
  // Written where the C++ round-trip test reads it. Committed, and regenerated here on every
  // run, so the two implementations of the format are compared against the SAME bytes rather
  // than against each other's idea of them.
  const fixture = path.join(projectRoot, "tests", "fixtures", "default.uipack");
  fs.mkdirSync(path.dirname(fixture), { recursive: true });
  fs.writeFileSync(fixture, result.bytes);
});
