import { describe, it, expect } from "vitest";
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

        expect(code).toContain("void RegisterEvents_Screen1() {");
        expect(code).toContain('ScreenManager::RegisterEvent("Button A Click", []() {');
        expect(code).toContain('DispatchAction("test.action.1");');
        expect(code).toContain('ScreenManager::RegisterEvent("Button B Click", []() {');
        expect(code).toContain('RequestScreenLoad("screen2");');
    });

    it("should generate correct C++ value binding code", () => {
        const ir = buildIntermediateRepresentation(kMockDataset, kMockTheme);
        const { source } = emitUiBindings(ir);

        expect(source).toContain("void UpdateValues_Screen1() {");
        expect(source).toContain('if (GetFirmwareValueString("test.value.1", buffer, sizeof(buffer))) {');
        expect(source).toContain('ScreenManager::UpdateElementText("val1", buffer);');
    });
});
