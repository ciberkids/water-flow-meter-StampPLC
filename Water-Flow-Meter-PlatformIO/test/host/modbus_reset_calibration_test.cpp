/**
 * Does `OFF_CMD_RESET_CALIBRATION` clear the calibration and KEEP every measurement?
 *
 * The register exists for the meter swap: a broken sensor replaced by one with different
 * characteristics. Everything the old meter measured was true when it measured it, so the volume stays
 * and keeps accumulating; only the figures describing the meter go away. That is a claim about what a
 * command does NOT touch, and the only way to state it is to arm a channel with real readings, issue the
 * command, and assert each surviving field by name.
 *
 * ── WHY THIS IS A SEPARATE BINARY, AND NOT IN interaction_test.cpp ───────────────────────
 *
 * interaction_test.cpp DEFINES `ModbusManager::applyHoldingWrite` itself, so it can drive the buttons
 * without linking eModbus. That stand-in records the address and returns — it never runs the register
 * arm. A test of these semantics added there would assert against the harness and would pass with the
 * production arm deleted, which is exactly the failure modbus_manager_clock_test.cpp was created to
 * avoid. So the UI half is asserted there (the gesture reaches the command, at the right ADDRESS) and
 * the semantic half is asserted here, against the real modbus_manager.cpp.
 *
 * ── AND WHY NOT IN modbus_manager_clock_test.cpp ─────────────────────────────────────────
 *
 * That file's subject is the clock wiring on the reset arms. This command deliberately does not touch
 * the clock — it restarts no session — so it has nothing to say there.
 *
 * CHANNEL 3 THROUGHOUT, never channel 1. A per-sensor command whose address is computed from a 1-based
 * navigator index and a 0-based register slot fails silently on channel 1: the off-by-one and the correct
 * answer are the same address. Every case below therefore arms channels 2, 3 and 4 and asserts the
 * NEIGHBOURS were left alone, which is the only form the wrong-channel assertion can take.
 */
#include <Preferences.h>

#include <cstdio>

#include "led/led_controller.h"
#include "modbus/modbus_manager.h"
#include "modbus/register_bank.h"
#include "modbus/register_map.h"
#include "modbus/sensor_types.h"
#include "time/device_clock.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-76s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) failures++;
}

/**
 * Everything ModbusManager reaches through, owned so each case starts from a clean device.
 *
 * Fully populated for the reason modbus_manager_clock_test.cpp records: `isWritableAddress` dereferences
 * `registers` on its first line and `syncGlobalRegisters` dereferences four more members
 * unconditionally, so a partial struct segfaults before it can assert anything.
 */
struct Device {
  plc::RegisterBank registers;
  SensorData sensors[plc::kNumSensors]{};
  SensorCharacteristics configs[plc::kNumSensors]{};
  Preferences preferences;
  LedController leds;
  plc::DeviceClock clock;
  uint16_t connectedBitmap = 0x0E;  // channels 2, 3 and 4
  uint16_t undersamplingFlags = 0;
  double totalSessionLiters = 0.0;
  double aggregateFlowLpm = 0.0;
  uint16_t displayFlowUnit = 0;
  bool allSensorsReady = false;
  volatile float pollingRateKhz = 3.3f;

  ModbusDependencies deps() {
    ModbusDependencies d;
    d.sensors = sensors;
    d.configs = configs;
    d.preferences = &preferences;
    d.registers = &registers;
    d.ledController = &leds;
    d.clock = &clock;
    d.connectedBitmap = &connectedBitmap;
    d.undersamplingFlags = &undersamplingFlags;
    d.totalSessionLitersCache = &totalSessionLiters;
    d.aggregateFlowLpmCache = &aggregateFlowLpm;
    d.displayFlowUnit = &displayFlowUnit;
    d.allSensorsReadyCache = &allSensorsReady;
    d.pollingRateKhz = &pollingRateKhz;
    d.sensorCount = plc::kNumSensors;
    return d;
  }

