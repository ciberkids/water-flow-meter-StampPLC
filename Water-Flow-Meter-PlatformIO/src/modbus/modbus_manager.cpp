#include "modbus/modbus_manager.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace {
bool configIsValid(const SensorCharacteristics& cfg) {
  if (cfg.q_max == 0 || cfg.f_multiplier == 0) {
    return false;
  }
  const int32_t multiplier = std::max<int32_t>(std::abs(static_cast<int32_t>(cfg.f_multiplier)), 1);
  const int32_t limit = static_cast<int32_t>(cfg.q_max) * multiplier * 10;
  const int32_t adjust = static_cast<int32_t>(cfg.adjust);
  return std::abs(adjust) <= limit;
}
}

using namespace plc;

ModbusManager::ModbusManager(const ModbusDependencies& deps) : deps_(deps) {}

bool ModbusManager::isWritableAddress(uint16_t address) const {
  if (!deps_.registers->isRangeValid(address)) {
    return false;
  }
  if (address == REG_CONNECTED_SENSORS_BITMAP ||
      address == REG_MASTER_RESET_ALL_SENSORS ||
      address == REG_MASTER_RESET_ALL_MEASURED ||
      address == REG_MASTER_RESET_ALL_SESSION ||
      address == REG_LED_RED_VOLUME_STEP ||
      address == REG_LED_RED_PULSE_PERIOD ||
      address == REG_LINK_SLAVE_ID ||
      address == REG_LINK_BAUD_INDEX ||
      address == REG_LINK_PARITY ||
      address == REG_LINK_STOP_BITS ||
      address == REG_LINK_APPLY) {
    return true;
  }
  if (address < SENSOR_1_BASE_ADDR) {
    return false;
  }
  std::size_t sensorIndex = (address - SENSOR_1_BASE_ADDR) / SENSOR_BLOCK_SIZE;
  if (sensorIndex >= deps_.sensorCount) {
    return false;
  }
  if (!deps_.sensors[sensorIndex].inUse) {
    return false;
  }
  const uint16_t offset = address - sensorBaseAddress(sensorIndex);
  switch (offset) {
    case OFF_CMD_RESET_SESSION:
    case OFF_CMD_RESET_ALL:
    case OFF_CMD_RESET_CONFIG:
    case OFF_CFG_Q_MAX:
    case OFF_CFG_F_MULT:
    case OFF_CFG_ADJUST:
      return true;
    default:
      return false;
  }
}

