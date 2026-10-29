/**
 * Which files the Select Menu lists — `SdPackStorage::isListablePackName`, from the review of N-g.
 *
 * `npm run export:pack` refuses a file name the device would never list, and it can only do that by
 * applying the device's rule to the device's constants. The constants it READS from the headers; the rule
 * it copies (`web/mockup/tools/exporter/packRules.ts::checkPackFileName`). Until this test the rule was an
 * expression inside `listPacks`, behind `#include <SD.h>`, so no host test could reach it and a change to
 * it would have left the exporter and the Select Menu disagreeing silently.
 *
 * Two halves, like `portal_lifecycle_test.cpp`. The RULE is driven here at its boundaries, each stated in
 * terms of the header constants rather than typed, so the test means the same thing after a limit changes.
 * The WIRING — that `listPacks` decides by calling the rule and by nothing else — is checked by reading
 * `ui_pack_storage_sd.cpp` as text, because that file includes `SD.h` and no host binary can link it.
 */
#include <cctype>
#include <cstdio>
#include <cstring>
#include <string>

#include "firmware_source_text.h"
#include "ui/pack/ui_pack_loader.h"
#include "ui/pack/ui_pack_storage_sd.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-78s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) failures++;
}

using plc::SdPackStorage;

const std::string kSuffix = SdPackStorage::kPackSuffix;
constexpr std::size_t kMaxName = ui::PackLoader::kMaxNameBytes;

bool listed(const std::string& name) { return SdPackStorage::isListablePackName(name.c_str()); }

/** A name of exactly `bytes` bytes that ends in the suffix. */
std::string nameOfLength(std::size_t bytes) { return std::string(bytes - kSuffix.size(), 'a') + kSuffix; }

void theSuffix() {
  std::printf("\n[the suffix, compared byte for byte, as the exporter's pack-file-name rule assumes]\n");

  check(listed("default" + kSuffix), "default<suffix> is listed");
  check(!listed("menu.bin"), "a name with another ending is not");
  std::string upper = "MENU" + kSuffix;
  for (char& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
  check(!listed(upper), "the suffix in capitals is not, though FAT would open the file");
  check(!listed("menu" + kSuffix + ".bak"), "nor one where the suffix is not the END of the name");
  check(!listed(kSuffix.substr(1)), "nor the suffix without its leading dot");
  // Pinned so the move into the header stays a pure move. The exporter refuses this name on its own
  // grounds (nothing in front of the suffix, a leading dot); the device lists it, as it always has.
  check(listed(kSuffix), "a bare suffix IS listed — the device's rule, looser than the exporter's");
  check(!SdPackStorage::isListablePackName(nullptr), "a null entry name is not listed");
}

void theLength() {
  std::printf("\n[the length, against PackLoader::kMaxNameBytes, which counts the terminator]\n");

  check(listed(nameOfLength(kMaxName - 1)), "a name of kMaxNameBytes - 1 bytes is listed: it fits with its NUL");
  check(!listed(nameOfLength(kMaxName)), "one byte longer is not, so names[] can never be overrun");
  check(std::strlen(SdPackStorage::kDirectory) + 1 + kMaxName <= SdPackStorage::kPathBytes,
        "and the longest listed name still fits kPathBytes beside the directory (also a static_assert)");
}

void listPacksDecidesByTheRule() {
  std::printf("\n[the wiring — listPacks must decide by calling the rule, not by a copy of it]\n");

  const std::string raw = firmware_source::loadSource("src/ui/pack/ui_pack_storage_sd.cpp");
  // A missing file must FAIL, not skip: a gate that passes for a file it never opened reports green for
  // nothing.
  check(!raw.empty(), "src/ui/pack/ui_pack_storage_sd.cpp is readable from the test's working directory");
  if (raw.empty()) return;
  // Comments and strings blanked, so the prose explaining the call cannot satisfy it, and so a suffix
  // literal put back is caught by what it compares with rather than by what it says.
  const std::string source = firmware_source::withoutCommentsOrStrings(raw);

  const auto definitions = firmware_source::offsetsOf(source, "SdPackStorage::listPacks(");
  const auto calls = firmware_source::offsetsOf(source, "isListablePackName(");
  check(definitions.size() == 1, "the file defines SdPackStorage::listPacks once");
  check(calls.size() == 1, "it calls isListablePackName exactly once");
  if (definitions.empty() || calls.empty()) return;
  // The body ends at the first closing brace in column 0 after the definition, which is how every
  // function in this file is formatted.
  const std::size_t bodyEnd = source.find("\n}", definitions.front());
  check(calls.front() > definitions.front() && calls.front() < bodyEnd, "and the call is inside listPacks");
  std::printf("      call at line %zu\n", firmware_source::lineOf(source, calls.front()));

  // A second, disagreeing copy of the rule would arrive as a suffix comparison of its own.
  check(firmware_source::offsetsOf(source, "kPackSuffix").empty() &&
            firmware_source::offsetsOf(source, "strcmp(").empty(),
        "and nothing else in the file compares a name with the suffix");

  // Both path buffers are sized by the constant the static_assert covers, not by a literal.
  const auto buffers = firmware_source::offsetsOf(source, "char path[");
  const auto sized = firmware_source::offsetsOf(source, "char path[kPathBytes]");
  check(buffers.size() == 2 && sized.size() == buffers.size(),
        "packSize and readPack size their path buffers with kPathBytes");
}

}  // namespace

int main() {
  std::printf("plc::SdPackStorage::isListablePackName — what the Select Menu lists\n");

  theSuffix();
  theLength();
  listPacksDecidesByTheRule();

  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
