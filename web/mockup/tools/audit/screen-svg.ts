/**
 * Render a specified screen as SVG, at the device's own glyph advances.
 *
 * `npx tsx tools/audit/screen-svg.ts <proposal.json> [--worst]`
 *
 * Every character is positioned individually at `x + i * advance` — 6 px for text and badges, 7 px for a
 * value (ui_renderer.cpp:16-17). That makes the output faithful regardless of which monospace font the
 * viewer has: the font's own advance never enters into it, so nothing can drift the way it would if the
 * string were drawn as one run and left to the browser's metrics.
 *
 * Colours come from src/data/themeTokens.json, the palette the firmware itself loads.
 *
 * `--worst` draws each element's declared worst case, which is what the geometry audit checks. Without it
 * the gallery draws realistic values, because a panel full of 9999999.99 tells you the layout holds but not
 * whether it reads.
 */

import fs from "node:fs";
import path from "node:path";
import { fileURLToPath } from "node:url";
import type { FirmwareValueDefinition } from "../../src/types/firmwareActions";
import { sampleValueFor } from "../../src/utils/sampleValues";
import { formatSetting, pendingRawFor, rangeHintFor, settingOfScreen } from "../../src/utils/settingHints";

const here = path.dirname(fileURLToPath(import.meta.url));
const mockupRoot = path.resolve(here, "..", "..");
const theme = JSON.parse(fs.readFileSync(path.join(mockupRoot, "src", "data", "themeTokens.json"), "utf-8"));
const manifest = JSON.parse(
  fs.readFileSync(path.join(mockupRoot, "src", "data", "actionManifest.json"), "utf-8")
) as { values: FirmwareValueDefinition[] };
const definitionById = new Map(manifest.values.map((v) => [v.id, v]));

const W = 240;
const H = 135;
const GLYPH_TEXT = 6;
const GLYPH_VALUE = 7;
const BADGE_PAD_X = 3;
const BADGE_PAD_Y = 2;

interface Element {
  id: string;
  kind: string;
  x: number;
  y: number;
  width?: number;
  height?: number;
  content?: string;
  binding?: string;
  emphasis?: string;
  metadata?: { assetId?: string };
  worst?: string;
}

interface Screen {
  id: string;
  name: string;
  description?: string;
  elements: Element[];
}

/**
 * Realistic values for the gallery — a two-sensor installation with one channel uncalibrated and one that
 * has hit its ceiling, which is the state worth looking at rather than an all-zeros or all-nines panel.
 */
