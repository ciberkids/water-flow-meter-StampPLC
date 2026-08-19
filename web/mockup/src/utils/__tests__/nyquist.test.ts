/**
 * The sampling ceiling, and whether the mockup and the firmware agree about it.
 *
 * `SimulatedSensor.undersampling` used to be a hand-set checkbox. Nothing derived it, so the mockup could
 * draw the warning but could not draw a CONFIGURATION CAUSING one — the state an operator actually meets,
 * and the state the firmware's whole §5.5 handshake exists for. It is derived now, which means the rule
 * lives in two languages and the interesting question stops being "does the TypeScript work" and becomes
 * "does it still say what the C++ says".
 *
 * So the constant and both ceiling expressions are READ OUT OF THE FIRMWARE SOURCE here, the way
 * `editorRamp.test.ts` reads `ui_accel.h` for the acceleration tiers. A second copy of a number that
 * nothing compares is not a mirror; it is a coincidence with a comment.
 *
 * WHAT IS DELIBERATELY NOT ASSERTED: that the rule is RIGHT. Several cases below pin behaviour this file's
 * own comments call broken — a negative multiplier passing the check, an offset ten times the reachable
 * frequency being accepted. Those are firmware defects, reported separately. A mockup that is more correct
 * than the device shows an operator a state the device will not show them, which is the same class of lie
 * as showing too few, so the mirror reproduces them and pins them as divergences from what is SANE rather
 * than as divergences from the code.
 */
import fs from "node:fs";
import path from "node:path";
import { describe, expect, it } from "vitest";
import {
  configIsValid,
  createSensorTable,
  deriveUndersampling,
  kDefaultPollingRateKhz,
  kSamplingMarginFactor,
  meetsSamplingLimit,
  pulsesForFlow,
  advanceSensorTick,
  resetCalibration,
  samplingCeilingHz,
  sensorAt,
  setSensor,
  statusSummaryText,
  warningSensorNumbers,
  warningSummaryText,
  type SimulatedSensor
} from "../sensorConfig";

const firmwareSource = (...parts: string[]) =>
  fs.readFileSync(
    path.join(__dirname, "..", "..", "..", "..", "..", "Water-Flow-Meter-PlatformIO", "src", ...parts),
    "utf-8"
  );

/** One channel of a fresh table, patched — so `normalizeSensor`'s rules apply as they do in the app. */
function channel(patch: Partial<Omit<SimulatedSensor, "number">>): SimulatedSensor {
  return sensorAt(setSensor(createSensorTable(), 3, patch), 3)!;
}

/** A formula-calibrated channel: `F = multiplier*Q + adjust`. */
const formula = (qMaxLpm: number, multiplier: number, adjust = 0) =>
  channel({ calibration: 0, qMaxLpm, multiplier, adjust, pulsesPerLitre: 0 });

/** A pulses-per-litre channel: K pulses per litre, multiplier unused and correctly zero. */
const pulses = (qMaxLpm: number, pulsesPerLitre: number) =>
  channel({ calibration: 1, qMaxLpm, pulsesPerLitre, multiplier: 0, adjust: 0 });

describe("the mockup and the firmware agree on the sampling margin", () => {
  it("declares the same factor as modbus_manager.h", () => {
    const source = firmwareSource("modbus", "modbus_manager.h");
    const declared = /kSamplingMarginFactor\s*=\s*([0-9.]+)/.exec(source);
    expect(declared, "firmware must declare plc::kSamplingMarginFactor").not.toBeNull();
    expect(Number(declared![1])).toBe(kSamplingMarginFactor);
  });

  it("uses that constant in the comparison rather than a second literal", () => {
    // The point of naming it. A `2.0` left inline in `meetsNyquistLimit` would make the assertion above
    // true and meaningless — the header would declare a constant the gate did not consult.
    const source = firmwareSource("modbus", "modbus_manager.cpp");
    const gate = source.slice(source.indexOf("bool ModbusManager::meetsNyquistLimit"));
    const body = gate.slice(0, gate.indexOf("\n}\n"));
    expect(
      /pollingHz\s*>=\s*\(\s*plc::kSamplingMarginFactor\s*\*\s*theoreticalFrequency\s*\)/.test(body),
      "meetsNyquistLimit must compare against plc::kSamplingMarginFactor"
    ).toBe(true);
    expect(/\b2\.0\s*\*/.test(body), "no bare 2.0 factor may remain in the comparison").toBe(false);
  });
});

