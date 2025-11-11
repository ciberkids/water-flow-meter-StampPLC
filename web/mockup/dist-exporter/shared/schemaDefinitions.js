const colorPattern = "^#([0-9a-fA-F]{6})$|^rgba\\(\\s*([01]?\\d{1,2}|2[0-4]\\d|25[0-5])\\s*,\\s*([01]?\\d{1,2}|2[0-4]\\d|25[0-5])\\s*,\\s*([01]?\\d{1,2}|2[0-4]\\d|25[0-5])\\s*,\\s*(0(\\.\\d+)?|1(\\.0+)?)\\s*\\)$";
export const elementSchema = {
    type: "object",
    additionalProperties: false,
    required: ["id", "kind", "x", "y"],
    properties: {
        id: { type: "string", minLength: 1 },
        kind: {
            type: "string",
            enum: ["text", "value", "badge", "box", "icon"]
        },
        x: { type: "integer" },
        y: { type: "integer" },
        width: { type: "integer", nullable: true },
        height: { type: "integer", nullable: true },
        content: { type: "string", minLength: 1, nullable: true },
        align: { type: "string", enum: ["left", "center", "right"], nullable: true },
        emphasis: {
            type: "string",
            enum: ["normal", "strong", "muted"],
            nullable: true
        },
        binding: { type: "string", nullable: true }
    }
};
const buttonTriggerSchema = {
    type: "object",
    additionalProperties: false,
    required: ["type", "button"],
    properties: {
        type: { type: "string", const: "button" },
        button: { type: "string", enum: ["up", "down", "enter"] },
        gesture: { type: "string", enum: ["short", "long", "hold"], nullable: true }
    }
};
const timeoutTriggerSchema = {
    type: "object",
    additionalProperties: false,
    required: ["type", "durationMs"],
    properties: {
        type: { type: "string", const: "timeout" },
        durationMs: { type: "integer", minimum: 1 }
    }
};
const dataTriggerSchema = {
    type: "object",
    additionalProperties: false,
    required: ["type", "source", "condition"],
    properties: {
        type: { type: "string", const: "data" },
        source: { type: "string", minLength: 1 },
        condition: { type: "string", minLength: 1 }
    }
};
export const flowSchema = {
    type: "object",
    additionalProperties: false,
    required: ["id", "label", "trigger"],
    properties: {
        id: { type: "string", minLength: 1 },
        label: { type: "string", minLength: 1 },
        targetScreenId: { type: "string", minLength: 1, nullable: true },
        trigger: {
            oneOf: [
                buttonTriggerSchema,
                timeoutTriggerSchema,
                dataTriggerSchema
            ]
        },
        guard: { type: "string", nullable: true },
        actionId: { type: "string", minLength: 1, nullable: true },
        actionParams: {
            type: "object",
            nullable: true,
            required: [],
            additionalProperties: {
                oneOf: [{ type: "string" }, { type: "number" }, { type: "boolean" }]
            }
        }
    }
};
export const graphicAssetSchema = {
    type: "object",
    additionalProperties: false,
    required: ["id", "type", "source"],
    properties: {
        id: { type: "string", minLength: 1 },
        type: { type: "string", enum: ["svg-sequence", "bitmap", "icon"] },
        source: { type: "string", minLength: 1 },
        frames: {
            type: "array",
            nullable: true,
            items: { type: "string", minLength: 1 }
        },
        fps: { type: "integer", nullable: true, minimum: 1 },
        palette: {
            type: "array",
            nullable: true,
            items: { type: "string", pattern: colorPattern }
        }
    }
};
export const animationKeyframeSchema = {
    type: "object",
    additionalProperties: false,
    required: ["at", "state"],
    properties: {
        at: { type: "number", minimum: 0 },
        state: {
            type: "object",
            required: [],
            additionalProperties: {
                oneOf: [{ type: "number" }, { type: "string" }, { type: "boolean" }]
            }
        },
        assetFrameIndex: { type: "integer", nullable: true, minimum: 0 }
    }
};
export const animationSchema = {
    type: "object",
    additionalProperties: false,
    required: ["id", "targetElementId", "kind", "frames"],
    properties: {
        id: { type: "string", minLength: 1 },
        targetElementId: { type: "string", minLength: 1 },
        kind: { type: "string", enum: ["frame-sequence", "property"] },
        frames: {
            type: "array",
            minItems: 1,
            items: animationKeyframeSchema
        },
        loop: { type: "boolean", nullable: true },
        easing: {
            type: "string",
            nullable: true,
            enum: ["linear", "step", "ease-in", "ease-out", "ease-in-out"]
        }
    }
};
export const submenuSchema = {
    type: "object",
    additionalProperties: false,
    required: ["id", "label", "screenId"],
    properties: {
        id: { type: "string", minLength: 1 },
        label: { type: "string", minLength: 1 },
        screenId: { type: "string", minLength: 1 },
        iconAssetId: { type: "string", nullable: true }
    }
};
export const screenSchema = {
    type: "object",
    additionalProperties: false,
    required: ["id", "name", "elements"],
    properties: {
        id: { type: "string", minLength: 1 },
        name: { type: "string", minLength: 1 },
        description: { type: "string", nullable: true },
        elements: {
            type: "array",
            minItems: 1,
            items: elementSchema
        },
        flows: {
            type: "array",
            nullable: true,
            items: flowSchema
        },
        assets: {
            type: "array",
            nullable: true,
            items: graphicAssetSchema
        },
        animations: {
            type: "array",
            nullable: true,
            items: animationSchema
        },
        submenus: {
            type: "array",
            nullable: true,
            items: submenuSchema
        }
    }
};
export const themeTokensSchema = {
    type: "object",
    additionalProperties: false,
    required: ["name", "colors", "typography", "animation"],
    properties: {
        name: { type: "string", minLength: 1 },
        colors: {
            type: "object",
            additionalProperties: false,
            required: [
                "displayBackground",
                "textPrimary",
                "textMuted",
                "textStrong",
                "value",
                "badgeBackground",
                "badgeBorder",
                "icon",
                "legend",
                "gridMinor",
                "gridMajor"
            ],
            properties: {
                displayBackground: { type: "string", pattern: colorPattern },
                textPrimary: { type: "string", pattern: colorPattern },
                textMuted: { type: "string", pattern: colorPattern },
                textStrong: { type: "string", pattern: colorPattern },
                value: { type: "string", pattern: colorPattern },
                badgeBackground: { type: "string", pattern: colorPattern },
                badgeBorder: { type: "string", pattern: colorPattern },
                icon: { type: "string", pattern: colorPattern },
                legend: { type: "string", pattern: colorPattern },
                gridMinor: { type: "string", pattern: colorPattern },
                gridMajor: { type: "string", pattern: colorPattern }
            }
        },
        typography: {
            type: "object",
            additionalProperties: false,
            required: ["base", "value", "badge"],
            properties: {
                base: { type: "integer", minimum: 1 },
                value: { type: "integer", minimum: 1 },
                badge: { type: "integer", minimum: 1 }
            }
        },
        animation: {
            type: "object",
            additionalProperties: false,
            required: ["easing"],
            properties: {
                easing: {
                    type: "string",
                    enum: ["linear", "ease-in", "ease-out", "ease-in-out", "cubic-bezier(0.4, 0, 0.2, 1)"]
                }
            }
        }
    }
};
const datasetSchemaDefinition = {
    type: "object",
    additionalProperties: false,
    required: ["screens", "theme"],
    properties: {
        screens: {
            type: "array",
            minItems: 1,
            items: screenSchema
        },
        theme: themeTokensSchema
    }
};
export const datasetSchema = datasetSchemaDefinition;
//# sourceMappingURL=schemaDefinitions.js.map