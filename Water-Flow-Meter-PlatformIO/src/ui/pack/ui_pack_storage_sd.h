#pragma once

#include <cstddef>
#include <cstdint>

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

/** `ui::PackAttemptCounter` over NVS, keyed `ui_pack_try` (§4.7). */
class NvsPackAttemptCounter : public ui::PackAttemptCounter {
 public:
  static constexpr const char* kKey = "ui_pack_try";

  explicit NvsPackAttemptCounter(Preferences& preferences) : preferences_(preferences) {}

  uint8_t read() override;
  void write(uint8_t value) override;

 private:
  Preferences& preferences_;
};

}  // namespace plc