describe("the ceiling is the state engine's own inversion, evaluated at q_max", () => {
  /**
   * Read from `sensor_state_engine.cpp`, NOT from `meetsNyquistLimit`'s comment about it.
   *
   * The engine is the authority: it is the code that turns a frequency into a flow, so the highest
   * frequency the device expects is whatever frequency its formula maps to `q_max`. Checking the ceiling
   * against the gate's own comment would pass with both of them wrong together, which is exactly how the
   * pulses-per-litre form came to have no ceiling at all.
   */
  it("reads both inversions out of sensor_state_engine.cpp", () => {
    const engine = firmwareSource("sensors", "sensor_state_engine.cpp");
    expect(
      /flowRateLpm\s*=\s*frequency\s*\*\s*60\.0f\s*\/\s*static_cast<float>\(config\.pulses_per_litre\)/.test(
        engine
      ),
      "engine must invert the pulses form as F * 60 / K"
    ).toBe(true);
    expect(
      /flowRateLpm\s*=\s*\(frequency\s*-\s*config\.adjust\)\s*\/\s*config\.f_multiplier/.test(engine),
      "engine must invert the formula as (F - adjust) / f_multiplier"
    ).toBe(true);
  });

  it("computes the same two ceilings the firmware gate computes", () => {
    const gate = firmwareSource("modbus", "modbus_manager.cpp");
    const body = gate.slice(gate.indexOf("bool ModbusManager::meetsNyquistLimit"));
    // K * q_max / 60 — the operands may be cast, so the shape is what is pinned, not the spelling.
    expect(
      /pulses_per_litre\)\s*\*\s*static_cast<double>\(cfg\.q_max\)\s*\/\s*60\.0/.test(body),
      "the pulses ceiling must be K * q_max / 60"
    ).toBe(true);
    expect(
      /f_multiplier\)\s*\*\s*static_cast<double>\(cfg\.q_max\)\s*\+/.test(body),
      "the formula ceiling must be f_multiplier * q_max + adjust"
    ).toBe(true);
  });

  it("gets the UNITS right: K is per litre and q_max is per minute", () => {
    // 450 pulses per litre at 100 L/min is 450 * 100 / 60 = 750 pulses per SECOND. Dropping the 60 would
    // give 45000 and flag every real meter; inverting it would give 270000. This is the arithmetic
    // 77c1252's commit body claims, checked rather than repeated.
    expect(samplingCeilingHz(pulses(100, 450))).toBe(750);
    expect(samplingCeilingHz(pulses(1000, 450))).toBe(7500);
    // The formula's multiplier is Hz per L/min by construction, so no 60 belongs in it.
    expect(samplingCeilingHz(formula(150, 10))).toBe(1500);
    expect(samplingCeilingHz(formula(150, 10, 200))).toBe(1700);
  });

  it("has NO ceiling where the firmware returns false before computing one", () => {
    // modbus_manager.cpp:734-762 — q_max is fatal to both forms, each divisor only to its own.
    expect(samplingCeilingHz(formula(0, 10))).toBeNull();
    expect(samplingCeilingHz(pulses(0, 450))).toBeNull();
    expect(samplingCeilingHz(formula(150, 0))).toBeNull();
    expect(samplingCeilingHz(pulses(150, 0))).toBeNull();
    // And a missing divisor is fatal only to the form that uses it: this is 77c1252's fix. A pulses
    // channel has f_multiplier == 0 as its NORMAL state, and used to fail a check it could not pass.
    expect(samplingCeilingHz(pulses(100, 450))).not.toBeNull();
    expect(meetsSamplingLimit(pulses(100, 450), 3.3)).toBe(true);
  });
});

