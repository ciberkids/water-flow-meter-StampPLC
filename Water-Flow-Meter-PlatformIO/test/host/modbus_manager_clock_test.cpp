/**
 * Does a session reset arriving through ModbusManager actually DATE the clock?
 *
 * `ModbusDependencies::clock` is nullable, and until this file existed it was null in every host test
 * that constructed the struct — which are the only two places outside firmware.cpp that construct it at
 * all. So `noteSessionStart()` was reachable from exactly nowhere a test could see: a nullable dependency
 * that is null everywhere is indistinguishable from a no-op, and would have stayed that way through any
 * refactor that dropped the two call sites.
 *
 * ── WHY THIS IS A SEPARATE BINARY ────────────────────────────────────────────────────────
 *
 * interaction_test.cpp cannot host this. It DEFINES `ModbusManager::applyHoldingWrite` itself — a
 * harness stand-in that records writes — precisely so it does not have to link modbus_manager.cpp and
 * drag in eModbus. That stand-in never calls noteSessionStart, so a test added there would assert
 * against the harness rather than against the firmware, and would pass with the production arms deleted.
 *
 * This file links the REAL modbus_manager.cpp. What that took is recorded in the stubs: millis() moved
 * into an Arduino.h stub the ESP32 core's Preferences.h transitively provides on the device, and the
 * ModbusMessage stub grew the members the three frame handlers call so the TU compiles. Those handlers
 * are not exercised here — a frame is the part eModbus parses — and the stub says so at each member.
 *
 * Every dependency is real: a real RegisterBank, a real LedController, a real DeviceClock, the
 * Preferences stub. Nothing about the reset path is mocked, so the arms under test run their register
 * writes, their NVS writes and their cache resets exactly as they do on the device.
 */
#include <Preferences.h>

#include <cstdio>

#include "led/led_controller.h"
#include "modbus/modbus_manager.h"
#include "modbus/register_bank.h"
#include "modbus/register_map.h"
#include "modbus/sensor_types.h"
#include "time/device_clock.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-76s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) failures++;
}

/** 2026-08-12T14:32:07Z. `date -u -d @1786545127` -> Wed Aug 12 14:32:07 UTC 2026. */
constexpr uint32_t kBootEpoch = 1786545127u;

/**
 * Everything ModbusManager reaches through, owned so each case starts from a clean device.
 *
 * `isWritableAddress` dereferences `registers` on its first line and `syncGlobalRegisters` dereferences
 * `pollingRateKhz`, `connectedBitmap`, `undersamplingFlags` and `ledController` unconditionally, so a
 * partially-populated struct segfaults before it can assert anything. Populating all of it is also what
 * makes the assertions mean something: the reset arms really do write the register bank and really do
 * touch NVS here.
 */
struct Device {
  plc::RegisterBank registers;
  SensorData sensors[plc::kNumSensors]{};
  SensorCharacteristics configs[plc::kNumSensors]{};
  Preferences preferences;
  LedController leds;
  plc::DeviceClock clock;
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

  /**
   * Channel 1, in use and validly calibrated, carrying measured volume.
   *
   * Without `inUse` every per-sensor loop in the reset arms iterates over nothing, and a test asserting
   * that a reset cleared the totals would pass against a reset that did nothing at all.
   */
  void armChannelOne() {
    sensors[0].inUse = true;
    sensors[0].sessionLiters = 1234.5f;
    sensors[0].cumulativeLiters = 98765.0;
    sensors[0].maxFlowSinceReset = 42.0f;
    configs[0].q_max = 100;
    configs[0].f_multiplier = 6;
    configs[0].adjust = 0;
    configs[0].calibration = CalibrationType::Formula;
    configs[0].pulses_per_litre = 0;
  }
};

/** Simulated `millis()`, which is what ModbusManager passes to noteSessionStart. */
void setMillis(uint32_t ms) { arduino_stub::clockMs() = ms; }

