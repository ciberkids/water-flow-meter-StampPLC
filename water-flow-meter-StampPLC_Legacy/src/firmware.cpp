#include <M5StamPLC.h>
#include <Preferences.h> // For persistent storage
#include <eModbus.h>
#include <array>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <cstring>
#include <vector>

// --- Configuration ---
#define NUM_SENSORS 8
#define MODBUS_SLAVE_ID 1
#define RS485_SERIAL_PORT Serial2 // M5Stack StampPLC uses Serial2 for RS485
const int RS485_TX_PIN = 17;
const int RS485_RX_PIN = 16;
const int RS485_DE_PIN = 2;  // Direction Enable pin for RS485 transceiver

// --- Modbus Register Definitions ---
// Global
#define REG_POLLING_RATE_KHZ 0
#define REG_CONNECTED_SENSORS_BITMAP 10
#define REG_MASTER_RESET_ALL_SENSORS 20
#define REG_MASTER_RESET_ALL_MEASURED 21
#define REG_MASTER_RESET_ALL_SESSION 22
// Per-sensor offsets
#define SENSOR_BLOCK_SIZE 40
#define SENSOR_1_BASE_ADDR 100
#define OFF_STATUS_FLAGS 0
#define OFF_INSTANT_FLOW 1
#define OFF_CUMULATIVE_LITERS 3
#define OFF_CUMULATIVE_M3 7
#define OFF_SESSION_LITERS 11
#define OFF_SESSION_M3 13
#define OFF_MAX_FLOW 15
#define OFF_CMD_RESET_SESSION 17
#define OFF_CMD_RESET_ALL 18
#define OFF_CMD_RESET_CONFIG 19
#define OFF_CFG_Q_MAX 20
#define OFF_CFG_F_MULT 21
#define OFF_CFG_ADJUST 22

// --- Data Structures ---
struct SensorCharacteristics {
  uint16_t q_max = 0;
  int16_t f_multiplier = 0;
  int16_t adjust = 0;
};

struct SensorData {
  bool inUse = false;
  bool isReady = false; // Configured and ready
  volatile uint32_t pulseCount = 0;
  float instantFlow_L_s = 0.0;
  double cumulativeLiters = 0.0;
  float sessionLiters = 0.0;
  float maxFlowSinceReset = 0.0;
};

// --- Global Variables ---
SensorData sensors[NUM_SENSORS];
SensorCharacteristics configs[NUM_SENSORS];
Preferences preferences;
ModbusServerRTU modbus(2000, RS485_DE_PIN);
volatile float pollingRate_kHz = 0.0;
TaskHandle_t PollingTask;
TaskHandle_t LogicTask;
constexpr uint16_t TOTAL_REG_COUNT = SENSOR_1_BASE_ADDR + SENSOR_BLOCK_SIZE * NUM_SENSORS;
std::array<uint16_t, TOTAL_REG_COUNT> holdingRegisters{};
uint16_t connectedSensorsBitmap = 0;

// --- Helper Function Prototypes ---
uint16_t sensorBaseAddress(uint8_t sensorIndex);
void writeRegister(uint16_t address, uint16_t value);
void writeFloatRegister(uint16_t address, float value);
void writeDoubleRegister(uint16_t address, double value);
bool isWritableAddress(uint16_t address);
void syncSensorToHolding(uint8_t sensorIndex);
void syncGlobalRegisters();
bool applyHoldingWrite(uint16_t address, uint16_t value);
ModbusMessage handleReadHolding(ModbusMessage request);
ModbusMessage handleWriteSingle(ModbusMessage request);
ModbusMessage handleWriteMultiple(ModbusMessage request);

// --- Function Prototypes ---
void resetSensorSession(uint8_t sensorIndex, bool updateHolding = true);
void resetSensorAll(uint8_t sensorIndex, bool updateHolding = true);
void resetSensorConfig(uint8_t sensorIndex, bool updateHolding = true);
void saveCumulativeData(uint8_t sensorIndex);
void loadCumulativeData(uint8_t sensorIndex);
bool isConfigValid(uint8_t sensorIndex);


// --- Helper Implementations ---
inline bool isRegisterRangeValid(uint16_t address, uint16_t count = 1) {
  return (address + count) <= TOTAL_REG_COUNT;
}

