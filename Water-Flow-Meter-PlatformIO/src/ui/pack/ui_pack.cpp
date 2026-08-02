#include "ui/pack/ui_pack.h"

#include <cstring>

namespace ui {

namespace {

/**
 * Little-endian readers.
 *
 * Explicit rather than casting a struct over the buffer. A cast would depend on the compiler's
 * padding and on the host's endianness, and the pack is written by a Node emitter on a
 * developer's machine and read by an ESP32 — so the layout has to be defined by this code, not
 * inherited from whatever the compiler chose. It also means an unaligned offset cannot fault,
 * which matters on a buffer whose contents came off an SD card.
 */
uint16_t readU16(const uint8_t* p) {
  return static_cast<uint16_t>(static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8));
}

uint32_t readU32(const uint8_t* p) {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

int16_t readI16(const uint8_t* p) { return static_cast<int16_t>(readU16(p)); }

constexpr char kMagic[6] = {'W', 'F', 'M', 'U', 'I', '\0'};

}  // namespace

const char* packStatusText(PackStatus status) {
  switch (status) {
    case PackStatus::Ok:                return "ok";
    case PackStatus::TooSmall:          return "file too small";
    case PackStatus::BadMagic:          return "not a menu pack";
    case PackStatus::BadFormatVersion:  return "wrong format version";
    case PackStatus::BadPayloadLength:  return "length mismatch";
    case PackStatus::BadCrc:            return "corrupt (CRC)";
    case PackStatus::BadOffset:         return "bad offset";
    case PackStatus::BadStringTable:    return "bad string table";
    case PackStatus::BadScreenIndex:    return "bad screen link";
    case PackStatus::BadCatalogueAbi:   return "needs newer firmware";
  }
  return "unknown";
}

uint32_t packCrc32(const uint8_t* bytes, std::size_t length) {
  // Table-free: 4 KB of lookup table is not worth spending on a device with 327 KB of RAM for a
  // checksum computed once per boot.
  uint32_t crc = 0xFFFFFFFFu;
  for (std::size_t i = 0; i < length; ++i) {
    crc ^= bytes[i];
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (0xEDB88320u & (~(crc & 1u) + 1u));
    }
  }
  return ~crc;
}

bool MenuPack::withinBuffer(uint32_t offset, std::size_t bytes) const {
  if (offset > length_) {
    return false;
  }
  // Compared as size_t after the offset check, so the addition cannot wrap.
  return static_cast<std::size_t>(offset) + bytes <= length_;
}

