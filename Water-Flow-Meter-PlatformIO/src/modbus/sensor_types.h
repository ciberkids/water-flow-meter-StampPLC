#pragma once

#include <cstdint>

struct SensorCharacteristics {
  uint16_t q_max = 0;
  int16_t f_multiplier = 0;
  int16_t adjust = 0;

  bool operator==(const SensorCharacteristics& other) const {
    return q_max == other.q_max && f_multiplier == other.f_multiplier && adjust == other.adjust;
  }

  bool operator!=(const SensorCharacteristics& other) const {
    return !(*this == other);
  }
};

struct SensorData {
  bool inUse = false;
  bool isReady = false;
  volatile uint32_t pulseCount = 0;
  float instantFlow_L_s = 0.0f;
  double cumulativeLiters = 0.0;
  float sessionLiters = 0.0f;
  float maxFlowSinceReset = 0.0f;
};
