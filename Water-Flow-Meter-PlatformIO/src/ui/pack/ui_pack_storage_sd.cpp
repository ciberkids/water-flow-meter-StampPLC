#include "ui/pack/ui_pack_storage_sd.h"

#include <Preferences.h>
#include <SD.h>
#include <SPI.h>

#include <cstring>

namespace plc {

namespace {

/** Builds "/ui/<name>" without allocating. Returns false if it would not fit. */
bool joinPath(char* out, std::size_t size, const char* name) {
  const std::size_t dirLen = std::strlen(SdPackStorage::kDirectory);
  const std::size_t nameLen = std::strlen(name);
  if (dirLen + 1 + nameLen + 1 > size) {
    return false;
  }
  std::memcpy(out, SdPackStorage::kDirectory, dirLen);
  out[dirLen] = '/';
  std::memcpy(out + dirLen + 1, name, nameLen);
  out[dirLen + 1 + nameLen] = '\0';
  return true;
}

/** Trims trailing whitespace and line endings from the pointer file's contents. */
void trimTrailing(char* text) {
  std::size_t length = std::strlen(text);
  while (length > 0) {
    const unsigned char c = static_cast<unsigned char>(text[length - 1]);
    if (c == '\n' || c == '\r' || c == ' ' || c == '\t') {
      text[--length] = '\0';
      continue;
    }
    break;
  }
}

}  // namespace

SdPackStorage::SdPackStorage(SpiArbiter& arbiter, uint8_t csPin, uint32_t frequencyHz)
    : arbiter_(arbiter), csPin_(csPin), frequencyHz_(frequencyHz) {}

bool SdPackStorage::takeBus() {
  const uint32_t startedAt = millis();
  arbiter_.requestCard(startedAt);
  // Spins only until the current frame closes. The arbiter's own timeout bounds this at
  // kFrameWaitTimeoutMs even if the renderer is wedged, so it cannot hang the caller.
  while (!arbiter_.cardGranted()) {
    arbiter_.update(millis());
    if (millis() - startedAt > SpiArbiter::kFrameWaitTimeoutMs * 2) {
      return false;  // belt and braces: the arbiter should already have granted by now
    }
    delay(1);
  }
  return true;
}

void SdPackStorage::releaseBus() { arbiter_.releaseCard(millis()); }

bool SdPackStorage::mount() {
  if (mounted_) {
    return true;
  }
  if (!takeBus()) {
    return false;
  }
  // Shares SPI with the LCD, so the card is given its own chip select and an explicitly modest
  // clock. SD.begin() re-initialises the bus, which is exactly why this must not overlap a frame.
  mounted_ = SD.begin(csPin_, SPI, frequencyHz_);
  releaseBus();
  return mounted_;
}

bool SdPackStorage::readPointer(char* out, std::size_t size) {
  if (!out || size == 0 || !mounted_) {
    return false;
  }
  if (!takeBus()) {
    return false;
  }
  bool ok = false;
  if (File file = SD.open(kPointerPath, FILE_READ)) {
    const std::size_t want = size - 1;
    const std::size_t read = file.readBytes(out, want);
    out[read] = '\0';
    file.close();
    // A stray newline from a desktop editor is the likeliest corruption of this file, and left in
    // place it fails to open in a way indistinguishable from a missing pack. The loader also
    // rejects control characters; trimming here means the ordinary case just works.
    trimTrailing(out);
    ok = out[0] != '\0';
  }
  releaseBus();
  return ok;
}

bool SdPackStorage::deletePointer() {
  if (!mounted_) {
    return false;
  }
  if (!takeBus()) {
    return false;
  }
  // FAT has no symlinks, which is why the selection is a pointer FILE (§3.1.1); removing it is
  // therefore an ordinary unlink and "no selection" is its absence.
  const bool ok = SD.remove(kPointerPath);
  releaseBus();
  return ok;
}

bool SdPackStorage::writePointer(const char* name) {
  if (!name || !mounted_) {
    return false;
  }
  if (!takeBus()) {
    return false;
  }
  bool ok = false;
  if (File file = SD.open(kPointerPath, FILE_WRITE)) {
    ok = file.print(name) > 0;
    file.close();
  }
  // Deliberately NOT released: §4.10 — the caller reboots immediately, and handing the bus back
  // would only buy a repaint of a screen about to disappear.
  return ok;
}

long SdPackStorage::packSize(const char* name) {
  if (!name || !mounted_) {
    return -1;
  }
  char path[96];
  if (!joinPath(path, sizeof(path), name)) {
    return -1;
  }
  if (!takeBus()) {
    return -1;
  }
  long size = -1;
  if (File file = SD.open(path, FILE_READ)) {
    size = static_cast<long>(file.size());
    file.close();
  }
  releaseBus();
  return size;
}

bool SdPackStorage::readPack(const char* name, uint8_t* buffer, std::size_t size) {
  if (!name || !buffer || !mounted_) {
    return false;
  }
  char path[96];
  if (!joinPath(path, sizeof(path), name)) {
    return false;
  }
  if (!takeBus()) {
    return false;
  }
  bool ok = false;
  if (File file = SD.open(path, FILE_READ)) {
    // A short read is corruption, not absence — the file was there a moment ago when packSize
    // measured it. The loader treats that distinctly, so the exact count matters.
    ok = file.read(buffer, size) == static_cast<int>(size);
    file.close();
  }
  releaseBus();
  return ok;
}

std::size_t SdPackStorage::listPacks(char (*names)[ui::PackLoader::kMaxNameBytes],
                                     std::size_t capacity) {
  if (!names || capacity == 0 || !mounted_) {
    return 0;
  }
  if (!takeBus()) {
    return 0;
  }
  std::size_t count = 0;
  if (File dir = SD.open(kDirectory)) {
    while (count < capacity) {
      File entry = dir.openNextFile();
      if (!entry) {
        break;
      }
      const char* path = entry.name();
      if (!entry.isDirectory() && path) {
        // basename: some cores hand back a full path here and others just the leaf.
        const char* leaf = std::strrchr(path, '/');
        leaf = leaf ? leaf + 1 : path;
        const std::size_t length = std::strlen(leaf);
        if (length >= 7 && std::strcmp(leaf + length - 7, ".uipack") == 0 && length < ui::PackLoader::kMaxNameBytes) {
          std::memcpy(names[count], leaf, length + 1);
          ++count;
        }
      }
      entry.close();
    }
    dir.close();
  }
  releaseBus();
  return count;
}

uint8_t NvsPackAttemptCounter::read() { return preferences_.getUChar(kKey, 0); }

void NvsPackAttemptCounter::write(uint8_t value) { preferences_.putUChar(kKey, value); }

}  // namespace plc
