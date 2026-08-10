import { describe, expect, it } from "vitest";
import fs from "node:fs";
import path from "node:path";
import type { FirmwareValueDefinition } from "../../types/firmwareActions";
import {
  formatSetting,
  formatSettingText,
  kMaxListedWidth,
  pendingRawFor,
  rangeHintFor,
  sampleRawFor,
  settingOfScreen,
  stepWithin
} from "../settingHints";

/**
 * These tests exist because a review of the rendered mockups found three defects that a green suite
 * of 75 tests had not noticed, all of the same shape: a per-screen fact served by one static string.
 *
 *   - eleven setting pages showed `1 to 247`, the Modbus ID range, Baud Rate and Parity among them
 *   - every value editor offered `New 19200`, including Modbus ID whose domain ends at 247
 *   - every numeric setting page drew `42`, the sample table's bare type fallback
 *
 * Nothing checked the text, so nothing failed. Each case below is one of those defects.
 */

const manifest = JSON.parse(
  fs.readFileSync(path.join(__dirname, "..", "..", "data", "actionManifest.json"), "utf-8")
) as { values: FirmwareValueDefinition[] };
const byId = new Map(manifest.values.map((v) => [v.id, v]));
const setting = (id: string): FirmwareValueDefinition => {
  const found = byId.get(id);
  if (!found) throw new Error(`manifest has no value "${id}"`);
  return found;
};

describe("rangeHintFor", () => {
  it("states a plain numeric domain", () => {
    expect(rangeHintFor(setting("config.modbusSlaveId"))).toBe("1 to 247");
  });

  it("appends the unit when the descriptor carries one", () => {
    expect(rangeHintFor(setting("config.sensor.maxFlow"))).toBe("0 to 65535 L/min");
  });

  it("lists a short option set", () => {
    expect(rangeHintFor(setting("config.parity"))).toBe("None / Even / Odd");
  });

  it("renders a boolean as its two labels", () => {
    expect(rangeHintFor(setting("config.sensor.connected"))).toBe("Off / On");
  });

  it("summarises an option list too long to show, rather than overflowing", () => {
    // Listed in full this is 57 characters — 342 px, 102 px past the panel. That overflow was the
    // original reason §7.2 made the hint derived instead of authored.
    expect(rangeHintFor(setting("config.baudRate"))).toBe("1200..115200 (8)");
  });

  it("says nothing for a text setting, which has no domain", () => {
    expect(rangeHintFor(setting("config.mqtt.host"))).toBe("");
  });

  it("says nothing for a value that is not a setting", () => {
    expect(rangeHintFor(setting("telemetry.totalFlowLpm"))).toBe("");
  });

  it("is the SAME hint on every screen only because it is derived per setting", () => {
    // The defect this file was written for: one static string served all of these.
    const distinct = new Set(
      ["config.modbusSlaveId", "config.baudRate", "config.parity", "config.stopBits"].map((id) =>
        rangeHintFor(setting(id))
      )
    );
    expect(distinct.size).toBe(4);
  });

  it("fits the panel for every setting in the catalogue", () => {
    // The hint sits at x = 2 with 6 px glyphs, so 39 characters is the physical limit.
    for (const value of manifest.values) {
      if (value.category !== "setting") continue;
      expect(2 + rangeHintFor(value).length * 6).toBeLessThanOrEqual(240);
    }
  });
});

describe("formatSetting", () => {
  it("renders an option as its label, not its stored number", () => {
    // Baud stores 0..7; showing "4" would be showing the register, not the rate.
    expect(formatSetting(setting("config.baudRate"), 4)).toBe("19200");
  });

  it("appends the unit to an option label", () => {
    expect(formatSetting(setting("config.ledPulseVolume"), 10)).toBe("10 L");
  });

  it("renders a plain numeric with its unit", () => {
    expect(formatSetting(setting("config.sensor.maxFlow"), 150)).toBe("150 L/min");
  });
});

describe("sampleRawFor", () => {
  it("gives every numeric setting a value inside its own domain", () => {
    // The `42` defect: a bare type fallback put a non-baud number on the Baud Rate page.
    for (const value of manifest.values) {
      if (value.category !== "setting" || value.type === "string") continue;
      const raw = sampleRawFor(value);
      if (value.options) {
        expect(value.options.map((o) => o.value)).toContain(raw);
      } else {
        expect(raw).toBeGreaterThanOrEqual(value.min ?? Number.NEGATIVE_INFINITY);
        expect(raw).toBeLessThanOrEqual(value.max ?? Number.POSITIVE_INFINITY);
      }
    }
  });
});