PackStatus MenuPack::validate(const uint8_t* bytes, std::size_t length, uint16_t firmwareAbi) {
  bytes_ = nullptr;
  length_ = 0;
  header_ = PackHeader{};

  if (!bytes || length < PackHeader::kSize) {
    return PackStatus::TooSmall;
  }
  if (std::memcmp(bytes, kMagic, sizeof(kMagic)) != 0) {
    return PackStatus::BadMagic;
  }

  PackHeader h{};
  h.formatVersion = readU16(bytes + 6);
  h.catalogueAbi  = readU16(bytes + 8);
  h.payloadBytes  = readU32(bytes + 10);
  h.crc32         = readU32(bytes + 14);
  h.levelCount    = readU16(bytes + 18);
  h.screenCount   = readU16(bytes + 20);
  h.levelsOffset  = readU32(bytes + 22);
  h.screensOffset = readU32(bytes + 26);
  h.themeOffset   = readU32(bytes + 30);
  h.stringsOffset = readU32(bytes + 34);
  h.stringsBytes  = readU32(bytes + 38);
  std::memcpy(h.label, bytes + 42, PackHeader::kLabelBytes);
  h.label[PackHeader::kLabelBytes] = '\0';

  if (h.formatVersion != PackHeader::kFormatVersion) {
    return PackStatus::BadFormatVersion;
  }
  if (static_cast<std::size_t>(h.payloadBytes) + PackHeader::kSize != length) {
    return PackStatus::BadPayloadLength;
  }

  // Checked BEFORE any offset is dereferenced. A corrupt card can produce offsets that pass the
  // bounds checks individually and still describe nonsense, so the cheapest way to reject the
  // whole class is to verify the payload is what was written.
  if (packCrc32(bytes + PackHeader::kSize, h.payloadBytes) != h.crc32) {
    return PackStatus::BadCrc;
  }

  // A pack may target an OLDER catalogue: the completeness rule means it can then only be
  // missing editors this firmware can supply itself. A newer one may reference values that do
  // not exist here, so it is refused rather than partially honoured.
  if (h.catalogueAbi > firmwareAbi) {
    return PackStatus::BadCatalogueAbi;
  }

  bytes_ = bytes;
  length_ = length;
  header_ = h;

  // themeOffset is bounds-checked even though the theme is not read yet. Leaving a declared
  // section unvalidated because nothing consumes it means the day something does, a corrupt
  // offset that has been accepted for months becomes a fault with no obvious cause. A
  // single-byte-corruption sweep over the header found exactly this: four bytes of themeOffset
  // and two of the level record could be flipped and the pack still validated.
  const bool sectionsFit =
      withinBuffer(h.levelsOffset, static_cast<std::size_t>(h.levelCount) * kLevelRecordBytes) &&
      withinBuffer(h.screensOffset, static_cast<std::size_t>(h.screenCount) * kScreenRecordBytes) &&
      withinBuffer(h.themeOffset, 2) &&
      withinBuffer(h.stringsOffset, h.stringsBytes);
  if (!sectionsFit) {
    bytes_ = nullptr;
    return PackStatus::BadOffset;
  }

  // Levels describe which screens belong to which page ring, so a level naming a screen that
  // does not exist is as dangerous as a flow doing so.
  for (uint16_t i = 0; i < h.levelCount; ++i) {
    const uint32_t at = h.levelsOffset + static_cast<uint32_t>(i) * kLevelRecordBytes;
    if (!withinBuffer(at, kLevelRecordBytes)) {
      bytes_ = nullptr;
      return PackStatus::BadOffset;
    }
    const uint8_t* p = bytes + at;
    const uint16_t pageCount = readU16(p + 4);
    const uint16_t firstScreen = readU16(p + 6);
    if (static_cast<uint32_t>(firstScreen) + pageCount > h.screenCount) {
      bytes_ = nullptr;
      return PackStatus::BadScreenIndex;
    }
    if (!stringAt(readU32(p))) {
      bytes_ = nullptr;
      return PackStatus::BadStringTable;
    }
  }

  // Every string is read as NUL-terminated, so the block must end in one. Without this a string
  // at the very end would run off the buffer.
  if (h.stringsBytes == 0 || bytes[h.stringsOffset + h.stringsBytes - 1] != '\0') {
    bytes_ = nullptr;
    return PackStatus::BadStringTable;
  }

  // Walk every screen once, up front. The alternative is checking on each access, which means a
  // pack that looks fine at boot can still fault on the first press of an untested button —
  // exactly the failure that is impossible to diagnose in the field.
  for (uint16_t i = 0; i < h.screenCount; ++i) {
    PackScreen screen{};
    if (!screenAt(i, &screen)) {
      bytes_ = nullptr;
      return PackStatus::BadOffset;
    }
    if (!withinBuffer(screen.elementsOffset,
                      static_cast<std::size_t>(screen.elementCount) * kElementRecordBytes) ||
        !withinBuffer(screen.flowsOffset,
                      static_cast<std::size_t>(screen.flowCount) * kFlowRecordBytes)) {
      bytes_ = nullptr;
      return PackStatus::BadOffset;
    }
    if (!stringAt(screen.idStr) || !stringAt(screen.nameStr)) {
      bytes_ = nullptr;
      return PackStatus::BadStringTable;
    }
    for (uint16_t f = 0; f < screen.flowCount; ++f) {
      PackFlow flow{};
      if (!flowAt(screen, f, &flow)) {
        bytes_ = nullptr;
        return PackStatus::BadOffset;
      }
      if (flow.targetScreenIndex != kNoTargetScreen && flow.targetScreenIndex >= h.screenCount) {
        bytes_ = nullptr;
        return PackStatus::BadScreenIndex;
      }
      if (flow.actionStr != 0 && !stringAt(flow.actionStr)) {
        bytes_ = nullptr;
        return PackStatus::BadStringTable;
      }
    }
    for (uint16_t e = 0; e < screen.elementCount; ++e) {
      PackElement element{};
      if (!elementAt(screen, e, &element)) {
        bytes_ = nullptr;
        return PackStatus::BadOffset;
      }
      if ((element.contentStr != 0 && !stringAt(element.contentStr)) ||
          (element.bindingStr != 0 && !stringAt(element.bindingStr))) {
        bytes_ = nullptr;
        return PackStatus::BadStringTable;
      }
    }
  }

  return PackStatus::Ok;
}

