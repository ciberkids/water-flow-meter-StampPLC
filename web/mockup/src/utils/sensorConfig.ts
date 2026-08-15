import type { FirmwareValueDefinition } from "../types/firmwareActions";

/**
 * Eight INDEPENDENT sensors, and the device's own rules for rendering them.
 *
 * The mockup used to collapse all eight channels into one shared config and drew every one of
 * them as connected-and-flowing, so the two states an operator most needs to recognise —
 * "channel 3 is not wired" and "channel 3 is enabled but not ready yet" — could not be seen at
 * all. This module is the pure half of the fix: the per-sensor table, the selected-sensor rule,
 * and the string formats. No React, no clock; the caller owns both.
 *
 * Every rule here mirrors a specific place in the firmware, cited inline. Two of those
 * mirrors are deliberately partial, and both are about disconnecting a channel:
 *
 *  - Per TICK, a disconnected channel keeps its accumulated totals: the engine's `else` arm
 *    zeroes `instantFlow_L_s` and touches nothing else
 *    (sensors/sensor_state_engine.cpp:53-60). `advanceSensorTick` mirrors that exactly.
 *  - The disconnect EVENT is a different story. Toggling `config.sensor.connected` off writes
 *    the connected-sensors bitmap (ui/core/ui_settings.cpp:239-247), and that write assigns
 *    `deps_.sensors[i] = SensorData{}` — clearing the totals and `isReady`
 *    (modbus/modbus_manager.cpp:110-114). This module does NOT model that erasure: the point of
 *    the simulator is to look at a disconnected sensor's frozen row, which an erasing toggle
 *    would leave empty. `setSensor` therefore leaves the totals alone; a caller that wants the
 *    device's erase-on-disconnect behaviour has to ask for it explicitly.
 */

/**
 * `plc::kNumSensors` — modbus/register_map.h:8.
 *
 * Not spelled 8 anywhere below, because the firmware has exactly one declaration of it and a
 * second one here is a second thing to forget.
 */
export const kSensorCount = 8;

/**
 * One channel's simulated state: the three stored configuration fields, the two state bits,
 * the four accumulated readings, and the one diagnostic bit.
 *
 * Field-for-field this is `SensorCharacteristics` + `SensorData` (modbus/sensor_types.h:5-27),
 * minus `pulseCount` — pulses arrive as a tick argument here rather than being latched by an
 * ISR. `connected` and `ready` are BOTH modelled because the device renders three different
 * things from them: a disconnected channel, an enabled-but-not-ready channel, and a working
 * one (ui/core/ui_bindings.cpp:266-279). Boot reaches the middle state for every channel:
 * `isReady` is forced false while `inUse` is restored from the saved bitmap
 * (firmware.cpp:687-688).
 */
export interface SimulatedSensor {
  /** 1-based, the number the device prints (`sensorIndex + 1`, ui/core/ui_bindings.cpp:263). */
  number: number;
  /** `SensorData::inUse` (modbus/sensor_types.h:20) — bit n of the connected-sensors bitmap. */
  connected: boolean;
  /** `SensorData::isReady` (modbus/sensor_types.h:21). */
  ready: boolean;
  /** `SensorCharacteristics::q_max` (modbus/sensor_types.h:6), L/min. Integer on the device. */
  qMaxLpm: number;
  /** `SensorCharacteristics::f_multiplier` (modbus/sensor_types.h:7). Integer on the device. */
  multiplier: number;
  /** `SensorCharacteristics::adjust` (modbus/sensor_types.h:8). Integer on the device. */
  adjust: number;
  /**
   * `SensorCharacteristics::calibration` — 0 Formula, 1 Pulses/L.
   *
   * Which of the two forms the meter's datasheet uses. They are not interchangeable: a K of 450
   * needs a multiplier of 7.5, which an integer field cannot hold.
   */
  calibration: number;
  /** `SensorCharacteristics::pulses_per_litre`. Exact, used only when `calibration` is 1. */
  pulsesPerLitre: number;
  /**
   * `SensorData::instantFlow_L_min` — LITRES PER MINUTE (§2a).
   *
   * The field was `instantFlowLpm` and the device stored L/s. §2a moved storage to L/min, which is
   * the unit of the meter's datasheet, of `q_max`, of the Nyquist limit, of MQTT and of the panel —
   * so the conversions this used to need on every one of those paths are gone.
   */
  instantFlowLpm: number;
  /** `SensorData::cumulativeLiters` (modbus/sensor_types.h:24). */
  cumulativeLiters: number;
  /** `SensorData::sessionLiters` (modbus/sensor_types.h:25). */
  sessionLiters: number;
  /** `SensorData::maxFlowSinceReset`, in L/min like the reading it tracks. */
  maxFlowLpm: number;
  /**
   * Bit n of `REG_UNDERSAMPLING_FLAGS` (modbus/register_map.h:17).
   *
   * A DISCONNECTED channel can never carry it: `evaluateSensorDiagnostics` skips
   * `!inUse` before it looks at the configuration (modbus/modbus_manager.cpp:387-390). Every
   * producer in this module clears the bit when `connected` is false, so the combination is
   * unreachable through the API.
   */
  undersampling: boolean;
}

/** What one call to the state engine consumes: a pulse count and the interval it spans. */
export interface SensorTickInput {
  /** Pulses counted during the interval — the engine's `sensor.pulseCount`. */
  pulses: number;
  /**
   * Interval length. The engine takes a DURATION, not a clock reading
   * (`SensorStateEngine::update(float elapsedSeconds)`, sensors/sensor_state_engine.cpp:7),
   * which is also why nothing here reads a clock: the caller subtracts its own two timestamps.
   */
  elapsedMs: number;
}

