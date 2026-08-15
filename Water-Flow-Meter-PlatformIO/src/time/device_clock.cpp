#include "time/device_clock.h"

namespace plc {

const char* clockSourceText(ClockSource source) {
  switch (source) {
    case ClockSource::None:
      // Five characters, ASCII, matching the vocabulary the panel already uses for WiFi states (§4.6).
      return "UNSET";
    case ClockSource::Rtc:
      return "RTC";
    case ClockSource::Operator:
      return "MANUAL";
    case ClockSource::Ntp:
      return "NTP";
  }
  return "UNSET";
}

uint32_t epochFromUtcCivil(int year, int month, int day, int hour, int minute, int second) {
  if (year < 1970 || month < 1 || month > 12 || day < 1 || day > 31) {
    return 0;
  }
  if (hour < 0 || hour > 23 || minute < 0 || minute > 59 || second < 0 || second > 60) {
    return 0;
  }
  // Howard Hinnant's days_from_civil: shifting the era to start in March makes the leap day the last day
  // of the year, so no branch is needed for it at all.
  const int y = year - (month <= 2 ? 1 : 0);
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned doy = static_cast<unsigned>((153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1);
  const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  const long long days = static_cast<long long>(era) * 146097 + static_cast<long long>(doe) - 719468;
  const long long seconds = days * 86400LL + hour * 3600LL + minute * 60LL + second;
  if (seconds <= 0) {
    return 0;
  }
  return static_cast<uint32_t>(seconds);
}

UtcCivil civilFromEpoch(uint32_t epoch) {
  // Howard Hinnant's civil_from_days, the exact inverse of days_from_civil above. Held in signed 64-bit
  // throughout: `doy` and `mp` are only meaningful relative to the March-based era, so narrowing any of
  // them to the uint32_t this function was handed would truncate the intermediate, not the result.
  const long long secondsOfDay = static_cast<long long>(epoch % 86400u);
  const long long z = static_cast<long long>(epoch / 86400u) + 719468;
  const long long era = (z >= 0 ? z : z - 146096) / 146097;
  const unsigned long long doe = static_cast<unsigned long long>(z - era * 146097);
  const unsigned long long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
  const long long y = static_cast<long long>(yoe) + era * 400;
  const unsigned long long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
  const unsigned long long mp = (5 * doy + 2) / 153;
  const unsigned long long d = doy - (153 * mp + 2) / 5 + 1;
  // Signed on purpose: the March-based month index has to move BACK nine for the autumn months, and in
  // unsigned arithmetic that subtraction is a wrap that happens to come out right — which is a thing a
  // reader has to prove rather than read.
  const long long m = static_cast<long long>(mp) + (mp < 10 ? 3 : -9);
  UtcCivil out;
  out.day = static_cast<int>(d);
  out.month = static_cast<int>(m);
  // The era began in March, so January and February belong to the following civil year.
  out.year = static_cast<int>(y + (m <= 2 ? 1 : 0));
  out.hour = static_cast<int>(secondsOfDay / 3600);
  out.minute = static_cast<int>((secondsOfDay / 60) % 60);
  out.second = static_cast<int>(secondsOfDay % 60);
  return out;
}

bool clockEpochPlausible(uint32_t epoch) {
  return epoch >= DeviceClock::kEarliestPlausibleEpoch && epoch < DeviceClock::kLatestPlausibleEpoch;
}

void DeviceClock::noteBootTrust(bool voltageLowFlagSet, uint32_t rtcEpoch, uint32_t nowMs) {
  if (voltageLowFlagSet || !clockEpochPlausible(rtcEpoch)) {
    /**
     * The RTC lost power, or came up with a date this device cannot have existed on.
     *
     * Deliberately stores NOTHING. An earlier shape kept the value and set a flag beside it, which is one
     * refactor away from something rendering the value and ignoring the flag — the same failure as the
     * cached `isReady` bit that survived a reboot and published a lifetime total of zero.
     */
    source_ = ClockSource::None;
    baseEpoch_ = 0;
    baseMs_ = nowMs;
    return;
  }
  source_ = ClockSource::Rtc;
  baseEpoch_ = rtcEpoch;
  baseMs_ = nowMs;
  lastSyncEpoch_ = rtcEpoch;
}

bool DeviceClock::setTime(uint32_t epoch, ClockSource source, uint32_t nowMs) {
  if (source == ClockSource::None || !clockEpochPlausible(epoch)) {
    return false;
  }
  const bool wasUnset = source_ == ClockSource::None;
  source_ = source;
  baseEpoch_ = epoch;
  baseMs_ = nowMs;
  lastSyncEpoch_ = epoch;

  /**
   * A session that began before the clock was trusted gets dated by the first sync.
   *
   * The alternative was leaving it permanently unknown, which is honest but needlessly lossy: the session
   * demonstrably started before this moment, so `now` is the earliest time it could have begun and is
   * therefore the tightest bound available. It is recorded as the start rather than as "unknown" only
   * because the operator has just supplied a trustworthy time — before that there was nothing to anchor
   * it to at all.
   */
  if (wasUnset && sessionStartUnknown_) {
    sessionStartEpoch_ = epoch;
    sessionStartUnknown_ = false;
  }
  return true;
}

uint32_t DeviceClock::now(uint32_t nowMs) const {
  if (source_ == ClockSource::None) {
    return 0;
  }
  // Wrap-safe: `millis()` rolls every 49.7 days and this device is meant to run for years, so the
  // subtraction is done in unsigned arithmetic on purpose rather than compared as a difference.
  const uint32_t elapsedMs = nowMs - baseMs_;
  return baseEpoch_ + elapsedMs / 1000u;
}

void DeviceClock::noteSessionStart(uint32_t nowMs) {
  if (source_ == ClockSource::None) {
    // Nothing to date it with. Remembered as unknown so the next sync can bound it, and so the panel can
    // say "unknown" rather than showing a zero that reads as 1970.
    sessionStartEpoch_ = 0;
    sessionStartUnknown_ = true;
    return;
  }
  sessionStartEpoch_ = now(nowMs);
  sessionStartUnknown_ = false;
}

uint32_t DeviceClock::sessionDurationSeconds(uint32_t nowMs) const {
  if (source_ == ClockSource::None || sessionStartEpoch_ == 0) {
    return 0;
  }
  const uint32_t current = now(nowMs);
  // A clock moved BACKWARDS by a correction can put the start in the future. Reporting 0 rather than a
  // huge unsigned wrap: the session did not last minus-ten-minutes, and a wrapped duration on the volume
  // page would read as a device that had been running for a century.
  return current > sessionStartEpoch_ ? current - sessionStartEpoch_ : 0;
}

}  // namespace plc
