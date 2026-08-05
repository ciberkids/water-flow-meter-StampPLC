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
#include "modbus/sensor_types.h"

struct ModbusDependencies {
  SensorData* sensors = nullptr;
  SensorCharacteristics* configs = nullptr;
  Preferences* preferences = nullptr;
  plc::RegisterBank* registers = nullptr;
  /**
   * The network block at 500-732 (WiFi_MQTT_Connectivity.md §5). May be null.
   *
   * Reached from here rather than intercepted in firmware.cpp's thin worker wrappers, because §5.5
   * requires ONE apply path shared by the display, a master and the portal — and this is where the
   * master's writes already land. Splitting it would give the register convention two owners.
   */
  plc::NetSettings* net = nullptr;
  LedController* ledController = nullptr;
  uint16_t* connectedBitmap = nullptr;
  uint16_t* undersamplingFlags = nullptr;
  double* totalSessionLitersCache = nullptr;
  double* aggregateFlowLpsCache = nullptr;
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
