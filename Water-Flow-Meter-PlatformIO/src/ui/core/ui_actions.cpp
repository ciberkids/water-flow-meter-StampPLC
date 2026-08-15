#include "ui/core/ui_actions.h"

#include "ui/core/ui_action_catalogue.h"

#include <cstring>

#include <Preferences.h>

#include "modbus/modbus_manager.h"
#include "modbus/register_map.h"

namespace ui {

namespace {

/**
 * The binding an editor screen uses to render the value being edited.
 *
 * Display_UI_Requirements §5.4: a value editor shows "the pending value (highlighted) ...
 * and the currently saved value". Only an editor has a pending value, so this element is
 * what distinguishes an editor from the list page above it — see isValueEditorScreen.
 */
constexpr const char* kEditorPendingBinding = "config.editor.pending";

/**
 * Whether this screen IS a value editor (§5.4), as opposed to a list page that merely
 * displays the setting currently in force.
 *
 * The list pages C1..C6 and S1..S4 each show their own saved value, so "declares a settable
 * binding" does not distinguish the two — every list page declares one. What only an editor
 * declares is the pending value, and it is declared on exactly the ten `-edit` screens. That
 * is asserted over the whole generated table by editorDatasetInvariantTests in
 * test/host/interaction_test.cpp, so a dataset that stopped satisfying it fails the suite
 * instead of silently disabling editing.
 */
bool isValueEditorScreen(const ui_exporter::Screen* screen) {
  if (!screen) {
    return false;
  }
  for (std::size_t i = 0; i < screen->elementCount; ++i) {
    const char* binding = screen->elements[i].bindingId;
    if (binding && std::strcmp(binding, kEditorPendingBinding) == 0) {
      return true;
    }
  }
  return false;
}

/** The settable binding an editor screen edits — this is how descending into an editor
 *  discovers which setting it edits, without a screen-id-to-setting table.
 *
 *  Returns null for anything that is not a value editor. Without that guard a config list
 *  page matched on its own saved-value element, so ENTER-short onto the LIST opened an
 *  editor; because goToSibling never touches editor state, the editor then leaked along the
 *  whole sibling ring and handleEditorRepeat swallowed every UP/DOWN held past 250 ms,
 *  breaking the §5.1 paging through C1..C6 and S1..S4. */
const SettingDescriptor* settingEditedByScreen(const ui_exporter::Screen* screen) {
  if (!isValueEditorScreen(screen)) {
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
/**
 * Steps to the next reachable sibling, over any that are hidden.
 *
 * The flow's own target is the next sibling in the DATASET, which may be gated off — the calibration
 * branch hides Multiplier and Adjust on a pulses-calibrated channel. Following the target blindly
 * would land the operator on a screen the level no longer contains, which then draws rows for a
 * calibration form they are not using.
 */
void handlePageNext(const UiActionContext& ctx, const ui_exporter::Flow&) {
  if (ctx.resolvedTarget) {
    auto& nav = ctx.controller.navigator();
    const ui_exporter::Screen* target = ctx.resolvedTarget;
    if (!nav.screenVisible(target)) {
      target = nav.nextVisibleSibling(target);
    }
    if (target) {
      nav.goToSibling(target);
      ctx.controller.syncPageFromScreen(target, ctx.nowMs);
    }
    return;
  }
  ctx.controller.nextPage(ctx.nowMs);
}

/**
 * The same, backwards.
 *
 * There is no `previousVisibleSibling`: the ring closes, so walking FORWARD from a hidden screen
 * eventually arrives at the one before it. Going forward from the hidden target lands on the next
 * visible screen after it, which is not where UP should go — so this walks forward from the CURRENT
 * screen instead and stops at the last visible one before it comes back round.
 */
void handlePagePrevious(const UiActionContext& ctx, const ui_exporter::Flow&) {
  if (ctx.resolvedTarget) {
    auto& nav = ctx.controller.navigator();
    const ui_exporter::Screen* target = ctx.resolvedTarget;
    if (!nav.screenVisible(target)) {
      const ui_exporter::Screen* const start = nav.current();
      const ui_exporter::Screen* candidate = nav.nextVisibleSibling(start);
      const ui_exporter::Screen* previous = candidate;
      // Walk the whole visible ring; the last member before returning to `start` is the one UP wants.
      for (uint8_t hops = 0; candidate && candidate != start && hops < 16; ++hops) {
        previous = candidate;
        candidate = nav.nextVisibleSibling(candidate);
      }
      target = previous;
    }
    if (target) {
      nav.goToSibling(target);
      ctx.controller.syncPageFromScreen(target, ctx.nowMs);
    }
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

/**
 * Clears the peak, and only the peak (P4's own reset).
 *
 * P4 had no reset route at all: the peak was reachable only through the session or measured resets, both
 * of which destroy something persistent to get at a number that a power cycle clears for free.
 */
void handleResetMaxFlow(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.notifyInteraction(ctx.nowMs);
  ctx.modbus.applyHoldingWrite(plc::REG_MASTER_RESET_ALL_MAX, 1);
}

/**
 * Returns ONE channel's calibration to defaults — the meter swap.
 *
 * The scenario it serves: a broken sensor is replaced by one with different characteristics. Everything
 * the old meter measured was true when it measured it, so the totals stay and keep accumulating; only
 * the figures describing the meter go back to unset, which is what puts the channel back to `SET?` and
 * invites the new datasheet's numbers.
 *
 * THE CHANNEL IS THE ONE THE NAVIGATOR SELECTED, and the index arithmetic is the whole risk in this
 * function. `navigator().sensorIndex()` is ONE-BASED with 0 meaning "no sensor level was entered" —
 * `sensorIndexFromId` parses it out of a `config-sensor-<n>` id — while `sensorBaseAddress` takes a
 * ZERO-BASED slot. Subtracting without the guard would turn "no sensor" into channel 1 and silently
 * reset a channel the operator was not even looking at. The same off-by-one is why `sensorSlot()` in
 * ui_settings.cpp exists rather than being open-coded at each call.
 *
 * `sensorIndex_` is sticky across the descent, which is what makes this work from a confirm screen:
 * `UiNavigator::descend` only reassigns it when the screen being LEFT parses as a sensor, and clears it
 * only on ascending to depth 0 or escaping. So standing on `confirm-reset-calibration` — two levels
 * below `config-sensor-3` — it still reads 3.
 *
 * Routed through the documented command register rather than assigning `configs[n]` here, for the reason
 * §4.3 note 3 gives for the other resets: the manager owns the override state, the diagnostics flags and
 * the holding-register mirror that all have to move together.
 */
void handleResetCalibration(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.notifyInteraction(ctx.nowMs);
  const uint8_t sensor = ctx.controller.navigator().sensorIndex();
  if (sensor == 0 || sensor > plc::kNumSensors) {
    return;
  }
  const uint16_t base = plc::sensorBaseAddress(static_cast<std::size_t>(sensor - 1));
  ctx.modbus.applyHoldingWrite(base + plc::OFF_CMD_RESET_CALIBRATION, 1);
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
/**
 * R8.2a — restore the portal login to admin/admin.
 *
 * The menu half of the recovery. Deliberately NOT routed through the factory reset that used to be
 * the only way out of a forgotten portal password: this touches the two portal fields and nothing
 * else, so the operator keeps their cumulative volume and their per-sensor calibration.
 *
 * Guarded by a hold-to-confirm screen like every other destructive action, because it does lower the
 * device's security to a published default — but only for someone who is already standing at it.
 */
void handleResetPortalLogin(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.notifyInteraction(ctx.nowMs);
  if (ctx.settings && ctx.settings->net) {
    ctx.settings->net->resetPortalCredentials();
  }
}

void handleNavDescend(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.notifyInteraction(ctx.nowMs);
  if (!ctx.resolvedTarget) {
    return;
  }
  if (!ctx.controller.navigator().descend(ctx.resolvedTarget)) {
    return;
  }
  ctx.controller.syncPageFromScreen(ctx.controller.navigator().current(), ctx.nowMs);

  // Entering a value editor screen opens the editor on it. The setting is discovered from
  // the screen's own bindings, so no screen-id-to-setting table has to be kept in step with
  // the dataset. Descending onto anything else — a list page, a sensor list, a confirm
  // screen — closes any editor instead of opening one.
  if (const auto* setting = settingEditedByScreen(ctx.resolvedTarget)) {
    const uint8_t sensor = ctx.controller.navigator().sensorIndex();
    // Text settings are NOT editable here. There is no on-device text entry at all: §6.3 records
    // why the three-button character wheel was removed. A screen that displays an SSID is a
    // read-only row, so descending onto it must not open an editor that cannot accept input.
    if (setting->kind == ui::SettingKind::Text) {
      ctx.controller.endEdit();
    } else {
      const int32_t current =
          ctx.settings ? readSetting(*setting, sensor, *ctx.settings) : 0;
      ctx.controller.beginEdit(setting, sensor, current);
    }
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

/**
 * Long ENTER ascends ONE level, not all of them.
 *
 * It called `escape()`, which clears the whole stack and lands on P0 from any depth — so holding
 * ENTER three levels deep in the sensor settings threw the operator out to the status page, and they
 * had to walk back down to see the change they had just made. One level up is what a hold means
 * everywhere else in the tree; it is already what the editor's `hold=cancel` does. Repeated holds
 * still walk out to the top for anyone who wants that.
 *
 * `escape()` itself stays and keeps its meaning — the BtnA+BtnB display-off gesture resets
 * navigation to P0 by that path (§3.1), which is a different thing and still resets fully.
 *
 * `endEdit()` first, unconditionally: a hold over an open editor discards the pending value, which is
 * what the editor footer promises, and it must happen whether or not there is a level to ascend to.
 */
void handleNavEscape(const UiActionContext& ctx, const ui_exporter::Flow&) {
  ctx.controller.endEdit();
  ctx.controller.navigator().ascend();
  ctx.controller.setMode(UiMode::Info, ctx.nowMs);
  ctx.controller.syncPageFromScreen(ctx.controller.navigator().current(), ctx.nowMs);
}

void handleValueCommitOverride(const UiActionContext& ctx, const ui_exporter::Flow& flow);

/**
 * While a prompt is showing, UP and DOWN are reinterpreted.
 *
 * §5.5's prompt reads "UP = Edit again / DOWN = Save anyway", and it renders on the editor
 * screen itself via the config.sensor.nyquistWarning binding. But UP/DOWN on that screen were
 * still wired to +/-1, so the prompt instructed the operator to do something the buttons did
 * not do. Reinterpreting them here keeps it to one screen — no new screen id, so the menu-pack
 * completeness rule stays satisfied and no dataset change is needed.
 *
 * Returns true when the press was consumed by the prompt.
 */
bool consumedByPrompt(const UiActionContext& ctx, bool isUp) {
  const auto& editor = ctx.controller.editor();
  if (!editor.nyquistPrompt && !editor.commitFailed) {
    return false;
  }
  if (isUp || editor.commitFailed) {
    // UP = edit again. A non-Nyquist failure has no override, so DOWN dismisses too rather
    // than silently doing nothing.
    ctx.controller.setNyquistPrompt(false);
    ctx.controller.setCommitFailed(false);
    return true;
  }
  // DOWN with a Nyquist prompt showing = save anyway.
  handleValueCommitOverride(ctx, ui_exporter::Flow{});
  return true;
}

void handleValueIncrement(const UiActionContext& ctx, const ui_exporter::Flow&) {
  if (consumedByPrompt(ctx, true)) {
    return;
  }
  const auto& editor = ctx.controller.editor();
  ctx.controller.adjustEdit(editor.setting ? editor.setting->step : 1, ctx.nowMs);
}

void handleValueDecrement(const UiActionContext& ctx, const ui_exporter::Flow&) {
  if (consumedByPrompt(ctx, false)) {
    return;
  }
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
    // Which failure? A Nyquist refusal parks the write awaiting an override confirmation, and
    // is the ONLY one an override can resolve — so it is the only one that may offer
    // "DOWN = Save anyway" (§5.5). Everything else (no Modbus, a rejected link write, a bad
    // sensor index, a missing bitmap, an unhandled target) needs a different message, and used
    // to get this one.
    const bool nyquist =
        editor.sensorIndex > 0 &&
        ctx.modbus.nyquistOverridePending(static_cast<std::size_t>(editor.sensorIndex - 1));
    ctx.controller.setNyquistPrompt(nyquist);
    ctx.controller.setCommitFailed(!nyquist);
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

constexpr UiActionBinding kDefaultBindings[] = {
    {"ui.action.page.next", handlePageNext},
    {"ui.action.page.previous", handlePagePrevious},
    {"ui.action.mode.idle", handleEnterIdle},
    {"core.action.save-config", handleSaveConfig},
    {"core.action.reset-session", handleResetSession},
    {"core.action.reset-all-measured", handleResetAllMeasured},
    {"core.action.reset-max-flow", handleResetMaxFlow},
    {"core.action.reset-calibration", handleResetCalibration},
    {"core.action.factory-reset", handleFactoryReset},
    {"core.action.reset-portal-login", handleResetPortalLogin},
    {"ui.action.nav.descend", handleNavDescend},
    {"ui.action.nav.back", handleNavBack},
    {"ui.action.nav.escape", handleNavEscape},
    {"config.action.value.increment", handleValueIncrement},
    {"config.action.value.decrement", handleValueDecrement},
    {"config.action.value.commit", handleValueCommit},
    {"config.action.value.commit-override", handleValueCommitOverride},
    {"config.action.value.discard", handleValueDiscard},
};

constexpr std::size_t kBindingCount = sizeof(kDefaultBindings) / sizeof(kDefaultBindings[0]);

/**
 * The catalogue the design tool sees and the handler table the firmware dispatches through
 * must agree exactly. Eight actions were once advertised with no handler behind them, so a
 * designer could wire a button to nothing and the export still passed.
 */
static_assert(kBindingCount == kActionCatalogueCount,
              "kActionCatalogue (ui_action_catalogue.h) must have one entry per handler");

constexpr bool actionCatalogueMatchesHandlers() {
  for (std::size_t i = 0; i < kBindingCount; ++i) {
    if (!actionIdsEqual(kDefaultBindings[i].actionId, kActionCatalogue[i].id)) {
      return false;
    }
  }
  return true;
}

static_assert(actionCatalogueMatchesHandlers(),
              "kActionCatalogue must list the same action ids, in the same order, as "
              "kDefaultBindings");

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
