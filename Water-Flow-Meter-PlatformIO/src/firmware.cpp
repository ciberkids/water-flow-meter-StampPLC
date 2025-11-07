#include <M5StamPLC.h>
#include <Preferences.h>
#include <eModbus.h>
#include <esp_system.h>

#include <cstdint>
#include <cstdio>

#include "button_input.h"
#include "led_controller.h"
#include "modbus_manager.h"
#include "register_bank.h"
#include "register_map.h"
#include "sensor_types.h"
#include "ui_controller.h"
#include "ui_renderer.h"

using namespace plc;

// --- Hardware configuration ---
constexpr HardwareSerial& RS485_SERIAL_PORT = Serial2;
constexpr int RS485_TX_PIN = 17;
constexpr int RS485_RX_PIN = 16;
constexpr int RS485_DE_PIN = 2;  // Direction Enable pin for RS485 transceiver

// --- Global State ---
SensorData sensors[kNumSensors];
SensorCharacteristics configs[kNumSensors];
Preferences preferences;
ModbusServerRTU modbus(2000, RS485_DE_PIN);
volatile float pollingRate_kHz = 0.0f;
TaskHandle_t PollingTask;
TaskHandle_t LogicTask;
RegisterBank registerBank;
LedController ledController;
ButtonInputManager buttonInput;
uint16_t connectedSensorsBitmap = 0;
uint16_t undersamplingFlags = 0;
double totalSessionLitersCache = 0.0;
double aggregateFlowLpsCache = 0.0;
bool allSensorsReadyCache = true;

ModbusDependencies modbusDeps{.sensors = sensors,
                              .configs = configs,
                              .preferences = &preferences,
                              .registers = &registerBank,
                              .ledController = &ledController,
                              .connectedBitmap = &connectedSensorsBitmap,
                              .undersamplingFlags = &undersamplingFlags,
                              .totalSessionLitersCache = &totalSessionLitersCache,
                              .aggregateFlowLpsCache = &aggregateFlowLpsCache,
                              .allSensorsReadyCache = &allSensorsReadyCache,
                              .pollingRateKhz = &pollingRate_kHz,
                              .sensorCount = kNumSensors};

ModbusManager modbusManager(modbusDeps);
UiRenderer uiRenderer;
UiController uiController;

