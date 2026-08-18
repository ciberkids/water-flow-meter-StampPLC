import { describe, expect, it } from "vitest";
import {
  advanceSensorTick,
  createSensorTable,
  isPerSensorSetting,
  kSensorCount,
  mayUndersample,
  pulsesForFlow,
  resetCalibration,
  resetMaxFlow,
  resetMeasured,
  resetSession,
  resolveSensorBinding,
  sensorAt,
  sensorIndexForScreen,
  setSensor,
  statusSummaryText,
  type SimulatedSensor,
  uncalibratedSensorNumbers,
  warningSensorNumbers,
  warningSummaryText
} from "../sensorConfig";
import manifest from "../../data/actionManifest.json";
import type { FirmwareValueDefinition } from "../../types/firmwareActions";

const values = manifest.values as FirmwareValueDefinition[];
const valueById = (id: string) => values.find((value) => value.id === id);

/** The real navigation chain to a per-sensor editor, read off screens.json. */
const kChainToSensor3 = [
  "info-p7-enter-config",
  "config-sensors",
  "config-sensor-3",
  "config-s2-multiplier"
];

describe("per-sensor table", () => {
  it("numbers its rows the way the device labels them", () => {
    expect(createSensorTable().map((sensor) => sensor.number)).toEqual([1, 2, 3, 4, 5, 6, 7, 8]);
    expect(kSensorCount).toBe(8);
  });

  it("defaults to a ready, flowing channel that renders what the old shared samples showed", () => {
    const table = createSensorTable();
    // These are sampleValues.ts's strings for sensor 3, which this module supersedes: wiring it
    // in must not change what the panel draws for a working device.
    expect(resolveSensorBinding("sensor.3.status", table, 0)).toBe("OK");
    expect(resolveSensorBinding("sensor.3.instantFlow", table, 0)).toBe(" 140.40");
    expect(resolveSensorBinding("sensor.3.maxFlowSinceReset", table, 0)).toBe(" 140.40");
    expect(resolveSensorBinding("sensor.3.cumulativeLiters", table, 0)).toBe(" 123.45");
    expect(resolveSensorBinding("sensor.3.sessionLiters", table, 0)).toBe(" 123.45");
    expect(resolveSensorBinding("sensor.3.cumulativeM3", table, 0)).toBe("   0.12");
    expect(resolveSensorBinding("sensor.3.sessionM3", table, 0)).toBe("   0.12");
  });

  it("keeps the eight rows independent", () => {
    const table = setSensor(createSensorTable(), 1, { connected: false });
    expect(resolveSensorBinding("sensor.1.instantFlow", table, 0)).toBe("--");
    expect(resolveSensorBinding("sensor.2.instantFlow", table, 0)).toBe(" 140.40");
    expect(sensorAt(table, 2)?.connected).toBe(true);
  });

  it("has no row for the 0 index", () => {
    const table = createSensorTable();
    expect(sensorAt(table, 0)).toBeUndefined();
    expect(sensorAt(table, 9)).toBeUndefined();
    expect(sensorAt(table, 1)?.number).toBe(1);
  });
});

