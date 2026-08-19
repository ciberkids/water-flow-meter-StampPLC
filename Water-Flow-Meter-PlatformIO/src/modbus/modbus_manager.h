#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

class Preferences;
// eModbus has no umbrella "eModbus.h"; the RTU server header pulls in
// ModbusMessage, ModbusTypeDefs and RTUutils.
#include <ModbusMessage.h>

#include "led/led_controller.h"
#include "modbus/link_settings.h"
#include "modbus/register_bank.h"
#include "net/net_register_map.h"
#include "modbus/register_map.h"
#include "time/device_clock.h"
#include "modbus/sensor_types.h"

namespace plc {

/**
 * How many samples per pulse period `meetsNyquistLimit` demands — the factor the theoretical ceiling
 * frequency is multiplied by before being compared against the achieved polling rate.
 *
 * NAMED rather than left as a `2.0` literal inside the comparison because it is now duplicated across a
 * language boundary: `web/mockup/src/utils/sensorConfig.ts` derives the same flag so an unsamplable
 * configuration can be seen in the simulator, and its unit test READS THIS DECLARATION rather than
 * trusting a second copy of the number (the pattern `editorRamp.test.ts` uses on `ui_accel.h`). A literal
 * buried in an expression cannot be pinned that way, and two silently diverging factors would mean the
 * mockup flagging configurations the device accepts, or worse, accepting ones it flags.
 *
 * WHAT THE FACTOR IS NOT: 2 is the Nyquist–Shannon rate for RECONSTRUCTING a band-limited analogue
 * signal. This is a polled edge counter (`plc::risingEdges`, firmware.cpp), and 2 samples per period is
 * not enough for one — at exactly 2x a square wave can be sampled at a constant phase and yield no
 * counted edges at all, and the test is frequency-only, so it says nothing about DUTY CYCLE: a 1 kHz
 * input with a 5% duty has a 50 us high phase, which a 303 us sampling period misses entirely while
 * `3300 >= 2*1000` passes. Raising it is a decision about what meters this product supports, not a
 * refactor, so the factor is recorded here with its own weakness rather than quietly changed.
 */
inline constexpr double kSamplingMarginFactor = 2.0;

}  // namespace plc

struct ModbusDependencies {
  SensorData* sensors = nullptr;
  SensorCharacteristics* configs = nullptr;
  Preferences* preferences = nullptr;
  plc::RegisterBank* registers = nullptr;
  /**
   * The network block at 500-751 (WiFi_MQTT_Connectivity.md §5). May be null.
   *
   * Reached from here rather than intercepted in firmware.cpp's thin worker wrappers, because §5.5
   * requires ONE apply path shared by the display, a master and the portal — and this is where the
   * master's writes already land. Splitting it would give the register convention two owners.
   */
  plc::NetSettings* net = nullptr;
  LedController* ledController = nullptr;
  /**
   * The device clock, so a session reset can be DATED. May be null.
   *
   * Reached from here for the same reason the network block is: this is where a session reset already
   * lands, whether it came from a Modbus master, the panel or the portal. Stamping the time in
   * firmware.cpp instead would mean finding every route into a reset and remembering each one — and the
   * routes are exactly what this class exists to unify.
   *
   * Nullable because two host tests construct this struct and neither has a clock; the calls are guarded
   * rather than the field being required, so adding it cannot break a test that has no opinion about time.
   */
  plc::DeviceClock* clock = nullptr;
  uint16_t* connectedBitmap = nullptr;
  uint16_t* undersamplingFlags = nullptr;
  double* totalSessionLitersCache = nullptr;
  double* aggregateFlowLpmCache = nullptr;
  /** REG_DISPLAY_FLOW_UNIT — which unit the panel shows flows in. A display preference only. */
  uint16_t* displayFlowUnit = nullptr;
  bool* allSensorsReadyCache = nullptr;
  volatile float* pollingRateKhz = nullptr;
  plc::LinkSettingsManager* link = nullptr;
  std::size_t sensorCount = 0;
};

class ModbusManager {
 public:
  explicit ModbusManager(const ModbusDependencies& deps);

  bool isWritableAddress(uint16_t address) const;
  /**
   * The single entry point for every holding-register write, from the bus or the UI.
   *
   * `origin` is passed rather than held as state on purpose: the eModbus server task
   * (priority 8) preempts the logic task that drives the UI on the same core, so a
   * mutable "current origin" member could be read by a bus frame that interrupted a
   * display write and disarm a rollback that must stay armed.
   */
  bool applyHoldingWrite(uint16_t address,
                         uint16_t value,
                         plc::WriteOrigin origin = plc::WriteOrigin::Bus);

  void syncSensorToHolding(std::size_t sensorIndex);
  void syncGlobalRegisters();
  void evaluateSensorDiagnostics();

  /**
   * True when this sensor's last config write was refused because it fails the Nyquist limit
   * and is awaiting an override confirmation.
   *
   * The UI needs this to tell a Nyquist refusal apart from the five other reasons
   * writeSetting can fail — no Modbus, a rejected link write, an out-of-range sensor index,
   * a missing bitmap, an unhandled target. Without it the editor showed "Sampling too slow"
   * for all six, which is a wrong diagnosis five times out of six.
   */
  bool nyquistOverridePending(std::size_t sensorIndex) const {
    return sensorIndex < plc::kNumSensors && overridePending_[sensorIndex];
  }

  /** True when an apply changed the live settings and the UART must restart. */
  bool consumeLinkRestartRequest();

  ModbusMessage handleReadHolding(ModbusMessage request);
  ModbusMessage handleWriteSingle(ModbusMessage request);
  ModbusMessage handleWriteMultiple(ModbusMessage request);

 private:
  void publishClock();
  /**
   * The staged halves of a clock write, held here until `REG_CLOCK_APPLY` composes them (N-d1).
   *
   * Not written into the register bank as they arrive, and not applied on the low word: two registers are
   * not atomic under FC6, so an epoch composed from one new half and one old one is a timestamp nobody
   * chose. They start at 0, which `setTime` refuses, so an apply before any staging cannot set the clock
   * to 1970.
   */
  uint16_t stagedClockHi_ = 0;
  uint16_t stagedClockLo_ = 0;
  bool meetsNyquistLimit(const SensorCharacteristics& cfg) const;
  void resetRuntimeCaches();
  void saveCumulativeToNvs(std::size_t index);
  bool prepareConfigUpdate(std::size_t index, const SensorCharacteristics& candidate, bool* acceptedOverride);

  ModbusDependencies deps_;
  bool linkRestartPending_ = false;
  std::array<bool, plc::kNumSensors> overridePending_{};
  std::array<bool, plc::kNumSensors> overrideActive_{};
  std::array<SensorCharacteristics, plc::kNumSensors> pendingOverrides_{};
};
