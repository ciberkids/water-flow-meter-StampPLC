#include "bus/spi_arbiter.h"

namespace plc {

const char* busStateText(BusState state) {
  switch (state) {
    case BusState::DisplayOwns:     return "display";
    case BusState::CardRequested:   return "card requested";
    case BusState::CardOwns:        return "card";
    case BusState::DisplayResuming: return "display resuming";
  }
  return "unknown";
}

void SpiArbiter::noteFrameBegan(uint32_t nowMs) {
  (void)nowMs;
  // A caller that ignores mayBeginFrame() is a bug, but it must not corrupt the arbiter's idea
  // of whether a frame is open — so the flag is set regardless and the state is left alone.
  frameOpen_ = true;
  if (state_ == BusState::DisplayResuming) {
    state_ = BusState::DisplayOwns;
  }
}

void SpiArbiter::noteFrameEnded(uint32_t nowMs) {
  frameOpen_ = false;
  // The frame boundary is the ONLY point at which the card may take over, which is what makes a
  // torn frame impossible rather than unlikely.
  if (state_ == BusState::CardRequested) {
    (void)nowMs;
    grantCard();
  }
}

bool SpiArbiter::consumeFullRepaintRequest() {
  const bool owed = fullRepaintOwed_;
  fullRepaintOwed_ = false;
  return owed;
}

bool SpiArbiter::requestCard(uint32_t nowMs) {
  if (state_ == BusState::CardOwns) {
    return true;  // already granted; idempotent
  }
  if (state_ == BusState::CardRequested) {
    return false;  // still waiting on a frame
  }

  state_ = BusState::CardRequested;
  requestedAtMs_ = nowMs;

  // At boot no frame has ever been opened, so the handover completes immediately and there is no
  // contention at all. This is the case §4.5 asks the firmware to stay in wherever possible.
  if (!frameOpen_) {
    grantCard();
    return true;
  }
  return false;
}

void SpiArbiter::releaseCard(uint32_t nowMs) {
  (void)nowMs;
  if (state_ != BusState::CardOwns && state_ != BusState::CardRequested) {
    return;
  }
  state_ = BusState::DisplayResuming;
  // Whatever is on the panel now describes state from before the handover, by an unknown amount.
  // An incremental update would leave that stale content visible, so a full repaint is owed.
  fullRepaintOwed_ = true;
}

void SpiArbiter::update(uint32_t nowMs) {
  if (state_ != BusState::CardRequested) {
    return;
  }
  if (!frameOpen_) {
    grantCard();
    return;
  }
  if (nowMs - requestedAtMs_ >= kFrameWaitTimeoutMs) {
    // The renderer has not closed a frame in half a second, so it is not drawing — it is stuck.
    // Taking the bus cannot tear anything that is not progressing, and refusing forever would
    // mean one wedged renderer permanently blocks menu selection and every future card read.
    ++timeouts_;
    frameOpen_ = false;
    grantCard();
  }
}

void SpiArbiter::grantCard() {
  state_ = BusState::CardOwns;
  ++handovers_;
}

}  // namespace plc
