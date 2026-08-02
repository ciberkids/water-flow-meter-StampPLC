// Host tests for the menu-pack reader, and the round trip that makes the two implementations of
// the format agree.
//
// The format is defined twice — by web/mockup/tools/exporter/packEmitter.ts and by
// src/ui/pack/ui_pack.cpp — because one runs in Node on a developer's machine and the other on an
// ESP32. Two implementations of one contract is precisely the shape of bug this project keeps
// finding, so they are not reconciled by review: this test reads the REAL pack the REAL emitter
// produced from the REAL 48-screen dataset, and checks it against the generated C++ table that
// the firmware itself renders. If the emitter and the reader ever disagree about a byte, one of
// these assertions says which.
//
// The fuzz section matters just as much. A pack comes off removable media that anyone can write,
// so a malformed one is an ordinary input, not an exceptional condition — and this runs on a
// device with no MMU, where an out-of-range read is a reboot at best.
#include "ui/pack/ui_pack.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "ui/generated/GeneratedUi.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-66s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

/** The fixture the emitter wrote, or an empty vector when it is missing. */
std::vector<uint8_t> loadFixture() {
  const char* candidates[] = {
      "../web/mockup/tests/fixtures/default.uipack",
      "web/mockup/tests/fixtures/default.uipack",
  };
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

constexpr uint16_t kFirmwareAbi = 1;

void crcTests() {
  std::printf("[crc32 — must match the emitter's implementation exactly]\n");

  // Known-answer vectors for CRC-32/ISO-HDLC. If these drift the two sides silently stop
  // agreeing about whether a card is corrupt, which is the worst possible way to be wrong.
  const uint8_t empty[1] = {0};
  check(ui::packCrc32(empty, 0) == 0x00000000u, "the empty input is 0");

  const char* check_str = "123456789";
  check(ui::packCrc32(reinterpret_cast<const uint8_t*>(check_str), 9) == 0xCBF43926u,
        "\"123456789\" is 0xCBF43926, the standard check value");

  const char* a = "a";
  check(ui::packCrc32(reinterpret_cast<const uint8_t*>(a), 1) == 0xE8B7BE43u,
        "\"a\" is 0xE8B7BE43");
}

void roundTripTests(const std::vector<uint8_t>& bytes) {
  std::printf("\n[round trip — the emitter's pack against the generated table]\n");

  ui::MenuPack pack;
  const auto status = pack.validate(bytes.data(), bytes.size(), kFirmwareAbi);
  check(status == ui::PackStatus::Ok, "the emitted pack validates");
  if (status != ui::PackStatus::Ok) {
    std::printf("      status: %s\n", ui::packStatusText(status));
    return;
  }

  check(pack.screenCount() == ui_exporter::kGeneratedScreenCount,
        "screen count matches the generated table");
  check(std::strcmp(pack.header().label, "Default") == 0, "the label survives the header");

  // Every screen, element and flow, compared against what the firmware would otherwise render.
  std::size_t mismatches = 0;
  std::size_t elementsChecked = 0;
  std::size_t flowsChecked = 0;

  for (uint16_t i = 0; i < pack.screenCount(); ++i) {
    ui::PackScreen screen{};
    if (!pack.screenAt(i, &screen)) {
      ++mismatches;
      continue;
    }
    const auto& expected = ui_exporter::kGeneratedScreens[i];
    const char* id = pack.stringAt(screen.idStr);

    if (!id || std::strcmp(id, expected.id) != 0) {
      std::printf("      screen %u: id \"%s\" != \"%s\"\n", i, id ? id : "(null)", expected.id);
      ++mismatches;
      continue;
    }
    if (screen.elementCount != expected.elementCount) {
      std::printf("      %s: %u elements != %zu\n", id, screen.elementCount, expected.elementCount);
      ++mismatches;
    }
    if (screen.flowCount != expected.flowCount) {
      std::printf("      %s: %u flows != %zu\n", id, screen.flowCount, expected.flowCount);
      ++mismatches;
    }

    for (uint16_t e = 0; e < screen.elementCount && e < expected.elementCount; ++e) {
      ui::PackElement element{};
      if (!pack.elementAt(screen, e, &element)) {
        ++mismatches;
        continue;
      }
      const auto& want = expected.elements[e];
      ++elementsChecked;
      if (element.x != want.x || element.y != want.y) {
        std::printf("      %s/%u: position %d,%d != %d,%d\n", id, e, element.x, element.y, want.x,
                    want.y);
        ++mismatches;
      }
      if (element.kind != static_cast<uint8_t>(want.type)) {
        std::printf("      %s/%u: kind %u != %u\n", id, e, element.kind,
                    static_cast<uint8_t>(want.type));
        ++mismatches;
      }
      // A dropped binding is the failure that renders a blank element on the device.
      const char* binding = pack.stringAt(element.bindingStr);
      const bool bindingAgrees = (binding == nullptr && want.bindingId == nullptr) ||
                                 (binding && want.bindingId && std::strcmp(binding, want.bindingId) == 0);
      if (!bindingAgrees) {
        std::printf("      %s/%u: binding \"%s\" != \"%s\"\n", id, e, binding ? binding : "(none)",
                    want.bindingId ? want.bindingId : "(none)");
        ++mismatches;
      }
    }

    for (uint16_t f = 0; f < screen.flowCount && f < expected.flowCount; ++f) {
      ui::PackFlow flow{};
      if (!pack.flowAt(screen, f, &flow)) {
        ++mismatches;
        continue;
      }
      const auto& want = expected.flows[f];
      ++flowsChecked;
      if (flow.triggerKind != static_cast<uint8_t>(want.trigger) ||
          flow.button != static_cast<uint8_t>(want.button) ||
          flow.gesture != static_cast<uint8_t>(want.gesture)) {
        std::printf("      %s flow %u: trigger/button/gesture %u/%u/%u != %u/%u/%u\n", id, f,
                    flow.triggerKind, flow.button, flow.gesture,
                    static_cast<uint8_t>(want.trigger), static_cast<uint8_t>(want.button),
                    static_cast<uint8_t>(want.gesture));
        ++mismatches;
      }
      if (flow.durationMs != want.timeoutMs) {
        std::printf("      %s flow %u: duration %u != %u\n", id, f, flow.durationMs,
                    want.timeoutMs);
        ++mismatches;
      }
      const char* action = pack.stringAt(flow.actionStr);
      const bool actionAgrees = (action == nullptr && want.actionId == nullptr) ||
                                (action && want.actionId && std::strcmp(action, want.actionId) == 0);
      if (!actionAgrees) {
        std::printf("      %s flow %u: action \"%s\" != \"%s\"\n", id, f, action ? action : "(none)",
                    want.actionId ? want.actionId : "(none)");
        ++mismatches;
      }
      // targetScreenId became an index at build time; it must resolve back to the same screen.
      if (want.targetScreenId) {
        const bool resolves =
            flow.targetScreenIndex < pack.screenCount() &&
            std::strcmp(ui_exporter::kGeneratedScreens[flow.targetScreenIndex].id,
                        want.targetScreenId) == 0;
        if (!resolves) {
          std::printf("      %s flow %u: target index %u does not resolve to \"%s\"\n", id, f,
                      flow.targetScreenIndex, want.targetScreenId);
          ++mismatches;
        }
      } else if (flow.targetScreenIndex != ui::kNoTargetScreen) {
        std::printf("      %s flow %u: has a target index but the table has none\n", id, f);
        ++mismatches;
      }
    }
  }

  std::printf("      compared %zu elements and %zu flows across %u screens\n", elementsChecked,
              flowsChecked, pack.screenCount());
  check(mismatches == 0, "every screen, element and flow round-trips identically");

  // Screen lookup by id, which the loader uses to find the router's required screens.
  check(pack.findScreen("info-p0-global-status") == 0, "findScreen locates the root");
  check(pack.findScreen("no-such-screen") == ui::kNoTargetScreen,
        "and reports a miss rather than guessing");
}

void fuzzTests(const std::vector<uint8_t>& good) {
  std::printf("\n[malformed packs must be refused, never trusted — §3.3]\n");

  ui::MenuPack pack;

  check(pack.validate(nullptr, 0, kFirmwareAbi) == ui::PackStatus::TooSmall,
        "a null buffer is refused");
  check(pack.validate(good.data(), 8, kFirmwareAbi) == ui::PackStatus::TooSmall,
        "a buffer shorter than the header is refused");
  check(!pack.valid(), "and the object is left unusable rather than half-initialised");

  {
    auto bad = good;
    bad[0] = 'X';
    check(pack.validate(bad.data(), bad.size(), kFirmwareAbi) == ui::PackStatus::BadMagic,
          "wrong magic is refused — most likely the wrong file entirely");
  }
  {
    auto bad = good;
    bad[6] = 99;  // formatVersion
    check(pack.validate(bad.data(), bad.size(), kFirmwareAbi) == ui::PackStatus::BadFormatVersion,
          "a pack from another layout version is refused");
  }
  {
    // Truncation is the realistic corruption: a card pulled mid-write.
    std::vector<uint8_t> truncated(good.begin(), good.begin() + static_cast<long>(good.size() / 2));
    const auto status = pack.validate(truncated.data(), truncated.size(), kFirmwareAbi);
    check(status == ui::PackStatus::BadPayloadLength,
          "a truncated pack is refused on its length, before any offset is dereferenced");
  }
  {
    auto bad = good;
    bad[bad.size() - 1] ^= 0xFF;  // flip a payload byte, leave the length intact
    check(pack.validate(bad.data(), bad.size(), kFirmwareAbi) == ui::PackStatus::BadCrc,
          "a single flipped payload byte is caught by the CRC");
  }
  {
    auto bad = good;
    // A screens offset past the end. The CRC covers the payload, so this has to be paired with a
    // recomputed CRC to reach the offset check at all — which is the point: the CRC is not a
    // substitute for bounds checking, because an attacker or a buggy emitter can compute one.
    bad[26] = 0xFF;
    bad[27] = 0xFF;
    bad[28] = 0xFF;
    bad[29] = 0x7F;
    const uint32_t crc = ui::packCrc32(bad.data() + 64, static_cast<uint32_t>(bad.size() - 64));
    bad[14] = static_cast<uint8_t>(crc & 0xFF);
    bad[15] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    bad[16] = static_cast<uint8_t>((crc >> 16) & 0xFF);
    bad[17] = static_cast<uint8_t>((crc >> 24) & 0xFF);
    check(pack.validate(bad.data(), bad.size(), kFirmwareAbi) == ui::PackStatus::BadOffset,
          "an out-of-range section offset is refused even with a valid CRC");
  }
  {
    auto bad = good;
    bad[8] = 99;  // catalogueAbi far in the future
    bad[9] = 0;
    const uint32_t crc = ui::packCrc32(bad.data() + 64, static_cast<uint32_t>(bad.size() - 64));
    bad[14] = static_cast<uint8_t>(crc & 0xFF);
    bad[15] = static_cast<uint8_t>((crc >> 8) & 0xFF);
    bad[16] = static_cast<uint8_t>((crc >> 16) & 0xFF);
    bad[17] = static_cast<uint8_t>((crc >> 24) & 0xFF);
    check(pack.validate(bad.data(), bad.size(), kFirmwareAbi) == ui::PackStatus::BadCatalogueAbi,
          "a pack needing a newer catalogue is refused");
  }
  {
    // An OLDER catalogue must be accepted: the completeness rule means such a pack can only be
    // missing editors this firmware can supply itself (§4.7b).
    auto older = good;
    check(pack.validate(older.data(), older.size(), 99) == ui::PackStatus::Ok,
          "a pack targeting an OLDER catalogue is accepted, not refused");
  }

  // Every single-byte corruption of the header must be refused rather than crash. This is the
  // check that would catch an offset the individual bounds tests happen to miss.
  std::size_t accepted = 0;
  for (std::size_t i = 0; i < ui::PackHeader::kSize; ++i) {
    auto bad = good;
    bad[i] ^= 0xFF;
    if (pack.validate(bad.data(), bad.size(), kFirmwareAbi) == ui::PackStatus::Ok) {
      ++accepted;
    }
  }
  // 23 of 64 bytes can be flipped and still yield a valid pack, and each is accounted for:
  //
  //   20  label      free-form display text; corrupting it yields a wrong name, not a bad pack
  //    2  unused     header padding, read by nothing
  //    1  themeOffset a low byte that keeps the offset in range while pointing at the wrong
  //                  data. Not detectable until the theme is actually parsed — §3.2 puts the
  //                  CRC over the payload only, so the header is protected field by field
  //                  rather than as a block, and an in-range-but-wrong offset survives that.
  //
  // Asserted exactly rather than loosely: if this number moves, a field has either gained or
  // lost protection, and either is worth knowing. The sweep already earned its place — it found
  // four unprotected themeOffset bytes and two unvalidated level-record bytes, and both are now
  // checked.
  constexpr std::size_t kExpectedFreeFormBytes = 23;
  check(accepted == kExpectedFreeFormBytes,
        "exactly the label, the padding and one in-range themeOffset byte survive corruption");
  std::printf("      %zu of %zu header corruptions still validated\n", accepted,
              ui::PackHeader::kSize);
}

}  // namespace

int main() {
  std::printf("ui::MenuPack — the loadable menu format\n\n");

  const auto bytes = loadFixture();
  if (bytes.empty()) {
    std::printf("  fixture web/mockup/tests/fixtures/default.uipack not found —\n");
    std::printf("  run `npm run test:exporter` in web/mockup to emit it.\n");
    return 1;
  }
  std::printf("  fixture: %zu bytes\n\n", bytes.size());

  crcTests();
  roundTripTests(bytes);
  fuzzTests(bytes);

  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
