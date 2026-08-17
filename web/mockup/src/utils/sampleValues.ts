import type { FirmwareValueDefinition } from "../types/firmwareActions";
import { formatSetting, formatSettingText, sampleRawFor } from "./settingHints";

/**
 * A plausible rendering for a bound value, so the panel shows what the DEVICE will show.
 *
 * The viewport used to render every value element as `{{<description>}}` — the catalogue's English
 * description in braces. With 137 value elements that made the whole panel unreadable: a row that
 * says `{{Sensor 3 instantaneous flow}}` tells you nothing about whether the layout fits, which is
 * the one question the viewport exists to answer. `12.34 L/s` does.
 *
 * These are SAMPLES, not simulations. They are shaped like the firmware's real output — the same
 * field widths, the same units, the same "--" for withheld — because the point is to see whether the
 * text fits a 240x135 panel and reads correctly. A per-element override (the Values panel) still wins,
 * so anyone checking a specific case can pin it.
 *
 * Where a specific id has a shape the type alone cannot imply, it is listed. Everything else falls
 * back to the type, which keeps this table from having to grow with the catalogue.
 */
const kByBinding: Record<string, string> = {
  // ── Network (§3.4, §7.3) — the strings the resolver actually emits ────────────────
  "net.status": "WiFi OK  MQTT OK",
  "net.wifi.state": "OK",
  "net.wifi.ssid": "PlantFloor",
  "net.wifi.ip": "192.168.1.50",
  "net.wifi.rssi": "-57",
  "net.mqtt.state": "OK",
  "net.ap.ssid": "water_flow_meter_309245",
  "net.ap.password": "KU67QJ4DRPDP",
  "net.ap.ip": "192.168.4.1",
  "net.portal.remaining": "540",

  // ── Aggregates and diagnostics ───────────────────────────────────────────────────
  "telemetry.total": "Total 1234.56 L | Flow 2.34 L/s",
  "telemetry.totalFlowLps": "2.34",
  "telemetry.totalVolumeLiters": "1234.56",
  /**
   * The DECLARED WORST CASE, 38 characters — not the happy string it used to hold.
   *
   * It read "All sensors ready" (17), which stopped being the widest thing this binding can say when the
   * summary learned to report uncalibrated channels: with eight of each kind at once the device draws
   * "8 channels not calibrated | 8 warnings". Same reasoning as `nav.position` below — tools/audit/screen-geometry.ts
   * measures the SAMPLE, so a sample narrower than the string would let the audit pass a row that
   * overflows on the panel. The live app resolves this from the sensor table via `aggregate()`, which is
   * also why `legend.warning` needs no entry: nothing but the audit ever reads one, and no screen binds it.
   */
  "telemetry.status": "8 channels not calibrated | 8 warnings",
  /**
   * `diagnostics.pollingRateKhz` — and the KEY is the fix here.
   *
   * It was `diagnostics.pollingRate`, which is not a binding id the manifest carries: the catalogue
   * declares `diagnostics.pollingRateKhz` (ui/core/ui_value_catalogue.cpp:138) and the resolver answers
   * that name (ui_bindings.cpp:543). So this entry matched nothing, the row fell through to the generic
   * `(not set)`, and the geometry audit measured a nine-character placeholder for a four-character
   * number — the one figure every sampling verdict on the panel is computed from, unreadable in the
   * mockup because of a missing four letters.
   *
   * `3.3`, not `3.31`, because the firmware formats it `%.1f`. The live app resolves it from the Sampler
   * control rather than from here; this is what the audit measures, so it must be the widest thing the
   * format can produce at a plausible rate — four characters, as in `12.3`, would be wider still, but
   * inventing a rate no part of this project uses to pad an audit is how samples stop meaning anything.
   */
  "diagnostics.pollingRateKhz": "3.3",
  "diagnostics.undersampling": "",

  // ── Config / editor ──────────────────────────────────────────────────────────────
  "config.editor.pending": "9",
  "config.selectedSensor": "3",
  "config.uartFrameSummary": "8N1",
  "config.sensor.nyquistWarning": "",
  // `L2 3/8` — depth, then entry within the level. Six characters is also the declared worst case,
  // which matters: with no sample this fell back to the generic string "(not set)", nine characters,
  // and the geometry audit correctly reported it colliding with the sensor number beside it.
  "nav.position": "L2 3/8",
  // The default unit; the live app resolves this from config.flowUnit.
  "telemetry.flowUnitLabel": "L/m",
  /**
   * The DECLARED WORST CASE, 20 characters, and that is the whole reason this key exists.
   *
   * The live simulator never reads it — `aggregate()` answers this binding above the sample layer, from
   * the simulated clock. But tools/audit/screen-geometry.ts calls sampleValueFor directly, so without an
   * entry here the audit measured the generic string fallback `(not set)` — nine characters — and would
   * have passed P3's new row while saying nothing about the 20-character timestamp actually rendered
   * there. Same reasoning as `nav.position` above: the sample is what the geometry audit checks against,
   * so it has to be the widest thing the device can print, not a typical one.
   *
   * The three shorter renderings are CLOCK UNSET, AWAITING CLOCK and UNKNOWN (ui_bindings.cpp).
   */
  "telemetry.sessionStart": "2026-08-12 14:32 UTC",
  "page.title": "System Status",
  "legend.led": "G ready  R volume  B card",
  "countdown.value": "3"
};

