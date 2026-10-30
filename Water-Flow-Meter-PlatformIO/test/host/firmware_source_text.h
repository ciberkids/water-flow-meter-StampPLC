#pragma once

/**
 * `src/firmware.cpp` read as TEXT, for the host tests whose invariant is about the file rather than about
 * anything a runtime assertion can see.
 *
 * Five tests use it, for the same reason. `boot_order_test.cpp` guards the ORDER of two lines in `setup()`
 * (N-d2), and `portal_lifecycle_test.cpp` guards that a call EXISTS at all (`DF28`): the configuration
 * page's server was declared in `firmware.cpp` and never driven, and no test linked against the firmware
 * glue could have noticed, because no host test can link it. `board_temperature_test.cpp` guards both
 * (`DF29`): that the chip is read at all, on the logic task, through a reader whose `bool` is the bus's,
 * and that the reading is handed to the MQTT snapshot once, before it is filled, with nothing writing
 * over it. `pack_listing_test.cpp` reads `ui_pack_storage_sd.cpp` through `loadSource` (`N-g`), for the
 * same reason: that file includes `SD.h`. `ha_discovery_job_test.cpp` (`DF30`) holds firmware.cpp to the
 * discovery job's three entry points — the latches consumed once and disconnect first, the connected
 * pass called once after the snapshot is filled, and a single teardown that drops the old session — and
 * to calling nothing that bypasses them. Moved here from `boot_order_test.cpp` when the second user
 * arrived, so the comment-blanking lesson below is learned once.
 *
 * A SECOND LESSON, from `DF29`'s review: a check on a call's PREFIX is a presence check on its name and
 * nothing more. That test's first version matched `plc::sampleBoardTemperature(boardTemperature` and
 * `m5::In_I2C.readRegister(plc::kLm75bAddress`, and five edits that kept both prefixes passed it, one of
 * them publishing 0.0 °C every minute again. Where the arguments ARE the requirement, compare the whole
 * statement, with `withoutWhitespace` and `functionBody` below.
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

/**
 * `text` with every whitespace byte removed, for comparing a whole statement with the one it must be.
 *
 * All of it rather than runs collapsed to one space, so `out, 2,` and `out,2,` compare equal and a
 * reformat cannot fail a check that is about the arguments. Two tokens can run together (`return true`
 * becomes `returntrue`), which is harmless when the result is compared against a known statement and is
 * why it is not used to search for a bare name. Run it on `withoutCommentsOrStrings` output, or a
 * comment's prose becomes part of the "code".
 */
inline std::string withoutWhitespace(const std::string& text) {
  std::string out;
  out.reserve(text.size());
  for (const char c : text) {
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r' && c != '\v' && c != '\f') out.push_back(c);
  }
  return out;
}

/**
 * The text between the braces of the one definition that starts with `signature`, or empty when
 * `signature` does not occur exactly once or the next thing after it is not `{`.
 *
 * Counts braces, which is only safe on `withoutCommentsOrStrings` output: there a brace inside a
 * comment, a string or a character literal has already been blanked. Still not a parser (see WHAT IT
 * CANNOT SEE above), but enough to compare a short function's whole body against the one statement it
 * must be.
 */
inline std::string functionBody(const std::string& source, const char* signature) {
  const std::vector<std::size_t> at = offsetsOf(source, signature);
  if (at.size() != 1) return std::string();
  std::size_t open = at.front() + std::strlen(signature);
  while (open < source.size() &&
         (source[open] == ' ' || source[open] == '\t' || source[open] == '\n' || source[open] == '\r')) {
    ++open;
  }
  if (open >= source.size() || source[open] != '{') return std::string();
  int depth = 0;
  for (std::size_t i = open; i < source.size(); ++i) {
    if (source[i] == '{') {
      ++depth;
    } else if (source[i] == '}' && --depth == 0) {
      return source.substr(open + 1, i - open - 1);
    }
  }
  return std::string();
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