void sessionResetDatesTheClock() {
  std::printf("\n[REG_MASTER_RESET_ALL_SESSION through ModbusManager dates the clock]\n");

  Device dev;
  dev.armChannelOne();
  setMillis(1000);
  dev.clock.noteBootTrust(false, kBootEpoch, 1000);
  ModbusManager modbus(dev.deps());

  check(dev.clock.isSet(), "the device booted with a trusted RTC");
  check(dev.clock.sessionStartEpoch() == 0, "and no session start, because nothing has reset one yet");

  // Five minutes later, a master writes 1 to the session-reset command register.
  setMillis(301'000);
  check(modbus.applyHoldingWrite(plc::REG_MASTER_RESET_ALL_SESSION, 1), "the write is accepted");

  check(dev.clock.sessionStartEpoch() == kBootEpoch + 300,
        "and the clock is dated to the moment of the reset, not to boot");
  check(dev.clock.sessionDurationSeconds(301'000) == 0, "with the session zero seconds old");
  check(dev.clock.sessionDurationSeconds(361'000) == 60, "and a minute old a minute later");

  // The reset itself still has to have happened. A test that only checked the timestamp would pass
  // against an arm that dated a session it never actually cleared.
  check(dev.sensors[0].sessionLiters == 0.0f, "the session volume really was cleared");
  check(dev.sensors[0].maxFlowSinceReset == 0.0f, "and the peak with it");
  check(dev.sensors[0].cumulativeLiters == 98765.0,
        "while the lifetime total SURVIVES — a session reset is not a measured reset");
  check(dev.registers.at(plc::REG_MASTER_RESET_ALL_SESSION) == 0,
        "and the command register self-cleared");
}

void measuredResetDatesTheClock() {
  std::printf("\n[REG_MASTER_RESET_ALL_MEASURED starts a new session too, so it dates it as well]\n");

  Device dev;
  dev.armChannelOne();
  setMillis(1000);
  dev.clock.noteBootTrust(false, kBootEpoch, 1000);
  ModbusManager modbus(dev.deps());

  setMillis(601'000);
  check(modbus.applyHoldingWrite(plc::REG_MASTER_RESET_ALL_MEASURED, 1), "the write is accepted");
  check(dev.clock.sessionStartEpoch() == kBootEpoch + 600,
        "a measured reset dates the clock — it clears the session volume, so it starts a session");
  check(dev.sensors[0].cumulativeLiters == 0.0, "and this one DOES clear the lifetime total");
  check(dev.sensors[0].sessionLiters == 0.0f, "and the session volume");
}

void peakResetDoesNotDateTheClock() {
  std::printf("\n[a peak reset is not a session reset, and must not pretend to be one]\n");

  Device dev;
  dev.armChannelOne();
  setMillis(1000);
  dev.clock.noteBootTrust(false, kBootEpoch, 1000);
  ModbusManager modbus(dev.deps());

  // Date a session first, so the assertion is "it did not CHANGE this" rather than "it is still zero",
  // which would also hold if the arm had written a zero over a real timestamp.
  setMillis(101'000);
  modbus.applyHoldingWrite(plc::REG_MASTER_RESET_ALL_SESSION, 1);
  const uint32_t dated = dev.clock.sessionStartEpoch();
  check(dated == kBootEpoch + 100, "a session reset dated the clock");

  setMillis(201'000);
  check(modbus.applyHoldingWrite(plc::REG_MASTER_RESET_ALL_MAX, 1), "then a peak reset is accepted");
  check(dev.clock.sessionStartEpoch() == dated,
        "and leaves the session start alone — the volume is still accumulating from the same moment");
  check(dev.sensors[0].maxFlowSinceReset == 0.0f, "while still clearing the peak it exists to clear");
}

void aWriteOfZeroIsNotACommand() {
  std::printf("\n[only a write of 1 is the command; a 0 must not date anything]\n");

  Device dev;
  dev.armChannelOne();
  setMillis(1000);
  dev.clock.noteBootTrust(false, kBootEpoch, 1000);
  ModbusManager modbus(dev.deps());

  setMillis(401'000);
  check(modbus.applyHoldingWrite(plc::REG_MASTER_RESET_ALL_SESSION, 0), "a write of 0 is accepted");
  check(dev.clock.sessionStartEpoch() == 0, "but dates nothing");
  check(dev.sensors[0].sessionLiters == 1234.5f, "and clears nothing");

  setMillis(402'000);
  check(modbus.applyHoldingWrite(plc::REG_MASTER_RESET_ALL_SESSION, 7),
        "and a write of 7 is accepted as well, matching every other master-reset arm");
  check(dev.clock.sessionStartEpoch() == 0, "and still dates nothing");
}

void aResetWithNoClockIsRememberedAsUndated() {
  std::printf("\n[a reset while the RTC is untrusted leaves the start awaiting a clock]\n");

  Device dev;
  dev.armChannelOne();
  setMillis(1000);
  // VLF set: the RTC lost power, so nothing it says can be believed.
  dev.clock.noteBootTrust(true, kBootEpoch, 1000);
  ModbusManager modbus(dev.deps());
  check(!dev.clock.isSet(), "the device booted with an untrusted RTC");

  setMillis(31'000);
  check(modbus.applyHoldingWrite(plc::REG_MASTER_RESET_ALL_SESSION, 1), "a master resets the session");
  check(dev.clock.sessionStartEpoch() == 0, "which cannot be dated");
  check(dev.clock.sessionStartAwaitingClock(),
        "but is remembered as awaiting one, so P3 can say so rather than showing 1970");
  check(dev.sensors[0].sessionLiters == 0.0f, "and the reset itself happened regardless");

  // The operator sets the clock. The reset that could not be dated now gets its bound.
  check(dev.clock.setTime(kBootEpoch, plc::ClockSource::Operator, 61'000), "the operator sets the time");
  check(dev.clock.sessionStartEpoch() == kBootEpoch, "which finally dates the reset");
  check(!dev.clock.sessionStartAwaitingClock(), "and nothing is left waiting");
}

void aNullClockIsStillHarmless() {
  std::printf("\n[the dependency is nullable, so a null one must still take the write]\n");

  Device dev;
  dev.armChannelOne();
  ModbusDependencies deps = dev.deps();
  // The state every host test that constructs this struct was in before this file existed. Asserted
  // rather than assumed: the guards are the reason a test with no opinion about time still passes, and a
  // refactor that made the clock mandatory would break those two files, not this line.
  deps.clock = nullptr;
  ModbusManager modbus(deps);

  setMillis(501'000);
  check(modbus.applyHoldingWrite(plc::REG_MASTER_RESET_ALL_SESSION, 1),
        "a session reset with no clock is accepted and does not crash");
  check(dev.sensors[0].sessionLiters == 0.0f, "and clears the session volume");
  check(modbus.applyHoldingWrite(plc::REG_MASTER_RESET_ALL_MEASURED, 1),
        "and so is a measured reset");
  check(dev.sensors[0].cumulativeLiters == 0.0, "clearing the lifetime total");
  check(dev.clock.sessionStartEpoch() == 0,
        "and the clock the manager was not given is untouched");
}

/**
 * ── THE CLOCK BLOCK AT 50-55 (N-d1) ──────────────────────────────────────────────────────
 *
 * `DeviceClock::setTime` had no production caller at all before this block: no register reached it, no
 * portal page offered it, the panel could not (there is no on-device text editor) and NTP existed only in
 * a comment. So a field device read `UNSET` unless its RTC happened to hold a date it was allowed to
 * trust, and the master that is the source of truth for every other setting could not fix it.
 *
 * These cases drive the REAL manager against a REAL DeviceClock, so they exercise the staging, the magic,
 * the plausibility floor and the published read-back the way a master would.
 */
void aStagedEpochIsAppliedByTheMagic() {
  std::printf("\n[50-52: staged halves, then the magic, set the clock as `operator`]\n");

  Device dev;
  dev.armChannelOne();
  setMillis(5000);
  ModbusManager modbus(dev.deps());

  check(!dev.clock.isSet(), "the clock starts unset — no RTC trust, nobody has written one");

  // 2026-08-18T09:00:00Z.
  constexpr uint32_t kEpoch = 1787043600u;
  check(modbus.applyHoldingWrite(plc::REG_CLOCK_SET_EPOCH_HI, static_cast<uint16_t>(kEpoch >> 16)),
        "the high word is accepted");
  check(modbus.applyHoldingWrite(plc::REG_CLOCK_SET_EPOCH_LO, static_cast<uint16_t>(kEpoch & 0xFFFFu)),
        "and the low word");
  check(!dev.clock.isSet(),
        "and NOTHING has happened yet — staging is not applying, because two registers are not atomic");

  check(modbus.applyHoldingWrite(plc::REG_CLOCK_APPLY, plc::LinkSettingsManager::kApplyMagic),
        "the magic is accepted");
  check(dev.clock.isSet(), "and now the clock is set");
  check(dev.clock.now(5000) == kEpoch, "to exactly the staged epoch");
  check(dev.clock.source() == plc::ClockSource::Operator,
        "with source `operator` — a master typed it, and the panel must be able to say so");

  // The read-back a master confirms with.
  const uint32_t published = (static_cast<uint32_t>(dev.registers.at(plc::REG_CLOCK_NOW_HI)) << 16) |
                             dev.registers.at(plc::REG_CLOCK_NOW_LO);
  check(published == kEpoch, "53-54 publish the time back, so a master can confirm its own write");
  check(dev.registers.at(plc::REG_CLOCK_SOURCE) == static_cast<uint16_t>(plc::ClockSource::Operator),
        "and 55 says who set it");
}

void anImplausibleEpochIsRefusedOnTheWire() {
  std::printf("\n[52: an implausible epoch is refused, and changes nothing]\n");

  Device dev;
  dev.armChannelOne();
  setMillis(5000);
  ModbusManager modbus(dev.deps());

  // 2019-12-31T23:59:59Z — one second under the floor, which is the year a drifted RTC and a mistyped
  // year both land near.
  constexpr uint32_t kTooEarly = 1577836799u;
  modbus.applyHoldingWrite(plc::REG_CLOCK_SET_EPOCH_HI, static_cast<uint16_t>(kTooEarly >> 16));
  modbus.applyHoldingWrite(plc::REG_CLOCK_SET_EPOCH_LO, static_cast<uint16_t>(kTooEarly & 0xFFFFu));
  check(!modbus.applyHoldingWrite(plc::REG_CLOCK_APPLY, plc::LinkSettingsManager::kApplyMagic),
        "the apply is REFUSED — false becomes a Modbus exception, so the master hears about it");
  check(!dev.clock.isSet(), "and the clock is still unset, not clamped to the floor");

  // One second later is inside the floor, and the staged halves survived the refusal.
  constexpr uint32_t kJustInside = 1577836800u;
  modbus.applyHoldingWrite(plc::REG_CLOCK_SET_EPOCH_LO, static_cast<uint16_t>(kJustInside & 0xFFFFu));
  check(modbus.applyHoldingWrite(plc::REG_CLOCK_APPLY, plc::LinkSettingsManager::kApplyMagic),
        "correcting ONE half and re-applying works — a refusal does not discard the staging");
  check(dev.clock.now(5000) == kJustInside, "2020-01-01 exactly, the earliest plausible time");
}

void theWrongMagicDoesNothing() {
  std::printf("\n[52: only the magic applies, and the read-only half stays read-only]\n");

  Device dev;
  dev.armChannelOne();
  setMillis(5000);
  ModbusManager modbus(dev.deps());
  constexpr uint32_t kEpoch = 1787043600u;
  modbus.applyHoldingWrite(plc::REG_CLOCK_SET_EPOCH_HI, static_cast<uint16_t>(kEpoch >> 16));
  modbus.applyHoldingWrite(plc::REG_CLOCK_SET_EPOCH_LO, static_cast<uint16_t>(kEpoch & 0xFFFFu));

  check(!modbus.applyHoldingWrite(plc::REG_CLOCK_APPLY, 1),
        "writing 1 is refused — the same rule the link and network blocks follow");
  check(!modbus.applyHoldingWrite(plc::REG_CLOCK_APPLY, 0),
        "and so is 0, which is what a lazy scan writes everywhere");
  check(!dev.clock.isSet(), "so the clock is untouched");

  // 53-55 are the device's own answer. A master that could write them could make the device disagree
  // with its own clock, which is the one thing this module exists to prevent.
  check(!modbus.applyHoldingWrite(plc::REG_CLOCK_NOW_HI, 0x1234), "53 is read-only");
  check(!modbus.applyHoldingWrite(plc::REG_CLOCK_NOW_LO, 0x1234), "54 is read-only");
  check(!modbus.applyHoldingWrite(plc::REG_CLOCK_SOURCE, 2), "and so is 55 — nobody may forge the source");
}

void anUndatedSessionIsFilledInByTheFirstWrite() {
  std::printf("\n[the reason this write matters: it dates a session that began before the clock did]\n");

  Device dev;
  dev.armChannelOne();
  setMillis(1000);
  ModbusManager modbus(dev.deps());

  // A reset while the clock is unset: the device genuinely cannot say when the session began.
  check(modbus.applyHoldingWrite(plc::REG_MASTER_RESET_ALL_SESSION, 1), "a session reset is accepted");
  check(dev.clock.sessionStartEpoch() == 0, "and it has no date, because there was no clock to date it");
  check(dev.clock.sessionStartAwaitingClock(),
        "but the clock remembers it is owed one — the state the panel renders differently");

  setMillis(61'000);
  constexpr uint32_t kEpoch = 1787043600u;
  modbus.applyHoldingWrite(plc::REG_CLOCK_SET_EPOCH_HI, static_cast<uint16_t>(kEpoch >> 16));
  modbus.applyHoldingWrite(plc::REG_CLOCK_SET_EPOCH_LO, static_cast<uint16_t>(kEpoch & 0xFFFFu));
  check(modbus.applyHoldingWrite(plc::REG_CLOCK_APPLY, plc::LinkSettingsManager::kApplyMagic),
        "then a master sets the clock over RS485");

  /**
   * BOUNDED, NOT BACKDATED — and I had this wrong first.
   *
   * I expected `kEpoch - 60`, reasoning from the millis gap between the reset and the sync.
   * `device_clock.cpp:107-116` does something more careful: it records the sync epoch itself, because the
   * session "demonstrably started before this moment, so `now` is the earliest time it could have begun
   * and is therefore the tightest bound available". Backdating by the millis difference would look more
   * precise and would be a fabrication — nothing recorded a trustworthy anchor at the moment of the reset,
   * which is the entire reason the timestamp was owed.
   */
  check(dev.clock.sessionStartEpoch() == kEpoch,
        "and the session start is BOUNDED to the sync moment — the earliest it demonstrably could have been");
  check(!dev.clock.sessionStartAwaitingClock(), "with nothing left owed");
  check(dev.clock.sessionDurationSeconds(61'000) == 0,
        "so the session reads zero seconds old, which is the honest floor rather than a guessed sixty");
  check(dev.clock.sessionDurationSeconds(121'000) == 60,
        "and grows from there — P3 can finally say something, and it is never longer than the truth");
}

void aClocklessManagerRefusesTheApply() {
  std::printf("\n[52 with no clock injected: refused, not crashed]\n");

  Device dev;
  dev.armChannelOne();
  setMillis(5000);
  ModbusDependencies deps = dev.deps();
  deps.clock = nullptr;
  ModbusManager modbus(deps);

  check(modbus.applyHoldingWrite(plc::REG_CLOCK_SET_EPOCH_HI, 0x6A8C), "staging is still accepted");
  check(!modbus.applyHoldingWrite(plc::REG_CLOCK_APPLY, plc::LinkSettingsManager::kApplyMagic),
        "and the apply is refused rather than dereferencing a null clock");
}

}  // namespace

int main() {
  std::printf("modbus_manager — the clock wiring on the session-reset arms\n");
  sessionResetDatesTheClock();
  measuredResetDatesTheClock();
  peakResetDoesNotDateTheClock();
  aWriteOfZeroIsNotACommand();
  aResetWithNoClockIsRememberedAsUndated();
  aNullClockIsStillHarmless();
  aStagedEpochIsAppliedByTheMagic();
  anImplausibleEpochIsRefusedOnTheWire();
  theWrongMagicDoesNothing();
  anUndatedSessionIsFilledInByTheFirstWrite();
  aClocklessManagerRefusesTheApply();
  if (failures > 0) {
    std::printf("\nFAILURES (%d of %d)\n", failures, checks);
    return 1;
  }
  std::printf("\nALL PASSED (%d checks)\n", checks);
  return 0;
}