/**
 * A default row that is READY and flowing, not the boot state.
 *
 * `q_max` and `f_multiplier` are non-zero for two reasons the firmware states: the engine
 * refuses to compute flow at all when `f_multiplier == 0`
 * (sensors/sensor_state_engine.cpp:26), and `configIsValid` — the predicate that decides
 * `isReady` after a config write (modbus/modbus_manager.cpp:287) — requires both to be
 * non-zero (modbus/modbus_manager.cpp:11-18). 150 L/min sits above the 140.4 L/min that
 * 2.34 L/s works out to, so the engine's `q_max` clamp does not cut the default flow
 * (sensors/sensor_state_engine.cpp:32-34).
 *
 * The readings are the ones the old shared sample strings showed (see sampleValues.ts), so
 * wiring this table in does not silently change what the panel draws for a working device.
 */
const kDefaultSensor: Omit<SimulatedSensor, "number"> = {
  connected: true,
  ready: true,
  qMaxLpm: 150,
  multiplier: 10,
  adjust: 0,
  calibration: 0,
  pulsesPerLitre: 450,
  instantFlowLpm: 140.4,  // was 2.34 L/s
  cumulativeLiters: 123.45,
  sessionLiters: 123.45,
  maxFlowLpm: 140.4,
  undersampling: false
};

/**
 * Forces the states the device cannot be in.
 *
 * Applied by every producer, because TypeScript cannot stop a caller writing
 * `{ connected: false, undersampling: true }` by hand.
 *
 *  - A channel that is not flowing reads 0 L/s: the engine assigns
 *    `instantFlow_L_s = 0.0f` both for a disabled channel (sensor_state_engine.cpp:54) and
 *    for an enabled one that is not ready or has no multiplier (sensor_state_engine.cpp:44-46).
 *  - A disconnected channel carries no undersampling flag (modbus_manager.cpp:387-390).
 *
 * Totals are never touched here — see the file header for why the disconnect event's erasure
 * is not mirrored.
 */
function normalizeSensor(sensor: SimulatedSensor): SimulatedSensor {
  // Which field has to be non-zero depends on the calibration form, exactly as `configIsValid`
  // branches (modbus/sensor_types.h). Testing the multiplier on a pulses-per-litre channel would
  // report a correctly configured channel as unable to produce a reading.
  const calibrated = sensor.calibration === 1 ? sensor.pulsesPerLitre !== 0 : sensor.multiplier !== 0;
  const flowing = sensor.connected && sensor.ready && calibrated;
  return {
    ...sensor,
    instantFlowLpm: flowing ? sensor.instantFlowLpm : 0,
    undersampling: sensor.connected ? sensor.undersampling : false
  };
}

/** Eight rows, numbered 1..8 the way the device labels them. */
export function createSensorTable(): SimulatedSensor[] {
  const table: SimulatedSensor[] = [];
  for (let number = 1; number <= kSensorCount; number += 1) {
    table.push(normalizeSensor({ ...kDefaultSensor, number }));
  }
  return table;
}

/**
 * The row for a 1-based sensor number, or undefined when there is none (0 included).
 *
 * Matched on the `number` FIELD, not on position. It was positional (`table[sensorNumber - 1]`, bounded
 * by `table.length`) while `setSensor` below matched on the field — two lookup rules for one fact, which
 * disagreed the moment a caller held anything other than a full eight-row table in position order.
 */
export function sensorAt(
  table: readonly SimulatedSensor[],
  sensorNumber: number
): SimulatedSensor | undefined {
  if (!Number.isInteger(sensorNumber) || sensorNumber < 1 || sensorNumber > kSensorCount) {
    return undefined;
  }
  return table.find((sensor) => sensor.number === sensorNumber);
}

/**
 * A new table with one row patched, so React state updates stay a one-liner.
 *
 * The patched row is normalized, which is what makes "disconnected with a warning"
 * unreachable no matter what the panel sends.
 */
export function setSensor(
  table: readonly SimulatedSensor[],
  sensorNumber: number,
  patch: Partial<Omit<SimulatedSensor, "number">>
): SimulatedSensor[] {
  return table.map((sensor) =>
    sensor.number === sensorNumber ? normalizeSensor({ ...sensor, ...patch }) : sensor
  );
}

/** True when this channel could carry an undersampling flag at all — i.e. it is connected. */
export function mayUndersample(sensor: SimulatedSensor): boolean {
  // `evaluateSensorDiagnostics` tests `if (!deps_.sensors[i].inUse) continue;` BEFORE it reads
  // the configuration or the Nyquist limit (modbus/modbus_manager.cpp:387-390). The limit
  // itself needs the polling rate, which is not per-sensor state and so is not modelled here.
  return sensor.connected;
}

/** The 1-based numbers of the flagged channels, in ascending order. */
export function warningSensorNumbers(table: readonly SimulatedSensor[]): number[] {
  return table.filter((sensor) => mayUndersample(sensor) && sensor.undersampling).map((s) => s.number);
}

