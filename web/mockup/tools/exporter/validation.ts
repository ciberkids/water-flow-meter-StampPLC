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
    /**
     * The LED pulse meaning must be readable somewhere on the panel.
     *
     * Accepts EITHER binding. `legend.status` (spec §3.4) folded the LED legend together with the
     * network row so P0 could give the walking dots the row back, which left `legend.led` with no
     * user — §3.4 anticipated exactly that and required it be resolved rather than left to rot.
     *
     * What this check is FOR is the fact, not the binding: an operator watching a red LED blink has
     * no way to know whether it means ten litres or a hundred unless the panel says so. Either
     * binding carries the pulse volume, so either satisfies it; neither present is still a failure.
     */
    (dataset) => {
      const legendBindings = ["legend.status", "legend.led"];
      const match = findElement(dataset, (element) =>
        legendBindings.includes(element.binding ?? "")
      );
      if (!match) {
        return {
          id: "led-legend",
          title: "LED legend is present",
          status: "fail",
          message:
            "No element binds legend.status or legend.led, so the LED pulse volume is nowhere on the panel.",
          recommendation:
            "Bind a text element on an info screen to legend.status (preferred, per §3.4) or legend.led."
        };
      }
      return {
        id: "led-legend",
        title: "LED legend is present",
        status: "pass",
        message: `Element ${match.element.id} on ${match.screen.id} exposes the LED legend via ${match.element.binding}.`,
        screenId: match.screen.id,
        elementId: match.element.id
      };
    },
    (dataset) => {
      // Semantic, not name-based: a confirm screen is one carrying a hold
      // countdown (a timeout trigger with holdButton) and showing the timer.
      // Matching on an id containing "countdown" broke the moment the screens
      // were renamed confirm-*, while the behaviour was still present.
      const confirmScreens = dataset.screens.filter((screen) =>
        (screen.flows ?? []).some(
          (flow) => flow.trigger.type === "timeout" && flow.trigger.holdButton !== undefined
        )
      );
      if (confirmScreens.length === 0) {
        return {
          id: "countdown-overlay",
          title: "Hold-to-confirm screen exists",
          status: "fail",
          message:
            "No screen declares a hold countdown (timeout trigger with holdButton), so no " +
            "destructive action can be confirmed.",
          recommendation:
            "Add a confirm screen with a timeout flow carrying holdButton: \"enter\"."
        };
      }
      const withoutTimer = confirmScreens.filter(
        (screen) => !screen.elements.some((element) => element.binding === "countdown.value")
      );
      if (withoutTimer.length > 0) {
        return {
          id: "countdown-overlay",
          title: "Hold-to-confirm screen exists",
          status: "fail",
          message: `${withoutTimer.length} confirm screen(s) do not show the remaining seconds.`,
          screenId: withoutTimer[0].id,
          recommendation:
            "Add a value element bound to countdown.value so the operator sees the timer " +
            `(missing on: ${withoutTimer.map((s2) => s2.id).join(", ")}).`
        };
      }
      return {
        id: "countdown-overlay",
        title: "Hold-to-confirm screen exists",
        status: "pass",
        message: `${confirmScreens.length} confirm screen(s) show countdown.value.`,
        screenId: confirmScreens[0].id
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
        /**
         * WARNING, not a failure — the owner's ruling, recorded rather than hidden.
         *
         * This check was written when P0 carried `telemetry.status` ("3 warnings" / "All sensors
         * ready"). The §3 redesign dropped that row to give the walking dots their space, so no
         * dataset element surfaces a fault any more and this check found nothing.
         *
         * The undersampling warning still reaches the operator, but through a path no dataset
         * element can declare: `drawWarningBanner` paints it edge to edge over the footer row
         * (§2c), from `context.warningFlags`, on whatever screen is showing. Asked whether the
         * summary row should come back, the owner's answer was that the banner is enough FOR NOW.
         *
         * So this stays as a standing note rather than being deleted: "for now" is not "never", and
         * a check quietly removed is a decision nobody can find again. It does not block the export,
         * and it was NOT rewritten to pass on something it does not actually verify.
         */
        return {
          id: "diagnostics-banner",
          title: "Diagnostics banner is available",
          status: "warning",
          message:
            "No dataset element surfaces a fault summary; undersampling reaches the operator only via " +
            "the firmware-drawn banner over the footer row (§2c).",
          recommendation:
            "Accepted by the owner for now. To put the summary back on the panel, bind a badge to " +
            "telemetry.status — P0 carried it before the §3 redesign."
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
  // Tracked so the pass message can report what was checked. Reporting the manifest's
  // size instead read as reassurance regardless of whether the dataset used anything.
  const checked = new Set<string>();

  for (const screen of dataset.screens) {
    for (const flow of screen.flows ?? []) {
      const id = flow.actionId;
      if (id) checked.add(id);
      if (id && !knownIds.has(id)) {
        missing.push(`${screen.id}/flow:${flow.id} references unknown action "${id}"`);
      }
    }
    for (const event of screen.events ?? []) {
      const id = event.actionId;
      if (id) checked.add(id);
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
    message:
      `${checked.size} distinct action(s) used by the dataset are all present in the ` +
      `manifest, which declares ${manifest.actions.length}.`
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

  const checkedBindings = new Set<string>();
  for (const screen of dataset.screens) {
    for (const element of screen.elements) {
      const binding = element.binding;
      if (binding) checkedBindings.add(binding);
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
    message:
      `${checkedBindings.size} distinct binding(s) used by the dataset are all present in ` +
      `the manifest, which declares ${(manifest.values ?? []).length}.`
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
