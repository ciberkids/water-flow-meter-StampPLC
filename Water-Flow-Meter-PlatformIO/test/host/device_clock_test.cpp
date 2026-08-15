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
/** Counted and REPORTED, like every sibling binary: a suite that prints a bare "ALL PASSED" cannot
 *  distinguish 90 passing checks from a file whose test functions were never called — which is this
 *  repo's most-repeated failure and the reason every other host test prints its total. */
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
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

/** Reads as the panel prints it, so a wrong field is visible rather than inferred from an integer. */
bool civilIs(uint32_t epoch, int year, int month, int day, int hour, int minute, int second) {
  const plc::UtcCivil c = plc::civilFromEpoch(epoch);
  return c.year == year && c.month == month && c.day == day && c.hour == hour && c.minute == minute &&
         c.second == second;
}

void civilFromEpochTests() {
  std::printf("\n[epoch back to a UTC civil date — every anchor below verified with `date -u -d @N`]\n");

  // `date -u -d @1577836800` -> Wed Jan  1 00:00:00 UTC 2020. This one is kEarliestPlausibleEpoch, so
  // it is also the oldest date the panel can ever be asked to render.
  check(civilIs(1577836800u, 2020, 1, 1, 0, 0, 0), "1577836800 is 2020-01-01 00:00:00");
  // `date -u -d @1786545127` -> Wed Aug 12 14:32:07 UTC 2026.
  check(civilIs(1786545127u, 2026, 8, 12, 14, 32, 7), "1786545127 is 2026-08-12 14:32:07");
  // `date -u -d @1767225599` / `@1767225600` — the year rollover, either side of midnight.
  check(civilIs(1767225599u, 2025, 12, 31, 23, 59, 59), "1767225599 is 2025-12-31 23:59:59");
  check(civilIs(1767225600u, 2026, 1, 1, 0, 0, 0), "1767225600 is 2026-01-01 00:00:00");

  // ── The four leap-year cases, which is where hand-rolled date arithmetic actually breaks ──
  // `date -u -d @1709164799` -> Wed Feb 28 23:59:59 UTC 2024, `@1709164800` -> Thu Feb 29 00:00:00.
  check(civilIs(1709164799u, 2024, 2, 28, 23, 59, 59), "1709164799 is 2024-02-28 23:59:59");
  check(civilIs(1709164800u, 2024, 2, 29, 0, 0, 0), "1709164800 is 2024-02-29 — the leap day itself");
  check(civilIs(1709251200u, 2024, 3, 1, 0, 0, 0), "1709251200 is 2024-03-01 — the day after it");
  // A non-leap year: February must END on the 28th, and the 29th must not exist.
  check(civilIs(1677542400u, 2023, 2, 28, 0, 0, 0), "1677542400 is 2023-02-28");
  check(civilIs(1677628800u, 2023, 3, 1, 0, 0, 0), "1677628800 is 2023-03-01, with no 29th between");
  // 2100 is divisible by 4 and NOT a leap year. `date -u -d @4107499200` -> Sun Feb 28 12:00:00 UTC
  // 2100, `@4107542400` -> Mon Mar  1 00:00:00 UTC 2100. A /4-only implementation reports Feb 29 here.
  check(civilIs(4107499200u, 2100, 2, 28, 12, 0, 0), "4107499200 is 2100-02-28 12:00:00");
  check(civilIs(4107542400u, 2100, 3, 1, 0, 0, 0),
        "4107542400 is 2100-03-01 — 2100 has no leap day, the /4-only bug");
  // 2096 IS a leap year, so the century rule must not have been applied too widely.
  // `date -u -d @3981312000` -> Wed Feb 29 00:00:00 UTC 2096.
  check(civilIs(3981312000u, 2096, 2, 29, 0, 0, 0), "3981312000 is 2096-02-29 — 2096 IS a leap year");

  // The plausibility window's far end. `date -u -d @4102444799` -> Fri Dec 31 23:59:59 UTC 2099.
  check(civilIs(4102444799u, 2099, 12, 31, 23, 59, 59), "4102444799 is 2099-12-31 23:59:59");
  check(civilIs(4102444800u, 2100, 1, 1, 0, 0, 0), "4102444800 is 2100-01-01 00:00:00");

  // 0 is never rendered by the panel — it is the sentinel for "no session start" — but the conversion
  // must still be total rather than undefined, because a resolver that forgot the guard would otherwise
  // read uninitialised fields instead of an obviously wrong date.
  check(civilIs(0u, 1970, 1, 1, 0, 0, 0), "0 is 1970-01-01 00:00:00, total rather than undefined");

  /**
   * And the exhaustive check the fixed anchors above cannot give: every day in the window the device
   * will ever display, round-tripped through both directions.
   *
   * 29,585 days at one second past noon, so an off-by-one in either function's day accounting shows up
   * as a mismatch rather than as a date nobody happened to pick as an anchor. This is the check that
   * would have caught the off-by-one-day already recorded against the forward function.
   */
  unsigned mismatches = 0;
  for (uint32_t epoch = plc::DeviceClock::kEarliestPlausibleEpoch + 43'201u;
       epoch < plc::DeviceClock::kLatestPlausibleEpoch;
       epoch += 86'400u) {
    const plc::UtcCivil c = plc::civilFromEpoch(epoch);
    if (plc::epochFromUtcCivil(c.year, c.month, c.day, c.hour, c.minute, c.second) != epoch) {
      ++mismatches;
    }
  }
  check(mismatches == 0, "every day from 2020 to 2100 round-trips epoch -> civil -> epoch");
}