describe("rendering, against the device's own format strings", () => {
  const table = createSensorTable();
  const flowing = table;
  const zeroFlow = setSensor(table, 4, { instantFlowLpm: 0 });
  // NOT-READY IS REACHED THE WAY THE DEVICE REACHES IT (DF16): by clearing the calibration, not by
  // clearing a bit. `ready` is derived from `configIsValid` now, so `{ ready: false }` on an otherwise
  // valid channel is a state neither this table nor the firmware can hold.
  const notReady = setSensor(table, 4, { qMaxLpm: 0 });
  const disconnected = setSensor(table, 4, { connected: false });

  it("prints a metric with the 1-based sensor number and a seven-wide value, no unit", () => {
    expect(resolveSensorBinding("sensor.4.instantFlow", flowing, 0)).toBe(" 140.40");
    expect(resolveSensorBinding("sensor.4.cumulativeLiters", flowing, 0)).toBe(" 123.45");
  });

  it("marks a peak that reached the channel's own ceiling with MAX", () => {
    // §5a's whole purpose: a channel pinned at its q_max is under-dimensioned for its pipe, and the
    // number alone cannot say so because a legitimate peak can sit just below the same value.
    // Compared in L/min against q_max, which is stored in L/min — comparing the stored L/s peak
    // against it would flag nothing, ever.
    const atCeiling = setSensor(createSensorTable(), 4, { qMaxLpm: 150, maxFlowLpm: 150 });
    expect(resolveSensorBinding("sensor.4.maxFlowSinceReset", atCeiling, 0)).toBe(" 150.00 MAX");

    const below = setSensor(createSensorTable(), 4, { qMaxLpm: 150, maxFlowLpm: 144 });
    expect(resolveSensorBinding("sensor.4.maxFlowSinceReset", below, 0)).toBe(" 144.00");
  });

  it("never marks the instantaneous row, only the peak", () => {
    // The marker belongs to §5a's page. A live reading AT the ceiling is being clamped right now,
    // which the peak page is what reports — flagging it twice would say two different things.
    const atCeiling = setSensor(createSensorTable(), 4, { qMaxLpm: 150, instantFlowLpm: 150 });
    expect(resolveSensorBinding("sensor.4.instantFlow", atCeiling, 0)).toBe(" 150.00");
  });

  it("prints zero flow as a measurement, not as a withheld value", () => {
    expect(resolveSensorBinding("sensor.4.instantFlow", zeroFlow, 0)).toBe("   0.00");
    expect(resolveSensorBinding("sensor.4.status", zeroFlow, 0)).toBe("OK");
  });

  it("distinguishes not-ready from both connected and disconnected", () => {
    expect(resolveSensorBinding("sensor.4.instantFlow", notReady, 0)).toBe("SET?");
    expect(resolveSensorBinding("sensor.4.status", notReady, 0)).toBe("SET?");
  });

  it("prints a disconnected channel's number and dashes — never hidden, never zero", () => {
    const rendered = resolveSensorBinding("sensor.4.cumulativeLiters", disconnected, 0);
    expect(rendered).toBe("--");
    expect(rendered).not.toBe("  0.00 L");
    expect(resolveSensorBinding("sensor.4.status", disconnected, 0)).toBe("--");
  });

  it("renders a status without the sensor number and a metric with it", () => {
    expect(resolveSensorBinding("sensor.4.status", disconnected, 0)).toBe("--");
    expect(resolveSensorBinding("sensor.4.instantFlow", disconnected, 0)).toBe("--");
    expect(resolveSensorBinding("sensor.4.status", notReady, 0)).toBe("SET?");
    expect(resolveSensorBinding("sensor.4.instantFlow", notReady, 0)).toBe("SET?");
  });

  it("refuses bindings it cannot answer instead of inventing a number", () => {
    expect(resolveSensorBinding("sensor.4.notAMetric", flowing, 0)).toBeUndefined();
    expect(resolveSensorBinding("sensor.0.instantFlow", flowing, 0)).toBeUndefined();
    expect(resolveSensorBinding("sensor.9.instantFlow", flowing, 0)).toBeUndefined();
    expect(resolveSensorBinding("net.wifi.ssid", flowing, 0)).toBeUndefined();
    // Editor state, not sensor state: the caller must keep its own answer for this one.
    expect(resolveSensorBinding("config.sensor.nyquistWarning", flowing, 3)).toBeUndefined();
  });
});

describe("selected-sensor resolution", () => {
  it("takes the index from the config-sensor-<n> ancestor, not from the current screen", () => {
    expect(sensorIndexForScreen("config-s2-multiplier-edit", kChainToSensor3)).toBe(3);
    expect(sensorIndexForScreen("config-s1-connected", kChainToSensor3.slice(0, 3))).toBe(3);
    // Standing ON the list entry implies nothing: the firmware fixes the index while LEAVING it.
    expect(sensorIndexForScreen("config-sensor-3", ["info-p7-enter-config", "config-sensors"])).toBe(0);
  });

  it("reports 0 when no sensor is implied", () => {
    expect(sensorIndexForScreen("info-p0-global-status", [])).toBe(0);
    expect(sensorIndexForScreen("config-modbus-slave-id", ["info-p7-enter-config"])).toBe(0);
    // A real screen whose id starts with the prefix but names no sensor.
    expect(sensorIndexForScreen("config-s1-connected", ["config-sensor-back"])).toBe(0);
    expect(sensorIndexForScreen("config-s1-connected", ["config-sensor-settings-back"])).toBe(0);
    // Out of range, and multi-digit, which the firmware's single-digit parse rejects too.
    expect(sensorIndexForScreen("config-s1-connected", ["config-sensor-9"])).toBe(0);
    expect(sensorIndexForScreen("config-s1-connected", ["config-sensor-12"])).toBe(0);
  });

  it("resolves every sensor list entry the dataset defines", () => {
    for (let number = 1; number <= kSensorCount; number += 1) {
      expect(sensorIndexForScreen("config-s1-connected", [`config-sensor-${number}`])).toBe(number);
    }
  });

  it("renders the selected sensor, and '-' when there is none", () => {
    const table = createSensorTable();
    expect(resolveSensorBinding("config.selectedSensor", table, 3)).toBe("3");
    expect(resolveSensorBinding("config.selectedSensor", table, 0)).toBe("-");
    expect(resolveSensorBinding("config.sensor.undersamplingFlag", table, 0)).toBe("-");
  });
});

