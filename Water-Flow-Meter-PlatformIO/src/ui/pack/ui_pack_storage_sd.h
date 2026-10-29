#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include "storage/nvs_write_health.h"

#include "bus/spi_arbiter.h"
#include "ui/pack/ui_pack_loader.h"

class Preferences;

namespace plc {

/**
 * The SD-card implementation of `ui::PackStorage`.
 *
 * Deliberately thin. All the decision-making lives in `ui::PackLoader`, which is Arduino-free
 * and host-tested against fakes; everything here is the smallest possible translation from that
 * interface to `SD.h`. The split is what allows every failure rung of §3.6 — no card, a dangling
 * pointer, a truncated pack — to be covered by tests, leaving only "does this board's card
 * actually mount" for the bench.
 *
 * Every operation takes the shared SPI bus through `SpiArbiter` first (§4.10) and releases it
 * afterwards, so a card read can never interrupt a frame. Requests block only until the current
 * frame closes, which is bounded: the renderer's slowest cadence is ~80 ms, and the arbiter's own
 * timeout is 500 ms.
 */
class SdPackStorage : public ui::PackStorage {
 public:
  /** Directory the packs and the pointer file live in (§3.1). */
  static constexpr const char* kDirectory = "/ui";
  static constexpr const char* kPointerPath = "/ui/active";

  /**
   * The ending a file must have for `listPacks` to offer it in the Select Menu (§3.1).
   *
   * Compared byte for byte, so `MENU.UIPACK` is not listed even though FAT would open it under either
   * case. A constant rather than the literal `listPacks` used to carry, because two other places have
   * to agree with it and both READ it from here: `npm run export:pack`
   * (`web/mockup/tools/exporter/packRules.ts`), which refuses to build a pack the device would never
   * list, and the wiki's SD Card page. A literal in the `.cpp` was a fact only this file could see (N-g).
   */
  static constexpr const char* kPackSuffix = ".uipack";

  /**
   * Whether `listPacks` offers a directory entry whose leaf name is `leaf` — the Select Menu's rule.
   *
   * It ends in the suffix above, and it is shorter than `ui::PackLoader::kMaxNameBytes`, so it fits the
   * selector's name buffer with its terminator. Defined here, in a header that never includes `SD.h`, so
   * the host suite can drive it (`test/host/pack_listing_test.cpp`). Until the review of N-g it was an
   * expression inside `listPacks`, behind `#include <SD.h>`, which no test could reach, while
   * `npm run export:pack` applies the same rule (`packRules.ts::checkPackFileName`). Changing the
   * comparison there would have left the exporter and the Select Menu disagreeing, with nothing to
   * notice. A pure move: it accepts exactly what the expression accepted, a bare suffix included, which
   * the exporter refuses on its own stricter grounds.
   */
  static bool isListablePackName(const char* leaf) {
    if (!leaf) {
      return false;
    }
    const std::size_t length = std::strlen(leaf);
    const std::size_t suffixLength = std::strlen(kPackSuffix);
    return length >= suffixLength && std::strcmp(leaf + length - suffixLength, kPackSuffix) == 0 &&
           length < ui::PackLoader::kMaxNameBytes;
  }

  /**
   * The buffer `packSize` and `readPack` build the directory, a separator and the name in, terminator
   * included.
   *
   * Named, and tied to `kMaxNameBytes` by the static_assert after this class, because both functions
   * used a bare `char path[96]`: a third limit on the name that nothing compared with the other two. A
   * `kMaxNameBytes` raised past what it holds would have let the exporter build, and the Select Menu
   * list, a name these two then failed to open, reported at boot as a missing pack.
   */
  static constexpr std::size_t kPathBytes = 96;

  /**
   * `csPin` is the card's chip select — 10 on the StampPLC, where the LCD holds 12.
   * `arbiter` must outlive this object.
   */
  SdPackStorage(SpiArbiter& arbiter, uint8_t csPin, uint32_t frequencyHz);

  bool mount() override;
  bool readPointer(char* out, std::size_t size) override;
  bool deletePointer() override;
  long packSize(const char* name) override;
  bool readPack(const char* name, uint8_t* buffer, std::size_t size) override;

  /**
   * Writes the pointer file — the Select Menu page's commit (§3.4).
   *
   * Not part of PackStorage: the loader only ever reads, and giving it a write it never uses
   * would widen an interface whose narrowness is the point.
   */
  bool writePointer(const char* name);

  /** True once mount() has succeeded, so callers can skip work when there is no card. */
  bool mounted() const { return mounted_; }

  /**
   * Lists `*.uipack` files, newest-first order not guaranteed.
   *
   * Fills `names` with up to `capacity` NUL-terminated entries of `ui::PackLoader::kMaxNameBytes`
   * and returns how many were written. This is the selector's directory scan, and the one card
   * operation that happens while the display is live.
   */
  std::size_t listPacks(char (*names)[ui::PackLoader::kMaxNameBytes], std::size_t capacity);

 private:
  /** Grants the bus or gives up; see SpiArbiter. */
  bool takeBus();
  void releaseBus();

  SpiArbiter& arbiter_;
  uint8_t csPin_;
  uint32_t frequencyHz_;
  bool mounted_ = false;
};

// The longest name `isListablePackName` accepts is `kMaxNameBytes - 1` bytes, and `joinPath` needs the
// directory, a '/', that name and a terminator: `kMaxNameBytes` plus the directory and the separator.
// A build failure here, rather than a pack the Select Menu lists and the loader cannot open.
static_assert(std::char_traits<char>::length(SdPackStorage::kDirectory) + 1 +
                      ui::PackLoader::kMaxNameBytes <=
                  SdPackStorage::kPathBytes,
              "SdPackStorage::kPathBytes cannot hold the longest name listPacks offers");

/** `ui::PackAttemptCounter` over NVS, keyed `ui_pack_try` (§4.7). */
class NvsPackAttemptCounter : public ui::PackAttemptCounter {
 public:
  static constexpr const char* kKey = "ui_pack_try";

  /**
   * `health` is optional and records a failed write under `StorageFault::PackAttemptCounter` (`DF25`).
   *
   * The counter records its OWN outcome rather than returning it, so `PackLoader` — which is
   * Arduino-free and knows nothing about storage — does not grow a health parameter to carry one bool
   * through two call sites. This was the last of the twelve fault codes with no author, and a code
   * nothing reports is the defect DF25 is about.
   */
  explicit NvsPackAttemptCounter(Preferences& preferences, plc::NvsWriteHealth* health = nullptr)
      : preferences_(preferences), health_(health) {}

  uint8_t read() override;
  void write(uint8_t value) override;

 private:
  Preferences& preferences_;
  plc::NvsWriteHealth* health_ = nullptr;
};

}  // namespace plc