bool ModbusManager::applyHoldingWrite(uint16_t address, uint16_t value) {
  if (!isWritableAddress(address)) {
    return false;
  }

  if (address == REG_CONNECTED_SENSORS_BITMAP) {
    *deps_.connectedBitmap = value;
    for (std::size_t i = 0; i < deps_.sensorCount; ++i) {
      const bool shouldEnable = (value >> i) & 0x01;
      if (shouldEnable && !deps_.sensors[i].inUse) {
        deps_.sensors[i].inUse = true;
        deps_.sensors[i].pulseCount = 0;
        deps_.sensors[i].sessionLiters = 0.0f;
        deps_.sensors[i].cumulativeLiters = 0.0;
        deps_.sensors[i].maxFlowSinceReset = 0.0f;
        deps_.configs[i] = SensorCharacteristics{};
        deps_.sensors[i].isReady = false;
        overridePending_[i] = false;
        overrideActive_[i] = false;
        pendingOverrides_[i] = SensorCharacteristics{};
        saveCumulativeToNvs(i);
      } else if (!shouldEnable && deps_.sensors[i].inUse) {
        deps_.sensors[i] = SensorData{};
        deps_.configs[i] = SensorCharacteristics{};
        overridePending_[i] = false;
        overrideActive_[i] = false;
        pendingOverrides_[i] = SensorCharacteristics{};
        saveCumulativeToNvs(i);
      }
      syncSensorToHolding(i);
    }
    evaluateSensorDiagnostics();
    syncGlobalRegisters();
    return true;
  }

  // 40-43 stage; 44 commits. See Project_document.md §4.1.1 for why the transport's
  // own parameters cannot be applied by the request that carries them.
  if (address == REG_LINK_SLAVE_ID || address == REG_LINK_BAUD_INDEX ||
      address == REG_LINK_PARITY || address == REG_LINK_STOP_BITS) {
    if (!deps_.link || !deps_.link->stage(address, value)) {
      return false;
    }
    deps_.link->publish(*deps_.registers);
    return true;
  }

  if (address == REG_LINK_APPLY) {
    if (!deps_.link || value != LinkSettingsManager::kApplyMagic) {
      return false;
    }
    if (deps_.link->apply(millis())) {
      linkRestartPending_ = true;
    }
    deps_.link->publish(*deps_.registers);
    return true;
  }

  if (address == REG_MASTER_RESET_ALL_SENSORS) {
    if (value == 1) {
      for (std::size_t i = 0; i < deps_.sensorCount; ++i) {
        if (deps_.sensors[i].inUse) {
          bool wasInUse = deps_.sensors[i].inUse;
          deps_.sensors[i] = SensorData{};
          deps_.sensors[i].inUse = wasInUse;
          deps_.configs[i] = SensorCharacteristics{};
          syncSensorToHolding(i);
          overridePending_[i] = false;
          overrideActive_[i] = false;
          pendingOverrides_[i] = SensorCharacteristics{};
          saveCumulativeToNvs(i);
        }
      }
      deps_.ledController->resetToDefaults();
      deps_.ledController->saveToPreferences(*deps_.preferences);
      deps_.ledController->markSessionsCleared();
      resetRuntimeCaches();
      evaluateSensorDiagnostics();
      syncGlobalRegisters();
    }
    deps_.registers->setUint16(address, 0);
    return true;
  }

  if (address == REG_MASTER_RESET_ALL_MEASURED) {
    if (value == 1) {
      for (std::size_t i = 0; i < deps_.sensorCount; ++i) {
        if (deps_.sensors[i].inUse) {
          deps_.sensors[i].sessionLiters = 0.0f;
          deps_.sensors[i].cumulativeLiters = 0.0;
          deps_.sensors[i].maxFlowSinceReset = 0.0f;
          syncSensorToHolding(i);
          saveCumulativeToNvs(i);
          overridePending_[i] = false;
          overrideActive_[i] = false;
          pendingOverrides_[i] = SensorCharacteristics{};
        }
      }
      deps_.ledController->markSessionsCleared();
      resetRuntimeCaches();
      evaluateSensorDiagnostics();
      syncGlobalRegisters();
    }
    deps_.registers->setUint16(address, 0);
    return true;
  }

  if (address == REG_MASTER_RESET_ALL_SESSION) {
    if (value == 1) {
      for (std::size_t i = 0; i < deps_.sensorCount; ++i) {
        if (deps_.sensors[i].inUse) {
          deps_.sensors[i].sessionLiters = 0.0f;
          deps_.sensors[i].maxFlowSinceReset = 0.0f;
          syncSensorToHolding(i);
          overridePending_[i] = false;
          overrideActive_[i] = false;
          pendingOverrides_[i] = SensorCharacteristics{};
        }
      }
      deps_.ledController->markSessionsCleared();
      resetRuntimeCaches();
      evaluateSensorDiagnostics();
      syncGlobalRegisters();
    }
    deps_.registers->setUint16(address, 0);
    return true;
  }

  if (address == REG_LED_RED_VOLUME_STEP) {
    if (value != 1 && value != 10 && value != 100) {
      return false;
    }
    deps_.ledController->setVolumeStepLiters(value);
    deps_.ledController->saveToPreferences(*deps_.preferences);
    deps_.registers->setUint16(address, deps_.ledController->volumeStepLiters());
    return true;
  }

  if (address == REG_LED_RED_PULSE_PERIOD) {
    if (value < 100 || value > 2000) {
      return false;
    }
    deps_.ledController->setPulsePeriodMs(value);
    deps_.ledController->saveToPreferences(*deps_.preferences);
    deps_.registers->setUint16(address, deps_.ledController->pulsePeriodMs());
    return true;
  }

  std::size_t sensorIndex = (address - SENSOR_1_BASE_ADDR) / SENSOR_BLOCK_SIZE;
  if (sensorIndex >= deps_.sensorCount || !deps_.sensors[sensorIndex].inUse) {
    return false;
  }
  const uint16_t base = sensorBaseAddress(sensorIndex);
  const uint16_t offset = address - base;

  switch (offset) {
    case OFF_CMD_RESET_SESSION:
      if (value == 1) {
        deps_.sensors[sensorIndex].sessionLiters = 0.0f;
        deps_.sensors[sensorIndex].maxFlowSinceReset = 0.0f;
      }
      syncSensorToHolding(sensorIndex);
      deps_.registers->setUint16(address, 0);
      evaluateSensorDiagnostics();
      return true;
    case OFF_CMD_RESET_ALL:
      if (value == 1) {
        deps_.sensors[sensorIndex].sessionLiters = 0.0f;
        deps_.sensors[sensorIndex].maxFlowSinceReset = 0.0f;
        deps_.sensors[sensorIndex].cumulativeLiters = 0.0;
      }
      syncSensorToHolding(sensorIndex);
      deps_.registers->setUint16(address, 0);
      evaluateSensorDiagnostics();
      saveCumulativeToNvs(sensorIndex);
      return true;
    case OFF_CMD_RESET_CONFIG:
      if (value == 1) {
        bool wasInUse = deps_.sensors[sensorIndex].inUse;
        deps_.sensors[sensorIndex] = SensorData{};
        deps_.sensors[sensorIndex].inUse = wasInUse;
        deps_.configs[sensorIndex] = SensorCharacteristics{};
        saveCumulativeToNvs(sensorIndex);
      }
      syncSensorToHolding(sensorIndex);
      deps_.registers->setUint16(address, 0);
      evaluateSensorDiagnostics();
      return true;
    case OFF_CFG_Q_MAX: {
      SensorCharacteristics candidate = deps_.configs[sensorIndex];
      candidate.q_max = value;
      bool overrideAccepted = false;
      if (!prepareConfigUpdate(sensorIndex, candidate, &overrideAccepted)) {
        return false;
      }
      deps_.configs[sensorIndex] = candidate;
      deps_.sensors[sensorIndex].isReady = configIsValid(candidate);
      syncSensorToHolding(sensorIndex);
      evaluateSensorDiagnostics();
      return true;
    }
    case OFF_CFG_F_MULT: {
      SensorCharacteristics candidate = deps_.configs[sensorIndex];
      candidate.f_multiplier = static_cast<int16_t>(value);
      bool overrideAccepted = false;
      if (!prepareConfigUpdate(sensorIndex, candidate, &overrideAccepted)) {
        return false;
      }
      deps_.configs[sensorIndex] = candidate;
      deps_.sensors[sensorIndex].isReady = configIsValid(candidate);
      syncSensorToHolding(sensorIndex);
      evaluateSensorDiagnostics();
      return true;
    }
    case OFF_CFG_ADJUST: {
      SensorCharacteristics candidate = deps_.configs[sensorIndex];
      candidate.adjust = static_cast<int16_t>(value);
      bool overrideAccepted = false;
      if (!prepareConfigUpdate(sensorIndex, candidate, &overrideAccepted)) {
        return false;
      }
      deps_.configs[sensorIndex] = candidate;
      deps_.sensors[sensorIndex].isReady = configIsValid(candidate);
      syncSensorToHolding(sensorIndex);
      evaluateSensorDiagnostics();
      return true;
    }
    default:
      return false;
  }
}

