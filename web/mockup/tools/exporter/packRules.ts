// What the firmware will accept from a menu pack on its SD card, read out of the firmware's own headers.
//
// N-g. `npm run export:pack` has to refuse a pack the device would not load, and "would not load" is
// decided in C++: `PackLoader::kMaxPackBytes` (anything larger is refused before it is read),
// `PackLoader::kMaxNameBytes` (the pointer file's buffer, which bounds the file name), `SdPackStorage`'s
// directory and suffix (what the Select Menu lists), and `PackHeader::kLabelBytes` (the label field). If
// those numbers were typed here they would be a second home for each, and the day someone raised the
// pack limit to 96 KB this file would go on refusing 70 KB packs the device could hold — or, worse, the
// day someone LOWERED it, it would go on building packs the device rejects at boot. So they are read from
// the headers on every run, the same way `tools/wiki/sync.sh` reads them for the SD Card page, and a
// header whose shape has changed stops the export rather than letting it fall back to a guess.
//
// What this file deliberately does NOT do is reimplement `MenuPack::validate` (§3.3's hard checks).
// §5.5 asks the exporter to run them against its own output, but a second validator in TypeScript would
// be a second implementation of the format, which is the shape of bug this project keeps finding. The
// format is reconciled by EXECUTION instead: the fixture the host `pack_test` validates and walks field
// by field is written by this same command (`__tests__/pack_emit.test.ts`), and CI fails when the
// committed copy differs from a fresh build.
import fs from "node:fs/promises";
import path from "node:path";
import type { ValidationCheck } from "./types.js";

/** The firmware's own limits, each read from the header that defines it. */
export interface FirmwarePackLimits {
  /** `ui::PackLoader::kMaxPackBytes`. The loader refuses a pack LARGER than this, so equal is fine. */
  maxPackBytes: number;
  /** `ui::PackLoader::kMaxNameBytes`, which counts the terminator: the longest name is one less. */
  maxNameBytes: number;
  /** `ui::PackHeader::kLabelBytes`, the header's fixed label field. */
  labelBytes: number;
  /** `plc::SdPackStorage::kDirectory`, where the pack goes on the card. */
  cardDirectory: string;
  /** `plc::SdPackStorage::kPackSuffix`, the only ending the Select Menu lists. */
  packSuffix: string;
  /** Where each value came from, repository-relative, for the export report. */
  sources: string[];
}

const kPackSourceDir = path.join("Water-Flow-Meter-PlatformIO", "src", "ui", "pack");
const kLoaderHeader = path.join(kPackSourceDir, "ui_pack_loader.h");
const kStorageHeader = path.join(kPackSourceDir, "ui_pack_storage_sd.h");
const kFormatHeader = path.join(kPackSourceDir, "ui_pack.h");

/**
 * Blanks comments, so a commented-out old definition cannot be mistaken for the live one.
 *
 * Crude — a `//` inside a string literal would be cut — but none of the constants read here holds one,
 * and a cut string fails the parse below loudly rather than yielding a plausible wrong value.
 */
function stripComments(source: string): string {
  return source.replace(/\/\*[\s\S]*?\*\//g, " ").replace(/\/\/[^\n]*/g, " ");
}

/**
 * The value of `static constexpr <type> <name> = <value>;` in a header.
 *
 * Two shapes are understood, because those are the two the pack headers use: a string literal
 * (`"/ui"`) and a product of integer literals (`64 * 1024`). Anything else — a constant defined in terms
 * of another, a cast, a ternary — THROWS, naming the header and the text it found. Evaluating a C++
 * expression generally is not this file's job, and returning something when it could not would be the
 * plausible wrong value this whole module exists to prevent. The definition must also appear exactly
 * once, so a second declaration cannot leave the reader guessing which one the compiler took.
 */
export function readHeaderConstant(source: string, name: string, where: string): number | string {
  const pattern = new RegExp(
    String.raw`static\s+constexpr\s+[A-Za-z_][\w:\s\*]*?\b${name}\s*=\s*([^;]+);`,
    "g"
  );
  const matches = [...stripComments(source).matchAll(pattern)];
  if (matches.length !== 1) {
    throw new Error(
      `${where}: expected exactly one \`static constexpr ... ${name} = ...;\`, found ${matches.length}. ` +
        "If the constant moved or was renamed, update tools/exporter/packRules.ts with it."
    );
  }
  const text = matches[0][1].trim();

  const literal = /^"([^"\\]*)"$/.exec(text);
  if (literal) {
    return literal[1];
  }

  const factors = text.split("*").map((factor) => factor.trim());
  if (factors.every((factor) => /^\d+(?:[uU]?[lL]{0,2}|[lL]{0,2}[uU]?)$/.test(factor))) {
    return factors.reduce((product, factor) => product * Number.parseInt(factor, 10), 1);
  }

  throw new Error(
    `${where}: ${name} is \`${text}\`, which is neither a string literal nor a product of integer ` +
      "literals. Update tools/exporter/packRules.ts to read it rather than typing the value here."
  );
}

