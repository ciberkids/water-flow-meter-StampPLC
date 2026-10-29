/**
 * When the configuration page is served — `DF28`.
 *
 * Two halves, because the defect had two halves. The RULE (`plc::drivePortal`) is Arduino-free and is
 * driven here against a fake server. The WIRING — that `firmware.cpp` calls the rule at all — is checked
 * by reading the file as text, the technique `boot_order_test.cpp` uses, because the defect was a server
 * declared and never driven, and nothing that links against a fake can see what the real loop forgot.
 */
#include <cctype>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

#include "firmware_source_text.h"
#include "net/portal_lifecycle.h"

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-74s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) failures++;
}

/** Counts what `drivePortal` asked of it, with the real server's idempotence. */
struct FakeServer {
  bool isUp = false;
  bool beginSucceeds = true;
  int begins = 0;
  int ends = 0;
  int updates = 0;
  uint32_t beganWith = 0;

  bool up() const { return isUp; }
  bool begin(uint32_t apIp) {
    ++begins;
    beganWith = apIp;
    isUp = beginSucceeds;
    return beginSucceeds;
  }
  void update() { ++updates; }
  void end() {
    ++ends;
    isUp = false;
  }
};

constexpr uint32_t kApIp = (192u << 24) | (168u << 16) | (4u << 8) | 1u;  // 192.168.4.1

void notServedOutsideTheAccessPoint() {
  std::printf("\n[no access point, no page — the DNS responder would hijack the station's network]\n");

  FakeServer server;
  for (int pass = 0; pass < 5; ++pass) {
    plc::drivePortal(server, false, kApIp);
  }
  check(server.begins == 0, "five passes with the access point down never start the page");
  check(server.updates == 0, "and never service it");
  check(server.ends == 0, "and do not call end() on a page that was never up");
}

void servedWhileTheAccessPointIsUp() {
  std::printf("\n[the access point comes up: the page starts once, then is serviced every pass]\n");

  FakeServer server;
  plc::drivePortal(server, true, kApIp);
  check(server.begins == 1, "the first pass in ApPortal starts the page");
  check(server.beganWith == kApIp, "with the access point's address, which the DNS replies point at");
  check(server.updates == 1, "and services it in the same pass, so the first request is not delayed");

  for (int pass = 0; pass < 9; ++pass) {
    plc::drivePortal(server, true, kApIp);
  }
  check(server.begins == 1, "nine more passes do not start it again");
  check(server.updates == 10, "and every one of them services it — a missed pass is a stalled browser");
}

void waitsForTheAddress() {
  std::printf("\n[no address: do not tell every DNS reply to point at 0.0.0.0]\n");

  FakeServer server;
  plc::drivePortal(server, true, 0);
  check(server.begins == 0, "ApPortal with address 0 does not start the page");
  // A guard, not a recovery path: WifiManager reads the address once, so on the device a 0 would hold
  // for the window. Stated so this check is not read as proof the firmware waits for one.
  plc::drivePortal(server, true, kApIp);
  check(server.begins == 1 && server.beganWith == kApIp, "and a pass that has an address does");
}

void stopsWhenTheAccessPointCloses() {
  std::printf("\n[the window closes or the form is submitted: the page stops with the access point]\n");

  FakeServer server;
  plc::drivePortal(server, true, kApIp);
  plc::drivePortal(server, false, kApIp);
  check(server.ends == 1, "leaving ApPortal stops the page");
  check(server.updates == 1, "and does not service it on the way out");
  plc::drivePortal(server, false, kApIp);
  check(server.ends == 1, "a second pass outside ApPortal does not stop it twice");

  plc::drivePortal(server, true, kApIp);
  check(server.begins == 2, "a reopened window starts it again — toggling WiFi off and on reopens it");
}

void aFailedStartIsRetried() {
  // The real server reports this since DF28's review: ArduinoPortalServer::begin() returns false when
  // ListeningWebServer says the socket is not listening. Before that it returned true on every path,
  // and this check certified a retry the hardware class could never trigger.
  std::printf("\n[a start that reports failure is retried next pass, not latched for the window]\n");

  FakeServer server;
  server.beginSucceeds = false;
  plc::drivePortal(server, true, kApIp);
  check(server.begins == 1 && !server.up(), "the first start fails");
  check(server.updates == 0, "and a page that is not up is not serviced");
  server.beginSucceeds = true;
  plc::drivePortal(server, true, kApIp);
  check(server.begins == 2 && server.up(), "the next pass tries again and the page comes up");
  check(server.updates == 1, "and is serviced from then on");
}

/**
 * Every offset at which `name` is ASSIGNED in comment-blanked `source`: the name, optional whitespace,
 * then `=` not followed by `=`. A declaration with an initialiser (`uint16_t name = 0;`) is excluded by
 * requiring that the name is not preceded by a type on the same statement, approximated by rejecting
 * an identifier character or a space-separated word immediately before it — good enough for the one
 * declaration this file has, and the check below fails loudly if that ever changes the count.
 */
std::vector<std::size_t> assignmentsTo(const std::string& source, const char* name) {
  std::vector<std::size_t> out;
  for (const std::size_t at : firmware_source::offsetsOf(source, name)) {
    std::size_t i = at + std::strlen(name);
    if (i < source.size() && (std::isalnum(static_cast<unsigned char>(source[i])) || source[i] == '_')) {
      continue;  // a longer identifier
    }
    while (i < source.size() && (source[i] == ' ' || source[i] == '\t')) ++i;
    if (i + 1 >= source.size() || source[i] != '=' || source[i + 1] == '=') continue;
    // A declaration: the previous non-space token on the line is a type name.
    std::size_t before = at;
    while (before > 0 && source[before - 1] == ' ') --before;
    if (before > 0 && (std::isalnum(static_cast<unsigned char>(source[before - 1])) ||
                       source[before - 1] == '_')) {
      continue;
    }
    out.push_back(at);
  }
  return out;
}