/**
 * There is deliberately no `sensor.<n>.<metric>` sample any more.
 *
 * There was one, and it had gone stale in three ways at once: it carried the `3: ` prefix that is now
 * a row label, it still said `L/s` after storage moved to L/min, and it ignored the display unit. It
 * was also unreachable — `resolveSensorBinding` answers every sensor metric from the simulated table
 * and runs first — so it was a second home for a format, silently wrong, waiting for the day
 * something stopped answering and it became visible.
 */

/**
 * The value to draw for a binding.
 *
 * `mode` decides what an unlisted binding falls back to:
 *   - `sample` — plausible device output. What you want when judging layout and legibility.
 *   - `id`     — the short binding id in braces. What you want in the design tab, where knowing
 *                WHICH value is bound matters more than how it looks. The id, never the
 *                description: `{{net.wifi.rssi}}` is readable, `{{Signal strength while
 *                associated}}` is what made the panel unusable.
 */
export function sampleValueFor(
  binding: string,
  value: FirmwareValueDefinition | undefined,
  mode: "sample" | "id"
): string {
  if (mode === "id") {
    return `{{${binding}}}`;
  }

  const specific = kByBinding[binding];
  if (specific !== undefined) {
    return specific;
  }

  /**
   * A SETTING is formatted from its own descriptor, never from its bare type.
   *
   * The type fallback below drew every numeric setting as `42`, so the Baud Rate page read `42` — not
   * a baud rate at all — and Parity, Stop Bits and QoS read `42` alongside a correct range hint that
   * contradicted them. The descriptor already says which values are legal and how they are labelled,
   * and `formatSetting` is the device's own rendering of it, so this asks the descriptor instead.
   */
  if (value?.category === "setting") {
    return value.type === "string" ? formatSettingText(value) : formatSetting(value, sampleRawFor(value));
  }

  // Fall back to the declared type, so a value added to the catalogue renders sensibly without
  // anyone having to remember this file exists.
  const unit = value?.unit ? ` ${value.unit}` : "";
  switch (value?.type) {
    case "boolean":
      return "On";
    case "number":
      return `42${unit}`;
    case "string":
      return "(not set)";
    default:
      // Unknown to the catalogue entirely — that IS worth showing as unresolved, because the export
      // gate would reject it and the panel should not disguise it as working.
      return `{{${binding}}}`;
  }
}
