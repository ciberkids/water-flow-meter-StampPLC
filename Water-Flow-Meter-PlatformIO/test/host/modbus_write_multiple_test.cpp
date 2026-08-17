/**
 * FC16 (write multiple registers) against the REAL frame handler.
 *
 * §5.1 of WiFi_MQTT_Connectivity.md requires a block write across the whole network region to succeed
 * rather than except — a master zero-filling the whole region must not except on its way past, and must not
 * silently reset the login either. `handleWriteMultiple` pre-validated every word with
 * `isWritableAddress`, a predicate that knows only the sensor and link registers, so every address from
 * 500 up answered false and the frame excepted with ILLEGAL_DATA_ADDRESS on its FIRST word. Writing an
 * SSID over FC16 was impossible while FC6 at the same address worked.
 *
 * ── WHY A NEW BINARY, AND WHY THE OLD ASSURANCE WAS FALSE ────────────────────────────────
 *
 * net_settings_test.cpp has a green case named for this exact requirement ("a block write across
 * read-only registers must not fail — §5.1"). It calls `NetRegisterMap::stageWrite` directly, one layer
 * BELOW the handler, so it passed throughout. Nothing in test/ called handleWriteMultiple at all.
 *
 * `deps_.net` is the load-bearing detail. modbus_manager_clock_test.cpp leaves it null, which makes the
 * entire network branch of applyHoldingWrite unreachable — so a test written there would pass against
 * the broken and the fixed code alike. Every Device here populates it.
 *
 * The requests come from `ModbusMessage::withFields`, which carries the values a request answers at the
 * indices the handler reads rather than a parsed RTU frame. What is under test is the validation and
 * dispatch loop; the framing is eModbus's and is not reimplemented. See the stub's own header.
 */
#include <Preferences.h>

#include <cstdio>
#include <algorithm>
#include <cstring>
#include <string>
#include <vector>

#include "led/led_controller.h"
#include "modbus/modbus_manager.h"
#include "modbus/register_bank.h"
#include "modbus/register_map.h"
#include "modbus/sensor_types.h"
#include "net/net_register_map.h"
#include "net/net_settings.h"
#include "time/device_clock.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-78s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) failures++;
}

/** Function codes, spelled so the frames read as what they are. */
constexpr uint8_t kFc06 = 0x06;
constexpr uint8_t kFc16 = 0x10;
constexpr uint8_t kSlave = 1;

/**
 * Everything ModbusManager reaches through, with a REAL NetSettings attached.
 *
 * Mirrors modbus_manager_clock_test.cpp's Device, plus `net`. Every field is populated because
 * isWritableAddress dereferences `registers` on its first line and syncGlobalRegisters dereferences
 * four more unconditionally.
 */
struct Device {
  plc::RegisterBank registers;
  SensorData sensors[plc::kNumSensors]{};
  SensorCharacteristics configs[plc::kNumSensors]{};
  Preferences preferences;
  LedController leds;
  plc::DeviceClock clock;
  plc::NetSettings net;
  uint16_t connectedBitmap = 0x01;
  uint16_t undersamplingFlags = 0;
  double totalSessionLiters = 0.0;
  double aggregateFlowLpm = 0.0;
  uint16_t displayFlowUnit = 0;
  bool allSensorsReady = false;
  volatile float pollingRateKhz = 3.3f;

  ModbusDependencies deps() {
    ModbusDependencies d;
    d.sensors = sensors;
    d.configs = configs;
    d.preferences = &preferences;
    d.registers = &registers;
    d.ledController = &leds;
    d.clock = &clock;
    d.net = &net;  // THE point of this file: null here makes the whole network branch unreachable
    d.connectedBitmap = &connectedBitmap;
    d.undersamplingFlags = &undersamplingFlags;
    d.totalSessionLitersCache = &totalSessionLiters;
    d.aggregateFlowLpmCache = &aggregateFlowLpm;
    d.displayFlowUnit = &displayFlowUnit;
    d.allSensorsReadyCache = &allSensorsReady;
    d.pollingRateKhz = &pollingRateKhz;
    d.sensorCount = plc::kNumSensors;
    return d;
  }

  void armChannelOne() {
    sensors[0].inUse = true;
    configs[0].q_max = 100;
    configs[0].f_multiplier = 6;
  }
};

