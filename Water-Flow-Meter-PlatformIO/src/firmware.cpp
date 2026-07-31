#include <M5StamPLC.h>
#include <Preferences.h>
#include <ModbusServerRTU.h>
#include <esp_system.h>

#include <cstdint>
#include <cstdio>

#include "input/button_input.h"
#include "input/interaction_handler.h"
#include "led/led_controller.h"
#include "modbus/modbus_manager.h"
#include "modbus/link_settings.h"
#include "modbus/link_settings_arduino.h"
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
//
// RS485 pin assignment, verified against three independent sources that all agree:
//   - the vendor spec  https://docs.m5stack.com/en/core/StamPLC
//   - docs/hardware docs/StampPLC pin map.md  (PWR-485 row: G0 / G39 / G46)
//   - M5StamPLC's own pin_config.h: STAMPLC_PIN_485_TX/RX/DIR = 0 / 39 / 46
//
// These were previously 17 / 16 / 2, which are not connected to the RS485
// transceiver at all — GPIO2 is STAMPLC_PIN_GROVE_RED_SDA, a Grove I2C data line.
// Modbus RTU could not have worked on real hardware.
//
// GPIO0 doubles as the BOOT pin; that is how the vendor wires it, so it is
// expected rather than a mistake.
//
// UART choice: the ESP32-S3 GPIO matrix lets any UART drive any pin, so Serial2 is
// kept deliberately. M5StamPLC's optional built-in Modbus slave claims UART1 and
// calls Serial1.end(); staying off UART1 means enabling that feature could never
// tear our port down. That built-in slave must also stay disabled (it defaults to
// false) because it serves its own register map, not the one in
// docs/Requirements/Project_document.md §4.
constexpr HardwareSerial& RS485_SERIAL_PORT = Serial2;
constexpr int RS485_TX_PIN = 0;
constexpr int RS485_RX_PIN = 39;
constexpr int RS485_DE_PIN = 46;  // Direction Enable pin for RS485 transceiver

// --- Global State ---
SensorData sensors[kNumSensors];
SensorCharacteristics configs[kNumSensors];
Preferences preferences;
ModbusServerRTU modbus(2000, RS485_DE_PIN);
volatile float pollingRate_kHz = 0.0f;
TaskHandle_t PollingTask;
TaskHandle_t LogicTask;
RegisterBank registerBank;
LinkSettingsManager linkSettings;
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
                              .link = &linkSettings,
                              .sensorCount = kNumSensors};

ModbusManager modbusManager(modbusDeps);
const ui::UiAssets kUiAssets = ui::loadGeneratedAssets();
ui::UiScreenRouter uiScreenRouter(kUiAssets);
ui::UiBindingResolver uiBindingResolver;
const ui::UiActionRegistry& kUiActionRegistry = ui::defaultActionRegistry();
UiRenderer uiRenderer;
UiController uiController;
ui::SettingsAccess uiSettingsAccess;

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

constexpr const char* kPrefLinkSlaveId = "lnk_id";
constexpr const char* kPrefLinkBaud = "lnk_baud";
constexpr const char* kPrefLinkParity = "lnk_par";
constexpr const char* kPrefLinkStop = "lnk_stop";

/**
 * Sensor calibration and the enable bitmap were RAM-only, so a power cycle silently
 * discarded every sensor's Q/F/Adjust and disabled all eight channels — while the
 * cumulative totals survived, leaving totals for sensors that were no longer "in use".
 * Display_UI_Requirements §5.5 requires configuration to persist across reboots.
 */
void saveSensorConfig(std::size_t index) {
  if (index >= kNumSensors) {
    return;
  }
  char key[10];
  std::snprintf(key, sizeof(key), "cfg_q%u", static_cast<unsigned>(index));
  preferences.putUShort(key, configs[index].q_max);
  std::snprintf(key, sizeof(key), "cfg_f%u", static_cast<unsigned>(index));
  preferences.putShort(key, configs[index].f_multiplier);
  std::snprintf(key, sizeof(key), "cfg_a%u", static_cast<unsigned>(index));
  preferences.putShort(key, configs[index].adjust);
}

