/**
 * When the configuration page is served — `DF28`.
 *
 * Two halves, because the defect had two halves. The RULE (`plc::drivePortal`) is Arduino-free and is
 * driven here against a fake server. The WIRING — that `firmware.cpp` calls the rule at all — is checked
 * by reading the file as text, the technique `boot_order_test.cpp` uses, because the defect was a server
 * declared and never driven, and nothing that links against a fake can see what the real loop forgot.
 */
#include <cstdint>
#include <cstdio>
#include <string>

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
  std::printf("\n[no address yet: wait, rather than tell every DNS reply to point at 0.0.0.0]\n");

  FakeServer server;
  plc::drivePortal(server, true, 0);
  check(server.begins == 0, "ApPortal with address 0 does not start the page");
  plc::drivePortal(server, true, kApIp);
  check(server.begins == 1 && server.beganWith == kApIp, "the next pass, with an address, does");
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
  std::printf("\n[a start that fails is retried next pass, not latched for the whole window]\n");

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

  // Nothing else may start or stop the page behind the rule's back — that is how a second, disagreeing
  // copy of "when is the page up" would arrive.
  check(firmware_source::offsetsOf(source, "portalServer.begin(").empty() &&
            firmware_source::offsetsOf(source, "portalServer.end(").empty() &&
            firmware_source::offsetsOf(source, "portalServer.update(").empty(),
        "and nothing else in firmware.cpp starts, stops or services the page directly");
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
