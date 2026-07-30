import { FIRMWARE_RENDERABLE_KINDS } from "../../src/types.js";
import type { ScreenDataset, ScreenDefinition, ScreenElement } from "../../src/types.js";
import type { ExportIR, ValidationCheck, ValidationReport, FirmwareManifest } from "./types.js";

type ElementMatch = {
  screen: ScreenDefinition;
  element: ScreenElement;
};

function findElement(
  dataset: ScreenDataset,
  predicate: (element: ScreenElement, screen: ScreenDefinition) => boolean
): ElementMatch | null {
  for (const screen of dataset.screens) {
    for (const element of screen.elements) {
      if (predicate(element, screen)) {
        return { screen, element };
      }
    }
  }
  return null;
}

const validationChecks: Array<
  (dataset: ScreenDataset, ir: ExportIR) => ValidationCheck
> = [
    (dataset) => {
      const match = findElement(dataset, (element) => element.binding === "legend.led");
      if (!match) {
        return {
          id: "led-legend",
          title: "LED legend is present",
          status: "fail",
          message: "Add a text element bound to legend.led so the LED legend renders in firmware.",
          recommendation: "Create a legend text element on any info screen and bind it to legend.led."
        };
      }
      return {
        id: "led-legend",
        title: "LED legend is present",
        status: "pass",
        message: `Element ${match.element.id} on ${match.screen.id} exposes the LED legend binding.`,
        screenId: match.screen.id,
        elementId: match.element.id
      };
    },
    (dataset) => {
      const countdownScreen = dataset.screens.find((screen) => /countdown/i.test(screen.id));
      if (!countdownScreen) {
        return {
          id: "countdown-overlay",
          title: "Countdown overlay exists",
          status: "fail",
          message: "Provide a screen with an id containing 'countdown' to cover factory reset overlays.",
          recommendation: "Duplicate the reference countdown screen and ensure it maps to UiMode::Idle hold logic."
        };
      }
      const timerElement = countdownScreen.elements.find(
        (element) => element.binding === "countdown.value"
      );
      if (!timerElement) {
        return {
          id: "countdown-overlay",
          title: "Countdown overlay exists",
          status: "fail",
          message: "Countdown screen is missing an element bound to countdown.value.",
          screenId: countdownScreen.id,
          recommendation: "Add a value element with binding countdown.value to show the timer."
        };
      }
      return {
        id: "countdown-overlay",
        title: "Countdown overlay exists",
        status: "pass",
        message: `Screen ${countdownScreen.id} exposes countdown.value via ${timerElement.id}.`,
        screenId: countdownScreen.id,
        elementId: timerElement.id
      };
    },
    (dataset) => {
      const diagElement = findElement(
        dataset,
        (element) =>
          element.kind === "badge" &&
          typeof element.binding === "string" &&
          /^(diagnostics|telemetry)\./i.test(element.binding)
      );
      if (!diagElement) {
        return {
          id: "diagnostics-banner",
          title: "Diagnostics banner is available",
          status: "fail",
          message: "Add a badge element bound to telemetry.* or diagnostics.* to surface faults.",
          recommendation: "Bind a badge/banner element to telemetry.status or diagnostics.summary."
        };
      }
      return {
        id: "diagnostics-banner",
        title: "Diagnostics banner is available",
        status: "pass",
        message: `Element ${diagElement.element.id} on ${diagElement.screen.id} surfaces diagnostics via ${diagElement.element.binding}.`,
        screenId: diagElement.screen.id,
        elementId: diagElement.element.id
      };
    }
  ];

/**
 * Validates that every actionId referenced in flows and screen events is present
 * in the firmware manifest.
 *
 * There is deliberately no prefix exemption here. `ui.*` and `core.*` were
 * previously treated as "built in" and skipped, which is exactly backwards:
 * those are the IDs the firmware action registry must implement, and exempting
 * them is how six unimplemented actions reached the dataset unnoticed.
 */
function checkManifestActionCoverage(
  dataset: ScreenDataset,
  manifest: FirmwareManifest | null | undefined
): ValidationCheck {
  if (!manifest) {
    return {
      id: "manifest-action-coverage",
      title: "Firmware manifest action coverage",
      status: "fail",
      message:
        "No firmware manifest available, so action bindings cannot be verified against firmware.",
      recommendation:
        "The exporter defaults to src/data/actionManifest.json. Restore that file, or pass " +
        "--manifest <path> explicitly."
    };
  }

  const knownIds = new Set(manifest.actions.map((a) => a.id));
  const missing: string[] = [];

  for (const screen of dataset.screens) {
    for (const flow of screen.flows ?? []) {
      const id = flow.actionId;
      if (id && !knownIds.has(id)) {
        missing.push(`${screen.id}/flow:${flow.id} references unknown action "${id}"`);
      }
    }
    for (const event of screen.events ?? []) {
      const id = event.actionId;
      if (id && !knownIds.has(id)) {
        missing.push(`${screen.id}/event:"${event.trigger}" references unknown action "${id}"`);
      }
    }
  }

  if (missing.length > 0) {
    return {
      id: "manifest-action-coverage",
      title: "Firmware manifest action coverage",
      status: "fail",
      message: `${missing.length} action binding(s) reference functions not found in the manifest.`,
      recommendation: missing.join("; ")
    };
  }

  return {
    id: "manifest-action-coverage",
    title: "Firmware manifest action coverage",
    status: "pass",
    message: `All action bindings are covered by the manifest (${manifest.actions.length} actions).`
  };
}

