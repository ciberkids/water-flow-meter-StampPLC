#include "modbus/modbus_manager.h"

#include <Preferences.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
#include "units.h"

// configIsValid now lives in modbus/sensor_types.h, beside the struct it tests. It was in an anonymous
// namespace here, unreachable from the state engine and the UI — which is why its answer was cached into
// SensorData::isReady and went stale across every reboot.

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
      address == REG_MASTER_RESET_ALL_MAX ||
      address == REG_LED_RED_VOLUME_STEP ||
      address == REG_LED_RED_PULSE_PERIOD ||
      address == REG_DISPLAY_FLOW_UNIT ||
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
    case OFF_CMD_RESET_CALIBRATION:
    case OFF_CFG_Q_MAX:
    case OFF_CFG_F_MULT:
    case OFF_CFG_ADJUST:
    case OFF_CFG_CAL_TYPE:
    case OFF_CFG_PULSES_PER_L:
      return true;
    default:
      return false;
  }
}

bool ModbusManager::applyHoldingWrite(uint16_t address,
                                      uint16_t value,
                                      plc::WriteOrigin origin) {
  // The network block first, before isWritableAddress() — that predicate only knows the sensor and
  // link registers, so every network address would be refused before reaching NetRegisterMap.
  if (plc::NetRegisterMap::contains(address)) {
    if (deps_.net == nullptr) {
      return false;
    }
    if (address == plc::net_reg::kApply) {
      const plc::NetApplyError error = plc::NetRegisterMap::applyWrite(*deps_.net, value);
      // §5.1 requires a block write across the region to SUCCEED rather than except, so a refusal is
      // reported through NET_LAST_ERROR rather than as a Modbus exception. NothingStaged is not a
      // failure — a master that wrote the values already live gets to see that in the revision.
      deps_.registers->setUint16(plc::net_reg::kLastError, static_cast<uint16_t>(error));
      return error == plc::NetApplyError::None || error == plc::NetApplyError::NothingStaged;
    }
    // A read-only address inside the block is IGNORED, not an error — §5.1 again. Returning false
    // here would make a legitimate block write across the region fail on its first read-only word.
    plc::NetRegisterMap::stageWrite(*deps_.net, address, value);
    return true;
  }

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
    if (!deps_.link || !deps_.link->stage(address, value, origin)) {
      return false;
    }
    deps_.link->publish(*deps_.registers);
    return true;
  }

  if (address == REG_LINK_APPLY) {
    if (!deps_.link || value != LinkSettingsManager::kApplyMagic) {
      return false;
    }
    if (deps_.link->apply(millis(), origin)) {
      // True for both origins: a display apply still persists, still bumps the revision
      // and still reopens the UART. Only the rollback arming differs (§4.1.1).
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
      // A measured reset clears the session volume as well, so it starts a new session and must be dated
      // like one. Missing this would leave P3 showing a start time from before the totals were wiped.
      if (deps_.clock) {
        deps_.clock->noteSessionStart(millis());
      }
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

  if (address == REG_MASTER_RESET_ALL_MAX) {
    if (value == 1) {
      for (std::size_t i = 0; i < deps_.sensorCount; ++i) {
        if (deps_.sensors[i].inUse) {
          deps_.sensors[i].maxFlowSinceReset = 0.0f;
          syncSensorToHolding(i);
        }
      }
      // Deliberately none of the rest of a measured reset: no NVS write, because the peak was never
      // persisted; no markSessionsCleared, because the LED's pulse accounting follows VOLUME; and no
      // touching the pending overrides, because nothing about the calibration has changed.
      syncGlobalRegisters();
    }
    deps_.registers->setUint16(address, 0);
    return true;
  }

  if (address == REG_MASTER_RESET_ALL_SESSION) {
    if (value == 1) {
      // Dated here, where every route into a session reset converges — a master's write, the panel's
      // confirm screen and the portal all arrive at this one command.
      if (deps_.clock) {
        deps_.clock->noteSessionStart(millis());
      }
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

  if (address == REG_DISPLAY_FLOW_UNIT) {
    // 0 L/m, 1 L/s, 2 m3/h. Refused rather than clamped: a master writing 7 has misunderstood the
    // register, and silently storing 0 would hide that.
    if (value > 2 || !deps_.displayFlowUnit) {
      return false;
    }
    *deps_.displayFlowUnit = value;
    deps_.preferences->putUShort("flow_unit", value);
    deps_.registers->setUint16(address, value);
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
    // The calibration, and ONLY the calibration. Deliberately not a narrowing of the arm above: that
    // one wipes `SensorData` as well, and this exists for the meter swap, where the volume the old
    // meter measured is real and must keep accumulating from where it stopped.
    //
    // WHAT THIS DOES NOT DO, each on purpose:
    //  - it does not touch `deps_.sensors[sensorIndex]`, so cumulativeLiters, sessionLiters,
    //    maxFlowSinceReset, pulseCount and instantFlow all survive;
    //  - it does not call `saveCumulativeToNvs`, because no measurement changed — writing the same
    //    total back would only spend a flash erase to record that nothing happened;
    //  - it does not write the cleared config to NVS either. The 60 s dirty check in firmware.cpp
    //    (`configs[i] != persistedConfigs[i]` -> `saveSensorConfig(i)`) is how EVERY config change
    //    already reaches flash, panel edits included, so persisting here would be a second path to
    //    one fact. The cost is the same as for any edit: a power cut inside that minute loses it.
    case OFF_CMD_RESET_CALIBRATION:
      if (value == 1) {
        deps_.configs[sensorIndex] = SensorCharacteristics{};
        // The Nyquist override belonged to the OLD meter's figures. Left standing, `overrideActive_`
        // makes prepareConfigUpdate return true for the very first candidate entered for the NEW
        // meter, so the replacement would be accepted without ever being sampling-checked — and
        // `evaluateSensorDiagnostics` below ORs both flags in, so the undersampling bit would also
        // stay lit for a channel that now has no configuration to undersample. The arm above does not
        // clear these; that is a separate defect, reported rather than fixed here.
        overridePending_[sensorIndex] = false;
        overrideActive_[sensorIndex] = false;
        pendingOverrides_[sensorIndex] = SensorCharacteristics{};
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
      syncSensorToHolding(sensorIndex);
      evaluateSensorDiagnostics();
      return true;
    }
    // The two calibration-form registers go through the SAME prepareConfigUpdate path as the three
    // above. That matters: a master must not be able to install a configuration the panel's own
    // editor would refuse — switching a channel to Pulses/L while its pulses-per-litre figure is
    // still zero would leave it silently unable to produce a reading.
    case OFF_CFG_CAL_TYPE: {
      if (value > static_cast<uint16_t>(CalibrationType::PulsesPerLitre)) {
        return false;
      }
      SensorCharacteristics candidate = deps_.configs[sensorIndex];
      candidate.calibration = static_cast<CalibrationType>(value);
      bool overrideAccepted = false;
      if (!prepareConfigUpdate(sensorIndex, candidate, &overrideAccepted)) {
        return false;
      }
      deps_.configs[sensorIndex] = candidate;
      syncSensorToHolding(sensorIndex);
      evaluateSensorDiagnostics();
      return true;
    }
    case OFF_CFG_PULSES_PER_L: {
      SensorCharacteristics candidate = deps_.configs[sensorIndex];
      candidate.pulses_per_litre = value;
      bool overrideAccepted = false;
      if (!prepareConfigUpdate(sensorIndex, candidate, &overrideAccepted)) {
        return false;
      }
      deps_.configs[sensorIndex] = candidate;
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
  if (configIsValid(deps_.configs[sensorIndex])) {
    status |= 0x02;
  }
  deps_.registers->setUint16(base + OFF_STATUS_FLAGS, status);

  // Readiness is derived, so a channel restored from NVS with a valid configuration publishes its real
  // totals immediately. Gated on the cached bit, this branch published 0.0 for the lifetime total after
  // every reboot — the cumulative value was intact in RAM and a master read zero.
  if (deps_.sensors[sensorIndex].inUse && configIsValid(deps_.configs[sensorIndex])) {
    deps_.registers->setFloat(base + OFF_INSTANT_FLOW, deps_.sensors[sensorIndex].instantFlow_L_min);
    deps_.registers->setDouble(base + OFF_CUMULATIVE_LITERS, deps_.sensors[sensorIndex].cumulativeLiters);
    deps_.registers->setDouble(base + OFF_CUMULATIVE_M3,
                               units::litresToCubicMeters(deps_.sensors[sensorIndex].cumulativeLiters));
    deps_.registers->setFloat(base + OFF_SESSION_LITERS, deps_.sensors[sensorIndex].sessionLiters);
    deps_.registers->setFloat(base + OFF_SESSION_M3,
                              units::litresToCubicMeters(deps_.sensors[sensorIndex].sessionLiters));
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
  deps_.registers->setUint16(base + OFF_CFG_CAL_TYPE,
                             static_cast<uint16_t>(deps_.configs[sensorIndex].calibration));
  deps_.registers->setUint16(base + OFF_CFG_PULSES_PER_L, deps_.configs[sensorIndex].pulses_per_litre);
}

void ModbusManager::syncGlobalRegisters() {
  if (deps_.link) {
    deps_.link->publish(*deps_.registers);
  }
  if (deps_.net) {
    // The whole network block, republished each sync. NetRegisterMap::publish owns the packing —
    // including R5.1's rule that secret fields read back as ZEROS rather than as stored text — so
    // the bank never holds a plaintext passphrase for a master to read.
    //
    // Republished wholesale rather than diffed: the block is 233 registers, the sync already runs at
    // the logic task's cadence, and a diff would be a second implementation of the packing
    // convention. That convention having exactly one implementation is the point of NetRegisterMap.
    uint16_t block[plc::net_reg::kEnd - plc::net_reg::kBase] = {};
    plc::NetRegisterMap::publish(*deps_.net, block, sizeof(block) / sizeof(block[0]));
    for (uint16_t i = 0; i < sizeof(block) / sizeof(block[0]); ++i) {
      deps_.registers->setUint16(static_cast<uint16_t>(plc::net_reg::kBase + i), block[i]);
    }
  }
  deps_.registers->setFloat(REG_POLLING_RATE_KHZ, *deps_.pollingRateKhz);
  deps_.registers->setUint16(REG_CONNECTED_SENSORS_BITMAP, *deps_.connectedBitmap);
  deps_.registers->setUint16(REG_MASTER_RESET_ALL_SENSORS, 0);
  deps_.registers->setUint16(REG_MASTER_RESET_ALL_MEASURED, 0);
  deps_.registers->setUint16(REG_MASTER_RESET_ALL_SESSION, 0);
  deps_.registers->setUint16(REG_UNDERSAMPLING_FLAGS, *deps_.undersamplingFlags);
  deps_.registers->setUint16(REG_LED_RED_VOLUME_STEP, deps_.ledController->volumeStepLiters());
  deps_.registers->setUint16(REG_LED_RED_PULSE_PERIOD, deps_.ledController->pulsePeriodMs());
  if (deps_.displayFlowUnit) {
    deps_.registers->setUint16(REG_DISPLAY_FLOW_UNIT, *deps_.displayFlowUnit);
  }
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
  // A request addressed to the LIVE slave ID proves the master followed the change,
  // which is what closes the rollback window after an apply. See noteValidFrame: a
  // frame on a stale ID must not confirm, or a slave-ID change can never roll back.
  if (deps_.link) {
    deps_.link->noteValidFrame(millis(), request.getServerID());
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
    deps_.link->noteValidFrame(millis(), request.getServerID());
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
    deps_.link->noteValidFrame(millis(), request.getServerID());
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

  /**
   * THE NETWORK BLOCK IS PRE-VALIDATED BY A DIFFERENT RULE, and leaving it out of this loop broke §5's
   * whole documented setup sequence.
   *
   * `isWritableAddress` knows only the sensor and link registers — `applyHoldingWrite` handles the
   * network block ABOVE its call to that predicate, for exactly that reason. This loop called the
   * predicate directly, so every address from 500 up answered false and a block write anywhere in the
   * region excepted with ILLEGAL_DATA_ADDRESS on its FIRST word. Writing an SSID over FC16 was
   * impossible; FC6 at the same address worked, which is the asymmetry that gave it away.
   *
   * Inside the block a read-only address is IGNORED rather than refused (§5.1), so the only thing that
   * can fail here is the block not being served at all — `deps_.net` null, which is the case on a build
   * with no network module. Refusing that as an address error matches what FC6 does with it.
   */
  for (uint16_t i = 0; i < words; ++i) {
    const uint16_t at = address + i;
    const bool writable =
        plc::NetRegisterMap::contains(at) ? (deps_.net != nullptr) : isWritableAddress(at);
    if (!writable) {
      response.setError(request.getServerID(), request.getFunctionCode(), Modbus::ILLEGAL_DATA_ADDRESS);
      return response;
    }
  }

  /**
   * NET_APPLY IS LIFTED OUT OF THE LOOP, for two reasons of quite different weight today.
   *
   * THE ONE THAT BITES NOW: a zero-fill across the region passes over 730, and a zero is not the apply
   * magic. In the loop that refusal became SERVER_DEVICE_FAILURE — so simply teaching the validation
   * loop about the network block would have traded an exception on register one for a partial write plus
   * an exception at 730, which is worse. Out here the return can be ignored, because §5.1 requires the
   * sequence to succeed and NET_LAST_ERROR already carries the reason.
   *
   * THE ONE THAT IS LATENT: order. Nothing WRITABLE currently sits above 730 — 731 and 732 are
   * read-only and 733 is the end — so in the layout as it stands, applying in address order would
   * commit no less than this does. It is deferred anyway because that is a property of the layout and
   * not of the protocol: the moment any writable register is placed above the apply, a frame spanning
   * it would commit before that register staged, and the revision would claim a value it had not
   * applied. Cheap here, invisible later.
   *
   * REG_LINK_APPLY needs no such treatment: it is the last writable address of its block (45 is
   * read-only), so any frame reaching it has already staged 40-43 and any frame reaching PAST it is
   * refused above.
   */
  bool applyRequested = false;
  uint16_t applyValue = 0;

  for (uint16_t i = 0; i < words; ++i) {
    const uint16_t at = address + i;
    const uint16_t value =
        (static_cast<uint16_t>(buffer[2 * i]) << 8) | static_cast<uint16_t>(buffer[2 * i + 1]);
    if (at == plc::net_reg::kApply) {
      applyRequested = true;
      applyValue = value;
      continue;
    }
    if (!applyHoldingWrite(at, value)) {
      response.setError(request.getServerID(), request.getFunctionCode(), Modbus::SERVER_DEVICE_FAILURE);
      return response;
    }
  }

  if (applyRequested) {
    // THE RETURN IS DELIBERATELY IGNORED HERE, and this is where FC6 and FC16 part company at 730.
    //
    // §5.1 requires a master zero-filling the whole region to SUCCEED, and a zero is not the apply
    // magic — so the refusal that a single-register write reports as an exception must not become one
    // here, or the documented sequence excepts on a register it was only passing over.
    // `applyHoldingWrite` has already recorded the reason at NET_LAST_ERROR, which is where a master
    // that meant to commit looks to find out that it did not.
    applyHoldingWrite(plc::net_reg::kApply, applyValue);
  }

  response.add(request.getServerID(), request.getFunctionCode(), address, words);
  return response;
}

/**
 * Can the sampler keep up with the highest frequency this channel can produce?
 *
 * EACH CALIBRATION FORM HAS ITS OWN CEILING, because each states the meter differently. This used to
 * compute the formula ceiling only and `return false` on `f_multiplier == 0` — which is the CORRECT and
 * normal state of a channel calibrated by pulses per litre, whose multiplier field is unused and reads
 * `--` on the panel by design (ui_settings_types.cpp). So every valid pulses-per-litre channel failed a
 * check it was never given a chance to pass, with two consequences on a device nobody had misconfigured:
 *
 *   - `evaluateSensorDiagnostics` ORs in `valid && !meets`, so the channel's bit in
 *     REG_UNDERSAMPLING_FLAGS stayed lit forever. Register 30 reported a fault that did not exist, the
 *     panel wore the warning banner permanently, and the P0 summary counted the channel as
 *     undersampling beside genuinely undersampling ones.
 *   - `prepareConfigUpdate` parks a candidate that fails this check, so the figures could only be
 *     installed by writing the same value twice — the §5.5 override handshake, demanded for a
 *     configuration that never needed overriding. An operator following the datasheet would see
 *     "Sampling too slow" on a meter well inside the sampler's budget.
 *
 * The ceilings are the ENGINE'S OWN inversions, read back from sensor_state_engine.cpp rather than
 * re-derived: it computes `flow = (F - adjust) / f_multiplier` for the formula and `flow = F * 60 / K`
 * for pulses per litre, so at `flow == q_max` the frequencies are `f_multiplier * q_max + adjust` and
 * `K * q_max / 60`. Anything else here would budget against a frequency the device does not actually
 * expect to see.
 *
 * `q_max == 0` stays fatal for both forms: with no nominal maximum there is no ceiling flow to convert.
 * The per-form divisor is fatal only for the form that uses it — and `configIsValid` already refuses
 * both of those cases, so a caller reaching here with one is asking about a configuration that will not
 * be installed either way.
 *
 * BOTH CEILINGS AND THE MARGIN ARE NOW MIRRORED IN TYPESCRIPT, in
 * `web/mockup/src/utils/sensorConfig.ts`, so the simulator can show an under-sampling condition ARISING
 * FROM A CONFIGURATION instead of from a checkbox nothing fed. `__tests__/nyquist.test.ts` reads this
 * function's two expressions and `plc::kSamplingMarginFactor` out of the source rather than trusting the
 * copies — including the engine's own inversions in `sensor_state_engine.cpp`, so the mirror is pinned to
 * the code that turns a frequency into a flow rather than to this comment about it. Changing either
 * ceiling here fails that test, which is the intended way to find out.
 *
 * TWO ARMS BELOW ADMIT CONFIGURATIONS THAT CANNOT BE MEASURED, and they are reported rather than changed:
 * the formula ceiling clamps at 0 and a ceiling of 0 returns true, so `m` negative (nothing rejects it —
 * `configIsValid` tests only `!= 0`) and `adjust` large and negative both PASS. Verified against this
 * function and the engine: `m=-10, q=150` reads 0 L/min at every frequency, and `m=200, a=-30000, q=150`
 * reads a flat 150 L/min at ZERO pulses. Both report `OK` with register 30 clear. Fixing that is a
 * decision about what `configIsValid` should bound, not a tidy-up of this function.
 */
bool ModbusManager::meetsNyquistLimit(const SensorCharacteristics& cfg) const {
  if (cfg.q_max == 0) {
    return false;
  }
  double theoreticalFrequency = 0.0;
  if (cfg.calibration == CalibrationType::PulsesPerLitre) {
    if (cfg.pulses_per_litre == 0) {
      return false;
    }
    // q_max is L/min and K is pulses per LITRE, so the 60 converts the rate to litres per second. It is
    // the same division the engine performs in reverse, kept in double so a 65535 x 65535 product
    // cannot overflow on the way to the comparison.
    theoreticalFrequency =
        static_cast<double>(cfg.pulses_per_litre) * static_cast<double>(cfg.q_max) / 60.0;
  } else {
    if (cfg.f_multiplier == 0) {
      return false;
    }
    theoreticalFrequency =
        std::max(0.0, static_cast<double>(cfg.f_multiplier) * static_cast<double>(cfg.q_max) +
                             static_cast<double>(cfg.adjust));
  }
  const double pollingHz = static_cast<double>(*deps_.pollingRateKhz) * 1000.0;
  if (theoreticalFrequency <= 0.0) {
    return true;
  }
  return pollingHz >= (plc::kSamplingMarginFactor * theoreticalFrequency);
}

bool ModbusManager::prepareConfigUpdate(std::size_t index,
                                        const SensorCharacteristics& candidate,
                                        bool* acceptedOverride) {
  if (acceptedOverride) {
    *acceptedOverride = false;
  }

  /**
   * AN INCOMPLETE CANDIDATE IS NOT A BAD ONE. It is what every field-by-field entry looks like before
   * the last field arrives, and refusing all of them deadlocked the device.
   *
   * This used to `return false` outright, which made a channel holding an all-zero configuration
   * impossible to configure BY ANY ROUTE. Every arm above builds its candidate from the config already
   * in force and changes one field, so from all zeros both entry orders are refused: q_max alone still
   * fails on `f_multiplier == 0` (or `pulses_per_litre == 0` in the pulses form), and the multiplier
   * alone still fails on `q_max == 0`. There is no third order — the two fields `configIsValid` demands
   * cannot both arrive in one single-register write. A factory-fresh channel therefore shipped
   * unconfigurable, and it went unnoticed because a channel whose NVS already held a valid
   * configuration kept working and stayed editable. `OFF_CMD_RESET_CALIBRATION` is what made it
   * reachable on purpose: the panel now offers the operator a way INTO that state, so a way out of it
   * is no longer optional.
   *
   * THE PROPERTY THAT MATTERS IS PRESERVED, and it is the one `register_map.h` relies on: a channel that
   * IS validly calibrated still cannot be demolished field by field, because each such write would
   * invalidate a valid configuration and is refused here. "Not set" remains expressible only as a
   * command, never as a value write. What changes is the mirror case, which was never the point of the
   * rule: an already-invalid channel may take a field on the way to becoming valid.
   *
   * Nothing downstream needs an incomplete config to be special-cased. Readiness is DERIVED
   * (`configIsValid(configs[n])`, evaluated wherever it is asked), so a half-entered channel renders
   * `SET?`, counts as uncalibrated in the panel summary, and produces no flow — exactly as it did while
   * every field was still zero. The sampling check is not skipped either, only deferred to the write
   * that completes the configuration, which is the first candidate a frequency can be computed from at
   * all.
   *
   * The override state is cleared on this path either way: whatever candidate was parked awaiting a
   * §5.5 confirmation is not the one being written now.
   */
  if (!configIsValid(candidate)) {
    overridePending_[index] = false;
    overrideActive_[index] = false;
    pendingOverrides_[index] = SensorCharacteristics{};
    return !configIsValid(deps_.configs[index]);
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
  if (deps_.aggregateFlowLpmCache) {
    *deps_.aggregateFlowLpmCache = 0.0;
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
