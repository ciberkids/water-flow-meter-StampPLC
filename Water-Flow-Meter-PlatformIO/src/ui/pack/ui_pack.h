#pragma once

#include <cstddef>
#include <cstdint>

namespace ui {

/**
 * A loadable UI menu pack, read in place from a byte buffer.
 *
 * Loadable_UI_Menu_Packs.md §3.2. The pack is **relocatable**: `ui_exporter::Screen` holds
 * `const char*` and `const Element*`, which are absolute addresses fixed at link time, so a pack
 * cannot contain pointers. Every internal reference is a byte offset from the start of the
 * buffer, and this class reads the structure where it lies rather than parsing it into a graph.
 * One allocation instead of hundreds, and no fragmentation on a device with 327 KB of RAM.
 *
 * The firmware must not trust the card (§3.3). Every accessor here is bounds-checked against the
 * buffer length, and `validate()` refuses a pack before any accessor is reachable. A pack comes
 * off removable media that anyone can write, so a malformed one is an expected input rather than
 * an exceptional condition — and this runs on a device with no MMU, where an out-of-range read is
 * a reboot at best.
 *
 * Deliberately free of Arduino and of the filesystem: the loader supplies bytes, and this decides
 * whether they mean anything. That makes the whole format testable on a host, which matters
 * because a corrupt pack is the one input we cannot conveniently produce on real hardware.
 */

/** Why a pack was refused. Reported to the operator and over Modbus, so each is distinct. */
enum class PackStatus : uint8_t {
  Ok = 0,
  TooSmall,          /**< Shorter than the fixed header. */
  BadMagic,          /**< Not a pack at all — most likely the wrong file. */
  BadFormatVersion,  /**< A pack from a newer or older layout. */
  BadPayloadLength,  /**< payloadBytes disagrees with the buffer length. */
  BadCrc,            /**< Payload corrupt: a bad card, or an interrupted write. */
  BadOffset,         /**< A section offset or length falls outside the buffer. */
  BadStringTable,    /**< The string block is not NUL-terminated at its end. */
  BadScreenIndex,    /**< A level or flow references a screen that does not exist. */
  BadCatalogueAbi    /**< Built against a catalogue this firmware does not offer (§4.7b). */
};

const char* packStatusText(PackStatus status);

/** The fixed 64-byte header of §3.2, read field by field rather than cast over. */
struct PackHeader {
  static constexpr std::size_t kSize = 64;
  static constexpr uint16_t kFormatVersion = 1;
  static constexpr std::size_t kLabelBytes = 20;

  uint16_t formatVersion = 0;
  uint16_t catalogueAbi = 0;
  uint32_t payloadBytes = 0;
  uint32_t crc32 = 0;
  uint16_t levelCount = 0;
  uint16_t screenCount = 0;
  uint32_t levelsOffset = 0;
  uint32_t screensOffset = 0;
  uint32_t themeOffset = 0;
  uint32_t stringsOffset = 0;
  uint32_t stringsBytes = 0;
  char label[kLabelBytes + 1] = {};
};

/** One screen's fixed record. Offsets are from the start of the buffer. */
struct PackScreen {
  uint32_t idStr = 0;
  uint32_t nameStr = 0;
  uint16_t elementCount = 0;
  uint32_t elementsOffset = 0;
  uint16_t flowCount = 0;
  uint32_t flowsOffset = 0;
};

struct PackElement {
  uint8_t kind = 0;
  uint8_t align = 0;
  uint8_t emphasis = 0;
  int16_t x = 0;
  int16_t y = 0;
  int16_t width = 0;
  int16_t height = 0;
  uint32_t contentStr = 0;
  uint32_t bindingStr = 0;
};

struct PackFlow {
  uint8_t triggerKind = 0;
  uint8_t button = 0;
  uint8_t gesture = 0;
  uint32_t durationMs = 0;
  /** An INDEX, not a string: links are resolved at build time so the device checks one int. */
  uint16_t targetScreenIndex = 0;
  uint32_t actionStr = 0;
};

/** Sentinel for "this flow does not navigate", distinct from screen 0. */
constexpr uint16_t kNoTargetScreen = 0xFFFF;

class MenuPack {
 public:
  static constexpr std::size_t kScreenRecordBytes = 16;
  static constexpr std::size_t kLevelRecordBytes = 8;
  static constexpr std::size_t kElementRecordBytes = 20;
  static constexpr std::size_t kFlowRecordBytes = 16;

  /**
   * Validates the buffer and, on success, leaves this object usable.
   *
   * `firmwareAbi` is the catalogue version this build offers. A pack targeting a NEWER
   * catalogue is refused; an older one is accepted, because the completeness rule means an
   * older pack can only be missing editors the firmware can supply itself (§4.7b).
   *
   * Nothing is retained on failure: an invalid pack leaves the object unusable rather than
   * half-initialised, so a caller that ignores the return value cannot read garbage.
   */
  PackStatus validate(const uint8_t* bytes, std::size_t length, uint16_t firmwareAbi);

  bool valid() const { return bytes_ != nullptr; }
  const PackHeader& header() const { return header_; }

  uint16_t screenCount() const { return header_.screenCount; }
  uint16_t levelCount() const { return header_.levelCount; }

  /** A screen record by index, or false when out of range. */
  bool screenAt(uint16_t index, PackScreen* out) const;
  bool elementAt(const PackScreen& screen, uint16_t index, PackElement* out) const;
  bool flowAt(const PackScreen& screen, uint16_t index, PackFlow* out) const;

  /**
   * A NUL-terminated string at an offset into the string block, or nullptr.
   *
   * Returns a pointer INTO the buffer — no copy. The caller must not outlive the buffer, which
   * the loader owns for as long as a pack is selected.
   */
  const char* stringAt(uint32_t offset) const;

  /** Index of the screen whose id matches, or kNoTargetScreen. */
  uint16_t findScreen(const char* id) const;

 private:
  bool withinBuffer(uint32_t offset, std::size_t bytes) const;

  const uint8_t* bytes_ = nullptr;
  std::size_t length_ = 0;
  PackHeader header_{};
};

/** CRC-32 (IEEE 802.3, reflected, polynomial 0xEDB88320) — the same one zlib computes. */
uint32_t packCrc32(const uint8_t* bytes, std::size_t length);

}  // namespace ui