describe("per-sensor settings", () => {
  it("selects the writable settings, not everything flagged perSensor", () => {
    expect(isPerSensorSetting(valueById("config.sensor.connected"))).toBe(true);
    expect(isPerSensorSetting(valueById("config.sensor.multiplier"))).toBe(true);
    expect(isPerSensorSetting(valueById("config.sensor.adjust"))).toBe(true);
    expect(isPerSensorSetting(valueById("config.sensor.maxFlow"))).toBe(true);

    // Both carry perSensor and are derived, read-only editor/diagnostic state.
    expect(valueById("config.sensor.nyquistWarning")?.perSensor).toBe(true);
    expect(isPerSensorSetting(valueById("config.sensor.nyquistWarning"))).toBe(false);
    expect(valueById("config.sensor.undersamplingFlag")?.perSensor).toBe(true);
    expect(isPerSensorSetting(valueById("config.sensor.undersamplingFlag"))).toBe(false);

    // A setting that is not per-sensor, and a per-sensor reading.
    expect(isPerSensorSetting(valueById("config.modbusSlaveId"))).toBe(false);
    expect(isPerSensorSetting(valueById("sensor.1.instantFlow"))).toBe(false);
    expect(isPerSensorSetting(undefined)).toBe(false);
  });

  it("resolves every per-sensor setting plus the three derived per-sensor values, and declines the rest", () => {
    // Resolving an id and it being an editable SETTING are different questions, and this test used to
    // assert they were the same set. They differ by the DERIVED per-sensor values: they read sensor
    // state and so must resolve, but they are read-only.
    //
    // Listing them explicitly is the point. This assertion is what caught `config.sensor.calibrationType`
    // and `config.sensor.pulsesPerLiter` missing from kPerSensorSettings when the calibration branch
    // landed — without it both would have fallen through to the static sample path and stopped tracking
    // the sensor table, which is exactly what `adjustTerm` and `formulaQ` were doing until they were
    // moved here.
    const derived = [
      "config.sensor.undersamplingFlag",
      "config.sensor.adjustTerm",
      "config.sensor.formulaQ"
    ];
    const resolved = values
      .filter((value) => value.id.startsWith("config.sensor."))
      .filter((value) => resolveSensorBinding(value.id, createSensorTable(), 3) !== undefined)
      .map((value) => value.id)
      .sort();
    const settings = values.filter(isPerSensorSetting).map((value) => value.id);
    expect(resolved).toEqual([...settings, ...derived].sort());

    // And the one per-sensor-FLAGGED id that is really editor state must be DECLINED, so the caller
    // falls back instead of being handed sensor state that does not exist.
    expect(resolveSensorBinding("config.sensor.nyquistWarning", createSensorTable(), 3)).toBeUndefined();
  });

  it("formats the selected sensor's stored value with the device's unit and labels", () => {
    const table = setSensor(createSensorTable(), 3, {
      multiplier: 12,
      adjust: -4,
      qMaxLpm: 150
    });
    expect(resolveSensorBinding("config.sensor.multiplier", table, 3)).toBe("12");
    expect(resolveSensorBinding("config.sensor.adjust", table, 3)).toBe("-4");
    expect(resolveSensorBinding("config.sensor.maxFlow", table, 3)).toBe("150 L/min");
    expect(resolveSensorBinding("config.sensor.connected", table, 3)).toBe("On");
    expect(resolveSensorBinding("config.sensor.connected", setSensor(table, 3, { connected: false }), 3)).toBe("Off");
  });

  it("reads 0 for every setting when no sensor is selected", () => {
    const table = createSensorTable();
    expect(resolveSensorBinding("config.sensor.multiplier", table, 0)).toBe("0");
    expect(resolveSensorBinding("config.sensor.adjust", table, 0)).toBe("0");
    expect(resolveSensorBinding("config.sensor.maxFlow", table, 0)).toBe("0 L/min");
    expect(resolveSensorBinding("config.sensor.connected", table, 0)).toBe("Off");
  });

  it("formats a supplied raw value in place of the stored one", () => {
    const table = createSensorTable();
    expect(resolveSensorBinding("config.sensor.maxFlow", table, 3, 900)).toBe("900 L/min");
    expect(resolveSensorBinding("config.sensor.connected", table, 3, 0)).toBe("Off");
    expect(resolveSensorBinding("config.sensor.maxFlow", table, 3)).toBe("150 L/min");
  });
});

