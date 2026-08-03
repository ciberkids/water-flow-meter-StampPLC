// Host tests for the configuration portal's form logic (WiFi_MQTT_Connectivity.md §7.6, §8).
//
// §6.3 deleted the on-device text editor, so this form is the only way to type a passphrase into a
// device with no Modbus master attached. Its failure modes are therefore not cosmetic: a rendered
// secret, an empty password field that wipes the stored one, an unescaped topic name, a numeric that
// overflows into something plausible. None of those are reproducible on a bench on demand, and all
// of them are decidable here.
//
// Two habits this file keeps deliberately, because this project has been burned by both:
//   - every secret asserted about is a distinctive CANARY that was stored first. "The password does
//     not appear in the HTML" is trivially true of an empty store, so it proves nothing at rest.
//   - every "nothing was applied" assertion observes BOTH sinks — NetSettings' revision AND the
//     recorded writes of the fake store. Checking only one would let the other leak.
#include "modbus/register_map.h"
#include "net/net_settings.h"
#include "net/portal_form.h"
#include "ui/core/ui_settings_types.h"

#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace {

int failures = 0;
int checks = 0;

void check(bool condition, const char* what) {
  ++checks;
  std::printf("  %-74s %s\n", what, condition ? "ok" : "FAIL");
  if (!condition) ++failures;
}

void checkStr(const char* actual, const char* expected, const char* what) {
  const bool same = std::strcmp(actual, expected) == 0;
  ++checks;
  std::printf("  %-74s %s\n", what, same ? "ok" : "FAIL");
  if (!same) {
    std::printf("      expected \"%s\"\n      actual   \"%s\"\n", expected, actual);
    ++failures;
  }
}

using plc::NetField;
using plc::NetSettings;
using plc::PortalFieldError;
using plc::PortalForm;
using plc::PortalSettingStore;
using plc::PortalSubmitResult;

/** Collects rendered HTML. The firmware adapter forwards the same calls to sendContent(). */
class StringSink : public plc::PortalSink {
 public:
  void writeBytes(const char* data, std::size_t length) override { text_.append(data, length); }
  const std::string& text() const { return text_; }

 private:
  std::string text_;
};

bool contains(const std::string& haystack, const std::string& needle) {
  return haystack.find(needle) != std::string::npos;
}

/**
 * The non-network settings, which live behind ModbusManager on the device.
 *
 * Records every write, so a test can assert that a refused submission wrote NOTHING — not merely
 * that NetSettings was untouched.
 */
class FakeStore : public PortalSettingStore {
 public:
  struct Write {
    std::string bindingId;
    uint8_t sensorIndex;
    int32_t value;
  };

  bool readValue(const ui::SettingDescriptor& setting,
                 uint8_t sensorIndex,
                 int32_t& out) const override {
    (void)sensorIndex;
    if (unreadable) return false;
    out = setting.min;  // a legal value, so rows render enabled
    return true;
  }

  bool writeValue(const ui::SettingDescriptor& setting,
                  uint8_t sensorIndex,
                  int32_t value) override {
    writes.push_back({setting.bindingId, sensorIndex, value});
    // Stands in for the Nyquist refusal, the one rejection ModbusManager can make and this module
    // cannot predict.
    return refuse != setting.bindingId;
  }

  std::vector<Write> writes;
  std::string refuse;
  bool unreadable = false;
};

std::string liveOf(const NetSettings& net, NetField field) {
  char buffer[NetSettings::kMaxValueBytes + 1] = {};
  net.get(field, buffer, sizeof(buffer));
  return buffer;
}

std::string base64(const std::string& in) {
  static const char* kAlphabet =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  uint32_t accumulator = 0;
  int bits = 0;
  for (const unsigned char c : in) {
    accumulator = (accumulator << 8) | c;
    bits += 8;
    while (bits >= 6) {
      bits -= 6;
      out += kAlphabet[(accumulator >> bits) & 0x3F];
    }
  }
  if (bits > 0) out += kAlphabet[(accumulator << (6 - bits)) & 0x3F];
  while (out.size() % 4 != 0) out += '=';
  return out;
}

/** The canaries. Distinctive on purpose: a value that could occur naturally proves nothing. */
constexpr const char* kPskCanary = "PSK-CANARY-9174";
constexpr const char* kMqttPasswordCanary = "MQTTCANARY-8812";
constexpr const char* kPortalPasswordCanary = "PORTALCANARY-3355";
constexpr const char* kSsidCanary = "SSID-CANARY-4420";

/** A device with every secret set to a canary and the login already changed. */
void seed(NetSettings& net) {
  net.stage(NetField::WifiSsid, kSsidCanary);
  net.stage(NetField::WifiPsk, kPskCanary);
  net.stage(NetField::MqttHost, "broker.example.com");
  net.stage(NetField::MqttPassword, kMqttPasswordCanary);
  net.stage(NetField::PortalUser, "plantops");
  net.stage(NetField::PortalPassword, kPortalPasswordCanary);
  net.apply();
}

