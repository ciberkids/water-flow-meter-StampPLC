#include "button_input.h"

#include <M5StamPLC.h>

void ButtonInputManager::begin(uint32_t nowMs) {
  clearEvents();
  for (std::size_t i = 0; i < states_.size(); ++i) {
    states_[i] = ButtonState{};
    const Button button = static_cast<Button>(i);
    const bool pressed = readHardware(button);
    states_[i].pressed = pressed;
    states_[i].pressStartMs = nowMs;
    states_[i].lastRepeatMs = nowMs;
    states_[i].longPressSent = false;
  }
}

void ButtonInputManager::update(uint32_t nowMs) {
  for (std::size_t i = 0; i < states_.size(); ++i) {
    const Button button = static_cast<Button>(i);
    const bool pressed = readHardware(button);
    auto& state = states_[i];

    if (pressed && !state.pressed) {
      // Transition to pressed.
      state.pressed = true;
      state.pressStartMs = nowMs;
      state.lastRepeatMs = nowMs;
      state.longPressSent = false;
      continue;
    }

    if (!pressed && state.pressed) {
      // Released.
      if (!state.longPressSent) {
        pushEvent(button, false, false);
      }
      state.pressed = false;
      state.longPressSent = false;
      continue;
    }

    if (!pressed) {
      continue;
    }

    const uint32_t heldMs = nowMs - state.pressStartMs;
    if (!state.longPressSent && heldMs >= kLongPressThresholdMs) {
      state.longPressSent = true;
      state.lastRepeatMs = nowMs;
      pushEvent(button, true, false);
      continue;
    }

    if (state.longPressSent && button != Button::Enter) {
      if (nowMs - state.lastRepeatMs >= kRepeatIntervalMs) {
        state.lastRepeatMs = nowMs;
        pushEvent(button, true, true);
      }
    }
  }
}

bool ButtonInputManager::popEvent(ButtonEvent* event) {
  if (!event || size_ == 0) {
    return false;
  }
  *event = events_[head_];
  head_ = (head_ + 1) % kEventCapacity;
  --size_;
  return true;
}

void ButtonInputManager::clearEvents() {
  head_ = 0;
  tail_ = 0;
  size_ = 0;
}

bool ButtonInputManager::isPressed(Button button) const {
  return states_[index(button)].pressed;
}

uint32_t ButtonInputManager::pressedDuration(Button button, uint32_t nowMs) const {
  const auto& state = states_[index(button)];
  if (!state.pressed) {
    return 0;
  }
  return nowMs >= state.pressStartMs ? (nowMs - state.pressStartMs) : 0;
}

void ButtonInputManager::pushEvent(Button button, bool isLongPress, bool isRepeat) {
  if (size_ == kEventCapacity) {
    return;
  }
  events_[tail_] = ButtonEvent{button, isLongPress, isRepeat};
  tail_ = (tail_ + 1) % kEventCapacity;
  ++size_;
}

bool ButtonInputManager::readHardware(Button button) const {
  switch (button) {
    case Button::Up:
      return M5StamPLC.BtnA.isPressed();
    case Button::Down:
      return M5StamPLC.BtnB.isPressed();
    case Button::Enter:
      return M5StamPLC.BtnC.isPressed();
    default:
      return false;
  }
}