void ModbusManager::syncSensorToHolding(std::size_t sensorIndex) {
  if (sensorIndex >= deps_.sensorCount) {
    return;
  }
  const uint16_t base = sensorBaseAddress(sensorIndex);
  uint16_t status = 0;
  if (deps_.sensors[sensorIndex].inUse) {
    status |= 0x01;
  }
  if (deps_.sensors[sensorIndex].isReady) {
    status |= 0x02;
  }
  deps_.registers->setUint16(base + OFF_STATUS_FLAGS, status);

  if (deps_.sensors[sensorIndex].inUse && deps_.sensors[sensorIndex].isReady) {
    deps_.registers->setFloat(base + OFF_INSTANT_FLOW, deps_.sensors[sensorIndex].instantFlow_L_s);
    deps_.registers->setDouble(base + OFF_CUMULATIVE_LITERS, deps_.sensors[sensorIndex].cumulativeLiters);
    deps_.registers->setDouble(base + OFF_CUMULATIVE_M3, deps_.sensors[sensorIndex].cumulativeLiters / 1000.0);
    deps_.registers->setFloat(base + OFF_SESSION_LITERS, deps_.sensors[sensorIndex].sessionLiters);
    deps_.registers->setFloat(base + OFF_SESSION_M3, deps_.sensors[sensorIndex].sessionLiters / 1000.0f);
    deps_.registers->setFloat(base + OFF_MAX_FLOW, deps_.sensors[sensorIndex].maxFlowSinceReset);
  } else {
    deps_.registers->setFloat(base + OFF_INSTANT_FLOW, 0.0f);
    deps_.registers->setDouble(base + OFF_CUMULATIVE_LITERS, 0.0);
    deps_.registers->setDouble(base + OFF_CUMULATIVE_M3, 0.0);
    deps_.registers->setFloat(base + OFF_SESSION_LITERS, 0.0f);
    deps_.registers->setFloat(base + OFF_SESSION_M3, 0.0f);
    deps_.registers->setFloat(base + OFF_MAX_FLOW, 0.0f);
  }

  deps_.registers->setUint16(base + OFF_CFG_Q_MAX, deps_.configs[sensorIndex].q_max);
  deps_.registers->setUint16(base + OFF_CFG_F_MULT, static_cast<uint16_t>(deps_.configs[sensorIndex].f_multiplier));
  deps_.registers->setUint16(base + OFF_CFG_ADJUST, static_cast<uint16_t>(deps_.configs[sensorIndex].adjust));
}

