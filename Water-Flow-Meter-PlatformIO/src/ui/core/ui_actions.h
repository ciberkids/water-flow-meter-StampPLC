#pragma once

#include <cstddef>
#include <cstdint>

#include "ui/core/ui_controller.h"
#include "ui/generated/GeneratedUi.h"

class ModbusManager;
class LedController;
class Preferences;

namespace ui {

struct UiActionContext {
  UiController& controller;
  ModbusManager& modbus;
  LedController& leds;
  Preferences& preferences;
  uint32_t nowMs = 0;
};

using UiActionFn = void (*)(const UiActionContext& ctx, const ui_exporter::Flow& flow);

struct UiActionBinding {
  const char* actionId;
  UiActionFn handler;
};

class UiActionRegistry {
 public:
  UiActionRegistry(const UiActionBinding* bindings, std::size_t count);

  bool dispatch(const char* actionId,
                const UiActionContext& context,
                const ui_exporter::Flow& flow) const;

 private:
  const UiActionBinding* bindings_ = nullptr;
  std::size_t count_ = 0;
};

const UiActionRegistry& defaultActionRegistry();

}  // namespace ui