uint16_t sensorBaseAddress(uint8_t sensorIndex) {
  return SENSOR_1_BASE_ADDR + sensorIndex * SENSOR_BLOCK_SIZE;
}

void writeRegister(uint16_t address, uint16_t value) {
  if (address < TOTAL_REG_COUNT) {
    holdingRegisters[address] = value;
  }
}

void writeFloatRegister(uint16_t address, float value) {
  if (!isRegisterRangeValid(address, 2)) {
    return;
  }
  uint32_t raw = 0;
  std::memcpy(&raw, &value, sizeof(float));
  holdingRegisters[address] = static_cast<uint16_t>((raw >> 16) & 0xFFFF);
  holdingRegisters[address + 1] = static_cast<uint16_t>(raw & 0xFFFF);
}

void writeDoubleRegister(uint16_t address, double value) {
  if (!isRegisterRangeValid(address, 4)) {
    return;
  }
  uint64_t raw = 0;
  std::memcpy(&raw, &value, sizeof(double));
  holdingRegisters[address] = static_cast<uint16_t>((raw >> 48) & 0xFFFF);
  holdingRegisters[address + 1] = static_cast<uint16_t>((raw >> 32) & 0xFFFF);
  holdingRegisters[address + 2] = static_cast<uint16_t>((raw >> 16) & 0xFFFF);
  holdingRegisters[address + 3] = static_cast<uint16_t>(raw & 0xFFFF);
}

