import fs from "node:fs/promises";
import path from "node:path";
import type { FirmwareManifest, ValidationCheck } from "./types.js";

/**
 * Extracts the action IDs the firmware action registry actually implements.
 *
 * This closes the last hole in the pipeline. The exporter can verify
 * dataset → manifest, but nothing verified manifest → firmware, which is how six
 * declared actions ended up with no handler and every button on the config
 * screens did nothing while the export reported a clean pass.
 *
 * Scraping C++ source is admittedly crude. The proper fix is decision D2: emit the
 * manifest *from* the firmware using the `kActionDescriptors` + `static_assert`
 * pattern that spike-report-SI-20251111-05 already recommended and specified. Until
 * that lands, a regex over the one table that defines dispatch is far better than
 * no check at all — and it fails loudly if the table's shape changes rather than
 * silently reporting zero.
 */
const kBindingsTablePattern = /const\s+UiActionBinding\s+kDefaultBindings\s*\[\s*\]\s*=\s*\{([\s\S]*?)\};/;
const kActionIdPattern = /\{\s*"([^"]+)"\s*,/g;

/**
 * Binding IDs the resolver actually handles, scraped from ui_bindings.cpp.
 *
 * `manifest-value-coverage` only proves a binding is *declared*. It does not prove
 * the firmware can *resolve* it — and an unresolved binding on an element with no
 * fallback text means UiRenderer::drawTextElement returns before drawing, so the
 * element is simply invisible on the device while the mockup shows a value. That is
 * how 39 elements across 21 screens ended up never rendering.
 */
export interface FirmwareBindingScrape {
  ok: boolean;
  exact: string[];
  sensorMetrics: string[];
  error?: string;
}

export interface FirmwareActionScrape {
  ok: boolean;
  actionIds: string[];
  error?: string;
  sourcePath: string;
}

export async function scrapeFirmwareActions(projectRoot: string): Promise<FirmwareActionScrape> {
  const sourcePath = path.join(
    projectRoot,
    "Water-Flow-Meter-PlatformIO",
    "src",
    "ui",
    "core",
    "ui_actions.cpp"
  );
  let source: string;
  try {
    source = await fs.readFile(sourcePath, "utf-8");
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    return { ok: false, actionIds: [], error: `Cannot read ${sourcePath}: ${message}`, sourcePath };
  }

  const table = kBindingsTablePattern.exec(source);
  if (!table) {
    return {
      ok: false,
      actionIds: [],
      error:
        "Could not locate the `const UiActionBinding kDefaultBindings[] = { ... }` table. " +
        "If it was renamed or restructured, update tools/exporter/firmwareActions.ts.",
      sourcePath
    };
  }

  const ids = [...table[1].matchAll(kActionIdPattern)].map((m) => m[1]);
  if (ids.length === 0) {
    return {
      ok: false,
      actionIds: [],
      error: "Found the bindings table but no action IDs inside it.",
      sourcePath
    };
  }
  return { ok: true, actionIds: ids, sourcePath };
}

/**
 * Reports manifest actions that the firmware registry does not implement.
 *
 * A declared-but-unimplemented action dispatches to nothing: the button is dead
 * and nothing says so. This is a warning rather than a failure because the dataset
 * is legitimately built ahead of the firmware — but the count is always stated, so
 * the interim state can never read as complete.
 */