/**
 * The 1-based numbers of the channels that are IN USE and have no calibration — the `SET?` rows.
 *
 * Mirrors the count `UiController::update` keeps beside its warning loop, which reads
 * `enabled && !ready` off the snapshot it has just built (ui/core/ui_controller.cpp). IN USE is part of
 * the definition and not a filter on top: a disconnected channel is ABSENT, not uncalibrated, and
 * counting all eight would report eight problems on a device with one sensor wired.
 *
 * `ready` IS THE PREDICATE, deliberately — not a `configIsValid` recomputed here. The row beside the
 * summary renders `!connected ? "--" : ready ? "OK" : "SET?"` from that same boolean, so deriving the
 * count from the configuration instead would let a row read `OK` while the summary said "1 channel not
 * calibrated" on the same screen. The device cannot disagree with itself there because it derives both
 * from one projection; this table's `ready` is a stored boolean, so agreeing with the row is the closest
 * available equivalent.
 *
 * THE DIVERGENCE THAT COMES WITH THAT, inherited from `resetCalibration` which records it too: nothing
 * recomputes `ready` when the configuration changes, so writing `qMaxLpm` to 0 through the Values panel
 * leaves the row saying `OK` and this count saying the channel is calibrated. The device would say
 * `SET?` for both. Fixing it means deriving `ready` in `normalizeSensor`, which is a change to what
 * every producer in this file means by the field, and is left for its own round rather than smuggled in
 * behind a summary line.
 */
export function uncalibratedSensorNumbers(table: readonly SimulatedSensor[]): number[] {
  return table.filter((sensor) => sensor.connected && !sensor.ready).map((s) => s.number);
}

/**
 * `telemetry.status` — the summary page's one-line verdict (ui/core/ui_bindings.cpp).
 *
 * Five states in the firmware's own order. It used to have two, both fed by the undersampling flags
 * alone, so a device whose channels all sat at `SET?` reported "All sensors ready" — the lie this
 * mirrors the fix for:
 *
 *   nothing in use       "No channels in use"
 *   both kinds present   "3 channels not calibrated | 2 warnings"
 *   commissioning gap    "3 channels not calibrated"
 *   sampling fault       "2 warnings"
 *   neither              "All sensors ready"
 *
 * The two counts stay DISTINCT: an uncalibrated channel is a channel nobody has set up, an
 * under-sampling one is a reading that is wrong, and one number covering both tells an operator
 * neither. Uncalibrated leads because a device nobody has finished commissioning is the more urgent
 * fact. The phrase is the same in both states that carry it — 38 characters at worst, inside the 40 a
 * 6 px row holds, so there is no reason for adjacent states of one row to name the fact two ways.
 */
export function statusSummaryText(table: readonly SimulatedSensor[]): string {
  const inUse = table.filter((sensor) => sensor.connected).length;
  if (inUse === 0) {
    // FIRST here and LAST in warningSummaryText below, mirroring each side's own implementation: the
    // firmware answers this binding in `resolveTelemetryBinding`, which tests `connectedBitmap == 0`
    // before anything else, while the summary string is composed in `UiController::update` after both
    // counts. The orders cannot disagree — no channel in use forces both counts to zero — so each
    // mirror follows the code it mirrors rather than a shared order neither side has.
    //
    // The device reads this off `connectedBitmap == 0`, which comes out of NVS with a default of 0, so
    // it is the state a device ships in rather than an edge case.
    return "No channels in use";
  }
  const uncalibrated = uncalibratedSensorNumbers(table).length;
  const warnings = warningSensorNumbers(table).length;
  if (uncalibrated > 0 && warnings > 0) {
    return `${uncalibrated} channel${uncalibrated === 1 ? "" : "s"} not calibrated | ${warnings} warning${warnings === 1 ? "" : "s"}`;
  }
  if (uncalibrated > 0) {
    return `${uncalibrated} channel${uncalibrated === 1 ? "" : "s"} not calibrated`;
  }
  if (warnings > 0) {
    return `${warnings} warning${warnings === 1 ? "" : "s"}`;
  }
  return "All sensors ready";
}

/**
 * `legend.warning`, and the text of the firmware's warning banner — one string, two consumers.
 *
 * Composed in `UiController::update` rather than in the resolver, so the banner and the legend row
 * cannot disagree; this mirrors the same precedence with the same wording.
 *
 * The sampling case NAMES the channels, as the device does, but only when it is alone: naming both sets
 * needs more than the 37 characters the banner has at x=16 with 6 px glyphs, so when both kinds are
 * present the list is traded for a count — and so is the word "channels", because "8 channels not
 * calibrated, 8 undersampling" is 42. This is the one line that drops it; `statusSummaryText` keeps it
 * in every state. Channel identity is not lost either way — the flagged rows are drawn in the warning
 * colour and an uncalibrated row says `SET?` itself.
 */
export function warningSummaryText(table: readonly SimulatedSensor[]): string {
  const uncalibrated = uncalibratedSensorNumbers(table).length;
  const flagged = warningSensorNumbers(table);
  if (uncalibrated > 0 && flagged.length > 0) {
    return `${uncalibrated} not calibrated, ${flagged.length} undersampling`;
  }
  if (uncalibrated > 0) {
    return `${uncalibrated} channel${uncalibrated === 1 ? "" : "s"} not calibrated`;
  }
  if (flagged.length > 0) {
    return `Sampling warning on sensors ${flagged.join(", ")}`;
  }
  if (table.every((sensor) => !sensor.connected)) {
    return "No channels in use";
  }
  return "All sensors nominal";
}

/**
 * `config-sensor-<n>` in the ANCESTOR chain fixes which sensor the level below applies to.
 *
 * Mirrors `UiNavigator`: `descend()` reads the index off the screen being LEFT
 * (ui/core/ui_navigator.cpp:80-82), so the index belongs to the descendants of a sensor list
 * entry, not to the entry itself — which is why the current screen id is matched only for the
 * shape check below and never contributes an index. That costs nothing: `config-sensor-3`'s own
 * elements bind `sensor.3.status`, an absolute id, and no `config.sensor.*` id (screens.json).
 *
 * `sensorIndexFromId` accepts a SINGLE digit '1'..'8' with nothing after it
 * (ui/core/ui_navigator.cpp:19-23), so `config-sensor-back` — a real screen in the dataset —
 * and `config-sensor-12` are both "not a sensor". Returns 0 for "none", the same sentinel the
 * navigator uses (`sensorIndex_ = 0` on reset, escape and return to depth 0,
 * ui/core/ui_navigator.cpp:62, 99, 107).
 *
 * `ancestorIds` is expected root-first; the nearest match wins. Order is academic in the real
 * dataset, where exactly one level of the tree carries these ids.
 *
 * `screenId` is part of the signature and deliberately not read: see above — the level a
 * sensor list entry SITS at carries no index, only the levels below it do.
 */
