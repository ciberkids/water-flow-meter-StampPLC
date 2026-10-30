/**
 * The board temperature — `DF29`.
 *
 * `boardTemperatureC` was published as `tempC` and announced to Home Assistant, and nothing assigned it,
 * so every device reported its board at 0 °C. Three halves, because the defect had three:
 *
 * - the DECODE, checked against the vendored library's own arithmetic for every 16-bit input, so the
 *   one thing this file re-derives from `LM75B_Class` cannot drift from it;
 * - the POLICY (`plc::BoardTemperature`): when to read, and the four cases in which there is no
 *   reading to publish. The case that matters most is a failed read after a good one, because keeping
 *   the old value there is how a plausible wrong number gets published again;
 * - the WIRING: that `firmware.cpp` calls the rule at all, from the logic task's MQTT block, with the
 *   reader that returns the bus's own verdict, and hands the result to the snapshot once, with nothing
 *   writing over it. Read as text, like `portal_lifecycle_test.cpp`, because "a published field nobody
 *   assigns" is invisible to every test that links against a fake. Whole statements are compared, not
 *   prefixes: review showed five prefix-passing edits, one of which published 0.0 C again.
 */
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>

#include "firmware_source_text.h"
#include "sensors/board_temperature.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-78s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) failures++;
}

uint8_t msbOf(uint16_t word) { return static_cast<uint8_t>(word >> 8); }
uint8_t lsbOf(uint16_t word) { return static_cast<uint8_t>(word & 0xFFu); }

float decodeWord(uint16_t word) { return plc::decodeLm75bCelsius(msbOf(word), lsbOf(word)); }

/**
 * `LM75B_Class::temp()` from `M5StamPLC/src/utils/lm75b/lm75b.cpp`, transcribed with only the bus read
 * removed: `read16` becomes the argument. Kept as the library wrote it — the shift of an unsigned value
 * into a `short`, the sign extension by OR-ing in 0xFC00, the multiply in double — with casts added
 * only where `-Werror` asks for them, because the point is to compare against what the library
 * computes, not against a tidier version of it.
 */
float libraryTemp(uint16_t read16) {
  short value;
  value = static_cast<short>(read16 >> 5);
  if (value & (1 << 10)) value = static_cast<short>(value | 0xFC00);
  return static_cast<float>(value * 0.125);
}

void theDecodeIsTheLibrarys() {
  std::printf("\n[the decode — 11-bit two's complement in the top bits, 0.125 C a step]\n");

  // Worked from the format, not quoted: 25 C is 200 steps = 0x0C8, shifted left five = 0x1900.
  check(decodeWord(0x1900) == 25.0f, "0x1900 is +25.0 C (200 steps)");
  check(decodeWord(0xE700) == -25.0f, "0xE700 is -25.0 C (2048 - 200 = 0x738)");
  check(decodeWord(0x0020) == 0.125f, "0x0020 is +0.125 C, one step");
  check(decodeWord(0xFFE0) == -0.125f, "0xFFE0 is -0.125 C, minus one step");
  check(decodeWord(0x7D00) == 125.0f, "0x7D00 is +125.0 C, the top of the operating range");
  check(decodeWord(0xC900) == -55.0f, "0xC900 is -55.0 C, the bottom of it");
  check(decodeWord(0x0000) == 0.0f, "0x0000 is 0.0 C");
  // The bottom five bits are not part of the reading, so whatever the chip leaves there must not move
  // the value.
  check(decodeWord(0x191F) == 25.0f, "the bottom five bits are ignored: 0x191F is still +25.0 C");

  bool allAgree = true;
  uint32_t firstDisagreement = 0;
  for (uint32_t word = 0; word <= 0xFFFFu; ++word) {
    if (decodeWord(static_cast<uint16_t>(word)) != libraryTemp(static_cast<uint16_t>(word))) {
      if (allAgree) firstDisagreement = word;
      allAgree = false;
    }
  }
  if (!allAgree) std::printf("      first disagreement at 0x%04X\n", firstDisagreement);
  check(allAgree, "all 65,536 inputs decode exactly as LM75B_Class::temp() does");
}