describe("advancing a tick", () => {
  const oneSecond = { pulses: 0, elapsedMs: 1000 };

  it("accumulates a ready channel's litres into both totals and raises the peak", () => {
    const sensor = sensorAt(createSensorTable(), 1) as SimulatedSensor;
    // multiplier 10, adjust 0: 1200 pulses in 1 s is 1200 Hz, so 120 L/min — which is now what the
    // channel STORES (§2a), where it used to store the 2 L/s that works out to.
    const next = advanceSensorTick({ ...sensor, maxFlowLpm: 0 }, { pulses: 1200, elapsedMs: 1000 });
    expect(next.instantFlowLpm).toBeCloseTo(120, 10);
    expect(next.maxFlowLpm).toBeCloseTo(120, 10);
    // Volume is still litres: 120 L/min over one second is 2 L. This is the one division §2a leaves,
    // and asserting it separately from the rate is what would catch it being dropped with the others.
    expect(next.sessionLiters).toBeCloseTo(sensor.sessionLiters + 2, 10);
    expect(next.cumulativeLiters).toBeCloseTo(sensor.cumulativeLiters + 2, 10);
  });

  it("clamps flow to q_max and never below zero", () => {
    const sensor = sensorAt(createSensorTable(), 1) as SimulatedSensor;
    const fast = advanceSensorTick(sensor, { pulses: 100000, elapsedMs: 1000 });
    // Clamped AT q_max, not at a sixtieth of it: both are L/min now, so the comparison is direct.
    expect(fast.instantFlowLpm).toBeCloseTo(sensor.qMaxLpm, 10);

    const backwards = advanceSensorTick({ ...sensor, adjust: 5000 }, { pulses: 10, elapsedMs: 1000 });
    expect(backwards.instantFlowLpm).toBe(0);
    expect(backwards.cumulativeLiters).toBe(sensor.cumulativeLiters);
  });

  it("keeps the peak when a later tick is slower", () => {
    const sensor = sensorAt(createSensorTable(), 1) as SimulatedSensor;
    const fast = advanceSensorTick(sensor, { pulses: 1200, elapsedMs: 1000 });
    const slow = advanceSensorTick(fast, { pulses: 60, elapsedMs: 1000 });
    // 60 pulses in 1 s is 60 Hz over a multiplier of 10, so 6 L/min — the old 0.1 L/s.
    expect(slow.instantFlowLpm).toBeCloseTo(6, 10);
    expect(slow.maxFlowLpm).toBeCloseTo(fast.maxFlowLpm, 10);
  });

  it("leaves a disconnected channel's totals frozen, however many pulses arrive", () => {
    const table = setSensor(createSensorTable(), 5, { connected: false });
    const before = sensorAt(table, 5) as SimulatedSensor;
    const after = advanceSensorTick(before, { pulses: 5000, elapsedMs: 1000 });

    expect(after.cumulativeLiters).toBe(123.45);
    expect(after.sessionLiters).toBe(123.45);
    // The default peak, in the stored unit: 140.4 L/min is the old 2.34 L/s (§2a).
    expect(after.maxFlowLpm).toBe(140.4);
    expect(after.instantFlowLpm).toBe(0);
    // And the row still renders as withheld rather than as zero.
    expect(resolveSensorBinding("sensor.5.cumulativeLiters", [after], 0)).toBe("--");
  });

  it("accumulates nothing for an enabled channel that is not ready or has no multiplier", () => {
    const sensor = sensorAt(createSensorTable(), 1) as SimulatedSensor;

    // `qMaxLpm: 0` rather than `ready: false` — see DF16. This is also exactly the configuration the
    // engine's own gate refuses (`configIsValid` requires a non-zero q_max), which is what makes the
    // channel produce nothing.
    const waiting = advanceSensorTick({ ...sensor, qMaxLpm: 0 }, { pulses: 1200, elapsedMs: 1000 });
    expect(waiting.instantFlowLpm).toBe(0);
    expect(waiting.cumulativeLiters).toBe(sensor.cumulativeLiters);
    expect(waiting.sessionLiters).toBe(sensor.sessionLiters);

    const unconfigured = advanceSensorTick({ ...sensor, multiplier: 0 }, { pulses: 1200, elapsedMs: 1000 });
    expect(unconfigured.instantFlowLpm).toBe(0);
    expect(unconfigured.cumulativeLiters).toBe(sensor.cumulativeLiters);
  });

  it("treats a non-positive interval as no time having passed", () => {
    const sensor = sensorAt(createSensorTable(), 1) as SimulatedSensor;
    expect(advanceSensorTick(sensor, { pulses: 1200, elapsedMs: 0 })).toEqual(sensor);
    expect(advanceSensorTick(sensor, { pulses: 1200, elapsedMs: -1000 })).toEqual(sensor);
    expect(advanceSensorTick(sensor, oneSecond).instantFlowLpm).toBe(0);
  });

  it("drives a target flow through the pulse count the device would have counted", () => {
    const sensor = sensorAt(createSensorTable(), 1) as SimulatedSensor;
    const pulses = pulsesForFlow(sensor, 2.34, 1000);
    expect(advanceSensorTick(sensor, { pulses, elapsedMs: 1000 }).instantFlowLpm).toBeCloseTo(2.34, 10);
    expect(pulsesForFlow({ ...sensor, multiplier: 0 }, 2.34, 1000)).toBe(0);
  });
});

