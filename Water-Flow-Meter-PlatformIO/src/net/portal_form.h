#pragma once

#include <cstddef>
#include <cstdint>

#include "net/net_settings.h"
#include "ui/core/ui_settings_types.h"

namespace plc {

/**
 * The configuration web portal's form logic (WiFi_MQTT_Connectivity.md §7.6).
 *
 * §6.3 deleted the on-device text editor, so this form is the ONLY way to type a string into a
 * device that has no Modbus master attached — the WiFi passphrase, the broker host, the portal's own
 * password. That makes it load-bearing rather than a convenience, and it is why the interesting
 * halves live here instead of inside a WebServer handler:
 *
 *   - the HTML is GENERATED from the settings catalogue (R7.9c), so a setting added to the firmware
 *     cannot be missing from the portal and the portal cannot offer one the firmware lacks;
 *   - parsing, validation, escaping and the Basic-auth comparison are all decisions with failure
 *     modes that no bench reproduces on demand — an unescaped topic name, an empty passphrase field
 *     that wipes the stored one, a numeric that overflows — so they are Arduino-free and tested on a
 *     host, exactly like the register map they write through.
 *
 * What is NOT here, deliberately: sockets, routes, headers, chunked replies. `WebServer` is a thin
 * adapter that authenticates, calls one of the render methods with a sink that forwards to
 * `sendContent`, and hands the POST body to `submit()`.
 */

/**
 * Where rendered HTML goes.
 *
 * A sink rather than a returned string because the page is a few kilobytes and this module is
 * allocation-free, matching NetSettings. The firmware adapter forwards to
 * `WebServer::sendContent`; the host test appends to a std::string.
 */
class PortalSink {
 public:
  virtual ~PortalSink() = default;
  virtual void writeBytes(const char* data, std::size_t length) = 0;

  /** Convenience for NUL-terminated literals. Non-virtual: there is one way to accept bytes. */
  void writeText(const char* text);
};

/**
 * HTML-escapes `in` into `out`.
 *
 * All FIVE metacharacters, not the usual three. Every value this module renders lands inside a
 * `value="..."` attribute, so escaping only `& < >` still leaves `watermeter/"><script>` as a
 * working breakout — the quotes are the attribute sink and matter more here than the angle brackets.
 * `&` is handled first so an escape is never escaped twice.
 *
 * Returns false when `out` was too small; `out` then holds a truncated but still fully escaped
 * string, because half-escaped output would be worse than short output.
 */
bool portalEscapeHtml(const char* in, char* out, std::size_t size);

/** The same escaping, streamed. Used for everything rendered from a stored or submitted value. */
void portalWriteEscaped(PortalSink& out, const char* text);

/**
 * Compares two NUL-terminated secrets without an early exit (§8).
 *
 * The login is the entire defence of an always-on server on the operator's LAN (R7.9), and
 * `strcmp` returns after the first differing byte — enough, over many requests, to recover a
 * password one character at a time. This folds every byte of a fixed window plus the two lengths
 * into one accumulator.
 *
 * Honest limit: `strlen` on both arguments is itself length-dependent, so this is constant-time in
 * the CONTENT of the secret and not in its length. Closing that too would mean carrying lengths
 * everywhere, and length is the part an attacker gains least from.
 */
bool portalConstantTimeEquals(const char* a, const char* b);

/** Why a submitted field was refused. Reported per field — see PortalSubmitResult. */
enum class PortalFieldError : uint8_t {
  None = 0,
  /** No catalogue setting has this binding id (or the name did not fit). */
  UnknownField,
  /** A malformed percent escape, a `%00`, or a control character in a text value. */
  BadEncoding,
  /** Longer than the field's capacity. Rejected, not truncated — see the note in submit(). */
  TooLong,
  /** Not an integer at all, or wider than int64. */
  NotANumber,
  /** An integer outside the descriptor's min/max. */
  OutOfRange,
  /** An enum value that is not one of the descriptor's options. */
  UnknownOption,
  /**
   * Validated here and then refused by the store that owns it.
   *
   * The only known cause is the Nyquist rule, which lives in ModbusManager because it depends on
   * the live polling rate. This module cannot evaluate it without a second implementation of it,
   * which §3.2 forbids — so it surfaces here instead.
   */
  Refused,
};

const char* portalFieldErrorText(PortalFieldError error);

/** What a submission did, or why it did nothing. */
struct PortalSubmitResult {
  static constexpr std::size_t kMaxErrors = 12;
  static constexpr std::size_t kMaxFieldNameBytes = 64;

