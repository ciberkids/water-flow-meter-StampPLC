#include "ui/core/ui_navigator.h"

#include <cstring>

namespace ui {

namespace {

/** Sensor list pages are `config-sensor-<n>`; returns n, or 0 if it is not one. */
uint8_t sensorIndexFromId(const char* id) {
  constexpr const char* kPrefix = "config-sensor-";
  if (!id) {
    return 0;
  }
  const std::size_t prefixLen = std::strlen(kPrefix);
  if (std::strncmp(id, kPrefix, prefixLen) != 0) {
    return 0;
  }
  const char* digits = id + prefixLen;
  if (*digits < '1' || *digits > '8' || digits[1] != '\0') {
    return 0;  // "config-sensor-back" and anything else are not a sensor.
  }
  return static_cast<uint8_t>(*digits - '0');
}

/**
 * Follows a screen's DOWN/short flow to its sibling, or null if it has none.
 *
 * Searches the generated table DIRECTLY rather than a caller-supplied copy of it. The copy used to
 * be a fixed `const Screen* all[kMaxRing * 4]` — 64 entries — which silently truncated once the
 * dataset passed 64 screens: every screen beyond the cap became unfindable, so any ring containing
 * one broke in the middle and reported the wrong member count. Adding the WiFi and MQTT levels took
 * the dataset to 82 and tripped exactly that. Searching the real table removes the capacity rather
 * than raising it, so the next batch of screens cannot reintroduce this.
 */
const ui_exporter::Screen* rawNextSibling(const ui_exporter::Screen* screen) {
  if (!screen || !screen->flows) {
    return nullptr;
  }
  for (std::size_t i = 0; i < screen->flowCount; ++i) {
    const auto& flow = screen->flows[i];
    if (flow.trigger != ui_exporter::FlowTrigger::Button) continue;
    if (flow.button != ui_exporter::FlowButton::Down) continue;
    if (!flow.targetScreenId) continue;
    for (std::size_t s = 0; s < ui_exporter::kGeneratedScreenCount; ++s) {
      const auto& candidate = ui_exporter::kGeneratedScreens[s];
      if (candidate.id && std::strcmp(candidate.id, flow.targetScreenId) == 0) {
        return &candidate;
      }
    }
    return nullptr;
  }
  return nullptr;
}

}  // namespace

bool UiNavigator::screenVisible(const ui_exporter::Screen* screen) const {
  if (!screen) {
    return false;
  }
  // No gate: unconditional, which is every screen but the three the calibration branch gates.
  if (!screen->visibleWhenBinding) {
    return true;
  }
  if (!visibility_) {
    // Nothing bound to answer it. Visible is the safe default: hiding a row because the question
    // could not be asked would make a setting unreachable, which is the failure the completeness
    // rule exists to prevent.
    return true;
  }
  return visibility_(*screen, visibilityContext_);
}

/**
 * The raw DOWN step with the firmware-appended root tail spliced in.
 *
 * The tail sits between the last DATASET member of the root level and the root — "the end of the
 * root level" of §3.4, made concrete: the step that would wrap back to P0 lands on the tail first,
 * and the tail's own step wraps. Only at depth 0; deeper levels are entirely the dataset's.
 */
const ui_exporter::Screen* UiNavigator::rawNextWithTail(const ui_exporter::Screen* from) const {
  if (!rootTail_ || depth_ != 0) {
    return rawNextSibling(from);
  }
  if (from == rootTail_) {
    return root_;
  }
  const ui_exporter::Screen* next = rawNextSibling(from);
  return (next && next == root_) ? rootTail_ : next;
}

/**
 * The next sibling the operator can actually reach — hidden screens are stepped over.
 *
 * One place, so navigation and `ringPosition` cannot disagree about how long a level is. Bounded by
 * kMaxRing: a ring whose every member is hidden would otherwise spin, and the honest answer there is
 * "nowhere to go" rather than a hang.
 *
 * The raw step is `rawNextWithTail`, not `rawNextSibling`: splicing the root tail at this one choke
 * point is what makes `ringPosition` count it and `previousVisibleSibling` find it, without a second
 * copy of the rule.
 */
const ui_exporter::Screen* UiNavigator::nextVisibleSibling(const ui_exporter::Screen* from) const {
  const ui_exporter::Screen* walk = rawNextWithTail(from);
  for (uint8_t hops = 0; walk && hops < kMaxRing; ++hops) {
    if (screenVisible(walk)) {
      return walk;
    }
    if (walk == from) {
      break;
    }
    walk = rawNextWithTail(walk);
  }
  return walk == from ? nullptr : walk;
}

const ui_exporter::Screen* UiNavigator::previousVisibleSibling(
    const ui_exporter::Screen* from) const {
  // The ring closes, so there is no backwards link to follow: walk FORWARD and keep the last
  // member before coming back round. Moved here from ui_actions.cpp, where it was inline in
  // handlePagePrevious and could not see the root tail.
  const ui_exporter::Screen* candidate = nextVisibleSibling(from);
  const ui_exporter::Screen* previous = candidate;
  for (uint8_t hops = 0; candidate && candidate != from && hops < kMaxRing; ++hops) {
    previous = candidate;
    candidate = nextVisibleSibling(candidate);
  }
  return previous;
}

const ui_exporter::Screen* UiNavigator::siblingAfter(const ui_exporter::Screen* from,
                                                     const ui_exporter::Screen* declared) const {
  // The tail cases are answered BEFORE `declared` is consulted, because the tail declares no
  // sibling of its own and a pack's declared target cannot know the tail exists.
  if (rootTail_ && depth_ == 0 && from == rootTail_) {
    return root_;
  }
  const ui_exporter::Screen* target = declared;
  if (target && !screenVisible(target)) {
    target = nextVisibleSibling(target);
  }
  if (!target) {
    target = nextVisibleSibling(from);
  }
  // The step that would wrap lands on the tail first. Wrapping the DECLARED result rather than
  // replacing it keeps the pack's own flow target on the resolution path. `target` is checked for
  // null explicitly: without that, "nothing resolved" and "the root is unset" would both compare
  // equal to `root_` and divert a caller that should fall through to the UiPage ring.
  if (rootTail_ && depth_ == 0 && target && target == root_) {
    return rootTail_;
  }
  return target;
}

const ui_exporter::Screen* UiNavigator::siblingBefore(const ui_exporter::Screen* from,
                                                      const ui_exporter::Screen* declared) const {
  // At the root with a tail bound, the declared target is stale by construction: P0's UP flow names
  // the last DATASET member, and the tail now sits between them. The spliced walk is authoritative.
  if (rootTail_ && depth_ == 0) {
    return previousVisibleSibling(from);
  }
  if (declared && screenVisible(declared)) {
    return declared;
  }
  return previousVisibleSibling(from);
}

void UiNavigator::reset(const ui_exporter::Screen* root) {
  root_ = root;
  current_ = root;
  depth_ = 0;
  sensorIndex_ = 0;
  for (auto& frame : stack_) {
    frame = nullptr;
  }
}

void UiNavigator::goToSibling(const ui_exporter::Screen* screen) {
  if (screen) {
    current_ = screen;
  }
}

bool UiNavigator::descend(const ui_exporter::Screen* screen) {
  if (!screen || depth_ >= kMaxDepth) {
    return false;
  }
  // Descending out of a sensor list page is what fixes which sensor the level below
  // applies to.
  if (const uint8_t sensor = sensorIndexFromId(current_ ? current_->id : nullptr)) {
    sensorIndex_ = sensor;
  }
  stack_[depth_] = current_;
  depth_ += 1;
  current_ = screen;
  return true;
}

bool UiNavigator::ascend() {
  if (depth_ == 0) {
    // Nothing to pop. A BACK entry does not exist on the root level, but a
    // malformed pack could still ask, so this must not underflow.
    return false;
  }
  depth_ -= 1;
  current_ = stack_[depth_];
  stack_[depth_] = nullptr;
  if (depth_ == 0) {
    sensorIndex_ = 0;
  }
  return true;
}

void UiNavigator::escape() {
  current_ = root_;
  depth_ = 0;
  sensorIndex_ = 0;
  for (auto& frame : stack_) {
    frame = nullptr;
  }
}

bool UiNavigator::ringPosition(uint8_t* indexOut, uint8_t* countOut) const {
  if (!indexOut || !countOut || !current_) {
    return false;
  }

  // The ring is only walkable through the generated screen table, which the
  // navigator does not hold — so nextSibling resolves against it directly.
  const ui_exporter::Screen* members[kMaxRing] = {};

  uint8_t count = 0;
  const ui_exporter::Screen* walk = current_;
  while (walk && count < kMaxRing) {
    members[count++] = walk;
    walk = nextVisibleSibling(walk);
    if (walk == current_) {
      break;  // Closed the ring.
    }
  }
  if (count == 0) {
    return false;
  }

  // At the root, anchor on the ROOT. The lowest-address rule below was "arbitrary but stable for a
  // given build" only while every member lived in kGeneratedScreens; the firmware-appended tail is a
  // constant in another translation unit, so link order could otherwise decide which member is index
  // 0 — and then a host assertion about ringIndex would be no evidence about the device at all.
  const ui_exporter::Screen* anchor = nullptr;
  if (depth_ == 0 && root_) {
    for (uint8_t i = 0; i < count; ++i) {
      if (members[i] == root_) {
        anchor = root_;
        break;
      }
    }
  }
  if (!anchor) {
    anchor = members[0];
    for (uint8_t i = 1; i < count; ++i) {
      if (members[i] < anchor) {
        anchor = members[i];
      }
    }
  }
  uint8_t index = 0;
  for (uint8_t i = 0; i < count; ++i) {
    if (members[i] == current_) {
      // members[] starts at current_, so the offset from the anchor is
      // (position of current) - (position of anchor), modulo the ring length.
      uint8_t anchorPos = 0;
      for (uint8_t j = 0; j < count; ++j) {
        if (members[j] == anchor) {
          anchorPos = j;
          break;
        }
      }
      index = static_cast<uint8_t>((count - anchorPos) % count);
      break;
    }
  }

  *indexOut = index;
  *countOut = count;
  return true;
}

bool UiNavigator::replaceCurrent(const ui_exporter::Screen* screen) {
  if (!screen) {
    return false;
  }
  // depth_ and stack_ are untouched on purpose: this is a substitution at the current level,
  // not a move between levels. sensorIndex_ is preserved for the same reason — the operator
  // has not left the sensor's sub-tree.
  current_ = screen;
  return true;
}

}  // namespace ui
