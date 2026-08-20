import { describe, it } from "node:test";
import assert from "node:assert/strict";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { loadManifest, buildManifestActionSet } from "../manifestLoader.js";
import { runExportValidations } from "../validation.js";
import type { ScreenDataset } from "../../../src/types.js";
import type { ThemeTokens } from "../../../src/theme/types.js";
import type { ExportIR } from "../types.js";

const testDir = path.dirname(fileURLToPath(import.meta.url));
const fixturesDir = path.resolve(testDir, "..", "..", "..", "..", "tests", "fixtures");

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
    // The export IR keeps its timestamp — unlike the manifest it is not a committed,
    // diffed artefact, and the firmware records it so a device can report what it is running.
    generatedAt: "2026-03-05T13:00:00.000Z",
    screenCount: 1,
    elementCount: 0,
    dataset: [],
    theme: {
        name: "Test",
        colors: {},
        // No `animation`: the IR theme stopped carrying the easing token with J2. The DATASET still has
        // it — the workspace's own CSS transitions read it through `--theme-animation-easing` — but nothing
        // on the device did, so it is no longer emitted or modelled here.
        typography: { base: 8, value: 10, badge: 8 }
    }
};

describe("manifestLoader", () => {
    it("returns missing status when path is not provided", async () => {
        const result = await loadManifest(undefined);
        assert.equal(result.status, "missing");
        assert.equal(result.manifest, null);
    });

    it("loads and validates the sample fixture successfully", async () => {
        const manifestPath = path.resolve(fixturesDir, "sample-manifest.json");
        const result = await loadManifest(manifestPath);
        assert.equal(result.status, "loaded");
        assert.notEqual(result.manifest, null);
        assert.equal(result.actionCount, 13);
    });

    it("returns invalid status for a missing file path", async () => {
        const result = await loadManifest("/nonexistent/path/manifest.json");
        assert.equal(result.status, "invalid");
        assert.ok(result.error?.includes("Cannot read manifest file"));
    });

    it("returns invalid for a JSON syntax error", async () => {
        const result = await loadManifest(path.resolve(fixturesDir, "..", "manual", "invalid_manifest_syntax.json"));
        // File may not exist; if not this is expected to fail gracefully.
        assert.ok(["invalid", "missing"].includes(result.status));
    });

    it("buildManifestActionSet returns null for no manifest", () => {
        const set = buildManifestActionSet(null);
        assert.equal(set, null);
    });

    it("buildManifestActionSet returns a set of action IDs", async () => {
        const manifestPath = path.resolve(fixturesDir, "sample-manifest.json");
        const result = await loadManifest(manifestPath);
        const set = buildManifestActionSet(result.manifest);
        assert.notEqual(set, null);
        assert.equal(set?.has("ui.action.page.next"), true);
        assert.equal(set?.has("core.action.save-config"), true);
        assert.equal(set?.has("unknown.action"), false);
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

    it("fails when no manifest is provided", () => {
        const report = runExportValidations(baseDataset, kMinimalIr, null);
        const manifestCheck = report.checks.find(c => c.id === "manifest-action-coverage");
        // Previously a non-blocking "warning", which left the coverage gate off
        // by default and let unimplemented actions through.
        assert.equal(manifestCheck?.status, "fail");
    });

    it("produces pass when all actions are covered by the manifest", async () => {
        const manifestPath = path.resolve(fixturesDir, "sample-manifest.json");
        const result = await loadManifest(manifestPath);
        const report = runExportValidations(baseDataset, kMinimalIr, result.manifest);
        const manifestCheck = report.checks.find(c => c.id === "manifest-action-coverage");
        assert.equal(manifestCheck?.status, "pass");
    });

    it("produces pass when all bindings are covered by the manifest", async () => {
        const manifestPath = path.resolve(fixturesDir, "sample-manifest.json");
        const result = await loadManifest(manifestPath);
        const report = runExportValidations(baseDataset, kMinimalIr, result.manifest);
        const valueCheck = report.checks.find(c => c.id === "manifest-value-coverage");
        assert.equal(valueCheck?.status, "pass");
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
        const manifestCheck = report.checks.find(c => c.id === "manifest-action-coverage");
        assert.equal(manifestCheck?.status, "fail");
        assert.ok(manifestCheck?.message.includes("1 action binding(s)"));
    });

    it("flags ui.* and core.* actions that the manifest does not declare", async () => {
        // These prefixes used to be exempted as "builtin", which is backwards:
        // they are exactly the IDs the firmware action registry must implement.
        const datasetWithUnknownBuiltin: ScreenDataset = {
            ...baseDataset,
            screens: [
                {
                    ...baseDataset.screens[0],
                    flows: [
                        {
                            id: "flow-ghost",
                            label: "Ghost",
                            trigger: { type: "button", button: "enter" },
                            actionId: "core.action.not-implemented"
                        }
                    ]
                },
                baseDataset.screens[1]
            ]
        };

        const manifestPath = path.resolve(fixturesDir, "sample-manifest.json");
        const result = await loadManifest(manifestPath);
        const report = runExportValidations(datasetWithUnknownBuiltin, kMinimalIr, result.manifest);
        const manifestCheck = report.checks.find(c => c.id === "manifest-action-coverage");
        assert.equal(manifestCheck?.status, "fail");
        assert.match(manifestCheck?.recommendation ?? "", /core\.action\.not-implemented/);
    });
});