void loadSensorConfig(std::size_t index) {
  if (index >= kNumSensors) {
    return;
  }
  char key[10];
  std::snprintf(key, sizeof(key), "cfg_q%u", static_cast<unsigned>(index));
  configs[index].q_max = preferences.getUShort(key, 0);
  std::snprintf(key, sizeof(key), "cfg_f%u", static_cast<unsigned>(index));
  configs[index].f_multiplier = preferences.getShort(key, 0);
  std::snprintf(key, sizeof(key), "cfg_a%u", static_cast<unsigned>(index));
  configs[index].adjust = preferences.getShort(key, 0);
}

constexpr const char* kPrefConnectedBitmap = "conn_map";
/** §3.5: solid white is held for the acknowledgement toast duration (§4.3.1). */
constexpr uint32_t kResetAcceptedHoldMs = 2000;

LinkSettings loadLinkSettings() {
  LinkSettings s;
  s.slaveId = preferences.getUChar(kPrefLinkSlaveId, s.slaveId);
  s.baudIndex = preferences.getUChar(kPrefLinkBaud, s.baudIndex);
  s.parity = preferences.getUChar(kPrefLinkParity, s.parity);
  s.stopBits = preferences.getUChar(kPrefLinkStop, s.stopBits);
  return s.valid() ? s : LinkSettings{};
}

void saveLinkSettings(const LinkSettings& s) {
  preferences.putUChar(kPrefLinkSlaveId, s.slaveId);
  preferences.putUChar(kPrefLinkBaud, s.baudIndex);
  preferences.putUChar(kPrefLinkParity, s.parity);
  preferences.putUChar(kPrefLinkStop, s.stopBits);
}

/** Reopens the RS485 port with the live link settings. */
void restartRs485() {
  const LinkSettings& s = linkSettings.live();
  RS485_SERIAL_PORT.end();
  RS485_SERIAL_PORT.begin(static_cast<unsigned long>(s.baudRate()),
                          arduinoSerialConfig(s),
                          RS485_RX_PIN,
                          RS485_TX_PIN);
}

