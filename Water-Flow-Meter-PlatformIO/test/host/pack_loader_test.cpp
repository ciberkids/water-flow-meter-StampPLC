// Host tests for the boot-time pack selection ladder (§3.6).
//
// Every interesting case here is a failure case: no card, a dangling pointer, a pointer with
// "../.." in it, a truncated pack, and a pack that validates and then crashes the renderer. None
// of those is convenient to produce on real hardware, and the last one cannot be produced at all
// without deliberately shipping a broken pack. So the ladder takes storage and the attempt
// counter as interfaces and is driven here with fakes.
#include "ui/pack/ui_pack_loader.h"
#include "ui/generated/GeneratedUi.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-70s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

std::vector<uint8_t> loadFixture() {
  const char* candidates[] = {"../web/mockup/tests/fixtures/default.uipack",
                              "web/mockup/tests/fixtures/default.uipack"};
  for (const char* path : candidates) {
    if (std::FILE* f = std::fopen(path, "rb")) {
      std::fseek(f, 0, SEEK_END);
      const long size = std::ftell(f);
      std::fseek(f, 0, SEEK_SET);
      std::vector<uint8_t> bytes(static_cast<std::size_t>(size));
      const std::size_t read = std::fread(bytes.data(), 1, bytes.size(), f);
      std::fclose(f);
      if (read == bytes.size()) return bytes;
    }
  }
  return {};
}

/** A card that can be told to misbehave in each way §3.6 anticipates. */
class FakeStorage : public ui::PackStorage {
 public:
  bool mountable = true;
  bool hasPointer = true;
  std::string pointer = "default.uipack";
  /**
   * Which files the card actually holds, keyed by name.
   *
   * Modelled separately from the pointer on purpose. The first version of this fake answered
   * packSize() by comparing against the pointer, so every name appeared to exist and the
   * dangling-pointer case could not be expressed — the fake was wrong, not the loader.
   */
  std::string presentName = "default.uipack";
  std::vector<uint8_t> contents;
  bool shortRead = false;
  /** Observed, so "the guard cleared the selection" is checkable rather than assumed. */
  bool pointerDeleted = false;
  int mountCalls = 0;

  bool mount() override {
    ++mountCalls;
    return mountable;
  }

  bool readPointer(char* out, std::size_t size) override {
    if (!hasPointer || pointer.empty()) return false;
    std::snprintf(out, size, "%s", pointer.c_str());
    return true;
  }

  bool deletePointer() override {
    pointerDeleted = true;
    hasPointer = false;
    return true;
  }

  long packSize(const char* name) override {
    if (presentName != name) return -1;
    return static_cast<long>(contents.size());
  }

  bool readPack(const char* name, uint8_t* buffer, std::size_t size) override {
    if (shortRead) return false;
    if (presentName != name || size > contents.size()) return false;
    std::memcpy(buffer, contents.data(), size);
    return true;
  }
};

class FakeCounter : public ui::PackAttemptCounter {
 public:
  uint8_t value = 0;
  int writes = 0;
  uint8_t read() override { return value; }
  void write(uint8_t v) override {
    value = v;
    ++writes;
  }
};

constexpr uint16_t kFirmwareAbi = 1;

struct Fixture {
  FakeStorage storage;
  FakeCounter counter;
  ui::PackLoader loader;
  ui::MenuPack pack;
  std::vector<uint8_t> buffer = std::vector<uint8_t>(ui::PackLoader::kMaxPackBytes);

  ui::LoadOutcome run() {
    return loader.load(storage, counter, kFirmwareAbi, buffer.data(), buffer.size(), &pack);
  }
};

