/**
 * The exhaustive sweep over the REAL generated screen table: can the operator get everywhere, and is
 * every edge in the table a real edge?
 *
 * Every other UI test in this suite drives a SCENARIO — descend into an editor, hold ENTER, page a ring.
 * Those are the right shape for behaviour and the wrong shape for coverage: `nyquist-warning` sat in the
 * dataset for months with plausible UP/DOWN flows and no route to it, and passed every one of them,
 * because no test ever asked "is every screen reachable?" (`J9`). `J1`'s exporter gate proves each
 * paging RING closes, which is a different and weaker claim: a closed ring nothing descends into is
 * still an island.
 *
 * ── WHY THIS RUNS ON THE TABLE AND NOT ON THE DATASET ────────────────────────────────────
 *
 * The dataset is what a designer authors; `kGeneratedScreens` is what the DEVICE navigates. Between them
 * sits the exporter, and this project has already shipped a case where the two disagreed — DF18's stale
 * `.uipack`, which a round-trip test passed because it compared positions but not sizes. Asking the
 * firmware's own table means a translation bug cannot hide behind a correct dataset.
 *
 * ── THE ONE SUBTLETY: GATED SCREENS ──────────────────────────────────────────────────────
 *
 * Six screens carry `visibleWhenBinding` — the calibration branch. The navigator SKIPS a screen whose
 * gate does not hold, so a naive walk is wrong in both directions: ignore gates and a legitimately
 * hidden screen looks reachable; treat every gate as false and it looks orphaned. The rule used here is
 * the one `assertCoversEverySetting` already applies at the dataset layer, cited so the two cannot
 * drift: a gated screen is reachable when its gate binding is a SETTING and `visibleWhenEquals` is one
 * of that setting's enumerable options. A gate naming something that is not a setting is treated as
 * reachable, which matches `UiNavigator::screenVisible` and `firmware.cpp`'s bound predicate — both
 * fail OPEN, because hiding a row when the question cannot be asked is how a setting becomes
 * unreachable.
 */
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "ui/core/ui_action_catalogue.h"
#include "ui/core/ui_pages.h"
#include "ui/core/ui_settings_types.h"
#include "ui/generated/GeneratedUi.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-72s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

using ui_exporter::Screen;

const Screen* screenById(const Screen* screens, std::size_t count, const char* id) {
  if (!id) return nullptr;
  for (std::size_t i = 0; i < count; ++i) {
    if (screens[i].id && std::strcmp(screens[i].id, id) == 0) return &screens[i];
  }
  return nullptr;
}

/** True when a screen's gate can hold — see the header comment. */
bool gateIsSatisfiable(const Screen& screen) {
  if (!screen.visibleWhenBinding) return true;  // unconditional
  const auto* setting = ui::findSetting(screen.visibleWhenBinding);
  if (!setting) return true;  // fail OPEN, as the firmware does
  // A setting with no enumerable options is a numeric one: any value is reachable by editing it, so
  // the gate is satisfiable. Only an OPTION list can prove a gate unreachable.
  if (!setting->options || setting->optionCount == 0) return true;
  for (std::size_t i = 0; i < setting->optionCount; ++i) {
    if (setting->options[i].value == screen.visibleWhenEquals) return true;
  }
  return false;
}

/**
 * Every screen reachable from `seeds` by following the table's own edges.
 *
 * Edges are flow targets and submenu entries — the two ways one screen names another. An unsatisfiable
 * gate is a wall: the screen is not entered, so nothing beyond it is entered either, which is what
 * makes this catch a whole branch stranded behind one impossible condition rather than just one screen.
 */
std::vector<std::string> reachableFrom(const Screen* screens, std::size_t count,
                                       const std::vector<std::string>& seeds) {
  std::vector<std::string> found;
  std::vector<std::string> queue;
  const auto seen = [&found](const std::string& id) {
    for (const auto& f : found) if (f == id) return true;
    return false;
  };
  for (const auto& seed : seeds) {
    const Screen* s = screenById(screens, count, seed.c_str());
    if (s && gateIsSatisfiable(*s) && !seen(seed)) { found.push_back(seed); queue.push_back(seed); }
  }
  while (!queue.empty()) {
    const std::string current = queue.back();
    queue.pop_back();
    const Screen* screen = screenById(screens, count, current.c_str());
    if (!screen) continue;
    const auto visit = [&](const char* id) {
      if (!id) return;
      const Screen* next = screenById(screens, count, id);
      if (!next || !gateIsSatisfiable(*next)) return;
      if (seen(id)) return;
      found.push_back(id);
      queue.push_back(id);
    };
    for (std::size_t i = 0; i < screen->flowCount; ++i) visit(screen->flows[i].targetScreenId);
    for (std::size_t i = 0; i < screen->submenuCount; ++i) visit(screen->submenus[i].screenId);
  }
  return found;
}

