import Ajv, { JSONSchemaType } from "ajv";
import { FirmwareActionManifest } from "../types/firmwareActions";

const ajv = new Ajv();

const schema: JSONSchemaType<FirmwareActionManifest> = {
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
            items: {
                type: "object",
                properties: {
                    id: { type: "string" },
                    name: { type: "string" },
                    type: { type: "string", enum: ["int", "float", "string", "bool"] },
                    readOnly: { type: "boolean" }
                },
                required: ["id", "name", "type", "readOnly"]
            }
        }
    },
    required: ["actions", "values"],
    additionalProperties: true
};

const validate = ajv.compile(schema);

export function validateManifest(data: unknown): FirmwareActionManifest {
    if (validate(data)) {
        // Determine if we need to filter out unknown properties or if additionalProperties: true is enough to just let them pass.
        // The type definition doesn't have an index signature, so technically unexpected fields won't be accessible via strict TS, which is fine.
        return data;
    }
    throw new Error(`Manifest validation failed: ${ajv.errorsText(validate.errors)}`);
}
