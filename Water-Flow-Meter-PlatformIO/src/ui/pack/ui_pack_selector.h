#pragma once

#include <cstddef>
#include <cstdint>

#include "ui/pack/ui_pack_loader.h"

namespace ui {

/**
 * The firmware-owned "Select Menu" page (Loadable_UI_Menu_Packs.md §3.4).
 *
 * **The selector must not live inside a loadable pack.** If the only route to changing packs were
 * a page inside the active one, a pack that omitted that page would trap the operator with no way
 * out — the same class of problem as the blind factory-reset combo §3.3 retired, and this project
 * has already shipped that mistake once.
 *
 * So the firmware appends this page to the end of the root level whatever the active pack defines,
 * and a pack cannot remove it. Entry 0 is always the embedded default, so returning to a
 * known-good UI never depends on the card being readable.
 *
 * Arduino-free: the list, the cursor and the commit decision are all pure state, so the awkward
 * cases — an empty card, a card with more packs than the list can hold, a selection whose file
 * disappeared between listing and committing — are host-testable. The storage adapter supplies
 * names and performs the write.
 */
class PackSelector {
 public:
  /**
   * How many card packs the page will show.
   *
   * Eight is chosen against the display rather than the filesystem: the page has to be navigable
   * with UP/DOWN on a 240x135 panel, and a list longer than this is worse than a truncated one.
   * `truncated()` reports when the card held more, so the operator is told rather than left to
   * wonder why their pack is missing.
   */
  static constexpr std::size_t kMaxEntries = 8;

  /** Index 0 is always the built-in default and is never supplied by the card. */
  static constexpr std::size_t kBuiltInIndex = 0;

  /**
   * Builds the list.
   *
   * `names` and `count` come from the storage adapter's directory scan; `activeName` is the
   * pointer file's contents, or nullptr when none is set. Safe with count == 0, which is the
   * ordinary "card with no packs on it" case.
   */
  void begin(const char (*names)[PackLoader::kMaxNameBytes],
             std::size_t count,
             const char* activeName);

  std::size_t entryCount() const { return entryCount_; }
  std::size_t cursor() const { return cursor_; }

  /** True when the card held more packs than the page can show. */
  bool truncated() const { return truncated_; }

  /** Label for an entry: "Built-in" for 0, otherwise the filename. */
  const char* labelAt(std::size_t index) const;

  /** True for the entry the pointer file currently names — the one marked on screen (§3.4). */
  bool isActive(std::size_t index) const;

  /** UP/DOWN, wrapping like every other ring in the UI. */
  void moveCursor(int32_t delta);

  /**
   * What committing the cursor's entry requires of the caller.
   *
   * The selector decides; it does not perform. Writing the pointer file needs the SD adapter and
   * rebooting needs the platform, and keeping both out of here is what makes the decision
   * testable.
   */
  enum class Commit : uint8_t {
    Nothing,          /**< The chosen entry is already active. */
    DeletePointer,    /**< Entry 0: remove the pointer so the built-in default runs. */
    WritePointer      /**< Write `commitName()` to the pointer file. */
  };

  Commit commitAction() const;

  /** The filename to write, valid when commitAction() is WritePointer. */
  const char* commitName() const;

 private:
  char names_[kMaxEntries][PackLoader::kMaxNameBytes] = {};
  std::size_t entryCount_ = 1;  // the built-in default is always present
  std::size_t cursor_ = 0;
  std::size_t activeIndex_ = kBuiltInIndex;
  bool truncated_ = false;
};

}  // namespace ui