  /**
   * One channel, in use, validly calibrated, and carrying volume it has already measured.
   *
   * `inUse` is not optional decoration: both `isWritableAddress` and the per-sensor half of
   * `applyHoldingWrite` refuse every address in a block whose channel is not in use, so without it the
   * command is rejected and a test asserting "the config was cleared" would be asserting that a
   * REFUSED write cleared nothing.
   */
  void armChannel(std::size_t index, uint16_t qMax, double lifetime) {
    sensors[index].inUse = true;
    sensors[index].sessionLiters = 1234.5f;
    sensors[index].cumulativeLiters = lifetime;
    sensors[index].maxFlowSinceReset = 42.0f;
    sensors[index].pulseCount = 777;
    sensors[index].instantFlow_L_min = 12.5f;
    configs[index].q_max = qMax;
    configs[index].f_multiplier = 6;
    configs[index].adjust = 0;
    configs[index].calibration = CalibrationType::Formula;
    configs[index].pulses_per_litre = 0;
  }

  /** The absolute address of a channel's reset-calibration command, 0-BASED index. */
  static uint16_t resetCalAddress(std::size_t index) {
    return static_cast<uint16_t>(plc::sensorBaseAddress(index) + plc::OFF_CMD_RESET_CALIBRATION);
  }

  /** Any offset within a channel's block, 0-BASED index — the config registers this file writes. */
  static uint16_t cfgAddress(std::size_t index, uint16_t offset) {
    return static_cast<uint16_t>(plc::sensorBaseAddress(index) + offset);
  }
};

void clearsTheCalibrationAndKeepsEveryMeasurement() {
  std::printf("\n[the calibration goes, the measurements stay - the meter swap]\n");

  Device dev;
  dev.armChannel(2, 100, 98765.0);  // channel 3
  ModbusManager modbus(dev.deps());

  check(configIsValid(dev.configs[2]), "channel 3 starts validly calibrated");

  check(modbus.applyHoldingWrite(Device::resetCalAddress(2), 1),
        "a write of 1 to channel 3's reset-calibration command is accepted");

  // What went. All five fields, because SensorCharacteristics{} is the whole of "not set" and a reset
  // that cleared four of them would leave a channel half-describing a meter that is gone.
  check(dev.configs[2].q_max == 0, "q_max is back to 0");
  check(dev.configs[2].f_multiplier == 0, "the multiplier is back to 0");
  check(dev.configs[2].adjust == 0, "the adjust term is back to 0");
  check(dev.configs[2].pulses_per_litre == 0, "pulses per litre is back to 0");
  check(dev.configs[2].calibration == CalibrationType::Formula,
        "and the calibration form is back to Formula, the struct's own default");
  check(dev.configs[2] == SensorCharacteristics{},
        "which is exactly a default-constructed SensorCharacteristics");

  /**
   * THE CONTRAST, stated rather than implied — this is the entire reason the command exists.
   *
   * OFF_CMD_RESET_CONFIG (19) assigns `SensorData{}` over this struct and then persists the zeroed
   * lifetime total. Every one of these four assertions would fail against it. They are what separates
   * "the meter was replaced" from "the channel was decommissioned".
   */
  check(dev.sensors[2].cumulativeLiters == 98765.0,
        "the LIFETIME TOTAL is untouched - the old meter's volume was real");
  check(dev.sensors[2].sessionLiters == 1234.5f, "the session volume is untouched");
  check(dev.sensors[2].maxFlowSinceReset == 42.0f, "the peak is untouched");
  check(dev.sensors[2].pulseCount == 777, "and so is the raw pulse count");
  check(dev.sensors[2].inUse, "the channel is still in use - it was not decommissioned");

  // Readiness is DERIVED, so clearing the config is the whole of what makes the panel say SET?.
  check(!configIsValid(dev.configs[2]),
        "and the channel now reports not-ready, which is what renders as SET? on the panel");
}