bool contains(const std::vector<std::string>& haystack, const char* needle) {
  for (const auto& h : haystack) if (h == needle) return true;
  return false;
}

// ═══════════════════════════════════════════════════════════════════════════════════════
// The algorithm, on fixtures — so its failing branches actually execute
// ═══════════════════════════════════════════════════════════════════════════════════════
//
// The real table passes everything below by construction, so mutating it to prove these assertions bite
// means editing the dataset and re-exporting through the podman compile gate. Fixtures are the cheap
// honest alternative, and the same split `coverage.test.mjs` uses for the same reason.

void theWalkFindsAnOrphan() {
  std::printf("\n[the algorithm, on fixtures — an island, a dead end, an impossible gate]\n");

  static const ui_exporter::Flow kRootFlows[] = {
      {"f", "to b", "b", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Down,
       ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "a", nullptr, 0}};
  static const ui_exporter::Flow kBackFlows[] = {
      {"f", "to root", "root", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up,
       ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "a", nullptr, 0}};
  // `island` has flows that look correct and no inbound edge — J9's shape exactly.
  static const ui_exporter::Flow kIslandFlows[] = {
      {"f", "to root", "root", ui_exporter::FlowTrigger::Button, ui_exporter::FlowButton::Up,
       ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr, "a", nullptr, 0}};
  static const Screen kScreens[] = {
      {"root", "Root", nullptr, 0, kRootFlows, 1, nullptr, 0, nullptr, 0, nullptr, 0},
      {"b", "B", nullptr, 0, kBackFlows, 1, nullptr, 0, nullptr, 0, nullptr, 0},
      {"island", "Island", nullptr, 0, kIslandFlows, 1, nullptr, 0, nullptr, 0, nullptr, 0},
      // A screen behind a gate naming a setting that does not exist: fail OPEN, so reachable.
      {"gated-open", "Gated", nullptr, 0, kBackFlows, 1, nullptr, 0, nullptr, 0,
       "no.such.setting", 7},
      // And one behind a gate on a REAL option setting, at a value that is not one of its options.
      {"gated-shut", "Gated shut", nullptr, 0, kBackFlows, 1, nullptr, 0, nullptr, 0,
       "config.sensor.calibrationType", 99}};
  constexpr std::size_t kCount = sizeof(kScreens) / sizeof(kScreens[0]);

  // `gated-open` and `gated-shut` are linked FROM root for this fixture via submenus, so the only
  // reason either could be missing is its gate.
  static const ui_exporter::Submenu kSubs[] = {{"s1", "open", "gated-open", nullptr},
                                               {"s2", "shut", "gated-shut", nullptr}};
  static const Screen kLinked[] = {
      {"root", "Root", nullptr, 0, kRootFlows, 1, nullptr, 0, kSubs, 2, nullptr, 0},
      kScreens[1], kScreens[2], kScreens[3], kScreens[4]};

  const auto reached = reachableFrom(kLinked, kCount, {"root"});
  check(contains(reached, "root") && contains(reached, "b"), "the walk follows flow targets");
  check(!contains(reached, "island"),
        "a screen with correct-looking flows and no inbound edge is NOT reachable — J9's shape");
  check(contains(reached, "gated-open"),
        "a gate naming no known setting fails OPEN, matching UiNavigator::screenVisible");
  check(!contains(reached, "gated-shut"),
        "but a gate on a real option setting at an impossible value is a wall");
  check(reached.size() == 3, "so three of the five fixture screens are reachable");

  // A target naming a screen that does not exist must not be followed or crash the walk.
  static const ui_exporter::Flow kDangling[] = {
      {"f", "nowhere", "does-not-exist", ui_exporter::FlowTrigger::Button,
       ui_exporter::FlowButton::Down, ui_exporter::FlowGesture::Short, 0, nullptr, nullptr, nullptr,
       "a", nullptr, 0}};
  static const Screen kOne[] = {
      {"root", "Root", nullptr, 0, kDangling, 1, nullptr, 0, nullptr, 0, nullptr, 0}};
  const auto fromDangling = reachableFrom(kOne, 1, {"root"});
  check(fromDangling.size() == 1, "a dangling target is skipped rather than followed");
}

