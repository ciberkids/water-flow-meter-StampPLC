#pragma once

// The HTTP half of the configuration portal (§7.6).
//
// NOT HOST-COMPILED. PortalForm owns everything decidable without a socket — form generation from the
// settings catalogue, urlencoded parsing, per-field validation, HTML escaping, Basic auth — and
// is covered by test/host/portal_form_test.cpp. This file is the socket, and nothing else. Keeping it that thin is what
// makes the untestable surface reviewable.
//
// §6.3 made this LOAD-BEARING rather than a convenience: with on-device text entry removed, this is
// the only way to provision a device that has no Modbus master attached.

#include <DNSServer.h>
#include <WebServer.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <new>

#include "net/httpd_task_policy.h"
#include "net/portal_form.h"

namespace plc {

/**
 * WebServer, plus the one fact it will not tell its owner: whether it is actually listening.
 *
 * `WebServer::begin()` is void, and the `WiFiServer` under it returns silently with `_listening` false
 * when socket(), bind() or listen() fails. So `ArduinoPortalServer::begin()` used to report success
 * unconditionally, and `drivePortal`'s retry of a failed start (`DF28`) could never happen: a failed
 * listen left the page "up" and answering nothing for the whole window. `_server` is protected and
 * `WiFiServer` converts to bool as `_listening`, so a subclass is the whole fix — no library change.
 */
class ListeningWebServer : public WebServer {
 public:
  using WebServer::WebServer;
  bool listening() { return static_cast<bool>(_server); }
  /**
   * Whether a raw body is being streamed, i.e. whether `raw()` may be dereferenced.
   *
   * The second callback a POST route carries is ALSO WebServer's upload callback: a `multipart/` POST
   * skips the raw branch and calls it from `_parseForm` for every part with a `filename=`, while
   * `_currentRaw` is null. `raw()` is `*_currentRaw`, so dereferencing it there read address 0 and
   * rebooted the device — before any login check, for anyone in radio range of the access point. Found
   * by the review of DF28's second round; this is what `receiveBody()` asks first.
   */
  bool rawActive() const { return static_cast<bool>(_currentRaw); }
};

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
  /** How often a DNS responder that failed to start is tried again. */
  static constexpr uint32_t kDnsRetryMs = 5000;

  explicit ArduinoPortalServer(PortalForm& form) : form_(form), server_(kHttpPort) {}

  /**
   * Starts serving. `apIp` is the softAP address a captive-portal DNS reply must point at.
   *
   * Idempotent: calling it while already up is a no-op, so the caller can drive it from a condition
   * rather than tracking edges itself.
   *
   * Returns false, and leaves nothing half-started, when the HTTP socket is not listening — the one
   * failure that leaves the page unreachable — so `drivePortal` tries again on the next pass. A DNS
   * responder that fails to start does NOT fail this: the page is still reachable at the access point's
   * address, and `update()` keeps trying the responder, which is what makes a phone pop the page.
   */
  bool begin(uint32_t apIp) {
    if (up_) {
      return true;
    }
    if (!routed_) {
      route();
      routed_ = true;
    }

    server_.begin();
    if (!server_.listening()) {
      server_.stop();
      return false;
    }
    apIp_ = apIp;
    startDns();
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
    // Rate-limited: a responder that keeps failing would otherwise be retried, and logged by the UDP
    // layer, on every logic pass, which at this firmware's debug level is about a thousand lines a second.
    if (!dnsUp_ && millis() - lastDnsAttemptMs_ >= kDnsRetryMs) {
      startDns();
    }
    if (dnsUp_) {
      dns_.processNextRequest();
    }
    server_.handleClient();
  }