// ────────────────────────────────────────────────────────────────────────────────────
// R7.9c — the form is generated, so a setting the firmware has cannot be missing from it.
//
// Breaks if: renderSettingsForm stops iterating the catalogue, a kind loses its arm in renderRow,
// the per-sensor loop is off by one, or the portal-login rows (which have no descriptor) are
// dropped. It cannot pass by accident: it names every binding id the firmware declares.
// ────────────────────────────────────────────────────────────────────────────────────
void coverageTests() {
  std::printf("[R7.9c — every catalogue setting reaches the form]\n");

  NetSettings net;
  FakeStore store;
  PortalForm form(net, &store, plc::kNumSensors);
  StringSink sink;
  form.renderSettingsPage(sink);
  const std::string& html = sink.text();

  std::size_t missing = 0;
  std::size_t perSensorSeen = 0;
  for (std::size_t i = 0; i < ui::settingCount(); ++i) {
    const ui::SettingDescriptor* setting = ui::settingAt(i);
    if (!setting) continue;
    if (setting->perSensor) {
      ++perSensorSeen;
      for (std::size_t sensor = 0; sensor < plc::kNumSensors; ++sensor) {
        const std::string name = std::string("name=\"") + setting->bindingId + "@" +
                                 std::to_string(sensor) + "\"";
        if (!contains(html, name)) {
          std::printf("      MISSING %s\n", name.c_str());
          ++missing;
        }
      }
    } else {
      const std::string name = std::string("name=\"") + setting->bindingId + "\"";
      if (!contains(html, name)) {
        std::printf("      MISSING %s\n", name.c_str());
        ++missing;
      }
    }
  }
  std::printf("      %zu catalogue settings, %zu of them per-sensor\n", ui::settingCount(),
              perSensorSeen);
  check(missing == 0, "every declared setting has an input carrying its binding id");
  check(ui::settingCount() >= 24, "and the catalogue really is the 24+ settings R7.9c describes");
  check(perSensorSeen > 0, "including per-sensor ones, so the @index path is actually exercised");

  // The LAST index is where an off-by-one lives, and the one past it is where an over-run does.
  check(contains(html, "name=\"config.sensor.multiplier@7\""),
        "the last sensor's calibration is present (sensorCount = kNumSensors)");
  check(!contains(html, "name=\"config.sensor.multiplier@8\""),
        "and one past the end is not");

  // Both portal credential fields exist even though neither is in the catalogue (R7.9a/b).
  check(contains(html, "name=\"config.portal.user\""), "the portal user has a field");
  check(contains(html, "name=\"config.portal.password\""), "and so does the portal password");

  // Same form, a different sensor count: a hardcoded 8 in the loop would survive the check above.
  NetSettings twoSensorNet;
  PortalForm small(twoSensorNet, &store, 2);
  StringSink smallSink;
  small.renderSettingsForm(smallSink);
  check(contains(smallSink.text(), "name=\"config.sensor.multiplier@1\""),
        "with sensorCount = 2 the second sensor is rendered");
  check(!contains(smallSink.text(), "name=\"config.sensor.multiplier@2\""),
        "and the third is not — the loop bound is the parameter, not a constant");

  // Every kind has to have produced a control, or a whole class of setting is silently unreachable.
  check(contains(html, "type=\"number\""), "numerics render as number inputs");
  check(contains(html, "type=\"checkbox\""), "booleans as checkboxes");
  check(contains(html, "<select"), "enums as selects");
  check(contains(html, "type=\"text\""), "and text as text inputs");
  check(contains(html, "min=\"100\" max=\"2000\""),
        "a numeric carries its descriptor's bounds (config.ledPulsePeriod)");
  check(contains(html, "maxlength=\"63\""),
        "and a text field its maxLength (config.wifi.psk, 63)");

  // renderSubmitPage is what the POST route replies with, so it is exercised rather than shipped
  // on the assumption that composing three tested methods cannot go wrong.
  const PortalSubmitResult rejected = form.submit("config.mqtt.port=70000");
  StringSink submitSink;
  form.renderSubmitPage(submitSink, rejected);
  check(contains(submitSink.text(), "Nothing was saved"),
        "the POST reply carries the outcome block");
  check(contains(submitSink.text(), "name=\"config.mqtt.port\""),
        "and re-renders the form, so the operator can correct the field in place");
  check(contains(submitSink.text(), "</body></html>"), "and closes the document");
}