describe("pendingRawFor", () => {
  it("differs from the saved value, so an editor demonstrates something", () => {
    for (const value of manifest.values) {
      if (value.category !== "setting" || value.type === "string") continue;
      expect(pendingRawFor(value)).not.toBe(sampleRawFor(value));
    }
  });

  it("stays inside the domain it is stepping through", () => {
    // `New 19200` on Modbus ID was out of range by a factor of 78.
    for (const value of manifest.values) {
      if (value.category !== "setting" || value.type === "string") continue;
      const raw = pendingRawFor(value);
      if (value.options) {
        expect(value.options.map((o) => o.value)).toContain(raw);
      } else {
        expect(raw).toBeGreaterThanOrEqual(value.min ?? Number.NEGATIVE_INFINITY);
        expect(raw).toBeLessThanOrEqual(value.max ?? Number.POSITIVE_INFINITY);
      }
    }
  });

  it("wraps around an option list rather than running off the end", () => {
    // QoS offers 0 and 1 and the sample is 1, so the only distinct choice is to wrap to 0.
    expect(pendingRawFor(setting("config.mqtt.qos"))).toBe(0);
  });
});

describe("stepWithin", () => {
  it("steps up when the ceiling allows it", () => {
    expect(stepWithin(5, 1, 0, 10)).toBe(6);
  });

  it("steps DOWN when stepping up would pass max", () => {
    // The branch `pendingRawFor` cannot reach, because sampleRawFor starts at min. Tested here for
    // what it actually is rather than through a caller that can only produce the pinned case.
    expect(stepWithin(10, 1, 0, 10)).toBe(9);
    expect(stepWithin(10, 4, 0, 10)).toBe(6);
  });

  it("returns the value unchanged when the domain is a single point", () => {
    expect(stepWithin(7, 1, 7, 7)).toBe(7);
  });

  it("returns the value unchanged when neither direction fits the step", () => {
    // Room of 2 either side, step of 5: moving at all would leave the domain.
    expect(stepWithin(5, 5, 3, 7)).toBe(5);
  });

  it("steps up freely when no ceiling is declared", () => {
    expect(stepWithin(100, 1, undefined, undefined)).toBe(101);
  });
});

describe("formatSettingText", () => {
  it("masks a secret, exactly as the device does", () => {
    expect(formatSettingText(setting("config.mqtt.password"))).toBe("********");
    expect(formatSettingText(setting("config.wifi.psk"))).toBe("********");
  });

  it("shows a plausible value for a readable text setting", () => {
    expect(formatSettingText(setting("config.wifi.ssid"))).toBe("PlantFloor");
  });
});

describe("settingOfScreen", () => {
  it("finds the setting by asking the manifest, not by element id", () => {
    const screen = {
      elements: [
        { binding: "page.title" },
        { binding: "config.baudRate" },
        { binding: "config.editor.range" }
      ]
    };
    expect(settingOfScreen(screen, byId)?.id).toBe("config.baudRate");
  });

  it("returns undefined for a screen that shows no setting", () => {
    const screen = { elements: [{ binding: "telemetry.totalFlowLpm" }, {}] };
    expect(settingOfScreen(screen, byId)).toBeUndefined();
  });
});

describe("the mockup and the firmware agree on when to summarise", () => {
  it("uses the same threshold as formatSettingRange", () => {
    // A mockup that lists where the device summarises lies about the one thing this row is for. The
    // constants are declared in two languages, so this reads the firmware's own source for its value.
    const source = fs.readFileSync(
      path.join(
        __dirname,
        "..", "..", "..", "..", "..",
        "Water-Flow-Meter-PlatformIO", "src", "ui", "core", "ui_settings_types.cpp"
      ),
      "utf-8"
    );
    const match = /kMaxListedRangeWidth\s*=\s*(\d+)/.exec(source);
    expect(match, "firmware must declare kMaxListedRangeWidth").not.toBeNull();
    // Compare against the IMPORTED constant, not a literal. Hardcoding 20 here checked only the
    // firmware side: changing kMaxListedWidth in settingHints.ts — the side more likely to move,
    // since that is the file being tuned — left this test passing while the two silently diverged.
    expect(Number(match![1])).toBe(kMaxListedWidth);
  });
});
