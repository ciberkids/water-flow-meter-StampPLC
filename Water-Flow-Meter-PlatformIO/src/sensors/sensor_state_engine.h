#pragma once

#include <cstddef>

#include "modbus/modbus_manager.h"
#include "modbus/register_bank.h"
#include "modbus/register_map.h"
#include "modbus/sensor_types.h"

namespace plc {

class SensorStateEngine {
 public:
  struct Dependencies {
    SensorData* sensors = nullptr;
    SensorCharacteristics* configs = nullptr;
    std::size_t sensorCount = 0;
    RegisterBank* registerBank = nullptr;
    ModbusManager* modbusManager = nullptr;
    double* totalSessionLitersCache = nullptr;
    double* aggregateFlowLpsCache = nullptr;
    bool* allSensorsReadyCache = nullptr;
    uint16_t* undersamplingFlags = nullptr;
  };

  explicit SensorStateEngine(const Dependencies& deps);

  void update(float elapsedSeconds);
  void refreshDiagnostics();

 private:
  Dependencies deps_;
};

}  // namespace plc
