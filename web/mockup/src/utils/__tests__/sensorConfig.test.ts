import { describe, expect, it } from "vitest";
import {
  advanceSensorTick,
  createSensorTable,
  isPerSensorSetting,
  kSensorCount,
  mayUndersample,
  pulsesForFlow,
  resolveSensorBinding,
  sensorAt,
  sensorIndexForScreen,
  setSensor,
  warningSensorNumbers,
  type SimulatedSensor
} from "../sensorConfig";
import manifest from "../../data/actionManifest.json";
import type { FirmwareValueDefinition } from "../../types/firmwareActions";

const values = manifest.values as FirmwareValueDefinition[];
const valueById = (id: string) => values.find((value) => value.id === id);

/** The real navigation chain to a per-sensor editor, read off screens.json. */
const kChainToSensor3 = [
  "info-p7-enter-config",
  "config-c7-sensor-select",
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
    expect(resolveSensorBinding("sensor.3.instantFlow", table, 0)).toBe("3:   2.34 L/s");
    expect(resolveSensorBinding("sensor.3.maxFlowSinceReset", table, 0)).toBe("3:   2.34 L/s");
    expect(resolveSensorBinding("sensor.3.cumulativeLiters", table, 0)).toBe("3: 123.45 L");
    expect(resolveSensorBinding("sensor.3.sessionLiters", table, 0)).toBe("3: 123.45 L");
    expect(resolveSensorBinding("sensor.3.cumulativeM3", table, 0)).toBe("3:   0.12 m^3");
    expect(resolveSensorBinding("sensor.3.sessionM3", table, 0)).toBe("3:   0.12 m^3");
  });

  it("keeps the eight rows independent", () => {
    const table = setSensor(createSensorTable(), 1, { connected: false });
    expect(resolveSensorBinding("sensor.1.instantFlow", table, 0)).toBe("1: --");
    expect(resolveSensorBinding("sensor.2.instantFlow", table, 0)).toBe("2:   2.34 L/s");
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
  const zeroFlow = setSensor(table, 4, { instantFlowLps: 0 });
  const notReady = setSensor(table, 4, { ready: false });
  const disconnected = setSensor(table, 4, { connected: false });

  it("prints a metric with the 1-based sensor number and a six-wide value", () => {
    expect(resolveSensorBinding("sensor.4.instantFlow", flowing, 0)).toBe("4:   2.34 L/s");
    expect(resolveSensorBinding("sensor.4.cumulativeLiters", flowing, 0)).toBe("4: 123.45 L");
  });

  it("prints zero flow as a measurement, not as a withheld value", () => {
    expect(resolveSensorBinding("sensor.4.instantFlow", zeroFlow, 0)).toBe("4:   0.00 L/s");
    expect(resolveSensorBinding("sensor.4.status", zeroFlow, 0)).toBe("OK");
  });

  it("distinguishes not-ready from both connected and disconnected", () => {
    expect(resolveSensorBinding("sensor.4.instantFlow", notReady, 0)).toBe("4: WAIT");
    expect(resolveSensorBinding("sensor.4.status", notReady, 0)).toBe("WAIT");
  });

  it("prints a disconnected channel's number and dashes — never hidden, never zero", () => {
    const rendered = resolveSensorBinding("sensor.4.cumulativeLiters", disconnected, 0);
    expect(rendered).toBe("4: --");
    expect(rendered).not.toBe("4:   0.00 L");
    expect(resolveSensorBinding("sensor.4.status", disconnected, 0)).toBe("--");
  });

  it("renders a status without the sensor number and a metric with it", () => {
    expect(resolveSensorBinding("sensor.4.status", disconnected, 0)).toBe("--");
    expect(resolveSensorBinding("sensor.4.instantFlow", disconnected, 0)).toBe("4: --");
    expect(resolveSensorBinding("sensor.4.status", notReady, 0)).toBe("WAIT");
    expect(resolveSensorBinding("sensor.4.instantFlow", notReady, 0)).toBe("4: WAIT");
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
    expect(sensorIndexForScreen("config-sensor-3", ["info-p7-enter-config", "config-c7-sensor-select"])).toBe(0);
  });

  it("reports 0 when no sensor is implied", () => {
    expect(sensorIndexForScreen("info-p0-global-status", [])).toBe(0);
    expect(sensorIndexForScreen("config-c1-modbus-id", ["info-p7-enter-config"])).toBe(0);
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
    // multiplier 10, adjust 0: 1200 pulses in 1 s is 120 L/min, so 2 L/s.
    const next = advanceSensorTick({ ...sensor, maxFlowLps: 0 }, { pulses: 1200, elapsedMs: 1000 });
    expect(next.instantFlowLps).toBeCloseTo(2, 10);
    expect(next.sessionLiters).toBeCloseTo(sensor.sessionLiters + 2, 10);
    expect(next.cumulativeLiters).toBeCloseTo(sensor.cumulativeLiters + 2, 10);
    expect(next.maxFlowLps).toBeCloseTo(2, 10);
  });

  it("clamps flow to q_max and never below zero", () => {
    const sensor = sensorAt(createSensorTable(), 1) as SimulatedSensor;
    const fast = advanceSensorTick(sensor, { pulses: 100000, elapsedMs: 1000 });
    expect(fast.instantFlowLps).toBeCloseTo(sensor.qMaxLpm / 60, 10);

    const backwards = advanceSensorTick({ ...sensor, adjust: 5000 }, { pulses: 10, elapsedMs: 1000 });
    expect(backwards.instantFlowLps).toBe(0);
    expect(backwards.cumulativeLiters).toBe(sensor.cumulativeLiters);
  });

  it("keeps the peak when a later tick is slower", () => {
    const sensor = sensorAt(createSensorTable(), 1) as SimulatedSensor;
    const fast = advanceSensorTick(sensor, { pulses: 1200, elapsedMs: 1000 });
    const slow = advanceSensorTick(fast, { pulses: 60, elapsedMs: 1000 });
    expect(slow.instantFlowLps).toBeCloseTo(0.1, 10);
    expect(slow.maxFlowLps).toBeCloseTo(fast.maxFlowLps, 10);
  });

  it("leaves a disconnected channel's totals frozen, however many pulses arrive", () => {
    const table = setSensor(createSensorTable(), 5, { connected: false });
    const before = sensorAt(table, 5) as SimulatedSensor;
    const after = advanceSensorTick(before, { pulses: 5000, elapsedMs: 1000 });

    expect(after.cumulativeLiters).toBe(123.45);
    expect(after.sessionLiters).toBe(123.45);
    expect(after.maxFlowLps).toBe(2.34);
    expect(after.instantFlowLps).toBe(0);
    // And the row still renders as withheld rather than as zero.
    expect(resolveSensorBinding("sensor.5.cumulativeLiters", [after], 0)).toBe("5: --");
  });

  it("accumulates nothing for an enabled channel that is not ready or has no multiplier", () => {
    const sensor = sensorAt(createSensorTable(), 1) as SimulatedSensor;

    const waiting = advanceSensorTick({ ...sensor, ready: false }, { pulses: 1200, elapsedMs: 1000 });
    expect(waiting.instantFlowLps).toBe(0);
    expect(waiting.cumulativeLiters).toBe(sensor.cumulativeLiters);
    expect(waiting.sessionLiters).toBe(sensor.sessionLiters);

    const unconfigured = advanceSensorTick({ ...sensor, multiplier: 0 }, { pulses: 1200, elapsedMs: 1000 });
    expect(unconfigured.instantFlowLps).toBe(0);
    expect(unconfigured.cumulativeLiters).toBe(sensor.cumulativeLiters);
  });

  it("treats a non-positive interval as no time having passed", () => {
    const sensor = sensorAt(createSensorTable(), 1) as SimulatedSensor;
    expect(advanceSensorTick(sensor, { pulses: 1200, elapsedMs: 0 })).toEqual(sensor);
    expect(advanceSensorTick(sensor, { pulses: 1200, elapsedMs: -1000 })).toEqual(sensor);
    expect(advanceSensorTick(sensor, oneSecond).instantFlowLps).toBe(0);
  });

  it("drives a target flow through the pulse count the device would have counted", () => {
    const sensor = sensorAt(createSensorTable(), 1) as SimulatedSensor;
    const pulses = pulsesForFlow(sensor, 2.34, 1000);
    expect(advanceSensorTick(sensor, { pulses, elapsedMs: 1000 }).instantFlowLps).toBeCloseTo(2.34, 10);
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
