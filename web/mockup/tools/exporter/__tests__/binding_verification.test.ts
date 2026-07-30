/**
 * PARKED — these cover emitUiEvents/emitUiBindings, which the export pipeline no
 * longer calls.
 *
 * Those emitters generate per-screen `RegisterEvents_` and `UpdateValues_`
 * functions against headers that do not exist in the firmware (FirmwareAction.h,
 * ScreenManager.h, FirmwareValues.h) and duplicate the runtime mechanism the
 * firmware actually uses (ui/core/ui_bindings.cpp resolving bindingId against
 * UiRenderContext, ui/core/ui_actions.cpp dispatching actionId). Writing their
 * output into src/ui/generated/ put uncompilable translation units in the build.
 *
 * They are kept green rather than deleted because the compile-time binding
 * design they represent is a real option that has not been formally dropped.
 * Decide: implement the firmware side, or delete emitters + these tests.
 */
import { describe, it } from "node:test";
import assert from "node:assert/strict";
import { buildIntermediateRepresentation } from "../ir.js";
import { emitUiEvents } from "../eventEmitter.js";
import { emitUiBindings } from "../valueEmitter.js";
import { ScreenDataset } from "../../../src/types.js";
import { ThemeTokens } from "../../../src/theme/types.js";

const kMockTheme: ThemeTokens = {
    name: "Test Theme",
    colors: {
        displayBackground: "#000000",
        textPrimary: "#ffffff",
        textMuted: "#cccccc",
        textStrong: "#ffffff",
        value: "#00ff00",
        badgeBackground: "#333333",
        badgeBorder: "#666666",
        icon: "#00ff00",
        legend: "#888888",
        gridMinor: "#111111",
        gridMajor: "#222222"
    },
    typography: { base: 8, value: 10, badge: 8 },
    animation: { easing: "linear" }
};

const kMockDataset: ScreenDataset = {
    screens: [
        {
            id: "screen1",
            name: "Screen 1",
            elements: [
                {
                    id: "val1",
                    kind: "value",
                    x: 0,
                    y: 0,
                    content: "0",
                    dataSourceId: "test.value.1"
                }
            ],
            events: [
                {
                    trigger: "Button A Click",
                    actionId: "test.action.1"
                },
                {
                    trigger: "Button B Click",
                    targetScreenId: "screen2"
                }
            ],
            flows: [],
            submenus: [],
            assets: [],
            animations: []
        }
    ],
    theme: kMockTheme
};

describe("Exporter Binding Verification", () => {
    it("should generate correct C++ event binding code", () => {
        const ir = buildIntermediateRepresentation(kMockDataset, kMockTheme);
        const code = emitUiEvents(ir);

        assert.ok(code.includes("void RegisterEvents_Screen1() {"));
        assert.ok(code.includes('ScreenManager::RegisterEvent("Button A Click", []() {'));
        assert.ok(code.includes('DispatchAction("test.action.1");'));
        assert.ok(code.includes('ScreenManager::RegisterEvent("Button B Click", []() {'));
        assert.ok(code.includes('RequestScreenLoad("screen2");'));
    });

    it("should generate correct C++ value binding code", () => {
        const ir = buildIntermediateRepresentation(kMockDataset, kMockTheme);
        const { source } = emitUiBindings(ir);

        assert.ok(source.includes("void UpdateValues_Screen1() {"));
        assert.ok(source.includes('if (GetFirmwareValueString("test.value.1", buffer, sizeof(buffer))) {'));
        assert.ok(source.includes('ScreenManager::UpdateElementText("val1", buffer);'));
    });
});