describe("the verdict at a given polling rate", () => {
  it("accepts a channel inside budget and refuses one outside it, in both forms", () => {
    // The pair 77c1252 pinned in C++, now pinned across the boundary at the same rate.
    expect(meetsSamplingLimit(pulses(100, 450), 3.3)).toBe(true);    //  750 Hz, needs 1500
    expect(meetsSamplingLimit(pulses(1000, 450), 3.3)).toBe(false);  // 7500 Hz, needs 15000
    expect(meetsSamplingLimit(formula(150, 10), 3.3)).toBe(true);    // 1500 Hz, needs 3000
    expect(meetsSamplingLimit(formula(150, 12), 3.3)).toBe(false);   // 1800 Hz, needs 3600
  });

  it("needs the factor's worth of headroom, not merely more samples than pulses", () => {
    // 1600 Hz against a 3.3 kHz sampler is over two samples per period and passes; 1700 Hz is not and
    // does not. A gate that had dropped the factor would accept both, and would accept anything under
    // 3300 Hz — which is the whole difference between "sampled" and "counted once in a while".
    expect(meetsSamplingLimit(formula(160, 10), 3.3)).toBe(true);
    expect(meetsSamplingLimit(formula(170, 10), 3.3)).toBe(false);
    expect(kSamplingMarginFactor * 1700).toBeGreaterThan(3300);
  });

  it("refuses everything valid at rate 0 — the boot window", () => {
    /**
     * `pollingRate_kHz` is 0.0f from boot until the sampler's first one-second window closes
     * (firmware.cpp:95, 656-660), and the Modbus server is already accepting frames by then
     * (`modbus.begin` runs at the top of the logic task). So this is a REAL device state, not a
     * hypothetical: every calibrated channel flags, and a master's config write is refused and parked.
     * Reachable in the simulator because the dial goes to 0, which is the point of it being a dial.
     */
    expect(meetsSamplingLimit(formula(150, 10), 0)).toBe(false);
    expect(meetsSamplingLimit(pulses(100, 450), 0)).toBe(false);
    const table = deriveUndersampling(createSensorTable(), 0);
    expect(warningSensorNumbers(table)).toEqual([1, 2, 3, 4, 5, 6, 7, 8]);
  });

  it("starts the dial at the assumed rate, and says so", () => {
    // G1: 3.3 kHz has never been measured. The constant exists so the assumption has one home and can be
    // moved when a board reports a real figure — not because 3.3 is known to be true.
    expect(kDefaultPollingRateKhz).toBe(3.3);
    expect(warningSensorNumbers(deriveUndersampling(createSensorTable(), kDefaultPollingRateKhz))).toEqual(
      []
    );
  });
});

