function findElement(dataset, predicate) {
    for (const screen of dataset.screens) {
        for (const element of screen.elements) {
            if (predicate(element, screen)) {
                return { screen, element };
            }
        }
    }
    return null;
}
const validationChecks = [
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
        const timerElement = countdownScreen.elements.find((element) => element.binding === "countdown.value");
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
        const diagElement = findElement(dataset, (element) => element.kind === "badge" &&
            typeof element.binding === "string" &&
            /^(diagnostics|telemetry)\./i.test(element.binding));
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
/** Built-in action prefixes that don't need a manifest entry. */
const BUILTIN_PREFIXES = ["ui.", "core."];
function isBuiltinAction(actionId) {
    return BUILTIN_PREFIXES.some((prefix) => actionId.startsWith(prefix));
}
/**
 * Validates that every non-builtin actionId referenced in flows and screen events
 * is present in the firmware manifest.
 */
function checkManifestBindingCoverage(dataset, manifest) {
    if (!manifest) {
        return {
            id: "manifest-binding-coverage",
            title: "Firmware manifest binding coverage",
            status: "warning",
            message: "No firmware manifest provided. Custom action bindings cannot be verified. " +
                "Upload a manifest via the Import tab to enable coverage checks.",
            recommendation: "Run 'manifest_gen' from the firmware repo or upload ui_manifest.json via the Import & Export tab."
        };
    }
    const knownIds = new Set(manifest.actions.map((a) => a.id));
    const missing = [];
    for (const screen of dataset.screens) {
        for (const flow of screen.flows ?? []) {
            const id = flow.actionId;
            if (id && !isBuiltinAction(id) && !knownIds.has(id)) {
                missing.push(`${screen.id}/flow:${flow.id} references unknown action "${id}"`);
            }
        }
        for (const event of screen.events ?? []) {
            const id = event.actionId;
            if (id && !isBuiltinAction(id) && !knownIds.has(id)) {
                missing.push(`${screen.id}/event:"${event.trigger}" references unknown action "${id}"`);
            }
        }
    }
    if (missing.length > 0) {
        return {
            id: "manifest-binding-coverage",
            title: "Firmware manifest binding coverage",
            status: "fail",
            message: `${missing.length} action binding(s) reference functions not found in the manifest.`,
            recommendation: missing.join("; ")
        };
    }
    return {
        id: "manifest-binding-coverage",
        title: "Firmware manifest binding coverage",
        status: "pass",
        message: `All action bindings are covered by the manifest (${manifest.actions.length} actions).`
    };
}
export function runExportValidations(dataset, ir, manifest) {
    const checks = [
        ...validationChecks.map((check) => check(dataset, ir)),
        checkManifestBindingCoverage(dataset, manifest)
    ];
    const failing = checks.filter((check) => check.status === "fail");
    const status = failing.length > 0 ? "fail" : "pass";
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
//# sourceMappingURL=validation.js.map