describe("undersampling", () => {
  it("cannot be carried by a disconnected channel, whatever a caller asks for", () => {
    const flagged = setSensor(createSensorTable(), 2, { undersampling: true });
    expect(sensorAt(flagged, 2)?.undersampling).toBe(true);

    const pulled = setSensor(flagged, 2, { connected: false });
    expect(sensorAt(pulled, 2)?.undersampling).toBe(false);

    const both = setSensor(createSensorTable(), 2, { connected: false, undersampling: true });
    expect(sensorAt(both, 2)?.undersampling).toBe(false);

    const ticked = advanceSensorTick(
      { ...(sensorAt(createSensorTable(), 2) as SimulatedSensor), connected: false, undersampling: true },
      { pulses: 1200, elapsedMs: 1000 }
    );
    expect(ticked.undersampling).toBe(false);

    expect(mayUndersample(sensorAt(pulled, 2) as SimulatedSensor)).toBe(false);
    expect(mayUndersample(sensorAt(flagged, 2) as SimulatedSensor)).toBe(true);
  });

  it("names the flagged channels by number and says OK when there are none", () => {
    const clean = createSensorTable();
    expect(resolveSensorBinding("diagnostics.undersampling", clean, 0)).toBe("OK");
    expect(warningSensorNumbers(clean)).toEqual([]);

    const flagged = setSensor(setSensor(clean, 3, { undersampling: true }), 5, { undersampling: true });
    expect(resolveSensorBinding("diagnostics.undersampling", flagged, 0)).toBe("! S3,5");
    expect(warningSensorNumbers(flagged)).toEqual([3, 5]);

    // Pulling sensor 3 out takes it off the list, because the device never tests a
    // disconnected channel's sampling rate at all.
    const pulled = setSensor(flagged, 3, { connected: false });
    expect(resolveSensorBinding("diagnostics.undersampling", pulled, 0)).toBe("! S5");
    expect(warningSensorNumbers(pulled)).toEqual([5]);

    const all = createSensorTable().reduce(
      (table, sensor) => setSensor(table, sensor.number, { undersampling: true }),
      createSensorTable()
    );
    expect(resolveSensorBinding("diagnostics.undersampling", all, 0)).toBe("! S1,2,3,4,5,6,7,8");
  });

  it("renders the selected sensor's flag as WARN or OK", () => {
    const flagged = setSensor(createSensorTable(), 3, { undersampling: true });
    expect(resolveSensorBinding("config.sensor.undersamplingFlag", flagged, 3)).toBe("WARN");
    expect(resolveSensorBinding("config.sensor.undersamplingFlag", flagged, 4)).toBe("OK");
    expect(resolveSensorBinding("config.sensor.undersamplingFlag", flagged, 0)).toBe("-");
  });
});

/**
 * The summary lines, and the lie they used to tell.
 *
 * `telemetry.status` was `warnings > 0 ? "N warnings" : "All sensors ready"` — undersampling flags its
 * only input — so a device whose channels all sat at `SET?` claimed to be ready. Both counts are now
 * reported and kept DISTINCT: a commissioning gap is not a faulty reading, and one number covering both
 * tells an operator neither.
 */