 private:
  /**
   * Registers the routes and the collected headers. ONCE per object, not once per `begin()`.
   *
   * `WebServer::on()` appends to a handler list (`_addRequestHandler`) that `stop()` does not clear —
   * `stop()` is `close()`, which closes the socket and leaves the list alone. So registering on every
   * `begin()`, which is what this code did while nothing called `begin()` at all (`DF28`), would add
   * three handlers each time the provisioning window reopened and leak them for the life of the device.
   * First match wins, so it would never have shown as wrong behaviour; only as heap gone.
   */
  void route() {
    server_.on("/", HTTP_GET, [this] { handleGet(); });
    // Both POST routes carry a second callback, and that is what makes WebServer hand over the body
    // RAW — see receiveBody(). With one callback, a urlencoded body is parsed into arguments and the
    // body itself is thrown away.
    server_.on("/", HTTP_POST, [this] { handlePost(); }, [this] { receiveBody(); });
    /**
     * The path the FORM actually posts to, which was routed nowhere.
     *
     * `PortalForm::kFormAction` is `/save` and its own comment states the contract — "the adapter must
     * route this exact path (R7.11 commits on POST only)". This adapter routed `POST` on `/` alone, and
     * ESP32's `Uri::canHandle` is `_uri == requestUri`, exact string equality. So every submission fell
     * through to `onNotFound`, got a 302 to `/`, and the browser re-issued it as a GET: the configuration
     * was discarded with no error shown, on the ONLY provisioning route a device without a Modbus master
     * has.
     *
     * `/` keeps its POST handler as well. Removing it would break nothing that exists today, but a form
     * that posts to the page it came from is the obvious thing for a future edit to do, and having both
     * routed costs one line against a silent failure that took a field report to find.
     *
     * Not caught by PortalForm's host checks, and it could not be: they exercise the form against
     * strings, and routing lives here — in the adapter no host test can construct. The same blind spot the
     * `collectHeaders` comment below warns about.
     */
    server_.on(PortalForm::kFormAction, HTTP_POST, [this] { handlePost(); }, [this] { receiveBody(); });
    // Anything else redirects to "/". That IS the captive-portal behaviour: phones probe a
    // vendor-specific URL and treat a redirect as "a login page is waiting", which is what makes the
    // portal appear without the operator typing an address.
    server_.onNotFound([this] { redirectToRoot(); });

    // Belt and braces, and the comment here used to overstate it. WebServer discards request headers it
    // was not told to collect, and `header("Authorization")` returning "" would lock the portal behind
    // a 401 for ever. But the core this firmware is pinned to (Arduino 2.0.17) ALWAYS collects
    // Authorization: `collectHeaders()` puts it at index 0 whatever it is given, and `begin()`'s
    // `close()` calls `collectHeaders(0, 0)` when nothing was registered. So this call registers it a
    // second time, harmlessly, and is kept for a core that stops doing that implicitly.
    static const char* kCollected[] = {"Authorization"};
    server_.collectHeaders(kCollected, 1);
  }

  /** Starts the captive-portal responder: every name resolves to us, only while the AP is up. */
  void startDns() {
    lastDnsAttemptMs_ = millis();
    // Deliberately only while the AP is up — hijacking DNS on a station interface would break the
    // network the device just joined. Retried from update() when it fails, without failing the page.
    dnsUp_ = dns_.start(kDnsPort, "*", IPAddress(static_cast<uint8_t>((apIp_ >> 24) & 0xFF),
                                                static_cast<uint8_t>((apIp_ >> 16) & 0xFF),
                                                static_cast<uint8_t>((apIp_ >> 8) & 0xFF),
                                                static_cast<uint8_t>(apIp_ & 0xFF)));
  }

