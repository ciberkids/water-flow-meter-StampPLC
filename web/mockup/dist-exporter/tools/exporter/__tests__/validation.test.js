import test from "node:test";
import assert from "node:assert/strict";
import { runExportValidations } from "../validation.js";
import { buildIntermediateRepresentation } from "../ir.js";
import { cloneTheme } from "../../../src/theme/types.js";
import { defaultTheme } from "../../../src/theme/defaultTheme.js";
function makeDataset() {
    return {
        screens: [
            {
                id: "info",
                name: "Info",
                elements: [
                    {
                        id: "legend-led",
                        kind: "text",
                        x: 0,
                        y: 0,
                        content: "Legend",
                        binding: "legend.led"
                    },
                    {
                        id: "diagnostics-badge",
                        kind: "badge",
                        x: 0,
                        y: 12,
                        content: "Diagnostics",
                        binding: "telemetry.status"
                    }
                ]
            },
            {
                id: "countdown",
                name: "Countdown",
                elements: [
                    {
                        id: "timer-value",
                        kind: "value",
                        x: 10,
                        y: 20,
                        content: "30 s",
                        binding: "countdown.value"
                    }
                ]
            }
        ],
        theme: cloneTheme(defaultTheme)
    };
}
test("runExportValidations passes when anchors exist", () => {
    const dataset = makeDataset();
    const ir = buildIntermediateRepresentation(dataset, dataset.theme);
    const report = runExportValidations(dataset, ir);
    assert.equal(report.status, "pass");
    assert.equal(report.issues.length, 0);
});
test("runExportValidations fails without LED legend", () => {
    const dataset = makeDataset();
    dataset.screens[0].elements = dataset.screens[0].elements.filter((element) => element.binding !== "legend.led");
    const ir = buildIntermediateRepresentation(dataset, dataset.theme);
    const report = runExportValidations(dataset, ir);
    assert.equal(report.status, "fail");
    assert.ok(report.issues.some((issue) => issue.includes("legend")));
});
test("runExportValidations fails without countdown screen", () => {
    const dataset = makeDataset();
    dataset.screens = dataset.screens.filter((screen) => screen.id !== "countdown");
    const ir = buildIntermediateRepresentation(dataset, dataset.theme);
    const report = runExportValidations(dataset, ir);
    assert.equal(report.status, "fail");
    assert.ok(report.issues.some((issue) => issue.includes("countdown")));
});
//# sourceMappingURL=validation.test.js.map