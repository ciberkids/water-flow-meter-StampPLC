// Host tests for the firmware-owned Select Menu page (§3.4).
//
// The page exists because a selector inside a loadable pack would let a pack that omits it trap
// the operator — the same class of problem as the blind factory-reset combo §3.3 retired, which
// this project has already shipped once. So the cases that matter are the awkward ones: an empty
// card, more packs than the page can show, and a selection that is already running.
#include "ui/pack/ui_pack_selector.h"

#include <cstdio>
#include <cstring>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-70s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

using ui::PackLoader;
using ui::PackSelector;

/** Builds a names array the way the storage adapter's directory scan does. */
struct Names {
  char buffer[PackSelector::kMaxEntries + 4][PackLoader::kMaxNameBytes] = {};
  std::size_t count = 0;

  void add(const char* name) {
    std::snprintf(buffer[count], PackLoader::kMaxNameBytes, "%s", name);
    ++count;
  }
};

void builtInAlwaysPresentTests() {
  std::printf("[entry 0 is always the built-in default — §3.4]\n");

  // The case that makes the page worth having: no card, or a card with nothing on it. Returning
  // to a known-good UI must not depend on the card being readable.
  PackSelector selector;
  selector.begin(nullptr, 0, nullptr);
  check(selector.entryCount() == 1, "an empty card still offers one entry");
  check(std::strcmp(selector.labelAt(0), "Built-in") == 0, "and it is the built-in default");
  check(selector.isActive(0), "which is marked active when no pointer is set");
  check(selector.commitAction() == PackSelector::Commit::Nothing,
        "selecting what is already running does nothing rather than rebooting pointlessly");
}

void listingTests() {
  std::printf("\n[listing the card]\n");

  Names names;
  names.add("production.uipack");
  names.add("commissioning.uipack");
  names.add("service.uipack");

  PackSelector selector;
  selector.begin(names.buffer, names.count, "commissioning.uipack");
  check(selector.entryCount() == 4, "three packs plus the built-in");
  check(std::strcmp(selector.labelAt(1), "production.uipack") == 0, "entry 1 is the first pack");
  check(std::strcmp(selector.labelAt(3), "service.uipack") == 0, "entry 3 is the last");
  check(selector.isActive(2), "the pointer file's pack is the one marked active");
  check(!selector.isActive(0), "and the built-in is not");

  // Opens on what is running, since the operator is here either to check it or to leave it.
  check(selector.cursor() == 2, "the cursor opens on the active entry, not on entry 0");
}

void cursorTests() {
  std::printf("\n[the cursor wraps like every other ring]\n");

  Names names;
  names.add("a.uipack");
  names.add("b.uipack");

  PackSelector selector;
  selector.begin(names.buffer, names.count, nullptr);
  check(selector.cursor() == 0, "no pointer set, so it opens on the built-in");

  selector.moveCursor(1);
  check(selector.cursor() == 1, "DOWN advances");
  selector.moveCursor(1);
  selector.moveCursor(1);
  check(selector.cursor() == 0, "and wraps past the end");
  selector.moveCursor(-1);
  check(selector.cursor() == 2, "UP wraps the other way");

  // A delta larger than the list must not run off it — the acceleration tiers can produce 25.
  selector.moveCursor(25);
  check(selector.cursor() < selector.entryCount(), "a large delta stays in range");
  selector.moveCursor(-99);
  check(selector.cursor() < selector.entryCount(), "and so does a large negative one");
}

void truncationTests() {
  std::printf("\n[a card with more packs than the page can show]\n");

  Names names;
  for (std::size_t i = 0; i < PackSelector::kMaxEntries + 3; ++i) {
    char name[32];
    std::snprintf(name, sizeof(name), "pack%zu.uipack", i);
    names.add(name);
  }

  PackSelector selector;
  selector.begin(names.buffer, names.count, nullptr);
  check(selector.entryCount() == PackSelector::kMaxEntries,
        "the list fills to its cap and no further");
  check(selector.truncated(),
        "and reports the truncation, so the operator is told rather than left wondering");

  // Slot 0 is the built-in, so a full page shows kMaxEntries - 1 card packs.
  check(std::strcmp(selector.labelAt(0), "Built-in") == 0,
        "the built-in still occupies entry 0 — it is never displaced by a full card");
}

void commitTests() {
  std::printf("\n[what committing asks the caller to do]\n");

  Names names;
  names.add("production.uipack");
  names.add("service.uipack");

  {
    PackSelector selector;
    selector.begin(names.buffer, names.count, "production.uipack");
    selector.moveCursor(1);  // production -> service
    check(selector.commitAction() == PackSelector::Commit::WritePointer,
          "choosing another pack writes the pointer file");
    check(std::strcmp(selector.commitName(), "service.uipack") == 0, "with that pack's name");
  }
  {
    PackSelector selector;
    selector.begin(names.buffer, names.count, "production.uipack");
    while (selector.cursor() != PackSelector::kBuiltInIndex) selector.moveCursor(-1);
    check(selector.commitAction() == PackSelector::Commit::DeletePointer,
          "choosing the built-in DELETES the pointer rather than writing a name for it");
    check(std::strcmp(selector.commitName(), "") == 0, "and names nothing to write");
  }
  {
    PackSelector selector;
    selector.begin(names.buffer, names.count, "service.uipack");
    check(selector.commitAction() == PackSelector::Commit::Nothing,
          "re-selecting the running pack does nothing, so ENTER is not a pointless reboot");
  }
}

void hostileInputTests() {
  std::printf("\n[hostile and degenerate input]\n");

  PackSelector selector;

  // A name exactly at the limit must not overrun. The adapter already filters longer ones, but
  // this class must not depend on that having happened.
  Names names;
  char long_name[PackLoader::kMaxNameBytes + 8];
  std::memset(long_name, 'x', sizeof(long_name) - 1);
  long_name[sizeof(long_name) - 1] = '\0';
  names.add(long_name);
  selector.begin(names.buffer, names.count, nullptr);
  check(std::strlen(selector.labelAt(1)) < PackLoader::kMaxNameBytes,
        "an over-long name is truncated rather than overrunning the buffer");

  // A pointer naming a pack the card does not hold: nothing is marked active, and the built-in
  // stays the fallback. That is the dangling-pointer case seen from the UI side.
  Names present;
  present.add("a.uipack");
  PackSelector dangling;
  dangling.begin(present.buffer, present.count, "vanished.uipack");
  check(dangling.isActive(PackSelector::kBuiltInIndex),
        "a pointer to a pack that is not listed leaves the built-in marked active");
  check(!dangling.isActive(1), "and does not mark an unrelated pack");

  // begin() must be safe to call twice — the page is rebuilt on every entry, since the card may
  // have changed since the last time it was opened.
  PackSelector reused;
  reused.begin(names.buffer, names.count, nullptr);
  reused.moveCursor(1);
  reused.begin(present.buffer, present.count, "a.uipack");
  check(reused.entryCount() == 2, "rebuilding replaces the previous list rather than appending");
  check(reused.isActive(1), "and re-marks the active entry from the fresh pointer");
}

}  // namespace

int main() {
  std::printf("ui::PackSelector — the firmware-owned Select Menu page\n\n");
  builtInAlwaysPresentTests();
  listingTests();
  cursorTests();
  truncationTests();
  commitTests();
  hostileInputTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
