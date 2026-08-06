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
  /** `SensorData::instantFlow_L_s` (modbus/sensor_types.h:23). */
  instantFlowLps: number;
  /** `SensorData::cumulativeLiters` (modbus/sensor_types.h:24). */
  cumulativeLiters: number;
  /** `SensorData::sessionLiters` (modbus/sensor_types.h:25). */
  sessionLiters: number;
  /** `SensorData::maxFlowSinceReset` (modbus/sensor_types.h:26). */
  maxFlowLps: number;
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
  instantFlowLps: 2.34,
  cumulativeLiters: 123.45,
  sessionLiters: 123.45,
  maxFlowLps: 2.34,
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
  const flowing = sensor.connected && sensor.ready && sensor.multiplier !== 0;
  return {
    ...sensor,
    instantFlowLps: flowing ? sensor.instantFlowLps : 0,
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

/** The row for a 1-based sensor number, or undefined when there is none (0 included). */
export function sensorAt(
  table: readonly SimulatedSensor[],
  sensorNumber: number
): SimulatedSensor | undefined {
  if (!Number.isInteger(sensorNumber) || sensorNumber < 1 || sensorNumber > table.length) {
    return undefined;
  }
  return table[sensorNumber - 1];
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

/** `%6.2f` — two decimals, right-aligned in a field of six, never truncated. */
function formatFixed6(value: number): string {
  return value.toFixed(2).padStart(6, " ");
}

type SettingField = "connected" | "multiplier" | "adjust" | "qMaxLpm";

interface PerSensorSettingDescriptor {
  field: SettingField;
  /** `SettingDescriptor::unit`; only maxFlow has one. */
  unit?: string;
  /** `SettingDescriptor::options`; only the boolean has them. */
  options?: { label: string; value: number }[];
}

/**
 * The four per-sensor settings, with the unit and option labels the device formats them with.
 *
 * Taken from the firmware's setting table, not from the manifest: these are the strings
 * `formatSetting` prints (ui/core/ui_settings_types.cpp:48-56, and `kBoolOptions` =
 * {"Off", 0}, {"On", 1} at ui/core/ui_settings_types.cpp:21).
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
  "config.sensor.maxFlow": { field: "qMaxLpm", unit: "L/min" }
};

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

/** The metrics `resolveSensorBinding` knows, and the unit each renders with. */
const kMetricUnits: Record<string, string> = {
  instantFlow: "L/s",
  cumulativeLiters: "L",
  cumulativeM3: "m^3",
  sessionLiters: "L",
  sessionM3: "m^3",
  maxFlowSinceReset: "L/s"
};

function metricValue(sensor: SimulatedSensor, metric: string): number | undefined {
  // ui/core/ui_bindings.cpp:281-304. The m^3 pair is the litre reading over 1000; there is no
  // separately stored cubic-metre total.
  switch (metric) {
    case "instantFlow":
      return sensor.instantFlowLps;
    case "cumulativeLiters":
      return sensor.cumulativeLiters;
    case "cumulativeM3":
      return sensor.cumulativeLiters / 1000;
    case "sessionLiters":
      return sensor.sessionLiters;
    case "sessionM3":
      return sensor.sessionLiters / 1000;
    case "maxFlowSinceReset":
      return sensor.maxFlowLps;
    default:
      return undefined;
  }
}

/**
 * What the device draws for `sensor.<n>.<metric>`, in the order the resolver tests it.
 *
 * ui/core/ui_bindings.cpp:249-307. Four outcomes, and a metric and a status differ in both
 * respects: the status omits the sensor number, and it answers even when no reading is
 * available.
 *
 *   status, any state      "--" | "WAIT" | "OK"     (:266-270)
 *   metric, disconnected   `%u: --`   -> "3: --"    (:272-275)
 *   metric, not ready      `%u: WAIT` -> "3: WAIT"  (:276-279)
 *   metric, ready          `%u: %6.2f %s`           (:306)
 *
 * A disconnected row is therefore neither hidden nor zero: it prints its number and "--".
 */
function resolveSensorMetric(sensor: SimulatedSensor, metric: string): string | undefined {
  if (metric === "status") {
    return !sensor.connected ? "--" : sensor.ready ? "OK" : "WAIT";
  }
  if (!sensor.connected) {
    return `${sensor.number}: --`;
  }
  if (!sensor.ready) {
    return `${sensor.number}: WAIT`;
  }
  const value = metricValue(sensor, metric);
  if (value === undefined) {
    // Unknown metric: the firmware returns false rather than rendering a plausible-looking
    // wrong number (ui/core/ui_bindings.cpp:301-303), and so does this.
    return undefined;
  }
  return `${sensor.number}: ${formatFixed6(value)} ${kMetricUnits[metric]}`;
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
  rawValue?: number
): string | undefined {
  const parsed = parseSensorMetricBinding(binding);
  if (parsed) {
    const sensor = sensorAt(table, parsed[0]);
    return sensor ? resolveSensorMetric(sensor, parsed[1]) : undefined;
  }

  if (binding === "config.selectedSensor") {
    // "-" for none, the number otherwise (ui/core/ui_bindings.cpp:386-392).
    return sensorAt(table, sensorIndex) ? `${sensorIndex}` : "-";
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
    return normalizeSensor({ ...sensor, instantFlowLps: 0 });
  }

  const frequency = input.pulses / elapsedSeconds;
  let flowLpm = (frequency - sensor.adjust) / sensor.multiplier;
  if (flowLpm < 0) {
    flowLpm = 0;
  }
  if (flowLpm > sensor.qMaxLpm) {
    flowLpm = sensor.qMaxLpm;
  }
  const instantFlowLps = flowLpm / 60;
  const liters = instantFlowLps * elapsedSeconds;
  return normalizeSensor({
    ...sensor,
    instantFlowLps,
    maxFlowLps: Math.max(sensor.maxFlowLps, instantFlowLps),
    sessionLiters: sensor.sessionLiters + liters,
    cumulativeLiters: sensor.cumulativeLiters + liters
  });
}

/**
 * The pulse count that makes a channel read `targetFlowLps` over an interval.
 *
 * The inverse of the engine's two lines — `frequency = pulses / elapsedSeconds` and
 * `flowLpm = (frequency - adjust) / f_multiplier` (sensors/sensor_state_engine.cpp:27-28) — so
 * a panel that lets someone drag a flow slider can drive `advanceSensorTick` with it instead
 * of asking the operator for a frequency. Returns 0 for a channel with no multiplier, which is
 * the one case the engine refuses to compute flow for (:26).
 */
export function pulsesForFlow(
  sensor: SimulatedSensor,
  targetFlowLps: number,
  elapsedMs: number
): number {
  if (sensor.multiplier === 0) {
    return 0;
  }
  const frequency = targetFlowLps * 60 * sensor.multiplier + sensor.adjust;
  return Math.max(0, frequency) * (elapsedMs / 1000);
}
