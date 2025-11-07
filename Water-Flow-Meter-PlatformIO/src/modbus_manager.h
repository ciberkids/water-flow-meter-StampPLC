#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include <Preferences.h>
#include <eModbus.h>

#include "led_controller.h"
#include "register_bank.h"
#include "register_map.h"
#include "sensor_types.h"

struct ModbusDependencies {
  SensorData* sensors = nullptr;
  SensorCharacteristics* configs = nullptr;
  Preferences* preferences = nullptr;
  RegisterBank* registers = nullptr;
  LedController* ledController = nullptr;
  uint16_t* connectedBitmap = nullptr;
  uint16_t* undersamplingFlags = nullptr;
  double* totalSessionLitersCache = nullptr;
  double* aggregateFlowLpsCache = nullptr;
  bool* allSensorsReadyCache = nullptr;
  volatile float* pollingRateKhz = nullptr;
  std::size_t sensorCount = 0;
};

class ModbusManager {
 public:
  explicit ModbusManager(const ModbusDependencies& deps);

  bool isWritableAddress(uint16_t address) const;
  bool applyHoldingWrite(uint16_t address, uint16_t value);

  void syncSensorToHolding(std::size_t sensorIndex);
  void syncGlobalRegisters();
  void evaluateSensorDiagnostics();

  ModbusMessage handleReadHolding(ModbusMessage request);
  ModbusMessage handleWriteSingle(ModbusMessage request);
  ModbusMessage handleWriteMultiple(ModbusMessage request);

 private:
  bool meetsNyquistLimit(const SensorCharacteristics& cfg) const;
  void resetRuntimeCaches();
  void saveCumulativeToNvs(std::size_t index);
  bool prepareConfigUpdate(std::size_t index, const SensorCharacteristics& candidate, bool* acceptedOverride);

  ModbusDependencies deps_;
  std::array<bool, plc::kNumSensors> overridePending_{};
  std::array<bool, plc::kNumSensors> overrideActive_{};
  std::array<SensorCharacteristics, plc::kNumSensors> pendingOverrides_{};
};