void ModbusManager::syncGlobalRegisters() {
  if (deps_.link) {
    deps_.link->publish(*deps_.registers);
  }
  deps_.registers->setFloat(REG_POLLING_RATE_KHZ, *deps_.pollingRateKhz);
  deps_.registers->setUint16(REG_CONNECTED_SENSORS_BITMAP, *deps_.connectedBitmap);
  deps_.registers->setUint16(REG_MASTER_RESET_ALL_SENSORS, 0);
  deps_.registers->setUint16(REG_MASTER_RESET_ALL_MEASURED, 0);
  deps_.registers->setUint16(REG_MASTER_RESET_ALL_SESSION, 0);
  deps_.registers->setUint16(REG_UNDERSAMPLING_FLAGS, *deps_.undersamplingFlags);
  deps_.registers->setUint16(REG_LED_RED_VOLUME_STEP, deps_.ledController->volumeStepLiters());
  deps_.registers->setUint16(REG_LED_RED_PULSE_PERIOD, deps_.ledController->pulsePeriodMs());
}

void ModbusManager::evaluateSensorDiagnostics() {
  uint16_t flags = 0;
  for (std::size_t i = 0; i < deps_.sensorCount; ++i) {
    if (!deps_.sensors[i].inUse) {
      continue;
    }
    const auto& cfg = deps_.configs[i];
    const bool valid = configIsValid(cfg);
    const bool meets = meetsNyquistLimit(cfg);
    if ((valid && !meets) || overrideActive_[i] || overridePending_[i]) {
      flags |= static_cast<uint16_t>(1u << i);
    }
  }
  *deps_.undersamplingFlags = flags;
  deps_.registers->setUint16(REG_UNDERSAMPLING_FLAGS, flags);
}

bool ModbusManager::consumeLinkRestartRequest() {
  const bool pending = linkRestartPending_;
  linkRestartPending_ = false;
  return pending;
}

ModbusMessage ModbusManager::handleReadHolding(ModbusMessage request) {
  // Any well-formed request proves the master can still reach us, which is what
  // closes the rollback window after an apply.
  if (deps_.link) {
    deps_.link->noteValidFrame(millis());
  }
  uint16_t address = 0;
  uint16_t words = 0;
  ModbusMessage response;
  request.get(2, address);
  request.get(4, words);

  if (words == 0 || !deps_.registers->isRangeValid(address, words)) {
    response.setError(request.getServerID(), request.getFunctionCode(), Modbus::ILLEGAL_DATA_ADDRESS);
    return response;
  }

  response.add(request.getServerID(), request.getFunctionCode(), static_cast<uint8_t>(words * 2));
  for (uint16_t i = 0; i < words; ++i) {
    response.add(deps_.registers->at(address + i));
  }
  return response;
}

ModbusMessage ModbusManager::handleWriteSingle(ModbusMessage request) {
  if (deps_.link) {
    deps_.link->noteValidFrame(millis());
  }
  uint16_t address = 0;
  uint16_t value = 0;
  ModbusMessage response;
  request.get(2, address);
  request.get(4, value);

  if (!applyHoldingWrite(address, value)) {
    response.setError(request.getServerID(), request.getFunctionCode(), Modbus::ILLEGAL_DATA_ADDRESS);
    return response;
  }

  response.add(request.getServerID(), request.getFunctionCode(), address, value);
  return response;
}

