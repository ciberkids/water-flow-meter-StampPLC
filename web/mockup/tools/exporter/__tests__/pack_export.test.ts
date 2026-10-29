// `npm run export:pack` (N-g): the firmware limits it reads, the rules it applies, and — the point of the
// command — the packs it refuses to write.
//
// Every limit here is READ, through the same `readFirmwarePackLimits` the command uses, and never typed.
// A test that asserted "64 KB" would be a third home for a number that already has one in
// `ui_pack_loader.h`, and would go on passing the day the header and the exporter disagreed. What the
// tests pin instead is the BOUNDARY relative to whatever the header says: exactly at the limit passes,
// one past it fails.
//
// Every refusal through the CLI also asserts that nothing was written — and, where a file was already
// there, that it was left alone. A command that refuses and then writes anyway is the failure that
// matters, and it is invisible in the exit code.
import test from "node:test";
import assert from "node:assert/strict";
import { spawnSync } from "node:child_process";
import fs from "node:fs";
import os from "node:os";
import path from "node:path";
import { fileURLToPath } from "node:url";
import {
  checkPackFileName,
  checkPackLabel,
  checkPackSize,
  readFirmwarePackLimits,
  readHeaderConstant,
  type FirmwarePackLimits
} from "../packRules.js";
import { buildPack } from "../packExport.js";
import type { FirmwareManifest } from "../types.js";
import type { ScreenDataset } from "../../../src/types.js";

const here = path.dirname(fileURLToPath(import.meta.url));
const workspaceRoot = path.resolve(here, "..", "..", "..", "..");
const repositoryRoot = path.resolve(workspaceRoot, "..", "..");
const cli = path.resolve(here, "..", "cli.js");

function shippedDataset(): ScreenDataset {
  return JSON.parse(
    fs.readFileSync(path.join(workspaceRoot, "src", "data", "screens.json"), "utf-8")
  ) as ScreenDataset;
}

function shippedManifest(): FirmwareManifest {
  return JSON.parse(
    fs.readFileSync(path.join(workspaceRoot, "src", "data", "actionManifest.json"), "utf-8")
  ) as FirmwareManifest;
}

const limitsPromise = readFirmwarePackLimits(repositoryRoot);

function scratch(): string {
  return fs.mkdtempSync(path.join(os.tmpdir(), "export-pack-"));
}

interface CliRun {
  status: number | null;
  report: { status: string; issues?: string[]; validation?: { checks: Array<{ id: string; status: string }> } };
}

function runCli(args: string[]): CliRun {
  const run = spawnSync(process.execPath, [cli, ...args], { encoding: "utf-8" });
  // Success reports on stdout and every refusal on stderr, as export:firmware does.
  const text = run.status === 0 ? run.stdout : run.stderr;
  let report: CliRun["report"];
  try {
    report = JSON.parse(text);
  } catch {
    throw new Error(`the CLI did not print a JSON report (exit ${run.status}):\n${run.stdout}\n${run.stderr}`);
  }
  return { status: run.status, report };
}

function failingCheckIds(run: CliRun): string[] {
  return (run.report.validation?.checks ?? []).filter((c) => c.status === "fail").map((c) => c.id);
}

// ── Reading the headers ─────────────────────────────────────────────────────────────────────────────

test("the firmware's pack limits are read from its headers", async () => {
  const limits = await limitsPromise;
  for (const key of ["maxPackBytes", "maxNameBytes", "labelBytes"] as const) {
    assert.ok(Number.isSafeInteger(limits[key]) && limits[key] > 0, `${key} is a positive whole number`);
  }
  assert.ok(limits.cardDirectory.startsWith("/"), "the card directory is an absolute card path");
  assert.ok(limits.packSuffix.startsWith("."), "the suffix is a file extension");
  assert.ok(limits.maxNameBytes - 1 > limits.packSuffix.length, "a name can be longer than its suffix");
  assert.deepEqual(limits.sources.map((s) => path.basename(s)),
                   ["ui_pack_loader.h", "ui_pack_storage_sd.h", "ui_pack.h"]);
});

test("readHeaderConstant evaluates the two shapes the pack headers use", () => {
  const header = [
    "static constexpr std::size_t kProduct = 4 * 16;",
    "  static constexpr const char* kText = \"/card\";",
    "static constexpr uint8_t kSuffixed = 7u;"
  ].join("\n");
  assert.equal(readHeaderConstant(header, "kProduct", "t.h"), 64);
  assert.equal(readHeaderConstant(header, "kText", "t.h"), "/card");
  assert.equal(readHeaderConstant(header, "kSuffixed", "t.h"), 7);
});

