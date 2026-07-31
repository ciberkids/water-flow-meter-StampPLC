#include "ui/core/ui_actions.h"

#include <cstring>

#include <Preferences.h>

#include "modbus/modbus_manager.h"
#include "modbus/register_map.h"

namespace ui {

namespace {

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

void handleEnterConfiguration(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.setMode(UiMode::Configuration, ctx.nowMs);
}

void handleEnterInfo(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.setMode(UiMode::Info, ctx.nowMs);
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
  if (ctx.controller.navigator().descend(ctx.resolvedTarget)) {
    ctx.controller.syncPageFromScreen(ctx.controller.navigator().current(), ctx.nowMs);
  }
}

void handleNavBack(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.notifyInteraction(ctx.nowMs);
  if (ctx.controller.navigator().ascend()) {
    ctx.controller.syncPageFromScreen(ctx.controller.navigator().current(), ctx.nowMs);
  }
}

void handleNavEscape(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.navigator().escape();
  ctx.controller.setMode(UiMode::Info, ctx.nowMs);
  ctx.controller.syncPageFromScreen(ctx.controller.navigator().current(), ctx.nowMs);
}

const UiActionBinding kDefaultBindings[] = {
    {"ui.action.page.next", handlePageNext},
    {"ui.action.page.previous", handlePagePrevious},
    {"ui.action.mode.configuration", handleEnterConfiguration},
    {"ui.action.mode.info", handleEnterInfo},
    {"ui.action.mode.idle", handleEnterIdle},
    {"core.action.save-config", handleSaveConfig},
    {"core.action.reset-session", handleResetSession},
    {"core.action.reset-all-measured", handleResetAllMeasured},
    {"core.action.factory-reset", handleFactoryReset},
    {"ui.action.nav.descend", handleNavDescend},
    {"ui.action.nav.back", handleNavBack},
    {"ui.action.nav.escape", handleNavEscape},
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
