// The decision half of `npm run export:pack`: given a dataset that has already passed every gate
// `export:firmware` runs, either produce the `.uipack` bytes or say exactly why not.
//
// Pure — no filesystem, no process — so every refusal is reachable from a test with an injected limit
// instead of a 64 KB dataset, and so `cli.ts` stays a shell that writes only what this returned. The
// bytes are WITHHELD on any failure rather than returned beside a failing report: a caller that forgot
// to look at `status` would otherwise write a pack the device refuses, which is the one outcome this
// command exists to prevent.
import { emitPack, type PackResult } from "./packEmitter.js";
import {
  checkPackCatalogueAbi,
  checkPackFileName,
  checkPackFlowTargets,
  checkPackLabel,
  checkPackSize,
  packStem,
  type FirmwarePackLimits
} from "./packRules.js";
import type { FirmwareManifest, ValidationCheck } from "./types.js";
import type { ScreenDataset } from "../../src/types.js";

export interface PackBuildInput {
  dataset: ScreenDataset;
  manifest: FirmwareManifest | null | undefined;
  /** The bare file name the pack will have on the card, e.g. `default.uipack`. */
  fileName: string;
  /** Omitted: the file name minus its suffix, which is what the Select Menu shows today. */
  label?: string;
  limits: FirmwarePackLimits;
}

export interface PackBuildOutcome {
  status: "pass" | "fail";
  /** The pack-specific checks, in the order they ran. The dataset gates are the caller's. */
  checks: ValidationCheck[];
  label: string;
  catalogueAbi?: number;
  /** Present only when every check passed. */
  pack?: PackResult;
}

/** The label used when none is given: the file name without the firmware's suffix. */
export function defaultPackLabel(fileName: string, limits: FirmwarePackLimits): string {
  return packStem(fileName, limits);
}

export function buildPack(input: PackBuildInput): PackBuildOutcome {
  const { dataset, manifest, fileName, limits } = input;
  const label = input.label ?? defaultPackLabel(fileName, limits);
  const catalogueAbi = manifest?.catalogueAbi;

  const checks: ValidationCheck[] = [
    checkPackFileName(fileName, limits),
    checkPackLabel(label, limits),
    checkPackCatalogueAbi(catalogueAbi)
  ];

  // Emitted whenever there is a true ABI to stamp, even if the name or label already failed, so one
  // run reports the size and the targets too instead of making the author fix one thing at a time.
  let pack: PackResult | undefined;
  if (typeof catalogueAbi === "number") {
    pack = emitPack(dataset, { label, catalogueAbi });
    checks.push(checkPackSize(pack.bytes.length, limits), checkPackFlowTargets(pack.unresolvedTargets));
  }

  const status = checks.some((check) => check.status === "fail") || !pack ? "fail" : "pass";
  return {
    status,
    checks,
    label,
    catalogueAbi,
    pack: status === "pass" ? pack : undefined
  };
}