void theRangeCatchesNonsenseOnly() {
  std::printf("\n[plausibility — the chip's operating range, and what it cannot see]\n");

  check(plc::lm75bCelsiusPlausible(125.0f) && plc::lm75bCelsiusPlausible(-55.0f),
        "both ends of -55..125 C are accepted, as writeAlertTempHelper's clamp is inclusive");
  check(!plc::lm75bCelsiusPlausible(decodeWord(0x7D20)), "+125.125 C, one step over, is refused");
  check(!plc::lm75bCelsiusPlausible(decodeWord(0xC8E0)), "-55.125 C, one step under, is refused");
  check(!plc::lm75bCelsiusPlausible(decodeWord(0x7FE0)) &&
            !plc::lm75bCelsiusPlausible(decodeWord(0x8000)),
        "and so are the register's own extremes, +127.875 and -128.0 C");
  check(!plc::lm75bCelsiusPlausible(std::nanf("")), "NaN is not a temperature");

  // THE HONEST LIMIT, asserted so nobody mistakes the range check for a bus check: the two patterns a
  // stuck bus is most likely to produce both decode to temperatures a real board can have.
  check(plc::lm75bCelsiusPlausible(decodeWord(0x0000)),
        "all-zero bytes (0.0 C) PASS the range check — only the bus's bool can refuse them");
  check(plc::lm75bCelsiusPlausible(decodeWord(0xFFFF)),
        "all-one bytes (-0.125 C) pass it too");

  // Exhaustive, so the count is a property of the rule and not of the cases picked: -440..+1000 steps
  // is 1,441 codes, each with 32 settings of the five ignored bits.
  uint32_t plausible = 0;
  for (uint32_t word = 0; word <= 0xFFFFu; ++word) {
    if (plc::lm75bCelsiusPlausible(decodeWord(static_cast<uint16_t>(word)))) ++plausible;
  }
  std::printf("      plausible 16-bit words: %u\n", static_cast<unsigned>(plausible));
  check(plausible == 1441u * 32u, "exactly 1,441 codes x 32 = 46,112 of the 65,536 words are accepted");
}

void nothingIsPublishedBeforeTheFirstRead() {
  std::printf("\n[before any read — no reading, and no read before the chip's first conversion]\n");

  plc::BoardTemperature t;
  check(std::isnan(t.celsius(0)) && std::isnan(t.celsius(500000)),
        "a fresh object has no reading at any time: NaN, which publishes as null");
  check(!t.due(0) && !t.due(plc::kLm75bConversionPeriodMs - 1),
        "no read inside the first conversion period, whose register content nobody has read about");
  check(t.due(plc::kLm75bConversionPeriodMs), "the first read is due once it has passed");
}

void aGoodReadIsPublishedForOneIntervalAndNoLonger() {
  std::printf("\n[a good read — published until the next read is due, and not a moment longer]\n");

  constexpr uint32_t kInterval = plc::BoardTemperature::kReadIntervalMs;
  plc::BoardTemperature t;
  check(t.noteRead(1000, true, 0x19, 0x00) == plc::BoardTemperatureRead::Good, "0x1900 reads Good");
  check(t.celsius(1000) == 25.0f, "and is published at once");
  check(t.celsius(1000 + kInterval - 1) == 25.0f, "and for the rest of the interval");
  check(!t.due(1000 + kInterval - 1), "no second read inside the interval");
  check(t.due(1000 + kInterval), "the next is due exactly one interval later");
  // The backstop: the caller stopped sampling — an MQTT outage, or a refactor that dropped the call.
  check(std::isnan(t.celsius(1000 + kInterval)),
        "a reading nobody refreshed goes non-finite at one interval, never stale-as-fresh");
  // What this bounds is the SNAPSHOT. A diagnostics message already queued before an outage is the
  // publisher's, and goes out first on the reconnect; board_temperature.h says so.
  check(std::isnan(t.celsius(1000 + 60u * kInterval)),
        "so no snapshot after an hour-long outage is filled with the value from before it");
}