bool isWritableAddress(uint16_t address) {
  if (!isRegisterRangeValid(address)) {
    return false;
  }
  if (address == REG_CONNECTED_SENSORS_BITMAP ||
      address == REG_MASTER_RESET_ALL_SENSORS ||
      address == REG_MASTER_RESET_ALL_MEASURED ||
      address == REG_MASTER_RESET_ALL_SESSION) {
    return true;
  }
  if (address < SENSOR_1_BASE_ADDR) {
    return false;
  }
  uint8_t sensorIndex = (address - SENSOR_1_BASE_ADDR) / SENSOR_BLOCK_SIZE;
  if (sensorIndex >= NUM_SENSORS) {
    return false;
  }
  uint16_t base = sensorBaseAddress(sensorIndex);
  uint16_t offset = address - base;
  if (!sensors[sensorIndex].inUse) {
    return false;
  }
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

void syncSensorToHolding(uint8_t sensorIndex) {
  if (sensorIndex >= NUM_SENSORS) {
    return;
  }
  uint16_t base = sensorBaseAddress(sensorIndex);
  uint16_t status = 0;
  if (sensors[sensorIndex].inUse) {
    status |= 0x01;
  }
  if (sensors[sensorIndex].isReady) {
    status |= 0x02;
  }
  writeRegister(base + OFF_STATUS_FLAGS, status);

  if (sensors[sensorIndex].inUse && sensors[sensorIndex].isReady) {
    writeFloatRegister(base + OFF_INSTANT_FLOW, sensors[sensorIndex].instantFlow_L_s);
    writeDoubleRegister(base + OFF_CUMULATIVE_LITERS, sensors[sensorIndex].cumulativeLiters);
    writeDoubleRegister(base + OFF_CUMULATIVE_M3, sensors[sensorIndex].cumulativeLiters / 1000.0);
    writeFloatRegister(base + OFF_SESSION_LITERS, sensors[sensorIndex].sessionLiters);
    writeFloatRegister(base + OFF_SESSION_M3, sensors[sensorIndex].sessionLiters / 1000.0f);
    writeFloatRegister(base + OFF_MAX_FLOW, sensors[sensorIndex].maxFlowSinceReset);
  } else {
    writeFloatRegister(base + OFF_INSTANT_FLOW, 0.0f);
    writeDoubleRegister(base + OFF_CUMULATIVE_LITERS, 0.0);
    writeDoubleRegister(base + OFF_CUMULATIVE_M3, 0.0);
    writeFloatRegister(base + OFF_SESSION_LITERS, 0.0f);
    writeFloatRegister(base + OFF_SESSION_M3, 0.0f);
    writeFloatRegister(base + OFF_MAX_FLOW, 0.0f);
  }

  writeRegister(base + OFF_CFG_Q_MAX, configs[sensorIndex].q_max);
  writeRegister(base + OFF_CFG_F_MULT, static_cast<uint16_t>(configs[sensorIndex].f_multiplier));
  writeRegister(base + OFF_CFG_ADJUST, static_cast<uint16_t>(configs[sensorIndex].adjust));
}

void syncGlobalRegisters() {
  writeFloatRegister(REG_POLLING_RATE_KHZ, pollingRate_kHz);
  writeRegister(REG_CONNECTED_SENSORS_BITMAP, connectedSensorsBitmap);
  writeRegister(REG_MASTER_RESET_ALL_SENSORS, 0);
  writeRegister(REG_MASTER_RESET_ALL_MEASURED, 0);
  writeRegister(REG_MASTER_RESET_ALL_SESSION, 0);
}

ModbusMessage handleReadHolding(ModbusMessage request) {
  uint16_t address = 0;
  uint16_t words = 0;
  ModbusMessage response;
  request.get(2, address);
  request.get(4, words);

  if (words == 0 || !isRegisterRangeValid(address, words)) {
    response.setError(request.getServerID(), request.getFunctionCode(), Modbus::ILLEGAL_DATA_ADDRESS);
    return response;
  }

  response.add(request.getServerID(), request.getFunctionCode(), static_cast<uint8_t>(words * 2));
  for (uint16_t i = 0; i < words; ++i) {
    response.add(holdingRegisters[address + i]);
  }
  return response;
}

ModbusMessage handleWriteSingle(ModbusMessage request) {
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

ModbusMessage handleWriteMultiple(ModbusMessage request) {
  uint16_t address = 0;
  uint16_t words = 0;
  uint8_t byteCount = 0;
  ModbusMessage response;
  request.get(2, address);
  request.get(4, words);
  request.get(6, byteCount);

  if (words == 0 || byteCount != words * 2 || !isRegisterRangeValid(address, words)) {
    response.setError(request.getServerID(), request.getFunctionCode(), Modbus::ILLEGAL_DATA_VALUE);
    return response;
  }

  std::vector<uint8_t> buffer;
  request.get(7, buffer, byteCount);

  // Pre-validate write access
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

bool applyHoldingWrite(uint16_t address, uint16_t value) {
  if (!isWritableAddress(address)) {
    return false;
  }

  if (address == REG_CONNECTED_SENSORS_BITMAP) {
    connectedSensorsBitmap = value;
    for (uint8_t i = 0; i < NUM_SENSORS; ++i) {
      bool shouldEnable = (value >> i) & 0x01;
      if (shouldEnable && !sensors[i].inUse) {
        sensors[i].inUse = true;
        resetSensorConfig(i, false);
        syncSensorToHolding(i);
      } else if (!shouldEnable && sensors[i].inUse) {
        sensors[i].inUse = false;
        sensors[i].pulseCount = 0;
        resetSensorConfig(i, false);
        syncSensorToHolding(i);
      } else {
        syncSensorToHolding(i);
      }
    }
    writeRegister(address, connectedSensorsBitmap);
    return true;
  }

  if (address == REG_MASTER_RESET_ALL_SENSORS) {
    if (value == 1) {
      for (uint8_t i = 0; i < NUM_SENSORS; ++i) {
        if (sensors[i].inUse) {
          resetSensorConfig(i, false);
          syncSensorToHolding(i);
        }
      }
    }
    writeRegister(address, 0);
    return true;
  }

  if (address == REG_MASTER_RESET_ALL_MEASURED) {
    if (value == 1) {
      for (uint8_t i = 0; i < NUM_SENSORS; ++i) {
        if (sensors[i].inUse) {
          resetSensorAll(i, false);
          syncSensorToHolding(i);
        }
      }
    }
    writeRegister(address, 0);
    return true;
  }

  if (address == REG_MASTER_RESET_ALL_SESSION) {
    if (value == 1) {
      for (uint8_t i = 0; i < NUM_SENSORS; ++i) {
        if (sensors[i].inUse) {
          resetSensorSession(i, false);
          syncSensorToHolding(i);
        }
      }
    }
    writeRegister(address, 0);
    return true;
  }

  // Sensor-scoped writes
  uint8_t sensorIndex = (address - SENSOR_1_BASE_ADDR) / SENSOR_BLOCK_SIZE;
  if (sensorIndex >= NUM_SENSORS || !sensors[sensorIndex].inUse) {
    return false;
  }
  uint16_t base = sensorBaseAddress(sensorIndex);
  uint16_t offset = address - base;

  switch (offset) {
    case OFF_CMD_RESET_SESSION:
      if (value == 1) {
        resetSensorSession(sensorIndex, false);
      }
      syncSensorToHolding(sensorIndex);
      writeRegister(address, 0);
      return true;
    case OFF_CMD_RESET_ALL:
      if (value == 1) {
        resetSensorAll(sensorIndex, false);
      }
      syncSensorToHolding(sensorIndex);
      writeRegister(address, 0);
      return true;
    case OFF_CMD_RESET_CONFIG:
      if (value == 1) {
        resetSensorConfig(sensorIndex, false);
      }
      syncSensorToHolding(sensorIndex);
      writeRegister(address, 0);
      return true;
    case OFF_CFG_Q_MAX:
      configs[sensorIndex].q_max = value;
      sensors[sensorIndex].isReady = isConfigValid(sensorIndex);
      writeRegister(address, configs[sensorIndex].q_max);
      syncSensorToHolding(sensorIndex);
      return true;
    case OFF_CFG_F_MULT:
      configs[sensorIndex].f_multiplier = static_cast<int16_t>(value);
      sensors[sensorIndex].isReady = isConfigValid(sensorIndex);
      writeRegister(address, static_cast<uint16_t>(configs[sensorIndex].f_multiplier));
      syncSensorToHolding(sensorIndex);
      return true;
    case OFF_CFG_ADJUST:
      configs[sensorIndex].adjust = static_cast<int16_t>(value);
      sensors[sensorIndex].isReady = isConfigValid(sensorIndex);
      writeRegister(address, static_cast<uint16_t>(configs[sensorIndex].adjust));
      syncSensorToHolding(sensorIndex);
      return true;
    default:
      return false;
  }
}

//===================================================================
// TASK 1: Dedicated Flow Meter Polling (runs on Core 0)
//===================================================================
void pollingTaskCode(void * pvParameters) {
  byte lastPinStates = M5StamPLC.IO.getDigitalInput();
  uint32_t loopCounter = 0;
  unsigned long lastRateCalcTime = millis();

  for (;;) {
    byte currentPinStates = M5StamPLC.IO.getDigitalInput();
    for (int i = 0; i < NUM_SENSORS; i++) {
      if (sensors[i].inUse) {
        if ((currentPinStates & (1 << i)) && !(lastPinStates & (1 << i))) {
          sensors[i].pulseCount++;
        }
      }
    }
    lastPinStates = currentPinStates;
    loopCounter++;

    // Calculate polling rate every second
    if (millis() - lastRateCalcTime >= 1000) {
      pollingRate_kHz = (float)loopCounter / (millis() - lastRateCalcTime);
      loopCounter = 0;
      lastRateCalcTime = millis();
    }
  }
}

//===================================================================
// TASK 2: Modbus, Calculations, and Logic (runs on Core 1)
//===================================================================
void logicTaskCode(void * pvParameters) {
  unsigned long lastCalcTime = millis();
  unsigned long lastSaveTime = millis();

  // Initialize Modbus RTU server
  RTUutils::prepareHardwareSerial(RS485_SERIAL_PORT);
  RS485_SERIAL_PORT.begin(9600, SERIAL_8N1, RS485_RX_PIN, RS485_TX_PIN);
  // Load persistent data at startup before serving Modbus requests
  preferences.begin("flow-data", false);
  for (int i = 0; i < NUM_SENSORS; i++) {
    loadCumulativeData(i);
    sensors[i].inUse = false;
    sensors[i].isReady = false;
    syncSensorToHolding(i);
  }
  syncGlobalRegisters();

  modbus.registerWorker(MODBUS_SLAVE_ID, Modbus::READ_HOLD_REGISTER, handleReadHolding);
  modbus.registerWorker(MODBUS_SLAVE_ID, Modbus::WRITE_HOLD_REGISTER, handleWriteSingle);
  modbus.registerWorker(MODBUS_SLAVE_ID, Modbus::WRITE_MULT_REGISTERS, handleWriteMultiple);
  modbus.begin(RS485_SERIAL_PORT);

  for (;;) {
    unsigned long now = millis();
    if (now - lastCalcTime >= 1000) {
      float elapsedTime_s = (now - lastCalcTime) / 1000.0f;
      lastCalcTime = now;
      for (int i = 0; i < NUM_SENSORS; i++) {
        if (sensors[i].inUse) {
          uint32_t pulses = sensors[i].pulseCount;
          sensors[i].pulseCount = 0; // Reset for next interval

          if (sensors[i].isReady && configs[i].f_multiplier != 0) {
            float frequency = (float)pulses / elapsedTime_s;
            // Formula: Q(L/min) = (Freq - Adjust) / Multiplier
            float flowRate_L_min = (frequency - configs[i].adjust) / configs[i].f_multiplier;
            if (flowRate_L_min < 0) flowRate_L_min = 0;
            if (flowRate_L_min > configs[i].q_max) flowRate_L_min = configs[i].q_max;

            sensors[i].instantFlow_L_s = flowRate_L_min / 60.0;
            if (sensors[i].instantFlow_L_s > sensors[i].maxFlowSinceReset) {
              sensors[i].maxFlowSinceReset = sensors[i].instantFlow_L_s;
            }

            double liters_in_interval = sensors[i].instantFlow_L_s * elapsedTime_s;
            sensors[i].sessionLiters += liters_in_interval;
            sensors[i].cumulativeLiters += liters_in_interval;
          } else {
            sensors[i].instantFlow_L_s = 0.0f;
          }
        } else {
          sensors[i].instantFlow_L_s = 0.0f;
        }
        syncSensorToHolding(i);
      }
      syncGlobalRegisters();
    }

    // Save cumulative data periodically to prevent excessive NVS writes
    if (now - lastSaveTime > 60000) { // Every minute
      for (int i = 0; i < NUM_SENSORS; i++) {
        if (sensors[i].inUse) saveCumulativeData(i);
      }
      lastSaveTime = now;
    }
    
    vTaskDelay(1); // Yield to other tasks
  }
}

//===================================================================
// SETUP: Initializes hardware and creates the tasks
//===================================================================
void setup() {
  M5StamPLC.begin();
  Serial.begin(115200);

  xTaskCreatePinnedToCore(pollingTaskCode, "PollingTask", 4096, NULL, 2, &PollingTask, 0);
  xTaskCreatePinnedToCore(logicTaskCode, "LogicTask", 10000, NULL, 1, &LogicTask, 1);
}

void loop() {
  // Empty, all work is done in tasks.
  vTaskDelete(NULL); // Delete the default Arduino loop task
}

// --- Helper Functions ---
bool isConfigValid(uint8_t i) {
  if (i >= NUM_SENSORS) {
    return false;
  }
  if (configs[i].q_max == 0 || configs[i].f_multiplier == 0) {
    return false;
  }
  int32_t adjust = static_cast<int32_t>(configs[i].adjust);
  int32_t multiplier = std::max<int32_t>(std::abs(static_cast<int32_t>(configs[i].f_multiplier)), 1);
  int32_t limit = static_cast<int32_t>(configs[i].q_max) * multiplier * 10;
  return std::abs(adjust) <= limit;
}

void resetSensorSession(uint8_t i, bool updateHolding) {
  if (i >= NUM_SENSORS) return;
  sensors[i].sessionLiters = 0.0;
  sensors[i].maxFlowSinceReset = 0.0;
  if (updateHolding) {
    syncSensorToHolding(i);
  }
}

void resetSensorAll(uint8_t i, bool updateHolding) {
  if (i >= NUM_SENSORS) return;
  resetSensorSession(i, false);
  sensors[i].cumulativeLiters = 0.0;
  saveCumulativeData(i); // Persist the reset
  if (updateHolding) {
    syncSensorToHolding(i);
  }
}

void resetSensorConfig(uint8_t i, bool updateHolding) {
  if (i >= NUM_SENSORS) return;
  resetSensorAll(i, false);
  configs[i].q_max = 0;
  configs[i].f_multiplier = 0;
  configs[i].adjust = 0;
  sensors[i].isReady = false;
  if (updateHolding) {
    syncSensorToHolding(i);
  }
}

void saveCumulativeData(uint8_t i) {
  if (i >= NUM_SENSORS) return;
  String key = "cml_" + String(i);
  preferences.putDouble(key.c_str(), sensors[i].cumulativeLiters);
}

void loadCumulativeData(uint8_t i) {
  if (i >= NUM_SENSORS) return;
  String key = "cml_" + String(i);
  sensors[i].cumulativeLiters = preferences.getDouble(key.c_str(), 0.0);
}
