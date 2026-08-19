#pragma once

#include <cstddef>

/**
 * The firmware's declaration of every action a UI flow may invoke.
 *
 * Paired with `kDefaultBindings` in ui_actions.cpp, which maps these ids to handlers. The
 * two are checked against each other at compile time (see the static_asserts there), so an
 * action cannot be advertised to the design tool without a handler behind it, nor a handler
 * registered without being documented.
 *
 * Arduino-free: `tools/manifest_gen` reads this table directly.
 */
struct ActionDescriptor {
  const char* id;
  /** Shown in the designer's action picker. */
  const char* label;
  const char* description;
};

inline constexpr ActionDescriptor kActionCatalogue[] = {
    {"ui.action.page.next", "Next page", "Advances to the next UI page"},
    {"ui.action.page.previous", "Previous page", "Returns to the previous UI page"},
    {"ui.action.mode.idle", "Enter idle", "Dims the display and enters idle mode"},
    {"core.action.save-config", "Save configuration",
     "Persists the current configuration block to NVS and Modbus registers"},
    {"core.action.reset-session", "Reset session counters", "Issues the Reset Session Modbus command for all ready sensors"},
    {"core.action.reset-all-measured", "Reset all measured totals", "Issues the Reset All Measured Modbus command"},
    {"core.action.reset-max-flow", "Reset peak flow",
     "Issues the Reset Max Flow Modbus command; clears the volatile peak and nothing else"},
    // Named for the calibration, not for "the sensor" or "the values". The owner asked for a way to
    // "reset sensor values", but this keeps every accumulated volume on purpose, and a menu entry that
    // promises to reset values it then keeps is a label the panel would be lying with. What it actually
    // returns to defaults is the calibration, so that is what it is called, here and on the confirm.
    {"core.action.reset-calibration", "Reset calibration",
     "Returns the SELECTED channel's calibration to defaults so a replacement meter can be entered; "
     "cumulative, session and peak readings are kept"},
    {"core.action.factory-reset", "Factory reset", "Wipes NVS, clears Modbus config, and reboots"},
    {"core.action.reset-portal-login", "Reset portal login",
     "Restores the configuration portal login to admin/admin, leaving totals, calibration and "
     "network settings untouched (R8.2a)"},
    {"ui.action.nav.descend", "Descend one level", "Push the flow's target level onto the navigation stack"},
    {"ui.action.nav.back", "Back one level", "Pop one level off the navigation stack"},
    {"ui.action.nav.escape", "Escape to main screen", "Clear the navigation stack back to P0, discarding any uncommitted edit"},
    {"config.action.value.increment", "Increment value", "Raise the pending value by its step, with hold acceleration"},
    {"config.action.value.decrement", "Decrement value", "Lower the pending value by its step, with hold acceleration"},
    {"config.action.value.commit", "Commit value",
     "Clamp, write the mapped register, validate, then ascend one level"},
    {"config.action.value.commit-override", "Save despite Nyquist warning",
     "Forces the pending value past a failed Nyquist check and raises bit n of register 30 "
     "(\u00a75.5)"},
    {"config.action.value.discard", "Discard edit", "Abandon the pending value and ascend one level"},
    {"ui.action.pack.select-menu", "Open the Select Menu",
     "Opens the firmware-drawn menu-pack selector. Declared by the firmware-appended root entry "
     "(Loadable_UI_Menu_Packs.md §3.4); the same page the UP+DOWN+ENTER 3 s gesture opens"}};

inline constexpr std::size_t kActionCatalogueCount =
    sizeof(kActionCatalogue) / sizeof(kActionCatalogue[0]);

/** Compile-time string equality, so the id cross-check below costs nothing at runtime. */
constexpr bool actionIdsEqual(const char* a, const char* b) {
  while (*a != '\0' && *a == *b) {
    ++a;
    ++b;
  }
  return *a == *b;
}