namespace {

constexpr uint32_t kFactoryResetHoldMs = 30000;
constexpr uint32_t kFactoryResetOverlayDelayMs = 3000;
constexpr uint32_t kFactoryResetRestartDelayMs = 1000;
constexpr uint32_t kEnterIdleHoldMs = 3000;

struct FactoryResetState {
  bool holdActive = false;
  bool overlayActive = false;
  uint32_t holdStartMs = 0;
  bool restartScheduled = false;
  uint32_t restartAtMs = 0;
};

FactoryResetState factoryResetState;
bool enterIdleLatch = false;

void saveCumulativeData(uint8_t index) {
  if (index >= kNumSensors) {
    return;
  }
  char key[8];
  std::snprintf(key, sizeof(key), "cml_%u", static_cast<unsigned>(index));
  preferences.putDouble(key, sensors[index].cumulativeLiters);
}

void loadCumulativeData(uint8_t index) {
  if (index >= kNumSensors) {
    return;
  }
  char key[8];
  std::snprintf(key, sizeof(key), "cml_%u", static_cast<unsigned>(index));
  sensors[index].cumulativeLiters = preferences.getDouble(key, 0.0);
}

void updateDiagnostics() {
  modbusManager.evaluateSensorDiagnostics();
  undersamplingFlags = registerBank.at(REG_UNDERSAMPLING_FLAGS);
}

void performFactoryReset() {
  modbusManager.applyHoldingWrite(REG_MASTER_RESET_ALL_SENSORS, 1);
  modbusManager.applyHoldingWrite(REG_CONNECTED_SENSORS_BITMAP, 0);

  preferences.clear();
  ledController.resetToDefaults();
  ledController.markSessionsCleared();
  ledController.saveToPreferences(preferences);

  registerBank.fill(0);
  totalSessionLitersCache = 0.0;
  aggregateFlowLpsCache = 0.0;
  allSensorsReadyCache = true;
  undersamplingFlags = 0;
  registerBank.setUint16(REG_UNDERSAMPLING_FLAGS, 0);

  for (std::size_t i = 0; i < kNumSensors; ++i) {
    sensors[i] = SensorData{};
    configs[i] = SensorCharacteristics{};
    modbusManager.syncSensorToHolding(i);
  }

  modbusManager.syncGlobalRegisters();
  updateDiagnostics();
}

void handleFactoryReset(uint32_t nowMs, UiCountdownState* countdown) {
  if (!countdown) {
    return;
  }

  const bool upPressed = buttonInput.isPressed(ButtonInputManager::Button::Up);
  const bool downPressed = buttonInput.isPressed(ButtonInputManager::Button::Down);
  const bool enterPressed = buttonInput.isPressed(ButtonInputManager::Button::Enter);

  if (!factoryResetState.restartScheduled) {
    if (!factoryResetState.holdActive) {
      if (upPressed && downPressed && !enterPressed) {
        factoryResetState.holdActive = true;
        factoryResetState.overlayActive = false;
        factoryResetState.holdStartMs = nowMs;
        buttonInput.clearEvents();
      }
    } else {
      if (!(upPressed && downPressed) || enterPressed) {
        factoryResetState.holdActive = false;
        factoryResetState.overlayActive = false;
        buttonInput.clearEvents();
      } else {
        const uint32_t elapsedMs = nowMs - factoryResetState.holdStartMs;
        if (!factoryResetState.overlayActive && elapsedMs >= kFactoryResetOverlayDelayMs) {
          factoryResetState.overlayActive = true;
        }
        if (factoryResetState.overlayActive) {
          const uint32_t remainingMs =
              (elapsedMs >= kFactoryResetHoldMs) ? 0 : (kFactoryResetHoldMs - elapsedMs);
          countdown->active = true;
          countdown->secondsRemaining = (remainingMs + 999) / 1000;
          countdown->label = "Hold UP+DOWN to factory reset (30->0)";
        }
        if (elapsedMs >= kFactoryResetHoldMs) {
          performFactoryReset();
          factoryResetState.holdActive = false;
          factoryResetState.overlayActive = false;
          factoryResetState.restartScheduled = true;
          factoryResetState.restartAtMs = nowMs + kFactoryResetRestartDelayMs;
          countdown->active = true;
          countdown->secondsRemaining = 0;
          countdown->label = "Factory reset complete";
          buttonInput.clearEvents();
        } else {
          buttonInput.clearEvents();
        }
      }
    }
  }

  if (factoryResetState.restartScheduled) {
    countdown->active = true;
    countdown->secondsRemaining = 0;
    countdown->label = "Factory reset complete";
  }
}

ModbusMessage handleReadHolding(ModbusMessage request) {
  return modbusManager.handleReadHolding(request);
}

ModbusMessage handleWriteSingle(ModbusMessage request) {
  return modbusManager.handleWriteSingle(request);
}

ModbusMessage handleWriteMultiple(ModbusMessage request) {
  return modbusManager.handleWriteMultiple(request);
}

}  // namespace