test("readHeaderConstant ignores a commented-out definition", () => {
  const header = [
    "// static constexpr std::size_t kLimit = 1;",
    "/* static constexpr std::size_t kLimit = 2; */",
    "static constexpr std::size_t kLimit = 3;"
  ].join("\n");
  assert.equal(readHeaderConstant(header, "kLimit", "t.h"), 3);
});

test("readHeaderConstant throws, naming the header, rather than guessing", () => {
  assert.throws(() => readHeaderConstant("int x = 1;", "kLimit", "moved.h"), /moved\.h.*found 0/);
  assert.throws(
    () => readHeaderConstant(
      "static constexpr int kLimit = 1;\nstatic constexpr int kLimit = 2;", "kLimit", "twice.h"),
    /twice\.h.*found 2/
  );
  assert.throws(
    () => readHeaderConstant("static constexpr int kLimit = kOther * 2;", "kLimit", "derived.h"),
    /derived\.h: kLimit is `kOther \* 2`/
  );
});

// ── The rules ───────────────────────────────────────────────────────────────────────────────────────

test("the file name rule accepts what the device lists and loads, at its exact length limit", async () => {
  const limits = await limitsPromise;
  assert.equal(checkPackFileName(`default${limits.packSuffix}`, limits).status, "pass");

  const longest = "a".repeat(limits.maxNameBytes - 1 - limits.packSuffix.length) + limits.packSuffix;
  assert.equal(Buffer.byteLength(longest), limits.maxNameBytes - 1);
  assert.equal(checkPackFileName(longest, limits).status, "pass", "the longest name the loader holds");
  assert.equal(checkPackFileName("a" + longest, limits).status, "fail", "and one byte more is refused");
});

test("the file name rule refuses what the device would not list or would not load", async () => {
  const limits = await limitsPromise;
  const refused: Array<[string, string]> = [
    ["menu.bin", "not the suffix: never listed"],
    [`MENU${limits.packSuffix.toUpperCase()}`, "the suffix in capitals: listPacks compares case-sensitively"],
    [limits.packSuffix, "a suffix with no name"],
    [`my menu${limits.packSuffix}`, "a space: listed, then refused at boot by nameIsSafe"],
    [`tab\t${limits.packSuffix}`, "a control character"],
    [`a:b${limits.packSuffix}`, "a character FAT forbids"],
    [`menü${limits.packSuffix}`, "non-ASCII, which depends on an unchecked FatFs code page"],
    [`.hidden${limits.packSuffix}`, "a leading dot"]
  ];
  for (const [name, why] of refused) {
    assert.equal(checkPackFileName(name, limits).status, "fail", `${JSON.stringify(name)} — ${why}`);
  }
});

test("the label must fit the header field in printable ASCII", async () => {
  const limits = await limitsPromise;
  assert.equal(checkPackLabel("default", limits).status, "pass");
  assert.equal(checkPackLabel("x".repeat(limits.labelBytes), limits).status, "pass", "exactly full");
  assert.equal(checkPackLabel("x".repeat(limits.labelBytes + 1), limits).status, "fail", "one over");
  assert.equal(checkPackLabel("Menü", limits).status, "fail", "a character the field would split");
  assert.equal(checkPackLabel("", limits).status, "fail", "empty");
});

test("the size rule matches the loader's boundary: at the limit loads, one byte over does not", async () => {
  const limits = await limitsPromise;
  assert.equal(checkPackSize(limits.maxPackBytes, limits).status, "pass");
  assert.equal(checkPackSize(limits.maxPackBytes + 1, limits).status, "fail");
});

// ── buildPack, with injected limits ─────────────────────────────────────────────────────────────────

test("buildPack produces the shipped pack and withholds the bytes whenever a check fails", async () => {
  const limits = await limitsPromise;
  const dataset = shippedDataset();
  const manifest = shippedManifest();
  const fileName = `default${limits.packSuffix}`;

  const good = buildPack({ dataset, manifest, fileName, limits });
  assert.equal(good.status, "pass");
  assert.ok(good.pack && good.pack.bytes.length > 0);

  // Smaller than the shipped pack, so the size rule fires without a 64 KB dataset.
  const tight: FirmwarePackLimits = { ...limits, maxPackBytes: good.pack.bytes.length - 1 };
  const tooBig = buildPack({ dataset, manifest, fileName, limits: tight });
  assert.equal(tooBig.status, "fail");
  assert.equal(tooBig.pack, undefined, "no bytes to write");
  assert.deepEqual(tooBig.checks.filter((c) => c.status === "fail").map((c) => c.id), ["pack-size"]);

  const { catalogueAbi: _dropped, ...withoutAbi } = manifest;
  const unstamped = buildPack({ dataset, manifest: withoutAbi as FirmwareManifest, fileName, limits });
  assert.equal(unstamped.status, "fail");
  assert.equal(unstamped.pack, undefined);
  assert.ok(unstamped.checks.some((c) => c.id === "pack-catalogue-abi" && c.status === "fail"),
            "a manifest without catalogueAbi is refused rather than stamped with a guess");
});

