/**
 * The two degenerate calibrations a Modbus master could install, and the readings they produced.
 *
 * Both were audit findings. Both were ACCEPTED by `applyHoldingWrite`, reported `OK` through the status
 * flags, and left REG_UNDERSAMPLING_FLAGS clear — so nothing on the wire or on the panel said the
 * channel was misconfigured:
 *
 *   - a NEGATIVE multiplier. The multiplier is a divisor, so the channel read 0.00 L/min at every
 *     frequency. The panel editor has been bounded at 1..32767 since the day its comment named this
 *     consequence, and the register wiki publishes "the legal range is 1..32767" — but neither the
 *     panel's commit path nor the register arm re-checked it, so the bound existed only in the stepper
 *     that clamps while editing. Over RS485 it did not exist at all.
 *   - an OFFSET CANCELLING THE WHOLE SPAN, e.g. m=200, a=-30000, q=150. `meetsNyquistLimit` clamped the
 *     ceiling at zero and then returned TRUE for a ceiling of zero, so the sampling gate waved it
 *     through. The engine reads a flat q_max at ZERO pulses: a dry pipe reporting full scale.
 *
 * The consequences are driven through the REAL SensorStateEngine rather than asserted from the formula,
 * because the claim being made is about what an operator sees, not about arithmetic.
 */
#include <Preferences.h>

#include <cstdio>

#include "led/led_controller.h"
#include "modbus/modbus_manager.h"
#include "modbus/register_bank.h"
#include "modbus/register_map.h"
#include "modbus/sensor_types.h"
#include "net/net_settings.h"
#include "sensors/sensor_state_engine.h"
#include "time/device_clock.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-78s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) failures++;
}

struct Device {
  plc::RegisterBank registers;
  SensorData sensors[plc::kNumSensors]{};
  SensorCharacteristics configs[plc::kNumSensors]{};
  Preferences preferences;
  LedController leds;
  plc::DeviceClock clock;
  plc::NetSettings net;
  uint16_t connectedBitmap = 0x01;
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
    d.net = &net;
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
};

/** Channel 1 in use, with a working formula calibration to start from. */
void arm(Device& dev) {
  dev.sensors[0].inUse = true;
  dev.configs[0].q_max = 150;
  dev.configs[0].f_multiplier = 10;
  dev.configs[0].adjust = 0;
  dev.configs[0].calibration = CalibrationType::Formula;
}

uint16_t addr(uint16_t offset) { return plc::sensorBaseAddress(0) + offset; }

/** The flow the REAL engine computes for `pulses` edges in one second. */
float flowAt(Device& dev, ModbusManager& modbus, uint32_t pulses) {
  plc::SensorStateEngine::Dependencies d;
  d.sensors = dev.sensors;
  d.configs = dev.configs;
  d.sensorCount = plc::kNumSensors;
  d.registerBank = &dev.registers;
  d.modbusManager = &modbus;
  d.totalSessionLitersCache = &dev.totalSessionLiters;
  d.aggregateFlowLpmCache = &dev.aggregateFlowLpm;
  d.allSensorsReadyCache = &dev.allSensorsReady;
  d.undersamplingFlags = &dev.undersamplingFlags;
  plc::SensorStateEngine engine(d);
  dev.sensors[0].pulseCount = pulses;
  engine.update(1.0f);
  return dev.sensors[0].instantFlow_L_min;
}

void negativeMultiplierIsRefused() {
  std::printf("\n[a negative multiplier: refused, where it used to install and read 0.00 forever]\n");

  Device dev;
  arm(dev);
  ModbusManager modbus(dev.deps());

  // 0xFF9C is -100 as an int16 — the write the audit used.
  check(!modbus.applyHoldingWrite(addr(plc::OFF_CFG_F_MULT), 0xFF9C),
        "the write is REFUSED rather than accepted");
  check(dev.configs[0].f_multiplier == 10,
        "and the channel keeps the multiplier it had — nothing was installed");

  // The control: the same register, a legal value, still accepted. Without this the check above would
  // pass against an arm that refused everything.
  //
  // 8, not 12. At q_max=150 a multiplier of 12 is 1800 Hz and needs 3600 — genuinely outside the 3.3 kHz
  // budget, so it is refused by the SAMPLING gate and would look like a validity refusal here. 8 is
  // 1200 Hz, needing 2400.
  check(modbus.applyHoldingWrite(addr(plc::OFF_CFG_F_MULT), 8),
        "a positive multiplier is still accepted (the control)");
  check(dev.configs[0].f_multiplier == 8, "and lands");

  // Zero was already refused, and still is — this fix widened that rule rather than replacing it.
  check(!modbus.applyHoldingWrite(addr(plc::OFF_CFG_F_MULT), 0), "zero is still refused");
  check(dev.configs[0].f_multiplier == 8, "leaving the previous value in place");

  // What made it harmful, shown rather than argued: had it installed, this is what the channel read.
  Device broken;
  arm(broken);
  broken.configs[0].f_multiplier = -100;
  ModbusManager brokenModbus(broken.deps());
  check(!configIsValid(broken.configs[0]),
        "such a configuration is no longer VALID, so the channel reads SET? instead of OK");
  check(flowAt(broken, brokenModbus, 1000) == 0.0f,
        "and the engine confirms why it mattered: 1000 pulses/s still reads 0.00 L/min");
}