/** Big-endian register pairs, the way the wire carries them. */
std::vector<uint8_t> words(const std::vector<uint16_t>& values) {
  std::vector<uint8_t> out;
  for (const uint16_t v : values) {
    out.push_back(static_cast<uint8_t>(v >> 8));
    out.push_back(static_cast<uint8_t>(v & 0xFF));
  }
  return out;
}

/** Text as the block packs it: two characters per register, high byte first. */
std::vector<uint16_t> packText(const std::string& text, std::size_t registerCount) {
  std::vector<uint16_t> out;
  for (std::size_t r = 0; r < registerCount; ++r) {
    const char high = 2 * r < text.size() ? text[2 * r] : '\0';
    const char low = 2 * r + 1 < text.size() ? text[2 * r + 1] : '\0';
    out.push_back(plc::NetRegisterMap::packChars(high, low));
  }
  return out;
}

ModbusMessage writeMultiple(ModbusManager& modbus, uint16_t address,
                            const std::vector<uint16_t>& values) {
  const std::vector<uint8_t> payload = words(values);
  return modbus.handleWriteMultiple(ModbusMessage::withFields(
      kSlave, kFc16, address, static_cast<uint16_t>(values.size()),
      static_cast<uint8_t>(payload.size()), payload));
}

std::string liveField(const plc::NetSettings& net, plc::NetField field) {
  char buffer[plc::NetSettings::kMaxValueBytes + 1] = {};
  net.get(field, buffer, sizeof(buffer));
  return std::string(buffer);
}

std::string stagedField(const plc::NetSettings& net, plc::NetField field) {
  char buffer[plc::NetSettings::kMaxValueBytes + 1] = {};
  net.getStaged(field, buffer, sizeof(buffer));
  return std::string(buffer);
}

/* ── The defect ──────────────────────────────────────────────────────────────────────────── */

void ssidOverBlockWrite() {
  std::printf("\n[an SSID written over FC16 — the defect]\n");

  Device dev;
  ModbusManager modbus(dev.deps());

  // Four registers at kWifiSsid: "SSID-abc" is eight characters.
  const ModbusMessage response = writeMultiple(modbus, plc::net_reg::kWifiSsid, packText("SSID-abc", 4));

  check(!response.errorSet, "the frame is accepted — it used to except with ILLEGAL_DATA_ADDRESS");
  check(response.addCalled, "and a normal response is assembled rather than an error one");
  check(stagedField(dev.net, plc::NetField::WifiSsid) == std::string("SSID-abc"),
        "the SSID really reached the staging store, so this is not a silent success");
  check(liveField(dev.net, plc::NetField::WifiSsid).empty(),
        "and it is only STAGED — a block write does not go live without an apply");
}

void theAsymmetryThatGaveItAway() {
  std::printf("\n[FC6 and FC16 must agree about what is writable]\n");

  Device dev;
  dev.armChannelOne();
  ModbusManager modbus(dev.deps());

  // The control: the same handler, the same frame shape, a SENSOR register. This always worked, so a
  // failure here would mean the harness is wrong rather than the firmware.
  const uint16_t qMaxAddress = plc::sensorBaseAddress(0) + plc::OFF_CFG_Q_MAX;
  const ModbusMessage sensorResponse = writeMultiple(modbus, qMaxAddress, {250});
  check(!sensorResponse.errorSet, "a sensor q_max over FC16 is accepted (the control)");
  check(dev.configs[0].q_max == 250, "and lands");

  // Every text field's first register, and the four scalars: all reachable by FC6, so all must be
  // reachable by FC16. This is the comparison that exposed the defect.
  const uint16_t addresses[] = {
      plc::net_reg::kWifiEnabled, plc::net_reg::kWifiSsid,  plc::net_reg::kWifiPsk,
      plc::net_reg::kMqttEnabled, plc::net_reg::kMqttPort,  plc::net_reg::kMqttHost,
      plc::net_reg::kMqttUser,    plc::net_reg::kMqttBaseTopic, plc::net_reg::kMqttPrefix,
      plc::net_reg::kPortalUser,  plc::net_reg::kPortalPassword};
  bool everyOneAccepted = true;
  for (const uint16_t at : addresses) {
    Device fresh;
    ModbusManager m(fresh.deps());
    if (writeMultiple(m, at, {0x4142}).errorSet) {
      everyOneAccepted = false;
      std::printf("      FC16 refused %u\n", static_cast<unsigned>(at));
    }
  }
  check(everyOneAccepted, "FC16 accepts every writable address in the network block");
}