// ═══════════════════════════════════════════════════════════════════════════════════════
// The real table
// ═══════════════════════════════════════════════════════════════════════════════════════

void everyScreenIsReachable() {
  std::printf("\n[the real table — every screen, from the screens the firmware resolves by name]\n");

  const Screen* screens = ui_exporter::kGeneratedScreens;
  const std::size_t count = ui_exporter::kGeneratedScreenCount;
  check(count > 0, "the generated table is not empty");

  /**
   * The seeds are `kRequiredScreens` and NOTHING else.
   *
   * That array is the firmware's own list of screens it resolves by name rather than by navigation, so
   * it is exactly the right seed set — and using anything wider would defeat the gate. Seeding from
   * "every screen with no inbound edge", say, would make an orphan a seed and this test unable to fail.
   * `kNyquistPromptScreenId` was proposed for this array on the branch that carried the retired
   * separate-screen design, which is precisely why the orphan was unreachable without it.
   */
  std::vector<std::string> seeds;
  bool everySeedExists = true;
  for (const auto& role : kRequiredScreens) {
    if (!screenById(screens, count, role.id)) {
      std::printf("      SEED MISSING FROM TABLE: %s (%s)\n", role.id, role.role);
      everySeedExists = false;
    }
    seeds.push_back(role.id);
  }
  check(!seeds.empty(), "there is at least one seed — an empty list would pass everything");
  check(everySeedExists, "every kRequiredScreens entry exists in the table (a typo shrinks the walk)");

  const auto reached = reachableFrom(screens, count, seeds);
  std::printf("      %zu of %zu screens reachable from %zu seeds\n", reached.size(), count,
              seeds.size());

  /**
   * The screens that are never navigated to ON PURPOSE, each with the reason.
   *
   * This list is the whole difference between a gate and a nuisance, and it has exactly one member.
   * Its first run flagged `state-idle`, which looked like a second `nyquist-warning` — plausible wake
   * flows, no inbound edge, nothing in the firmware naming it. It is not: idle is a MODE, not a screen.
   * `UiRenderer::update` takes the `UiMode::Idle` branch, sets the backlight off, fills the screen and
   * RETURNS, so no element of `state-idle` is ever drawn; `InteractionHandler` owns the wake.
   *
   * It is kept because the MOCKUP uses it: `App.tsx` reads its wake flows as the spec for swallowing
   * the waking press, and `DisplayViewport.tsx` deliberately refuses to draw its `- Display off -`
   * label, calling that label "the dataset being unfaithful". `tools/audit/screen-geometry.ts` exempts
   * it from the banner-band rule for the same reason, "by construction rather than by exemption".
   *
   * So the fact was recorded in three prose comments in three files and asserted nowhere. It is
   * asserted here, in both directions — see below.
   */
  struct NeverNavigated { const char* id; const char* why; };
  static constexpr NeverNavigated kNeverNavigated[] = {
      {"state-idle",
       "idle is a MODE: UiRenderer blanks the panel and returns, so no element here is ever drawn. "
       "Kept for the mockup, which reads its wake flows as a spec and refuses to draw its label."}};

  std::vector<std::string> unreachable;
  for (std::size_t i = 0; i < count; ++i) {
    if (!contains(reached, screens[i].id)) unreachable.push_back(screens[i].id);
  }

  bool everyUnreachableIsAllowed = true;
  for (const auto& id : unreachable) {
    bool allowed = false;
    for (const auto& entry : kNeverNavigated) {
      if (id == entry.id) { allowed = true; break; }
    }
    if (!allowed) {
      std::printf("      UNREACHABLE AND UNEXPLAINED: %s\n", id.c_str());
      everyUnreachableIsAllowed = false;
    }
  }
  check(everyUnreachableIsAllowed,
        "every unreachable screen is one the list above explains — an unexplained one is J9's shape");

  // AND THE OTHER DIRECTION, which is what stops the list rotting into a blanket excuse: an entry that
  // has BECOME reachable is a stale exemption, and an entry naming a screen that no longer exists is a
  // dead one. Either way the list has to be corrected rather than silently carried.
  bool listIsCurrent = true;
  for (const auto& entry : kNeverNavigated) {
    if (!screenById(screens, count, entry.id)) {
      std::printf("      STALE EXEMPTION (no such screen): %s\n", entry.id);
      listIsCurrent = false;
    } else if (contains(reached, entry.id)) {
      std::printf("      STALE EXEMPTION (now reachable): %s — %s\n", entry.id, entry.why);
      listIsCurrent = false;
    }
  }
  check(listIsCurrent, "and every exemption still names a screen that exists and is still unreached");
  std::printf("      %zu screen(s) unreachable, all explained\n", unreachable.size());
}