const TYPICAL: Record<string, string> = {
  "telemetry.totalFlowLpm": " 279.30",
  // The panel shows m3 only (§2a.1); the label is now its own element, so this is the bare number
  // the resolver emits rather than a whole row.
  "telemetry.totalVolumeM3": "    0.99",
  "telemetry.totalVolumeLiters": "987.60",
  "telemetry.maxFlowLpm": "Max Flow:  150.00 L/m (S2)",
  "legend.status": "WiFi OK  MQTT OK  LED 1p/10L",
  "sensor.1.instantFlow": "1:  140.40",
  "sensor.2.instantFlow": "2:  138.90",
  "sensor.3.instantFlow": "3: --",
  "sensor.4.instantFlow": "4: SET?",
  "sensor.5.instantFlow": "5:    0.00",
  "sensor.6.instantFlow": "6: --",
  "sensor.7.instantFlow": "7: --",
  "sensor.8.instantFlow": "8:   96.20",
  "sensor.1.cumulativeM3": "1:     12.34",
  "sensor.2.cumulativeM3": "2:      9.87",
  "sensor.3.cumulativeM3": "3: --",
  "sensor.4.cumulativeM3": "4: SET?",
  "sensor.5.cumulativeM3": "5:      0.00",
  "sensor.6.cumulativeM3": "6: --",
  "sensor.7.cumulativeM3": "7: --",
  "sensor.8.cumulativeM3": "8:      3.02",
  "sensor.1.sessionM3": "1:      1.21",
  "sensor.2.sessionM3": "2:      0.94",
  "sensor.3.sessionM3": "3: --",
  "sensor.4.sessionM3": "4: SET?",
  "sensor.5.sessionM3": "5:      0.00",
  "sensor.6.sessionM3": "6: --",
  "sensor.7.sessionM3": "7: --",
  "sensor.8.sessionM3": "8:      0.31",
  "sensor.1.maxFlowSinceReset": "1:   98.40",
  "sensor.2.maxFlowSinceReset": "2:  150.00 MAX",
  "sensor.3.maxFlowSinceReset": "3: --",
  "sensor.4.maxFlowSinceReset": "4: SET?",
  "sensor.5.maxFlowSinceReset": "5:    0.00",
  "sensor.6.maxFlowSinceReset": "6: --",
  "sensor.7.maxFlowSinceReset": "7: --",
  "sensor.8.maxFlowSinceReset": "8:   96.20",
  "sensor.1.status": "OK",
  "sensor.2.status": "OK",
  "sensor.3.status": "--",
  "sensor.4.status": "SET?",
  "sensor.5.status": "OK",
  "sensor.6.status": "--",
  "sensor.7.status": "--",
  "sensor.8.status": "OK",
  "net.wifi.enabled": "On",
  "net.wifi.state": "OK",
  "net.wifi.ssid": "PlantFloor",
  "net.wifi.ip": "192.168.1.50",
  "net.wifi.rssi": "-57",
  "net.ap.ssid": "water_flow_meter_309245",
  "net.ap.password": "KU67QJ4DRPDP",
  "net.ap.ip": "192.168.4.1",
  "net.portal.remaining": "540",
  "net.mqtt.state": "OK",
  "net.mqtt.enabled": "On",
  "net.mqtt.host": "broker.plant.local",
  "net.mqtt.port": "1883",
  "net.mqtt.period": "30 s",
  "net.mqtt.qos": "1",
  "net.mqtt.haDiscovery": "On",
  "net.status": "WiFi OK  MQTT OK"
};

/**
 * What a binding draws on THIS screen.
 *
 * The settings used to be listed in TYPICAL above, which made this file a second home for facts the
 * manifest descriptor already held — and the two disagreed: the gallery drew Modbus ID as `1` where
 * the running app drew `42`, so a mockup reviewed here could not be trusted to match the simulator.
 * Now only device-memory values (sensors, telemetry, net) are sampled locally, and everything a
 * descriptor can answer is asked of the descriptor through the same helpers the app uses.
 *
 * The range hint and the pending value depend on WHICH screen is being drawn, which is why the
 * screen is a parameter rather than a lookup key: one static string for all of them is exactly the
 * defect that put the Modbus range on the Baud Rate page.
 */
function resolveTypical(binding: string, screen: Screen): string {
  const local = TYPICAL[binding];
  if (local !== undefined) return local;

  const setting = settingOfScreen(screen, definitionById);
  if (binding === "config.editor.range") {
    return setting ? rangeHintFor(setting) : "";
  }
  if (binding === "config.editor.pending") {
    return setting && setting.type !== "string" ? formatSetting(setting, pendingRawFor(setting)) : "";
  }
  return sampleValueFor(binding, definitionById.get(binding), "sample");
}

const escapeXml = (raw: string) =>
  raw.replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;").replace(/"/g, "&quot;");

function colorFor(element: Element): string {
  if (element.kind === "value") return theme.colors.value;
  if (element.emphasis === "muted") return theme.colors.textMuted;
  if (element.emphasis === "strong") return theme.colors.textStrong;
  return theme.colors.textPrimary;
}

/** One <text> per character, so the advance is the device's and not the font's. */
function glyphRun(text: string, x: number, y: number, advance: number, size: number, fill: string, bold: boolean) {
  const parts: string[] = [];
  for (let i = 0; i < text.length; i += 1) {
    const ch = text[i];
    if (ch === " ") continue;
    parts.push(
      `<text x="${x + i * advance}" y="${y + 7}" font-size="${size}" fill="${fill}"` +
        `${bold ? ' font-weight="700"' : ""}>${escapeXml(ch)}</text>`
    );
  }
  return parts.join("");
}

