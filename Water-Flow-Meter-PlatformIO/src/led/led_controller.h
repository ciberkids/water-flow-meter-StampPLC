#pragma once

#include <cstdint>

// Forward-declared rather than included: this header only takes Preferences by reference,
// and pulling the Arduino header in makes the whole UI layer impossible to compile on a
// host. Same idiom as ui/core/ui_actions.h.
class Preferences;

#include "led/led_patterns.h"

class LedController {
 public:
  void begin();

  void loadFromPreferences(Preferences& prefs);
  void saveToPreferences(Preferences& prefs) const;

  void setVolumeStepLiters(uint16_t stepLiters);
  void setPulsePeriodMs(uint16_t periodMs);

  uint16_t volumeStepLiters() const { return volumeStepLiters_; }
  uint16_t pulsePeriodMs() const { return pulsePeriodMs_; }

  /**
   * §3.4: start the boot pattern. Runs until noteReady(), degrading to a red blink
   * after kBootStallMs so a hang looks different from a slow start.
   */
  void beginBoot(uint32_t nowMs);
  /** §3.4: initialisation finished; hand back to the normal channel semantics. */
  void noteReady();

  /**
   * §3.5: drive the accelerating white flash of a reset countdown.
   *
   * Replaces the old "all channels off during the countdown", which gave the most
   * destructive operation in the system the least indication.
   */
  void setResetRamp(uint32_t remainingMs, uint32_t totalMs);
  /** §3.5: solid white for `holdMs`, the signal that a reset was accepted. */
  void noteResetAccepted(uint32_t nowMs, uint32_t holdMs);

  /**
   * The card has taken the shared SPI bus, so the LEDs must carry the status (§3.4/§3.5
   * vocabulary; see bus/spi_arbiter.h). Held until clearCardBusy().
   */
  void setCardBusy(uint32_t nowMs);
  void clearCardBusy();
  /** §3.5: countdown aborted — stop with no white flash, so it cannot read as done. */
  void clearResetRamp();

  void setSuspended(bool suspended);
  bool isSuspended() const { return suspended_; }

  void resetToDefaults();
  void markSessionsCleared();

  void update(uint32_t nowMs,
              double totalSessionLiters,
              double aggregateFlowLps,
              bool allSensorsReady,
              bool hasUndersampling);

 private:
  static constexpr uint16_t kDefaultVolumeStep = 1;
  static constexpr uint16_t kDefaultPulseMs = 500;
  static constexpr uint16_t kMinPulseMs = 100;
  static constexpr uint16_t kMaxPulseMs = 2000;
  static constexpr uint32_t kBlueHoldMs = 500;
  static constexpr uint32_t kBlueBlinkIntervalMs = 250;

  void applyOutputs(bool redOn, bool greenOn, bool blueOn);
  void clampConfig();
  void resetPulseState();

  double lastTotalLiters_ = 0.0;
  double accumLiters_ = 0.0;
  uint16_t volumeStepLiters_ = kDefaultVolumeStep;
  uint16_t pulsePeriodMs_ = kDefaultPulseMs;
  bool pulseActive_ = false;
  uint32_t pulseStartMs_ = 0;
  uint32_t lastFlowTimestampMs_ = 0;
  bool suspended_ = false;

  plc::LedOverride override_ = plc::LedOverride::None;
  uint32_t bootStartMs_ = 0;
  uint32_t rampRemainingMs_ = 0;
  uint32_t rampTotalMs_ = 0;
  uint32_t acceptedUntilMs_ = 0;
  uint32_t cardBusyStartMs_ = 0;
};