export function sensorIndexForScreen(screenId: string, ancestorIds: readonly string[]): number {
  for (let i = ancestorIds.length - 1; i >= 0; i -= 1) {
    const index = sensorIndexFromId(ancestorIds[i]);
    if (index !== 0) {
      return index;
    }
  }
  return 0;
}

/** `config-sensor-<n>` -> n, 0 for anything else. ui/core/ui_navigator.cpp:10-24. */
function sensorIndexFromId(screenId: string): number {
  const match = /^config-sensor-(\d)$/.exec(screenId);
  if (!match) {
    return 0;
  }
  const index = Number(match[1]);
  return index >= 1 && index <= kSensorCount ? index : 0;
}

/**
 * True for the manifest entries that are per-sensor SETTINGS.
 *
 * `perSensor` alone over-selects. Six entries in actionManifest.json carry it, and two are
 * category `derived`, `readOnly` — `config.sensor.nyquistWarning`, which the firmware
 * catalogue sources from `ValueSource::UiState` and describes as the validation prompt for the
 * current sensor (ui/core/ui_value_catalogue.cpp:88-89), and
 * `config.sensor.undersamplingFlag`, which is read out of the diagnostics register
 * (ui/core/ui_bindings.cpp:403-409). Neither is stored per sensor and neither is writable.
 * The four that survive are exactly the four `perSensor` descriptors in the firmware's setting
 * table (ui/core/ui_settings_types.cpp:48-56).
 */
export function isPerSensorSetting(manifestValue: FirmwareValueDefinition | undefined): boolean {
  return manifestValue?.perSensor === true && manifestValue.category === "setting";
}

/**
 * `%7.2f` — two decimals, right-aligned in a field of seven, never truncated.
 *
 * Seven, not six (§4.3): under §2a a single channel can reach 9999.99 L/m, and `%6.2f` cannot hold
 * it. A printf field width is a MINIMUM, so a channel clamped to q_max = 65535 still renders all
 * eight characters of `65535.00` rather than being cut — which is why the declared worst cases come
 * from the physical bound and not from the format string.
 */
function formatFixed7(value: number): string {
  return value.toFixed(2).padStart(7, " ");
}

type SettingField = "connected" | "multiplier" | "adjust" | "qMaxLpm" | "calibration" | "pulsesPerLitre";

interface PerSensorSettingDescriptor {
  field: SettingField;
  /** `SettingDescriptor::unit`; only maxFlow has one. */
  unit?: string;
  /** `SettingDescriptor::options`; only the boolean has them. */
  options?: { label: string; value: number }[];
}

/**
 * The SIX per-sensor settings, with the unit and option labels the device formats them with.
 *
 * Taken from the firmware's setting table, not from the manifest: these are the strings
 * `formatSetting` prints (ui/core/ui_settings_types.cpp, and `kBoolOptions` =
 * {"Off", 0}, {"On", 1} there too).
 *
 * Calibration type and pulses per litre joined the four when the calibration branch landed. A
 * unit test enumerates this against the manifest's `perSensor` descriptors, which is what caught
 * them missing here — without it the two new rows would have silently fallen through to the
 * generic sample path and stopped tracking the simulated sensor table.
 */
const kPerSensorSettings: Record<string, PerSensorSettingDescriptor> = {
  "config.sensor.connected": {
    field: "connected",
    options: [
      { label: "Off", value: 0 },
      { label: "On", value: 1 }
    ]
  },
  "config.sensor.multiplier": { field: "multiplier" },
  "config.sensor.adjust": { field: "adjust" },
  "config.sensor.maxFlow": { field: "qMaxLpm", unit: "L/min" },
  "config.sensor.calibrationType": {
    field: "calibration",
    options: [
      { label: "Formula", value: 0 },
      { label: "Pulses/L", value: 1 }
    ]
  },
  "config.sensor.pulsesPerLiter": { field: "pulsesPerLitre", unit: "p/L" }
};

/**
 * The integer a per-sensor setting currently holds, for the sensor selected.
 *
 * The read counterpart of `writeSensorSetting`, and descriptor-driven for the same reason: an editor
 * has to start from the value in force, and a hard-coded field chain would start the two newest
 * settings from nothing.
 */
export function readSensorSettingRaw(
  table: readonly SimulatedSensor[],
  sensorNumber: number,
  definition: FirmwareValueDefinition
): number {
  const descriptor = kPerSensorSettings[definition.id];
  if (!descriptor) {
    return 0;
  }
  return readSettingValue(descriptor, table, sensorNumber);
}

/**
 * Parses a typed string into the integer the device would store, and writes it.
 *
 * DESCRIPTOR-DRIVEN, deliberately. This replaced a hard-coded chain in App.tsx that named four
 * bindings and dropped anything else — and whose own comment predicted the failure: "a fifth entry
 * in the manifest would otherwise silently land in qMaxLpm". The calibration branch added a fifth
 * and a sixth, so setting Calibration to Pulses/L in the values panel did nothing at all, silently,
 * while the input went on showing the typed text.
 *
 * An OPTION is matched by its label first, because that is what the panel displays: the row reads
 * `Pulses/L`, so `Pulses/L` is what a person types back. Matching only the stored number would make
 * the field unwritable by anyone reading it.
 *
 * Returns the table unchanged when the binding is not a per-sensor setting, when no sensor is
 * selected, or when the text parses to nothing — a write that cannot be honoured must not guess.
 */