ModbusMessage ModbusManager::handleWriteMultiple(ModbusMessage request) {
  if (deps_.link) {
    deps_.link->noteValidFrame(millis());
  }
  uint16_t address = 0;
  uint16_t words = 0;
  uint8_t byteCount = 0;
  ModbusMessage response;
  request.get(2, address);
  request.get(4, words);
  request.get(6, byteCount);

  if (words == 0 || byteCount != words * 2 || !deps_.registers->isRangeValid(address, words)) {
    response.setError(request.getServerID(), request.getFunctionCode(), Modbus::ILLEGAL_DATA_VALUE);
    return response;
  }

  std::vector<uint8_t> buffer;
  request.get(7, buffer, byteCount);

  for (uint16_t i = 0; i < words; ++i) {
    if (!isWritableAddress(address + i)) {
      response.setError(request.getServerID(), request.getFunctionCode(), Modbus::ILLEGAL_DATA_ADDRESS);
      return response;
    }
  }

  for (uint16_t i = 0; i < words; ++i) {
    uint16_t value = (static_cast<uint16_t>(buffer[2 * i]) << 8) | static_cast<uint16_t>(buffer[2 * i + 1]);
    if (!applyHoldingWrite(address + i, value)) {
      response.setError(request.getServerID(), request.getFunctionCode(), Modbus::SERVER_DEVICE_FAILURE);
      return response;
    }
  }

  response.add(request.getServerID(), request.getFunctionCode(), address, words);
  return response;
}

bool ModbusManager::meetsNyquistLimit(const SensorCharacteristics& cfg) const {
  if (cfg.q_max == 0 || cfg.f_multiplier == 0) {
    return false;
  }
  const double theoreticalFrequency =
      std::max(0.0, static_cast<double>(cfg.f_multiplier) * static_cast<double>(cfg.q_max) +
                           static_cast<double>(cfg.adjust));
  const double pollingHz = static_cast<double>(*deps_.pollingRateKhz) * 1000.0;
  if (theoreticalFrequency <= 0.0) {
    return true;
  }
  return pollingHz >= (2.0 * theoreticalFrequency);
}

bool ModbusManager::prepareConfigUpdate(std::size_t index,
                                        const SensorCharacteristics& candidate,
                                        bool* acceptedOverride) {
  if (acceptedOverride) {
    *acceptedOverride = false;
  }

  if (!configIsValid(candidate)) {
    overridePending_[index] = false;
    overrideActive_[index] = false;
    pendingOverrides_[index] = SensorCharacteristics{};
    return false;
  }

  if (meetsNyquistLimit(candidate)) {
    overridePending_[index] = false;
    overrideActive_[index] = false;
    pendingOverrides_[index] = SensorCharacteristics{};
    return true;
  }

  if (overrideActive_[index]) {
    if (acceptedOverride) {
      *acceptedOverride = true;
    }
    return true;
  }

  if (overridePending_[index] && candidate == pendingOverrides_[index]) {
    overridePending_[index] = false;
    overrideActive_[index] = true;
    if (acceptedOverride) {
      *acceptedOverride = true;
    }
    return true;
  }

  overridePending_[index] = true;
  overrideActive_[index] = false;
  pendingOverrides_[index] = candidate;
  const uint16_t bit = static_cast<uint16_t>(1u << index);
  *deps_.undersamplingFlags |= bit;
  deps_.registers->setUint16(REG_UNDERSAMPLING_FLAGS, *deps_.undersamplingFlags);
  return false;
}

void ModbusManager::resetRuntimeCaches() {
  if (deps_.totalSessionLitersCache) {
    *deps_.totalSessionLitersCache = 0.0;
  }
  if (deps_.aggregateFlowLpsCache) {
    *deps_.aggregateFlowLpsCache = 0.0;
  }
  if (deps_.allSensorsReadyCache) {
    *deps_.allSensorsReadyCache = true;
  }
}

void ModbusManager::saveCumulativeToNvs(std::size_t index) {
  if (!deps_.preferences) {
    return;
  }
  char key[8];
  std::snprintf(key, sizeof(key), "cml_%u", static_cast<unsigned>(index));
  deps_.preferences->putDouble(key, deps_.sensors[index].cumulativeLiters);
}
