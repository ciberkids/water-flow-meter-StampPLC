#include <M5StamPLC.h>
#include <Preferences.h>
#include <ModbusServerRTU.h>
#include <esp_system.h>
#include <esp_wifi.h>

#include <cstdint>
#include <cstdio>

// Core 0 belongs to pulse polling (Project_document §3.2). The WiFi task would land there by
// default, at priority 23 — above both the polling task and lwIP.
//
// platformio.ini defines CONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_1 to move it, and this assert is
// what proves the flag arrived: WIFI_TASK_CORE_ID is what WIFI_INIT_CONFIG_DEFAULT() writes into
// wifi_init_config_t::wifi_task_core_id, so if it reads 1 here, the task esp_wifi_init() creates
// is pinned to core 1.
//
// An assert rather than a comment because the alternative is a silent regression: delete the build
// flag and the firmware still compiles, still runs, still associates — and quietly starts stealing
// cycles from the measurement that is the reason this product exists. This project has been bitten
// repeatedly by checks that did not check; a build-time failure is the cheapest possible check.
static_assert(WIFI_TASK_CORE_ID == 1,
              "The WiFi task must not run on core 0, which is reserved for pulse polling. "
              "Restore -DCONFIG_ESP32_WIFI_TASK_PINNED_TO_CORE_1=1 in platformio.ini "
              "(see WiFi_MQTT_Connectivity.md §2.1.3).");

#include "input/button_input.h"
#include "input/interaction_handler.h"
#include "led/led_controller.h"
#include "modbus/modbus_manager.h"
#include "modbus/link_settings.h"
#include "modbus/link_settings_arduino.h"
#include "modbus/register_bank.h"
#include "modbus/register_map.h"
#include "modbus/sensor_types.h"
#include "net/net_settings.h"
#include "sensors/pulse_counter.h"
#include "sensors/sensor_state_engine.h"
#include "ui/core/ui_actions.h"
#include "ui/core/ui_bindings.h"
#include "ui/core/ui_module.h"
#include "ui/core/ui_screen_router.h"
#include "ui/core/ui_value_catalogue.h"
#include "ui/pack/ui_pack_storage_sd.h"

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

// ── Loadable menu packs (Loadable_UI_Menu_Packs.md) ──────────────────────────────
//
// The arbiter must be constructed before anything that can touch the shared SPI bus, and the
// storage adapter holds a reference to it for its lifetime.
plc::SpiArbiter spiArbiter;
/** SD chip select is 10; the LCD holds 12. 4 MHz is deliberately modest for a shared bus. */
plc::SdPackStorage packStorage(spiArbiter, 10, 4000000);
ui::PackLoader packLoader;
ui::MenuPack menuPack;
/**
 * The pack is read IN PLACE, so these bytes must outlive it. Heap rather than static: a device
 * with no card should not pay 64 KB of RAM for a feature it is not using, and this is freed
 * again when the load does not succeed.
 */
uint8_t* packBuffer = nullptr;
ui::LoadOutcome packOutcome = ui::LoadOutcome::BuiltInNoCard;
bool packRenderConfirmed = false;
ui::UiScreenRouter uiScreenRouter(kUiAssets);
ui::UiBindingResolver uiBindingResolver;
const ui::UiActionRegistry& kUiActionRegistry = ui::defaultActionRegistry();
UiRenderer uiRenderer;
UiController uiController;
ui::SettingsAccess uiSettingsAccess;

/**
 * WiFi, MQTT and portal configuration (WiFi_MQTT_Connectivity.md §6.1).
 *
 * Present from N1c so the fourteen settings have live storage the moment the menu can reach them.
 * The radio itself arrives in N4; until then these values are editable, persisted and readable over
 * RS485 but act on nothing — which is the correct order, because a setting whose storage does not
 * yet exist cannot be exercised by the pack-completeness rule.
 */
plc::NetSettings netSettings;

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

/**
 * The slave ID the eModbus workers are currently registered under.
 *
 * eModbus keys its worker map by server ID, so the registration made at boot is the only
 * ID the device answers on until something re-registers. This shadow is what lets
 * restartRs485() tell whether a rebind is needed at all.
 */
uint8_t registeredSlaveId = 0;

// Defined below; declared here so bindModbusWorkers can name them.
ModbusMessage handleReadHolding(ModbusMessage request);
ModbusMessage handleWriteSingle(ModbusMessage request);
ModbusMessage handleWriteMultiple(ModbusMessage request);

