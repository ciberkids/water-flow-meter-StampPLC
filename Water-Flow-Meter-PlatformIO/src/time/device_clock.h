#pragma once

#include <cstdint>

namespace plc {

/**
 * Wall-clock time, and — the part that matters — whether it can be believed.
 *
 * The board has an RX8130CE at I2C 0x32, so a timestamp is possible. Whether the clock RAN across the
 * last power cut is a different question, and it is not one the datasheet can answer for us: the chip has
 * a backup-supply pin and a charging circuit, but whether M5Stack populated a cell or a supercapacitor is
 * not stated anywhere in the vendor documentation. Sibling boards differ — the Tab5 uses a 70000 uF
 * supercap, the CardputerZero runs its RX8130CE off the device battery.
 *
 * So this module never assumes. The RX8130CE answers the question itself on every boot through VLF, the
 * Voltage Low Flag (register 0x1D, bit 1), which the chip sets when its oscillator has lost power. A set
 * VLF means the calendar registers hold whatever they drifted to, which is typically the year 2000 — a
 * number that looks entirely plausible on a panel and is worse than no number at all.
 *
 * ── THE ORDERING CONSTRAINT, which is the whole reason this is a module and not two lines ────────
 *
 * `M5StamPLC.begin()` calls `_rtc_init()`, which calls `RX8130.clearIrqFlags()`, which writes 0 to the
 * WHOLE flag register — VLF included. The library exposes no reader for it. So the flag has to be read
 * out of I2C directly, BEFORE `begin()` runs, and there is exactly one moment in the program's life when
 * that is possible. Read it late and the device has destroyed the only evidence that its own clock is
 * wrong, and will then report a confident 1 January 2000 to the panel, to Modbus and to MQTT.
 *
 * Nothing here touches I2C or Arduino. `noteBootTrust` takes the flag as a bool so the whole trust model
 * is host-testable, and so the one ordering-sensitive read stays visible at the call site in firmware.cpp
 * rather than being buried behind an abstraction that could be called at any time.
 */

/** Where the current time came from. Reported, because "what is the time" and "who says so" differ. */
enum class ClockSource : uint8_t {
  /** Nobody has set it. The epoch below is meaningless and every consumer must say so. */
  None = 0,
  /** The RTC was running across the boot and its VLF was clear. */
  Rtc,
  /** An operator typed it, at the panel or through the portal. */
  Operator,
  /** NTP, which requires WiFi and therefore cannot be relied on. */
  Ntp
};

const char* clockSourceText(ClockSource source);

class DeviceClock {
 public:
  /**
   * Unix epoch seconds below which a time is refused as obviously wrong.
   *
   * 2020-01-01. A drifted RX8130CE typically comes up in 2000, and an operator fat-fingering a year is
   * the other case; either way a device that did not exist yet cannot have measured anything. This is a
   * sanity floor, not a validation of correctness — it catches the failure that actually happens.
   */
  static constexpr uint32_t kEarliestPlausibleEpoch = 1577836800u;

  /** 2100-01-01, the matching ceiling. */
  static constexpr uint32_t kLatestPlausibleEpoch = 4102444800u;

  /**
   * What the RTC said at boot, and whether to believe it.
   *
   * `voltageLowFlagSet` is VLF, read from register 0x1D before anything cleared it. When it is set the
   * epoch argument is ignored entirely rather than stored and flagged: a stored-but-untrusted timestamp
   * is one somebody eventually renders.
   */
  void noteBootTrust(bool voltageLowFlagSet, uint32_t rtcEpoch, uint32_t nowMs);

  /**
   * An operator or NTP supplies the time. Returns false — changing nothing — for an implausible value.
   *
   * Refused rather than clamped, because a clamp would silently substitute a time nobody chose, and the
   * point of the whole module is that a displayed timestamp is either right or visibly absent.
   */
  bool setTime(uint32_t epoch, ClockSource source, uint32_t nowMs);

  /** True when there is a time worth showing. */
  bool isSet() const { return source_ != ClockSource::None; }
  ClockSource source() const { return source_; }

  /**
   * Seconds since the epoch, or 0 when unset.
   *
   * Advanced from `millis()` rather than re-read from the RTC on every call: the RTC is on the same I2C
   * bus as the pulse-counting inputs, and a read per frame is bus traffic the measurement pays for. The
   * chip's job is to survive the power cut; counting between reads is the MCU's.
   */
  uint32_t now(uint32_t nowMs) const;

  /** When the time was last set, as an epoch, or 0 if never. Reported so staleness is visible. */
  uint32_t lastSyncEpoch() const { return lastSyncEpoch_; }

  /**
   * The moment the session counters were last cleared, or 0 when it happened before the clock was set.
   *
   * Zero is a real answer and must stay distinguishable: a device whose session began while its clock was
   * untrusted genuinely cannot say when that was, and "unknown" is the honest rendering. Guessing the
   * boot time would be a fabricated timestamp on the one page whose whole subject is elapsed volume.
   */
  uint32_t sessionStartEpoch() const { return sessionStartEpoch_; }

  /** Records a session reset happening now. Call from wherever the session counters are cleared. */
  void noteSessionStart(uint32_t nowMs);

  /**
   * Seconds the session has been running, or 0 when that cannot be known.
   *
   * Derived from the epochs rather than kept as a counter, so it cannot disagree with the timestamp shown
   * beside it.
   */
  uint32_t sessionDurationSeconds(uint32_t nowMs) const;

 private:
  ClockSource source_ = ClockSource::None;
  /** Epoch at the moment `baseMs_` was taken. */
  uint32_t baseEpoch_ = 0;
  uint32_t baseMs_ = 0;
  uint32_t lastSyncEpoch_ = 0;
  uint32_t sessionStartEpoch_ = 0;
  /** Set when a session reset happened while the clock was unset, so it can be dated on the next sync. */
  bool sessionStartUnknown_ = false;
};

/**
 * Unix epoch from a UTC civil date, or 0 when the fields are not a real date.
 *
 * Written out rather than delegated, and it lives HERE rather than beside the I2C read so that it can be
 * tested: `mktime` applies the local zone, which on a device with no zone configured is a silent offset
 * that changes with the season, and `timegm` — the correct function — is absent from this toolchain's
 * newlib. Fifteen lines of civil-calendar arithmetic with no zone, no locale and no environment is the
 * remaining option, and date arithmetic without a test is how a leap-year off-by-one ships.
 *
 * `year` is the full year (2026), `month` is 1..12, unlike `struct tm`'s offsets.
 */
uint32_t epochFromUtcCivil(int year, int month, int day, int hour, int minute, int second);

/** True for an epoch this device is willing to display. */
bool clockEpochPlausible(uint32_t epoch);

}  // namespace plc
