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
import { clipToPanel } from "../../src/utils/layout";
import {
  createSensorTable,
  resolveSensorBinding,
  setSensor,
  type SimulatedSensor
} from "../../src/utils/sensorConfig";

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
 * The installation the gallery depicts: two channels running, one of them pinned at its ceiling, one
 * uncalibrated, three out of service.
 *
 * A STATE, not a set of strings. The forty-odd `sensor.N.metric` rows used to be written out by hand
 * here, which made this file a third home for the row format alongside the firmware and
 * `sensorConfig.ts` — and all three drifted: the firmware and the app rendered `1: 2.34 L/s` under an
 * `Instant Flow (L/m)` header while this file alone had the agreed `1:  140.40`. Choosing the state
 * and resolving it through the same function the app uses means the gallery cannot show a row the
 * simulator would not.
 */
const gallerySensors: SimulatedSensor[] = (() => {
  let table = createSensorTable();
  // 2.34 L/s is 140.4 L/min, comfortably inside the default 150 ceiling.
  table = setSensor(table, 2, { instantFlowLps: 2.5, maxFlowLps: 2.5, qMaxLpm: 150 });  // at MAX
  for (const out of [3, 6, 7]) table = setSensor(table, out, { connected: false });
  // Uncalibrated: connected, but no valid configuration yet, so every row reads SET?.
  table = setSensor(table, 4, { ready: false, multiplier: 0 });
  table = setSensor(table, 5, { instantFlowLps: 0, maxFlowLps: 0 });
  return table;
})();

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

  // Per-sensor rows come from the REAL resolver over the state above, so the gallery and the
  // simulator cannot disagree about the format.
  const fromSensors = resolveSensorBinding(binding, gallerySensors, 2);
  if (fromSensors !== undefined) return fromSensors;

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

/**
 * One <text> per character, so the advance is the device's and not the font's.
 *
 * NO weight is ever set. Font0 has no bold, so emphasis on the device is a colour change and nothing
 * else; drawing 700 here showed a distinction the panel cannot make.
 */
function glyphRun(text: string, x: number, y: number, advance: number, size: number, fill: string) {
  const parts: string[] = [];
  for (let i = 0; i < text.length; i += 1) {
    const ch = text[i];
    if (ch === " ") continue;
    parts.push(
      `<text x="${x + i * advance}" y="${y + 7}" font-size="${size}" fill="${fill}">${escapeXml(ch)}</text>`
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
    // Clipped exactly as UiRenderer clips, so the gallery cannot show a hostname the panel can't.
    const shown = element.kind === "badge" ? text : clipToPanel(text, advance, element.x, element.width);

    if (element.kind === "badge") {
      const bw = shown.length * advance + BADGE_PAD_X * 2;
      body.push(
        `<rect x="${element.x}" y="${element.y}" width="${bw}" height="${8 + BADGE_PAD_Y * 2}" ` +
          `fill="${theme.colors.badgeBackground}" stroke="${theme.colors.badgeBorder}" stroke-width="0.5"/>`
      );
      body.push(glyphRun(shown, element.x + BADGE_PAD_X, element.y + BADGE_PAD_Y, advance, size, theme.colors.textPrimary));
      continue;
    }

    body.push(glyphRun(shown, element.x, element.y, advance, size, colorFor(element)));
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