// ── The command ─────────────────────────────────────────────────────────────────────────────────────

test("export:pack refuses a file name the device would not list, and writes nothing", async () => {
  const dir = scratch();
  const out = path.join(dir, "menu.bin");
  const run = runCli(["--emit-pack", out]);
  assert.equal(run.status, 2);
  assert.equal(run.report.status, "validation-error");
  assert.deepEqual(failingCheckIds(run), ["pack-file-name"]);
  assert.equal(fs.existsSync(out), false);
});

test("export:pack with an empty path is refused, not mistaken for export:firmware", () => {
  const run = runCli(["--emit-pack", "", "--dry-run"]);
  assert.equal(run.status, 2);
  // `includes`, not an exact list: the empty path resolves to the checkout's own directory, whose name
  // differs between machines and can also be too long to serve as the default label.
  assert.ok(failingCheckIds(run).includes("pack-file-name"));
});

test("export:pack refuses a dataset export:firmware refuses, and leaves an existing pack alone", async () => {
  const limits = await limitsPromise;
  const dir = scratch();
  const dataset = shippedDataset();
  const element = dataset.screens[0].elements.find((e) => e.binding);
  assert.ok(element, "the shipped root screen binds a value");
  element.binding = "not.a.firmware.value";
  const screens = path.join(dir, "screens.json");
  fs.writeFileSync(screens, JSON.stringify(dataset));

  const out = path.join(dir, `default${limits.packSuffix}`);
  fs.writeFileSync(out, "the pack that was already on the card");
  const run = runCli(["--screens", screens, "--emit-pack", out]);
  assert.equal(run.status, 2);
  assert.ok(failingCheckIds(run).includes("manifest-value-coverage"), "the same gate export:firmware runs");
  assert.equal(fs.readFileSync(out, "utf-8"), "the pack that was already on the card");
});

test("export:pack refuses a pack over the device's size limit, read from the header", async () => {
  const limits = await limitsPromise;
  const dir = scratch();
  const dataset = shippedDataset();
  const text = dataset.screens.flatMap((s) => s.elements).find((e) => e.kind === "text" && e.content);
  assert.ok(text);
  text.content = "x".repeat(limits.maxPackBytes);
  const screens = path.join(dir, "screens.json");
  fs.writeFileSync(screens, JSON.stringify(dataset));

  const out = path.join(dir, `big${limits.packSuffix}`);
  const run = runCli(["--screens", screens, "--emit-pack", out]);
  assert.equal(run.status, 2);
  assert.deepEqual(failingCheckIds(run), ["pack-size"],
                   "every export:firmware gate passes; only the device's limit refuses it");
  assert.equal(fs.existsSync(out), false);
});

test("export:pack refuses an ENTER target that names no screen, which no export:firmware gate sees", async () => {
  const limits = await limitsPromise;
  const dir = scratch();
  const dataset = shippedDataset();
  const flow = dataset.screens
    .flatMap((s) => s.flows ?? [])
    .find((f) => f.trigger.type === "button" && f.trigger.button === "enter" && f.targetScreenId);
  assert.ok(flow, "the shipped menu descends somewhere with ENTER");
  flow.targetScreenId = "no-such-screen";
  const screens = path.join(dir, "screens.json");
  fs.writeFileSync(screens, JSON.stringify(dataset));

  const out = path.join(dir, `typo${limits.packSuffix}`);
  const run = runCli(["--screens", screens, "--emit-pack", out]);
  assert.equal(run.status, 2);
  assert.deepEqual(failingCheckIds(run), ["pack-flow-targets"]);
  assert.equal(fs.existsSync(out), false);
});

test("export:pack --dry-run runs every check and writes nothing", async () => {
  const limits = await limitsPromise;
  const dir = scratch();
  const out = path.join(dir, `default${limits.packSuffix}`);
  const run = runCli(["--emit-pack", out, "--dry-run"]);
  assert.equal(run.status, 0);
  assert.match(run.report.status, /^ok/);
  assert.equal(fs.existsSync(out), false);
});
