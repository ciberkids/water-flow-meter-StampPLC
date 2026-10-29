// The decision half of `npm run export:pack`: given a dataset that has already passed every gate
// `export:firmware` runs, either produce the `.uipack` bytes or say exactly why not.
//
// Pure — no filesystem, no process — so every refusal is reachable from a test with an injected limit
// instead of a 64 KB dataset, and so `cli.ts` stays a shell that writes only what this returned. The
// bytes are WITHHELD on any failure rather than returned beside a failing report: a caller that forgot
// to look at `status` would otherwise write a pack the device refuses, which is the one outcome this
// command exists to prevent.
import { emitPack, PACK_LABEL_BYTES, type PackResult } from "./packEmitter.js";
import {
  checkPackCatalogueAbi,
  checkPackFileName,
  checkPackFlowTargets,
  checkPackLabel,
  checkPackSize,
  packStem,
  type FirmwarePackLimits
} from "./packRules.js";
import type { FirmwareManifest, IRTheme, ValidationCheck } from "./types.js";
import type { ScreenDataset } from "../../src/types.js";

export interface PackBuildInput {
  dataset: ScreenDataset;
  /**
   * The theme from the `ExportIR` the dataset gates built, so `--theme` reaches the pack exactly as it
   * reaches the C++ tables (Loadable_UI_Menu_Packs §5.2). See `PackOptions.theme` in packEmitter.ts.
   */
  theme: IRTheme;
  manifest: FirmwareManifest | null | undefined;
  /** The bare file name the pack will have on the card, e.g. `default.uipack`. */
  fileName: string;
  /**
   * An explicit label, refused when it does not fit. Omitted: the file name minus its suffix, cut to the
   * header field — see `defaultPackLabel`.
   */
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

/**
 * The label used when none is given: the file name without the firmware's suffix, CUT to
 * `PackHeader::kLabelBytes`.
 *
 * Cut, where an explicit `--pack-label` is refused, because nobody chose this text. Refusing it turned a
 * file name the device lists, loads and shows into a refusal over a field the Select Menu does not
 * display (`PackSelector::labelAt` returns the file name), so `service-menu-for-site-b.uipack` could not
 * be built at all (review of N-g). Cutting by UTF-16 unit is cutting by byte only for ASCII, which is the
 * one case where the result is ever written: a stem outside ASCII also fails `pack-file-name`, and its cut
 * label fails `pack-label` as well, so it never reaches the emitter.
 */
export function defaultPackLabel(fileName: string, limits: FirmwarePackLimits): string {
  return packStem(fileName, limits).slice(0, limits.labelBytes);
}

export function buildPack(input: PackBuildInput): PackBuildOutcome {
  const { dataset, theme, manifest, fileName, limits } = input;

  // The header's label field is part of the emitter's fixed layout, and `ui_pack.h` is where the reader's
  // size for it is read from. If they differ, the format has changed under this exporter: a reason to
  // stop, like a header whose shape `readHeaderConstant` no longer recognises, and not a check the author
  // can satisfy by editing the design. Before this comparison a larger `kLabelBytes` let `pack-label`
  // accept a label the emitter then cut short without a word.
  if (limits.labelBytes !== PACK_LABEL_BYTES) {
    throw new Error(
      `PackHeader::kLabelBytes is ${limits.labelBytes} in ui_pack.h, and packEmitter.ts lays out a ` +
        `${PACK_LABEL_BYTES}-byte label field. The pack format has changed; update packEmitter.ts with ` +
        "ui_pack.cpp before building a pack."
    );
  }

  const label = input.label ?? defaultPackLabel(fileName, limits);
  const catalogueAbi = manifest?.catalogueAbi;

  const labelCheck = checkPackLabel(label, limits);
  const checks: ValidationCheck[] = [
    checkPackFileName(fileName, limits),
    labelCheck,
    checkPackCatalogueAbi(catalogueAbi)
  ];

  // Emitted whenever there is a true ABI to stamp AND a label that fits, even if the file name already
  // failed, so one run reports the size and the targets too instead of making the author fix one thing
  // at a time. Not with a label that does not fit: `emitPack` refuses to cut one, and measuring with a
  // stand-in label would report the size of a pack nobody asked for. The label check's own message says
  // the size and the targets are checked once it fits.
  let pack: PackResult | undefined;
  if (typeof catalogueAbi === "number" && labelCheck.status !== "fail") {
    pack = emitPack(dataset, { label, catalogueAbi, theme });
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
