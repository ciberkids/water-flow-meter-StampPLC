import Ajv from "ajv";
import { firmwareManifestSchema, type FirmwareManifest } from "../../shared/schemaDefinitions";

/**
 * Validates a firmware manifest against the single canonical schema in
 * shared/schemaDefinitions.ts — the same one tools/exporter/manifestLoader.ts
 * uses.
 *
 * This file previously carried a second, hand-rolled schema that accepted
 * manifests the exporter rejected and vice versa (it required `name` where the
 * exporter required `label`, and made `values` optional with no field
 * requirements). That divergence is why the checked-in manifest could not be
 * passed to the exporter at all, leaving binding coverage unenforced.
 */
const AjvConstructor =
    (Ajv as unknown as { default?: new (...args: unknown[]) => Ajv }).default ??
    (Ajv as unknown as new (...args: unknown[]) => Ajv);

const ajv = new AjvConstructor({ strict: false });
const validate = ajv.compile(firmwareManifestSchema);

export function validateManifest(data: unknown): FirmwareManifest {
    if (validate(data)) {
        return data as FirmwareManifest;
    }
    throw new Error(`Manifest validation failed: ${ajv.errorsText(validate.errors)}`);
}
