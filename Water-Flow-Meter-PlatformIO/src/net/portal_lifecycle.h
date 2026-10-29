#pragma once

#include <cstdint>

namespace plc {

/**
 * When the configuration page is served, and the one call that serves it — DF28.
 *
 * `ArduinoPortalServer` was constructed in `firmware.cpp` and never driven: nothing called `begin()`,
 * `update()` or `end()`, so the provisioning access point came up for its ten minutes and nothing
 * answered on it. The commit that introduced the server described this rule in its message and never
 * committed it. So the rule lives HERE, Arduino-free and tested by `test/host/portal_lifecycle_test.cpp`,
 * and `firmware.cpp` makes exactly one call to it — a call that test also checks is present, because
 * "declared and never driven" is invisible to every other check this project has.
 *
 * THE RULE: the page is up exactly while the WiFi state machine is in `ApPortal` and the access point
 * has an address, and down otherwise. Three consequences, each a decision rather than a detail:
 *
 * - **Never on the station interface.** The server carries a captive-portal DNS responder that answers
 *   EVERY name with the device's own address. That is right for a phone that has just joined the
 *   device's access point, and would break name resolution on the network the device joined as a
 *   station. So `Connected` is not a reason to serve, even though a page on the LAN would be convenient.
 * - **Not without an address.** `apIp == 0` would tell every DNS reply to point at 0.0.0.0, so the page
 *   is not started. This is a guard, not a recovery: `WifiManager` reads the address once, when it
 *   raises the access point, and the access point's network interface is created with a static
 *   address, so 0 is not expected; if it ever happened, the page would stay down for that window.
 * - **A failed `begin()` is retried on the next pass**, not latched. `ArduinoPortalServer::begin()`
 *   returns false when its socket is not actually listening (it could not tell, until `DF28`'s review
 *   gave it `ListeningWebServer`), and a page that silently stays down for the rest of its window
 *   because one start failed is the defect this function replaces, in a smaller form.
 *
 * `update()` runs the server on the CALLER'S task, which is the logic task at priority 1 — a slow client
 * blocks that task and never the sampler (`portal_server_arduino.h` explains why that server was chosen).
 *
 * A template over the server so the host test can drive it with a fake; the real server is the only
 * other instantiation. `Server` needs `bool up() const`, `bool begin(uint32_t)`, `void update()` and
 * `void end()`, all of which `ArduinoPortalServer` has.
 */
template <typename Server>
void drivePortal(Server& server, bool apPortalActive, uint32_t apIp) {
  if (!apPortalActive || apIp == 0) {
    if (server.up()) {
      server.end();
    }
    return;
  }
  if (!server.up() && !server.begin(apIp)) {
    return;
  }
  server.update();
}

}  // namespace plc
