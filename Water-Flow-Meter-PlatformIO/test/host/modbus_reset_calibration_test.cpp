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
  if (failures > 0) {
    std::printf("\nFAILURES (%d of %d)\n", failures, checks);
    return 1;
  }
  std::printf("\n%d checks, no failures\n", checks);
  return 0;
}
