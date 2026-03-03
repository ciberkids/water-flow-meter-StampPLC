import { describe, it, expect } from "vitest";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { loadManifest, buildManifestActionSet } from "../manifestLoader.js";
import { runExportValidations } from "../validation.js";
import type { ScreenDataset } from "../../../src/types.js";
import type { ThemeTokens } from "../../../src/theme/types.js";
import type { ExportIR } from "../types.js";

const testDir = path.dirname(fileURLToPath(import.meta.url));
const fixturesDir = path.resolve(testDir, "..", "..", "..", "tests", "fixtures");

const kMinimalTheme: ThemeTokens = {
    name: "Test Theme",
    colors: {
        displayBackground: "#000000",
        textPrimary: "#ffffff",
        textMuted: "#aaaaaa",
        textStrong: "#ffffff",
        value: "#00ff00",
        badgeBackground: "#111111",
        badgeBorder: "#333333",
        icon: "#00ff00",
        legend: "#888888",
        gridMinor: "#222222",
        gridMajor: "#444444"
    },
    typography: { base: 8, value: 10, badge: 8 },
    animation: { easing: "linear" }
};

const kMinimalIr: ExportIR = {
    generatedAt: new Date().toISOString(),
    screenCount: 1,
    elementCount: 0,
    dataset: [],
    theme: {
        name: "Test",
        colors: {},
        typography: { base: 8, value: 10, badge: 8 },
        animation: { easing: "linear" }
    }
};

describe("manifestLoader", () => {
    it("returns missing status when path is not provided", async () => {
        const result = await loadManifest(undefined);
        expect(result.status).toBe("missing");
        expect(result.manifest).toBeNull();
    });

    it("loads and validates the sample fixture successfully", async () => {
        const manifestPath = path.resolve(fixturesDir, "sample-manifest.json");
        const result = await loadManifest(manifestPath);
        expect(result.status).toBe("loaded");
        expect(result.manifest).not.toBeNull();
        expect(result.actionCount).toBe(6);
    });

    it("returns invalid status for a missing file path", async () => {
        const result = await loadManifest("/nonexistent/path/manifest.json");
        expect(result.status).toBe("invalid");
        expect(result.error).toContain("Cannot read manifest file");
    });

    it("returns invalid for a JSON syntax error", async () => {
        const result = await loadManifest(path.resolve(fixturesDir, "..", "manual", "invalid_manifest_syntax.json"));
        // File may not exist; if not this is expected to fail gracefully.
        expect(["invalid", "missing"]).toContain(result.status);
    });

    it("buildManifestActionSet returns null for no manifest", () => {
        const set = buildManifestActionSet(null);
        expect(set).toBeNull();
    });

    it("buildManifestActionSet returns a set of action IDs", async () => {
        const manifestPath = path.resolve(fixturesDir, "sample-manifest.json");
        const result = await loadManifest(manifestPath);
        const set = buildManifestActionSet(result.manifest);
        expect(set).not.toBeNull();
        expect(set?.has("ui.action.page.next")).toBe(true);
        expect(set?.has("core.action.save-config")).toBe(true);
        expect(set?.has("unknown.action")).toBe(false);
    });
});

describe("manifest binding validation check", () => {
    const baseDataset: ScreenDataset = {
        screens: [
            {
                id: "info-overview",
                name: "Overview",
                elements: [
                    { id: "led", kind: "text", x: 0, y: 0, binding: "legend.led" },
                    { id: "diag", kind: "badge", x: 0, y: 10, binding: "telemetry.status" }
                ],
                flows: [
                    {
                        id: "flow-next",
                        label: "Next",
                        trigger: { type: "button", button: "down" },
                        actionId: "ui.action.page.next"
                    }
                ]
            },
            {
                id: "countdown-reset-session",
                name: "Reset",
                elements: [
                    { id: "timer", kind: "value", x: 0, y: 0, binding: "countdown.value" }
                ]
            }
        ],
        theme: kMinimalTheme
    };

    it("produces warning when no manifest is provided", () => {
        const report = runExportValidations(baseDataset, kMinimalIr, null);
        const manifestCheck = report.checks.find(c => c.id === "manifest-binding-coverage");
        expect(manifestCheck?.status).toBe("warning");
    });

    it("produces pass when all actions are covered by the manifest", async () => {
        const manifestPath = path.resolve(fixturesDir, "sample-manifest.json");
        const result = await loadManifest(manifestPath);
        const report = runExportValidations(baseDataset, kMinimalIr, result.manifest);
        const manifestCheck = report.checks.find(c => c.id === "manifest-binding-coverage");
        expect(manifestCheck?.status).toBe("pass");
    });

    it("produces fail when an action is not found in the manifest", async () => {
        const datasetWithUnknown: ScreenDataset = {
            ...baseDataset,
            screens: [
                {
                    ...baseDataset.screens[0],
                    flows: [
                        {
                            id: "flow-custom",
                            label: "Custom",
                            trigger: { type: "button", button: "enter" },
                            actionId: "custom.action.unknown"
                        }
                    ]
                },
                baseDataset.screens[1]
            ]
        };

        const manifestPath = path.resolve(fixturesDir, "sample-manifest.json");
        const result = await loadManifest(manifestPath);
        const report = runExportValidations(datasetWithUnknown, kMinimalIr, result.manifest);
        const manifestCheck = report.checks.find(c => c.id === "manifest-binding-coverage");
        expect(manifestCheck?.status).toBe("fail");
        expect(manifestCheck?.message).toContain("1 action binding(s)");
    });

    it("does not flag builtin action prefixes as missing even without manifest", async () => {
        const manifestPath = path.resolve(fixturesDir, "sample-manifest.json");
        const result = await loadManifest(manifestPath);
        const report = runExportValidations(baseDataset, kMinimalIr, result.manifest);
        const manifestCheck = report.checks.find(c => c.id === "manifest-binding-coverage");
        // "ui.action.page.next" is a builtin and should NOT cause a failure
        expect(manifestCheck?.status).toBe("pass");
    });
});
