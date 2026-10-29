#pragma once

/**
 * `src/firmware.cpp` read as TEXT, for the host tests whose invariant is about the file rather than about
 * anything a runtime assertion can see.
 *
 * Two tests use it, for the same reason. `boot_order_test.cpp` guards the ORDER of two lines in `setup()`
 * (N-d2), and `portal_lifecycle_test.cpp` guards that a call EXISTS at all (`DF28`): the configuration
 * page's server was declared in `firmware.cpp` and never driven, and no test linked against the firmware
 * glue could have noticed, because no host test can link it. Moved here from `boot_order_test.cpp` when the
 * second user arrived, so the comment-blanking lesson below is learned once. A third, `pack_listing_test.cpp`,
 * reads `ui_pack_storage_sd.cpp` through `loadSource`, for the same reason: that file includes `SD.h`.
 *
 * WHAT IT CANNOT SEE: it does not parse C++. A call moved inside a conditional, or into a function nobody
 * calls, satisfies a presence check. It catches the ordinary refactor, which is the one that happens.
 */
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace firmware_source {

/**
 * A firmware source file, named relative to `Water-Flow-Meter-PlatformIO/` (e.g. `src/firmware.cpp`), as
 * one string — or empty when it cannot be found from the working directory.
 *
 * Generalised from `loadFirmwareSource` when `pack_listing_test.cpp` needed `ui_pack_storage_sd.cpp`: the
 * SD adapter is Arduino-only, so whether `listPacks` still decides by its host-tested rule is, like the
 * two checks on `firmware.cpp`, a fact about the file rather than about anything a host binary can run.
 */
inline std::string loadSource(const char* relative) {
  const std::string candidates[] = {std::string(relative), std::string("../") + relative,
                                    std::string("Water-Flow-Meter-PlatformIO/") + relative};
  for (const std::string& path : candidates) {
    if (std::FILE* f = std::fopen(path.c_str(), "rb")) {
      std::string out;
      char buffer[4096];
      std::size_t read = 0;
      while ((read = std::fread(buffer, 1, sizeof(buffer), f)) > 0) out.append(buffer, read);
      std::fclose(f);
      if (!out.empty()) return out;
    }
  }
  return std::string();
}

/** `src/firmware.cpp` as one string, or empty when it cannot be found. */
inline std::string loadFirmwareSource() { return loadSource("src/firmware.cpp"); }

/**
 * The source with comments and string literals blanked to spaces, preserving every byte's position.
 *
 * A REGEX OVER THE RAW FILE DOES NOT WORK, and the first run proved it: the comment above the probe
 * explains that "`M5StamPLC.begin()` clears the RX8130CE's flag register", so a raw scan found a
 * `begin()` call six lines ABOVE the probe and reported the ordering broken. The prose about an
 * invariant must not be able to violate it. `gen-actions.mjs` learned the same lesson and calls its
 * approach "a scanner, not a regex".
 *
 * Blanking rather than deleting keeps offsets and line numbers true, so a failure still names the line
 * a human has to look at.
 */
inline std::string withoutCommentsOrStrings(const std::string& source) {
  std::string out = source;
  enum class In { Code, LineComment, BlockComment, String, Char };
  In state = In::Code;
  for (std::size_t i = 0; i < out.size(); ++i) {
    const char c = out[i];
    const char next = (i + 1 < out.size()) ? out[i + 1] : '\0';
    switch (state) {
      case In::Code:
        if (c == '/' && next == '/') { state = In::LineComment; out[i] = ' '; out[i + 1] = ' '; ++i; }
        else if (c == '/' && next == '*') { state = In::BlockComment; out[i] = ' '; out[i + 1] = ' '; ++i; }
        else if (c == '"') { state = In::String; out[i] = ' '; }
        else if (c == '\'') { state = In::Char; out[i] = ' '; }
        break;
      case In::LineComment:
        if (c == '\n') state = In::Code; else out[i] = ' ';
        break;
      case In::BlockComment:
        if (c == '*' && next == '/') { state = In::Code; out[i] = ' '; out[i + 1] = ' '; ++i; }
        else if (c != '\n') out[i] = ' ';
        break;
      case In::String:
      case In::Char: {
        const char quote = (state == In::String) ? '"' : '\'';
        if (c == '\\') { out[i] = ' '; if (i + 1 < out.size() && out[i + 1] != '\n') out[++i] = ' '; }
        else if (c == quote) { state = In::Code; out[i] = ' '; }
        else if (c != '\n') out[i] = ' ';
        break;
      }
    }
  }
  return out;
}

/** Every 0-based offset at which `needle` occurs. */
inline std::vector<std::size_t> offsetsOf(const std::string& haystack, const char* needle) {
  std::vector<std::size_t> out;
  const std::size_t length = std::strlen(needle);
  for (std::size_t at = haystack.find(needle); at != std::string::npos;
       at = haystack.find(needle, at + length)) {
    out.push_back(at);
  }
  return out;
}

/** 1-based line number of an offset, for a message a human can act on. */
inline std::size_t lineOf(const std::string& text, std::size_t offset) {
  std::size_t line = 1;
  for (std::size_t i = 0; i < offset && i < text.size(); ++i) {
    if (text[i] == '\n') ++line;
  }
  return line;
}

}  // namespace firmware_source