export function checkFirmwareActionCoverage(
  manifest: FirmwareManifest | null | undefined,
  scrape: FirmwareActionScrape,
  usedActionIds: Set<string>
): ValidationCheck {
  const id = "firmware-action-coverage";
  const title = "Firmware implements the declared actions";

  if (!scrape.ok) {
    return {
      id,
      title,
      status: "fail",
      message: "Could not determine which actions the firmware implements.",
      recommendation: scrape.error
    };
  }
  if (!manifest) {
    return { id, title, status: "fail", message: "No manifest to compare against." };
  }

  const implemented = new Set(scrape.actionIds);
  // Only actions the dataset actually uses matter: an unused declaration is
  // harmless, an unimplemented *used* one is a dead button.
  const declaredAndUsed = manifest.actions.map((a) => a.id).filter((a) => usedActionIds.has(a));
  const dead = declaredAndUsed.filter((a) => !implemented.has(a)).sort();
  const orphaned = scrape.actionIds.filter((a) => !manifest.actions.some((m) => m.id === a)).sort();

  if (dead.length > 0) {
    return {
      id,
      title,
      status: "warning",
      message:
        `${dead.length} action(s) used by the dataset are declared in the manifest but have ` +
        `no handler in ui_actions.cpp — those buttons will do nothing on hardware.`,
      recommendation:
        `Dead: ${dead.join(", ")}. Register each in kDefaultBindings.` +
        (orphaned.length > 0 ? ` Implemented but undeclared: ${orphaned.join(", ")}.` : "")
    };
  }

  return {
    id,
    title,
    status: "pass",
    message: `All ${declaredAndUsed.length} action(s) used by the dataset have a firmware handler.`,
    recommendation:
      orphaned.length > 0 ? `Implemented but undeclared: ${orphaned.join(", ")}.` : undefined
  };
}

/** `binding == "x"` / `metric == "y"` comparisons in the resolver. */
const kExactBindingPattern = /binding\s*==\s*"([^"]+)"/g;
const kMetricPattern = /metric\s*==\s*"([^"]+)"/g;

export async function scrapeFirmwareBindings(projectRoot: string): Promise<FirmwareBindingScrape> {
  const sourcePath = path.join(
    projectRoot, "Water-Flow-Meter-PlatformIO", "src", "ui", "core", "ui_bindings.cpp"
  );
  let source: string;
  try {
    source = await fs.readFile(sourcePath, "utf-8");
  } catch (error) {
    const message = error instanceof Error ? error.message : String(error);
    return { ok: false, exact: [], sensorMetrics: [], error: `Cannot read ${sourcePath}: ${message}` };
  }
  const exact = [...new Set([...source.matchAll(kExactBindingPattern)].map((m) => m[1]))];
  const sensorMetrics = [...new Set([...source.matchAll(kMetricPattern)].map((m) => m[1]))];
  if (exact.length === 0 && sensorMetrics.length === 0) {
    return {
      ok: false, exact: [], sensorMetrics: [],
      error: "Found no binding comparisons in ui_bindings.cpp; the resolver's shape may have changed."
    };
  }
  return { ok: true, exact, sensorMetrics };
}

/** True when the firmware resolver can produce a value for this binding id. */
function isResolvable(binding: string, scrape: FirmwareBindingScrape): boolean {
  if (scrape.exact.includes(binding)) return true;
  const sensor = /^sensor\.(\d+)\.(.+)$/.exec(binding);
  if (sensor) return scrape.sensorMetrics.includes(sensor[2]);
  return false;
}

/**
 * Reports element bindings the firmware cannot resolve.
 *
 * A warning, not a failure: the dataset is legitimately authored ahead of the
 * resolver. But the count is always stated, because the failure mode is silent —
 * an element with an unresolvable binding and no fallback text draws nothing at all.
 */
export function checkFirmwareBindingCoverage(
  usedBindings: Map<string, string[]>,
  scrape: FirmwareBindingScrape
): ValidationCheck {
  const id = "firmware-binding-coverage";
  const title = "Firmware resolves the bound values";

  if (!scrape.ok) {
    return { id, title, status: "fail", message: "Could not determine which bindings the firmware resolves.", recommendation: scrape.error };
  }

  const unresolved = [...usedBindings.keys()].filter((b) => !isResolvable(b, scrape)).sort();
  if (unresolved.length > 0) {
    const elementCount = unresolved.reduce((n, b) => n + (usedBindings.get(b)?.length ?? 0), 0);
    return {
      id,
      title,
      status: "warning",
      message:
        `${unresolved.length} binding(s) used by ${elementCount} element(s) have no case in ` +
        `UiBindingResolver — elements with no fallback text will not render at all on the device.`,
      recommendation: `Unresolved: ${unresolved.join(", ")}.`
    };
  }
  return {
    id, title, status: "pass",
    message: `All ${usedBindings.size} bound value(s) are resolvable by the firmware.`
  };
}