void happyPathTests(const std::vector<uint8_t>& good) {
  std::printf("[a valid pack on a good card]\n");

  Fixture f;
  f.storage.contents = good;
  check(f.run() == ui::LoadOutcome::CardPack, "the pack is selected");
  check(f.pack.valid(), "and is usable");
  // Derived from the emitted table, not a literal: the fixture IS the default dataset, so the
  // count is whatever the generator last produced. A hard-coded 48 made this test fail every time
  // a screen was added, which says nothing about the loader.
  check(f.pack.screenCount() == ui_exporter::kGeneratedScreenCount,
        "with every screen the generated table has");
  check(std::strcmp(f.loader.selectedName(), "default.uipack") == 0,
        "the selection is reported for the boot message");

  // §3.6 step 6/11: the attempt stands until a frame has actually been drawn.
  check(f.counter.value == 1, "an attempt is recorded, and stands until a render succeeds");
  f.loader.noteSuccessfulRender(f.counter);
  check(f.counter.value == 0, "a successful render clears it");

  // Idempotent — the render hook runs every frame in the real loop.
  const int writes = f.counter.writes;
  f.loader.noteSuccessfulRender(f.counter);
  check(f.counter.writes == writes, "and does not rewrite NVS on every subsequent frame");
}

void normalStateTests(const std::vector<uint8_t>& good) {
  std::printf("\n[no card and no selection are NORMAL — they must not burn an attempt]\n");

  {
    Fixture f;
    f.storage.mountable = false;
    check(f.run() == ui::LoadOutcome::BuiltInNoCard, "an unmountable card falls back to built-in");
    check(f.counter.value == 0, "without burning an attempt");
    check(!ui::loadOutcomeIsFailure(ui::LoadOutcome::BuiltInNoCard),
          "and is not reported to the operator as a fault");
  }
  {
    Fixture f;
    f.storage.contents = good;
    f.storage.hasPointer = false;
    check(f.run() == ui::LoadOutcome::BuiltInNoPointer, "no /ui/active falls back to built-in");
    check(f.counter.value == 0, "without burning an attempt");
    check(!ui::loadOutcomeIsFailure(ui::LoadOutcome::BuiltInNoPointer), "and is not a fault");
  }
  {
    Fixture f;
    f.storage.contents = good;
    f.storage.pointer = "";
    check(f.run() == ui::LoadOutcome::BuiltInNoPointer, "an empty pointer file is the same case");
    check(f.counter.value == 0, "and still burns nothing");
  }

  // This is the consequence §3.6 calls out: burning attempts on a missing card would, after two
  // boots without one, DELETE a selection that was never at fault.
  {
    Fixture f;
    f.storage.mountable = false;
    f.run();
    f.run();
    f.run();
    check(f.counter.value == 0 && !f.storage.pointerDeleted,
          "three cardless boots do not eventually delete a good selection");
  }
}

void badPointerTests(const std::vector<uint8_t>& good) {
  std::printf("\n[an unusable selection — §3.1.1]\n");

  const char* traversals[] = {"../secrets", "sub/dir.uipack", "..\\windows", "/etc/passwd"};
  for (const char* attempt : traversals) {
    Fixture f;
    f.storage.contents = good;
    f.storage.pointer = attempt;
    const auto outcome = f.run();
    ++checks;
    const bool refused = outcome == ui::LoadOutcome::BuiltInBadPointer;
    std::printf("  %-70s %s\n",
                (std::string("a pointer naming \"") + attempt + "\" is refused").c_str(),
                refused ? "ok" : "FAIL");
    if (!refused) ++failures;
    check(f.counter.value == 0, "  and burns no attempt, since nothing was attempted");
  }
  {
    // A stray newline from an editor is the likeliest real corruption, and it would otherwise
    // fail to open in a way indistinguishable from a missing pack.
    Fixture f;
    f.storage.contents = good;
    f.storage.pointer = "default.uipack\n";
    check(f.run() == ui::LoadOutcome::BuiltInBadPointer,
          "a trailing newline is refused as a bad name, not misreported as missing");
  }
  {
    Fixture f;
    f.storage.contents = good;
    f.storage.pointer = "not-there.uipack";
    check(f.run() == ui::LoadOutcome::BuiltInPackMissing,
          "a pointer to a pack that is not there is the dangling-link case");
    check(f.counter.value == 0, "which also burns nothing — there was nothing to try");
  }
}