describe("the summary lines report a commissioning gap, and rank it first", () => {
   * Uncalibrated in this table means what the ROW means: connected, and not ready.
   *
   * Since DF16 that state is reached by clearing the CALIBRATION, because `ready` is derived from
   * `configIsValid`. `q_max = 0` is the smallest way to say "no valid calibration" and is the same field
   * the device's own predicate checks first.
   */
  const uncalibrate = (table: readonly SimulatedSensor[], ...numbers: number[]) =>
    numbers.reduce((next, number) => setSensor(next, number, { qMaxLpm: 0 }), table as SimulatedSensor[]);

  it("counts N uncalibrated in-use channels and says so", () => {
    const table = uncalibrate(createSensorTable(), 2, 4, 7);
    expect(uncalibratedSensorNumbers(table)).toEqual([2, 4, 7]);
    expect(statusSummaryText(table)).toBe("3 channels not calibrated");
    expect(warningSummaryText(table)).toBe("3 channels not calibrated");

    // One channel is singular, the same concession `%u warning%s` already made on the device.
    const one = uncalibrate(createSensorTable(), 6);
    expect(statusSummaryText(one)).toBe("1 channel not calibrated");
    expect(warningSummaryText(one)).toBe("1 channel not calibrated");

    // And the rows agree, because both read the same derived `ready` (DF16).
    expect(resolveSensorBinding("sensor.6.status", one, 0)).toBe("SET?");
    expect(resolveSensorBinding("sensor.5.status", one, 0)).toBe("OK");
  });

  it("puts the commissioning gap ahead of a sampling warning, without merging the two", () => {
    const table = setSensor(
      setSensor(uncalibrate(createSensorTable(), 1), 5, { undersampling: true }),
      6,
      { undersampling: true }
    );
    expect(uncalibratedSensorNumbers(table)).toEqual([1]);
    expect(warningSensorNumbers(table)).toEqual([5, 6]);
    // NOT "3 warnings": three problems of two kinds, and the operator needs to know which is which.
    expect(statusSummaryText(table)).toBe("1 channel not calibrated | 2 warnings");
    // The banner's string trades the channel list for a count when both are present — 37 columns.
    expect(warningSummaryText(table)).toBe("1 not calibrated, 2 undersampling");
    expect(statusSummaryText(table).length).toBeLessThanOrEqual(40);
    expect(warningSummaryText(table).length).toBeLessThanOrEqual(37);

    // One of each: both singulars at once, which a plural on the wrong count would survive.
    const oneEach = setSensor(uncalibrate(createSensorTable(), 1), 5, { undersampling: true });
    expect(statusSummaryText(oneEach)).toBe("1 channel not calibrated | 1 warning");
    expect(warningSummaryText(oneEach)).toBe("1 not calibrated, 1 undersampling");

    // Alone, the sampling case keeps the wording and the channel list it always had.
    const samplingOnly = setSensor(setSensor(createSensorTable(), 5, { undersampling: true }), 6, {
      undersampling: true
    });
    expect(statusSummaryText(samplingOnly)).toBe("2 warnings");
    expect(warningSummaryText(samplingOnly)).toBe("Sampling warning on sensors 5, 6");
  });

  it("still says all-ready when every in-use channel is calibrated and unflagged", () => {
    const table = createSensorTable();
    expect(uncalibratedSensorNumbers(table)).toEqual([]);
    expect(statusSummaryText(table)).toBe("All sensors ready");
    expect(warningSummaryText(table)).toBe("All sensors nominal");
  });

  it("does not count a DISCONNECTED channel as uncalibrated — it is absent, not unset", () => {
    // One sensor wired, seven bare terminals: one channel to commission, not eight. Disconnecting also
    // has to clear `ready`, or a channel could return as calibrated when it is reconnected untouched.
    let table = createSensorTable();
    for (let number = 2; number <= kSensorCount; number += 1) {
      table = setSensor(table, number, { connected: false, qMaxLpm: 0 });
    }
    expect(uncalibratedSensorNumbers(table)).toEqual([]);
    expect(statusSummaryText(table)).toBe("All sensors ready");
    expect(resolveSensorBinding("sensor.2.status", table, 0)).toBe("--");

    // Now leave the ONE wired channel uncalibrated: one problem reported, not eight.
    const wiredOnly = setSensor(table, 1, { qMaxLpm: 0 });
    expect(uncalibratedSensorNumbers(wiredOnly)).toEqual([1]);
    expect(statusSummaryText(wiredOnly)).toBe("1 channel not calibrated");
  });

  it("refuses to call a device with nothing wired ready", () => {
    // The connected bitmap comes out of NVS with a default of 0, so this is the state a device ships in
    // — both counts are legitimately zero and "All sensors ready" would be a vacuous claim about no
    // sensors at all. `telemetry.maxFlowLpm` already refuses it the same way with `Max Flow: --`.
    let table = createSensorTable();
    for (let number = 1; number <= kSensorCount; number += 1) {
      table = setSensor(table, number, { connected: false });
    }
    expect(statusSummaryText(table)).toBe("No channels in use");
    expect(warningSummaryText(table)).toBe("No channels in use");
  });

  it("keeps every string inside the panel's 40 columns, and ASCII-only", () => {
    // Eight of each kind at once: the widest either line can be.
    let table = createSensorTable();
    for (let number = 1; number <= kSensorCount; number += 1) {
      table = setSensor(table, number, { qMaxLpm: 0, undersampling: true });
    }
    // The noun survives on the 40-column status row and is dropped only on the banner's 37, where
    // "8 channels not calibrated, 8 undersampling" would be 42.
    expect(statusSummaryText(table)).toBe("8 channels not calibrated | 8 warnings");
    expect(warningSummaryText(table)).toBe("8 not calibrated, 8 undersampling");
    for (const text of [statusSummaryText(table), warningSummaryText(table)]) {
      expect(text.length).toBeLessThanOrEqual(40);
      // Font0 draws a blank cell above 255 and the wrong glyph above 175 — §Font0, no exceptions.
      expect(/^[\x20-\x7E]*$/.test(text)).toBe(true);
    }
  });
});

