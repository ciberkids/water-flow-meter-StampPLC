#include "ui/core/ui_actions.h"

#include <cstring>

#include <Preferences.h>

#include "modbus/modbus_manager.h"
#include "modbus/register_map.h"

namespace ui {

namespace {

/** The settable binding declared on a screen, if any — this is how descending into
 *  an editor discovers which setting it edits, without a screen-id-to-setting table. */
const SettingDescriptor* settingOnScreen(const ui_exporter::Screen* screen) {
  if (!screen) {
    return nullptr;
  }
  for (std::size_t i = 0; i < screen->elementCount; ++i) {
    if (const auto* found = findSetting(screen->elements[i].bindingId)) {
      return found;
    }
  }
  return nullptr;
}

// A sibling move: follow the flow's target without touching depth. Falls back to
// the UiPage ring when the dataset names no target, so an under-specified screen
// still pages.
void handlePageNext(const UiActionContext& ctx, const ui_exporter::Flow&) {
  if (ctx.resolvedTarget) {
    ctx.controller.navigator().goToSibling(ctx.resolvedTarget);
    ctx.controller.syncPageFromScreen(ctx.resolvedTarget, ctx.nowMs);
    return;
  }
  ctx.controller.nextPage(ctx.nowMs);
}

void handlePagePrevious(const UiActionContext& ctx, const ui_exporter::Flow&) {
  if (ctx.resolvedTarget) {
    ctx.controller.navigator().goToSibling(ctx.resolvedTarget);
    ctx.controller.syncPageFromScreen(ctx.resolvedTarget, ctx.nowMs);
    return;
  }
  ctx.controller.previousPage(ctx.nowMs);
}

void handleEnterIdle(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.enterIdle(ctx.nowMs);
}

void handleSaveConfig(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.notifyInteraction(ctx.nowMs);
  ctx.leds.saveToPreferences(ctx.preferences);
}

// Display_UI_Requirements §4.3 note 3: a completed reset must issue the matching
// Modbus command so persisted state stays coherent, rather than mutating sensor
// structs directly. Both go through the documented global command registers.
void handleResetSession(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.notifyInteraction(ctx.nowMs);
  ctx.modbus.applyHoldingWrite(plc::REG_MASTER_RESET_ALL_SESSION, 1);
}

void handleResetAllMeasured(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.notifyInteraction(ctx.nowMs);
  ctx.modbus.applyHoldingWrite(plc::REG_MASTER_RESET_ALL_MEASURED, 1);
}

void handleFactoryReset(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.notifyInteraction(ctx.nowMs);
  if (ctx.factoryReset) {
    ctx.factoryReset();
  }
}

// Display_UI_Requirements §3.2: ENTER-short descends, ENTER-long escapes to P0,
// BACK ascends one level. UP/DOWN move within a level, which is a sibling move and
// must not change depth.
void handleNavDescend(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.notifyInteraction(ctx.nowMs);
  if (!ctx.resolvedTarget) {
    return;
  }
  if (!ctx.controller.navigator().descend(ctx.resolvedTarget)) {
    return;
  }
  ctx.controller.syncPageFromScreen(ctx.controller.navigator().current(), ctx.nowMs);

  // Entering a screen that declares a settable binding opens the editor on it. The
  // setting is discovered from the screen's own bindings, so no screen-id-to-setting
  // table has to be kept in step with the dataset.
  if (const auto* setting = settingOnScreen(ctx.resolvedTarget)) {
    const uint8_t sensor = ctx.controller.navigator().sensorIndex();
    const int32_t current =
        ctx.settings ? readSetting(*setting, sensor, *ctx.settings) : 0;
    ctx.controller.beginEdit(setting, sensor, current);
  } else {
    ctx.controller.endEdit();
  }
}

void handleNavBack(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.notifyInteraction(ctx.nowMs);
  ctx.controller.endEdit();
  if (ctx.controller.navigator().ascend()) {
    ctx.controller.syncPageFromScreen(ctx.controller.navigator().current(), ctx.nowMs);
  }
}

void handleNavEscape(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.endEdit();
  ctx.controller.navigator().escape();
  ctx.controller.setMode(UiMode::Info, ctx.nowMs);
  ctx.controller.syncPageFromScreen(ctx.controller.navigator().current(), ctx.nowMs);
}

void handleValueIncrement(const UiActionContext& ctx, const ui_exporter::Flow&) {
  const auto& editor = ctx.controller.editor();
  ctx.controller.adjustEdit(editor.setting ? editor.setting->step : 1, ctx.nowMs);
}

void handleValueDecrement(const UiActionContext& ctx, const ui_exporter::Flow&) {
  const auto& editor = ctx.controller.editor();
  ctx.controller.adjustEdit(-(editor.setting ? editor.setting->step : 1), ctx.nowMs);
}

/**
 * §5.5: clamp, write, validate, then ascend.
 *
 * A rejected sensor write means the Nyquist check failed, so the editor stays open
 * with the prompt raised rather than ascending — otherwise the operator would be
 * returned to the previous screen with no indication their value was refused.
 */
void handleValueCommit(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.notifyInteraction(ctx.nowMs);
  const auto& editor = ctx.controller.editor();
  if (!editor.active || !editor.setting || !ctx.settings) {
    return;
  }
  if (!writeSetting(*editor.setting, editor.sensorIndex, editor.pending, *ctx.settings)) {
    ctx.controller.setNyquistPrompt(true);
    return;
  }
  ctx.controller.endEdit();
  if (ctx.controller.navigator().ascend()) {
    ctx.controller.syncPageFromScreen(ctx.controller.navigator().current(), ctx.nowMs);
  }
}

/**
 * DOWN on the Nyquist prompt. The second write is what the existing two-write
 * override protocol in ModbusManager::prepareConfigUpdate treats as the override, so
 * this deliberately repeats the same call rather than adding a bypass.
 */
void handleValueCommitOverride(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.notifyInteraction(ctx.nowMs);
  const auto& editor = ctx.controller.editor();
  if (!editor.active || !editor.setting || !ctx.settings) {
    return;
  }
  writeSetting(*editor.setting, editor.sensorIndex, editor.pending, *ctx.settings);
  ctx.controller.endEdit();
  if (ctx.controller.navigator().ascend()) {
    ctx.controller.syncPageFromScreen(ctx.controller.navigator().current(), ctx.nowMs);
  }
}

void handleValueDiscard(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.notifyInteraction(ctx.nowMs);
  ctx.controller.endEdit();
  if (ctx.controller.navigator().ascend()) {
    ctx.controller.syncPageFromScreen(ctx.controller.navigator().current(), ctx.nowMs);
  }
}

const UiActionBinding kDefaultBindings[] = {
    {"ui.action.page.next", handlePageNext},
    {"ui.action.page.previous", handlePagePrevious},
    {"ui.action.mode.idle", handleEnterIdle},
    {"core.action.save-config", handleSaveConfig},
    {"core.action.reset-session", handleResetSession},
    {"core.action.reset-all-measured", handleResetAllMeasured},
    {"core.action.factory-reset", handleFactoryReset},
    {"ui.action.nav.descend", handleNavDescend},
    {"ui.action.nav.back", handleNavBack},
    {"ui.action.nav.escape", handleNavEscape},
    {"config.action.value.increment", handleValueIncrement},
    {"config.action.value.decrement", handleValueDecrement},
    {"config.action.value.commit", handleValueCommit},
    {"config.action.value.commit-override", handleValueCommitOverride},
    {"config.action.value.discard", handleValueDiscard},
};

UiActionRegistry initDefaultRegistry() {
  return UiActionRegistry(kDefaultBindings, sizeof(kDefaultBindings) / sizeof(kDefaultBindings[0]));
}

}  // namespace

UiActionRegistry::UiActionRegistry(const UiActionBinding* bindings, std::size_t count)
    : bindings_(bindings), count_(count) {}

bool UiActionRegistry::dispatch(const char* actionId,
                                const UiActionContext& context,
                                const ui_exporter::Flow& flow) const {
  if (!actionId || !bindings_) {
    return false;
  }
  for (std::size_t i = 0; i < count_; ++i) {
    const auto& entry = bindings_[i];
    if (entry.actionId && std::strcmp(entry.actionId, actionId) == 0) {
      if (entry.handler) {
        entry.handler(context, flow);
        return true;
      }
      return false;
    }
  }
  return false;
}

const UiActionRegistry& defaultActionRegistry() {
  static UiActionRegistry registry = initDefaultRegistry();
  return registry;
}

}  // namespace ui