void invalidPackTests(const std::vector<uint8_t>& good) {
  std::printf("\n[a pack that is there but wrong — these DO burn an attempt]\n");

  {
    Fixture f;
    f.storage.contents = good;
    f.storage.contents[0] = 'X';  // magic
    check(f.run() == ui::LoadOutcome::BuiltInInvalid, "a corrupt pack falls back to built-in");
    check(f.loader.packStatus() == ui::PackStatus::BadMagic, "and says why");
    check(f.counter.value == 1, "burning an attempt, because it really was attempted");
  }
  {
    Fixture f;
    f.storage.contents = good;
    f.storage.shortRead = true;
    check(f.run() == ui::LoadOutcome::BuiltInInvalid, "a short read is treated as corruption");
    check(f.counter.value == 1, "and burns its attempt");
  }
  {
    Fixture f;
    f.storage.contents.assign(ui::PackLoader::kMaxPackBytes + 1, 0);
    check(f.run() == ui::LoadOutcome::BuiltInTooLarge, "an oversized pack is refused");
    check(f.counter.value == 1, "after burning its attempt, per the step order in §3.6");
  }
  {
    // A buffer smaller than the pack must be refused rather than overrun.
    Fixture f;
    f.storage.contents = good;
    f.buffer.resize(64);
    check(f.run() == ui::LoadOutcome::BuiltInTooLarge,
          "a pack larger than the supplied buffer is refused, not truncated into it");
  }
}

void bootLoopGuardTests(const std::vector<uint8_t>& good) {
  std::printf("\n[the anti-boot-loop guard — the part that must not be wrong]\n");

  // The scenario the guard exists for: a pack that passes every structural check and then takes
  // the renderer down, so noteSuccessfulRender is never reached.
  Fixture f;
  f.storage.contents = good;

  check(f.run() == ui::LoadOutcome::CardPack, "boot 1: the pack loads");
  check(f.counter.value == 1, "and the attempt stands, because no frame was drawn");

  check(f.run() == ui::LoadOutcome::CardPack, "boot 2: it loads again");
  check(f.counter.value == 2, "and the second attempt stands");

  const auto third = f.run();
  check(third == ui::LoadOutcome::BuiltInBootLoopGuard,
        "boot 3: the guard forces the built-in default");
  check(f.storage.pointerDeleted, "and clears the selection so boot 4 is clean");
  check(ui::loadOutcomeIsFailure(third), "the operator is told (§4.9)");

  // With the pointer gone the device is usable again, and no longer reports a fault.
  Fixture after;
  after.storage.contents = good;
  after.storage.hasPointer = false;
  after.counter.value = 0;
  check(after.run() == ui::LoadOutcome::BuiltInNoPointer,
        "boot 4: back to a normal built-in boot");

  // The guard must survive an unreadable card: it cannot clear the pointer, but it must still
  // refuse to load. Otherwise a card that intermittently fails to mount could evade it.
  Fixture stuck;
  stuck.storage.contents = good;
  stuck.storage.mountable = false;
  stuck.counter.value = 2;
  check(stuck.run() == ui::LoadOutcome::BuiltInBootLoopGuard,
        "the guard still refuses when the card cannot be mounted to clear the pointer");
  check(!stuck.storage.pointerDeleted, "the pointer is untouched, and that is survivable");
}

void abiTests(const std::vector<uint8_t>& good) {
  std::printf("\n[catalogue ABI — §4.7b]\n");

  Fixture f;
  f.storage.contents = good;
  // The fixture targets ABI 1. A firmware offering a NEWER catalogue must still accept it: the
  // completeness rule means an older pack can only be missing editors the firmware can supply.
  const auto outcome =
      f.loader.load(f.storage, f.counter, 7, f.buffer.data(), f.buffer.size(), &f.pack);
  check(outcome == ui::LoadOutcome::CardPack, "a pack built against an older catalogue is accepted");
}

}  // namespace

int main() {
  std::printf("ui::PackLoader — the boot selection ladder\n\n");

  const auto good = loadFixture();
  if (good.empty()) {
    std::printf("  fixture default.uipack not found — run `npm run test:exporter` first.\n");
    return 1;
  }

  happyPathTests(good);
  normalStateTests(good);
  badPointerTests(good);
  invalidPackTests(good);
  bootLoopGuardTests(good);
  abiTests(good);

  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
