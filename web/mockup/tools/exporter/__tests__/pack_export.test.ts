// `npm run export:pack` (N-g): the firmware limits it reads, the rules it applies, and — the point of the
// command — the packs it refuses to write.
//
// Every real limit here is READ, through the same `readFirmwarePackLimits` the command uses, and never
// typed. A test that asserted "64 KB" would be a third home for a number that already has one in
// `ui_pack_loader.h`, and would go on passing the day the header and the exporter disagreed. What the
// tests pin instead is the BOUNDARY relative to whatever the header says: exactly at the limit passes,
// one past it fails. The one place values ARE typed is a set of fake headers, chosen unlike the real
// ones, which is how a test can tell a reader that reads from one that types.
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
import { PACK_LABEL_BYTES } from "../packEmitter.js";
import { buildIntermediateRepresentation, parseColorToArgb8888 } from "../ir.js";
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
  report: {
    status: string;
    issues?: string[];
    summary?: { label: string; fileName: string };
    validation?: { checks: Array<{ id: string; status: string }> };
  };
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

/** The theme as `buildPack` is given it by the command: from the IR, built from the dataset's own theme. */
function shippedTheme(dataset: ScreenDataset) {
  return buildIntermediateRepresentation(dataset, dataset.theme).theme;
}

/**
 * The colours in a pack's theme block, by name — read from the bytes the way §3.2 lays them out, so the
 * assertions below are about what a card would hold rather than about what the emitter was handed.
 */
function packColors(bytes: Buffer): Map<string, number> {
  const themeOffset = bytes.readUInt32LE(30);
  const stringsOffset = bytes.readUInt32LE(34);
  const text = (offset: number) => {
    const start = stringsOffset + offset;
    return bytes.subarray(start, bytes.indexOf(0, start)).toString("utf-8");
  };
  const colors = new Map<string, number>();
  const count = bytes.readUInt16LE(themeOffset);
  for (let i = 0; i < count; i += 1) {
    const at = themeOffset + 2 + i * 8;
    colors.set(text(bytes.readUInt32LE(at)), bytes.readUInt32LE(at + 4));
  }
  return colors;
}

