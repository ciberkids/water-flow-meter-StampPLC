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
export function runExportValidations(dataset, ir) {
    const checks = validationChecks.map((check) => check(dataset, ir));
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