// ────────────────────────────────────────────────────────────────────────────────────
// §8.1 — a writeOnly value is never rendered.
//
// Breaks if: renderTextRow gains a value for the password branch, a secret is echoed into a
// placeholder or a hint, or the form starts pre-filling from getStaged. The canaries are stored
// FIRST, so this cannot pass because the store is empty.
// ────────────────────────────────────────────────────────────────────────────────────
void secrecyTests() {
  std::printf("\n[§8.1 — no writeOnly value ever reaches the HTML]\n");

  NetSettings net;
  seed(net);
  FakeStore store;
  PortalForm form(net, &store, plc::kNumSensors);
  StringSink sink;
  form.renderSettingsPage(sink);
  const std::string& html = sink.text();

  // The control: a NON-secret canary must be present, otherwise "no secret is present" would pass
  // for a page that renders no values at all.
  check(contains(html, kSsidCanary), "the SSID (not a secret) IS rendered — values do reach the page");
  check(contains(html, "broker.example.com"), "so is the broker host");

  check(!contains(html, kPskCanary), "the WiFi passphrase does not appear anywhere in the page");
  check(!contains(html, kMqttPasswordCanary), "nor the MQTT password");
  check(!contains(html, kPortalPasswordCanary), "nor the portal password");

  // Exact markup, because "type=password somewhere on the page" is not the claim being made.
  check(contains(html,
                 "<input id=\"config.wifi.psk\" name=\"config.wifi.psk\" type=\"password\" "
                 "autocomplete=\"new-password\" value=\"\" maxlength=\"63\" "
                 "placeholder=\"unchanged\">"),
        "the passphrase input is a password field with an empty value");
  check(contains(html,
                 "<input id=\"config.mqtt.password\" name=\"config.mqtt.password\" "
                 "type=\"password\" autocomplete=\"new-password\" value=\"\" maxlength=\"32\" "
                 "placeholder=\"unchanged\">"),
        "and so is the MQTT password");
  check(contains(html, "name=\"config.portal.password\" type=\"password\""),
        "and the portal password");
  check(contains(html, "autocomplete=\"new-password\""),
        "secrets opt out of autofill, so a browser cannot overwrite a stored passphrase on save");
}

// ────────────────────────────────────────────────────────────────────────────────────
// The empty-writeOnly rule. The required test, and the one with the worst consequence if wrong:
// saving any setting would wipe the passphrase, and §6.3 left no way to type it back in.
//
// Breaks if: the empty-value short circuit is removed (the PSK becomes empty), or if it swallows
// non-empty values too (the paired positive assertion catches that), or if a validation error made
// the whole submission a no-op (the changed SSID catches that).
// ────────────────────────────────────────────────────────────────────────────────────
void writeOnlyRoundTripTests() {
  std::printf("\n[an empty secret means \"unchanged\", a non-empty one means \"replace\"]\n");

  NetSettings net;
  seed(net);
  FakeStore store;
  PortalForm form(net, &store, plc::kNumSensors);

  const uint16_t before = net.revision();
  PortalSubmitResult result =
      form.submit("config.wifi.ssid=RetypedNothing&config.wifi.psk=&config.mqtt.password=");
  check(result.ok(), "a submission with both secrets left blank is accepted");
  check(result.committed, "and commits");
  check(net.revision() > before, "the revision bumped");
  checkStr(liveOf(net, NetField::WifiSsid).c_str(), "RetypedNothing",
           "the ordinary field DID change — so this is not a no-op passing by accident");
  checkStr(liveOf(net, NetField::WifiPsk).c_str(), kPskCanary,
           "and the passphrase is still the canary, not empty");
  checkStr(liveOf(net, NetField::MqttPassword).c_str(), kMqttPasswordCanary,
           "nor was the MQTT password cleared");

  // A non-empty value must replace.
  result = form.submit("config.wifi.psk=n3wpassphrase");
  check(result.ok() && result.committed, "a retyped passphrase is accepted");
  checkStr(liveOf(net, NetField::WifiPsk).c_str(), "n3wpassphrase", "and replaces the stored one");

  // The portal login is the same rule, and the case that locks an operator out if it is wrong.
  result = form.submit("config.portal.user=newops&config.portal.password=");
  check(result.ok(), "changing the portal user without retyping the password is accepted");
  checkStr(liveOf(net, NetField::PortalUser).c_str(), "newops", "the user changed");
  checkStr(liveOf(net, NetField::PortalPassword).c_str(), kPortalPasswordCanary,
           "and the password survived — otherwise the operator is locked out by saving a form");

  result = form.submit("config.portal.password=deliberate-change");
  check(result.ok() && result.committed, "a deliberate password change is accepted");
  checkStr(liveOf(net, NetField::PortalPassword).c_str(), "deliberate-change",
           "and takes effect");
}