void actsOnOneChannelAndLeavesItsNeighboursAlone() {
  std::printf("\n[the wrong-channel case: only channel 3 changes]\n");

  Device dev;
  dev.armChannel(1, 60, 111.0);   // channel 2
  dev.armChannel(2, 100, 222.0);  // channel 3, the target
  dev.armChannel(3, 250, 333.0);  // channel 4
  ModbusManager modbus(dev.deps());

  check(modbus.applyHoldingWrite(Device::resetCalAddress(2), 1), "channel 3's command is issued");

  check(!configIsValid(dev.configs[2]), "channel 3 is now unset");
  // The off-by-one this is really testing: a 1-based navigator index used as a 0-based register slot
  // would have produced channel 2's address, and one used the other way round channel 4's.
  check(configIsValid(dev.configs[1]) && dev.configs[1].q_max == 60,
        "channel 2 keeps its calibration - the index was not read one too low");
  check(configIsValid(dev.configs[3]) && dev.configs[3].q_max == 250,
        "channel 4 keeps its calibration - nor one too high");
  check(dev.sensors[1].cumulativeLiters == 111.0 && dev.sensors[3].cumulativeLiters == 333.0,
        "and neither neighbour lost any volume either");
}

void theCommandRegisterSelfClears() {
  std::printf("\n[the command register self-clears, like every other command arm]\n");

  Device dev;
  dev.armChannel(2, 100, 500.0);
  ModbusManager modbus(dev.deps());
  const uint16_t address = Device::resetCalAddress(2);

  check(modbus.applyHoldingWrite(address, 1), "the command is issued");
  check(dev.registers.at(address) == 0,
        "and the register reads back 0, so a master polling it cannot see a stuck command");
}

void onlyAWriteOfOneIsTheCommand() {
  std::printf("\n[a write of 0 is accepted and does nothing, matching the other arms]\n");

  Device dev;
  dev.armChannel(2, 100, 640.0);
  ModbusManager modbus(dev.deps());

  check(modbus.applyHoldingWrite(Device::resetCalAddress(2), 0), "a write of 0 is accepted");
  check(configIsValid(dev.configs[2]) && dev.configs[2].q_max == 100,
        "but clears nothing - the calibration is still there");
}

void anUnwiredChannelRefusesTheCommand() {
  std::printf("\n[a channel that is not in use refuses it, like every address in its block]\n");

  Device dev;
  dev.armChannel(2, 100, 700.0);
  dev.sensors[4].inUse = false;
  dev.configs[4].q_max = 90;  // configured but not wired, which the device permits
  ModbusManager modbus(dev.deps());

  // Not a special case of this command: `isWritableAddress` refuses EVERY per-sensor address on a
  // channel that is not in use, so the panel row is a no-op on an unwired channel exactly as its
  // editors are. Asserted so the behaviour is a decision on record rather than a surprise on a bench.
  check(!modbus.applyHoldingWrite(Device::resetCalAddress(4), 1),
        "channel 5's command is refused while the channel is not in use");
  check(dev.configs[4].q_max == 90, "and its calibration is left exactly as it was");
}

/** Latches a §5.5 Nyquist override on `index` and returns once its flag is lit. */
void latchNyquistOverride(ModbusManager& modbus, Device& dev, std::size_t index) {
  const uint16_t qMaxAddress =
      static_cast<uint16_t>(plc::sensorBaseAddress(index) + plc::OFF_CFG_Q_MAX);
  // Two IDENTICAL writes: the first is refused and parks the candidate, the second repeats it and
  // latches the override. That handshake is the only route to `overrideActive_` from outside the class.
  check(!modbus.applyHoldingWrite(qMaxAddress, 65000),
        "an undersampling figure is refused once and parked for confirmation");
  check(modbus.applyHoldingWrite(qMaxAddress, 65000),
        "and repeating it confirms the override, which latches it");
  check((dev.undersamplingFlags & (1u << index)) != 0, "so the channel's undersampling flag is lit");
}