export function writeSensorSetting(
  table: readonly SimulatedSensor[],
  sensorNumber: number,
  definition: FirmwareValueDefinition | undefined,
  text: string
): SimulatedSensor[] {
  const base = [...table];
  if (!definition || !isPerSensorSetting(definition)) {
    return base;
  }
  const descriptor = kPerSensorSettings[definition.id];
  if (!descriptor || !sensorAt(table, sensorNumber)) {
    return base;
  }

  const trimmed = text.trim();
  let raw: number | undefined;

  const options = definition.options ?? descriptor.options;
  if (options && options.length > 0) {
    const byLabel = options.find((o) => o.label.toLowerCase() === trimmed.toLowerCase());
    if (byLabel) {
      raw = byLabel.value;
    } else {
      const numeric = Number.parseInt(trimmed, 10);
      raw = options.find((o) => o.value === numeric)?.value;
    }
  } else {
    const numeric = Number.parseInt(trimmed, 10);
    if (Number.isFinite(numeric)) {
      // Clamped to the DESCRIPTOR's domain, not the storage type's. The panel must not hold a value
      // the device's own editor would refuse — a multiplier of 0 is an int16 but not a legal
      // multiplier, and it would leave the channel unable to produce a reading.
      const min = definition.min ?? Number.NEGATIVE_INFINITY;
      const max = definition.max ?? Number.POSITIVE_INFINITY;
      raw = Math.min(Math.max(numeric, min), max);
    }
  }
  if (raw === undefined || !Number.isFinite(raw)) {
    return base;
  }

  if (descriptor.field === "connected") {
    return setSensor(table, sensorNumber, { connected: raw !== 0 });
  }
  return setSensor(table, sensorNumber, { [descriptor.field]: raw });
}

/** `formatSetting` — option label if one matches the value, else the integer; unit appended. */
function formatSettingValue(descriptor: PerSensorSettingDescriptor, value: number): string {
  // ui/core/ui_settings_types.cpp:164-188. `%ld` of an int32_t, so no decimals: every stored
  // field is an integer on the device (modbus/sensor_types.h:6-8).
  const option = descriptor.options?.find((candidate) => candidate.value === value);
  const text = option ? option.label : `${Math.trunc(value)}`;
  return descriptor.unit ? `${text} ${descriptor.unit}` : text;
}

/** The int32 the device would read for a setting, given the selected sensor. */
function readSettingValue(
  descriptor: PerSensorSettingDescriptor,
  table: readonly SimulatedSensor[],
  sensorIndex: number
): number {
  // `sensorSlot` maps a 0 index (and anything past the count) to `sensorCount`, and
  // `readSetting` then returns 0 rather than reading a row — so with no sensor selected the
  // numeric settings render "0" and the boolean renders its 0 label
  // (ui/core/ui_settings.cpp:13-18 and 118-131).
  const sensor = sensorAt(table, sensorIndex);
  if (!sensor) {
    return 0;
  }
  if (descriptor.field === "connected") {
    return sensor.connected ? 1 : 0;
  }
  return sensor[descriptor.field];
}

/** `sensor.<n>.<metric>` -> [1-based number, metric], or undefined when it is not one. */
function parseSensorMetricBinding(binding: string): [number, string] | undefined {
  // Mirrors `parseSensorBinding` (ui/core/ui_bindings.cpp:49-84): digits only, 1..kNumSensors,
  // and a MISSING suffix is accepted — the metric is then empty, which only the disconnected
  // arm below can answer.
  const match = /^sensor\.(\d+)(?:\.([\s\S]*))?$/.exec(binding);
  if (!match) {
    return undefined;
  }
  const number = Number(match[1]);
  if (!Number.isInteger(number) || number < 1 || number > kSensorCount) {
    return undefined;
  }
  return [number, match[2] ?? ""];
}

/**
 * The panel's flow unit — a DISPLAY choice, applied at render and nowhere else.
 *
 * Mirrors `units::flowFromLpm`. Storage is L/min (§2a) and every wire surface keeps it, so this
 * converts on the way to the glyphs and never on the way to a register. One function, because
 * otherwise each of the flow readings would carry its own factor and they would drift the way the
 * `/ 1000` volume conversions did.
 */
export type FlowUnit = 0 | 1 | 2;

export function flowFromLpm(litresPerMinute: number, unit: FlowUnit): number {
  switch (unit) {
    case 1:
      return litresPerMinute / 60;
    case 2:
      // 1 m3/h is 1000 L per 60 min, so L/min * 60 / 1000.
      return litresPerMinute * 0.06;
    default:
      return litresPerMinute;
  }
}

/** What the header prints. The same strings as kFlowUnitOptions' labels. */
export function flowUnitLabel(unit: FlowUnit): string {
  return unit === 1 ? "L/s" : unit === 2 ? "m3/h" : "L/m";
}

/** Which metrics are a FLOW, and so follow the display unit. Volumes do not. */
const kFlowMetrics = new Set(["instantFlow", "maxFlowSinceReset"]);

/**
 * The metrics `resolveSensorBinding` knows. NO UNITS — the unit lives in the header (§4.3).
 *
 * This was a unit table, so P1 rendered `1: 2.34 L/s` under a title reading `Instant Flow (L/m)`:
 * the row contradicting its own header, in the wrong unit, with the L/s number. §4.3 rejected per-row
 * units because they are impossible on the volume pages — `8: 99999999.99 m^3` is 18 characters and
 * overflows two columns by 19 px — and put the unit in the title, two rows above and always visible.
 *
 * The set is kept as a list so an unknown metric still fails loudly rather than rendering a plausible
 * wrong number.
 */
