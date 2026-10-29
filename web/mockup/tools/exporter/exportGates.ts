// The dataset gates, shared by `export:firmware` and `export:pack`.
//
// Moved out of `cli.ts` when the pack command arrived (N-g), so the two outputs are refused by ONE
// sequence of checks rather than by two copies that agree today. A pack and the compiled-in tables are
// two renderings of the same menu (Loadable_UI_Menu_Packs §5.2); a gate added to one path and not the
// other is exactly how a card pack would come to show a screen the built-in menu was refused for.
//
// What is NOT here is the compile check. It proves the generated C++ builds, and a pack is data the
// firmware validates when it loads it (§3.3) — there is nothing to compile, so `export:pack` says so in
// its report instead of running it or pretending it passed.
import fs from "node:fs/promises";
import { ensureValidDataset, ensureValidTheme, ExportValidationError } from "./schema.js";
import { buildIntermediateRepresentation } from "./ir.js";
import { checkRenderableElementKinds, runExportValidations } from "./validation.js";
import {
  checkFirmwareBindingCoverage,
  checkManifestResolvable,
  scrapeFirmwareBindings
} from "./firmwareActions.js";
import { loadManifest } from "./manifestLoader.js";
import type { ExportIR, FirmwareManifest, ManifestSummary, ValidationReport } from "./types.js";
import type { ScreenDataset } from "../../src/types.js";

export interface DatasetGateInput {
  /** The repository root, where the firmware sources the binding scrape reads live. */
  projectRoot: string;
  screensPath: string;
  themePath?: string;
  manifestPath?: string;
}

export type DatasetGateOutcome =
  | {
      /** The manifest was named but could not be read or failed its schema: nothing else ran. */
      kind: "manifest-invalid";
      manifestSummary: ManifestSummary;
    }
  | {
      /** Every gate ran and none failed; `validationReport` may still carry warnings. */
      kind: "passed";
      dataset: ScreenDataset;
      ir: ExportIR;
      manifest: FirmwareManifest | null;
      manifestSummary: ManifestSummary;
      validationReport: ValidationReport;
    };

async function readJsonFile<T>(filePath: string): Promise<T> {
  const raw = await fs.readFile(filePath, "utf-8");
  return JSON.parse(raw) as T;
}

/**
 * Runs every dataset gate. Throws `ExportValidationError` on a failing gate, which is what both
 * commands turn into exit code 2; returns `manifest-invalid` for the one case both report differently.
 */
export async function runDatasetGates(input: DatasetGateInput): Promise<DatasetGateOutcome> {
  const dataset = ensureValidDataset(await readJsonFile(input.screensPath));
  const themeTokens = input.themePath
    ? ensureValidTheme(await readJsonFile(input.themePath))
    : dataset.theme;

  // Must run before buildIntermediateRepresentation: the IR converter throws a
  // bare Error on an unmappable element kind, so checking first is what turns
  // that into a reportable validation failure instead of a stack trace.
  const kindCheck = checkRenderableElementKinds(dataset);
  if (kindCheck.status === "fail") {
    throw new ExportValidationError("Post-export validation failed", [kindCheck.message], {
      status: "fail",
      checks: [kindCheck],
      issues: [kindCheck.message],
      log: `[FAIL] ${kindCheck.title} — ${kindCheck.message}\n${kindCheck.recommendation ?? ""}`
    });
  }

  const ir = buildIntermediateRepresentation(dataset, themeTokens);

  // Load manifest (optional) and build the manifest status summary.
  const manifestResult = await loadManifest(input.manifestPath);
  const manifestSummary: ManifestSummary = {
    status: manifestResult.status,
    path: manifestResult.path,
    actionCount: manifestResult.actionCount,
    error: manifestResult.error
  };

  // Block export if manifest was explicitly provided but failed to load.
  if (manifestResult.status === "invalid") {
    return { kind: "manifest-invalid", manifestSummary };
  }

  const validationReport = runExportValidations(dataset, ir, manifestResult.manifest);

  // manifest -> firmware for the values the dataset binds. The action direction is now
  // covered without scraping: the firmware static_asserts its catalogue against its
  // handler table, and CI diffs the committed manifest against a fresh generation from
  // that catalogue. See the note in firmwareActions.ts.
  const usedBindings = new Map<string, string[]>();
  for (const screen of dataset.screens) {
    for (const element of screen.elements) {
      if (!element.binding) continue;
      const where = usedBindings.get(element.binding) ?? [];
      where.push(`${screen.id}/${element.id}`);
      usedBindings.set(element.binding, where);
    }
  }
  // Scraped once and shared: both checks ask the same question of the resolver, one for
  // the bindings this design uses and one for everything the manifest advertises.
  const bindingScrape = await scrapeFirmwareBindings(input.projectRoot);
  const bindingCheck = checkFirmwareBindingCoverage(usedBindings, bindingScrape);
  const advertisedCheck = checkManifestResolvable(manifestResult.manifest, bindingScrape);
  for (const check of [bindingCheck, advertisedCheck]) {
    validationReport.checks.push(check);
    if (check.status === "fail") {
      validationReport.status = "fail";
      validationReport.issues.push(check.message);
    }
    validationReport.log += `\n[${check.status.toUpperCase()}] ${check.title} — ${check.message}`;
  }

  if (validationReport.status === "fail") {
    throw new ExportValidationError(
      "Post-export validation failed",
      validationReport.issues,
      validationReport
    );
  }

  return {
    kind: "passed",
    dataset,
    ir,
    manifest: manifestResult.manifest,
    manifestSummary,
    validationReport
  };
}