void clearsTheNyquistOverrideWithTheCalibration() {
  std::printf("\n[the old meter's Nyquist override does not survive the swap]\n");

  Device dev;
  dev.armChannel(2, 1, 800.0);
  dev.configs[2].f_multiplier = 1;
  ModbusManager modbus(dev.deps());
  latchNyquistOverride(modbus, dev, 2);

  check(modbus.applyHoldingWrite(Device::resetCalAddress(2), 1), "the calibration is then reset");

  /**
   * THE UNDERSAMPLING FLAG IS THE PROOF, because `overrideActive_` is private.
   *
   * `evaluateSensorDiagnostics` recomputes the flag as `(valid && !meets) || overrideActive_ ||
   * overridePending_`. After the reset the config is invalid, so the first term is false and the bit can
   * only still be lit if one of the override flags survived. A clear bit is therefore exactly the
   * statement "both override flags were cleared" — and it is the assertion the contrast case below
   * fails, which is what makes it a real check rather than a tautology.
   *
   * Why it matters: a latched override makes `prepareConfigUpdate` accept the FIRST candidate offered
   * without a sampling check. Left standing, the exemption granted to the meter that broke would be
   * silently inherited by the meter that replaced it.
   */
  check((dev.undersamplingFlags & 0x04) == 0,
        "and the override goes with it - the new meter does not inherit the old one's exemption");
  check(dev.registers.at(plc::REG_UNDERSAMPLING_FLAGS) == 0,
        "with the diagnostics register republished, not just the shadow variable");
}

void theDecommissionArmIsStillDifferentInBothWays() {
  std::printf("\n[OFF_CMD_RESET_CONFIG, side by side - the reason 25 exists at all]\n");

  Device dev;
  dev.armChannel(2, 1, 98765.0);
  dev.configs[2].f_multiplier = 1;
  ModbusManager modbus(dev.deps());
  latchNyquistOverride(modbus, dev, 2);

  const uint16_t address =
      static_cast<uint16_t>(plc::sensorBaseAddress(2) + plc::OFF_CMD_RESET_CONFIG);
  check(modbus.applyHoldingWrite(address, 1), "offset 19 is issued instead");

  /**
   * Offset 19's behaviour, PINNED rather than corrected.
   *
   * These four assertions document a shipped command exactly as it behaves, including the stale
   * override, so that narrowing it later is a deliberate act with a failing test attached rather than
   * an accident. It is the right command for decommissioning a channel; it is the wrong one for a meter
   * swap, and every line here is a line the arm at offset 25 does the opposite of.
   */
  check(dev.sensors[2].cumulativeLiters == 0.0, "it DOES destroy the lifetime total");
  check(dev.sensors[2].sessionLiters == 0.0f, "and the session volume");
  check(dev.sensors[2].maxFlowSinceReset == 0.0f, "and the peak");
  check((dev.undersamplingFlags & 0x04) != 0,
        "and it leaves the Nyquist override latched, which is a defect reported, not fixed here");
  check(dev.sensors[2].inUse, "though it does keep the channel in use");
}

/**
 * ── THE OTHER HALF OF THE SWAP: can the REPLACEMENT meter's figures be entered? ─────────────
 *
 * Everything above proves the reset does the right thing. It says nothing about whether the channel it
 * produces can ever be configured again — and until `prepareConfigUpdate` was changed, it could not, by
 * any route. That made the command above a one-way door: a working channel could be sent to `SET?` from
 * the panel and never brought back, on a device with no factory reset short of wiping every channel.
 *
 * Each case enters channel 3 field by field, exactly as the panel's editors and a Modbus master both do
 * — one single-register write per field, each candidate built from the config already in force. The
 * FIRST write of every pair is the one that used to be refused, so asserting it is accepted is the whole
 * point; asserting the channel is still not ready after it is what stops "accepted" from being read as
 * "installed a configuration that cannot produce a reading".
 */