describe("the flag is derived, which is the whole change", () => {
  it("lights from a configuration alone, with no checkbox touched", () => {
    // The reported gap: `SimulatedSensor.undersampling` was hand-set, so no configuration could raise it.
    const table = deriveUndersampling(setSensor(createSensorTable(), 3, { multiplier: 40 }), 3.3);
    expect(sensorAt(table, 3)?.undersampling).toBe(true);
    expect(warningSensorNumbers(table)).toEqual([3]);
    // 40 * 150 = 6000 Hz against 3300 — and the ceiling is what says so.
    expect(samplingCeilingHz(sensorAt(table, 3)!)).toBe(6000);
  });

  it("clears again when the configuration is brought back inside budget", () => {
    // The half a checkbox could never do: a stale warning on a channel that is now fine.
    let stored = setSensor(createSensorTable(), 3, { multiplier: 40 });
    expect(sensorAt(deriveUndersampling(stored, 3.3), 3)?.undersampling).toBe(true);
    stored = setSensor(stored, 3, { multiplier: 10 });
    expect(sensorAt(deriveUndersampling(stored, 3.3), 3)?.undersampling).toBe(false);
  });

  it("follows the RATE too, with the configuration untouched", () => {
    const stored = createSensorTable();  // 150 L/min at x10 = 1500 Hz
    expect(warningSensorNumbers(deriveUndersampling(stored, 3.3))).toEqual([]);
    expect(warningSensorNumbers(deriveUndersampling(stored, 2.9))).toEqual([1, 2, 3, 4, 5, 6, 7, 8]);
  });

  it("raises the pulses-per-litre form as well, which had no ceiling until 77c1252", () => {
    const stored = setSensor(createSensorTable(), 5, { calibration: 1, pulsesPerLitre: 4000 });
    const table = deriveUndersampling(stored, 3.3);
    // 4000 p/L at 150 L/min = 10000 Hz.
    expect(samplingCeilingHz(sensorAt(table, 5)!)).toBe(10000);
    expect(warningSensorNumbers(table)).toEqual([5]);
    // And a sane K on the same channel does not flag, so the form is CHECKED rather than exempted.
    expect(
      warningSensorNumbers(
        deriveUndersampling(setSensor(stored, 5, { pulsesPerLitre: 450 }), 3.3)
      )
    ).toEqual([]);
  });

  it("counts an UNCALIBRATED channel as uncalibrated, never as undersampling", () => {
    // `evaluateSensorDiagnostics` ORs `valid && !meets`, so an invalid config cannot flag on its own —
    // and it must not, or a channel at `SET?` would report a sampling fault it has no figures to have.
    const stored = setSensor(createSensorTable(), 4, { qMaxLpm: 0, ready: false });
    const table = deriveUndersampling(stored, 3.3);
    expect(configIsValid(sensorAt(table, 4)!)).toBe(false);
    expect(warningSensorNumbers(table)).toEqual([]);
    expect(statusSummaryText(table)).toBe("1 channel not calibrated");
  });

  it("zeroes the flow of a channel with no q_max, which the Values panel can reach", () => {
    /**
     * ONE definition of "can this configuration produce a reading", now shared by the flow row and the
     * sampling derivation. `normalizeSensor` used to test the calibration form's field alone, so writing
     * `q_max = 0` through the panel left the channel reporting 140.4 L/min — the engine's gate is
     * `configIsValid(config)`, which refuses that channel and zeroes it (sensor_state_engine.cpp:30, 69).
     * Worth pinning here rather than beside the flag: a channel the derivation calls unconfigured while
     * the row beside it prints a flow is the self-contradiction the shared predicate exists to prevent.
     */
    const sensor = channel({ qMaxLpm: 0, multiplier: 10, instantFlowLpm: 140.4 });
    expect(configIsValid(sensor)).toBe(false);
    expect(sensor.instantFlowLpm).toBe(0);
    expect(samplingCeilingHz(sensor)).toBeNull();
  });

  it("never flags a DISCONNECTED channel, however it is configured", () => {
    const stored = setSensor(createSensorTable(), 2, { connected: false, multiplier: 40 });
    expect(warningSensorNumbers(deriveUndersampling(stored, 3.3))).toEqual([]);
    // Not even with the override ticked: the firmware's `!inUse` continue comes first.
    const overridden = setSensor(stored, 2, { samplingOverride: true });
    expect(sensorAt(deriveUndersampling(overridden, 3.3), 2)?.undersampling).toBe(false);
  });
});

describe("the §5.5 override is the arm that stays an input", () => {
  it("flags a channel that is inside budget, because the operator was warned and proceeded", () => {
    const stored = setSensor(createSensorTable(), 6, { samplingOverride: true });
    const table = deriveUndersampling(stored, 3.3);
    expect(meetsSamplingLimit(sensorAt(table, 6)!, 3.3)).toBe(true);
    expect(sensorAt(table, 6)?.undersampling).toBe(true);
    expect(warningSummaryText(table)).toBe("Sampling warning on 1 sensor");
  });

  it("is cleared by S.RESET, because the confirmation described the OLD meter", () => {
    // modbus_manager.cpp:362-364 clears both override arms beside the config (and offset 19 clears the
    // same three since 2026-08-17). Left standing, the flag would survive on a channel with nothing to
    // undersample — it would NOT wave the next meter's first figures through unchecked, because
    // `prepareConfigUpdate` clears the flags for any candidate failing `configIsValid` and the first write
    // onto an all-zero config is necessarily one. Now the flag is derived, omitting it would be VISIBLE.
    const stored = setSensor(createSensorTable(), 6, { samplingOverride: true });
    const after = resetCalibration(stored, 6);
    expect(sensorAt(after, 6)?.samplingOverride).toBe(false);
    expect(warningSensorNumbers(deriveUndersampling(after, 3.3))).toEqual([]);
  });
});