  struct FieldError {
    /** The submitted name. Attacker-controlled for UnknownField, so it MUST be escaped to render. */
    char field[kMaxFieldNameBytes] = {};
    PortalFieldError error = PortalFieldError::None;
  };

  FieldError errors[kMaxErrors] = {};
  std::size_t errorCount = 0;
  /** True when more fields failed than there was room to report. */
  bool moreErrors = false;

  /** True when NetSettings::apply() reported an actual change. False for a no-op submission. */
  bool committed = false;
  /**
   * How many network fields this submission actually staged.
   *
   * Exists to gate the apply. submit() used to call NetSettings::apply() unconditionally, which
   * means a POST touching only store-backed fields — or a POST that staged nothing at all — still
   * promoted whatever was pending. R5.5a accepts that a *deliberate* apply promotes a Modbus
   * master's in-flight block (one apply path, last apply wins); it does not excuse applying when
   * this surface staged nothing, which would destroy a master's half-written block for no reason
   * whatsoever and with nothing to show the operator for it.
   */
  std::size_t networkFieldsStaged = 0;
  /**
   * Writes the injected store accepted.
   *
   * Tracked separately from `committed` because they are two different sinks, and a submission may
   * touch either alone. Collapsing them was a real bug: a body carrying only `config.modbusSlaveId`
   * leaves `committed` false, and the page then told the operator "nothing changed" about a value it
   * had just written. On the only surface that can show these values at all, a page that misreports
   * what landed is as bad as one that loses it.
   */
  std::size_t externalWrites = 0;
  /** How many fields carried a value that was accepted and written. */
  std::size_t fieldsAccepted = 0;
  /**
   * True when something was already stored and a later store write was refused
   * (PortalFieldError::Refused). The one case where a submission is partially applied, and it is
   * reported rather than hidden.
   */
  bool partiallyApplied = false;

  /** True when either sink took something. What the page may honestly call "saved". */
  bool storedSomething() const { return committed || externalWrites > 0; }

  bool ok() const { return errorCount == 0; }
};

/**
 * The settings this module cannot reach on its own.
 *
 * NetSettings is Arduino-free, so the WiFi/MQTT/portal fields are owned directly. The Modbus link
 * block, the LED behaviour and the per-sensor calibration live behind LinkSettingsManager,
 * LedController and ModbusManager — all Arduino-side — so they arrive through an interface, the same
 * way PackLoader takes its storage (ui_pack_loader.h). Two things follow, both wanted:
 * the form logic stays host-testable, and the adapter can forward to `ui::writeSetting`, which is
 * the same entry point the panel editors use, so the portal earns no private write path (R7.11).
 *
 * A null store is legitimate: those settings then render disabled rather than vanishing, because a
 * silently missing row looks identical to a setting the firmware does not have.
 */
class PortalSettingStore {
 public:
  virtual ~PortalSettingStore() = default;

  /** Current value, for pre-filling the form. False when unavailable. */
  virtual bool readValue(const ui::SettingDescriptor& setting,
                         uint8_t sensorIndex,
                         int32_t& out) const = 0;

  /**
   * Commits a value already validated against its descriptor.
   *
   * False means refused — reported as PortalFieldError::Refused rather than retried.
   */
  virtual bool writeValue(const ui::SettingDescriptor& setting,
                          uint8_t sensorIndex,
                          int32_t value) = 0;
};

class PortalForm {
 public:
  /**
   * Decode buffer for one submitted value.
   *
   * Deliberately LARGER than the widest field (MqttHost, 64 bytes). If it were exactly the
   * capacity, an over-long submission would decode into a full buffer and be indistinguishable
   * from an exactly-fitting one — the request would be silently truncated instead of reported as
   * TooLong, which is the failure mode this whole module exists to avoid.
   */
  static constexpr std::size_t kMaxValueBytes = 192;

  /** Longest `Authorization:` payload decoded. 64 user + ':' + 64 password is more than ample. */
  static constexpr std::size_t kMaxAuthBytes = 160;

  /** Realm for the 401 challenge the adapter sends. */
  static constexpr const char* kAuthRealm = "Water Flow Meter";

  /** Where the form posts. The adapter must route this exact path (R7.11 commits on POST only). */
  static constexpr const char* kFormAction = "/save";

  /**
   * The portal's own credentials have NO catalogue descriptor.
   *
   * R7.9b names them `config.portal.user` and `config.portal.password`, but slice N1c declared
   * fourteen network settings and these two are not among them — they live in NetSettings and
   * nowhere else. A purely catalogue-generated form therefore could not change the portal password,
   * which would leave R7.9a's "the login lands on the change-password form" unimplementable. So
   * they are rendered explicitly, and these are the names they submit under.
   */
  static constexpr const char* kPortalUserField = "config.portal.user";
  static constexpr const char* kPortalPasswordField = "config.portal.password";