/** (Re)binds the three function-code workers to `slaveId`, if it has changed. */
void bindModbusWorkers(uint8_t slaveId) {
  if (slaveId == registeredSlaveId) {
    // Never touch eModbus' workerMap for a baud/parity-only change. The map is a plain
    // std::map with no mutex, read concurrently by the priority-8 server task, so the
    // cheapest mitigation for that race is not to mutate it unless we must.
    return;
  }
  // New ID first, old ID second: there is never an instant where neither is served.
  modbus.registerWorker(slaveId, Modbus::READ_HOLD_REGISTER, handleReadHolding);
  modbus.registerWorker(slaveId, Modbus::WRITE_HOLD_REGISTER, handleWriteSingle);
  modbus.registerWorker(slaveId, Modbus::WRITE_MULT_REGISTERS, handleWriteMultiple);
  if (registeredSlaveId != 0) {
    // functionCode 0 removes every worker for the ID (ModbusServer.h:43). Leaving the old
    // ID registered is the bug: frames on it would keep answering, and — before the
    // servedSlaveId test in LinkSettingsManager::noteValidFrame — would keep confirming an
    // apply the master never followed, so the 60 s rollback could never fire.
    modbus.unregisterWorker(registeredSlaveId);
  }
  registeredSlaveId = slaveId;
}

/**
 * Reopens the RS485 port with the live link settings, rebinding the workers if the slave
 * ID moved.
 *
 * The rebind lives here rather than at the call sites because both callers — the apply
 * path and the rollback path — already mean "make the hardware match linkSettings.live()",
 * and after rollback() that is the reverted value. One place, both directions.
 */