// ────────────────────────────────────────────────────────────────────────────────────
// Percent- and plus-decoding.
//
// Breaks if: '+' stops meaning space, %XX stops decoding, lowercase hex is rejected, a %00 is
// silently dropped instead of refused, or the pair splitter takes the last '=' instead of the first.
// ────────────────────────────────────────────────────────────────────────────────────
void decodingTests() {
  std::printf("\n[application/x-www-form-urlencoded, including the awkward cases]\n");

  NetSettings net;
  FakeStore store;
  PortalForm form(net, &store, plc::kNumSensors);

  check(form.submit("config.mqtt.baseTopic=watermeter%2Fplant-3%2Finlet").ok(),
        "a percent-encoded topic is accepted");
  checkStr(liveOf(net, NetField::MqttBaseTopic).c_str(), "watermeter/plant-3/inlet",
           "and %2F decodes to a slash");

  check(form.submit("config.wifi.ssid=Guest+Network+2").ok(), "'+' in an SSID is accepted");
  checkStr(liveOf(net, NetField::WifiSsid).c_str(), "Guest Network 2",
           "and decodes to spaces, not to plus signs");

  check(form.submit("config.wifi.psk=a%2Bb+c").ok(), "a passphrase mixing %2B and '+' is accepted");
  checkStr(liveOf(net, NetField::WifiPsk).c_str(), "a+b c",
           "%2B is a literal plus and a bare '+' is a space — the two are not the same character");

  check(form.submit("config.mqtt.baseTopic=%3ca%3e%7Eb").ok(), "lowercase hex escapes are accepted");
  checkStr(liveOf(net, NetField::MqttBaseTopic).c_str(), "<a>~b",
           "and decode the same as uppercase");

  // '=' inside a value: base64-ish secrets end in padding, and splitting on the last '=' eats it.
  check(form.submit("config.mqtt.password=cGFzcw==").ok(), "a value containing '=' is accepted");
  checkStr(liveOf(net, NetField::MqttPassword).c_str(), "cGFzcw==",
           "and keeps every '=' after the first one");

  // ── The refusals ──
  NetSettings strict;
  FakeStore strictStore;
  PortalForm strictForm(strict, &strictStore, plc::kNumSensors);
  strict.stage(NetField::MqttHost, "keep.me");
  strict.apply();
  const uint16_t revision = strict.revision();

  PortalSubmitResult result = strictForm.submit("config.mqtt.host=host%00evil");
  check(!result.ok() && result.errors[0].error == PortalFieldError::BadEncoding,
        "a %00 is refused, not dropped — it would truncate the value past its own validation");
  result = strictForm.submit("config.mqtt.host=trunc%4");
  check(!result.ok() && result.errors[0].error == PortalFieldError::BadEncoding,
        "an escape cut off by the end of the body is refused");
  result = strictForm.submit("config.mqtt.host=bad%zz");
  check(!result.ok() && result.errors[0].error == PortalFieldError::BadEncoding,
        "and one whose digits are not hex");
  result = strictForm.submit("config.mqtt.baseTopic=inject%0D%0Aeverything");
  check(!result.ok() && result.errors[0].error == PortalFieldError::BadEncoding,
        "a CRLF in a topic is refused — these strings end up in MQTT payloads");
  checkStr(liveOf(strict, NetField::MqttHost).c_str(), "keep.me",
           "and none of those four changed the stored host");
  check(strict.revision() == revision, "nor bumped the revision");
}

// ────────────────────────────────────────────────────────────────────────────────────
// Validation, and the R7.11 all-or-nothing rule.
//
// Breaks if: bounds stop being checked, enum membership is treated as range, a numeric overflow
// wraps instead of being refused, or a failing field lets its neighbours through. The mixed
// submission is the load-bearing one: it observes BOTH sinks, so a partial apply cannot hide in the
// store.
// ────────────────────────────────────────────────────────────────────────────────────
void validationTests() {
  std::printf("\n[R7.11 — validated together, then committed, or not at all]\n");

  NetSettings net;
  FakeStore store;
  PortalForm form(net, &store, plc::kNumSensors);
  net.stage(NetField::MqttHost, "original.host");
  net.stageMqttPort(1883);
  net.apply();
  const uint16_t revision = net.revision();

  PortalSubmitResult result = form.submit("config.mqtt.port=70000");
  check(!result.ok() && result.errors[0].error == PortalFieldError::OutOfRange,
        "a port above 65535 is out of range");
  result = form.submit("config.mqtt.port=0");
  check(!result.ok() && result.errors[0].error == PortalFieldError::OutOfRange,
        "and port 0 too — the descriptor's minimum is 1");
  result = form.submit("config.mqtt.port=99999999999999999999");
  check(!result.ok() && result.errors[0].error == PortalFieldError::NotANumber,
        "a value wider than int64 is refused rather than wrapped into something plausible");
  result = form.submit("config.mqtt.port=8883abc");
  check(!result.ok() && result.errors[0].error == PortalFieldError::NotANumber,
        "trailing rubbish is refused, not parsed up to it");
  result = form.submit("config.mqtt.port=");
  check(!result.ok() && result.errors[0].error == PortalFieldError::NotANumber,
        "an empty numeric is refused — it is not zero");

  result = form.submit("config.ledPulseVolume=50");
  check(!result.ok() && result.errors[0].error == PortalFieldError::UnknownOption,
        "50 is inside config.ledPulseVolume's 1..100 range and still not one of its options");
  result = form.submit("config.baudRate=9");
  check(!result.ok() && result.errors[0].error == PortalFieldError::OutOfRange,
        "an enum index past the end of the list is out of range");
  store.writes.clear();
  result = form.submit("config.ledPulseVolume=10");
  check(result.ok(), "an option that IS in the list is accepted, so the check is not refusing all");
  check(store.writes.size() == 1 && store.writes[0].value == 10,
        "and reaches its store — the accept path works, so the refusals above are discriminating");

  result = form.submit("config.wifi.ssid=ThisIsAVeryLongNetworkNameThatExceedsThirtyTwoBytes");
  check(!result.ok() && result.errors[0].error == PortalFieldError::TooLong,
        "an over-long SSID is REFUSED, not truncated (a browser POST carries no padding)");

  result = form.submit("config.does.not.exist=1");
  check(!result.ok() && result.errors[0].error == PortalFieldError::UnknownField,
        "a field the firmware has no setting for is named as unknown");
  result = form.submit("config.sensor.multiplier=5");
  check(!result.ok() && result.errors[0].error == PortalFieldError::UnknownField,
        "a per-sensor setting with no @index is refused rather than applied to sensor 0");
  result = form.submit("config.sensor.multiplier@99=5");
  check(!result.ok() && result.errors[0].error == PortalFieldError::UnknownField,
        "and an index past sensorCount is refused");

  // ── The all-or-nothing case, watching both sinks ──
  store.writes.clear();
  result = form.submit(
      "config.mqtt.host=new.host&config.modbusSlaveId=9&config.mqtt.port=70000&"
      "config.wifi.enabled=1");
  check(!result.ok(), "one bad field refuses the whole submission");
  check(result.errorCount == 1, "and reports exactly the field that was wrong");
  checkStr(liveOf(net, NetField::MqttHost).c_str(), "original.host",
           "the good text field beside it was NOT applied");
  check(net.mqttPort() == 1883, "the port kept its old value");
  check(!net.wifiEnabled(), "the boolean beside it was not applied either");
  check(!net.dirty(), "and nothing was left staged for a later apply to pick up");
  // Nothing in this whole section was allowed to touch the network block: every submission either
  // failed validation or (config.ledPulseVolume) belonged to the store alone.
  check(net.revision() == revision, "and the network revision never moved for any of them");
  check(store.writes.empty(),
        "and the external store recorded ZERO writes — the second sink a NetSettings-only check "
        "would have missed");

  // More errors than there is room for.
  result = form.submit(
      "a=1&b=1&c=1&d=1&e=1&f=1&g=1&h=1&i=1&j=1&k=1&l=1&m=1&n=1&o=1&p=1&q=1&r=1&s=1&t=1");
  check(!result.ok() && result.errorCount == PortalSubmitResult::kMaxErrors && result.moreErrors,
        "a flood of bad fields is capped and says so rather than overrunning the array");
}

