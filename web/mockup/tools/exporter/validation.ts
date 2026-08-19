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

/**
 * Blocks export of a dataset whose paging rings do not close (J1).
 *
 * WHY THIS IS AN EXPORT GATE AND NOT A RUNTIME CHECK. Paging wraps in the DATASET, not in code:
 * `UiNavigator` follows each screen's own DOWN flow and resolves the target by linear search of
 * `kGeneratedScreens`, so a level is a ring only because the authored data says so. There is no modulo
 * arithmetic to fall back on. A pack whose last member points nowhere, or points back into the middle,
 * strands the operator on the device with UP and DOWN that no longer return — and nothing on the device
 * can detect that, because a flow with a target is exactly what a working flow looks like.
 *
 * The built-in dataset is closed because `tools/skeleton/generate.mjs` emits it that way. A third-party
 * `.uipack` has no such guarantee, which is what `Loadable_UI_Menu_Packs.md` assumes and what this
 * supplies. Same family as N-b: an export-time gate the format's design takes for granted.
 *
 * FOUR failure modes, each its own message, because "the ring is broken" is not actionable:
 *
 *  1. a DOWN or UP target naming a screen that does not exist;
 *  2. a walk that never returns to where it started — a dead end, or a jump into another level;
 *  3. UP that is not the exact inverse of DOWN, which is how a ring loses a member in one direction
 *     while looking whole in the other;
 *  4. a screen carrying one direction and not the other, which is a level you can leave and not re-enter.
 *
 * What it deliberately does NOT require: that every screen belong to a ring. Twenty of the eighty do not
 * — editors, `-back` rows and confirm screens are entered by ENTER and left by hold, and demanding a ring
 * of them would fail the shipped dataset for being correct.
 */
export function checkRingClosure(dataset: ScreenDataset): ValidationCheck {
  const id = "ring-closure";
  const title = "Every paging ring closes";
  const known = new Set(dataset.screens.map((screen) => screen.id));

  const next = new Map<string, string | undefined>();
  const previous = new Map<string, string | undefined>();
  const problems: string[] = [];

  /**
   * A PAGING EDGE IS A BUTTON PRESS, not merely the action id.
   *
   * This first matched on `actionId` alone and failed the exporter's own fixture, which carries a
   * `page.next` on a TIMEOUT trigger with no target — a confirm screen advancing itself after a hold, which
   * is not paging and has no ring to belong to. `UiNavigator` pages when UP or DOWN is pressed, so that is
   * what the ring is built from.
   */
  const isButton = (flow: { trigger?: { type?: string; button?: string } }, button: string) =>
    flow.trigger?.type === "button" && flow.trigger?.button === button;

  for (const screen of dataset.screens) {
    for (const flow of screen.flows ?? []) {
      if (flow.actionId === "ui.action.page.next" && isButton(flow, "down")) {
        next.set(screen.id, flow.targetScreenId);
      }
      if (flow.actionId === "ui.action.page.previous" && isButton(flow, "up")) {
        previous.set(screen.id, flow.targetScreenId);
      }
    }
  }

  // (1) and (4), per screen: a target that resolves, and both directions present.
  for (const [screenId, target] of next) {
    if (!target) {
      problems.push(`${screenId}: its DOWN flow carries no target screen`);
    } else if (!known.has(target)) {
      problems.push(`${screenId}: DOWN points at "${target}", which is not a screen in this dataset`);
    }
    if (!previous.has(screenId)) {
      problems.push(`${screenId}: pages DOWN but has no UP flow — a level it can leave and not re-enter`);
    }
  }
  for (const [screenId, target] of previous) {
    if (!target) {
      problems.push(`${screenId}: its UP flow carries no target screen`);
    } else if (!known.has(target)) {
      problems.push(`${screenId}: UP points at "${target}", which is not a screen in this dataset`);
    }
    if (!next.has(screenId)) {
      problems.push(`${screenId}: pages UP but has no DOWN flow`);
    }
  }

  // (3) UP is the inverse of DOWN. Checked per edge rather than per ring, so the message names the pair
  // that disagrees instead of the level that contains it.
  for (const [screenId, target] of next) {
    if (!target || !known.has(target)) continue;
    const back = previous.get(target);
    if (back !== screenId) {
      problems.push(
        `${screenId}: DOWN goes to "${target}", but that screen's UP returns to ` +
          `"${back ?? "(nothing)"}" — UP must be the exact inverse of DOWN`
      );
    }
  }

  // (2) every ring closes, and closes on itself rather than on a member further along.
  const visited = new Set<string>();
  let ringsChecked = 0;
  let membersChecked = 0;
  for (const start of next.keys()) {
    if (visited.has(start)) continue;
    const walk: string[] = [];
    let cursor: string | undefined = start;
    while (cursor && next.has(cursor) && !walk.includes(cursor)) {
      walk.push(cursor);
      cursor = next.get(cursor);
    }
    for (const member of walk) visited.add(member);
    ringsChecked += 1;
    membersChecked += walk.length;
    if (cursor === undefined || !next.has(cursor)) {
      problems.push(
        `${start}: paging DOWN reaches "${cursor ?? "(nothing)"}", which pages no further — ` +
          `the ring is a dead end after ${walk.length} screen(s)`
      );
    } else if (cursor !== start) {
      problems.push(
        `${start}: paging DOWN returns to "${cursor}" rather than to itself — the ring closes on a ` +
          `member further along, so the screens before it are unreachable once you leave them`
      );
    }
  }

  if (problems.length > 0) {
    return {
      id,
      title,
      status: "fail",
      message:
        `${problems.length} paging problem(s) across ${ringsChecked} ring(s). On the device this is an ` +
        `operator pressing UP or DOWN and not coming back.`,
      recommendation: problems.join("; ")
    };
  }

  return {
    id,
    title,
    status: "pass",
    message:
      `${ringsChecked} ring(s) covering ${membersChecked} screen(s) close in both directions, and UP is ` +
      `the inverse of DOWN throughout.`
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
    checkRingClosure(dataset),
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
