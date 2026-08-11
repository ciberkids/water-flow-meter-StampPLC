import type { JSONSchemaType } from "ajv";
import type {
  ScreenDataset,
  ScreenDefinition,
  ScreenElement,
  ScreenEvent,
  ScreenFlow,
  ScreenGraphicAsset,
  ScreenAnimation,
  ScreenSubmenu,
  ButtonFlowTrigger,
  TimeoutFlowTrigger,
  DataFlowTrigger
} from "../src/types.js";
import type { ThemeTokens } from "../src/theme/types.js";

const colorPattern =
  "^#([0-9a-fA-F]{6})$|^rgba\\(\\s*([01]?\\d{1,2}|2[0-4]\\d|25[0-5])\\s*,\\s*([01]?\\d{1,2}|2[0-4]\\d|25[0-5])\\s*,\\s*([01]?\\d{1,2}|2[0-4]\\d|25[0-5])\\s*,\\s*(0(\\.\\d+)?|1(\\.0+)?)\\s*\\)$";

export const elementSchema: JSONSchemaType<ScreenElement> = {
  type: "object",
  additionalProperties: false,
  required: ["id", "kind", "x", "y"],
  properties: {
    id: { type: "string", minLength: 1 },
    kind: {
      type: "string",
      enum: ["text", "value", "badge", "box", "icon", "scrollbar"]
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
    binding: { type: "string", nullable: true },
    metadata: {
      type: "object",
      nullable: true,
      required: [],
      additionalProperties: {
        oneOf: [{ type: "string" }, { type: "number" }, { type: "boolean" }]
      }
    }
  }
};

export const screenEventSchema: JSONSchemaType<ScreenEvent> = {
  type: "object",
  additionalProperties: false,
  required: ["trigger"],
  properties: {
    trigger: { type: "string", minLength: 1 },
    actionId: { type: "string", nullable: true },
    targetScreenId: { type: "string", nullable: true }
  }
};

const buttonTriggerSchema: JSONSchemaType<ButtonFlowTrigger> = {
  type: "object",
  additionalProperties: false,
  required: ["type", "button"],
  properties: {
    type: { type: "string", const: "button" },
    button: { type: "string", enum: ["up", "down", "enter"] },
    gesture: { type: "string", enum: ["short", "long", "hold"], nullable: true }
  }
};

const timeoutTriggerSchema: JSONSchemaType<TimeoutFlowTrigger> = {
  type: "object",
  additionalProperties: false,
  required: ["type", "durationMs"],
  properties: {
    type: { type: "string", const: "timeout" },
    durationMs: { type: "integer", minimum: 1 },
    // "enter" = hold countdown (aborts on release); null/absent = auto timeout.
    holdButton: { type: "string", enum: ["up", "down", "enter"], nullable: true }
  }
};

const dataTriggerSchema: JSONSchemaType<DataFlowTrigger> = {
  type: "object",
  additionalProperties: false,
  required: ["type", "source", "condition"],
  properties: {
    type: { type: "string", const: "data" },
    source: { type: "string", minLength: 1 },
    condition: { type: "string", minLength: 1 }
  }
};

export const flowSchema: JSONSchemaType<ScreenFlow> = {
  type: "object",
  additionalProperties: false,
  required: ["id", "label", "trigger"],
  properties: {
    id: { type: "string", minLength: 1 },
    label: { type: "string", minLength: 1 },
    targetScreenId: { type: "string", minLength: 1, nullable: true },
    trigger: {
      oneOf: [
        buttonTriggerSchema as JSONSchemaType<ScreenFlow["trigger"]>,
        timeoutTriggerSchema as JSONSchemaType<ScreenFlow["trigger"]>,
        dataTriggerSchema as JSONSchemaType<ScreenFlow["trigger"]>
      ]
    } as unknown as JSONSchemaType<ScreenFlow["trigger"]>,
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

export const graphicAssetSchema: JSONSchemaType<ScreenGraphicAsset> = {
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
    },
    embeddedFrames: {
      type: "array",
      nullable: true,
      items: { type: "string", minLength: 1 }
    }
  }
};

export const animationKeyframeSchema: JSONSchemaType<ScreenAnimation["frames"][number]> = {
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

export const animationSchema: JSONSchemaType<ScreenAnimation> = {
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

export const submenuSchema: JSONSchemaType<ScreenSubmenu> = {
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

export const screenSchema: JSONSchemaType<ScreenDefinition> = {
  type: "object",
  additionalProperties: false,
  required: ["id", "name", "elements"],
  properties: {
    id: { type: "string", minLength: 1 },
    name: { type: "string", minLength: 1 },
    description: { type: "string", nullable: true },
    /**
     * When present, the screen is only part of its level while `binding` holds `equals`.
     *
     * The binding must be a SETTING with an unguarded editor — that is what keeps the completeness
     * rule decidable after R7.3 was relaxed for the calibration branch.
     */
    visibleWhen: {
      type: "object",
      nullable: true,
      additionalProperties: false,
      required: ["binding", "equals"],
      properties: {
        binding: { type: "string", minLength: 1 },
        equals: { type: "number" }
      }
    },
    elements: {
      type: "array",
      minItems: 0,
      items: elementSchema
    },
    events: {
      type: "array",
      nullable: true,
      items: screenEventSchema
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

export const themeTokensSchema: JSONSchemaType<ThemeTokens> = {
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
} as const;

export const datasetSchema: JSONSchemaType<ScreenDataset> =
  datasetSchemaDefinition as unknown as JSONSchemaType<ScreenDataset>;

// ── Firmware manifest schema ────────────────────────────────────────────────

export interface FirmwareActionParam {
  type: "string" | "number" | "boolean";
}

export interface FirmwareAction {
  id: string;
  label: string;
  description?: string;
  params?: Record<string, FirmwareActionParam>;
}

/**
 * What kind of thing a firmware value is. The design tool groups its palette by
 * this, and it is the difference between "place a reading" and "place a setting
 * the operator can edit".
 */
export type FirmwareValueCategory =
  /** Operator-editable setting with its own value editor screen. */
  | "setting"
  /** Live per-sensor reading (instant flow, status). */
  | "reading"
  /** Accumulated per-sensor total (cumulative, session, max). */
  | "accumulated"
  /** System-wide aggregate or diagnostic. */
  | "system"
  /** UI-supplied text with no register behind it (legends, countdown text). */
  | "derived";

/**
 * A value the firmware can supply to an element `binding`.
 *
 * This is the **catalogue** the design tool offers: a designer picks a value from
 * here rather than typing a binding string, so an element cannot reference
 * something the firmware does not expose. `register` is the Modbus holding
 * register backing it, where one exists — derived values (m³ from litres,
 * formatted legends, countdown text) have none.
 *
 * For `category: "setting"`, the editor behaviour is fully described here:
 * `min`/`max`/`step` for numerics, `options` for cycle lists. One declaration drives
 * the web simulator and the firmware editor alike, so they cannot drift.
 */
export interface FirmwareValue {
  id: string;
  type?: "string" | "number" | "boolean";
  category?: FirmwareValueCategory;
  unit?: string;
  register?: number;
  description?: string;
  readOnly?: boolean;
  /** Inclusive lower bound for a numeric setting. */
  min?: number;
  /** Inclusive upper bound for a numeric setting. */
  max?: number;
  /** Increment applied by a single short UP/DOWN press. */
  step?: number;
  /**
   * Option list for a cycle-list or boolean setting.
   *
   * Each option carries the value actually written to the register, not just a label. A
   * bare label list was ambiguous: `stopBits` offers "1" and "2" but stores 1 and 2, while
   * `baudRate` offers "1200".."115200" and stores 0..7. Position was not the value.
   */
  options?: { label: string; value: number }[];
  /**
   * Offset within a sensor's register block, for settings that exist once per sensor and
   * therefore have no single absolute address.
   */
  registerOffset?: number;
  /** Capacity in bytes of a string setting, excluding the terminator. */
  maxLength?: number;
  /**
   * A secret — the WiFi passphrase or MQTT password.
   *
   * Renders masked on the device and reads back as zeros over Modbus, so a design must not
   * bind it anywhere the full value would be displayed.
   */
  writeOnly?: boolean;
  /**
   * True when the value is scoped to the sensor implied by the current navigation
   * level rather than naming one. Lets one editor screen serve all 8 sensors.
   */
  perSensor?: boolean;
}

/**
 * A screen ID the firmware router resolves by name. If the dataset renames or
 * drops one of these, UiScreenRouter::findById returns nullptr and the display
 * goes blank with no other symptom — which is exactly what happened with the
 * stale "info-overview"/"configuration"/"countdown" defaults.
 */
export interface FirmwareScreen {
  id: string;
  /** What the firmware uses it for, e.g. "info-page-3", "countdown-overlay". */
  role?: string;
  description?: string;
}

export interface FirmwareManifest {
  version: string;
  actions: FirmwareAction[];
  /**
   * Optional so a manifest emitted by a firmware build that only knows about
   * actions still validates. Absent `values` makes every element binding
   * unresolvable, which the value-coverage check reports as a failure.
   */
  values?: FirmwareValue[];
  /** Screen IDs the firmware requires the dataset to define. */
  screens?: FirmwareScreen[];
}

export const firmwareActionParamSchema: JSONSchemaType<FirmwareActionParam> = {
  type: "object",
  additionalProperties: false,
  required: ["type"],
  properties: {
    type: { type: "string", enum: ["string", "number", "boolean"] }
  }
};

export const firmwareActionSchema: JSONSchemaType<FirmwareAction> = {
  type: "object",
  additionalProperties: false,
  required: ["id", "label"],
  properties: {
    id: { type: "string", minLength: 1 },
    label: { type: "string", minLength: 1 },
    description: { type: "string", nullable: true },
    params: {
      type: "object",
      nullable: true,
      required: [],
      additionalProperties: firmwareActionParamSchema
    }
  }
};

export const firmwareValueSchema: JSONSchemaType<FirmwareValue> = {
  type: "object",
  additionalProperties: false,
  required: ["id"],
  properties: {
    id: { type: "string", minLength: 1 },
    type: { type: "string", enum: ["string", "number", "boolean"], nullable: true },
    category: {
      type: "string",
      enum: ["setting", "reading", "accumulated", "system", "derived"],
      nullable: true
    },
    unit: { type: "string", nullable: true },
    register: { type: "integer", minimum: 0, nullable: true },
    description: { type: "string", nullable: true },
    readOnly: { type: "boolean", nullable: true },
    min: { type: "number", nullable: true },
    max: { type: "number", nullable: true },
    step: { type: "number", nullable: true },
    options: {
      type: "array",
      minItems: 1,
      nullable: true,
      items: {
        type: "object",
        additionalProperties: false,
        required: ["label", "value"],
        properties: {
          label: { type: "string", minLength: 1 },
          value: { type: "number" }
        }
      }
    },
    registerOffset: { type: "integer", minimum: 0, nullable: true },
    maxLength: { type: "integer", minimum: 1, nullable: true },
    writeOnly: { type: "boolean", nullable: true },
    perSensor: { type: "boolean", nullable: true }
  }
};

export const firmwareScreenSchema: JSONSchemaType<FirmwareScreen> = {
  type: "object",
  additionalProperties: false,
  required: ["id"],
  properties: {
    id: { type: "string", minLength: 1 },
    role: { type: "string", nullable: true },
    description: { type: "string", nullable: true }
  }
};

const firmwareManifestDefinition = {
  type: "object",
  additionalProperties: false,
  // No `generatedAt`: the manifest is generated from the firmware catalogues and committed,
  // then CI diffs it against a fresh generation. A timestamp would make every run a change.
  required: ["version", "actions"],
  properties: {
    version: { type: "string", minLength: 1 },
    actions: {
      type: "array",
      minItems: 0,
      items: firmwareActionSchema
    },
    values: {
      type: "array",
      minItems: 0,
      nullable: true,
      items: firmwareValueSchema
    },
    screens: {
      type: "array",
      minItems: 0,
      nullable: true,
      items: firmwareScreenSchema
    }
  }
} as const;

export const firmwareManifestSchema: JSONSchemaType<FirmwareManifest> =
  firmwareManifestDefinition as unknown as JSONSchemaType<FirmwareManifest>;