/** The firmware's compiled-in colours, read from the committed `GeneratedUi.cpp` that CI keeps fresh. */
function compiledInColors(): Map<string, number> {
  const source = fs.readFileSync(
    path.join(repositoryRoot, "Water-Flow-Meter-PlatformIO", "src", "ui", "generated", "GeneratedUi.cpp"),
    "utf-8"
  );
  const table = /kThemeColors\[\]\s*=\s*\{([\s\S]*?)\};/.exec(source);
  assert.ok(table, "GeneratedUi.cpp still defines kThemeColors[]");
  const colors = new Map<string, number>();
  for (const [, name, hex] of table[1].matchAll(/\{\s*"([^"]+)",\s*0x([0-9A-Fa-f]{8})u\s*\}/g)) {
    colors.set(name, Number.parseInt(hex, 16));
  }
  assert.ok(colors.size > 0, "and it holds at least one colour");
  return colors;
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

/**
 * The three headers, written into a scratch repository root at the paths the reader opens.
 *
 * The test above can only say the values look like limits; this one says they are the HEADERS' values.
 * Every constant is given a value unlike the real one, so a reader that typed any of the five back in
 * would fail here — the reviewer of N-g typed all five in and the suite stayed green, because every other
 * test takes its limits from this same reader.
 */
interface FakeHeaders {
  loader?: string;
  storage?: string;
  format?: string;
}

function fakeRepository(headers: FakeHeaders): string {
  const root = scratch();
  const dir = path.join(root, "Water-Flow-Meter-PlatformIO", "src", "ui", "pack");
  fs.mkdirSync(dir, { recursive: true });
  const files: Array<[string, string | undefined]> = [
    ["ui_pack_loader.h", headers.loader],
    ["ui_pack_storage_sd.h", headers.storage],
    ["ui_pack.h", headers.format]
  ];
  for (const [name, text] of files) {
    if (text !== undefined) fs.writeFileSync(path.join(dir, name), text);
  }
  return root;
}

const kFakeLoader = [
  "class PackLoader {",
  "  static constexpr uint8_t kMaxAttempts = 2;",
  "  static constexpr std::size_t kMaxPackBytes = 3 * 1024;",
  "  static constexpr std::size_t kMaxNameBytes = 40;",
  "};"
].join("\n");
const kFakeStorage = [
  "class SdPackStorage {",
  "  static constexpr const char* kDirectory = \"/menus\";",
  "  static constexpr const char* kPointerPath = \"/menus/active\";",
  "  static constexpr const char* kPackSuffix = \".menu\";",
  "};"
].join("\n");
const kFakeFormat = "struct PackHeader {\n  static constexpr std::size_t kLabelBytes = 7;\n};";

test("the pack limits are the values in the headers, whatever those are", async () => {
  const real = await limitsPromise;
  const read = await readFirmwarePackLimits(
    fakeRepository({ loader: kFakeLoader, storage: kFakeStorage, format: kFakeFormat })
  );
  const expected = {
    maxPackBytes: 3 * 1024,
    maxNameBytes: 40,
    labelBytes: 7,
    cardDirectory: "/menus",
    packSuffix: ".menu"
  };
  for (const [key, value] of Object.entries(expected) as Array<[keyof typeof expected, number | string]>) {
    assert.equal(read[key], value, `${key} is the fake header's`);
    // Guards the test itself: a fake equal to the real value could not tell reading from typing.
    assert.notEqual(real[key], value, `${key}'s fake value differs from the real header's`);
  }
  assert.deepEqual(read.sources, [
    "Water-Flow-Meter-PlatformIO/src/ui/pack/ui_pack_loader.h",
    "Water-Flow-Meter-PlatformIO/src/ui/pack/ui_pack_storage_sd.h",
    "Water-Flow-Meter-PlatformIO/src/ui/pack/ui_pack.h"
  ]);
});

test("a missing header, or a missing constant, stops the read and names the header", async () => {
  await assert.rejects(
    readFirmwarePackLimits(fakeRepository({ loader: kFakeLoader, storage: kFakeStorage })),
    /Water-Flow-Meter-PlatformIO\/src\/ui\/pack\/ui_pack\.h: cannot be read/
  );
  await assert.rejects(
    readFirmwarePackLimits(fakeRepository({
      loader: kFakeLoader,
      storage: kFakeStorage.replace(/^.*kPackSuffix.*$/m, ""),
      format: kFakeFormat
    })),
    /Water-Flow-Meter-PlatformIO\/src\/ui\/pack\/ui_pack_storage_sd\.h: expected exactly one `static constexpr \.\.\. kPackSuffix = \.\.\.;`, found 0/
  );
  await assert.rejects(
    readFirmwarePackLimits(fakeRepository({
      loader: kFakeLoader.replace("3 * 1024", "kKilobyte * 3"),
      storage: kFakeStorage,
      format: kFakeFormat
    })),
    /ui_pack_loader\.h: kMaxPackBytes is `kKilobyte \* 3`/
  );
});

test("readHeaderConstant evaluates the two shapes the pack headers use", () => {
  const header = [
    "static constexpr std::size_t kProduct = 4 * 16;",
    "  static constexpr const char* kText = \"/card\";",
    "static constexpr uint8_t kSuffixed = 7u;",
    "static constexpr std::size_t kZero = 0;",
    "static constexpr std::size_t kTen = 10 * 1u;"
  ].join("\n");
  assert.equal(readHeaderConstant(header, "kProduct", "t.h"), 64);
  assert.equal(readHeaderConstant(header, "kText", "t.h"), "/card");
  assert.equal(readHeaderConstant(header, "kSuffixed", "t.h"), 7);
  assert.equal(readHeaderConstant(header, "kZero", "t.h"), 0, "a lone 0 is decimal and octal alike");
  assert.equal(readHeaderConstant(header, "kTen", "t.h"), 10, "a 0 inside a number is not a leading one");
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
  // C++ reads `0100` as OCTAL, sixty-four; base-10 parsing would have returned a hundred.
  assert.throws(
    () => readHeaderConstant("static constexpr std::size_t kMaxNameBytes = 0100;", "kMaxNameBytes", "octal.h"),
    /octal\.h: kMaxNameBytes is `0100`/
  );
  assert.throws(
    () => readHeaderConstant("static constexpr std::size_t kLimit = 64 * 01024;", "kLimit", "factor.h"),
    /factor\.h: kLimit is `64 \* 01024`/,
    "a leading zero in any factor, not only the first"
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

  const theme = shippedTheme(dataset);

  const good = buildPack({ dataset, theme, manifest, fileName, limits });
  assert.equal(good.status, "pass");
  assert.ok(good.pack && good.pack.bytes.length > 0);

  // Smaller than the shipped pack, so the size rule fires without a 64 KB dataset.
  const tight: FirmwarePackLimits = { ...limits, maxPackBytes: good.pack.bytes.length - 1 };
  const tooBig = buildPack({ dataset, theme, manifest, fileName, limits: tight });
  assert.equal(tooBig.status, "fail");
  assert.equal(tooBig.pack, undefined, "no bytes to write");
  assert.deepEqual(tooBig.checks.filter((c) => c.status === "fail").map((c) => c.id), ["pack-size"]);

  const { catalogueAbi: _dropped, ...withoutAbi } = manifest;
  const unstamped = buildPack({ dataset, theme, manifest: withoutAbi as FirmwareManifest, fileName, limits });
  assert.equal(unstamped.status, "fail");
  assert.equal(unstamped.pack, undefined);
  assert.ok(unstamped.checks.some((c) => c.id === "pack-catalogue-abi" && c.status === "fail"),
            "a manifest without catalogueAbi is refused rather than stamped with a guess");
});

test("buildPack stops when ui_pack.h's label field and the emitter's layout disagree", async () => {
  const limits = await limitsPromise;
  assert.equal(limits.labelBytes, PACK_LABEL_BYTES, "today they agree");
  const dataset = shippedDataset();
  // A wider field in the header would have let pack-label accept a label the emitter then cut short.
  const wider: FirmwarePackLimits = { ...limits, labelBytes: PACK_LABEL_BYTES + 2 };
  assert.throws(
    () => buildPack({
      dataset, theme: shippedTheme(dataset), manifest: shippedManifest(),
      fileName: `default${limits.packSuffix}`, label: "x".repeat(PACK_LABEL_BYTES + 1), limits: wider
    }),
    new RegExp(`kLabelBytes is ${PACK_LABEL_BYTES + 2} in ui_pack\\.h.*${PACK_LABEL_BYTES}-byte label field`)
  );
});

test("an explicit label that does not fit is a refusal, never an emitter error", async () => {
  const limits = await limitsPromise;
  const dataset = shippedDataset();
  const outcome = buildPack({
    dataset, theme: shippedTheme(dataset), manifest: shippedManifest(),
    fileName: `default${limits.packSuffix}`, label: "x".repeat(limits.labelBytes + 1), limits
  });
  assert.equal(outcome.status, "fail");
  assert.equal(outcome.pack, undefined);
  assert.deepEqual(outcome.checks.filter((c) => c.status === "fail").map((c) => c.id), ["pack-label"]);
  assert.equal(outcome.checks.some((c) => c.id === "pack-size"), false,
               "and the size is not measured with a label that is not the one asked for");
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
  // differs between machines and may hold characters the default label also refuses.
  assert.ok(failingCheckIds(run).includes("pack-file-name"));
});

test("export:pack builds a long file name the device lists, cutting only the label nobody typed", async () => {
  const limits = await limitsPromise;
  const dir = scratch();
  // Thirty characters before the suffix: well inside the file name limit, past the label field.
  const stem = "service-menu-for-site-b-hall-2";
  assert.ok(stem.length > limits.labelBytes && stem.length + limits.packSuffix.length < limits.maxNameBytes);
  const out = path.join(dir, `${stem}${limits.packSuffix}`);
  const run = runCli(["--emit-pack", out]);
  assert.equal(run.status, 0, `refused: ${JSON.stringify(run.report.issues)}`);
  assert.equal(run.report.summary?.label, stem.slice(0, limits.labelBytes),
               "the derived label is the stem cut to PackHeader::kLabelBytes");
  assert.equal(fs.readFileSync(out).subarray(42, 42 + limits.labelBytes).toString("latin1"),
               stem.slice(0, limits.labelBytes), "and that is what the header holds");
});

test("export:pack refuses an explicit --pack-label that does not fit, as a validation error", async () => {
  const limits = await limitsPromise;
  const dir = scratch();
  const out = path.join(dir, `default${limits.packSuffix}`);
  const run = runCli(["--emit-pack", out, "--pack-label", "x".repeat(limits.labelBytes + 1)]);
  // Exit 2 with a report, not exit 1 with `status: "error"`: the emitter refuses to cut a label, and
  // that refusal must never be what the author sees.
  assert.equal(run.status, 2);
  assert.equal(run.report.status, "validation-error");
  assert.deepEqual(failingCheckIds(run), ["pack-label"]);
  assert.equal(fs.existsSync(out), false);
});

test("export:pack refuses a long non-ASCII file name with no --pack-label, as a validation error", async () => {
  const limits = await limitsPromise;
  const dir = scratch();
  // Cut to the field by UTF-16 unit, this stem's label is still more bytes than the field holds.
  const out = path.join(dir, `${"ü".repeat(limits.labelBytes + 5)}${limits.packSuffix}`);
  const run = runCli(["--emit-pack", out]);
  assert.equal(run.status, 2);
  assert.equal(run.report.status, "validation-error");
  assert.deepEqual(failingCheckIds(run).sort(), ["pack-file-name", "pack-label"]);
  assert.equal(fs.existsSync(out), false);
});

// ── The theme: one menu from one set of inputs (Loadable_UI_Menu_Packs §5.2) ─────────────────────────

test("the shipped pack's colours are the compiled-in table's, rgba() ones included", async () => {
  const limits = await limitsPromise;
  const dir = scratch();
  const out = path.join(dir, `default${limits.packSuffix}`);
  assert.equal(runCli(["--emit-pack", out]).status, 0);

  const packed = packColors(fs.readFileSync(out));
  const compiledIn = compiledInColors();
  assert.deepEqual([...packed.keys()], [...compiledIn.keys()], "the same colours, in the same order");
  for (const [name, argb] of compiledIn) {
    assert.equal(packed.get(name)?.toString(16), argb.toString(16), `${name} matches GeneratedUi.cpp`);
  }
  // What this test exists for: the pack's own hex-only parser read these as opaque black.
  const source = shippedDataset().theme.colors as unknown as Record<string, string>;
  assert.ok(Object.values(source).some((c) => c.startsWith("rgba(")),
            "the shipped theme still has an rgba() colour, so the case is still covered");
});

test("export:pack honours --theme, as export:firmware does", async () => {
  const limits = await limitsPromise;
  const dir = scratch();
  const dataset = shippedDataset();
  const theme = structuredClone(dataset.theme);
  theme.colors.displayBackground = "#123456";
  const themePath = path.join(dir, "theme.json");
  fs.writeFileSync(themePath, JSON.stringify(theme));

  const out = path.join(dir, `default${limits.packSuffix}`);
  const run = runCli(["--theme", themePath, "--emit-pack", out]);
  assert.equal(run.status, 0, `refused: ${JSON.stringify(run.report.issues)}`);

  const packed = packColors(fs.readFileSync(out));
  for (const [name, color] of Object.entries(theme.colors)) {
    assert.equal(packed.get(name), parseColorToArgb8888(color).argb8888, `${name} is the --theme file's`);
  }
  assert.notEqual(packed.get("displayBackground"), compiledInColors().get("displayBackground"),
                  "and differs from the shipped theme where the file does");
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
