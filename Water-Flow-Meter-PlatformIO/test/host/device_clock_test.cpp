/**
 * The clock's TRUST model, which is the whole reason this module exists.
 *
 * Arduino-free like the navigator and the acceleration tiers, so the questions that matter — "does a
 * device whose RTC lost power ever show a timestamp", "what does a session started before the clock was
 * set report" — are a second on a host instead of a power-cycling expedition on a bench.
 */
#include <cstdio>
#include <cstring>

#include "time/device_clock.h"

namespace {

int failures = 0;

void check(bool condition, const char* what) {
  std::printf("  %-72s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) failures++;
}

/** 2026-08-14T00:00:00Z, a plausible time to work from. Verified with `date -u -d ... +%s`. */
constexpr uint32_t kNow = 1786665600u;

void voltageLowFlagTests() {
  std::printf("\n[a clock that lost power has no time, not a wrong one]\n");

  plc::DeviceClock clock;
  // VLF set, and the RTC offering a perfectly plausible-looking value anyway. This is the case that
  // matters: 2000-01-01 renders as a real date and a drifted RX8130CE reports it with total confidence.
  clock.noteBootTrust(true, kNow, 1000);
  check(!clock.isSet(), "VLF set: the clock reports unset");
  check(clock.now(1000) == 0, "and now() is 0, not the value the RTC offered");
  check(clock.source() == plc::ClockSource::None, "and the source is None");
  check(std::strcmp(plc::clockSourceText(clock.source()), "UNSET") == 0, "which renders as UNSET");

  plc::DeviceClock trusted;
  trusted.noteBootTrust(false, kNow, 1000);
  check(trusted.isSet(), "VLF clear and a plausible epoch: trusted");
  check(trusted.now(1000) == kNow, "and now() is what the RTC said");
  check(trusted.source() == plc::ClockSource::Rtc, "sourced from the RTC");
}

void plausibilityTests() {
  std::printf("\n[an implausible time is refused, never clamped]\n");

  plc::DeviceClock clock;
  // The failure that actually happens: VLF somehow clear but the calendar back at 2000.
  clock.noteBootTrust(false, 946684800u, 1000);   // 2000-01-01
  check(!clock.isSet(), "a year-2000 RTC value is refused even with VLF clear");

  check(!clock.setTime(946684800u, plc::ClockSource::Operator, 1000), "and setTime refuses it too");
  check(!clock.isSet(), "leaving the clock unset rather than clamped to the floor");
  check(clock.setTime(kNow, plc::ClockSource::Operator, 1000), "a plausible time is accepted");
  check(clock.source() == plc::ClockSource::Operator, "with the source recorded");
  check(!clock.setTime(kNow, plc::ClockSource::None, 1000), "and None is not a source anything can set");
}

void advanceTests() {
  std::printf("\n[time advances from millis, not from an I2C read per frame]\n");

  plc::DeviceClock clock;
  clock.noteBootTrust(false, kNow, 10'000);
  check(clock.now(10'000) == kNow, "at the moment it was read");
  check(clock.now(11'000) == kNow + 1, "one second later");
  check(clock.now(10'999) == kNow, "and it does not round up early");
  check(clock.now(70'000) == kNow + 60, "a minute later");

  // millis() wraps every 49.7 days. The subtraction is unsigned on purpose.
  plc::DeviceClock wrapping;
  const uint32_t nearWrap = 0xFFFFFF00u;
  wrapping.noteBootTrust(false, kNow, nearWrap);
  check(wrapping.now(nearWrap + 2000u) == kNow + 2, "and it survives the millis() wrap");
}

void sessionTests() {
  std::printf("\n[a session that began before the clock was set says so]\n");

  plc::DeviceClock clock;
  clock.noteBootTrust(true, kNow, 1000);           // untrusted boot
  clock.noteSessionStart(1000);
  check(clock.sessionStartEpoch() == 0, "session start is 0 — unknown, not 1970");
  check(clock.sessionDurationSeconds(60'000) == 0, "and no duration is claimed");

  // The first trustworthy time bounds it: the session demonstrably started before now.
  check(clock.setTime(kNow, plc::ClockSource::Ntp, 61'000), "NTP supplies a time");
  check(clock.sessionStartEpoch() == kNow, "which dates the session start to the sync moment");
  check(clock.sessionDurationSeconds(121'000) == 60, "and the duration runs from there");

  plc::DeviceClock trusted;
  trusted.noteBootTrust(false, kNow, 1000);
  trusted.noteSessionStart(31'000);                 // 30 s after boot
  check(trusted.sessionStartEpoch() == kNow + 30, "a session started with a trusted clock is dated");
  check(trusted.sessionDurationSeconds(91'000) == 60, "and its duration is the elapsed time");

  // A correction that moves the clock BACKWARDS must not produce a wrapped duration.
  trusted.setTime(kNow - 3600u, plc::ClockSource::Operator, 91'000);
  check(trusted.sessionDurationSeconds(91'000) == 0,
        "a backwards correction reports 0 rather than an unsigned wrap");
}

void civilConversionTests() {
  std::printf("\n[UTC civil date to epoch, written by hand because timegm is absent]\n");

  // Anchors checked against known Unix timestamps rather than against the same arithmetic.
  check(plc::epochFromUtcCivil(1970, 1, 1, 0, 0, 0) == 0, "the epoch itself");
  check(plc::epochFromUtcCivil(2000, 1, 1, 0, 0, 0) == 946684800u, "2000-01-01");
  check(plc::epochFromUtcCivil(2026, 8, 14, 0, 0, 0) == 1786665600u, "2026-08-14");
  check(plc::epochFromUtcCivil(2026, 8, 14, 13, 45, 30) == 1786715130u, "and with a time of day");

  // Leap years are where hand-rolled date arithmetic goes wrong, so all three rules are checked.
  check(plc::epochFromUtcCivil(2024, 2, 29, 0, 0, 0) == 1709164800u, "2024-02-29 — a leap year");
  check(plc::epochFromUtcCivil(2000, 2, 29, 0, 0, 0) == 951782400u, "2000-02-29 — divisible by 400");
  check(plc::epochFromUtcCivil(2024, 3, 1, 0, 0, 0) == 1709251200u, "and the day after a leap day");
  check(plc::epochFromUtcCivil(2100, 3, 1, 0, 0, 0) == 4107542400u,
        "2100-03-01 — 2100 is NOT a leap year, the rule a naive /4 gets wrong");

  // Rejections. A drifted RTC can present anything at all.
  check(plc::epochFromUtcCivil(1969, 12, 31, 0, 0, 0) == 0, "a pre-epoch date is refused");
  check(plc::epochFromUtcCivil(2026, 13, 1, 0, 0, 0) == 0, "month 13 is refused");
  check(plc::epochFromUtcCivil(2026, 0, 1, 0, 0, 0) == 0, "month 0 is refused");
  check(plc::epochFromUtcCivil(2026, 8, 32, 0, 0, 0) == 0, "day 32 is refused");
  check(plc::epochFromUtcCivil(2026, 8, 14, 24, 0, 0) == 0, "hour 24 is refused");
  check(plc::epochFromUtcCivil(2026, 8, 14, 0, 60, 0) == 0, "minute 60 is refused");
}

}  // namespace

int main() {
  std::printf("device_clock — trust, plausibility, advance, session\n");
  voltageLowFlagTests();
  plausibilityTests();
  advanceTests();
  sessionTests();
  civilConversionTests();
  if (failures > 0) {
    std::printf("\nFAILURES (%d)\n", failures);
    return 1;
  }
  std::printf("\nALL PASSED\n");
  return 0;
}