const kKnownMetrics = new Set([
  "instantFlow",
  "cumulativeLiters",
  "cumulativeM3",
  "sessionLiters",
  "sessionM3",
  "maxFlowSinceReset",
  "pulsesPerLitre"
]);

function metricValue(sensor: SimulatedSensor, metric: string): number | undefined {
  // ui/core/ui_bindings.cpp:281-304. The m^3 pair is the litre reading over 1000; there is no
  // separately stored cubic-metre total.
  switch (metric) {
    case "instantFlow":
      return sensor.instantFlowLpm;
    case "cumulativeLiters":
      return sensor.cumulativeLiters;
    case "cumulativeM3":
      return sensor.cumulativeLiters / 1000;
    case "sessionLiters":
      return sensor.sessionLiters;
    case "sessionM3":
      return sensor.sessionLiters / 1000;
    case "maxFlowSinceReset":
      return sensor.maxFlowLpm;
    /**
     * SET on a pulses-calibrated channel, CALCULATED on a formula-calibrated one.
     *
     * `F = m*Q` with Q in L/min means K = 60*m, since F = K*Q/60. The offset is not folded in: `a`
     * shifts the line rather than scaling it, so no single K expresses a formula with a non-zero
     * offset — S5 reports the offset on its own row instead.
     */
    case "pulsesPerLitre":
      return sensor.calibration === 1 ? sensor.pulsesPerLitre : sensor.multiplier * 60;
    default:
      return undefined;
  }
}

/**
 * What the device draws for `sensor.<n>.<metric>`, in the order the resolver tests it.
 *
 * Mirrors `UiBindingResolver::resolveSensorBinding`. Four outcomes, and a metric and a status differ
 * in both respects: the status omits the sensor number, and it answers even when no reading exists.
 *
 *   status, any state      "--" | "SET?" | "OK"
 *   metric, disconnected   `%u: --`     -> "3: --"
 *   metric, uncalibrated   `%u: SET?`   -> "3: SET?"
 *   metric, ready          `%u: %7.2f` plus " MAX" on the peak page
 *
 * `SET?` not `WAIT` (§4.4): `WAIT` implies warming up, when the real condition is that the channel
 * has no valid calibration and needs an operator. Neither the firmware nor this had followed the
 * spec — only the gallery's hand-written sample table did, so all three disagreed about what a
 * just-wired channel says.
 *
 * `--` means NOT IN SERVICE, never "not detected". No presence detection exists and none is possible:
 * an idle passive pulse sensor is indistinguishable from one whose wire fell off.
 *
 * `%7.2f`, not `%6.2f` — a channel can reach 9999.99 L/m, which six characters cannot hold — and no
 * trailing space when there is no marker, so P1's row is `1: 65535.00` exactly as declared.
 */
function resolveSensorMetric(sensor: SimulatedSensor, metric: string, flowUnit: FlowUnit = 0): string | undefined {
  if (metric === "status") {
    return !sensor.connected ? "--" : sensor.ready ? "OK" : "SET?";
  }
  // No channel-number prefix: it is a row LABEL, which the telemetry pages now carry themselves. It
  // was redundant on the per-sensor config pages, whose header already names the channel.
  if (!sensor.connected) {
    return "--";
  }
  if (!sensor.ready) {
    return "SET?";
  }
  if (!kKnownMetrics.has(metric)) {
    // The firmware returns false rather than rendering a plausible-looking wrong number, so does this.
    return undefined;
  }
  const stored = metricValue(sensor, metric);
  if (stored === undefined) {
    return undefined;
  }
  // Flows follow the display unit; volumes and the pulse count do not.
  const value = kFlowMetrics.has(metric) ? flowFromLpm(stored, flowUnit) : stored;
  /**
   * `MAX` marks a peak that reached the channel's OWN q_max ceiling — the whole point of §5a's page.
   * A channel pinned at its ceiling is under-dimensioned for the pipe it is on, and the number alone
   * cannot say so, because a legitimate peak can sit just below the same value.
   *
   * Compared in L/min against q_max, which is stored in L/min. Comparing the stored L/s peak against
   * it would flag nothing, ever.
   */
  // Compared in L/MIN against q_max, which is stored in L/min — BEFORE the display conversion.
  // Testing the converted value would flag nothing on a channel being shown in m3/h.
  const marker =
    metric === "maxFlowSinceReset" && sensor.qMaxLpm > 0 && stored >= sensor.qMaxLpm ? " MAX" : "";
  // Pulses per litre is a COUNT: `360.00 p/L` invites the reader to wonder what the hundredths mean
  // on a quantity that only ever takes whole values.
  const text =
    metric === "pulsesPerLitre"
      ? String(Math.round(value)).padStart(7, " ")
      : formatFixed7(value);
  return `${text}${marker}`;
}

/**
 * The device's string for a per-sensor binding, or undefined when the binding is not one.
 *
 * Supersedes sampleValues.ts for everything it answers: those were plausible samples with one
 * shared state, these are the real formats over eight independent rows. Returning undefined is
 * how a caller knows to fall back — `config.sensor.nyquistWarning` is the notable case, being
 * editor state rather than sensor state (ui/core/ui_value_catalogue.cpp:88-89).
 *
 * `sensorIndex` is the selected sensor from `sensorIndexForScreen`, 1-based, 0 for none.
 * `rawValue`, when given, is formatted in place of the stored one — the device does exactly
 * this for an editor's pending value (`formatSetting(*editor.setting, editor.pending, ...)`,
 * ui/core/ui_bindings.cpp:377-383).
 */