describe("the warning the panel paints from the flag", () => {
  it("counts the channels when sampling is the only fault", () => {
    const table = deriveUndersampling(setSensor(createSensorTable(), 3, { multiplier: 40 }), 3.3);
    // A COUNT, not a list, and singular at one — the wording the firmware composes in
    // `UiController::update`. Naming them cost 28 characters of prefix and overflowed the banner at four.
    expect(warningSummaryText(table)).toBe("Sampling warning on 1 sensor");
    expect(statusSummaryText(table)).toBe("1 warning");
  });

  it("drops the noun \"channels\" when a commissioning gap shares the screen", () => {
    let stored = setSensor(createSensorTable(), 3, { multiplier: 40 });
    stored = setSensor(stored, 7, { qMaxLpm: 0, ready: false });
    const table = deriveUndersampling(stored, 3.3);
    expect(warningSummaryText(table)).toBe("1 not calibrated, 1 undersampling");
    expect(statusSummaryText(table)).toBe("1 channel not calibrated | 1 warning");
  });

  /**
   * WHAT THIS DOES NOT SHOW. Two firmware paths carry this fact and only one of them is derived here.
   *
   * The live FLAG is `evaluateSensorDiagnostics`, mirrored above, and it feeds these two strings and the
   * `config.sensor.undersamplingFlag` row. The `nyquist-warning` SCREEN — "Sampling too slow. UP=Edit
   * DOWN=Save anyway" (ui_bindings.cpp:728) — is editor state, raised when `prepareConfigUpdate` REFUSES a
   * write, and the simulator has no write gate to refuse one. So an out-of-budget configuration lights the
   * flag here; it does not push that screen.
   *
   * What used to be the second caveat here is fixed: on hardware the banner is now drawn at §2c's y=116,
   * over the footer row, and it paints AFTER the screen rather than being overpainted by the screen's own
   * rows. What survives is the caveat about this SCREEN: `nyquist-warning` is in no flow's
   * targetScreenId and is named in no `ui_pages.h` table, so UiScreenRouter can never resolve it and the
   * device cannot show it. The live surface is the in-place `config.sensor.nyquistWarning` text element on
   * each edit screen. So "nothing here should be read as 'the operator sees this'" still holds — for the
   * reachability reason, not the painting one.
   */
  it("resolves the per-channel row from the derived bit", () => {
    const table = deriveUndersampling(setSensor(createSensorTable(), 3, { multiplier: 40 }), 3.3);
    expect(sensorAt(table, 3)?.undersampling).toBe(true);
    expect(sensorAt(table, 4)?.undersampling).toBe(false);
  });
});

describe("the ceiling is a limit something can actually be driven to", () => {
  it("round-trips through pulsesForFlow at q_max, in both forms", () => {
    /**
     * The ceiling is only meaningful if it is the frequency the channel produces AT FULL FLOW. Driving
     * `pulsesForFlow` to `q_max` over one second must therefore yield exactly it — otherwise the gate is
     * budgeting against a number the device never reaches, which is a plausible-looking way to be wrong.
     */
    for (const sensor of [formula(150, 10), formula(150, 10, 200), pulses(100, 450), pulses(1000, 450)]) {
      expect(pulsesForFlow(sensor, sensor.qMaxLpm, 1000)).toBeCloseTo(samplingCeilingHz(sensor)!, 6);
    }
  });

  it("reads back the driving flow at HALF scale, in both forms", () => {
    /**
     * And the engine's half of the round trip. HALF of q_max deliberately, not q_max: the engine clamps
     * at `q_max`, so an assertion at full scale is satisfied by any wrong answer that is too big —
     * including the `Infinity` a formula-only tick produces on a pulses channel, whose multiplier is 0.
     * That is not a hypothetical: it is exactly the bug this round fixed, and asserting at q_max let the
     * mutation back in undetected.
     */
    for (const sensor of [formula(150, 10), formula(150, 10, 200), pulses(100, 450), pulses(1000, 450)]) {
      const half = sensor.qMaxLpm / 2;
      const pulseCount = pulsesForFlow(sensor, half, 1000);
      const ticked = advanceSensorTick(sensor, { pulses: pulseCount, elapsedMs: 1000 });
      expect(ticked.instantFlowLpm).toBeCloseTo(half, 4);
      expect(ticked.instantFlowLpm).toBeLessThan(sensor.qMaxLpm);
    }
  });

  it("drives a pulses-per-litre channel at all, which it could not before", () => {
    // `advanceSensorTick` gated on `multiplier === 0` and used the formula unconditionally, so the form
    // the default row describes read 0 L/min forever. Deriving the flag made that a contradiction: the
    // channel would have flagged for outrunning the sampler while displaying no flow.
    //
    // BELOW q_max for the reason above — 562.5 Hz is 75 L/min on a 450 p/L meter, and the formula branch
    // would divide by a zero multiplier and be clamped to 150 instead.
    const sensor = pulses(150, 450);
    const ticked = advanceSensorTick(sensor, { pulses: 562.5, elapsedMs: 1000 });
    expect(ticked.instantFlowLpm).toBeCloseTo(75, 6);  // 562.5 Hz * 60 / 450
    expect(ticked.sessionLiters).toBeGreaterThan(sensor.sessionLiters);
    // A litre count that follows from the rate, not from a clamp: 75 L/min for 1 s is 1.25 L.
    expect(ticked.sessionLiters - sensor.sessionLiters).toBeCloseTo(1.25, 6);
  });
});