function asNumber(value: number | string, name: string, where: string): number {
  if (typeof value !== "number" || !Number.isSafeInteger(value) || value <= 0) {
    throw new Error(`${where}: ${name} should be a positive whole number, read ${JSON.stringify(value)}.`);
  }
  return value;
}

function asString(value: number | string, name: string, where: string): string {
  if (typeof value !== "string" || value.length === 0) {
    throw new Error(`${where}: ${name} should be a non-empty string literal, read ${JSON.stringify(value)}.`);
  }
  return value;
}

/** Reads every limit `export:pack` enforces. Throws when any header cannot be read or parsed. */
export async function readFirmwarePackLimits(projectRoot: string): Promise<FirmwarePackLimits> {
  const read = async (relative: string) => ({
    where: relative,
    source: await fs.readFile(path.join(projectRoot, relative), "utf-8")
  });
  const [loader, storage, format] = await Promise.all([
    read(kLoaderHeader),
    read(kStorageHeader),
    read(kFormatHeader)
  ]);
  const number = (header: { where: string; source: string }, name: string) =>
    asNumber(readHeaderConstant(header.source, name, header.where), name, header.where);
  const string = (header: { where: string; source: string }, name: string) =>
    asString(readHeaderConstant(header.source, name, header.where), name, header.where);

  return {
    maxPackBytes: number(loader, "kMaxPackBytes"),
    maxNameBytes: number(loader, "kMaxNameBytes"),
    labelBytes: number(format, "kLabelBytes"),
    cardDirectory: string(storage, "kDirectory"),
    packSuffix: string(storage, "kPackSuffix"),
    sources: [kLoaderHeader, kStorageHeader, kFormatHeader].map((p) => p.split(path.sep).join("/"))
  };
}

/**
 * Characters a pack file name may contain: ASCII letters, digits, `.`, `-` and `_`.
 *
 * STRICTER THAN THE DEVICE, on purpose. `PackLoader::nameIsSafe` refuses a separator, a space, any
 * control byte and DEL, and accepts everything else — so `my menu.uipack` is listed by the Select Menu
 * and then refused at boot as a bad pointer, which is precisely a pack that "would not load". Of what the
 * loader does accept, FAT itself forbids `" * : < > ? |` in a name, and anything beyond ASCII depends on
 * the FatFs code page the core was built with, which nothing in this project has checked. The remaining
 * ASCII punctuation would probably work; "probably" is not what an export gate is for, so the rule is
 * the set that certainly does, and widening it is the owner's call rather than this file's.
 */
const kPortableNameCharacter = /^[A-Za-z0-9._-]$/;

/** The stem of a pack file name: the name without the firmware's suffix. */
export function packStem(fileName: string, limits: FirmwarePackLimits): string {
  return fileName.endsWith(limits.packSuffix) ? fileName.slice(0, -limits.packSuffix.length) : fileName;
}

/**
 * The file name rule: what `SdPackStorage::listPacks` will list AND `PackLoader` will load.
 *
 * The two can disagree, which is why both are checked: `listPacks` only needs the suffix and a length
 * under `kMaxNameBytes`, while the loader also refuses the characters `nameIsSafe` names. A name only one
 * of them accepts is a pack the operator can pick and the device then refuses.
 */
export function checkPackFileName(fileName: string, limits: FirmwarePackLimits): ValidationCheck {
  const id = "pack-file-name";
  const title = "The file name is one the device lists and loads";
  const problems: string[] = [];
  const longest = limits.maxNameBytes - 1;

  if (!fileName.endsWith(limits.packSuffix)) {
    problems.push(
      `it does not end in "${limits.packSuffix}", so the Select Menu will not list it ` +
        "(the comparison is case-sensitive)"
    );
  } else if (packStem(fileName, limits).length === 0) {
    problems.push(`"${limits.packSuffix}" alone has no name in front of it`);
  }
  const bytes = Buffer.byteLength(fileName, "utf-8");
  if (bytes > longest) {
    problems.push(
      `it is ${bytes} bytes and the device holds at most ${longest} ` +
        `(PackLoader::kMaxNameBytes is ${limits.maxNameBytes}, terminator included)`
    );
  }
  const refused = [...new Set([...fileName].filter((c) => !kPortableNameCharacter.test(c)))];
  if (refused.length > 0) {
    problems.push(
      `it contains ${refused.map((c) => JSON.stringify(c)).join(", ")}; use only letters, digits, ` +
        "\".\", \"-\" and \"_\" (spaces and separators are refused by the device at boot)"
    );
  }
  if (fileName.startsWith(".")) {
    problems.push("it starts with \".\", which many card readers treat as a hidden file");
  }

  if (problems.length > 0) {
    return {
      id,
      title,
      status: "fail",
      message: `"${fileName}" would not work on the card: ${problems.join("; ")}.`,
      recommendation: `Name it like default${limits.packSuffix} — at most ${longest} characters.`
    };
  }
  return {
    id,
    title,
    status: "pass",
    message:
      `"${fileName}" ends in ${limits.packSuffix}, is ${bytes} of at most ${longest} bytes, ` +
      "and uses only characters the device and FAT both accept."
  };
}

