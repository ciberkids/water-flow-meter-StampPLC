#pragma once

// The HTTP half of the configuration portal (§7.6).
//
// NOT HOST-COMPILED. PortalForm owns everything decidable without a socket — form generation from the
// settings catalogue, urlencoded parsing, per-field validation, HTML escaping, Basic auth — and
// carries 226 host checks. This file is the socket, and nothing else. Keeping it that thin is what
// makes the untestable surface reviewable.
//
// §6.3 made this LOAD-BEARING rather than a convenience: with on-device text entry removed, this is
// the only way to provision a device that has no Modbus master attached.

#include <DNSServer.h>
#include <WebServer.h>

#include <cstddef>
#include <cstdint>

#include "net/httpd_task_policy.h"
#include "net/portal_form.h"

namespace plc {

/**
 * Serves PortalForm over WebServer, with a captive-portal DNS responder.
 *
 * WebServer, not esp_http_server: it runs on the CALLER'S task via handleClient(), so it inherits the
 * logic task's priority 1 rather than creating a task of its own. httpd_task_policy.h exists because
 * esp_http_server's HTTPD_DEFAULT_CONFIG would put an unpinned priority-5 task on whichever core the
 * scheduler chose — above the priority-2 sampler. WebServer sidesteps that entirely, which is the
 * reason to prefer it here even though it is the less capable server.
 */
class ArduinoPortalServer {
 public:
  /** Where a browser is sent when it asks for anything else — the captive-portal redirect. */
  static constexpr uint16_t kHttpPort = 80;
  static constexpr uint16_t kDnsPort = 53;

  explicit ArduinoPortalServer(PortalForm& form) : form_(form), server_(kHttpPort) {}

  /**
   * Starts serving. `apIp` is the softAP address a captive-portal DNS reply must point at.
   *
   * Idempotent: calling it while already up is a no-op, so the caller can drive it from a condition
   * rather than tracking edges itself.
   */
  bool begin(uint32_t apIp) {
    if (up_) {
      return true;
    }
    server_.on("/", HTTP_GET, [this] { handleGet(); });
    server_.on("/", HTTP_POST, [this] { handlePost(); });
    // Anything else redirects to "/". That IS the captive-portal behaviour: phones probe a
    // vendor-specific URL and treat a redirect as "a login page is waiting", which is what makes the
    // portal appear without the operator typing an address.
    server_.onNotFound([this] { redirectToRoot(); });

    // WITHOUT THIS, header("Authorization") ALWAYS RETURNS EMPTY. WebServer keeps only a short
    // built-in set of request headers and discards the rest, so authorize() would be handed "" on
    // every request, refuse, and the portal would 401 forever — locked out by construction, on the
    // one surface §6.3 left as the only way to provision a device with no Modbus master.
    //
    // Not caught by the compiler and not caught by PortalForm's 226 checks either: they test
    // authorize() against a header string, and this is the code that fails to obtain one.
    static const char* kCollected[] = {"Authorization"};
    server_.collectHeaders(kCollected, 1);

    server_.begin();

    // Every name resolves to us while the AP is up. Deliberately only while the AP is up — hijacking
    // DNS on a station interface would break the network the device just joined.
    dnsUp_ = dns_.start(kDnsPort, "*", IPAddress(static_cast<uint8_t>((apIp >> 24) & 0xFF),
                                                static_cast<uint8_t>((apIp >> 16) & 0xFF),
                                                static_cast<uint8_t>((apIp >> 8) & 0xFF),
                                                static_cast<uint8_t>(apIp & 0xFF)));
    up_ = true;
    return true;
  }

  void end() {
    if (!up_) {
      return;
    }
    if (dnsUp_) {
      dns_.stop();
      dnsUp_ = false;
    }
    server_.stop();
    up_ = false;
  }

  bool up() const { return up_; }

  /**
   * One service pass. Must be called often while the portal is up.
   *
   * Runs on the caller's task, so a slow client blocks THIS task — the logic task at priority 1 —
   * and never the sampler or the Modbus server. That is the whole reason for choosing WebServer.
   */
  void update() {
    if (!up_) {
      return;
    }
    if (dnsUp_) {
      dns_.processNextRequest();
    }
    server_.handleClient();
  }

 private:
  /** Adapts PortalForm's byte sink onto WebServer's chunked response. */
  class ChunkSink final : public PortalSink {
   public:
    explicit ChunkSink(WebServer& server) : server_(server) {}
    void writeBytes(const char* data, std::size_t length) override {
      if (data == nullptr || length == 0) {
        return;
      }
      // sendContent with an explicit length: PortalForm's output is not NUL-terminated per call, and
      // the escaped HTML can legitimately contain characters a String constructor would stop at.
      server_.sendContent(data, length);
    }

   private:
    WebServer& server_;
  };

  /**
   * Basic auth, refused with a WWW-Authenticate challenge.
   *
   * PortalForm::authorize does the constant-time comparison; this only hands it the header. Returning
   * 401 WITH the challenge is what makes a browser prompt rather than showing a bare error — and the
   * realm is deliberately generic, because naming the device in the realm tells an unauthenticated
   * caller what it has found.
   */
  bool authorized() {
    const String header = server_.header("Authorization");
    if (form_.authorize(header.c_str())) {
      return true;
    }
    server_.sendHeader("WWW-Authenticate", "Basic realm=\"Configuration\"");
    server_.send(401, "text/plain", "Unauthorized");
    return false;
  }

  void beginChunked() {
    // Chunked, because the generated form is a few kilobytes and buffering it would mean a String
    // that size on the logic task's stack. §7.6's form is built from the whole settings catalogue.
    server_.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server_.send(200, "text/html", "");
  }

  void handleGet() {
    if (!authorized()) {
      return;
    }
    beginChunked();
    ChunkSink sink(server_);
    form_.renderSettingsPage(sink);
    server_.sendContent("");  // terminating empty chunk
  }

  void handlePost() {
    if (!authorized()) {
      return;
    }
    // WebServer has already parsed the body into arguments, but PortalForm wants the raw urlencoded
    // string: its own parser is what the 226 host checks exercise, including the percent- and
    // plus-decoding and the empty-writeOnly-means-unchanged rule. Re-parsing here would be a second
    // implementation of the part that most needs to be single.
    const String body = server_.hasArg("plain") ? server_.arg("plain") : String();
    const PortalSubmitResult result = form_.submit(body.c_str());
    beginChunked();
    ChunkSink sink(server_);
    form_.renderSubmitPage(sink, result);
    server_.sendContent("");
  }

  void redirectToRoot() {
    server_.sendHeader("Location", "/", true);
    server_.send(302, "text/plain", "");
  }

  PortalForm& form_;
  WebServer server_;
  DNSServer dns_;
  bool up_ = false;
  bool dnsUp_ = false;
};

}  // namespace plc