export function resolveSensorBinding(
  binding: string,
  table: readonly SimulatedSensor[],
  sensorIndex: number,
  rawValue?: number,
  flowUnit: FlowUnit = 0
): string | undefined {
  const parsed = parseSensorMetricBinding(binding);
  if (parsed) {
    const sensor = sensorAt(table, parsed[0]);
    return sensor ? resolveSensorMetric(sensor, parsed[1], flowUnit) : undefined;
  }

  if (binding === "config.selectedSensor") {
    // "-" for none, the number otherwise (ui/core/ui_bindings.cpp:386-392).
    return sensorAt(table, sensorIndex) ? `${sensorIndex}` : "-";
  }

  /**
   * The formula line's two derived pieces, mirroring `ui_bindings.cpp`'s arm exactly.
   *
   * They were served from the static sample table, which made them the ONLY per-sensor values on
   * the screen that ignored the selected sensor: S4 showed `F = 0 *Q - 8  Q 0..150 L/m` — a
   * multiplier read from memory beside an adjust and a range read from nowhere. Both now follow
   * memory, so the whole row moves together.
   *
   * `--` in two cases, both the firmware's: no sensor implied by the navigation, and a channel
   * calibrated by pulses per litre, which has no formula for a term to belong to.
   */
  if (binding === "config.sensor.adjustTerm" || binding === "config.sensor.formulaQ") {
    const sensor = sensorAt(table, sensorIndex);
    if (!sensor || sensor.calibration !== 0) {
      return "--";
    }
    if (binding === "config.sensor.adjustTerm") {
      return `${sensor.adjust < 0 ? "-" : "+"} ${Math.abs(Math.trunc(sensor.adjust))}`;
    }
    return `Q 0..${Math.trunc(sensor.qMaxLpm)} L/m`;
  }

  if (binding === "config.sensor.undersamplingFlag") {
    // "-" with no sensor selected, else the bit as WARN/OK (ui/core/ui_bindings.cpp:403-409).
    const sensor = sensorAt(table, sensorIndex);
    if (!sensor) {
      return "-";
    }
    return mayUndersample(sensor) && sensor.undersampling ? "WARN" : "OK";
  }

  if (binding === "diagnostics.undersampling") {
    // "OK" when no bits are set, else the offending channels by NUMBER rather than a bitmask
    // (ui/core/ui_bindings.cpp:322-338). The device's list buffer is 24 bytes and the widest
    // possible list, "1,2,3,4,5,6,7,8", is 15 — so all eight fit and no truncation arm is
    // needed here.
    const flagged = warningSensorNumbers(table);
    return flagged.length === 0 ? "OK" : `! S${flagged.join(",")}`;
  }

  const descriptor = kPerSensorSettings[binding];
  if (descriptor) {
    const value = rawValue ?? readSettingValue(descriptor, table, sensorIndex);
    return formatSettingValue(descriptor, value);
  }

  return undefined;
}

/**
 * One pass of the state engine over one channel.
 *
 * `SensorStateEngine::update` (sensors/sensor_state_engine.cpp:7-60), per channel:
 *
 *  - a non-positive interval is a no-op (:8-10);
 *  - a DISCONNECTED channel has its instant flow zeroed and nothing else touched (:53-60) —
 *    the totals are retained and do not advance, no matter how many pulses arrive;
 *  - an enabled channel that is not ready, or whose multiplier is 0, reads 0 L/s and
 *    accumulates nothing (:26, :44-46);
 *  - otherwise flow comes from the pulse frequency, is clamped to [0, q_max] L/min, and the
 *    interval's litres are added to BOTH totals while the peak is raised if beaten (:27-43).
 */
export function advanceSensorTick(sensor: SimulatedSensor, input: SensorTickInput): SimulatedSensor {
  const elapsedSeconds = input.elapsedMs / 1000;
  if (!(elapsedSeconds > 0)) {
    return sensor;
  }
  if (!sensor.connected || !sensor.ready || sensor.multiplier === 0) {
    return normalizeSensor({ ...sensor, instantFlowLpm: 0 });
  }

  const frequency = input.pulses / elapsedSeconds;
  let flowLpm = (frequency - sensor.adjust) / sensor.multiplier;
  if (flowLpm < 0) {
    flowLpm = 0;
  }
  if (flowLpm > sensor.qMaxLpm) {
    flowLpm = sensor.qMaxLpm;
  }
  // Stored as computed, mirroring the engine after §2a: the q_max clamp above is already in L/min,
  // so this is where the conversion is REMOVED rather than added. The one division that remains is
  // the volume one below, and it is unavoidable — volume is a rate times a TIME, and the interval is
  // in seconds while the rate is per minute.
  const instantFlowLpm = flowLpm;
  const liters = (flowLpm * elapsedSeconds) / 60;
  return normalizeSensor({
    ...sensor,
    instantFlowLpm,
    maxFlowLpm: Math.max(sensor.maxFlowLpm, instantFlowLpm),
    sessionLiters: sensor.sessionLiters + liters,
    cumulativeLiters: sensor.cumulativeLiters + liters
  });
}

