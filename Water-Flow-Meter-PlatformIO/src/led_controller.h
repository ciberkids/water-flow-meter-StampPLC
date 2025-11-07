#pragma once

#include <cstdint>

#include <Preferences.h>

class LedController {
 public:
  void begin();

  void loadFromPreferences(Preferences& prefs);
  void saveToPreferences(Preferences& prefs) const;

  void setVolumeStepLiters(uint16_t stepLiters);
  void setPulsePeriodMs(uint16_t periodMs);

  uint16_t volumeStepLiters() const { return volumeStepLiters_; }
  uint16_t pulsePeriodMs() const { return pulsePeriodMs_; }

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
};
