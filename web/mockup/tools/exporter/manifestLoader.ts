import fs from "node:fs/promises";
import AjvModule from "ajv";
import {
    firmwareManifestSchema,
    type FirmwareManifest
} from "../../shared/schemaDefinitions.js";

export type ManifestLoadStatus = "loaded" | "missing" | "invalid";

export interface ManifestLoadResult {
    status: ManifestLoadStatus;
    manifest: FirmwareManifest | null;
    actionCount?: number;
    path?: string;
    error?: string;
}

const AjvConstructor =
  (AjvModule as unknown as { default: new (...args: unknown[]) => any }).default ??
  (AjvModule as unknown as new (...args: unknown[]) => any);

const ajv = new AjvConstructor({ strict: false });
const validateManifest = ajv.compile(firmwareManifestSchema);

/**
 * Loads and validates a firmware manifest from disk.
 *
 * - If `manifestPath` is falsy → returns `{ status: "missing" }` (non-blocking warning).
 * - If the file cannot be read or fails JSON parse → returns `{ status: "invalid", error }`.
 * - If it doesn't match the schema → returns `{ status: "invalid", error }`.
 * - On success → `{ status: "loaded", manifest, actionCount }`.
 */
export async function loadManifest(
    manifestPath: string | undefined
): Promise<ManifestLoadResult> {
    if (!manifestPath) {
        return { status: "missing", manifest: null };
    }

    let raw: string;
    try {
        raw = await fs.readFile(manifestPath, "utf-8");
    } catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        return {
            status: "invalid",
            manifest: null,
            path: manifestPath,
            error: `Cannot read manifest file: ${message}`
        };
    }

    let parsed: unknown;
    try {
        parsed = JSON.parse(raw);
    } catch (err) {
        const message = err instanceof Error ? err.message : String(err);
        return {
            status: "invalid",
            manifest: null,
            path: manifestPath,
            error: `Manifest is not valid JSON: ${message}`
        };
    }

    if (!validateManifest(parsed)) {
        const errors = ajv.errorsText(validateManifest.errors);
        return {
            status: "invalid",
            manifest: null,
            path: manifestPath,
            error: `Manifest schema validation failed: ${errors}`
        };
    }

    const manifest = parsed as FirmwareManifest;
    return {
        status: "loaded",
        manifest,
        actionCount: manifest.actions.length,
        path: manifestPath
    };
}

/**
 * Returns the set of action IDs defined in the manifest, or null if no manifest.
 */
export function buildManifestActionSet(manifest: FirmwareManifest | null): Set<string> | null {
    if (!manifest) return null;
    return new Set(manifest.actions.map((a) => a.id));
}
