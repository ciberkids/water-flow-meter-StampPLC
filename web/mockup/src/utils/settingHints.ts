import type { FirmwareValueDefinition } from "../types/firmwareActions";

/**
 * The only thing this file needs of a screen: its elements' bindings.
 *
 * Structural rather than an imported screen type, because the dataset reaches the app as
 * `screens.json` and the audit tools read it with their own local interfaces. A narrow shape
 * lets every caller pass what it already has.
 */
interface BindingBearingScreen {
  elements: { binding?: string }[];
}

/**
 * The two strings a setting page derives from its own descriptor: the range hint and the
 * pending ("New") value.
 *
 * WHY THIS EXISTS. Both used to be single static strings, and the review that prompted this file
 * caught the consequence on eleven screens at once: every setting page showed `1 to 247` — the
 * Modbus ID range — including Baud Rate, Parity, Stop Bits, LED Pulse Volume and the sensor pages,
 * and every value editor showed `New 19200` including Modbus ID, whose domain stops at 247. A
 * single placeholder standing in for a per-screen fact reads as a real value and is wrong
 * everywhere but its one home screen.
 *
 * Both facts are DERIVED, per §7.2 of Display_Per_Screen_Spec: `min`/`max`/`unit`/`options` are
 * already in the manifest, which `manifest_gen` generates from `ui_settings_types.cpp`. So the
 * descriptor is the single home and this file is a second formatter over it — never a second copy.
 *
 * There was also a second sample table: the app resolved settings through `sampleValues.ts` while
 * `tools/audit/screen-svg.ts` carried its own `TYPICAL` map. They disagreed — the gallery drew
 * Modbus ID as `1`, the running app as `42` — so a mockup reviewed in one could not be trusted to
 * match the other. `sampleRawFor` is now the one home for "what value does this setting plausibly
 * hold", and both render paths format it through the same functions.
 */

/**
 * Widest listed option list, in characters, before it is summarised instead.
 *
 * The range hint sits at x = 2, so 39 characters would physically fit. The cap is well below that
 * because the hint shares its screens with nothing that wants the rest of the row, and because a
 * list long enough to need the whole row has stopped being scannable. The one list that exceeds it
 * is the eight baud rates, at 57 characters.
 */
export const kMaxListedWidth = 20;

/** Option labels for a setting that offers a fixed set, or null for a free numeric. */
function optionLabels(definition: FirmwareValueDefinition): string[] | null {
  if (definition.options && definition.options.length > 0) {
    return definition.options.map((option) => option.label);
  }
  // A boolean with no authored options still has exactly two, and the firmware renders them
  // through the same labels (`ui_settings_types.cpp` gives every boolean an Off/On pair).
  if (definition.type === "boolean") {
    return ["Off", "On"];
  }
  return null;
}

/**
 * The range line for a setting, per §7.2.
 *
 * Empty for anything that has no domain to state — a text setting, or a value that is not a
 * setting at all. An empty string is deliberate rather than a failure: the element then draws
 * nothing, which is what a page with no range to show should do.
 *
 * A long option list is SUMMARISED rather than listed. §7.2 first wrote that summary as
 * `8 rates: 1200..115200`, which needed a per-setting noun ("rates" is meaningless for parity) and
 * so would have put a display string into a generated descriptor. `1200..115200 (8)` needs no
 * noun, is five characters narrower, and reads as eight discrete choices spanning that span.
 */
export function rangeHintFor(definition: FirmwareValueDefinition | undefined): string {
  if (!definition || definition.category !== "setting") {
    return "";
  }
  if (definition.type === "string") {
    return "";
  }
  const labels = optionLabels(definition);
  if (labels) {
    const listed = labels.join(" / ");
    if (listed.length <= kMaxListedWidth) {
      return listed;
    }
    return `${labels[0]}..${labels[labels.length - 1]} (${labels.length})`;
  }
  if (definition.min === undefined || definition.max === undefined) {
    return "";
  }
  return definition.unit
    ? `${definition.min} to ${definition.max} ${definition.unit}`
    : `${definition.min} to ${definition.max}`;
}

/**
 * `formatSetting` (`ui_settings_types.cpp:164-188`), in TypeScript.
 *
 * Quoted so the mockup cannot render a string the device would not: an option renders as its
 * LABEL and not its stored number — baud stores 0..7 and shows `19200` — and a unit is appended to
 * either form when the descriptor carries one.
 */
export function formatSetting(definition: FirmwareValueDefinition, raw: number): string {
  const option = definition.options?.find((candidate) => candidate.value === raw);
  if (option) {
    return definition.unit ? `${option.label} ${definition.unit}` : option.label;
  }
  return definition.unit ? `${raw} ${definition.unit}` : `${raw}`;
}

/**
 * A plausible stored value per setting, as an integer, the way the device holds it.
 *
 * Realistic rather than extremal: a panel full of `65535` proves the layout holds but not that it
 * reads, and the worst case is already checked separately against each element's declared `worst`.
 * Only settings whose plausible value is not implied by their bounds are listed; everything else
 * falls back to the descriptor.
 */
