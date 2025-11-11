#include "ui/core/ui_actions.h"

#include <cstring>

#include <Preferences.h>

namespace ui {

namespace {

void handlePageNext(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.nextPage(ctx.nowMs);
}

void handlePagePrevious(const UiActionContext& ctx, const ui_exporter::Flow&) {
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

const UiActionBinding kDefaultBindings[] = {
    {"ui.action.page.next", handlePageNext},
    {"ui.action.page.previous", handlePagePrevious},
    {"ui.action.mode.configuration", handleEnterConfiguration},
    {"ui.action.mode.info", handleEnterInfo},
    {"ui.action.mode.idle", handleEnterIdle},
    {"core.action.save-config", handleSaveConfig},
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