// ────────────────────────────────────────────────────────────────────────────────────
// Booleans, and the hidden-companion contract.
//
// Breaks if: the hidden "0" is removed from the form (then "off" cannot be expressed), or if an
// absent field is treated as false (then a partial POST silently disables the radio).
// ────────────────────────────────────────────────────────────────────────────────────
void booleanTests() {
  std::printf("\n[checkboxes: absent means unchanged, which is why the hidden companion exists]\n");

  NetSettings net;
  FakeStore store;
  PortalForm form(net, &store, plc::kNumSensors);

  check(form.submit("config.wifi.enabled=0&config.wifi.enabled=1").ok(),
        "the pair a checked box submits (hidden 0 then 1) is accepted");
  check(net.wifiEnabled(), "and reads as ON");

  check(form.submit("config.wifi.ssid=Somewhere").ok(),
        "a submission that omits the checkbox entirely is accepted");
  check(net.wifiEnabled(),
        "and leaves WiFi ON — an absent field is unchanged, never off (R7.11 stages only what came)");

  check(form.submit("config.wifi.enabled=0").ok(), "the hidden companion alone is accepted");
  check(!net.wifiEnabled(), "and switches WiFi off, which is how 'unchecked' is expressed at all");

  check(form.submit("config.wifi.enabled=on").ok(), "a bare 'on' (a hand-written checkbox) works");
  check(net.wifiEnabled(), "and means true");

  const PortalSubmitResult result = form.submit("config.wifi.enabled=maybe");
  check(!result.ok() && result.errors[0].error == PortalFieldError::NotANumber,
        "anything else is refused rather than guessed at");
  check(net.wifiEnabled(), "and leaves the flag alone");

  StringSink sink;
  form.renderSettingsForm(sink);
  check(contains(sink.text(),
                 "<input type=\"hidden\" name=\"config.wifi.enabled\" value=\"0\">"),
        "the form emits the hidden companion, without which 'off' is unrepresentable");
  check(contains(sink.text(), "type=\"checkbox\" value=\"1\" checked"),
        "and reflects the current state as checked");
}