export function renderScreen(screen: Screen, useWorst: boolean): string {
  const body: string[] = [];
  body.push(`<rect x="0" y="0" width="${W}" height="${H}" fill="${theme.colors.displayBackground}"/>`);

  for (const element of screen.elements) {
    const isGeometry = element.kind === "box" || element.kind === "icon" || element.kind === "scrollbar";

    if (element.kind === "scrollbar") {
      const w = element.width ?? 5;
      const h = element.height ?? 100;
      body.push(
        `<rect x="${element.x}" y="${element.y}" width="${w}" height="${h}" fill="none" ` +
          `stroke="${theme.colors.badgeBorder}" stroke-width="0.5" opacity="0.5"/>` +
          `<rect x="${element.x + 1}" y="${element.y + 1}" width="${w - 2}" height="${Math.round(h / 9)}" ` +
          `fill="${theme.colors.badgeBorder}"/>`
      );
      continue;
    }

    if (element.kind === "icon") {
      // drawFlowDots: four dots in a chase, the leftmost lit. radius = min(spacing, height) / 3.
      const w = element.width ?? 40;
      const h = element.height ?? 12;
      const count = 4;
      const spacing = w / count;
      const r = Math.min(spacing, h) / 3;
      const cy = element.y + h / 2;
      for (let i = 0; i < count; i += 1) {
        const cx = element.x + spacing / 2 + i * spacing;
        const lit = i === 0;
        body.push(
          `<circle cx="${cx}" cy="${cy}" r="${r}" fill="${lit ? theme.colors.value : "none"}" ` +
            `stroke="${theme.colors.value}" stroke-width="0.75" opacity="${lit ? 1 : 0.35}"/>`
        );
      }
      continue;
    }

    if (element.kind === "box") {
      body.push(
        `<rect x="${element.x}" y="${element.y}" width="${element.width ?? 40}" height="${element.height ?? 12}" ` +
          `fill="${theme.colors.badgeBackground}" stroke="${theme.colors.badgeBorder}" stroke-width="0.5"/>`
      );
      continue;
    }

    if (isGeometry) continue;

    const text = useWorst
      ? element.worst ?? element.content ?? ""
      : element.binding
        ? resolveTypical(element.binding, screen)
        : element.content ?? "";
    if (!text) continue;

    const advance = element.kind === "value" ? GLYPH_VALUE : GLYPH_TEXT;
    const size = element.kind === "value" ? 10 : 8.5;

    if (element.kind === "badge") {
      const bw = text.length * advance + BADGE_PAD_X * 2;
      body.push(
        `<rect x="${element.x}" y="${element.y}" width="${bw}" height="${8 + BADGE_PAD_Y * 2}" ` +
          `fill="${theme.colors.badgeBackground}" stroke="${theme.colors.badgeBorder}" stroke-width="0.5"/>`
      );
      body.push(glyphRun(text, element.x + BADGE_PAD_X, element.y + BADGE_PAD_Y, advance, size, theme.colors.textPrimary, false));
      continue;
    }

    body.push(glyphRun(text, element.x, element.y, advance, size, colorFor(element), element.emphasis === "strong"));
  }

  return (
    `<svg viewBox="0 0 ${W} ${H}" width="${W}" height="${H}" xmlns="http://www.w3.org/2000/svg" ` +
    `font-family="ui-monospace, SFMono-Regular, Menlo, Consolas, monospace" shape-rendering="crispEdges">` +
    body.join("") +
    `</svg>`
  );
}

/**
 * CLI entry, only when this file IS the entry point.
 *
 * Without the guard, importing `renderScreen` from another tool ran this block against the
 * importer's argv — so `screen-gallery.mts out.html` tried to parse its own output path as a screen.
 */
const invokedDirectly = process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url);
if (invokedDirectly && process.argv[2]) {
  const screen = JSON.parse(fs.readFileSync(path.resolve(process.argv[2]), "utf-8")) as Screen;
  process.stdout.write(renderScreen(screen, process.argv.includes("--worst")));
}
