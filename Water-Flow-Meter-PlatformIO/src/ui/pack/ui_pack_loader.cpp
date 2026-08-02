#include "ui/pack/ui_pack_loader.h"

#include <cstring>

namespace ui {

const char* loadOutcomeText(LoadOutcome outcome) {
  switch (outcome) {
    case LoadOutcome::CardPack:             return "card pack active";
    case LoadOutcome::BuiltInNoCard:        return "no card";
    case LoadOutcome::BuiltInNoPointer:     return "no menu selected";
    case LoadOutcome::BuiltInBadPointer:    return "bad selection";
    case LoadOutcome::BuiltInPackMissing:   return "selected menu missing";
    case LoadOutcome::BuiltInTooLarge:      return "menu too large";
    case LoadOutcome::BuiltInInvalid:       return "menu rejected";
    case LoadOutcome::BuiltInBootLoopGuard: return "menu disabled after 2 failures";
  }
  return "unknown";
}

bool loadOutcomeIsFailure(LoadOutcome outcome) {
  switch (outcome) {
    // A card pack running, no card at all, or no selection made are all NORMAL states. §3.6 is
    // explicit that a missing card is not a failed attempt, and the same reasoning applies to
    // what the operator is shown: reporting a fault for a device the owner never configured
    // would train them to ignore the report.
    case LoadOutcome::CardPack:
    case LoadOutcome::BuiltInNoCard:
    case LoadOutcome::BuiltInNoPointer:
      return false;
    case LoadOutcome::BuiltInBadPointer:
    case LoadOutcome::BuiltInPackMissing:
    case LoadOutcome::BuiltInTooLarge:
    case LoadOutcome::BuiltInInvalid:
    case LoadOutcome::BuiltInBootLoopGuard:
      return true;
  }
  return true;
}

bool PackLoader::nameIsSafe(const char* name) {
  if (!name || name[0] == '\0') {
    return false;
  }
  std::size_t length = 0;
  for (const char* p = name; *p != '\0'; ++p, ++length) {
    if (length >= kMaxNameBytes - 1) {
      return false;
    }
    const char c = *p;
    if (c == '/' || c == '\\') {
      return false;  // §3.1.1 — no path separators, so "../.." cannot escape /ui.
    }
    // Control characters and whitespace are almost always a stray newline from an editor, and a
    // filename with a trailing CR fails to open in a way that looks like a missing pack.
    if (static_cast<unsigned char>(c) <= 0x20 || static_cast<unsigned char>(c) == 0x7F) {
      return false;
    }
  }
  return true;
}

LoadOutcome PackLoader::load(PackStorage& storage,
                             PackAttemptCounter& attempts,
                             uint16_t firmwareAbi,
                             uint8_t* buffer,
                             std::size_t bufferSize,
                             MenuPack* pack) {
  name_[0] = '\0';
  packStatus_ = PackStatus::Ok;

  // ── Step 2: the guard, before anything else is touched ──
  if (attempts.read() >= kMaxAttempts) {
    // Best-effort. If the card is unreadable the pointer cannot be cleared, and that is fine:
    // the counter alone already forces the built-in default, so the device stays usable either
    // way. Mounting is attempted only to clear it, and a failure here changes nothing.
    if (storage.mount()) {
      storage.deletePointer();
    }
    return LoadOutcome::BuiltInBootLoopGuard;
  }

  // ── Steps 3-5: the cheap checks. None of these burns an attempt. ──
  if (!storage.mount()) {
    return LoadOutcome::BuiltInNoCard;
  }
  if (!storage.readPointer(name_, sizeof(name_))) {
    name_[0] = '\0';
    return LoadOutcome::BuiltInNoPointer;
  }
  if (!nameIsSafe(name_)) {
    return LoadOutcome::BuiltInBadPointer;
  }

  const long size = storage.packSize(name_);
  if (size < 0) {
    return LoadOutcome::BuiltInPackMissing;  // the dangling-pointer case §3.1.1 describes
  }

  // ── Step 6: from here a failure counts, because from here we are actually trying ──
  attempts.write(static_cast<uint8_t>(attempts.read() + 1));

  // ── Step 7 ──
  const auto byteCount = static_cast<std::size_t>(size);
  if (byteCount > kMaxPackBytes || byteCount > bufferSize) {
    return LoadOutcome::BuiltInTooLarge;
  }
  if (!storage.readPack(name_, buffer, byteCount)) {
    // A short read is corruption, not absence — the file was there a moment ago. Treated as
    // invalid so it burns the attempt it already has.
    packStatus_ = PackStatus::BadPayloadLength;
    return LoadOutcome::BuiltInInvalid;
  }

  // ── Step 8 ──
  if (!pack) {
    packStatus_ = PackStatus::TooSmall;
    return LoadOutcome::BuiltInInvalid;
  }
  packStatus_ = pack->validate(buffer, byteCount, firmwareAbi);
  if (packStatus_ != PackStatus::Ok) {
    return LoadOutcome::BuiltInInvalid;
  }

  // Step 9 (completeness patching) and step 10 (activation) belong to the caller: the loader's
  // job ends once there is a validated pack. The attempt stands until noteSuccessfulRender.
  return LoadOutcome::CardPack;
}

void PackLoader::noteSuccessfulRender(PackAttemptCounter& attempts) {
  if (attempts.read() != 0) {
    attempts.write(0);
  }
}

}  // namespace ui