void aFailedReadDoesNotFallBackToTheLastGoodOne() {
  std::printf("\n[a failed read after a good one — null, NOT the previous value]\n");

  constexpr uint32_t kInterval = plc::BoardTemperature::kReadIntervalMs;
  plc::BoardTemperature t;
  t.noteRead(1000, true, 0x19, 0x00);
  const uint32_t failedAt = 1000 + kInterval;
  // Bytes that WOULD decode to a plausible 26 C, with the bus saying the read failed. That is
  // LM75B_Class::read16's defect exactly: a buffer that holds something, decoded regardless.
  check(t.noteRead(failedAt, false, 0x1A, 0x00) == plc::BoardTemperatureRead::BusFailed,
        "a read the bus refused is BusFailed");
  check(std::isnan(t.celsius(failedAt)),
        "and publishes null at once — the 25 C from a minute ago is not kept as a fallback");
  check(std::isnan(t.celsius(failedAt + 1)), "and stays null");
  check(!t.due(failedAt + 1000),
        "a failure is NOT retried sooner: a stalled bus is the costly case, and faster would multiply it");
  check(t.due(failedAt + kInterval), "it is retried at the ordinary cadence");
  check(t.noteRead(failedAt + kInterval, true, 0x1A, 0x00) == plc::BoardTemperatureRead::Good &&
            t.celsius(failedAt + kInterval) == 26.0f,
        "and the next good read is published again");

  // At the real cadence the failed read lands exactly when the previous value reaches its maximum age,
  // so the two rules agree and the checks above cannot tell which one produced the null — the first
  // mutation run proved it, staying green with the fallback rule deleted. So the failure rule is also
  // checked on its own, with a failure recorded while the previous value is still young.
  plc::BoardTemperature young;
  young.noteRead(1000, true, 0x19, 0x00);
  young.noteRead(2000, false, 0x19, 0x00);
  check(std::isnan(young.celsius(2000)),
        "a failure one second after a good read still publishes null: no fallback, whatever its age");
}

void anImplausibleReadIsTreatedAsAFailure() {
  std::printf("\n[an implausible read — the bus answered, with a code no working chip produces]\n");

  plc::BoardTemperature t;
  t.noteRead(1000, true, 0x19, 0x00);
  check(t.noteRead(2000, true, 0x7F, 0xE0) == plc::BoardTemperatureRead::Implausible,
        "+127.875 C is Implausible");
  check(std::isnan(t.celsius(2000)), "and publishes null, not the previous 25 C and not 127.9");
}

void survivesTheMillisRollover() {
  std::printf("\n[the ~49-day millis() rollover]\n");

  constexpr uint32_t kInterval = plc::BoardTemperature::kReadIntervalMs;
  plc::BoardTemperature t;
  const uint32_t beforeWrap = 0xFFFFFFFFu - 1000u;
  t.noteRead(beforeWrap, true, 0x19, 0x00);
  const uint32_t afterWrap = beforeWrap + 30000u;  // wraps past zero
  check(afterWrap < beforeWrap, "the clock has wrapped in this case");
  check(t.celsius(afterWrap) == 25.0f, "a reading taken before the wrap is still current after it");
  check(!t.due(afterWrap), "and the next read is not brought forward by the wrap");
  check(t.due(beforeWrap + kInterval), "nor pushed back: it is due one interval after the last");
}

/** A fake bus: counts transactions and answers as told. */
struct FakeBus {
  bool succeeds = true;
  uint8_t msb = 0x19;
  uint8_t lsb = 0x00;
  bool writesOnFailure = false;
  int calls = 0;
  bool bufferArrivedZeroed = true;

  bool operator()(uint8_t* out) {
    ++calls;
    if (out[0] != 0 || out[1] != 0) bufferArrivedZeroed = false;
    if (succeeds || writesOnFailure) {
      out[0] = msb;
      out[1] = lsb;
    }
    return succeeds;
  }
};

void rarelyMeansOnceAMinute() {
  std::printf("\n[the cadence — an hour of logic passes, one millisecond apart]\n");

  plc::BoardTemperature t;
  FakeBus bus;
  int notDue = 0;
  for (uint32_t now = 0; now < 3600000u; ++now) {
    if (plc::sampleBoardTemperature(t, now, bus) == plc::BoardTemperatureRead::NotDue) ++notDue;
  }
  std::printf("      bus transactions in an hour: %d\n", bus.calls);
  check(bus.calls == 60, "3,600,000 passes cost the shared bus 60 transactions, one a minute");
  check(t.attempts() == 60u, "and the policy counted the same 60");
  check(notDue == 3600000 - 60, "every other pass touched nothing");
  check(bus.bufferArrivedZeroed, "the buffer reaches the bus zeroed, so a silent reader leaks no stack");
}