void restartRs485() {
  const LinkSettings& s = linkSettings.live();
  RS485_SERIAL_PORT.end();
  // Rebind while the port is closed. It does not make the unsynchronised workerMap access
  // safe, but it is the narrowest window available without forking eModbus: with the UART
  // down the server task has nothing to dispatch.
  bindModbusWorkers(s.slaveId);
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
// ── Reading the eight digital inputs ─────────────────────────────────────────────────
//
// ModbusManager::meetsNyquistLimit() budgets against (pollingRate_kHz * 1000 >= 2 * f_theoretical),
// so this function's cost sets the highest flow the device can measure. Measure the real rate on
// hardware before trusting the sensor configuration limits.
//
// THE COST IS I²C TRANSACTIONS, NOT CPU. At 400 kHz (aw9523.h:36) one readRegister8 is about 40 bit
// periods — roughly 100 µs on the wire, plus IDF driver overhead. The per-channel loop this replaced
// issued EIGHT of them per sample, because AW9523_Class::digitalRead() reads a whole 8-bit port
// register and then masks one bit (aw9523.cpp:120-122) — so it had already fetched the answer for up
// to eight channels and threw seven away. Two reads cover all sixteen pins.
//
// The inputs are AW9523 pins {4,5,6,7,12,13,14,15} (M5StamPLC.cpp:113), which straddle both ports:
// channels 0-3 are bits 4-7 of INPUT0, channels 4-7 are bits 4-7 of INPUT1. Both happen to sit in the
// high nibble, which makes the unpacking symmetric.
//
// A previous version of this comment said the bulk read needed the PI4IOE5V6408 expander. That was
// wrong: PI4IOE5V6408 drives the backlight and status LED, and the digital inputs are on the AW9523
// at 0x59 (`_io_expander_b`, M5StamPLC.cpp:118). The bulk read is reachable through m5::In_I2C
// directly, on the same bus and the same driver mutex, so no library change is needed.
constexpr uint8_t kAw9523Address = 0x59;
constexpr uint32_t kAw9523FreqHz = 400000;
constexpr uint8_t kAw9523InputPort0 = 0x00;
constexpr uint8_t kAw9523InputPort1 = 0x01;

/**
 * Whether the two-transaction path is trusted.
 *
 * Verified once at boot against the per-channel path (see verifyBulkInputRead). The pin map above is
 * hard-coded from the library's private `_in_pin_list`, so a library update that renumbers it would
 * silently transpose channels — and a transposed channel on a meter is wrong data attributed to the
 * wrong sensor, which is worse than no data. If the two paths disagree the device keeps the slower
 * path it can prove correct.
 */
bool bulkInputReadTrusted = false;

/** Two I²C transactions for all eight channels. */
uint8_t readDigitalInputBitmapBulk() {
  const uint8_t port0 = m5::In_I2C.readRegister8(kAw9523Address, kAw9523InputPort0, kAw9523FreqHz);
  const uint8_t port1 = m5::In_I2C.readRegister8(kAw9523Address, kAw9523InputPort1, kAw9523FreqHz);
  return static_cast<uint8_t>(((port0 >> 4) & 0x0Fu) | (((port1 >> 4) & 0x0Fu) << 4));
}

/** Eight I²C transactions, via the library's public per-channel accessor. Kept as the reference. */
uint8_t readDigitalInputBitmapPerChannel() {
  uint8_t bitmap = 0;
  for (uint8_t channel = 0; channel < kNumSensors; ++channel) {
    if (M5StamPLC.readPlcInput(channel)) {
      bitmap |= static_cast<uint8_t>(1u << channel);
    }
  }
  return bitmap;
}

uint8_t readDigitalInputBitmap() {
  return bulkInputReadTrusted ? readDigitalInputBitmapBulk() : readDigitalInputBitmapPerChannel();
}

/**
 * Proves the bulk unpacking against the library before trusting it.
 *
 * Reads both ways several times and requires every pair to agree. Several rather than once because a
 * single agreement is cheap to get by luck when every input happens to be low, which is the state an
 * unwired bench device is in — so the check would pass for the wrong reason exactly when it is least
 * informative. Disagreement is not treated as a fault to report: the slow path is correct, so the
 * device simply keeps it and says so on the console.
 */
void verifyBulkInputRead() {
  for (int attempt = 0; attempt < 4; ++attempt) {
    const uint8_t viaLibrary = readDigitalInputBitmapPerChannel();
    const uint8_t viaBulk = readDigitalInputBitmapBulk();
    if (viaLibrary != viaBulk) {
      Serial.printf(
          "[poll] bulk input read disagrees with the library (lib=0x%02X bulk=0x%02X); keeping the "
          "8-transaction path. Check M5StamPLC's _in_pin_list against firmware.cpp's pin map.\n",
          viaLibrary, viaBulk);
      bulkInputReadTrusted = false;
      return;
    }
  }
  bulkInputReadTrusted = true;
  Serial.println("[poll] bulk input read verified: 2 I2C transactions per sample instead of 8");
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
    const uint8_t currentPinStates = readDigitalInputBitmap();

    // The enabled mask is rebuilt from the connected-sensors bitmap rather than by testing
    // sensors[i].inUse once per channel per sample. Cheap either way, but it keeps the per-sample
    // cost independent of how many sensors are configured, so sample spacing does not change when
    // the operator enables one.
    const uint8_t enabledMask = plc::enabledMaskFromBitmap(connectedSensorsBitmap, kNumSensors);

    // `lastPinStates` tracks the FULL bitmap, including disabled channels — see the phantom-edge
    // note in pulse_counter.h. Masking it too would manufacture a pulse the first time a channel is
    // enabled while its input happens to be high.
    uint8_t rising = plc::risingEdges(lastPinStates, currentPinStates, enabledMask);
    plc::forEachRisingChannel(rising, [](std::size_t channel) { sensors[channel].pulseCount++; });

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

  // Same helper the apply/rollback paths use, so boot and reconfiguration cannot drift.
  bindModbusWorkers(linkSettings.live().slaveId);
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
    } else if (interactions.resetAccepted || interactions.restartScheduled) {
      // resetAccepted covers reset-totals and reset-session, which do not reboot. Gating this
      // on restartScheduled alone meant only the factory reset produced §3.5's solid-white
      // acceptance latch, so the two resets an operator actually uses gave no panel signal at
      // all. kResetAcceptedHoldMs is 2000 ms, matching the acknowledgement toast §3.5 ties it to.
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

    // §3.6 step 11 — clear the attempt counter only after a card pack has actually DRAWN.
    //
    // Clearing it after validation would prove nothing: validation is exactly what a pack that
    // crashes the renderer already passed. Waiting for a real frame is what makes the
    // anti-boot-loop guard meaningful, so a pack that validates and then takes the renderer down
    // is on its second strike next boot and its third boot runs the built-in default.
    //
    // Gated on renderer progress rather than on a timer: a frame that the arbiter skipped because
    // the card held the bus must not count as a successful render.
    if (packOutcome == ui::LoadOutcome::CardPack && !packRenderConfirmed &&
        uiController.context().currentScreen != nullptr && spiArbiter.mayBeginFrame()) {
      plc::NvsPackAttemptCounter packAttempts(preferences);
      packLoader.noteSuccessfulRender(packAttempts);
      packRenderConfirmed = true;
      Serial.println("[ui] menu pack rendered; boot-loop guard cleared");
    }

    // ── §3.4.1: the recovery gesture opens the firmware-drawn Select Menu page ──────
    if (interactions.openPackSelector && !uiController.selectorActive()) {
      // The directory is scanned fresh, not cached: the card may have been changed since the page
      // was last opened, and a stale list would offer a pack that is no longer there. This is the
      // one card access that happens with the display live, so it goes through the arbiter (§4.10)
      // and the LEDs report during the handover.
      static char packNames[ui::PackSelector::kMaxEntries][ui::PackLoader::kMaxNameBytes];
      std::size_t found = 0;
      char activeName[ui::PackLoader::kMaxNameBytes] = {};
      if (packStorage.mount()) {
        found = packStorage.listPacks(packNames, ui::PackSelector::kMaxEntries);
        if (!packStorage.readPointer(activeName, sizeof(activeName))) {
          activeName[0] = '\0';
        }
      }
      uiController.openPackSelector(packNames, found, activeName[0] ? activeName : nullptr, now);
    }

    // ── §3.5: a committed selection writes the pointer and reboots ─────────────────
    if (interactions.packSelectionCommitted) {
      const auto action = uiController.packSelector().commitAction();
      bool written = false;
      if (packStorage.mount()) {
        // Reboot rather than a hot swap: the active pack's buffer is referenced by the router, the
        // renderer and the interaction handler at once, and swapping it while they hold offsets
        // into it is a use-after-free waiting to happen (§3.5).
        written = action == ui::PackSelector::Commit::DeletePointer
                      ? packStorage.deletePointer()
                      : packStorage.writePointer(uiController.packSelector().commitName());
      }
      if (written) {
        Serial.println("[ui] menu selection saved; restarting");
        // The boot snake already signals "reloading" (§3.4 of the LED spec), so no extra
        // acknowledgement is needed for the second or so before the reset.
        delay(50);
        esp_restart();
      } else {
        Serial.println("[ui] could not save the menu selection; staying on the current menu");
        uiController.closePackSelector(now);
      }
    }

    // The LEDs are the only status channel while the card holds the shared bus (§4.10).
    if (spiArbiter.cardBusy()) {
      ledController.setCardBusy(now);
    } else {
      ledController.clearCardBusy();
    }

    // A committed link change reopens the port AND rebinds the eModbus workers, so a
    // slave-ID change takes effect without a reboot. It used to need one, which meant the
    // device kept answering on the old ID while the new one was already persisted.
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
  uiSettingsAccess.net = &netSettings;
  uiBindingResolver.bindSettings(&uiSettingsAccess, &uiController);

  // ── Load the selected menu pack, BEFORE the display is initialised ──────────────
  //
  // §4.5 and §4.10: no frame has ever been opened here, so the arbiter grants the bus
  // immediately and there is no contention to manage. This is the cheapest window there will
  // ever be, which is why the specification puts card access in it.
  {
    plc::NvsPackAttemptCounter packAttempts(preferences);
    packBuffer = static_cast<uint8_t*>(malloc(ui::PackLoader::kMaxPackBytes));
    if (!packBuffer) {
      // Out of heap is not a pack failure — nothing was attempted — so it must not burn an
      // attempt or delete a good selection.
      Serial.println("[ui] no heap for a menu pack; running the built-in default");
      packOutcome = ui::LoadOutcome::BuiltInNoCard;
    } else {
      packOutcome = packLoader.load(packStorage, packAttempts, ui::kUiCatalogueAbi, packBuffer,
                                    ui::PackLoader::kMaxPackBytes, &menuPack);
      if (packOutcome != ui::LoadOutcome::CardPack) {
        free(packBuffer);
        packBuffer = nullptr;
      }
    }

    // §4.9 — a silent fallback would leave the operator believing their pack loaded.
    if (packOutcome == ui::LoadOutcome::CardPack) {
      Serial.printf("[ui] menu pack \"%s\" loaded: %u screens\n", packLoader.selectedName(),
                    static_cast<unsigned>(menuPack.screenCount()));
    } else if (ui::loadOutcomeIsFailure(packOutcome)) {
      Serial.printf("[ui] menu pack NOT loaded (%s", ui::loadOutcomeText(packOutcome));
      if (packOutcome == ui::LoadOutcome::BuiltInInvalid) {
        Serial.printf(": %s", ui::packStatusText(packLoader.packStatus()));
      }
      Serial.println("); running the built-in default");
    } else {
      Serial.printf("[ui] built-in menu (%s)\n", ui::loadOutcomeText(packOutcome));
    }
  }

  uiRenderer.bindSpiArbiter(&spiArbiter);
  uiRenderer.bindScreenRouter(&uiScreenRouter);
  uiRenderer.bindBindingResolver(&uiBindingResolver);
  uiRenderer.applyTheme(kUiAssets.palette);
  uiRenderer.begin();

  // Before the polling task exists, so the verification has the bus to itself and cannot race the
  // sampler it is about to hand the bus to.
  verifyBulkInputRead();

  xTaskCreatePinnedToCore(pollingTaskCode, "PollingTask", 4096, NULL, 2, &PollingTask, 0);
  xTaskCreatePinnedToCore(logicTaskCode, "LogicTask", 10000, NULL, 1, &LogicTask, 1);
}

void loop() {
  // Empty, all work is done in tasks.
  vTaskDelete(NULL); // Delete the default Arduino loop task
}
