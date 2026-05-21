#include <M5StamPLC.h>
#include <Preferences.h>
#include <eModbus.h>
#include <esp_system.h>

#include <cstdint>
#include <cstdio>

#include "input/button_input.h"
#include "input/interaction_handler.h"
#include "led/led_controller.h"
#include "modbus/modbus_manager.h"
#include "modbus/register_bank.h"
#include "modbus/register_map.h"
#include "modbus/sensor_types.h"
#include "sensors/sensor_state_engine.h"
#include "ui/core/ui_actions.h"
#include "ui/core/ui_bindings.h"
#include "ui/core/ui_module.h"
#include "ui/core/ui_screen_router.h"

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
const ui::UiAssets kUiAssets = ui::loadGeneratedAssets();
ui::UiScreenRouter uiScreenRouter(kUiAssets);
ui::UiBindingResolver uiBindingResolver;
const ui::UiActionRegistry& kUiActionRegistry = ui::defaultActionRegistry();
UiRenderer uiRenderer;
UiController uiController;

SensorStateEngine::Dependencies sensorEngineDeps{
    .sensors = sensors,
    .configs = configs,
    .sensorCount = kNumSensors,
    .registerBank = &registerBank,
    .modbusManager = &modbusManager,
    .totalSessionLitersCache = &totalSessionLitersCache,
    .aggregateFlowLpsCache = &aggregateFlowLpsCache,
    .allSensorsReadyCache = &allSensorsReadyCache,
    .undersamplingFlags = &undersamplingFlags,
};
SensorStateEngine sensorStateEngine(sensorEngineDeps);
InteractionHandler interactionHandler;

namespace {

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
  sensorStateEngine.refreshDiagnostics();
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
  InteractionHandler::Dependencies interactionDeps{};
  interactionDeps.screenRouter = &uiScreenRouter;
  interactionDeps.actions = &kUiActionRegistry;
  interactionDeps.modbus = &modbusManager;
  interactionDeps.ledController = &ledController;
  interactionDeps.preferences = &preferences;
  interactionHandler.begin(millis(), performFactoryReset, interactionDeps);
  for (std::size_t i = 0; i < kNumSensors; ++i) {
    loadCumulativeData(static_cast<uint8_t>(i));
    sensors[i].inUse = false;
    sensors[i].isReady = false;
    modbusManager.syncSensorToHolding(i);
  }
  sensorStateEngine.refreshDiagnostics();
  modbusManager.syncGlobalRegisters();

  modbus.registerWorker(kDefaultModbusSlaveId, Modbus::READ_HOLD_REGISTER, handleReadHolding);
  modbus.registerWorker(kDefaultModbusSlaveId, Modbus::WRITE_HOLD_REGISTER, handleWriteSingle);
  modbus.registerWorker(kDefaultModbusSlaveId, Modbus::WRITE_MULT_REGISTERS, handleWriteMultiple);
  modbus.begin(RS485_SERIAL_PORT);

  for (;;) {
    unsigned long now = millis();
    buttonInput.update(now);

    const InteractionResult interactions =
        interactionHandler.update(now, buttonInput, uiController);

    if (now - lastCalcTime >= 1000) {
      const float elapsedTime_s = static_cast<float>(now - lastCalcTime) / 1000.0f;
      lastCalcTime = now;
      sensorStateEngine.update(elapsedTime_s);
    }

    // Save cumulative data periodically to prevent excessive NVS writes
    if (now - lastSaveTime > 60000) { // Every minute
      for (std::size_t i = 0; i < kNumSensors; ++i) {
        if (sensors[i].inUse) saveCumulativeData(static_cast<uint8_t>(i));
      }
      lastSaveTime = now;
    }
    const bool suspendLeds = interactions.ledsSuspended;
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
                        interactions.countdown);

    uiRenderer.update(now, uiController.context());

    if (interactions.restartScheduled && now >= interactions.restartAtMs) {
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
  uiRenderer.bindScreenRouter(&uiScreenRouter);
  uiRenderer.bindBindingResolver(&uiBindingResolver);
  uiRenderer.applyTheme(kUiAssets.palette);
  uiRenderer.begin();

  xTaskCreatePinnedToCore(pollingTaskCode, "PollingTask", 4096, NULL, 2, &PollingTask, 0);
  xTaskCreatePinnedToCore(logicTaskCode, "LogicTask", 10000, NULL, 1, &LogicTask, 1);
}

void loop() {
  // Empty, all work is done in tasks.
  vTaskDelete(NULL); // Delete the default Arduino loop task
}