// ────────────────────────────────────────────────────────────────────────────────────
// Escaping. Two sinks: the pre-filled value attribute, and the error list, which echoes a
// submitted field name straight back.
//
// Breaks if: any of the five metacharacters stops being escaped, if '&' is escaped last (which
// double-encodes), or if renderResult prints the field name raw.
// ────────────────────────────────────────────────────────────────────────────────────
void escapingTests() {
  std::printf("\n[escaping — a base topic is not markup, and neither is a field name]\n");

  char out[128] = {};
  check(plc::portalEscapeHtml("<>&\"'", out, sizeof(out)), "escaping all five metacharacters fits");
  checkStr(out, "&lt;&gt;&amp;&quot;&#39;", "and each becomes its entity");
  plc::portalEscapeHtml("&lt;", out, sizeof(out));
  checkStr(out, "&amp;lt;", "an existing entity is escaped once, not twice");
  plc::portalEscapeHtml("plain", out, sizeof(out));
  checkStr(out, "plain", "ordinary text is untouched");
  char tiny[4] = {};
  check(!plc::portalEscapeHtml("<x", tiny, sizeof(tiny)),
        "a buffer too small reports failure rather than emitting half an entity");
  // "&lt;" needs five bytes with its terminator, so nothing at all should have been written: a
  // partial "&lt" in the output would be markup the browser never closes.
  checkStr(tiny, "", "and leaves the output empty rather than a truncated entity");

  // Sink 1: the stored value, rendered into value="...".
  NetSettings net;
  FakeStore store;
  PortalForm form(net, &store, plc::kNumSensors);
  // Both quote characters matter as much as the brackets: value="..." is the attribute sink, so
  // `w/"><script>` breaks out of a page that escaped only < > &.
  check(form.submit("config.mqtt.baseTopic=w%2F%22%3E%3Cscript%3E%26%27").ok(),
        "a topic full of metacharacters is stored as typed");
  checkStr(liveOf(net, NetField::MqttBaseTopic).c_str(), "w/\"><script>&'",
           "stored verbatim — escaping is a rendering concern, not a storage one");

  StringSink sink;
  form.renderSettingsPage(sink);
  const std::string& html = sink.text();
  // '/' is deliberately NOT escaped: it is not a metacharacter in an attribute value, and topics are
  // full of them. So this is the one exact expected rendering, not a choice of two.
  check(contains(html, "value=\"w/&quot;&gt;&lt;script&gt;&amp;&#39;\""),
        "and rendered with every metacharacter escaped inside the value attribute");
  check(!contains(html, "<script>"), "no executable <script> tag appears in the page");
  check(!contains(html, "\"><script"), "and no attribute breakout either");

  // Sink 2: the reflected field name in the error list.
  const PortalSubmitResult result = form.submit("%3Cimg+src%3Dx+onerror%3D1%3E=1");
  check(!result.ok() && result.errors[0].error == PortalFieldError::UnknownField,
        "an unknown field name is reported back to the operator");
  checkStr(result.errors[0].field, "<img src=x onerror=1>",
          "the raw submitted name is what got recorded (so the escaping has work to do)");
  StringSink errorSink;
  form.renderResult(errorSink, result);
  check(contains(errorSink.text(), "&lt;img src=x onerror=1&gt;"),
        "and the error list escapes it");
  check(!contains(errorSink.text(), "<img"), "so the reflected name cannot become a tag");
}

// ────────────────────────────────────────────────────────────────────────────────────
// R7.9b — one barrier, no exceptions.
//
// Breaks if: authorize() starts returning true unconditionally (the positive case catches it) or
// false unconditionally (every negative case would pass, so the positive case is what makes the
// negatives mean anything). NOT verified here: that the comparison is constant-TIME. A host test
// cannot measure that credibly; what is verified is that it is correct and that both halves are
// always compared.
// ────────────────────────────────────────────────────────────────────────────────────
void authTests() {
  std::printf("\n[HTTP Basic against the stored login — R7.9b]\n");

  NetSettings net;
  FakeStore store;
  PortalForm form(net, &store, plc::kNumSensors);

  const std::string good = "Basic " + base64("admin:admin");
  check(form.authorize(good.c_str()), "the shipped admin/admin is accepted");
  check(!form.authorize(("Basic " + base64("admin:wrong")).c_str()),
        "a wrong password is rejected");
  check(!form.authorize(("Basic " + base64("root:admin")).c_str()),
        "a wrong user with the right password is rejected");
  check(!form.authorize(("Basic " + base64("admin:admin ")).c_str()),
        "a trailing space in the password is rejected — no trimming of secrets");
  check(!form.authorize(("Basic " + base64("admin:adm")).c_str()),
        "a prefix of the password is rejected, so the length is part of the comparison");
  check(!form.authorize(("Basic " + base64("adminadmin")).c_str()),
        "a payload with no colon at all is rejected");
  check(!form.authorize(("Basic " + base64(":admin")).c_str()),
        "an empty user is rejected while the stored user is not empty");
  check(!form.authorize("Basic !!!!not base64!!!!"), "a malformed base64 payload is rejected");
  check(!form.authorize("Bearer abcdef"), "a non-Basic scheme is rejected");
  check(!form.authorize("Basic"), "a scheme with no payload is rejected");
  check(!form.authorize(""), "an empty header is rejected");
  check(!form.authorize(nullptr), "and an absent header is rejected");
  check(form.authorize(("basic " + base64("admin:admin")).c_str()),
        "the scheme token is case-insensitive, as RFC 7235 requires");

  // A password containing a colon: splitting on the LAST colon would break this login.
  net.stage(NetField::PortalUser, "ops");
  net.stage(NetField::PortalPassword, "pa:ss:word");
  net.apply();
  check(form.authorize(("Basic " + base64("ops:pa:ss:word")).c_str()),
        "a password containing colons works — the split is on the first colon only");
  check(!form.authorize(good.c_str()),
        "and the old admin/admin no longer works, so authorize() reads the CURRENT store");
}