void sessionStartAwaitingClockTests() {
  std::printf("\n[the two reasons a session start cannot be stated are told apart]\n");

  // Boot with a dead RTC and nothing reset yet: no time, and nothing waiting to be dated either.
  plc::DeviceClock dead;
  dead.noteBootTrust(true, kNow, 1000);
  check(!dead.isSet(), "VLF set: no clock");
  check(dead.sessionStartEpoch() == 0, "and no session start");
  check(!dead.sessionStartAwaitingClock(),
        "and nothing is awaiting a clock — no reset has happened yet");

  // A reset while there is no clock. Now something IS waiting, and setting the clock will fill it in.
  dead.noteSessionStart(2000);
  check(dead.sessionStartEpoch() == 0, "a reset with no clock still has no start epoch");
  check(dead.sessionStartAwaitingClock(), "but it is now awaiting a clock");
  check(dead.setTime(kNow, plc::ClockSource::Operator, 3000), "the operator sets the time");
  check(dead.sessionStartEpoch() == kNow, "which dates the waiting reset");
  check(!dead.sessionStartAwaitingClock(), "and clears the wait");

  // A trusted clock at boot with no reset since: the start is unknown, but nothing is WAITING —
  // setting the clock again would not produce a start time, only a reset will.
  plc::DeviceClock trusted;
  trusted.noteBootTrust(false, kNow, 1000);
  check(trusted.isSet(), "a trusted clock at boot");
  check(trusted.sessionStartEpoch() == 0, "has no session start until one is recorded");
  check(!trusted.sessionStartAwaitingClock(),
        "and is NOT awaiting a clock — it has one; this is the case a second sync cannot fix");
  trusted.noteSessionStart(4000);
  check(trusted.sessionStartEpoch() == kNow + 3, "a reset dates it immediately");
  check(!trusted.sessionStartAwaitingClock(), "with nothing left waiting");
}

}  // namespace

int main() {
  std::printf("device_clock — trust, plausibility, advance, session\n");
  voltageLowFlagTests();
  plausibilityTests();
  advanceTests();
  sessionTests();
  civilConversionTests();
  civilFromEpochTests();
  sessionStartAwaitingClockTests();
  if (failures > 0) {
    std::printf("\nFAILURES (%d of %d)\n", failures, checks);
    return 1;
  }
  std::printf("\nALL PASSED (%d checks)\n", checks);
  return 0;
}