//===================================================================
// TASK 1: Dedicated Flow Meter Polling (runs on Core 0)
//===================================================================
void pollingTaskCode(void * pvParameters) {
  byte lastPinStates = M5StamPLC.IO.getDigitalInput();
  uint32_t loopCounter = 0;
  unsigned long lastRateCalcTime = millis();

  for (;;) {
    byte currentPinStates = M5StamPLC.IO.getDigitalInput();
    for (std::size_t i = 0; i < kNumSensors; ++i) {
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
  ledController.loadFromPreferences(preferences);
  ledController.begin();
  uiController.begin(millis());
  buttonInput.begin(millis());
  factoryResetState = FactoryResetState{};
  enterIdleLatch = false;
  for (std::size_t i = 0; i < kNumSensors; ++i) {
    loadCumulativeData(static_cast<uint8_t>(i));
    sensors[i].inUse = false;
    sensors[i].isReady = false;
    modbusManager.syncSensorToHolding(i);
  }
  updateDiagnostics();
  modbusManager.syncGlobalRegisters();

  modbus.registerWorker(kDefaultModbusSlaveId, Modbus::READ_HOLD_REGISTER, handleReadHolding);
  modbus.registerWorker(kDefaultModbusSlaveId, Modbus::WRITE_HOLD_REGISTER, handleWriteSingle);
  modbus.registerWorker(kDefaultModbusSlaveId, Modbus::WRITE_MULT_REGISTERS, handleWriteMultiple);
  modbus.begin(RS485_SERIAL_PORT);

  for (;;) {
    unsigned long now = millis();
    buttonInput.update(now);

    UiCountdownState countdown{};
    handleFactoryReset(now, &countdown);
    if (!factoryResetState.holdActive && !factoryResetState.restartScheduled) {
      ButtonInputManager::ButtonEvent event;
      while (buttonInput.popEvent(&event)) {
        switch (event.button) {
          case ButtonInputManager::Button::Up:
            uiController.previousPage(now);
            break;
          case ButtonInputManager::Button::Down:
            uiController.nextPage(now);
            break;
          case ButtonInputManager::Button::Enter:
            if (!event.isLongPress) {
              uiController.notifyInteraction(now);
            }
            break;
        }
      }

      if (buttonInput.isPressed(ButtonInputManager::Button::Enter)) {
        const uint32_t heldMs = buttonInput.pressedDuration(ButtonInputManager::Button::Enter, now);
        if (heldMs >= kEnterIdleHoldMs && !enterIdleLatch) {
          uiController.enterIdle(now);
          enterIdleLatch = true;
        }
      } else {
        enterIdleLatch = false;
      }
    } else {
      enterIdleLatch = false;
      buttonInput.clearEvents();
    }

    if (now - lastCalcTime >= 1000) {
      float elapsedTime_s = (now - lastCalcTime) / 1000.0f;
      lastCalcTime = now;
      double totalSessionLiters = 0.0;
      double aggregateFlowLps = 0.0;
      bool allReady = true;
      std::size_t activeSensors = 0;
      for (std::size_t i = 0; i < kNumSensors; ++i) {
        if (sensors[i].inUse) {
          ++activeSensors;
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

          totalSessionLiters += sensors[i].sessionLiters;
          aggregateFlowLps += sensors[i].instantFlow_L_s;
          if (!sensors[i].isReady) {
            allReady = false;
          }
        } else {
          sensors[i].instantFlow_L_s = 0.0f;
          allReady = false;
        }
        modbusManager.syncSensorToHolding(i);
      }
      if (activeSensors == 0) {
        allReady = false;
      }
      totalSessionLitersCache = totalSessionLiters;
      aggregateFlowLpsCache = aggregateFlowLps;
      allSensorsReadyCache = allReady;
      updateDiagnostics();
      modbusManager.syncGlobalRegisters();
    }

    // Save cumulative data periodically to prevent excessive NVS writes
    if (now - lastSaveTime > 60000) { // Every minute
      for (std::size_t i = 0; i < kNumSensors; ++i) {
        if (sensors[i].inUse) saveCumulativeData(static_cast<uint8_t>(i));
      }
      lastSaveTime = now;
    }
    const bool suspendLeds = factoryResetState.holdActive || factoryResetState.restartScheduled;
    ledController.setSuspended(suspendLeds);
    ledController.update(now,
                         totalSessionLitersCache,
                         aggregateFlowLpsCache,
                         allSensorsReadyCache,
                         undersamplingFlags != 0);

    uiController.update(now,
                        sensors,
                        configs,
                        undersamplingFlags,
                        connectedSensorsBitmap,
                        totalSessionLitersCache,
                        aggregateFlowLpsCache,
                        ledController,
                        countdown);

    uiRenderer.update(now, uiController.context());

    if (factoryResetState.restartScheduled && now >= factoryResetState.restartAtMs) {
      esp_restart();
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
  uiRenderer.begin();

  xTaskCreatePinnedToCore(pollingTaskCode, "PollingTask", 4096, NULL, 2, &PollingTask, 0);
  xTaskCreatePinnedToCore(logicTaskCode, "LogicTask", 10000, NULL, 1, &LogicTask, 1);
}

void loop() {
  // Empty, all work is done in tasks.
  vTaskDelete(NULL); // Delete the default Arduino loop task
}
