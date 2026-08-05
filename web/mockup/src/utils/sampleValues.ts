import type { FirmwareValueDefinition } from "../types/firmwareActions";

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
  "telemetry.status": "All sensors ready",
  "diagnostics.pollingRate": "3.31",
  "diagnostics.undersampling": "",

  // ── Config / editor ──────────────────────────────────────────────────────────────
  "config.editor.pending": "9",
  "config.selectedSensor": "3",
  "config.uartFrameSummary": "8N1",
  "config.sensor.nyquistWarning": "",
  "page.title": "System Status",
  "legend.led": "G ready  R volume  B card",
  "countdown.value": "3"
};

/** `sensor.<n>.<metric>` — the resolver's exact format, including the index prefix. */
function sampleForSensorBinding(binding: string): string | undefined {
  const match = /^sensor\.(\d+)\.(.+)$/.exec(binding);
  if (!match) {
    return undefined;
  }
  const index = match[1];
  switch (match[2]) {
    case "status":
      return "OK";
    case "instantFlow":
    case "maxFlowSinceReset":
      return `${index}:   2.34 L/s`;
    case "cumulativeLiters":
    case "sessionLiters":
      return `${index}: 123.45 L`;
    case "cumulativeM3":
    case "sessionM3":
      return `${index}:   0.12 m^3`;
    default:
      return undefined;
  }
}

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

  const specific = kByBinding[binding] ?? sampleForSensorBinding(binding);
  if (specific !== undefined) {
    return specific;
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