void blockWriteAcrossTheWholeRegion() {
  std::printf("\n[§5.1 — zero-filling the whole region must succeed, and must not reset the login]\n");

  Device dev;
  ModbusManager modbus(dev.deps());

  // A login the master is about to write straight past.
  check(dev.net.stage(plc::NetField::PortalUser, "operator"), "an operator login is staged");
  check(dev.net.stage(plc::NetField::PortalPassword, "s3cret"), "with a password");
  check(dev.net.apply(), "and applied, so it is the live login");
  const uint16_t revisionAfterLogin = dev.net.revision();

  /**
   * Zeros across the whole block — but NOT in one frame, because the protocol forbids it.
   *
   * FC16 carries its byte count in a single byte, so the spec caps one frame at 123 registers and the
   * region does not fit in one. A master zero-filling the region necessarily issues a SEQUENCE of frames,
   * which is why the apply deferral has to hold per frame rather than per region: the frame carrying
   * 730 also carries 731 and 732 behind it.
   *
   * net_settings_test.cpp's case for this requirement writes every address in one loop through
   * `stageWrite`, which is not a frame and cannot see this — the layer difference that let the defect
   * live.
   */
  constexpr std::size_t kMaxRegistersPerFrame = 123;
  const std::size_t span = plc::net_reg::kEnd - plc::net_reg::kBase;
  bool everyFrameAccepted = true;
  bool everyFrameResponded = true;
  std::size_t frames = 0;
  for (std::size_t offset = 0; offset < span; offset += kMaxRegistersPerFrame) {
    const std::size_t count = std::min(kMaxRegistersPerFrame, span - offset);
    const ModbusMessage response = writeMultiple(
        modbus, static_cast<uint16_t>(plc::net_reg::kBase + offset), std::vector<uint16_t>(count, 0));
    ++frames;
    if (response.errorSet) everyFrameAccepted = false;
    if (!response.addCalled) everyFrameResponded = false;
  }

  const std::size_t expectedFrames = (span + kMaxRegistersPerFrame - 1) / kMaxRegistersPerFrame;
  check(span > kMaxRegistersPerFrame,
        "the region is larger than one frame can carry, so a master must split it");
  check(frames == expectedFrames, "and it was written in exactly that many frames");
  check(everyFrameAccepted, "the whole-region zero-fill is accepted rather than excepting");
  check(everyFrameResponded, "with a normal response to each frame");
  check(liveField(dev.net, plc::NetField::PortalUser) == std::string("operator"),
        "and the live login survived the write that passed over it");
  check(dev.net.revision() == revisionAfterLogin,
        "the zero at NET_APPLY did NOT commit — it is not the magic, so nothing applied");
  check(dev.registers.at(plc::net_reg::kLastError) ==
            static_cast<uint16_t>(plc::NetApplyError::BadMagic),
        "the refusal is reported at NET_LAST_ERROR instead, which is where a master looks");
}

void applyIsDeferredToTheEndOfTheBlock() {
  std::printf("\n[NET_APPLY at 730 has two registers after it, so it must commit LAST]\n");

  Device dev;
  ModbusManager modbus(dev.deps());

  // One frame that stages a portal user AND carries the apply magic, with kRevision/kLastError after
  // it. Applying in address order would commit before the words past 730 had staged.
  std::vector<uint16_t> frame = packText("admin2", 4);        // 712..715, the portal user
  frame.resize(plc::net_reg::kApply - plc::net_reg::kPortalUser, 0);
  frame.push_back(plc::net_reg::kApplyMagic);                 // 730
  frame.push_back(0);                                         // 731, read-only
  frame.push_back(0);                                         // 732, read-only

  const ModbusMessage response = writeMultiple(modbus, plc::net_reg::kPortalUser, frame);

  check(!response.errorSet, "a frame that spans the apply register is accepted");
  check(dev.net.revision() == 1, "the magic committed the block");
  check(liveField(dev.net, plc::NetField::PortalUser) == std::string("admin2"),
        "and what went live is the value staged EARLIER in the same frame");
}

