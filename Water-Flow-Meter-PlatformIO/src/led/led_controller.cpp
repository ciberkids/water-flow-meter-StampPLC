#include "led/led_controller.h"

#include <Preferences.h>

#include <algorithm>

#include <M5StamPLC.h>

namespace {
constexpr float kRedDutyCycle = 0.4f;
constexpr char kPrefKeyStep[] = "led_step";
constexpr char kPrefKeyPeriod[] = "led_period";
}  // namespace

void LedController::begin() {
  clampConfig();
  resetPulseState();
  applyOutputs(false, false, false);
}

void LedController::loadFromPreferences(Preferences& prefs) {
  volumeStepLiters_ = prefs.getUShort(kPrefKeyStep, kDefaultVolumeStep);
  pulsePeriodMs_ = prefs.getUShort(kPrefKeyPeriod, kDefaultPulseMs);
  clampConfig();
}

void LedController::saveToPreferences(Preferences& prefs) const {
  prefs.putUShort(kPrefKeyStep, volumeStepLiters_);
  prefs.putUShort(kPrefKeyPeriod, pulsePeriodMs_);
}

void LedController::setVolumeStepLiters(uint16_t stepLiters) {
  if (stepLiters == 0) {
    stepLiters = kDefaultVolumeStep;
  }
  volumeStepLiters_ = stepLiters;
  resetPulseState();
}

void LedController::setPulsePeriodMs(uint16_t periodMs) {
  pulsePeriodMs_ = std::clamp<uint16_t>(periodMs, kMinPulseMs, kMaxPulseMs);
}

void LedController::setSuspended(bool suspended) {
  if (suspended_ == suspended) {
    return;
  }
  suspended_ = suspended;
  if (suspended_) {
    applyOutputs(false, false, false);
  }
}

void LedController::resetToDefaults() {
  volumeStepLiters_ = kDefaultVolumeStep;
  pulsePeriodMs_ = kDefaultPulseMs;
  markSessionsCleared();
}

void LedController::markSessionsCleared() {
  lastTotalLiters_ = 0.0;
  accumLiters_ = 0.0;
  resetPulseState();
}

void LedController::beginBoot(uint32_t nowMs) {
  override_ = plc::LedOverride::Boot;
  bootStartMs_ = nowMs;
}

void LedController::noteReady() {
  if (override_ == plc::LedOverride::Boot || override_ == plc::LedOverride::BootStalled) {
    override_ = plc::LedOverride::None;
  }
}

void LedController::setResetRamp(uint32_t remainingMs, uint32_t totalMs) {
  // An accepted reset outranks the ramp: once solid white is showing, a late countdown
  // update must not drop it back to flashing.
  if (override_ == plc::LedOverride::ResetAccepted) {
    return;
  }
  override_ = plc::LedOverride::ResetRamp;
  rampRemainingMs_ = remainingMs;
  rampTotalMs_ = totalMs;
}

void LedController::noteResetAccepted(uint32_t nowMs, uint32_t holdMs) {
  override_ = plc::LedOverride::ResetAccepted;
  acceptedUntilMs_ = nowMs + holdMs;
}

void LedController::clearResetRamp() {
  if (override_ == plc::LedOverride::ResetRamp) {
    override_ = plc::LedOverride::None;
  }
}

