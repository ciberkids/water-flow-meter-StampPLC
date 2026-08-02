// Decision D4: the exporter must export the dataset it is GIVEN, not whatever is on disk.
//
// Before this, the browser POSTed no body and the endpoint let the CLI re-read
// src/data/screens.json — so Export exported the last saved dataset rather than the edited
// one, and the nine validation gates all validated that stale copy. The bug was accidentally
// protective for a while: datasetClamp defaulted to portrait bounds and mutated 49 of 375
// elements on every ingest, and the export was correct only because it ignored the clamped
// in-memory copy. That clamp is fixed, so this can be too.
//
// Covers the CLI contract the endpoint depends on: --screens must be honoured. The endpoint
// itself is a vite plugin and not reachable from node:test, so what is checked here is the
// property the endpoint relies on being true.
import test from "node:test";
import assert from "node:assert/strict";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { execFileSync } from "node:child_process";

const here = path.dirname(fileURLToPath(import.meta.url));
// NOTE ON PATHS: these tests run COMPILED, from dist-exporter/tools/exporter/__tests__, so
// the project root is four levels up — the same convention manifest_validation.test.ts uses.
// Three levels resolves to dist-exporter/, which contains a build-artifact COPY of
// src/data/screens.json. Reading that copy works by accident and is wrong in principle: a
// test that claims to check the shipped dataset must open the shipped dataset.
const projectRoot = path.resolve(here, "..", "..", "..", "..");
const cli = path.join(projectRoot, "dist-exporter", "tools", "exporter", "cli.js");

function loadShipped(): { screens: Array<{ id: string }>; theme: unknown } {
  return JSON.parse(
    fs.readFileSync(path.join(projectRoot, "src", "data", "screens.json"), "utf-8")
  ) as { screens: Array<{ id: string }>; theme: unknown };
}

test("--screens is honoured, so a posted dataset reaches the exporter", () => {
  const shipped = loadShipped();

  // A recognisably different dataset: the shipped one with its last screen removed. Using a
  // MODIFIED copy of the real dataset rather than a synthetic fixture means the run still
  // passes the schema, so a difference in the report can only come from the input being read.
  const trimmed = { ...shipped, screens: shipped.screens.slice(0, -1) };
  const tmp = path.join(os.tmpdir(), `d4-posted-${process.pid}.json`);
  fs.writeFileSync(tmp, JSON.stringify(trimmed), "utf-8");

  try {
    // --dry-run so nothing is written to the firmware tree. That matters: an earlier Cypress
    // spec checked a similar property by letting the exporter overwrite the committed assets.
    const out = execFileSync(
      "node",
      [cli, "--screens", tmp, "--dry-run", "--allow-missing-toolchain"],
      { cwd: projectRoot, encoding: "utf-8" }
    );
    const report = JSON.parse(out.slice(out.indexOf("{"))) as {
      summary?: { screens?: number };
    };

    assert.equal(
      report.summary?.screens,
      trimmed.screens.length,
      "the exporter reported the screen count of the dataset it was GIVEN"
    );
    assert.notEqual(
      report.summary?.screens,
      shipped.screens.length,
      "and not the count of the dataset on disk — otherwise --screens was ignored"
    );
  } finally {
    fs.rmSync(tmp, { force: true });
  }
});