void bothEntryOrdersReachAValidConfigurationFromAllZeros() {
  std::printf("\n[after the reset, the replacement meter's figures can be entered - both orders]\n");

  Device dev;
  dev.armChannel(1, 60, 111.0);   // channel 2, a neighbour that must not move
  dev.armChannel(2, 100, 222.0);  // channel 3, the target
  dev.armChannel(3, 250, 333.0);  // channel 4, the other neighbour
  ModbusManager modbus(dev.deps());

  check(modbus.applyHoldingWrite(Device::resetCalAddress(2), 1), "channel 3's calibration is reset");
  check(dev.configs[2] == SensorCharacteristics{}, "so it holds an all-zero configuration");

  // Q FIRST. The candidate is {q_max=120, f_multiplier=0}, which fails configIsValid on the multiplier —
  // this is the write that returned false before, and there was no other order that got further.
  check(modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_Q_MAX), 120),
        "Q alone is ACCEPTED on a channel with nothing to lose");
  check(dev.configs[2].q_max == 120, "and it is actually stored, not merely acknowledged");
  check(!configIsValid(dev.configs[2]),
        "the channel is still NOT ready though - half a specification is not a calibration");

  check(modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_F_MULT), 6),
        "the multiplier completes it");
  check(configIsValid(dev.configs[2]) && dev.configs[2].f_multiplier == 6,
        "and NOW the channel is ready - which is what clears SET? on the panel");

  // THE REVERSE ORDER, on the same channel, because a fix that only worked one way round would leave the
  // operator guessing which row to edit first.
  check(modbus.applyHoldingWrite(Device::resetCalAddress(2), 1), "reset again, to enter it the other way");
  check(modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_F_MULT), 6),
        "the multiplier alone is accepted first this time");
  check(!configIsValid(dev.configs[2]), "still not ready with no Q to clamp to");
  check(modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_Q_MAX), 120),
        "and Q completes it from the other direction");
  check(configIsValid(dev.configs[2]) && dev.configs[2].q_max == 120, "the channel is ready either way");

  // The wrong-channel guard the whole file is built around: entry writes are per-channel too.
  check(configIsValid(dev.configs[1]) && dev.configs[1].q_max == 60,
        "channel 2 was never touched by any of it");
  check(configIsValid(dev.configs[3]) && dev.configs[3].q_max == 250, "nor was channel 4");
  check(dev.sensors[2].cumulativeLiters == 222.0,
        "and channel 3 still carries the volume the OLD meter measured, through the whole re-entry");
}

void thePulsesPerLitreFormCanBeEnteredToo() {
  std::printf("\n[the pulses-per-litre form, entered from all zeros in three writes]\n");

  Device dev;
  dev.armChannel(2, 100, 444.0);
  ModbusManager modbus(dev.deps());
  check(modbus.applyHoldingWrite(Device::resetCalAddress(2), 1), "channel 3's calibration is reset");

  /**
   * Three fields, and the FORM has to be settable before the figure it selects exists.
   *
   * `configIsValid` reads `pulses_per_litre` only once `calibration == PulsesPerLitre`, so switching the
   * form first leaves a candidate that fails on both q_max and the pulse figure — the most invalid state
   * of any entry path, and the one most likely to have been missed by a narrower fix.
   */
  check(modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_CAL_TYPE),
                                 static_cast<uint16_t>(CalibrationType::PulsesPerLitre)),
        "the calibration FORM can be switched on an unset channel");
  check(dev.configs[2].calibration == CalibrationType::PulsesPerLitre, "and the switch is stored");
  check(modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_PULSES_PER_L), 450),
        "450 pulses/L is accepted next, with Q still zero");
  check(!configIsValid(dev.configs[2]), "not ready yet - the pulses form still needs its Q");
  check(modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_Q_MAX), 100),
        "and Q completes the pulses form");
  check(configIsValid(dev.configs[2]) && dev.configs[2].pulses_per_litre == 450,
        "the channel is ready on the pulses-per-litre form");
}