/**
 * `core.action.reset-all-measured` — clear every channel's measured values, keep its calibration.
 *
 * One home for what "reset the measurements" means, because App.tsx had spelled the three fields out
 * twice inline and the loop panel's own reset did something different again: it rebuilt the table from
 * `createSensorTable()`, which RESTORES the seeded 123.45 L and 140.4 L/min rather than clearing
 * anything. A reset that puts fabricated readings back is not a reset, and it made the aggregates
 * impossible to check by hand — a steady 60 L/min on eight channels showed 1.15 m3 after twenty
 * seconds, being 0.99 m3 of seed plus the 0.16 m3 actually accumulated.
 *
 * `instantFlowLpm` is included, unlike the firmware's action, for a reason the firmware does not have:
 * the device recomputes instant flow from pulses on its very next pass, so clearing it there would be
 * redundant. Here the loop may be stopped, and a stopped simulator showing flow it is not producing is
 * the same lie in a different place.
 */
export function resetMeasured(table: readonly SimulatedSensor[]): SimulatedSensor[] {
  return table.map((sensor) =>
    normalizeSensor({
      ...sensor,
      instantFlowLpm: 0,
      sessionLiters: 0,
      cumulativeLiters: 0,
      maxFlowLpm: 0
    })
  );
}

/**
 * `core.action.reset-max-flow` — the peak only, on every channel.
 *
 * The cheapest reset in the system: the peak is volatile, so a power cycle already clears it, and nothing
 * here is persisted. Its own function because the alternatives both destroy something — a session reset
 * takes the session volume with it, a measured reset takes the lifetime total.
 */
export function resetMaxFlow(table: readonly SimulatedSensor[]): SimulatedSensor[] {
  return table.map((sensor) => normalizeSensor({ ...sensor, maxFlowLpm: 0 }));
}

/** `core.action.reset-session` — the session volume only, on every channel. */
export function resetSession(table: readonly SimulatedSensor[]): SimulatedSensor[] {
  return table.map((sensor) => normalizeSensor({ ...sensor, sessionLiters: 0 }));
}

/**
 * `core.action.reset-calibration` — one channel's calibration, keeping everything it measured.
 *
 * The only per-channel reset in the file, hence the second argument: the other three act on all eight
 * because the commands behind them are the device-wide master registers, while this one is
 * `OFF_CMD_RESET_CALIBRATION` inside a single sensor's block. Matched on the `number` FIELD via
 * `sensorAt`, not on position — the file header records what happened when two lookup rules for one
 * fact disagreed.
 *
 * The four fields it zeroes are exactly `SensorCharacteristics{}`: q_max, f_multiplier, adjust and
 * pulses_per_litre, with `calibration` back to 0 (Formula), which is the struct's own default. The
 * three accumulated readings are not in that list on purpose — this is the meter swap, and the volume
 * the old meter measured was true when it measured it.
 *
 * `ready: false` IS THE FIRMWARE'S DERIVATION, WRITTEN OUT BY HAND. The device has no stored readiness
 * bit at all: `SensorData` deliberately dropped `isReady`, and readiness is recomputed as
 * `configIsValid(configs[n])` every frame (modbus/sensor_types.h). A defaulted config has q_max = 0, so
 * the device's answer flips to false the instant the config clears, and the channel renders `SET?`.
 * Here `ready` is still a separate boolean that nothing recomputes, so leaving it alone would give a
 * channel with no calibration still reporting `OK` and printing flow figures for a meter that is not
 * there. Setting it is the local fix; the general divergence is wider than this function — writing
 * `qMaxLpm` to 0 through the Values panel still leaves `ready` true — and is reported, not fixed here.
 *
 * `instantFlowLpm` is left to `normalizeSensor`, which zeroes it once `calibrated` goes false. Asserting
 * that rather than assigning it is what keeps this helper honest about where the rule lives.
 */
export function resetCalibration(
  table: readonly SimulatedSensor[],
  sensorNumber: number
): SimulatedSensor[] {
  // A number no row carries — 0 included, which is the "no sensor level entered" sentinel the
  // navigator uses — must change nothing. The firmware's own guard is the same one:
  // `handleResetCalibration` returns before writing when `sensorIndex()` is 0.
  if (sensorAt(table, sensorNumber) === undefined) {
    return table.map((sensor) => normalizeSensor({ ...sensor }));
  }
  return table.map((sensor) =>
    sensor.number === sensorNumber
      ? normalizeSensor({
          ...sensor,
          qMaxLpm: 0,
          multiplier: 0,
          adjust: 0,
          calibration: 0,
          pulsesPerLitre: 0,
          ready: false,
          // Mirrors `evaluateSensorDiagnostics`, which recomputes the undersampling bit from the config
          // and drops it for a channel whose config is invalid. A channel with no calibration cannot be
          // undersampling anything, and the firmware register arm clears the Nyquist override with the
          // config for the same reason: it described the OLD meter.
          undersampling: false
        })
      : normalizeSensor({ ...sensor })
  );
}

/**
 * The pulse count that makes a channel read `targetFlowLpm` over an interval.
 *
 * The inverse of the engine's two lines — `frequency = pulses / elapsedSeconds` and
 * `flowLpm = (frequency - adjust) / f_multiplier` (sensors/sensor_state_engine.cpp:27-28) — so
 * a panel that lets someone drag a flow slider can drive `advanceSensorTick` with it instead
 * of asking the operator for a frequency. Returns 0 for a channel with no multiplier, which is
 * the one case the engine refuses to compute flow for (:26).
 */
export function pulsesForFlow(
  sensor: SimulatedSensor,
  targetFlowLpm: number,
  elapsedMs: number
): number {
  if (sensor.multiplier === 0) {
    return 0;
  }
  // The `* 60` that used to be here converted the caller's L/s into the L/min the formula wants.
  // Callers now speak L/min (§2a), so it is gone rather than being cancelled by a second conversion.
  const frequency = targetFlowLpm * sensor.multiplier + sensor.adjust;
  return Math.max(0, frequency) * (elapsedMs / 1000);
}