void thePortalPasswordIsWritableToItsFullCapacity() {
  std::printf("\n[the portal password: all 32 bytes reachable over RS485, not 20]\n");

  Device dev;
  ModbusManager modbus(dev.deps());

  // A 32-byte password — the capacity netFieldCapacity() declares and the web portal accepts. The
  // field's 16 registers used to start at 720 and collide with kApply at 730, so only bytes 0..19 could
  // be written from the bus: a credential settable from the web form and not over RS485.
  const std::string password = "0123456789abcdefghijABCDEFGHIJ!?";
  check(password.size() == 32, "the case really is 32 bytes long");
  check(plc::netFieldCapacity(plc::NetField::PortalPassword) == 32,
        "and 32 is what the field's capacity says it holds");

  const ModbusMessage response =
      writeMultiple(modbus, plc::net_reg::kPortalPassword, packText(password, 16));
  check(!response.errorSet, "16 registers of password are accepted in one frame");
  check(stagedField(dev.net, plc::NetField::PortalPassword) == password,
        "and ALL 32 bytes staged — the last 12 used to be unreachable");

  check(modbus.applyHoldingWrite(plc::net_reg::kApply, plc::net_reg::kApplyMagic),
        "the block applies");
  check(liveField(dev.net, plc::NetField::PortalPassword) == password,
        "so the full-length password is the live one");
  check(dev.net.revision() == 1, "with one apply, not one caused by a password byte landing on 730");

  // The vacated window must do nothing at all rather than land in another field.
  Device fresh;
  ModbusManager m(fresh.deps());
  const ModbusMessage intoTheHole =
      writeMultiple(m, plc::net_reg::kPortalPasswordReserved, std::vector<uint16_t>(10, 0x4142));
  check(!intoTheHole.errorSet,
        "a master written against the old address is not refused — §5.1 ignores, it does not except");
  // Compared against the DEFAULT rather than against empty: the portal login ships as admin/admin, so
  // "nothing happened" here means "still the default", not "blank".
  check(stagedField(fresh.net, plc::NetField::PortalPassword) ==
            std::string(plc::NetSettings::kDefaultPortalPassword),
        "but nothing stages: the reserved window is not quietly part of some other field");
  check(stagedField(fresh.net, plc::NetField::PortalUser) ==
            std::string(plc::NetSettings::kDefaultPortalUser),
        "and the field BELOW it, which really does end at 719, is untouched too");
  check(fresh.net.revision() == 0, "and nothing applied");
}

void theApplyMustCommitAFTERTheRegistersBEYONDIt() {
  std::printf("\n[a frame starting AT the apply and reaching past it — the ordering case]\n");

  /**
   * THIS IS THE CASE THE DEFERRAL EXISTS FOR, and it only became reachable when kPortalPassword moved to
   * 736-751. Before that nothing writable sat above 730, so applying in address order committed exactly
   * as much as deferring did and the deferral was pure precaution. It is now load-bearing.
   *
   * One frame, 22 registers, starting at the apply itself:
   *
   *   730        the apply magic
   *   731, 732   read-only, ignored (§5.1)
   *   733-735    free, ignored
   *   736-751    the portal password, 16 registers
   *
   * In address order the magic commits an EMPTY stage, and the password then stages behind a revision
   * that already claimed to have applied it — so the master's write is silently not in force.
   */
  Device dev;
  ModbusManager modbus(dev.deps());

  const std::string password = "committed-after-the-apply-word!!";
  check(password.size() == 32, "a full-length password, so the frame reaches 751");

  std::vector<uint16_t> frame;
  frame.push_back(plc::net_reg::kApplyMagic);                       // 730
  while (frame.size() < plc::net_reg::kPortalPassword - plc::net_reg::kApply) {
    frame.push_back(0);                                             // 731, 732, then 733-735
  }
  for (const uint16_t word : packText(password, 16)) {
    frame.push_back(word);                                          // 736-751
  }
  check(frame.size() == 22, "22 registers: the apply, the two read-only words, the gap, the password");
  check(plc::net_reg::kApply + frame.size() == plc::net_reg::kEnd,
        "and the frame ends exactly at the end of the block");

  const ModbusMessage response = writeMultiple(modbus, plc::net_reg::kApply, frame);
  check(!response.errorSet, "the frame is accepted");
  check(dev.net.revision() == 1, "one apply happened");
  check(liveField(dev.net, plc::NetField::PortalPassword) == password,
        "and the password is LIVE — it staged before the commit, though it is addressed after it");
  check(stagedField(dev.net, plc::NetField::PortalPassword) == password,
        "with staged and live agreeing, so nothing is left pending");
}

