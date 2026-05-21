import Ajv from "ajv";
import { FirmwareActionManifest } from "../types/firmwareActions";

const ajv = new Ajv();

// eslint-disable-next-line @typescript-eslint/no-explicit-any
const schema: Record<string, any> = {
    type: "object",
    properties: {
        actions: {
            type: "array",
            items: {
                type: "object",
                properties: {
                    id: { type: "string" },
                    name: { type: "string" },
                    description: { type: "string", nullable: true },
                    triggers: { type: "array", items: { type: "string" }, nullable: true }
                },
                required: ["id", "name"]
            }
        },
        values: {
            type: "array",
            nullable: true,
            items: {
                type: "object",
                properties: {
                    id: { type: "string" },
                    type: { type: "string", nullable: true },
                    unit: { type: "string", nullable: true },
                    register: { type: "number", nullable: true },
                    description: { type: "string", nullable: true },
                    readOnly: { type: "boolean", nullable: true }
                },
                required: ["id"]
            }
        }
    },
    required: ["actions"],
    additionalProperties: true
};

const validate = ajv.compile(schema);

export function validateManifest(data: unknown): FirmwareActionManifest {
    if (validate(data)) {
        // Determine if we need to filter out unknown properties or if additionalProperties: true is enough to just let them pass.
        // The type definition doesn't have an index signature, so technically unexpected fields won't be accessible via strict TS, which is fine.
        return data as FirmwareActionManifest;
    }
    throw new Error(`Manifest validation failed: ${ajv.errorsText(validate.errors)}`);
}