  /**
   * Collects a POST body as it arrives, called by WebServer once per raw chunk (`DF28`).
   *
   * THE BODY USED TO BE READ FROM `arg("plain")`, AND IT IS NEVER THERE FOR THIS FORM. WebServer
   * creates "plain" only for a body that is NOT `application/x-www-form-urlencoded` (Parsing.cpp: the
   * urlencoded branch sets `isEncoded`, folds the body into the parsed arguments, and skips "plain"),
   * and urlencoded is exactly what a browser sends for a form with no enctype. So every submission
   * reached submit() as "", which reports success with nothing changed, and the page answered "Nothing
   * changed" to an operator who had just typed their WiFi credentials. Unreachable until the page was
   * served at all, which is why it survived.
   *
   * Registering a second callback makes `canRaw()` true for these routes, and WebServer then streams
   * the body here in HTTP_RAW_BUFLEN chunks instead of parsing it: RAW_START, one RAW_WRITE per chunk,
   * then RAW_END, or RAW_ABORTED when the client stops sending (in which case handlePost is not called
   * at all). PortalForm keeps its own decoder, which is the part the host checks cover; rebuilding the
   * string from WebServer's decoded arguments would have been a second implementation of it.
   *
   * The same callback is WebServer's UPLOAD callback for a `multipart/` POST, where there is no raw
   * buffer at all; `rawActive()` is asked first, and such a body is refused (see ListeningWebServer).
   *
   * Buffered in memory allocated ONCE, at RAW_START, from the request's Content-Length, which is refused
   * first if it exceeds `PortalForm::kMaxSubmitBytes` (derived from the form by the host tests). Not an
   * Arduino `String`, for two reasons the first version of this code had: `concat(ptr, len)` copies
   * `len + 1` bytes, so a full chunk was read one byte past the end of `HTTPRaw::buf`; and `concat`
   * returns false and appends nothing when it cannot grow, which that version ignored, so a chunk could
   * be dropped from the middle of a body that was then submitted as complete. One allocation of the
   * declared length, checked, removes both. A body that overruns its declared length, carries a NUL, or
   * cannot be allocated is refused whole — never truncated, because a truncated urlencoded body is a
   * valid shorter one and would apply part of what was sent.
   *
   * KNOWN COST, not a defect: WebServer asks the socket for a full chunk however few bytes remain, and
   * `Stream::readBytes` waits out its one-second timeout for the bytes that never come. So each save
   * holds the logic task about a second longer than the old path did — the display and MQTT pause for
   * that second, the sampler and Modbus (other task, other core) do not. Removing it would mean changing
   * `HTTP_RAW_BUFLEN` for the whole framework build, which is a bigger lever than this cost justifies.
   */
  void receiveBody() {
    if (!server_.rawActive()) {
      releaseBody();
      bodyRefused_ = true;
      return;
    }
    HTTPRaw& raw = server_.raw();
    switch (raw.status) {
      case RAW_START: {
        releaseBody();
        const int declared = server_.clientContentLength();
        if (declared < 0 || static_cast<std::size_t>(declared) > PortalForm::kMaxSubmitBytes) {
          bodyRefused_ = true;
          break;
        }
        bodyCapacity_ = static_cast<std::size_t>(declared);
        body_.reset(new (std::nothrow) char[bodyCapacity_ + 1]);
        if (!body_) {
          bodyRefused_ = true;
          break;
        }
        body_[0] = '\0';
        break;
      }
      case RAW_WRITE:
        if (bodyRefused_ || !body_) break;
        if (bodyLength_ + raw.currentSize > bodyCapacity_ ||
            std::memchr(raw.buf, '\0', raw.currentSize) != nullptr) {
          releaseBody();
          bodyRefused_ = true;
          break;
        }
        std::memcpy(body_.get() + bodyLength_, raw.buf, raw.currentSize);
        bodyLength_ += raw.currentSize;
        break;
      case RAW_END:
        if (bodyRefused_ || !body_) break;
        body_[bodyLength_] = '\0';
        bodyComplete_ = true;
        break;
      case RAW_ABORTED:
        releaseBody();
        break;
    }
  }

  /** Frees the body and clears every flag about it, so no request inherits the previous one's state. */
  void releaseBody() {
    body_.reset();
    bodyLength_ = 0;
    bodyCapacity_ = 0;
    bodyComplete_ = false;
    bodyRefused_ = false;
  }

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
    // The body and its flags are released on every path out, so a refused or unauthorised request does
    // not leave a copy of somebody's passphrase on the heap, and the next request — which may not take
    // the raw path at all — cannot inherit "complete" or "refused" from this one.
    if (!authorized()) {
      releaseBody();
      return;
    }
    if (bodyRefused_ || !bodyComplete_) {
      const bool refused = bodyRefused_;
      releaseBody();
      server_.send(refused ? 413 : 400, "text/plain",
                   refused ? "The form was larger than this device accepts, or malformed. Nothing was saved."
                           : "The form did not arrive complete. Nothing was saved.");
      return;
    }
    // The RAW urlencoded string, as submit() expects: its own decoder is what the host checks exercise,
    // including the percent- and plus-decoding and the empty-writeOnly-means-unchanged rule.
    const PortalSubmitResult result = form_.submit(body_.get());
    releaseBody();
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
  ListeningWebServer server_;
  DNSServer dns_;
  /** The access point's address, kept so update() can retry the DNS responder. */
  uint32_t apIp_ = 0;
  bool up_ = false;
  bool dnsUp_ = false;
  /** The POST body being received: Content-Length bytes plus a terminator, none between requests. */
  std::unique_ptr<char[]> body_;
  std::size_t bodyLength_ = 0;
  std::size_t bodyCapacity_ = 0;
  bool bodyComplete_ = false;
  bool bodyRefused_ = false;
  uint32_t lastDnsAttemptMs_ = 0;
  /** Whether `route()` has run. The routes outlive `end()`, so they are registered once. */
  bool routed_ = false;
};

}  // namespace plc