void performFactoryReset() {
  modbusManager.applyHoldingWrite(REG_MASTER_RESET_ALL_SENSORS, 1);
  modbusManager.applyHoldingWrite(REG_CONNECTED_SENSORS_BITMAP, 0);

  preferences.clear();
  linkSettings.begin(LinkSettings{});
  saveLinkSettings(linkSettings.live());
  preferences.putUShort(kPrefConnectedBitmap, 0);
  for (std::size_t i = 0; i < kNumSensors; ++i) {
    configs[i] = SensorCharacteristics{};
    saveSensorConfig(i);
  }
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

namespace {

// M5StamPLC 1.2.0 removed the bulk `IO.getDigitalInput()` accessor; the only
// public input API is per-channel readPlcInput(ch), which performs one I2C
// expander read each. That makes a full 8-channel sample eight I2C
// transactions instead of one.
//
// PERFORMANCE WARNING: this lowers the achievable pollingRate_kHz roughly
// 8-fold, and pollingRate_kHz is what the Nyquist check in
// ModbusManager::meetsNyquistLimit() budgets against
// (pollingRate_kHz * 1000 >= 2 * f_theoretical). Measure the real rate on
// hardware before trusting the sensor configuration limits. Recovering the
// single-transaction read needs a bulk register read on the PI4IOE5V6408
// expander, which M5StamPLC keeps private.
uint8_t readDigitalInputBitmap() {
  uint8_t bitmap = 0;
  for (uint8_t channel = 0; channel < kNumSensors; ++channel) {
    if (M5StamPLC.readPlcInput(channel)) {
      bitmap |= static_cast<uint8_t>(1u << channel);
    }
  }
  return bitmap;
}

}  // namespace

//===================================================================
// TASK 1: Dedicated Flow Meter Polling (runs on Core 0)
//===================================================================
void pollingTaskCode(void * pvParameters) {
  uint8_t lastPinStates = readDigitalInputBitmap();
  uint32_t loopCounter = 0;
  unsigned long lastRateCalcTime = millis();

  for (;;) {
    uint8_t currentPinStates = readDigitalInputBitmap();
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
  // Shadow copies so only genuine changes reach NVS.
  double persistedCumulative[kNumSensors] = {};
  SensorCharacteristics persistedConfigs[kNumSensors] = {};
  uint16_t persistedBitmap = 0;

  // Project_document.md §4.1.1: link settings must load from NVS BEFORE the RS485
  // port is opened. This used to call preferences.begin() *after* Serial.begin()
  // with 9600/8N1 hardcoded, so a configured baud rate could never take effect on
  // the first open.
  preferences.begin("flow-data", false);
  linkSettings.begin(loadLinkSettings());

  RTUutils::prepareHardwareSerial(RS485_SERIAL_PORT);
  {
    const LinkSettings& link = linkSettings.live();
    RS485_SERIAL_PORT.begin(static_cast<unsigned long>(link.baudRate()),
                            arduinoSerialConfig(link),
                            RS485_RX_PIN,
                            RS485_TX_PIN);
  }
  ledController.loadFromPreferences(preferences);
  ledController.begin();
  ledController.beginBoot(millis());
  uiController.begin(millis());
  // Seed the navigator with the root screen (P0). Everything else follows the
  // dataset's own flows from here.
  uiController.navigator().reset(
      uiScreenRouter.screenForMode(UiMode::Info, UiPage::GlobalStatus));
  buttonInput.begin(millis());
  InteractionHandler::Dependencies interactionDeps{};
  interactionDeps.screenRouter = &uiScreenRouter;
  interactionDeps.actions = &kUiActionRegistry;
  interactionDeps.modbus = &modbusManager;
  interactionDeps.ledController = &ledController;
  interactionDeps.preferences = &preferences;
  interactionDeps.settings = &uiSettingsAccess;
  interactionHandler.begin(millis(), performFactoryReset, interactionDeps);
  // Restore calibration and which channels were enabled, then let the state engine
  // decide readiness from the restored config rather than forcing everything off.
  connectedSensorsBitmap = preferences.getUShort(kPrefConnectedBitmap, 0);
  for (std::size_t i = 0; i < kNumSensors; ++i) {
    loadCumulativeData(static_cast<uint8_t>(i));
    loadSensorConfig(i);
    sensors[i].inUse = (connectedSensorsBitmap >> i) & 0x01;
    sensors[i].isReady = false;
    modbusManager.syncSensorToHolding(i);
  }
  sensorStateEngine.refreshDiagnostics();
  modbusManager.syncGlobalRegisters();
  for (std::size_t i = 0; i < kNumSensors; ++i) {
    persistedCumulative[i] = sensors[i].cumulativeLiters;
    persistedConfigs[i] = configs[i];
  }
  persistedBitmap = connectedSensorsBitmap;

  const uint8_t slaveId = linkSettings.live().slaveId;
  modbus.registerWorker(slaveId, Modbus::READ_HOLD_REGISTER, handleReadHolding);
  modbus.registerWorker(slaveId, Modbus::WRITE_HOLD_REGISTER, handleWriteSingle);
  modbus.registerWorker(slaveId, Modbus::WRITE_MULT_REGISTERS, handleWriteMultiple);
  // Pin the eModbus server task to core 1, alongside this logic task.
  //
  // Without the explicit coreID, ModbusServerRTU::doBegin() creates its task with
  // tskNO_AFFINITY at priority 8 — free to be scheduled on core 0, where it would
  // preempt the priority-2 polling task. That contradicts Project_document §3.2's
  // guarantee that "pulse counting is never delayed or interrupted by other
  // application logic, such as Modbus communication delays".
  //
  // Priority 8 also means Modbus still preempts this task (priority 1) on core 1,
  // so a slow UI redraw cannot delay a Modbus response.
  constexpr int kModbusCoreId = 1;
  modbus.begin(RS485_SERIAL_PORT, kModbusCoreId);

  for (;;) {
    unsigned long now = millis();
    buttonInput.update(now);

    const InteractionResult interactions =
        interactionHandler.update(now, buttonInput, uiController);

    if (now - lastCalcTime >= 1000) {
      // §3.4: the boot pattern ends the moment core 0 reports a polling rate, which is
      // the first instant the device can actually count pulses.
      if (pollingRate_kHz > 0.0f) {
        ledController.noteReady();
      }
      const float elapsedTime_s = static_cast<float>(now - lastCalcTime) / 1000.0f;
      lastCalcTime = now;
      sensorStateEngine.update(elapsedTime_s);
    }

    // Persist periodically rather than on every change, to spare NVS. Only values
    // that actually moved are written: Preferences::put* does not guarantee it skips
    // an identical write, and at one pass per minute an unconditional write would be
    // roughly 525k writes per key per year.
    if (now - lastSaveTime > 60000) { // Every minute
      for (std::size_t i = 0; i < kNumSensors; ++i) {
        if (!sensors[i].inUse) {
          continue;
        }
        if (sensors[i].cumulativeLiters != persistedCumulative[i]) {
          saveCumulativeData(static_cast<uint8_t>(i));
          persistedCumulative[i] = sensors[i].cumulativeLiters;
        }
        if (configs[i] != persistedConfigs[i]) {
          saveSensorConfig(i);
          persistedConfigs[i] = configs[i];
        }
      }
      if (connectedSensorsBitmap != persistedBitmap) {
        preferences.putUShort(kPrefConnectedBitmap, connectedSensorsBitmap);
        persistedBitmap = connectedSensorsBitmap;
      }
      lastSaveTime = now;
    }
    // §3.5: the countdown drives the LED ramp; acceptance latches solid white for the
    // acknowledgement toast. Releasing early clears the ramp with no white flash, so an
    // aborted reset can never look like a completed one.
    if (interactions.countdown.active && interactions.countdown.totalMs > 0) {
      ledController.setResetRamp(interactions.countdown.remainingMs,
                                 interactions.countdown.totalMs);
    } else if (interactions.restartScheduled) {
      ledController.noteResetAccepted(now, kResetAcceptedHoldMs);
    } else {
      ledController.clearResetRamp();
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
                        pollingRate_kHz,
                        ledController,
                        interactions.countdown);

    uiRenderer.update(now, uiController.context());

    // A committed link change reopens the port. A slave-ID change additionally needs
    // a reboot, because eModbus binds workers to the ID at registration time.
    if (modbusManager.consumeLinkRestartRequest()) {
      saveLinkSettings(linkSettings.live());
      restartRs485();
      modbusManager.syncGlobalRegisters();
    }

    // §4.1.1 rollback: an apply that is never confirmed by a valid frame is assumed
    // to have broken the link, and the previous settings are restored.
    if (linkSettings.rollbackDue(now)) {
      if (linkSettings.rollback()) {
        saveLinkSettings(linkSettings.live());
        restartRs485();
        modbusManager.syncGlobalRegisters();
      }
    }

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
  // A screen ID the router expects but the exported dataset does not define
  // renders as a blank page with no other symptom, so say so at boot. The
  // exporter's manifest-screen-coverage check is the build-time counterpart.
  if (!uiScreenRouter.isFullyResolved()) {
    Serial.println(
        "[ui] ERROR: generated UI assets are missing screens the router requires. "
        "Re-run the UI exporter (npm run export:firmware).");
  }
  uiSettingsAccess.link = &linkSettings;
  uiSettingsAccess.leds = &ledController;
  uiSettingsAccess.modbus = &modbusManager;
  uiSettingsAccess.configs = configs;
  uiSettingsAccess.connectedBitmap = &connectedSensorsBitmap;
  uiSettingsAccess.sensorCount = kNumSensors;
  uiBindingResolver.bindSettings(&uiSettingsAccess, &uiController);

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
