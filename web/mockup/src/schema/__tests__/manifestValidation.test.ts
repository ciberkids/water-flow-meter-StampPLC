import { describe, it, expect } from "vitest";
import { validateManifest } from "../manifestValidation";

describe("validateManifest", () => {
    it("should validate a correct manifest", () => {
        const valid = {
            actions: [{ id: "a1", name: "Action 1" }],
            values: [{ id: "v1", name: "Value 1", type: "int", readOnly: true }]
        };
        expect(() => validateManifest(valid)).not.toThrow();
    });

    it("should fail if actions are missing", () => {
        const invalid = { values: [] };
        expect(() => validateManifest(invalid)).toThrow();
    });

    it("should fail if values are missing", () => {
        const invalid = { actions: [] };
        expect(() => validateManifest(invalid)).toThrow();
    });

    it("should fail if action is missing required fields", () => {
        const invalid = {
            actions: [{ id: "a1" }], // missing name
            values: []
        };
        expect(() => validateManifest(invalid)).toThrow();
    });

    it("should fail if value is missing required fields", () => {
        const invalid = {
            actions: [],
            values: [{ id: "v1", name: "Value 1" }] // missing type/readOnly
        };
        expect(() => validateManifest(invalid)).toThrow();
    });

    it("should allow optional fields", () => {
        const valid = {
            actions: [{ id: "a1", name: "Action 1", description: "Desc", triggers: ["t1"] }],
            values: []
        };
        expect(() => validateManifest(valid)).not.toThrow();
    });
});
