#pragma once

#include <cstddef>
#include <cstdint>

#include "ui/generated/GeneratedUi.h"

namespace ui {

/**
 * Tracks where the operator is in the navigation tree
 * (Display_UI_Requirements.md §5.1).
 *
 * The dataset's flows already describe the entire graph: every UP/DOWN flow names
 * its sibling and every ENTER-short flow names the level below, both as
 * `targetScreenId`. So navigation is just "follow the flow's target", and this class
 * only has to remember how to get *back*. That is why there are no per-level screen
 * tables here — adding them would duplicate what the dataset already states, which is
 * how the router's stale `info-overview`/`configuration`/`countdown` defaults came to
 * disagree with reality in the first place.
 *
 * Fixed capacity, no allocation. Depth 5 covers the deepest documented path:
 * Info -> Config root -> Sensor list -> Sensor settings -> Value editor.
 */
class UiNavigator {
 public:
  static constexpr uint8_t kMaxDepth = 5;

  /**
   * Answers whether a conditional screen is currently part of its level.
   *
   * A function pointer rather than an owned dependency: deciding it needs the SETTINGS, which the
   * navigator deliberately does not hold — it knows the screen table and the stack, nothing else.
   * Whoever owns settings binds this once at startup.
   *
   * Unbound, every screen is visible, which is what the host navigation tests want and what the
   * device does before settings are loaded.
   */
  using VisibilityFn = bool (*)(const ui_exporter::Screen&, void* context);
  void bindVisibility(VisibilityFn fn, void* context) {
    visibility_ = fn;
    visibilityContext_ = context;
  }

  /** Whether this screen is part of its level right now. True when it carries no gate. */
  bool screenVisible(const ui_exporter::Screen* screen) const;

  /** The next sibling the operator can reach, stepping over any that are hidden. */
  const ui_exporter::Screen* nextVisibleSibling(const ui_exporter::Screen* from) const;

  /** Establishes the root (P0) and places the operator on it. */
  void reset(const ui_exporter::Screen* root);

  const ui_exporter::Screen* current() const { return current_; }
  const ui_exporter::Screen* root() const { return root_; }
  uint8_t depth() const { return depth_; }

  /** Moves within the current level; does not change depth. */
  void goToSibling(const ui_exporter::Screen* screen);

  /** Pushes the current screen and moves down. False if full or target is null. */
  bool descend(const ui_exporter::Screen* screen);

  /** Pops one level. False at the root, where there is nothing to pop. */
  bool ascend();

  /** Unconditionally returns to the root, discarding the whole stack. */
  void escape();

  /**
   * Swaps the current screen for another AT THE SAME DEPTH, leaving the stack untouched.
   *
   * The distinction from `descend` matters, and the acknowledgement toast is why. A toast
   * dismisses itself with `ui.action.nav.back` (see the Timeout flows on `toast-*` in the
   * generated table), so pushing it onto the confirm screen would ascend straight back into
   * "RESET TOTALS?" — the operator would be asked again whether to do the thing they had just
   * done. Replacing the confirm screen means that same ascend lands on the page they started
   * from, which is what Display_UI_Requirements §4.3.1 asks for.
   *
   * Deliberately a general operation rather than a toast special case: any screen that stands
   * in for another at the same level needs it, and the menu-pack work will want it to swap a
   * loaded pack's screen for a built-in one without adding a level.
   *
   * False when `screen` is null. Safe at the root, where it simply changes which screen the
   * root shows without inventing a level to pop.
   */
  bool replaceCurrent(const ui_exporter::Screen* screen);

  /**
   * 1-8 while inside a sensor's sub-tree, 0 otherwise.
   *
   * Cached when descending out of a `config-sensor-<n>` page rather than re-parsed
   * from the stack on every binding lookup. This is the "the sensor is the level you
   * came from" rule (§5.1) made explicit in code, and it is why no
   * `selectedSensor` setting exists.
   */
  uint8_t sensorIndex() const { return sensorIndex_; }

  /**
   * Position of the current screen within its level's ring, for the scrollbar.
   *
   * The ring is discovered by following DOWN flows until they come back round. The
   * anchor is whichever member has the lowest address — arbitrary, but stable for a
   * given build, which is all a position indicator needs.
   */
  bool ringPosition(uint8_t* indexOut, uint8_t* countOut) const;

 private:
  static constexpr uint8_t kMaxRing = 16;

  const ui_exporter::Screen* root_ = nullptr;
  const ui_exporter::Screen* current_ = nullptr;
  const ui_exporter::Screen* stack_[kMaxDepth] = {};
  uint8_t depth_ = 0;
  uint8_t sensorIndex_ = 0;
  VisibilityFn visibility_ = nullptr;
  void* visibilityContext_ = nullptr;
};

}  // namespace ui