// ────────────────────────────────────────────────────────────────────────────────────
// R7.9a — the warning is required until the password has been changed.
//
// Breaks if: the banner is emitted unconditionally (the second half fails) or never (the first).
// ────────────────────────────────────────────────────────────────────────────────────
void warningTests() {
  std::printf("\n[R7.9a — a default password must be nagged about, on every page]\n");

  NetSettings net;
  FakeStore store;
  PortalForm form(net, &store, plc::kNumSensors);

  check(form.warningRequired(), "a shipped device requires the warning");
  StringSink before;
  form.renderSettingsPage(before);
  check(contains(before.text(), "still uses the default password"),
        "and the page carries it, naming the risk rather than saying 'warning'");
  check(contains(before.text(), "do not port-forward"),
        "including R7.10's clear-text caveat, which R7.9 removed the timeout mitigation for");

  // Any page the adapter adds gets it from renderDocumentStart, not from remembering to draw it.
  StringSink header;
  form.renderDocumentStart(header, "Status");
  check(contains(header.text(), "still uses the default password"),
        "renderDocumentStart alone emits it, so a status page cannot omit it");

  check(form.submit("config.portal.password=something-else").ok(), "changing the password works");
  check(!form.warningRequired(), "after which the warning is no longer required");
  StringSink after;
  form.renderSettingsPage(after);
  check(!contains(after.text(), "still uses the default password"),
        "and the page stops showing it — asserting the CHANGE, not the state");
}

// ────────────────────────────────────────────────────────────────────────────────────
// The store boundary: non-network settings, and the one partial-apply case.
//
// Breaks if: external writes stop happening, are sent the wrong sensor index, run before
// validation, or a store refusal is swallowed instead of reported.
// ────────────────────────────────────────────────────────────────────────────────────
void storeTests() {
  std::printf("\n[the injected store — settings that do not live in NetSettings]\n");

  NetSettings net;
  FakeStore store;
  PortalForm form(net, &store, plc::kNumSensors);

  check(form.submit("config.modbusSlaveId=9&config.sensor.maxFlow@5=120").ok(),
        "a non-network submission is accepted");
  check(store.writes.size() == 2, "and reached the store once per field");
  bool sawSlaveId = false;
  bool sawSensor = false;
  for (const FakeStore::Write& write : store.writes) {
    if (write.bindingId == "config.modbusSlaveId" && write.value == 9) sawSlaveId = true;
    if (write.bindingId == "config.sensor.maxFlow" && write.sensorIndex == 5 && write.value == 120) {
      sawSensor = true;
    }
  }
  check(sawSlaveId, "the global setting arrived with its value");
  check(sawSensor, "and the per-sensor one with the index from its @suffix, not sensor 0");

  // What the page SAYS about a store-only submission. NetSettings never committed, so a result that
  // only knew about `committed` reported "nothing changed" over a value it had just written — and
  // this page is the operator's only view of these settings.
  store.writes.clear();
  PortalSubmitResult storeOnly = form.submit("config.modbusSlaveId=7");
  check(storeOnly.ok() && !storeOnly.committed,
        "a store-only submission is accepted without committing the network block");
  check(storeOnly.externalWrites == 1 && storeOnly.storedSomething(),
        "but it did store something, and says so");
  StringSink storeOnlySink;
  form.renderResult(storeOnlySink, storeOnly);
  check(contains(storeOnlySink.text(), "Saved 1 field"),
        "so the page reports it as saved");
  check(!contains(storeOnlySink.text(), "Nothing changed"),
        "and does NOT claim nothing changed — the write is in store.writes to prove otherwise");
  check(store.writes.size() == 1 && store.writes[0].value == 7, "which it is");

  // A refusal only the store can make (the Nyquist rule lives in ModbusManager).
  store.writes.clear();
  store.refuse = "config.sensor.maxFlow";
  const PortalSubmitResult result = form.submit("config.mqtt.host=ok.host&config.sensor.maxFlow@1=9");
  check(!result.ok() && result.errors[0].error == PortalFieldError::Refused,
        "a store refusal is reported as Refused rather than silently dropped");
  check(result.partiallyApplied,
        "and flagged as a PARTIAL apply — the network block committed before the store was asked");
  checkStr(liveOf(net, NetField::MqttHost).c_str(), "ok.host",
           "which is the honest description: the network field really did land");

  // The same refusal with NOTHING else in the body applies nothing at all, so it must not be called
  // a partial save — that would send the operator hunting for a change that never happened.
  const uint16_t revisionBefore = net.revision();
  const PortalSubmitResult onlyRefused = form.submit("config.sensor.maxFlow@1=9");
  check(!onlyRefused.ok() && onlyRefused.errors[0].error == PortalFieldError::Refused,
        "a store-only submission that the store refuses is reported as refused");
  check(!onlyRefused.partiallyApplied && !onlyRefused.storedSomething(),
        "and NOT as partly saved, because nothing landed in either sink");
  check(net.revision() == revisionBefore, "the network block was never touched");
  StringSink refusedSink;
  form.renderResult(refusedSink, onlyRefused);
  check(contains(refusedSink.text(), "Nothing was saved"),
        "so the page says nothing was saved");
  check(!contains(refusedSink.text(), "Partly saved"),
        "rather than claiming some values were stored when none were");

  // No store at all: rows must still appear, disabled, rather than vanishing.
  NetSettings bare;
  PortalForm storeless(bare, nullptr, 2);
  StringSink sink;
  storeless.renderSettingsForm(sink);
  check(contains(sink.text(), "name=\"config.modbusSlaveId\""),
        "with no store the row is still rendered — an absent row looks like an absent setting");
  check(contains(sink.text(), "disabled"),
        "but disabled, so the browser will not submit a value nobody could read");
  check(!storeless.submit("config.modbusSlaveId=9").ok(),
        "and a submission for it is refused rather than dropped on the floor");

  // A store that is present but cannot read this value right now — a sensor index ModbusManager has
  // no block for. Distinct from a null store, and it must degrade the same way.
  FakeStore blind;
  blind.unreadable = true;
  NetSettings blindNet;
  PortalForm blindForm(blindNet, &blind, 2);
  StringSink blindSink;
  blindForm.renderSettingsForm(blindSink);
  check(contains(blindSink.text(), "name=\"config.modbusSlaveId\""),
        "an unreadable value still gets a row");
  check(contains(blindSink.text(), "not readable right now"),
        "labelled as unreadable rather than pre-filled with a value nobody supplied");
}