const kSampleRaw: Record<string, number> = {
  "config.modbusSlaveId": 1,
  "config.baudRate": 4, // stores the list index; 4 is 19200
  "config.parity": 0, // None
  "config.stopBits": 1,
  "config.ledPulseVolume": 10,
  "config.ledPulsePeriod": 500,
  "config.sensor.connected": 1, // On
  "config.sensor.calibrationType": 0, // Formula
  "config.sensor.multiplier": 6,
  "config.sensor.adjust": -8,
  "config.sensor.pulsesPerLiter": 450,
  "config.sensor.maxFlow": 150,
  "config.mqtt.port": 1883,
  "config.mqtt.publishPeriod": 30,
  "config.mqtt.qos": 1,
  "config.mqtt.enabled": 1,
  "config.mqtt.haDiscovery": 1,
  "config.wifi.enabled": 1
};

/**
 * Plausible contents for the seven TEXT settings, which have no numeric domain to sample.
 *
 * The secrets are not listed: `formatSettingText` masks them on the device, so a mockup that showed
 * one would be showing something the panel never can. They resolve to `********` below.
 */
const kSampleText: Record<string, string> = {
  "config.wifi.ssid": "PlantFloor",
  "config.mqtt.host": "broker.plant.local",
  "config.mqtt.user": "meter",
  "config.mqtt.baseTopic": "water/meter",
  "config.mqtt.discoveryPrefix": "homeassistant"
};

/**
 * `formatSettingText` (`ui_settings_types.cpp:190`), in TypeScript.
 *
 * A secret reads as asterisks and a never-set field as `(not set)` — the device's own two special
 * cases, so a read-only network page shows what the panel would show and not a blank row.
 */
export function formatSettingText(definition: FirmwareValueDefinition): string {
  if (definition.writeOnly) {
    return "********";
  }
  return kSampleText[definition.id] ?? "(not set)";
}

/** The stored value this setting plausibly holds. */
export function sampleRawFor(definition: FirmwareValueDefinition): number {
  const listed = kSampleRaw[definition.id];
  if (listed !== undefined) {
    return listed;
  }
  const labels = definition.options;
  if (labels && labels.length > 0) {
    return labels[0].value;
  }
  if (definition.min !== undefined) {
    return definition.min;
  }
  return 0;
}

/**
 * The value an open editor has dialled up but not committed — deliberately NOT the saved one.
 *
 * An editor screen exists to show `New` against `Saved`, so a mockup in which they match hides
 * the only thing the screen is for. This steps one option along the list, or one `step` up the
 * numeric domain, clamped — and falls back DOWN when the sample already sits at the ceiling, so
 * the two never coincide.
 */
export function pendingRawFor(definition: FirmwareValueDefinition): number {
  const saved = sampleRawFor(definition);
  const options = definition.options;
  if (options && options.length > 1) {
    const index = options.findIndex((option) => option.value === saved);
    const next = options[(Math.max(index, 0) + 1) % options.length];
    return next.value;
  }
  return stepWithin(saved, definition.step && definition.step > 0 ? definition.step : 1, definition.min, definition.max);
}

/**
 * One step away from `saved` without leaving `[min, max]` — up if there is room, otherwise down.
 *
 * Exported so the down-step case can be tested for what it is. Reached through `pendingRawFor` it is
 * currently unreachable: `sampleRawFor` returns `min` for any unlisted setting, and from `min` the
 * `saved - step >= min` guard is false by construction. A test that went through `pendingRawFor`
 * could only ever assert the pinned case while claiming to cover the step down.
 *
 * Returns `saved` unchanged when the domain is a single point, because inventing a value outside it
 * would be worse than showing an editor with nothing to demonstrate.
 */
export function stepWithin(saved: number, step: number, min?: number, max?: number): number {
  if (max !== undefined && saved + step > max) {
    return min !== undefined && saved - step >= min ? saved - step : saved;
  }
  return saved + step;
}

/**
 * The setting a screen is about, or undefined for a screen that edits nothing.
 *
 * Found by ASKING THE MANIFEST which of the screen's bindings is a setting, rather than by
 * matching element ids. Element ids are a layout convention (`field-value` on a setting page,
 * `saved-value` on its editor) and a screen that renames one would silently lose its range hint;
 * the category is the fact that actually decides it.
 */
export function settingOfScreen(
  screen: BindingBearingScreen | undefined,
  definitionById: Map<string, FirmwareValueDefinition>
): FirmwareValueDefinition | undefined {
  if (!screen) {
    return undefined;
  }
  for (const element of screen.elements) {
    if (!element.binding) {
      continue;
    }
    const definition = definitionById.get(element.binding);
    if (definition?.category === "setting") {
      return definition;
    }
  }
  return undefined;
}
