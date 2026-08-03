#pragma once

#include <cstddef>
#include <cstdint>

#include "ui/pack/ui_pack.h"

namespace ui {

/**
 * The boot-time menu-pack selection ladder (Loadable_UI_Menu_Packs.md §3.6).
 *
 * The ladder is separated from the filesystem on purpose. Every interesting case here is a
 * FAILURE case — no card, a dangling pointer file, a truncated pack, a pack that passes every
 * structural check and still crashes the renderer — and none of those can be conveniently
 * produced on real hardware. So storage and the attempt counter are interfaces, the decision
 * logic is Arduino-free, and the whole ladder is exercised on a host with fakes.
 *
 * The anti-boot-loop guard is the part that must not be got wrong. A pack can satisfy §3.3 and
 * still take the renderer down; without a counter that pack stays selected and the device
 * reboot-loops with no way in. Two details from §3.6 carry the weight:
 *
 *   - the counter is incremented AFTER the cheap "is there anything to load" checks, so a
 *     missing card or an absent pointer is a normal state rather than a failed attempt. Burning
 *     an attempt on those would eventually delete a perfectly good selection;
 *   - it is cleared only after a SUCCESSFUL RENDER, not after validation. Clearing it on
 *     validation would prove nothing, because validation is exactly what the crashing pack
 *     already passed.
 */

/** What the ladder settled on, and why. Reported to the operator and over Modbus (§4.8). */
enum class LoadOutcome : uint8_t {
  CardPack = 0,          /**< A pack from the card is active. */
  BuiltInNoCard,         /**< No card, or it would not mount. Not an error. */
  BuiltInNoPointer,      /**< No /ui/active, or it is empty. Not an error. */
  BuiltInBadPointer,     /**< The pointer names something unusable — see §3.1.1. */
  BuiltInPackMissing,    /**< The pointer names a pack that is not there: a dangling link. */
  BuiltInTooLarge,       /**< Larger than the buffer the firmware is willing to hold. */
  BuiltInInvalid,        /**< Failed the §3.3 checks; `packStatus()` says which. */
  BuiltInBootLoopGuard   /**< Two attempts already failed; the selection was cleared. */
};

const char* loadOutcomeText(LoadOutcome outcome);

/** True when the operator should be told at boot (§4.9) — a selection was made and not honoured. */
bool loadOutcomeIsFailure(LoadOutcome outcome);

/**
 * Card access, in the few operations the ladder needs.
 *
 * Kept this narrow so the Arduino implementation is a thin adapter over SD and the test double
 * is a handful of lines. Nothing here knows about pack structure.
 */
class PackStorage {
 public:
  virtual ~PackStorage() = default;

  /** False when there is no card or it will not mount. */
  virtual bool mount() = 0;

  /** Reads /ui/active into `out`. False when absent or empty. */
  virtual bool readPointer(char* out, std::size_t size) = 0;

  /** Removes /ui/active, so the next boot runs the built-in default. */
  virtual bool deletePointer() = 0;

  /** Size of /ui/<name> in bytes, or -1 when it does not exist. */
  virtual long packSize(const char* name) = 0;

  /** Reads /ui/<name> into `buffer`. False on any short or failed read. */
  virtual bool readPack(const char* name, uint8_t* buffer, std::size_t size) = 0;
};

/** The attempt counter, which lives in NVS under `ui_pack_try` (§4.7). */
class PackAttemptCounter {
 public:
  virtual ~PackAttemptCounter() = default;
  virtual uint8_t read() = 0;
  virtual void write(uint8_t value) = 0;
};

class PackLoader {
 public:
  /** §3.6 step 2. Two failed attempts is enough to conclude the selection is at fault. */
  static constexpr uint8_t kMaxAttempts = 2;

  /** §3.6 step 7. A pack larger than this is refused rather than allowed to exhaust the heap. */
  static constexpr std::size_t kMaxPackBytes = 64 * 1024;

  /**
   * Walks the ladder and, on success, leaves `pack` valid over `buffer`.
   *
   * `buffer` must outlive the pack: it is read in place, so the bytes stay live for as long as
   * the pack is active. Nothing is allocated here.
   */
  LoadOutcome load(PackStorage& storage,
                   PackAttemptCounter& attempts,
                   uint16_t firmwareAbi,
                   uint8_t* buffer,
                   std::size_t bufferSize,
                   MenuPack* pack);

  /**
   * §3.6 step 11 — call once the first frame from a card pack has actually been drawn.
   *
   * This is what makes the guard mean anything. Until it is called the attempt stands, so a pack
   * that validates and then crashes the renderer is on its second strike next boot and its third
   * boot runs the built-in default.
   */
  void noteSuccessfulRender(PackAttemptCounter& attempts);

  /**
   * Longest pointer-file name accepted, including the terminator.
   *
   * Public because the storage adapter's directory listing has to agree with it — a name the
   * adapter is willing to report but the loader will not accept would appear in the selector and
   * then fail to load, which is the worst of both.
   */
  static constexpr std::size_t kMaxNameBytes = 64;

  /** Why a card pack was refused, when the outcome is BuiltInInvalid. */
  PackStatus packStatus() const { return packStatus_; }

  /** The pointer file's contents, for the boot report. Empty when none was read. */
  const char* selectedName() const { return name_; }

 private:
  /**
   * §3.1.1: a pointer naming anything with a path separator is refused.
   *
   * The pointer is operator-editable text on removable media, and it is concatenated onto a
   * directory prefix. Without this, `../../` in that file reads an arbitrary path — so the check
   * is a boundary, not tidiness.
   */
  static bool nameIsSafe(const char* name);

  char name_[kMaxNameBytes] = {};
  PackStatus packStatus_ = PackStatus::Ok;
};

}  // namespace ui
