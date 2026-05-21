#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

/**
 * ButtonInputManager polls the StampPLC hardware buttons (UP/DOWN/ENTER)
 * and emits debounced events for short presses, long presses, and long-hold
 * repeats. It is designed to run from the application logic task where
 * other UI state transitions are processed.
 */
class ButtonInputManager {
 public:
  enum class Button { Up = 0, Down = 1, Enter = 2 };

  struct ButtonEvent {
    Button button = Button::Up;
    bool isLongPress = false;
    bool isRepeat = false;
  };

  void begin(uint32_t nowMs);
  void update(uint32_t nowMs);

  bool popEvent(ButtonEvent* event);
  void clearEvents();

  bool isPressed(Button button) const;
  uint32_t pressedDuration(Button button, uint32_t nowMs) const;

 private:
  static constexpr uint32_t kLongPressThresholdMs = 1500;
  static constexpr uint32_t kRepeatIntervalMs = 250;
  static constexpr std::size_t kEventCapacity = 8;

  struct ButtonState {
    bool pressed = false;
    uint32_t pressStartMs = 0;
    uint32_t lastRepeatMs = 0;
    bool longPressSent = false;
  };

  void pushEvent(Button button, bool isLongPress, bool isRepeat);
  bool readHardware(Button button) const;
  std::size_t index(Button button) const {
    return static_cast<std::size_t>(button);
  }

  std::array<ButtonState, 3> states_{};
  std::array<ButtonEvent, kEventCapacity> events_{};
  std::size_t head_ = 0;
  std::size_t tail_ = 0;
  std::size_t size_ = 0;
};