/**
 * The header label: printable ASCII, at most `PackHeader::kLabelBytes` bytes.
 *
 * REFUSED rather than truncated. `emitPack` writes the label into a fixed 20-byte field, so a longer one
 * would be cut, and a multi-byte character cut in the middle is not a character at all. ASCII makes
 * characters and bytes the same count, which is the only way the limit can be stated to a person.
 *
 * Today's Select Menu shows the FILE NAME, not this label (`PackSelector::labelAt`), so a wrong label is
 * invisible on the device. It is checked anyway because §7 Q8 decided the selector should show it, and a
 * pack built now will still be on a card when that lands.
 */
export function checkPackLabel(label: string, limits: FirmwarePackLimits): ValidationCheck {
  const id = "pack-label";
  const title = "The label fits the pack header";
  const problems: string[] = [];
  if (label.length === 0) {
    problems.push("it is empty");
  }
  if (!/^[\x20-\x7e]*$/.test(label)) {
    problems.push("it contains characters outside printable ASCII");
  }
  const bytes = Buffer.byteLength(label, "utf-8");
  if (bytes > limits.labelBytes) {
    problems.push(`it is ${bytes} bytes and the header holds ${limits.labelBytes} (PackHeader::kLabelBytes)`);
  }
  if (problems.length > 0) {
    return {
      id,
      title,
      status: "fail",
      message: `The label "${label}" cannot be stored: ${problems.join("; ")}.`,
      recommendation:
        `Pass --pack-label with at most ${limits.labelBytes} printable ASCII characters. Without it, ` +
        "the label is the file name minus its suffix."
    };
  }
  return {
    id,
    title,
    status: "pass",
    message: `"${label}" is ${bytes} of at most ${limits.labelBytes} bytes.`
  };
}

/** `PackLoader::load` step 7: a pack larger than `kMaxPackBytes` is refused before it is read. */
export function checkPackSize(byteLength: number, limits: FirmwarePackLimits): ValidationCheck {
  const id = "pack-size";
  const title = "The pack fits the device's buffer";
  if (byteLength > limits.maxPackBytes) {
    return {
      id,
      title,
      status: "fail",
      message:
        `The pack is ${byteLength} bytes and the device refuses anything over ${limits.maxPackBytes} ` +
        "(PackLoader::kMaxPackBytes): it would boot the built-in menu and report \"menu too large\".",
      recommendation: "Remove screens or shorten repeated text; strings are already deduplicated."
    };
  }
  return {
    id,
    title,
    status: "pass",
    message: `${byteLength} of at most ${limits.maxPackBytes} bytes.`
  };
}

/**
 * The ABI the pack is stamped with must come from the manifest, never from a default.
 *
 * `MenuPack::validate` refuses a pack whose ABI is newer than the firmware's, and compares nothing else
 * about the vocabulary, so a pack stamped from a guess either fails on every device or passes on devices
 * whose catalogue it does not match. The manifest schema leaves the field optional for old manifests;
 * this is where its absence stops being acceptable.
 */
export function checkPackCatalogueAbi(catalogueAbi: number | undefined): ValidationCheck {
  const id = "pack-catalogue-abi";
  const title = "The pack can be stamped with the firmware's catalogue ABI";
  if (typeof catalogueAbi !== "number") {
    return {
      id,
      title,
      status: "fail",
      message:
        "The manifest carries no catalogueAbi, so there is nothing true to stamp the pack with.",
      recommendation:
        "Regenerate the manifest from the firmware (Water-Flow-Meter-PlatformIO/tools/manifest_gen/run.sh), " +
        "which emits ui::kUiCatalogueAbi."
    };
  }
  return {
    id,
    title,
    status: "pass",
    message: `Stamped with catalogue ABI ${catalogueAbi}, the manifest's.`
  };
}

/**
 * Every flow target names a screen in this pack.
 *
 * Pack-specific, because only the pack resolves targets at BUILD time: the emitter turns each
 * `targetScreenId` into an index and writes `NO_TARGET_SCREEN` for a name it cannot find — which the
 * firmware reads as "this flow does not navigate". A typo in an ENTER target is therefore a dead button
 * with no report anywhere. The ring-closure gate covers UP and DOWN only, so this is the check that
 * covers the rest.
 */
export function checkPackFlowTargets(unresolvedTargets: string[]): ValidationCheck {
  const id = "pack-flow-targets";
  const title = "Every flow target is a screen in the pack";
  if (unresolvedTargets.length > 0) {
    return {
      id,
      title,
      status: "fail",
      message:
        `${unresolvedTargets.length} flow(s) name a target screen the pack does not contain; on the ` +
        "device each would be a button that does nothing.",
      recommendation: unresolvedTargets.join("; ")
    };
  }
  return {
    id,
    title,
    status: "pass",
    message: "Every flow with a target resolves to a screen index."
  };
}