void refusalsThatMustStay() {
  std::printf("\n[what FC16 must still refuse]\n");

  Device dev;
  ModbusManager modbus(dev.deps());

  // Past the end of the block. kEnd and the bank are both 752, so 751 is the last servable address.
  check(writeMultiple(modbus, static_cast<uint16_t>(plc::net_reg::kEnd - 1), {0, 0}).errorSet,
        "a frame running past the end of the register space is refused");
  check(writeMultiple(modbus, plc::net_reg::kWifiSsid, {}).errorSet,
        "a zero-word frame is refused");

  // A channel that is not in use: its config registers are not writable, and that is unchanged.
  const uint16_t qMaxAddress = plc::sensorBaseAddress(3) + plc::OFF_CFG_Q_MAX;
  check(writeMultiple(modbus, qMaxAddress, {100}).errorSet,
        "a sensor register on a channel that is not in use is still refused");

  // The block with no network module attached at all.
  Device headless;
  ModbusDependencies d = headless.deps();
  d.net = nullptr;
  ModbusManager noNet(d);
  const ModbusMessage response = writeMultiple(noNet, plc::net_reg::kWifiSsid, {0x4142});
  check(response.errorSet && response.errorCode == Modbus::ILLEGAL_DATA_ADDRESS,
        "with no NetSettings the block is not served, and FC16 says so as FC6 does");
}

void byteCountIsStillChecked() {
  std::printf("\n[the frame's own arithmetic]\n");

  Device dev;
  ModbusManager modbus(dev.deps());

  // byteCount must be exactly twice the word count. A frame claiming otherwise is malformed, and this
  // guard predates the fix — it is asserted because the new pre-validation runs after it.
  const ModbusMessage bad = modbus.handleWriteMultiple(
      ModbusMessage::withFields(kSlave, kFc16, plc::net_reg::kWifiSsid, 4, 6, words({1, 2, 3})));
  check(bad.errorSet && bad.errorCode == Modbus::ILLEGAL_DATA_VALUE,
        "a byte count that disagrees with the word count is ILLEGAL_DATA_VALUE");
}

void singleWriteStillWorks() {
  std::printf("\n[FC6 is unchanged]\n");

  Device dev;
  ModbusManager modbus(dev.deps());
  const ModbusMessage response = modbus.handleWriteSingle(
      ModbusMessage::withFields(kSlave, kFc06, plc::net_reg::kWifiSsid, 0x4142, 0));
  check(!response.errorSet, "FC6 into the network block is still accepted");
  check(stagedField(dev.net, plc::NetField::WifiSsid) == std::string("AB"),
        "and still stages the two characters");

  // The deliberate difference: a single write of a non-magic value to NET_APPLY reports the refusal as
  // an exception, because a master aiming one register at the apply meant to commit. Inside a block
  // write the same value is a word being passed over, and must not except.
  const ModbusMessage badMagic = modbus.handleWriteSingle(
      ModbusMessage::withFields(kSlave, kFc06, plc::net_reg::kApply, 0x1234, 0));
  check(badMagic.errorSet, "FC6 with a wrong magic at NET_APPLY still excepts");
  check(!writeMultiple(modbus, plc::net_reg::kApply, {0x1234}).errorSet,
        "while FC16 with the same wrong magic does not — §5.1's zero-fill must survive");
}

}  // namespace

int main() {
  std::printf("FC16 write-multiple, through the real frame handler\n");
  ssidOverBlockWrite();
  theAsymmetryThatGaveItAway();
  blockWriteAcrossTheWholeRegion();
  applyIsDeferredToTheEndOfTheBlock();
  thePortalPasswordIsWritableToItsFullCapacity();
  theApplyMustCommitAFTERTheRegistersBEYONDIt();
  refusalsThatMustStay();
  byteCountIsStillChecked();
  singleWriteStillWorks();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