  /**
   * `sensorCount` bounds the per-sensor rows (`config.sensor.multiplier@3`). Passed in rather than
   * taken from plc::kNumSensors so the loop bound is exercised at a value other than the real one.
   */
  PortalForm(NetSettings& net, PortalSettingStore* store, std::size_t sensorCount);

  /**
   * Checks an HTTP Basic `Authorization` header value against the stored portal login (R7.9b).
   *
   * Not `WebServer::authenticate()`: that compares with `==` on String, which exits at the first
   * differing byte. One barrier with no exceptions means this one has to be the careful one.
   */
  bool authorize(const char* authorizationHeader) const;

  /** True while the login is still `admin`/`admin`, i.e. while the §7.9a warning is required. */
  bool warningRequired() const;

  // ── Rendering ────────────────────────────────────────────────────────────────────
  //
  // Split so the adapter can add pages (the R7.14 status view) that inherit the §7.9a banner
  // instead of each remembering to draw it.

  void renderDocumentStart(PortalSink& out, const char* title) const;
  void renderDocumentEnd(PortalSink& out) const;

  /** The §7.9a banner. Emits nothing once the password has been changed. */
  void renderDefaultPasswordWarning(PortalSink& out) const;

  /** The `<form>`: the portal login, then every catalogue setting, generated from its descriptor. */
  void renderSettingsForm(PortalSink& out) const;

  /** The outcome block for a POST — the per-field errors, or a confirmation. */
  void renderResult(PortalSink& out, const PortalSubmitResult& result) const;

  /** Whole page for GET. */
  void renderSettingsPage(PortalSink& out) const;

  /** Whole page for POST: the outcome, then the form re-rendered from what is now stored. */
  void renderSubmitPage(PortalSink& out, const PortalSubmitResult& result) const;

  /**
   * Parses an `application/x-www-form-urlencoded` body, validates every field, then commits.
   *
   * Validation is COMPLETE before anything is written (R7.11): one bad field means the whole
   * submission is refused, because a form that half-applies leaves a configuration nobody chose.
   * Over-long text is refused rather than truncated — the opposite of NetSettings::stage, which
   * truncates because a Modbus master's block write legitimately carries NUL padding. A browser's
   * POST carries no padding, so a value that does not fit is a mistake worth naming.
   *
   * The network fields are staged and applied ONCE, so the radio and the MQTT client observe one
   * transition rather than one per field.
   *
   * Known hazard, not fixed here: NetSettings' pending block is shared with the Modbus register map
   * (§5). If a master has staged registers without applying them, this apply() commits those too.
   * Calling revert() first would be worse — it would silently discard a master's in-flight block
   * write — so the choice is deliberate and stays visible.
   */
  PortalSubmitResult submit(const char* body);

 private:
  /** One resolved form field: which descriptor and which storage it belongs to. */
  struct FieldRef {
    /** Null for the two portal credential pseudo-fields, which have no descriptor. */
    const ui::SettingDescriptor* setting = nullptr;
    /** The NetSettings text field, or Count when this field is not stored as text. */
    NetField textField = NetField::Count;
    uint8_t sensorIndex = 0;
    bool writeOnly = false;
    /** Text capacity in bytes, excluding the terminator. Zero for non-text fields. */
    std::size_t textCapacity = 0;
    bool isText = false;
    /** True when the value lives behind PortalSettingStore rather than in NetSettings. */
    bool external = false;
  };

  /** What one pass over the body is for. Three passes, so no per-field value has to be buffered. */
  enum class Pass : uint8_t { Validate, StageNetwork, WriteExternal };

  bool resolve(const char* name, FieldRef& ref) const;
  void runPass(const char* body, Pass pass, PortalSubmitResult& result);
  void handleField(const char* name,
                   const char* value,
                   Pass pass,
                   PortalSubmitResult& result);

  void renderRow(PortalSink& out,
                 const ui::SettingDescriptor& setting,
                 uint8_t sensorIndex,
                 bool perSensor) const;
  void renderTextRow(PortalSink& out,
                     const char* name,
                     const char* label,
                     const char* hint,
                     const char* value,
                     std::size_t capacity,
                     bool writeOnly,
                     bool disabled) const;

  static void addError(PortalSubmitResult& result, const char* field, PortalFieldError error);

  NetSettings& net_;
  PortalSettingStore* store_;
  std::size_t sensorCount_;
};

}  // namespace plc