/**
 * Validates that every element `binding` resolves to a value the firmware
 * declares. An unknown binding renders its static placeholder text on hardware
 * and looks like a live value in the mockup, so it must block the export.
 */
function checkManifestValueCoverage(
  dataset: ScreenDataset,
  manifest: FirmwareManifest | null | undefined
): ValidationCheck {
  if (!manifest) {
    return {
      id: "manifest-value-coverage",
      title: "Firmware manifest value coverage",
      status: "fail",
      message:
        "No firmware manifest available, so value bindings cannot be verified against firmware.",
      recommendation: "See the action coverage check for how to supply a manifest."
    };
  }

  const knownIds = new Set((manifest.values ?? []).map((v) => v.id));
  const missing: string[] = [];

  for (const screen of dataset.screens) {
    for (const element of screen.elements) {
      const binding = element.binding;
      if (binding && !knownIds.has(binding)) {
        missing.push(`${screen.id}/${element.id} binds unknown value "${binding}"`);
      }
    }
  }

  if (missing.length > 0) {
    return {
      id: "manifest-value-coverage",
      title: "Firmware manifest value coverage",
      status: "fail",
      message: `${missing.length} element binding(s) reference values not found in the manifest.`,
      recommendation: missing.join("; ")
    };
  }

  return {
    id: "manifest-value-coverage",
    title: "Firmware manifest value coverage",
    status: "pass",
    message: `All element bindings are covered by the manifest (${(manifest.values ?? []).length} values).`
  };
}

/**
 * Validates that every screen ID the firmware router resolves by name exists in
 * the dataset.
 *
 * This is the third vocabulary, and the one whose drift is hardest to notice:
 * UiScreenRouter::findById returns nullptr for a missing ID, the renderer bails
 * after clearing the display, and the only symptom is a blank page. Renaming a
 * screen otherwise passes every other check and compiles cleanly.
 */
function checkManifestScreenCoverage(
  dataset: ScreenDataset,
  manifest: FirmwareManifest | null | undefined
): ValidationCheck {
  const required = manifest?.screens ?? [];
  if (required.length === 0) {
    return {
      id: "manifest-screen-coverage",
      title: "Firmware screen coverage",
      status: "fail",
      message:
        "The manifest declares no required screens, so screen-ID drift cannot be detected.",
      recommendation:
        "Add a `screens` array to the manifest listing every ID the firmware router " +
        "resolves (see kInfoScreenIds in ui_screen_router.cpp)."
    };
  }

  const defined = new Set(dataset.screens.map((screen) => screen.id));
  const missing = required
    .filter((screen) => !defined.has(screen.id))
    .map((screen) => `${screen.id}${screen.role ? ` (${screen.role})` : ""}`);

  if (missing.length > 0) {
    return {
      id: "manifest-screen-coverage",
      title: "Firmware screen coverage",
      status: "fail",
      message: `${missing.length} screen(s) required by firmware are not defined in the dataset.`,
      recommendation:
        `Missing: ${missing.join("; ")}. Renaming or deleting these makes the ` +
        "corresponding page render blank on hardware."
    };
  }

  return {
    id: "manifest-screen-coverage",
    title: "Firmware screen coverage",
    status: "pass",
    message: `All ${required.length} firmware-required screens are defined in the dataset.`
  };
}

/**
 * Blocks export of element kinds the firmware renderer cannot draw.
 *
 * Every kind in FIRMWARE_RENDERABLE_KINDS has an ir.ts case, a cppEmitter
 * mapping and a UiRenderer case. Anything else would be silently dropped on
 * hardware while the mockup kept showing it, so the export fails instead.
 */
export function checkRenderableElementKinds(dataset: ScreenDataset): ValidationCheck {
  const renderable = new Set<string>(FIRMWARE_RENDERABLE_KINDS);
  const unsupported: string[] = [];

  for (const screen of dataset.screens) {
    for (const element of screen.elements) {
      if (!renderable.has(element.kind)) {
        unsupported.push(`${screen.id}/${element.id} uses kind "${element.kind}"`);
      }
    }
  }

  if (unsupported.length > 0) {
    return {
      id: "renderable-element-kinds",
      title: "All element kinds are renderable by firmware",
      status: "fail",
      message:
        `${unsupported.length} element(s) use a kind the firmware renderer cannot draw ` +
        `(supported: ${FIRMWARE_RENDERABLE_KINDS.join(", ")}).`,
      recommendation: unsupported.join("; ")
    };
  }

  return {
    id: "renderable-element-kinds",
    title: "All element kinds are renderable by firmware",
    status: "pass",
    message: `Every element uses one of: ${FIRMWARE_RENDERABLE_KINDS.join(", ")}.`
  };
}

export function runExportValidations(
  dataset: ScreenDataset,
  ir: ExportIR,
  manifest?: FirmwareManifest | null
): ValidationReport {
  const checks = [
    ...validationChecks.map((check) => check(dataset, ir)),
    checkRenderableElementKinds(dataset),
    checkManifestScreenCoverage(dataset, manifest),
    checkManifestActionCoverage(dataset, manifest),
    checkManifestValueCoverage(dataset, manifest)
  ];
  const failing = checks.filter((check) => check.status === "fail");
  const status: ValidationReport["status"] = failing.length > 0 ? "fail" : "pass";
  const log = checks
    .map((check) => `[${check.status.toUpperCase()}] ${check.title} — ${check.message}`)
    .join("\n");

  return {
    status,
    checks,
    issues: failing.map((item) => item.message),
    log
  };
}