// ────────────────────────────────────────────────────────────────────────────────────
// Two declarations of one fact: the catalogue's maxLength and NetSettings' capacity.
//
// Breaks if either side is changed alone — which is exactly the drift that would make the portal
// accept a value the store then truncates.
// ────────────────────────────────────────────────────────────────────────────────────
void capacityAgreementTests() {
  std::printf("\n[the catalogue's maxLength must agree with NetSettings' capacity]\n");

  struct Pair {
    const char* bindingId;
    NetField field;
  };
  const Pair pairs[] = {
      {"config.wifi.ssid", NetField::WifiSsid},
      {"config.wifi.psk", NetField::WifiPsk},
      {"config.mqtt.host", NetField::MqttHost},
      {"config.mqtt.user", NetField::MqttUser},
      {"config.mqtt.password", NetField::MqttPassword},
      {"config.mqtt.baseTopic", NetField::MqttBaseTopic},
      {"config.mqtt.discoveryPrefix", NetField::MqttDiscoveryPrefix},
  };
  std::size_t textSettings = 0;
  for (std::size_t i = 0; i < ui::settingCount(); ++i) {
    const ui::SettingDescriptor* setting = ui::settingAt(i);
    if (setting && setting->kind == ui::SettingKind::Text) ++textSettings;
  }
  check(textSettings == sizeof(pairs) / sizeof(pairs[0]),
        "every Text setting in the catalogue is covered by this table");

  std::size_t disagreements = 0;
  for (const Pair& pair : pairs) {
    const ui::SettingDescriptor* setting = ui::findSetting(pair.bindingId);
    if (!setting || setting->maxLength != plc::netFieldCapacity(pair.field)) {
      std::printf("      %s: maxLength %u vs capacity %zu\n", pair.bindingId,
                  setting ? setting->maxLength : 0, plc::netFieldCapacity(pair.field));
      ++disagreements;
    }
  }
  check(disagreements == 0, "and declares the same capacity the store enforces");

  // A value of exactly the capacity must be accepted, and one byte more refused. Off-by-one lives
  // here and nowhere else.
  NetSettings net;
  FakeStore store;
  PortalForm form(net, &store, plc::kNumSensors);
  const std::string exact(plc::netFieldCapacity(NetField::WifiSsid), 'x');
  check(form.submit(("config.wifi.ssid=" + exact).c_str()).ok(),
        "an SSID of exactly 32 bytes is accepted");
  check(liveOf(net, NetField::WifiSsid) == exact, "and stored whole");
  check(!form.submit(("config.wifi.ssid=" + exact + "x").c_str()).ok(),
        "33 bytes is refused");
  check(liveOf(net, NetField::WifiSsid) == exact, "leaving the 32-byte value in place");
}

void constantTimeTests() {
  std::printf("\n[the comparison itself]\n");
  check(plc::portalConstantTimeEquals("admin", "admin"), "equal strings compare equal");
  check(!plc::portalConstantTimeEquals("admin", "admiN"), "a differing last byte is caught");
  check(!plc::portalConstantTimeEquals("admin", "admin "), "a longer candidate is caught");
  check(!plc::portalConstantTimeEquals("admin", "admi"), "and a shorter one");
  check(plc::portalConstantTimeEquals("", ""), "two empty strings are equal");
  check(!plc::portalConstantTimeEquals("", "x"), "an empty candidate does not match a set password");
  check(!plc::portalConstantTimeEquals(nullptr, "x"), "and a null one is never equal");
}

}  // namespace

int main() {
  std::printf("plc::PortalForm — the configuration portal's form logic (§7.6, §7.9a, §8)\n\n");
  coverageTests();
  secrecyTests();
  writeOnlyRoundTripTests();
  decodingTests();
  validationTests();
  booleanTests();
  escapingTests();
  authTests();
  warningTests();
  storeTests();
  capacityAgreementTests();
  constantTimeTests();
  std::printf("\n%s (%d checks, %d failures)\n", failures == 0 ? "ALL PASSED" : "FAILURES", checks,
              failures);
  return failures == 0 ? 0 : 1;
}