void everyEdgeIsReal() {
  std::printf("\n[every edge names something that exists, and no button is bound twice]\n");

  const Screen* screens = ui_exporter::kGeneratedScreens;
  const std::size_t count = ui_exporter::kGeneratedScreenCount;

  bool targetsExist = true;
  bool actionsExist = true;
  bool noAmbiguity = true;
  bool everyScreenLeaves = true;
  std::size_t buttonFlows = 0;

  for (std::size_t i = 0; i < count; ++i) {
    const Screen& screen = screens[i];
    bool leaves = false;
    // (button, gesture) pairs already bound on THIS screen.
    std::vector<std::pair<int, int>> bound;
    for (std::size_t f = 0; f < screen.flowCount; ++f) {
      const ui_exporter::Flow& flow = screen.flows[f];
      if (flow.targetScreenId && !screenById(screens, count, flow.targetScreenId)) {
        std::printf("      DANGLING: %s.%s -> %s\n", screen.id, flow.id, flow.targetScreenId);
        targetsExist = false;
      }
      // An actionId must be one the firmware advertises. A flow naming an action nobody dispatches is
      // a button that does nothing — and the dataset can name any string it likes.
      if (flow.actionId) {
        bool known = false;
        for (std::size_t a = 0; a < kActionCatalogueCount; ++a) {
          if (std::strcmp(kActionCatalogue[a].id, flow.actionId) == 0) { known = true; break; }
        }
        if (!known) {
          std::printf("      UNKNOWN ACTION: %s.%s -> %s\n", screen.id, flow.id, flow.actionId);
          actionsExist = false;
        }
      }
      if (flow.trigger == ui_exporter::FlowTrigger::Button) {
        ++buttonFlows;
        const std::pair<int, int> key{static_cast<int>(flow.button), static_cast<int>(flow.gesture)};
        for (const auto& already : bound) {
          if (already == key) {
            std::printf("      AMBIGUOUS: %s binds button %d gesture %d twice\n", screen.id,
                        key.first, key.second);
            noAmbiguity = false;
          }
        }
        bound.push_back(key);
        leaves = true;
      } else {
        // A timeout flow counts as an exit: that is how a toast leaves (§3.8's auto-dismiss), and
        // requiring a BUTTON exit would fail every toast by design.
        leaves = true;
      }
    }
    if (!leaves) {
      std::printf("      DEAD END: %s has no flow at all\n", screen.id);
      everyScreenLeaves = false;
    }
  }

  std::printf("      %zu button flows across %zu screens\n", buttonFlows, count);
  check(targetsExist, "every flow target names a screen that exists in the table");
  check(actionsExist, "every flow action is one the firmware's catalogue advertises");
  check(noAmbiguity, "no screen binds the same button and gesture twice");
  check(everyScreenLeaves, "no screen is a dead end — every one has a way out");
}

void everyGateCanHold() {
  std::printf("\n[every gated screen's condition is satisfiable, or the branch is stranded]\n");

  const Screen* screens = ui_exporter::kGeneratedScreens;
  const std::size_t count = ui_exporter::kGeneratedScreenCount;
  std::size_t gated = 0;
  bool allSatisfiable = true;
  for (std::size_t i = 0; i < count; ++i) {
    if (!screens[i].visibleWhenBinding) continue;
    ++gated;
    if (!gateIsSatisfiable(screens[i])) {
      std::printf("      UNSATISFIABLE: %s gated on %s == %d\n", screens[i].id,
                  screens[i].visibleWhenBinding, static_cast<int>(screens[i].visibleWhenEquals));
      allSatisfiable = false;
    }
  }
  std::printf("      %zu gated screens\n", gated);
  check(gated > 0, "there are gated screens, so the rule above is exercised on the real table");
  check(allSatisfiable, "every gate names a value its setting can actually take");
}

}  // namespace

int main() {
  std::printf("ui_walk — can the operator get everywhere, and is every edge real? (J9's generalisation)\n");
  theWalkFindsAnOrphan();
  everyScreenIsReachable();
  everyEdgeIsReal();
  everyGateCanHold();
  if (failures > 0) {
    std::printf("\nFAILURES (%d of %d)\n", failures, checks);
    return 1;
  }
  std::printf("\nALL PASSED (%d checks)\n", checks);
  return 0;
}