bool MenuPack::screenAt(uint16_t index, PackScreen* out) const {
  if (!bytes_ || !out || index >= header_.screenCount) {
    return false;
  }
  const uint32_t at = header_.screensOffset + static_cast<uint32_t>(index) * kScreenRecordBytes;
  if (!withinBuffer(at, kScreenRecordBytes)) {
    return false;
  }
  const uint8_t* p = bytes_ + at;
  out->idStr          = readU32(p);
  out->nameStr        = readU32(p + 4);
  out->elementCount   = readU16(p + 8);
  out->flowCount      = readU16(p + 10);
  out->elementsOffset = readU32(p + 12);
  // flowsOffset shares the record's tail; see the emitter for the exact packing.
  out->flowsOffset    = out->elementsOffset +
                        static_cast<uint32_t>(out->elementCount) * kElementRecordBytes;
  return true;
}

bool MenuPack::elementAt(const PackScreen& screen, uint16_t index, PackElement* out) const {
  if (!bytes_ || !out || index >= screen.elementCount) {
    return false;
  }
  const uint32_t at = screen.elementsOffset + static_cast<uint32_t>(index) * kElementRecordBytes;
  if (!withinBuffer(at, kElementRecordBytes)) {
    return false;
  }
  const uint8_t* p = bytes_ + at;
  out->kind       = p[0];
  out->align      = p[1];
  out->emphasis   = p[2];
  out->x          = readI16(p + 4);
  out->y          = readI16(p + 6);
  out->width      = readI16(p + 8);
  out->height     = readI16(p + 10);
  out->contentStr = readU32(p + 12);
  out->bindingStr = readU32(p + 16);
  return true;
}

bool MenuPack::flowAt(const PackScreen& screen, uint16_t index, PackFlow* out) const {
  if (!bytes_ || !out || index >= screen.flowCount) {
    return false;
  }
  const uint32_t at = screen.flowsOffset + static_cast<uint32_t>(index) * kFlowRecordBytes;
  if (!withinBuffer(at, kFlowRecordBytes)) {
    return false;
  }
  const uint8_t* p = bytes_ + at;
  out->triggerKind       = p[0];
  out->button            = p[1];
  out->gesture           = p[2];
  out->durationMs        = readU32(p + 4);
  out->targetScreenIndex = readU16(p + 8);
  out->actionStr         = readU32(p + 12);
  return true;
}

const char* MenuPack::stringAt(uint32_t offset) const {
  if (!bytes_) {
    return nullptr;
  }
  // §3.2: "every *Str is an offset into this block" — RELATIVE to stringsOffset, not absolute
  // into the buffer. Reading it as absolute made every string offset fall short of the block and
  // the whole pack read as BadStringTable. The round-trip test caught that on its first run,
  // which is the entire reason the format is checked by execution rather than by review.
  //
  // Offset 0 is the reserved "no string" sentinel: the emitter opens the block with a lone NUL
  // that nothing else can occupy, so a zero *Str means absent rather than empty.
  if (offset == 0 || offset >= header_.stringsBytes) {
    return nullptr;
  }
  return reinterpret_cast<const char*>(bytes_ + header_.stringsOffset + offset);
}

uint16_t MenuPack::findScreen(const char* id) const {
  if (!bytes_ || !id) {
    return kNoTargetScreen;
  }
  for (uint16_t i = 0; i < header_.screenCount; ++i) {
    PackScreen screen{};
    if (!screenAt(i, &screen)) {
      continue;
    }
    const char* candidate = stringAt(screen.idStr);
    if (candidate && std::strcmp(candidate, id) == 0) {
      return i;
    }
  }
  return kNoTargetScreen;
}

}  // namespace ui