/** The [open, close] offsets of the braces of the function whose signature is `signature`. */
std::pair<std::size_t, std::size_t> functionBodySpan(const std::string& source, const char* signature) {
  const std::size_t at = source.find(signature);
  if (at == std::string::npos) return {std::string::npos, std::string::npos};
  const std::size_t open = source.find('{', at);
  if (open == std::string::npos) return {std::string::npos, std::string::npos};
  int depth = 0;
  for (std::size_t i = open; i < source.size(); ++i) {
    if (source[i] == '{') ++depth;
    if (source[i] == '}' && --depth == 0) return {open, i};
  }
  return {std::string::npos, std::string::npos};
}

void firmwareDrivesIt() {
  std::printf("\n[DF28's actual defect — firmware.cpp must CALL the rule, not just declare the server]\n");

  const std::string raw = firmware_source::loadFirmwareSource();
  // A missing file must FAIL, not skip: a gate that passes for a file it never opened reports green
  // for nothing.
  check(!raw.empty(), "src/firmware.cpp is readable from the test's working directory");
  if (raw.empty()) return;
  // Comments blanked first, so the paragraph in firmware.cpp that explains this call cannot satisfy it.
  const std::string source = firmware_source::withoutCommentsOrStrings(raw);

  const auto calls = firmware_source::offsetsOf(source, "plc::drivePortal(portalServer");
  check(calls.size() == 1, "firmware.cpp calls plc::drivePortal(portalServer, ...) exactly once");
  if (calls.empty()) return;
  std::printf("      call at line %zu\n", firmware_source::lineOf(source, calls.front()));

  // The rule reads the state machine's state, so it must follow the pass that updates it.
  const auto stateUpdates = firmware_source::offsetsOf(source, "wifiManager.update(");
  check(!stateUpdates.empty() && stateUpdates.front() < calls.front(),
        "and the call follows wifiManager.update(), so it acts on this pass's state");

  // The ARGUMENTS, not just the call. A call with `true` for the first one would serve the page on the
  // station network, which is the DNS hijack the rule exists to prevent; one with a constant address
  // would point every reply somewhere else. Checked on the call's own text, up to its closing `;`.
  const std::size_t callEnd = source.find(';', calls.front());
  const std::string call = source.substr(calls.front(), callEnd - calls.front());
  check(call.find("wifiManager.state() == plc::WifiState::ApPortal") != std::string::npos,
        "and passes 'the state machine is in ApPortal' as the condition");
  check(call.find("wifiManager.apIpAddress()") != std::string::npos,
        "and the access point's own address");

  // Nothing else may start or stop the page behind the rule's back — that is how a second, disagreeing
  // copy of "when is the page up" would arrive.
  check(firmware_source::offsetsOf(source, "portalServer.begin(").empty() &&
            firmware_source::offsetsOf(source, "portalServer.end(").empty() &&
            firmware_source::offsetsOf(source, "portalServer.update(").empty(),
        "and nothing else in firmware.cpp starts, stops or services the page directly");

  /**
   * The provisioning notice, without which serving the page achieves nothing.
   *
   * DF28's review found `netSettingsNotifiedRevision = netSettings.revision();` inside the logic loop's
   * save-success branch, a seed meant for setup(). The page applies a submission between the notify
   * check and the save, so that line marked every submission as already notified, and the access point
   * stayed up for the rest of its ten-minute window before the device joined the network it had been
   * given. So the shadow may be ASSIGNED in exactly two places: right inside the notify check, and in
   * setup()'s body.
   *
   * Every assignment counts, whatever its right-hand side or spacing: the first version of this check
   * matched one exact statement, and a review showed `netSettingsNotifiedRevision = netSettingsSavedRevision;`
   * or the same line without spaces passed it. "In setup()" is bounded by setup()'s own braces, not by
   * "anywhere after the text `void setup()`", which would have counted loop() too.
   */
  const std::vector<std::size_t> writes = assignmentsTo(source, "netSettingsNotifiedRevision");
  const std::pair<std::size_t, std::size_t> setup = functionBodySpan(source, "void setup()");
  const auto notifyChecks =
      firmware_source::offsetsOf(source, "if (netSettings.revision() != netSettingsNotifiedRevision)");
  check(writes.size() == 2, "netSettingsNotifiedRevision is assigned in exactly two places");
  check(setup.first != std::string::npos && !notifyChecks.empty(),
        "setup()'s body and the notify check are found");
  if (writes.size() != 2 || setup.first == std::string::npos || notifyChecks.empty()) return;
  std::size_t inSetup = 0;
  std::size_t inNotify = 0;
  for (const std::size_t at : writes) {
    if (at > setup.first && at < setup.second) ++inSetup;
    // Within the notify block's first statement: the check's own line plus one short line.
    if (at > notifyChecks.front() && at - notifyChecks.front() < 120) ++inNotify;
  }
  check(inNotify == 1, "one is the first statement inside the notify check");
  check(inSetup == 1, "and the other is inside setup()'s braces, the seed — never in the save path");
}

}  // namespace

int main() {
  std::printf("portal_lifecycle — when the configuration page is served (DF28)\n");
  notServedOutsideTheAccessPoint();
  servedWhileTheAccessPointIsUp();
  waitsForTheAddress();
  stopsWhenTheAccessPointCloses();
  aFailedStartIsRetried();
  firmwareDrivesIt();
  if (failures > 0) {
    std::printf("\nFAILURES (%d of %d)\n", failures, checks);
    return 1;
  }
  std::printf("\nALL PASSED (%d checks)\n", checks);
  return 0;
}