void LedController::update(uint32_t nowMs,
                           double totalSessionLiters,
                           double aggregateFlowLpm,
                           bool allSensorsReady,
                           bool hasUndersampling) {
  clampConfig();

  const double deltaLiters = totalSessionLiters - lastTotalLiters_;
  if (deltaLiters > 0.0) {
    accumLiters_ += deltaLiters;
  }
  // Allow the baseline to track resets without discarding accumulated pulses.
  lastTotalLiters_ = totalSessionLiters;

  // Handle red pulse accumulation
  if (volumeStepLiters_ > 0) {
    while (!pulseActive_ && accumLiters_ >= volumeStepLiters_) {
      accumLiters_ -= volumeStepLiters_;
      pulseActive_ = true;
      pulseStartMs_ = nowMs;
    }
  }

  bool redOn = false;
  if (pulseActive_) {
    uint32_t elapsed = nowMs - pulseStartMs_;
    if (elapsed >= pulsePeriodMs_) {
      pulseActive_ = false;
      redOn = false;
    } else {
      redOn = elapsed < static_cast<uint32_t>(pulsePeriodMs_ * kRedDutyCycle);
    }
  }

  // Green channel reflects configuration health
  bool greenOn = allSensorsReady && !hasUndersampling;

  // Blue channel indicates live flow
  if (aggregateFlowLpm > 0.0f) {
    lastFlowTimestampMs_ = nowMs;
  }
  bool blueOn = false;
  if ((nowMs - lastFlowTimestampMs_) <= kBlueHoldMs) {
    blueOn = ((nowMs / kBlueBlinkIntervalMs) % 2) == 0;
  }

  // Overrides displace the channel semantics above for their duration (§3.5).
  switch (override_) {
    case plc::LedOverride::Boot: {
      const uint32_t elapsed = nowMs - bootStartMs_;
      if (elapsed >= plc::kBootStallMs) {
        override_ = plc::LedOverride::BootStalled;
      }
      const plc::LedState state = (override_ == plc::LedOverride::BootStalled)
                                      ? plc::bootStalledState(elapsed)
                                      : plc::bootSnakeState(elapsed);
      applyOutputs(state.red, state.green, state.blue);
      return;
    }
    case plc::LedOverride::BootStalled: {
      const plc::LedState state = plc::bootStalledState(nowMs - bootStartMs_);
      applyOutputs(state.red, state.green, state.blue);
      return;
    }
    case plc::LedOverride::CardBusy: {
      // The SD card holds the shared SPI bus, so the display cannot report anything and the LEDs
      // are the only channel left. Held until the arbiter says the handover is over — unlike the
      // other overrides this one has no duration of its own, because a card read does not.
      const plc::LedState state = plc::cardBusyState(nowMs - cardBusyStartMs_);
      applyOutputs(state.red, state.green, state.blue);
      return;
    }
    case plc::LedOverride::ResetRamp: {
      const plc::LedState state = plc::resetRampState(nowMs, rampRemainingMs_, rampTotalMs_);
      applyOutputs(state.red, state.green, state.blue);
      return;
    }
    case plc::LedOverride::ResetAccepted: {
      if (static_cast<int32_t>(nowMs - acceptedUntilMs_) >= 0) {
        override_ = plc::LedOverride::None;
        break;
      }
      const plc::LedState state = plc::resetAcceptedState();
      applyOutputs(state.red, state.green, state.blue);
      return;
    }
    case plc::LedOverride::None:
      break;
  }

  if (suspended_) {
    applyOutputs(false, false, false);
    return;
  }

  applyOutputs(redOn, greenOn, blueOn);
}

void LedController::applyOutputs(bool redOn, bool greenOn, bool blueOn) {
  // ── Dirty check. This is a measurement-accuracy fix, not a tidiness one. ──────────
  //
  // setStatusLight writes the PI4IOE5V6408 expander at 0x43, and that sits on the SAME I2C bus
  // (SCL 15 / SDA 13, one driver mutex) as the AW9523B at 0x59 that readPlcInput reads the flow
  // sensors through. The sensor "polling" is therefore I2C traffic, not GPIO reads — there is no
  // pin, which is also why hardware pulse counting is unavailable on this board.
  //
  // update() runs every pass of the logic loop, which ends in vTaskDelay(1) at 1000 Hz, and this
  // function used to write unconditionally. So core 1 was issuing a read-modify-write to the LED
  // expander roughly a thousand times a second on the bus core 0 depends on for every sample.
  //
  // The steady state is overwhelmingly "the LEDs are already showing the right thing": green
  // solid, blue idle, red between pulses. Writing only on a change removes almost all of that
  // traffic while leaving the boot snake, the reset ramp and the card-busy pattern — which
  // genuinely change state — completely unaffected.
  //
  // This matters for R2.1.1's 5 % budget as much as for accuracy: it must land BEFORE the
  // radio-off baseline is recorded, or a post-fix radio-ON measurement could beat a pre-fix
  // radio-OFF one and the comparison would flatter the radio.
  if (outputsInitialised_ && redOn == lastRed_ && greenOn == lastGreen_ && blueOn == lastBlue_) {
    return;
  }
  lastRed_ = redOn;
  lastGreen_ = greenOn;
  lastBlue_ = blueOn;
  outputsInitialised_ = true;
  M5StamPLC.setStatusLight(redOn ? 255 : 0, greenOn ? 255 : 0, blueOn ? 255 : 0);
}

void LedController::clampConfig() {
  if (volumeStepLiters_ == 0) {
    volumeStepLiters_ = kDefaultVolumeStep;
  } else if (volumeStepLiters_ != 1 && volumeStepLiters_ != 10 && volumeStepLiters_ != 100) {
    // Snap to nearest supported value
    if (volumeStepLiters_ < 5) {
      volumeStepLiters_ = 1;
    } else if (volumeStepLiters_ < 50) {
      volumeStepLiters_ = 10;
    } else {
      volumeStepLiters_ = 100;
    }
  }
  pulsePeriodMs_ = std::clamp<uint16_t>(pulsePeriodMs_, kMinPulseMs, kMaxPulseMs);
}

void LedController::resetPulseState() {
  pulseActive_ = false;
  pulseStartMs_ = 0;
}

void LedController::setCardBusy(uint32_t nowMs) {
  if (override_ != plc::LedOverride::CardBusy) {
    override_ = plc::LedOverride::CardBusy;
    cardBusyStartMs_ = nowMs;
  }
}

void LedController::clearCardBusy() {
  if (override_ == plc::LedOverride::CardBusy) {
    override_ = plc::LedOverride::None;
  }
}
