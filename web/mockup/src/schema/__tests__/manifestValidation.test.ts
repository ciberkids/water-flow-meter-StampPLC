import { describe, it, expect } from "vitest";
import { validateManifest } from "../manifestValidation";

/**
 * These assert the canonical manifest schema from shared/schemaDefinitions.ts —
 * the same one the exporter validates against. They previously described a
 * second, app-only schema (`name` instead of `label`, no `version`), which is
 * how the checked-in manifest ended up in a format the exporter rejected.
 */
const validManifest = () => ({
    version: "1",
    actions: [{ id: "a1", label: "Action 1" }],
    values: [{ id: "v1", type: "number", register: 101, readOnly: true }]
});

describe("validateManifest", () => {
    it("validates a correct manifest", () => {
        expect(() => validateManifest(validManifest())).not.toThrow();
    });

    it("fails if version is missing", () => {
        const { version: _version, ...invalid } = validManifest();
        expect(() => validateManifest(invalid)).toThrow(/version/);
    });

    it("rejects generatedAt, which the generated manifest deliberately omits", () => {
        // A self-reported timestamp would make every regeneration look like a change, so
        // the manifest carries none and provenance comes from git.
        const invalid = { ...validManifest(), generatedAt: "2026-03-05T13:00:00.000Z" };
        expect(() => validateManifest(invalid)).toThrow(/additional properties/);
    });

    it("fails if actions are missing", () => {
        const { actions: _actions, ...invalid } = validManifest();
        expect(() => validateManifest(invalid)).toThrow(/actions/);
    });

    it("fails if an action is missing its label", () => {
        const invalid = { ...validManifest(), actions: [{ id: "a1" }] };
        expect(() => validateManifest(invalid)).toThrow(/label/);
    });

    it("fails if a value is missing its id", () => {
        const invalid = { ...validManifest(), values: [{ type: "number" }] };
        expect(() => validateManifest(invalid)).toThrow(/id/);
    });

    it("rejects a value type outside string|number|boolean", () => {
        const invalid = { ...validManifest(), values: [{ id: "v1", type: "int" }] };
        expect(() => validateManifest(invalid)).toThrow();
    });

    it("rejects unknown top-level properties", () => {
        const invalid = { ...validManifest(), updatedAt: "2026-03-05T13:00:00.000Z" };
        expect(() => validateManifest(invalid)).toThrow();
    });

    it("allows values to be omitted entirely", () => {
        const { values: _values, ...manifest } = validManifest();
        expect(() => validateManifest(manifest)).not.toThrow();
    });

    it("allows optional action description", () => {
        const valid = {
            ...validManifest(),
            actions: [{ id: "a1", label: "Action 1", description: "Desc" }]
        };
        expect(() => validateManifest(valid)).not.toThrow();
    });
});