describe("the flow a caller asks for is the flow that comes back", () => {
  it("round-trips a target through pulsesForFlow and advanceSensorTick", () => {
    /**
     * The guard the loop simulator needed and did not have.
     *
     * `pulsesForFlow` once multiplied by 60 to convert a caller's L/s; when storage moved to L/min
     * (§2a) that conversion was correctly deleted, and App.tsx's loop was not updated — it went on
     * passing `qMaxLpm / 60` under the name `ceilingLps`. So every simulated run drove the channels at
     * one sixtieth of the intended flow for months, and nothing noticed, because a random target per
     * tick produces no number anybody can check.
     *
     * This is that check: ask for a flow, get that flow.
     */
    const [sensor] = createSensorTable();
    for (const targetLpm of [1, 30, 60, 149]) {
      const ticked = advanceSensorTick(sensor, {
        pulses: pulsesForFlow(sensor, targetLpm, 1000),
        elapsedMs: 1000
      });
      expect(ticked.instantFlowLpm, `asked for ${targetLpm} L/min`).toBeCloseTo(targetLpm, 6);
    }
  });

  it("accumulates the volume the flow implies", () => {
    // 60 L/min for 30 s is 30 L. The loop panel's steady mode exists so this is checkable on screen;
    // it is checkable here so a regression does not wait for somebody to look.
    let sensor = { ...createSensorTable()[0], sessionLiters: 0, cumulativeLiters: 0, maxFlowLpm: 0 };
    for (let tick = 0; tick < 30; tick += 1) {
      sensor = advanceSensorTick(sensor, {
        pulses: pulsesForFlow(sensor, 60, 1000),
        elapsedMs: 1000
      });
    }
    expect(sensor.sessionLiters).toBeCloseTo(30, 6);
    expect(sensor.cumulativeLiters).toBeCloseTo(30, 6);
    expect(sensor.maxFlowLpm).toBeCloseTo(60, 6);
  });

  it("clamps a steady target to the channel's own q_max rather than exceeding it", () => {
    // What the loop does when the steady figure is above a channel's ceiling: the engine clamps, and
    // the channel visibly runs slower than its neighbours instead of the panel hiding the difference.
    const sensor = { ...createSensorTable()[0], qMaxLpm: 100 };
    const ticked = advanceSensorTick(sensor, {
      pulses: pulsesForFlow(sensor, 500, 1000),
      elapsedMs: 1000
    });
    expect(ticked.instantFlowLpm).toBe(100);
  });
});

describe("resetting measured values", () => {
  it("clears the four readings and keeps the calibration", () => {
    const table = createSensorTable();
    const cleared = resetMeasured(table);
    for (const sensor of cleared) {
      expect(sensor.instantFlowLpm).toBe(0);
      expect(sensor.sessionLiters).toBe(0);
      expect(sensor.cumulativeLiters).toBe(0);
      expect(sensor.maxFlowLpm).toBe(0);
      // Calibration survives — the device's reset-all-measured keeps it, and it is usually what
      // somebody is editing when they want a run from zero.
      expect(sensor.qMaxLpm).toBe(150);
      expect(sensor.multiplier).toBe(10);
      expect(sensor.connected).toBe(true);
    }
  });

  it("clears only the session volume for a session reset", () => {
    const table = createSensorTable();
    const cleared = resetSession(table);
    for (const sensor of cleared) {
      expect(sensor.sessionLiters).toBe(0);
      expect(sensor.cumulativeLiters).toBeGreaterThan(0);
      expect(sensor.maxFlowLpm).toBeGreaterThan(0);
    }
  });
});