/**
 * ── THE PULSES-PER-LITRE FORM HAS ITS OWN SAMPLING CEILING ──────────────────────────────────
 *
 * Found by the case above failing. `meetsNyquistLimit` computed the FORMULA ceiling only and returned
 * false on `f_multiplier == 0` — the correct, normal state of a channel calibrated by pulses — so a
 * perfectly ordinary 450 p/L meter was refused a check it could not pass, then flagged for it forever.
 *
 * Both directions are asserted here, and the pair is the point: a fix that merely stopped failing PPL
 * configs would pass the first case and quietly exempt the form from the check altogether. The second
 * case is the one that proves the ceiling is computed rather than skipped.
 *
 * 3.3 kHz sampler throughout (`Device::pollingRateKhz`), against `F = K*Q/60`:
 *   K=450, Q=100  ->   750 Hz, needs 1500 -> inside the budget
 *   K=450, Q=1000 ->  7500 Hz, needs 15000 -> four and a half times over it
 */
void aPulsesPerLitreChannelIsSamplingCheckedOnItsOwnTerms() {
  std::printf("\n[the pulses form is checked against K*Q/60, not against a multiplier it does not use]\n");

  Device dev;
  dev.armChannel(2, 100, 888.0);
  ModbusManager modbus(dev.deps());
  check(modbus.applyHoldingWrite(Device::resetCalAddress(2), 1), "channel 3's calibration is reset");
  check(modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_CAL_TYPE),
                                 static_cast<uint16_t>(CalibrationType::PulsesPerLitre)),
        "the pulses form is selected");
  check(modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_PULSES_PER_L), 450),
        "450 p/L is entered");

  // ONE write, not two. This is what the override handshake used to be demanded for.
  check(modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_Q_MAX), 100),
        "a 100 L/min ceiling - 750 Hz - is accepted on the FIRST write, with no override to confirm");
  check(configIsValid(dev.configs[2]) && dev.configs[2].q_max == 100, "the channel is ready");
  check(!modbus.nyquistOverridePending(2), "and nothing is parked awaiting a confirmation");

  /**
   * REGISTER 30 IS THE OBSERVABLE HALF. `evaluateSensorDiagnostics` ORs in `valid && !meets`, so before
   * this fix the bit was lit here — a master polling it saw a permanent sampling fault on a channel
   * inside budget, and the panel wore the warning banner over it.
   */
  check((dev.undersamplingFlags & 0x04) == 0, "no undersampling flag is raised on a channel in budget");
  check(dev.registers.at(plc::REG_UNDERSAMPLING_FLAGS) == 0,
        "and register 30 says so too, not just the shadow variable");

  // THE OTHER DIRECTION: the check is real for this form, not disabled for it.
  Device fast;
  fast.armChannel(2, 100, 999.0);
  ModbusManager fastModbus(fast.deps());
  check(fastModbus.applyHoldingWrite(Device::resetCalAddress(2), 1), "a second channel 3 is reset");
  check(fastModbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_CAL_TYPE),
                                     static_cast<uint16_t>(CalibrationType::PulsesPerLitre)),
        "the pulses form is selected again");
  check(fastModbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_PULSES_PER_L), 450),
        "with the same 450 p/L meter");
  check(!fastModbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_Q_MAX), 1000),
        "but a 1000 L/min ceiling - 7500 Hz - IS refused, four times past the sampler");
  check(fastModbus.nyquistOverridePending(2), "and parked for the §5.5 confirmation");
  check((fast.undersamplingFlags & 0x04) != 0, "with the flag lit while it waits");
  check(fastModbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_Q_MAX), 1000),
        "repeating it overrides deliberately, exactly as the formula form does");
  check(configIsValid(fast.configs[2]) && fast.configs[2].q_max == 1000,
        "and only then is the out-of-budget figure installed");
}