void theSamplerTrustsTheBusVerdict() {
  std::printf("\n[sampleBoardTemperature — the bus's bool decides, whatever the buffer holds]\n");

  plc::BoardTemperature t;
  FakeBus good;
  check(plc::sampleBoardTemperature(t, 100, good) == plc::BoardTemperatureRead::Good &&
            t.celsius(100) == 25.0f,
        "a bus that answers 0x1900 publishes 25.0 C");

  FakeBus lying;
  lying.succeeds = false;
  lying.writesOnFailure = true;  // fills the buffer with a plausible 26 C and still reports failure
  lying.msb = 0x1A;
  check(plc::sampleBoardTemperature(t, 100 + plc::BoardTemperature::kReadIntervalMs, lying) ==
            plc::BoardTemperatureRead::BusFailed,
        "a bus that reports failure is BusFailed even with plausible bytes in the buffer");
  check(std::isnan(t.celsius(100 + plc::BoardTemperature::kReadIntervalMs)),
        "and publishes null — the defect M5StamPLC.getTemp() has, refused here");
}

void firmwareReadsItAndPublishesIt() {
  std::printf("\n[DF29's actual defect — firmware.cpp must READ the chip and HAND the reading on]\n");

  const std::string raw = firmware_source::loadFirmwareSource();
  check(!raw.empty(), "src/firmware.cpp is readable from the test's working directory");
  if (raw.empty()) return;
  // Comments blanked, so the paragraphs explaining these calls cannot satisfy the checks for them.
  const std::string source = firmware_source::withoutCommentsOrStrings(raw);
  // The same text with no whitespace at all, for the checks that compare a WHOLE statement. A reformat
  // is not a defect and must not fail them.
  const std::string compact = firmware_source::withoutWhitespace(source);

  // ── The one call, and the reader it is handed ──
  //
  // This used to check the call's PREFIX, `plc::sampleBoardTemperature(boardTemperature`, and a
  // review's mutation run passed every host check with `[](uint8_t*) { return true; }` in place of
  // the reader: a lambda that never touches the bus and always says it succeeded.
  // sampleBoardTemperature zeroes its buffer, 0x0000 decodes to 0.0 C, and 0.0 C passes the range
  // check (asserted above, on purpose). So that one argument published "tempC":0.0 as a good reading
  // every minute, which is DF29's original symptom, with the gate green. The whole call is compared now.
  const auto samples = firmware_source::offsetsOf(source, "sampleBoardTemperature(");
  check(samples.size() == 1, "firmware.cpp calls sampleBoardTemperature once");
  check(firmware_source::offsetsOf(
            compact, "plc::sampleBoardTemperature(boardTemperature,now,readBoardTemperatureBytes)")
                .size() == 1,
        "with (boardTemperature, now, readBoardTemperatureBytes) and nothing else");

  // The reader's whole body, compared as one statement. Until then the only check on it was the route
  // check at the bottom, `m5::In_I2C.readRegister(plc::kLm75bAddress` in one place, and that let three
  // more mutations through, each a build that works and publishes a wrong number: the bus's bool thrown
  // away and `true` returned (LM75B_Class::read16's defect, back in the one place this fix moved the
  // read to), the configuration register read in place of the temperature, and one byte read instead
  // of two. `|| true`, a second statement and another bus speed are the same shape. Anything but this
  // one statement fails, so the bool the reader returns is the bus's own.
  const std::string readerBody =
      firmware_source::functionBody(source, "bool readBoardTemperatureBytes(uint8_t* out)");
  check(firmware_source::withoutWhitespace(readerBody) ==
            "returnm5::In_I2C.readRegister(plc::kLm75bAddress,plc::kLm75bTemperatureRegister,out,2,"
            "plc::kLm75bBusFrequencyHz);",
        "the reader is defined once and is only `return m5::In_I2C.readRegister(...)`");
  check(firmware_source::offsetsOf(source, "readBoardTemperatureBytes").size() == 2,
        "and named only twice, defined and handed over: no read off the cadence");

  // ── Handed on once, and nothing writes over it ──
  //
  // Review's fifth mutation kept the assignment and added `snapshotInputs.boardTemperatureC = 41.2f;`
  // after it, and the old check, which looked for the assignment's prefix, still found it. So the
  // field's NAME may appear only once in firmware.cpp's code. That also refuses a write to
  // `snapshot.diagnostics.boardTemperatureC` after the fill, which a check on `snapshotInputs.` would
  // miss. And that one appearance must be the whole statement, with this pass's `now`.
  const auto assigns = firmware_source::offsetsOf(source, "boardTemperatureC");
  check(assigns.size() == 1,
        "boardTemperatureC appears once in firmware.cpp: one writer, no overwrite");
  check(firmware_source::offsetsOf(
            compact, "snapshotInputs.boardTemperatureC=boardTemperature.celsius(now);")
                .size() == 1,
        "and it is snapshotInputs.boardTemperatureC = boardTemperature.celsius(now)");
  const auto fills = firmware_source::offsetsOf(source, "plc::fillMqttSnapshot(snapshot");
  check(fills.size() == 1, "and fills the MQTT snapshot in one place");
  if (samples.empty() || assigns.empty() || fills.empty()) return;
  std::printf("      read at line %zu, handed on at line %zu, snapshot filled at line %zu\n",
              firmware_source::lineOf(source, samples.front()),
              firmware_source::lineOf(source, assigns.front()),
              firmware_source::lineOf(source, fills.front()));

  check(samples.front() < assigns.front() && assigns.front() < fills.front(),
        "in that order: read, then hand on, then fill — so the snapshot carries this pass's read");

  // The core constraint. pollingTaskCode is defined before logicTaskCode and setup() after it, so a read
  // strictly between the logic task's opening and setup() is a read on the logic task.
  const auto logicTask = firmware_source::offsetsOf(source, "void logicTaskCode(");
  const auto setupFn = firmware_source::offsetsOf(source, "void setup(");
  const auto pollingTask = firmware_source::offsetsOf(source, "void pollingTaskCode(");
  check(logicTask.size() == 1 && setupFn.size() == 1 && pollingTask.size() == 1 &&
            pollingTask.front() < logicTask.front() && logicTask.front() < samples.front() &&
            samples.front() < setupFn.front(),
        "the read is inside logicTaskCode, on core 1 — never in pollingTaskCode on the sampler's core");

  // Inside the connected block: R2.1.6's "only as often as its purpose requires", and the purpose is
  // the snapshot. The nearest connected() test before the fill must also precede the read. Since DF30
  // the block's gate also asks for a session the logic task started, and waits while a CONNECTED is
  // latched but unconsumed — `plc::mqttConnectedPassMayRun`, which ha_discovery_job_test holds — so it
  // is matched by that exact text: a gate that is neither would fail here, not pass.
  const auto connectedTests = firmware_source::offsetsOf(
      source,
      "if (plc::mqttConnectedPassMayRun(mqttTransport.connected(), mqttSessionStarted, "
      "mqttConnectLatched)) {");
  std::size_t nearest = std::string::npos;
  for (std::size_t at : connectedTests) {
    if (at < fills.front()) nearest = at;
  }
  check(nearest != std::string::npos && nearest < samples.front(),
        "and inside the MQTT connected block, so no broker means no bus time taken from the sampler");

  // Exactly one route onto the bus for this chip, and the library's unchecked one is not it.
  check(firmware_source::offsetsOf(source, "m5::In_I2C.readRegister(plc::kLm75bAddress").size() == 1,
        "the chip is read through m5::In_I2C.readRegister, which reports failure, in one place");
  check(firmware_source::offsetsOf(source, "getTemp(").empty() &&
            firmware_source::offsetsOf(source, "LM75B.temp(").empty(),
        "and never through M5StamPLC.getTemp() / LM75B.temp(), which decode a failed read anyway");
  check(firmware_source::offsetsOf(source, "boardTemperature.noteRead(").empty(),
        "nothing records a reading behind sampleBoardTemperature's back");
}

}  // namespace

int main() {
  std::printf("board_temperature — the LM75B, and when to believe it (DF29)\n");
  theDecodeIsTheLibrarys();
  theRangeCatchesNonsenseOnly();
  nothingIsPublishedBeforeTheFirstRead();
  aGoodReadIsPublishedForOneIntervalAndNoLonger();
  aFailedReadDoesNotFallBackToTheLastGoodOne();
  anImplausibleReadIsTreatedAsAFailure();
  survivesTheMillisRollover();
  rarelyMeansOnceAMinute();
  theSamplerTrustsTheBusVerdict();
  firmwareReadsItAndPublishesIt();
  if (failures > 0) {
    std::printf("\nFAILURES (%d of %d)\n", failures, checks);
    return 1;
  }
  std::printf("\nALL PASSED (%d checks)\n", checks);
  return 0;
}