describe("resetting the peak only", () => {
  it("clears max flow and keeps every volume", () => {
    /**
     * P4's own reset. The point of a separate helper is what it does NOT touch: the two resets that
     * could previously reach the peak each destroy something persistent to get at a number a power
     * cycle clears for free.
     */
    const cleared = resetMaxFlow(createSensorTable());
    for (const sensor of cleared) {
      expect(sensor.maxFlowLpm).toBe(0);
      expect(sensor.sessionLiters).toBeGreaterThan(0);
      expect(sensor.cumulativeLiters).toBeGreaterThan(0);
      expect(sensor.instantFlowLpm).toBeGreaterThan(0);
    }
  });

  it("is the only reset that keeps the session volume", () => {
    const table = createSensorTable();
    // Stated as a contrast, because the distinction is the whole reason it exists: a session reset takes
    // the peak with it, so before this there was no way to clear one without the other.
    expect(resetSession(table)[0].sessionLiters).toBe(0);
    expect(resetMaxFlow(table)[0].sessionLiters).toBeGreaterThan(0);
    expect(resetMeasured(table)[0].cumulativeLiters).toBe(0);
    expect(resetMaxFlow(table)[0].cumulativeLiters).toBeGreaterThan(0);
  });
});

describe("resetting one channel's calibration", () => {
  it("clears the five calibration fields and keeps every measurement", () => {
    /**
     * The meter swap: a broken sensor replaced by one with different characteristics. The calibration
     * describes the meter, so it goes; the volume was measured truthfully before the failure, so it
     * stays. Mirrors `OFF_CMD_RESET_CALIBRATION`, whose host test asserts the same split field for field.
     */
    const table = createSensorTable();
    const after = resetCalibration(table, 3);
    const channel = sensorAt(after, 3);
    if (!channel) throw new Error("channel 3 should exist in an eight-row table");

    expect(channel.qMaxLpm).toBe(0);
    expect(channel.multiplier).toBe(0);
    expect(channel.adjust).toBe(0);
    expect(channel.pulsesPerLitre).toBe(0);
    expect(channel.calibration).toBe(0);

    // The contrast, stated the way resetMaxFlow's tests state theirs: what SURVIVES is the requirement.
    const before = sensorAt(table, 3);
    if (!before) throw new Error("channel 3 should exist before the reset too");
    expect(channel.cumulativeLiters).toBe(before.cumulativeLiters);
    expect(channel.cumulativeLiters).toBeGreaterThan(0);
    expect(channel.sessionLiters).toBe(before.sessionLiters);
    expect(channel.sessionLiters).toBeGreaterThan(0);
    expect(channel.maxFlowLpm).toBe(before.maxFlowLpm);
    expect(channel.maxFlowLpm).toBeGreaterThan(0);
    expect(channel.connected).toBe(true);
  });

  it("makes the channel render SET?, which is the whole visible effect", () => {
    /**
     * `resolveSensorMetric` decides `SET?` from the `ready` FLAG, while the firmware derives readiness
     * from `configIsValid` and so flips the moment the config clears. The helper therefore has to set
     * `ready` itself — and this is the test that would fail if it stopped, which is the only reason the
     * simulator and the device agree about what a just-swapped channel looks like.
     */
    const after = resetCalibration(createSensorTable(), 3);
    expect(resolveSensorBinding("sensor.3.status", after, 3)).toBe("SET?");
    expect(resolveSensorBinding("sensor.3.instantFlow", after, 3)).toBe("SET?");
    // And it is the peak's row too, not only the live one: every metric on an unset channel says so.
    expect(resolveSensorBinding("sensor.3.maxFlow", after, 3)).toBe("SET?");
    // `normalizeSensor` owns this, not the helper: a channel that cannot produce a reading reads 0.
    const channel = sensorAt(after, 3);
    expect(channel?.instantFlowLpm).toBe(0);
  });

  it("touches exactly one channel", () => {
    // The wrong-channel assertion, and the reason the helper takes a number at all: the other three
    // resets map the whole table, so a per-channel one that did the same would be indistinguishable
    // from them in every test that only looked at the channel it aimed for.
    const after = resetCalibration(createSensorTable(), 3);
    for (const sensor of after) {
      if (sensor.number === 3) continue;
      expect(sensor.qMaxLpm).toBeGreaterThan(0);
      expect(sensor.ready).toBe(true);
      expect(resolveSensorBinding(`sensor.${sensor.number}.status`, after, sensor.number)).toBe("OK");
    }
  });

  it("changes nothing for a channel number no row carries", () => {
    // 0 is the navigator's "no sensor level was entered" sentinel, and the firmware handler returns
    // before writing anything when it sees it. Both ends have to agree that it is a no-op.
    const table = createSensorTable();
    for (const number of [0, 9, -1]) {
      const after = resetCalibration(table, number);
      expect(after.map((s) => s.qMaxLpm)).toEqual(table.map((s) => s.qMaxLpm));
      expect(after.every((s) => s.ready)).toBe(true);
    }
  });
});
