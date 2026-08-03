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
const ui_exporter::Screen* nextSibling(const ui_exporter::Screen* screen) {
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
    walk = nextSibling(walk);
    if (walk == current_) {
      break;  // Closed the ring.
    }
  }
  if (count == 0) {
    return false;
  }

  // Anchor on the lowest address so the reported index is stable across renders.
  const ui_exporter::Screen* anchor = members[0];
  for (uint8_t i = 1; i < count; ++i) {
    if (members[i] < anchor) {
      anchor = members[i];
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