void aNegativeOffsetIsRefusedOutright() {
  std::printf("\n[a negative offset: refused on every attempt, with no override — DF14]\n");

  Device dev;
  arm(dev);
  dev.configs[0].f_multiplier = 200;
  ModbusManager modbus(dev.deps());

  // 0x8AD0 is -30000. It used to be PARKED: the ceiling collapsed to 200*150 - 30000 = 0, the sampling
  // gate flagged it, and §5.5's handshake let a second identical write install it. DF14 moved the rule
  // into `configIsValid`, so a negative offset is now invalid rather than unsamplable — and an invalid
  // candidate never reaches the handshake at all (`prepareConfigUpdate` clears the override state and
  // returns early for one).
  check(!modbus.applyHoldingWrite(addr(plc::OFF_CFG_ADJUST), 0x8AD0),
        "the write is refused on its first attempt");
  check(dev.configs[0].adjust == 0, "so nothing was installed");

  check(!modbus.applyHoldingWrite(addr(plc::OFF_CFG_ADJUST), 0x8AD0),
        "and a SECOND identical write is refused too — there is no §5.5 override for an invalid config");
  check(dev.configs[0].adjust == 0, "so no determined master can install it either");

  // The harm the refusal exists to prevent, asserted from the other side: force the figures straight into
  // the struct, bypassing every write path, and the ENGINE still produces nothing — because readiness is
  // `configIsValid`, evaluated where it is asked. Before DF14 this same channel read 150 L/min at zero
  // pulses: full scale on a dry pipe, accumulating volume into the session and lifetime totals.
  Device forced;
  arm(forced);
  forced.configs[0].f_multiplier = 200;
  forced.configs[0].adjust = -30000;
  ModbusManager forcedModbus(forced.deps());
  check(!configIsValid(forced.configs[0]), "a negative offset is not a valid configuration");
  check(flowAt(forced, forcedModbus, 0) == 0.0f,
        "and a dry pipe reads 0.0, where it used to read 150.0");
  check(flowAt(forced, forcedModbus, 1000) == 0.0f,
        "as does a channel with water in it — an invalid calibration measures nothing, by design");
}

void anUnsamplableButVALIDConfigIsStillParked() {
  std::printf("\n[§5.5's handshake, on the case it is actually for: valid but faster than the sampler]\n");

  Device dev;
  arm(dev);
  ModbusManager modbus(dev.deps());

  // q_max 150 with a multiplier of 200 reaches 30 kHz, an order of magnitude past what a 3.3 kHz sampler
  // can resolve — and it is a perfectly INTERPRETABLE configuration, which is the distinction DF14 drew.
  // This case previously rode on a negative offset; it now uses a legal one, so the handshake stays
  // covered by something the new rule cannot reclassify.
  check(!modbus.applyHoldingWrite(addr(plc::OFF_CFG_F_MULT), 200),
        "the first write is parked, not installed — the channel would outrun the sampler");
  check(dev.configs[0].f_multiplier == 10, "so the old multiplier still stands");

  modbus.evaluateSensorDiagnostics();
  check((dev.undersamplingFlags & 0x01) != 0, "REG_UNDERSAMPLING_FLAGS names the channel");
  check(dev.registers.at(plc::REG_UNDERSAMPLING_FLAGS) == dev.undersamplingFlags,
        "with the register published, not just the cache updated");

  check(modbus.applyHoldingWrite(addr(plc::OFF_CFG_F_MULT), 200),
        "a SECOND identical write is accepted — that is the §5.5 override handshake");
  check(dev.configs[0].f_multiplier == 200,
        "so a determined master can still install a meter the gate is wrong about");
}

void theConfigurationsThatMustStillInstall() {
  std::printf("\n[the controls — 77c1252's pair, unchanged by this]\n");

  Device dev;
  arm(dev);
  ModbusManager modbus(dev.deps());

  // A 450 pulses/L meter at 100 L/min: 750 Hz against 3.3 kHz, comfortably inside budget.
  check(modbus.applyHoldingWrite(addr(plc::OFF_CFG_Q_MAX), 100), "q_max installs");
  // K BEFORE the form, deliberately: switching a validly calibrated channel to the pulses form while its
  // K is still 0 offers an INVALID candidate, and 77c1252's rule is that a valid configuration cannot be
  // demolished field by field. This ordering is a property of that rule, not of this fix.
  check(modbus.applyHoldingWrite(addr(plc::OFF_CFG_PULSES_PER_L), 450),
        "K installs on the FIRST write, with no override handshake");
  check(modbus.applyHoldingWrite(addr(plc::OFF_CFG_CAL_TYPE),
                                 static_cast<uint16_t>(CalibrationType::PulsesPerLitre)),
        "and the pulses form installs after it");
  modbus.evaluateSensorDiagnostics();
  check((dev.undersamplingFlags & 0x01) == 0, "with register 30 clear");
  check(configIsValid(dev.configs[0]), "and the channel valid");

  // A positive adjust is a normal calibration and must be untouched by any of this.
  Device formula;
  arm(formula);
  ModbusManager formulaModbus(formula.deps());
  check(formulaModbus.applyHoldingWrite(addr(plc::OFF_CFG_ADJUST), 100),
        "a positive offset still installs");
  check(formula.configs[0].adjust == 100, "and lands");
  formulaModbus.evaluateSensorDiagnostics();
  check((formula.undersamplingFlags & 0x01) == 0,
        "1600 Hz needs 3200 and so stays inside the 3.3 kHz budget, unflagged");
}

}  // namespace

int main() {
  std::printf("The sensor configuration gate: what a master may install\n");
  negativeMultiplierIsRefused();
  aNegativeOffsetIsRefusedOutright();
  anUnsamplableButVALIDConfigIsStillParked();
  theConfigurationsThatMustStillInstall();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
