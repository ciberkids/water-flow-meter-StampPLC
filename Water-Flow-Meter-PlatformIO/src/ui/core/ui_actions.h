#pragma once

#include <cstddef>
#include <cstdint>

#include "ui/core/ui_controller.h"
#include "ui/core/ui_settings.h"
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
  /**
   * Full factory reset (clears NVS, Modbus config and reboots). Owned by
   * firmware.cpp because it touches every subsystem, so it reaches actions as a
   * callback rather than through one of the references above. May be null.
   */
  void (*factoryReset)() = nullptr;
  /**
   * The screen this flow's `targetScreenId` resolves to, or null.
   *
   * InteractionHandler resolves it before dispatch so the action handlers stay dumb
   * and need no router pointer of their own.
   */
  /** Live read/write access to the settings catalogue. May be null. */
  const SettingsAccess* settings = nullptr;
  const ui_exporter::Screen* resolvedTarget = nullptr;
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