describe("the two degenerate configurations the gate used to wave through", () => {
  /**
   * Both were audit findings, both are now refused, and both are pinned HERE because the mockup is where
   * a reviewer can see the operator's side of them: a channel reporting OK, register 30 clear, and a
   * reading that has nothing to do with the water.
   *
   * The C++ is read as well as mirrored. These two arms are one-word changes — `return true` to
   * `return false`, `!= 0` to `< 1` — so a revert would be invisible to a test that only exercised the
   * TypeScript copy.
   */
  it("refuses a NEGATIVE multiplier, which read zero forever while reporting OK", () => {
    const sensor = formula(150, -10);
    // `configIsValid` now demands a positive multiplier, so the channel is not merely unsamplable — it is
    // not configured. That is the state the panel already refused to let an operator reach.
    expect(configIsValid(sensor)).toBe(false);
    expect(meetsSamplingLimit(sensor, 3.3)).toBe(false);
    // NOT an undersampling warning: `evaluateSensorDiagnostics` ORs in `valid && !meets`, and it is not
    // valid, so the channel reads SET? rather than wearing a sampling fault it cannot have.
    expect(warningSensorNumbers(deriveUndersampling([sensor], 3.3))).toEqual([]);
    // The engine is unchanged and still reads nothing — which is now unreachable through any write.
    expect(advanceSensorTick(sensor, { pulses: 5000, elapsedMs: 1000 }).instantFlowLpm).toBe(0);
  });

  it("refuses a ceiling of zero or less instead of calling it samplable", () => {
    // `adjust` cancelling the whole span is a configuration, not an absence, so it reaches the ceiling arm
    // rather than the null one. That arm returned TRUE, on the same grounds the `q_max == 0` arm returns
    // false — the inconsistency was the defect.
    const cancelled = formula(150, 100, -15000);
    expect(samplingCeilingHz(cancelled)).toBe(0);
    expect(meetsSamplingLimit(cancelled, 3.3)).toBe(false);
    expect(meetsSamplingLimit(cancelled, 0)).toBe(false);
    // And a ceiling driven NEGATIVE is no longer flattened to zero on the way in.
    expect(samplingCeilingHz(formula(150, 10, -15000))).toBe(-13500);
    expect(meetsSamplingLimit(formula(150, 10, -15000), 3.3)).toBe(false);
  });

  it("reads both corrected arms out of the firmware, not just out of this module", () => {
    const gate = firmwareSource("modbus", "modbus_manager.cpp");
    const body = gate.slice(gate.indexOf("bool ModbusManager::meetsNyquistLimit"));
    const arm = /if\s*\(theoreticalFrequency\s*<=\s*0\.0\)\s*\{\s*return\s+(true|false);/.exec(body);
    expect(arm, "meetsNyquistLimit must still have a non-positive-ceiling arm").not.toBeNull();
    expect(arm![1], "a ceiling of zero or less must be REFUSED, not accepted").toBe("false");
    // The clamp that used to hide a negative ceiling from that arm must stay gone.
    expect(/std::max\(0\.0,/.test(body), "no clamp may flatten a negative ceiling to zero").toBe(false);

    const types = firmwareSource("modbus", "sensor_types.h");
    const predicate = types.slice(types.indexOf("inline bool configIsValid"));
    expect(
      /if\s*\(cfg\.f_multiplier\s*<\s*1\)\s*\{\s*return\s+false;/.test(predicate),
      "configIsValid must demand a POSITIVE multiplier, not merely a non-zero one"
    ).toBe(true);
    expect(
      /cfg\.f_multiplier\s*==\s*0/.test(predicate),
      "the old `!= 0`-only test must not come back"
    ).toBe(false);
  });
});

describe("divergences from SANE that are STILL mirrored on purpose", () => {
  /**
   * A firmware looseness reproduced here rather than fixed, because a mockup that refused what the device
   * accepts would show a warning hardware will not show. Pinned so the mirror cannot be "improved"
   * silently — and so that narrowing the C++ bound makes a test fail here and be revisited.
   */
  it("REFUSES an offset ten times the reachable frequency, which it used to accept (DF14)", () => {
    // The bound was `q_max * multiplier * 10` while the comment beside it said the offset "may not exceed
    // the frequency the channel can actually reach" — `q_max * multiplier`, ten times smaller. The code and
    // its own stated rule disagreed, and the code was the looser of the two. DF14 kept the comment's
    // promise, so this magnitude is now invalid whichever sign it carries.
    expect(configIsValid(formula(150, 10, 15000))).toBe(false);
    const sensor = formula(150, 10, -15000);
    expect(configIsValid(sensor)).toBe(false);
    // The sampling arms are unchanged and still say what they said: this ceiling is unreachable. Nothing
    // about the SAMPLER was wrong in either of these cases, which is why the rule belongs in the
    // configuration predicate.
    expect(meetsSamplingLimit(sensor, 3.3)).toBe(false);
    // And the engine now measures nothing, where it used to read full scale on a dry pipe.
    expect(advanceSensorTick(sensor, { pulses: 0, elapsedMs: 1000 }).instantFlowLpm).toBe(0);
  });

  it("REFUSES a small negative offset, which the ceiling arm could never catch (DF14)", () => {
    // INVERTED, not deleted — the pinning test for the case that decided DF14. At m=10, q=150, a=-1400 the
    // ceiling is a healthy 100 Hz, so every sampling arm passes it: `samplingCeilingHz` is fine,
    // `meetsSamplingLimit` is fine, and no undersampling warning is raised. Nothing about the SAMPLER was
    // ever wrong here. What was wrong is the calibration, and only `configIsValid` can say so — which is
    // why the rule went there and not into the gate.
    const sensor = formula(150, 10, -1400);
    expect(configIsValid(sensor)).toBe(false);
    expect(samplingCeilingHz(sensor)).toBe(100);
    expect(meetsSamplingLimit(sensor, 3.3)).toBe(true);
    expect(warningSensorNumbers(deriveUndersampling([sensor], 3.3))).toEqual([]);
    // 140 L/min on a dry pipe was the harm. An invalid calibration measures nothing instead.
    expect(advanceSensorTick(sensor, { pulses: 0, elapsedMs: 1000 }).instantFlowLpm).toBe(0);
  });

  it("still accepts the offsets a datasheet actually prints", () => {
    // The tightening has a cost and this is its shape: a POSITIVE offset up to the reachable frequency is
    // fine, one beyond it is not, and the ten-times headroom that used to hide the difference is gone.
    expect(configIsValid(formula(150, 10, 0))).toBe(true);
    expect(configIsValid(formula(150, 10, 1500))).toBe(true);   // exactly q_max * multiplier
    expect(configIsValid(formula(150, 10, 1501))).toBe(false);  // one past it — accepted before DF14
    expect(configIsValid(formula(150, 10, -1))).toBe(false);    // and no negative offset at all
  });
});