void aValidCalibrationStillCannotBeDemolishedFieldByField() {
  std::printf("\n[the guard that must NOT have been widened: zeros over a WORKING channel]\n");

  Device dev;
  dev.armChannel(2, 100, 555.0);
  dev.armChannel(3, 250, 666.0);
  ModbusManager modbus(dev.deps());
  check(configIsValid(dev.configs[2]), "channel 3 starts validly calibrated");

  /**
   * THIS IS THE ASSERTION THAT KEEPS THE ONE ABOVE HONEST.
   *
   * `register_map.h`'s OFF_CMD_RESET_CALIBRATION note rests on exactly this: returning a channel to "not
   * set" is expressible only as a command, never as a value write. A fix that simply stopped checking
   * `configIsValid` would pass every entry case above and silently give a Modbus master a second,
   * undocumented way to unset a channel - one that skips the override clearing the command performs.
   */
  check(!modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_Q_MAX), 0),
        "Q = 0 over a valid calibration is REFUSED");
  check(dev.configs[2].q_max == 100, "and the calibration is untouched, not partially applied");
  check(!modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_F_MULT), 0),
        "a zero multiplier is refused for the same reason");
  check(dev.configs[2].f_multiplier == 6, "and that field is untouched too");
  check(configIsValid(dev.configs[2]), "so the channel is still ready - nothing was demolished");
  check(configIsValid(dev.configs[3]), "and channel 4 is still ready as well");
}

void theSamplingCheckIsDeferredByEntryAndNotSkipped() {
  std::printf("\n[an incomplete entry does not smuggle a figure past the Nyquist check]\n");

  Device dev;
  dev.armChannel(2, 100, 777.0);
  ModbusManager modbus(dev.deps());
  check(modbus.applyHoldingWrite(Device::resetCalAddress(2), 1), "channel 3's calibration is reset");

  /**
   * The worry this answers: if an invalid candidate is now accepted, does a channel entered field by
   * field ever get sampling-checked at all?
   *
   * It does, on the write that COMPLETES the configuration - which is the first candidate a frequency
   * can be computed from. `meetsNyquistLimit` needs f_multiplier and q_max together, so checking the
   * halves would be checking nothing. Here Q is entered alone (accepted, incomplete, unchecked), and the
   * multiplier that completes it puts the theoretical frequency at 65000 Hz against a 3.3 kHz sampler -
   * refused, and parked for the §5.5 two-write confirmation exactly as it would be on any other edit.
   */
  check(modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_Q_MAX), 65000),
        "an enormous Q is accepted while the configuration is incomplete");
  check((dev.undersamplingFlags & 0x04) == 0,
        "and no undersampling flag is raised yet - there is no frequency to compute");

  check(!modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_F_MULT), 1),
        "the multiplier that COMPLETES it is refused - the sampling check finally has both terms");
  check(dev.configs[2].f_multiplier == 0, "so the completing field was not stored");
  check(!configIsValid(dev.configs[2]), "the channel stays unset rather than becoming a wrong reading");
  check((dev.undersamplingFlags & 0x04) != 0, "and the channel's undersampling flag is lit");

  check(modbus.applyHoldingWrite(Device::cfgAddress(2, plc::OFF_CFG_F_MULT), 1),
        "repeating the write confirms the override, as §5.5 requires");
  check(configIsValid(dev.configs[2]),
        "and only THEN is the channel ready - the operator overrode it deliberately");
}

}  // namespace

int main() {
  std::printf("modbus_manager - OFF_CMD_RESET_CALIBRATION keeps what it must keep\n");
  clearsTheCalibrationAndKeepsEveryMeasurement();
  actsOnOneChannelAndLeavesItsNeighboursAlone();
  theCommandRegisterSelfClears();
  onlyAWriteOfOneIsTheCommand();
  anUnwiredChannelRefusesTheCommand();
  clearsTheNyquistOverrideWithTheCalibration();
  theDecommissionArmIsStillDifferentInBothWays();
  bothEntryOrdersReachAValidConfigurationFromAllZeros();
  thePulsesPerLitreFormCanBeEnteredToo();
  aPulsesPerLitreChannelIsSamplingCheckedOnItsOwnTerms();
  aValidCalibrationStillCannotBeDemolishedFieldByField();
  theSamplingCheckIsDeferredByEntryAndNotSkipped();
  if (failures > 0) {
    std::printf("\nFAILURES (%d of %d)\n", failures, checks);
    return 1;
  }
  std::printf("\n%d checks, no failures\n", checks);
  return 0;
}
