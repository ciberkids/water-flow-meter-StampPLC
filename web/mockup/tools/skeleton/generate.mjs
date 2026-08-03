#!/usr/bin/env node
/**
 * One-shot scaffold generator for the hierarchical navigation model
 * (docs/Requirements/feature addition/Display_UI_Requirements.md §5).
 *
 * This is NOT part of the build. It derives the *structure* of the screen tree
 * from the firmware catalogue (actionManifest.json) so that ~27 near-identical
 * screens are consistent by construction rather than hand-authored 27 times.
 * Once generated, the dataset is the source of truth again: per decision D5 the
 * JSON owns screen order, sub-level nesting, and the placement and wording of
 * text, and a designer is expected to tune placement in the design tool.
 *
 * Deliberately does NOT regenerate the existing info pages P0-P6. Their
 * two-column 8-sensor layouts were hand-tuned and already fit landscape
 * (y = 2..133); only their footers sat out of bounds. Regenerating them would be
 * inventing a design nobody asked for.
 *
 * Usage: node tools/skeleton/generate.mjs [--write]
 */
import fs from "node:fs";
import path from "node:path";
import process from "node:process";

const ROOT = path.resolve(import.meta.dirname, "..", "..");
const SCREENS = path.join(ROOT, "src", "data", "screens.json");
const MANIFEST = path.join(ROOT, "src", "data", "actionManifest.json");

// ── Landscape geometry (decision D3): 240 wide x 135 tall ────────────────────
const W = 240;
const H = 135;
const L = {
  headerY: 2,
  bodyY: 30,
  valueY: 50,
  savedLabelY: 78,
  savedValueY: 92,
  footerY: 124,
  padX: 8,
  scrollbar: { x: 232, y: 14, width: 5, height: 104 }
};

const manifest = JSON.parse(fs.readFileSync(MANIFEST, "utf8"));
const byId = new Map(manifest.values.map((v) => [v.id, v]));

const A = {
  next: "ui.action.page.next",
  prev: "ui.action.page.previous",
  descend: "ui.action.nav.descend",
  back: "ui.action.nav.back",
  escape: "ui.action.nav.escape",
  inc: "config.action.value.increment",
  dec: "config.action.value.decrement",
  commit: "config.action.value.commit",
  discard: "config.action.value.discard"
};

const text = (id, y, content, extra = {}) => ({
  id, kind: "text", x: L.padX, y, content, ...extra
});
const value = (id, y, binding, extra = {}) => ({
  id, kind: "value", x: L.padX, y, binding, ...extra
});
const scrollbar = () => ({ id: "level-position", kind: "scrollbar", ...L.scrollbar });
const overlay = () => ({ id: "overlay-bg", kind: "box", x: 0, y: 0, width: W, height: H });

const btn = (id, label, button, gesture, actionId, targetScreenId) => {
  const flow = { id, label, trigger: { type: "button", button, gesture } };
  if (targetScreenId) flow.targetScreenId = targetScreenId;
  if (actionId) flow.actionId = actionId;
  return flow;
};

/** Wires a level into a ring: UP/DOWN step siblings and wrap. */
function ringFlows(ids, index) {
  const prev = ids[(index - 1 + ids.length) % ids.length];
  const next = ids[(index + 1) % ids.length];
  return [
    btn("f-next", "Next entry", "down", "short", A.next, next),
    btn("f-prev", "Previous entry", "up", "short", A.prev, prev)
  ];
}

const screens = [];

// ── L1 Config root: C1..C7 + BACK ───────────────────────────────────────────
const DEVICE = [
  { page: "C1", id: "config-c1-modbus-id", title: "Modbus ID", binding: "config.modbusSlaveId" },
  { page: "C2", id: "config-c2-baud-rate", title: "Baud Rate", binding: "config.baudRate" },
  { page: "C3", id: "config-c3-parity", title: "Parity", binding: "config.parity" },
  { page: "C4", id: "config-c4-stop-bits", title: "Stop Bits", binding: "config.stopBits" },
  { page: "C5", id: "config-c5-led-pulse-vol", title: "LED Pulse Volume", binding: "config.ledPulseVolume" },
  { page: "C6", id: "config-c6-led-pulse-period", title: "LED Pulse Period", binding: "config.ledPulsePeriod" },
  { page: "C7", id: "config-c7-sensor-select", title: "Sensors", binding: null }
];
const CONFIG_RING = [...DEVICE.map((d) => d.id), "config-root-back"];

const editorId = (settingScreenId) => `${settingScreenId}-edit`;

/**
 * The completeness rule (Loadable_UI_Menu_Packs §3.0.1) says a menu is invalid unless every
 * settable value has a reachable editor. Enforcing it HERE is the cheapest place: the
 * skeleton is where the default menu comes from, so a setting added to the firmware
 * catalogue without a screen becomes a loud generator failure rather than an export failure
 * discovered later, or worse, a setting the operator simply cannot reach.
 *
 * Deliberately not automatic. Deriving a title from a binding id gives labels like
 * "Modbusslaveid", and this text is what a human reads on a 240x135 panel. So the lists
 * stay hand-written and the generator checks they are exhaustive.
 */
function assertCoversEverySetting(deviceList, sensorList) {
  const declared = new Set([...deviceList, ...sensorList].map((d) => d.binding).filter(Boolean));
  const missing = manifest.values
    .filter((v) => v.category === "setting")
    // TEXT SETTINGS ARE EXEMPT. There is no on-device text entry: three buttons and a 97-position
    // character wheel is not a usable way to type a 63-character WPA2 passphrase, and the owner
    // ruled it out. Text reaches the device by the three surfaces that were always the real ones —
    // the configuration web portal (§7.6), the RS485 register block (§5.2), and the SD credential
    // file (Q2).
    //
    // This exemption is the ONLY hole in the completeness rule, and it is safe because it is decided
    // by KIND rather than by a runtime condition: `type === "string"` is statically knowable, so the
    // gate still proves every setting an operator can change AT THE PANEL has an editor there.
    // Guarded editors were rejected for exactly the opposite reason (R7.3) — a guard is not
    // statically decidable. See §6.3.
    .filter((v) => v.type !== "string")
    .map((v) => v.id)
    .filter((id) => !declared.has(id));
  if (missing.length > 0) {
    throw new Error(
      `the skeleton would violate the completeness rule: no editor screen for ` +
      `${missing.join(", ")}. Add each to DEVICE (device-wide) or SENSOR_SETTINGS ` +
      `(per-sensor) in tools/skeleton/generate.mjs, with a title a human can read. ` +
      `(Text settings are exempt — they are set via the portal, RS485 or the SD file.)`
    );
  }
}

DEVICE.forEach((d, i) => {
  const isDescent = d.binding === null;
  const target = isDescent ? "config-sensor-1" : editorId(d.id);
  const v = d.binding ? byId.get(d.binding) : null;
  const unit = v?.unit ? ` ${v.unit}` : "";
  screens.push({
    id: d.id,
    name: `${d.page} — ${d.title}`,
    description: `Config root entry ${d.page}. ENTER descends; UP/DOWN move within the level.`,
    elements: [
      text("hdr-title", L.headerY, `Config > ${d.title}`),
      text("field-label", L.bodyY, isDescent ? "Select a sensor" : `Current${unit}`, { emphasis: "muted" }),
      ...(d.binding ? [value("field-value", L.valueY, d.binding, { emphasis: "strong" })]
                    : [text("field-value", L.valueY, "Sensors 1-8 >", { emphasis: "strong" })]),
      text("footer-hint", L.footerY,
        isDescent ? "UP/DN page  ENTER open  hold ENTER exit"
                  : "UP/DN page  ENTER edit  hold ENTER exit",
        { emphasis: "muted" }),
      scrollbar()
    ],
    flows: [
      ...ringFlows(CONFIG_RING, i),
      btn("f-enter", isDescent ? "Open sensor list" : "Edit value", "enter", "short", A.descend, target),
      btn("f-escape", "Exit to main screen", "enter", "long", A.escape, "info-p0-global-status")
    ]
  });
});

// BACK page for the config root
screens.push({
  id: "config-root-back",
  name: "C.BACK — Back",
  description: "Ascends one level, out of Configuration.",
  elements: [
    text("hdr-title", L.headerY, "Config"),
    text("back-label", L.valueY, "< BACK", { emphasis: "strong" }),
    text("footer-hint", L.footerY, "ENTER go back", { emphasis: "muted" }),
    scrollbar()
  ],
  flows: [
    ...ringFlows(CONFIG_RING, CONFIG_RING.length - 1),
    btn("f-back", "Back one level", "enter", "short", A.back),
    btn("f-escape", "Exit to main screen", "enter", "long", A.escape, "info-p0-global-status")
  ]
});

// ── L2 Value editors for C1..C6 ─────────────────────────────────────────────
function editorScreen({ page, screenId, title, binding, parentId }) {
  const v = byId.get(binding);
  if (!v) throw new Error(`catalogue has no value "${binding}"`);
  const unit = v.unit ? ` ${v.unit}` : "";
  // The unit belongs on enum settings too: "1 / 10 / 100 L" is meaningful,
  // "1 / 10 / 100" is not (config.ledPulseVolume is litres).
  // Manifest format 3 replaced the bare `enum` label list with `options`, which carry the
  // value actually written to the register. Reading the old field silently produced a
  // min/max hint instead — "0 to 7" where the user needed to see the baud rates.
  const range = v.options
    ? `${v.options.map((o) => o.label).join(" / ")}${unit}`
    : (v.min !== undefined && v.max !== undefined ? `${v.min} to ${v.max}${unit}` : "");
  return {
    id: screenId,
    name: `${page}.V — Edit ${title}`,
    description: `Value editor for ${page}. ENTER commits and ascends; hold ENTER discards.`,
    elements: [
      text("hdr-title", L.headerY, `Edit > ${title}`),
      ...(range ? [text("range-hint", L.bodyY - 14, range, { emphasis: "muted" })] : []),
      text("pending-label", L.bodyY, "New value", { emphasis: "muted" }),
      // The pending value binds config.editor.pending, not the setting id: both the
      // pending and the saved element would otherwise bind the same id and the
      // resolver could not tell them apart. saved-value keeps the setting id, which is
      // also how descending discovers which setting an editor edits.
      value("pending-value", L.valueY, "config.editor.pending", { emphasis: "strong" }),
      text("saved-label", L.savedLabelY, "Saved", { emphasis: "muted" }),
      value("saved-value", L.savedValueY, binding, { emphasis: "muted" }),
      text("footer-hint", L.footerY, "UP/DN adjust  ENTER save  hold ENTER discard", { emphasis: "muted" })
    ],
    flows: [
      btn("f-inc", "Increase", "up", "short", A.inc),
      btn("f-dec", "Decrease", "down", "short", A.dec),
      btn("f-commit", "Save and go back", "enter", "short", A.commit, parentId),
      btn("f-discard", "Discard and go back", "enter", "long", A.discard, parentId)
    ]
  };
}

DEVICE.filter((d) => d.binding).forEach((d) => {
  screens.push(editorScreen({
    page: d.page, screenId: editorId(d.id), title: d.title,
    binding: d.binding, parentId: d.id
  }));
});

// ── L2 Sensor list: Sensor 1..8 + BACK ──────────────────────────────────────
const SENSOR_IDS = [...Array.from({ length: 8 }, (_, i) => `config-sensor-${i + 1}`), "config-sensor-back"];

for (let n = 1; n <= 8; n += 1) {
  screens.push({
    id: `config-sensor-${n}`,
    name: `SEN${n} — Sensor ${n}`,
    description: `Sensor list entry. ENTER descends into sensor ${n}'s settings; the level carries the sensor index, so no selectedSensor state exists.`,
    elements: [
      text("hdr-title", L.headerY, "Config > Sensors"),
      text("field-label", L.bodyY, "Sensor", { emphasis: "muted" }),
      text("field-value", L.valueY, `${n} >`, { emphasis: "strong" }),
      value("status-value", L.savedValueY, `sensor.${n}.status`, { emphasis: "muted" }),
      text("footer-hint", L.footerY, "UP/DN sensor  ENTER open  hold ENTER exit", { emphasis: "muted" }),
      scrollbar()
    ],
    flows: [
      ...ringFlows(SENSOR_IDS, n - 1),
      btn("f-enter", `Open sensor ${n} settings`, "enter", "short", A.descend, "config-s1-connected"),
      btn("f-escape", "Exit to main screen", "enter", "long", A.escape, "info-p0-global-status")
    ]
  });
}
screens.push({
  id: "config-sensor-back",
  name: "SEN.BACK — Back",
  description: "Ascends from the sensor list to the config root.",
  elements: [
    text("hdr-title", L.headerY, "Config > Sensors"),
    text("back-label", L.valueY, "< BACK", { emphasis: "strong" }),
    text("footer-hint", L.footerY, "ENTER go back", { emphasis: "muted" }),
    scrollbar()
  ],
  flows: [
    ...ringFlows(SENSOR_IDS, SENSOR_IDS.length - 1),
    btn("f-back", "Back one level", "enter", "short", A.back),
    btn("f-escape", "Exit to main screen", "enter", "long", A.escape, "info-p0-global-status")
  ]
});

// ── L3 Sensor settings: S1..S4 + BACK, and L4 their editors ─────────────────
const SENSOR_SETTINGS = [
  { page: "S1", id: "config-s1-connected", title: "Connected", binding: "config.sensor.connected" },
  { page: "S2", id: "config-s2-multiplier", title: "Multiplier (F)", binding: "config.sensor.multiplier" },
  { page: "S3", id: "config-s3-adjust", title: "Adjust", binding: "config.sensor.adjust" },
  { page: "S4", id: "config-s4-max-flow", title: "Max Flow (Q)", binding: "config.sensor.maxFlow" }
];
const S_RING = [...SENSOR_SETTINGS.map((s) => s.id), "config-sensor-settings-back"];

SENSOR_SETTINGS.forEach((s, i) => {
  const v = byId.get(s.binding);
  const unit = v?.unit ? ` ${v.unit}` : "";
  screens.push({
    id: s.id,
    name: `${s.page} — ${s.title}`,
    description: `Sensor settings entry ${s.page}, scoped to the sensor of the current level.`,
    elements: [
      text("hdr-title", L.headerY, `Sensor > ${s.title}`),
      value("sensor-index", L.headerY, "config.selectedSensor", { emphasis: "muted", x: 200 }),
      text("field-label", L.bodyY, `Current${unit}`, { emphasis: "muted" }),
      value("field-value", L.valueY, s.binding, { emphasis: "strong" }),
      value("nyquist-warning", L.savedValueY, "config.sensor.nyquistWarning", { emphasis: "muted" }),
      text("footer-hint", L.footerY, "UP/DN page  ENTER edit  hold ENTER exit", { emphasis: "muted" }),
      scrollbar()
    ],
    flows: [
      ...ringFlows(S_RING, i),
      btn("f-enter", "Edit value", "enter", "short", A.descend, editorId(s.id)),
      btn("f-escape", "Exit to main screen", "enter", "long", A.escape, "info-p0-global-status")
    ]
  });
  screens.push(editorScreen({
    page: s.page, screenId: editorId(s.id), title: s.title,
    binding: s.binding, parentId: s.id
  }));
});
screens.push({
  id: "config-sensor-settings-back",
  name: "S.BACK — Back",
  description: "Ascends from sensor settings to the sensor list.",
  elements: [
    text("hdr-title", L.headerY, "Sensor"),
    text("back-label", L.valueY, "< BACK", { emphasis: "strong" }),
    text("footer-hint", L.footerY, "ENTER go back", { emphasis: "muted" }),
    scrollbar()
  ],
  flows: [
    ...ringFlows(S_RING, S_RING.length - 1),
    btn("f-back", "Back one level", "enter", "short", A.back),
    btn("f-escape", "Exit to main screen", "enter", "long", A.escape, "info-p0-global-status")
  ]
});

// ── L0/L1/L2 WiFi and MQTT (WiFi_MQTT_Connectivity.md §7.1) ─────────────────
//
// Two new root-level entries, siblings of P0..P8, each descending into its own level. Ordinary
// dataset screens, not firmware-drawn: §7.1 is explicit that a menu pack must be able to restyle
// or relocate them, unlike the Select Menu page which is firmware-drawn because it is the recovery
// route.
//
// The INFORMATION pages of §7.1 (AP info, WiFi info, MQTT info) are deliberately NOT here. They
// display runtime state — association status, the DHCP address, the AP password — and none of those
// derived values exist in the catalogue until N4 builds the state machine that produces them.
// Emitting the pages now would mean binding ids the resolver cannot serve, which the
// firmware-manifest-resolvable gate would reject, or worse, placeholder text pretending to be
// status. They arrive with the state they describe.
//
// Nothing here is guarded either. R7.3 permits a guard to hide an information page and forbids it
// from hiding an editor, because the completeness rule has to be decidable statically and no static
// check can evaluate a runtime guard. Since this slice emits only editors, the honest thing is to
// emit them unconditionally: every one of the fourteen settings is reachable by paging, with no
// guard for the export gate to reason around. Configuring a broker before switching MQTT on is also
// the friendlier order.
const WIFI_L1 = [
  { page: "W1", id: "net-wifi-enabled", title: "Enabled", binding: "config.wifi.enabled" },
  { page: "W2", id: "net-wifi-ssid", title: "Network (SSID)", binding: "config.wifi.ssid" },  // read-only
  { page: "W3", id: "net-wifi-psk", title: "Passphrase", binding: "config.wifi.psk" },
  // R8.2a. Lives in the WiFi level because the portal is reached through the AP the radio raises,
  // and because §6.3 left the panel with no other way to influence the portal at all.
  { page: "W4", id: "net-wifi-portal-reset", title: "Reset portal login", binding: null,
    descendTo: "confirm-reset-portal-login" }
];
const WIFI_RING = [...WIFI_L1.map((w) => w.id), "net-wifi-back"];

const MQTT_L1 = [
  { page: "M1", id: "net-mqtt-enabled", title: "Enabled", binding: "config.mqtt.enabled" },
  // "Broker", not "Broker setup". Since §6.3 removed on-device text entry, five of the ten rows in
  // that level are read-only displays — a name promising setup would be half a lie, and the first
  // row the descent lands on (the broker host) is one of the read-only ones. The rows themselves
  // say where to change them.
  { page: "M2", id: "net-mqtt-setup", title: "Broker", binding: null, descendTo: "net-mqtt-host" }
];
const MQTT_RING = [...MQTT_L1.map((m) => m.id), "net-mqtt-back"];

const MQTT_SETUP = [
  { page: "B1", id: "net-mqtt-host", title: "Broker host", binding: "config.mqtt.host" },
  { page: "B2", id: "net-mqtt-port", title: "Port", binding: "config.mqtt.port" },
  { page: "B3", id: "net-mqtt-user", title: "Username", binding: "config.mqtt.user" },
  { page: "B4", id: "net-mqtt-password", title: "Password", binding: "config.mqtt.password" },
  { page: "B5", id: "net-mqtt-base-topic", title: "Base topic", binding: "config.mqtt.baseTopic" },
  { page: "B6", id: "net-mqtt-prefix", title: "HA prefix", binding: "config.mqtt.discoveryPrefix" },
  { page: "B7", id: "net-mqtt-period", title: "Publish period", binding: "config.mqtt.publishPeriod" },
  { page: "B8", id: "net-mqtt-ha-discovery", title: "HA discovery", binding: "config.mqtt.haDiscovery" },
  { page: "B9", id: "net-mqtt-tls", title: "TLS", binding: "config.mqtt.tls" },
  { page: "B10", id: "net-mqtt-qos", title: "QoS", binding: "config.mqtt.qos" }
];
const MQTT_SETUP_RING = [...MQTT_SETUP.map((b) => b.id), "net-mqtt-setup-back"];

/**
 * Emits one navigation level: an entry screen per item, an editor for each that binds a setting,
 * and the closing BACK page.
 *
 * Factored rather than copied because there are now five levels with identical structure, and the
 * ring wiring is exactly the part that is silently wrong when hand-repeated.
 */
function emitLevel({ items, ring, crumb, backId, backName }) {
  items.forEach((item, i) => {
    const isDescent = item.binding === null;
    const v = item.binding ? byId.get(item.binding) : null;
    if (item.binding && !v) throw new Error(`catalogue has no value "${item.binding}"`);
    const unit = v?.unit ? ` ${v.unit}` : "";
    // A text value is DISPLAYED, never edited here — see assertCoversEverySetting. The row still
    // exists because an operator standing at the device needs to see which network and which broker
    // it is configured for; that is the diagnostic half of §7.1's information pages, at zero input
    // cost. Secrets render as "********" via formatSettingText, so the row is safe on a wall panel.
    const isReadOnly = v?.type === "string";
    screens.push({
      id: item.id,
      name: `${item.page} — ${item.title}`,
      description: `${crumb} entry ${item.page}. ENTER ${isDescent ? "descends" : "edits"}; UP/DOWN move within the level.`,
      elements: [
        text("hdr-title", L.headerY, `${crumb} > ${item.title}`),
        text("field-label", L.bodyY, isDescent ? "Open" : `Current${unit}`, { emphasis: "muted" }),
        ...(item.binding
          ? [value("field-value", L.valueY, item.binding, { emphasis: "strong" })]
          : [text("field-value", L.valueY, "Settings >", { emphasis: "strong" })]),
        text("footer-hint", L.footerY,
          isDescent ? "UP/DN page  ENTER open  hold ENTER exit"
                    : isReadOnly ? "Set via web portal or RS485"
                                 : "UP/DN page  ENTER edit  hold ENTER exit",
          { emphasis: "muted" }),
        scrollbar()
      ],
      flows: [
        ...ringFlows(ring, i),
        // No ENTER-short on a read-only row: there is nothing below it to descend to, and a flow
        // pointing at a nonexistent editor is what the export gate would reject.
        ...(isReadOnly ? [] : [
          btn("f-enter", isDescent ? `Open ${item.title}` : "Edit value", "enter", "short",
              A.descend, isDescent ? item.descendTo : editorId(item.id))
        ]),
        btn("f-escape", "Exit to main screen", "enter", "long", A.escape, "info-p0-global-status")
      ]
    });
    if (item.binding && !isReadOnly) {
      screens.push(editorScreen({
        page: item.page, screenId: editorId(item.id), title: item.title,
        binding: item.binding, parentId: item.id
      }));
    }
  });
  screens.push({
    id: backId,
    name: backName,
    description: `Ascends one level, out of ${crumb}.`,
    elements: [
      text("hdr-title", L.headerY, crumb),
      text("back-label", L.valueY, "< BACK", { emphasis: "strong" }),
      text("footer-hint", L.footerY, "ENTER go back", { emphasis: "muted" }),
      scrollbar()
    ],
    flows: [
      ...ringFlows(ring, ring.length - 1),
      btn("f-back", "Back one level", "enter", "short", A.back),
      btn("f-escape", "Exit to main screen", "enter", "long", A.escape, "info-p0-global-status")
    ]
  });
}

// The two root-level entries. Their ring is the INFO ring, so they page with P0..P8.
[
  { id: "net-wifi-root", name: "WIFI — WiFi", crumb: "WiFi", enter: "net-wifi-enabled",
    prev: "info-p8-factory-reset", next: "net-mqtt-root",
    lines: ["Radio, network name and", "passphrase."] },
  { id: "net-mqtt-root", name: "MQTT — MQTT", crumb: "MQTT", enter: "net-mqtt-enabled",
    prev: "net-wifi-root", next: "info-p0-global-status",
    lines: ["Broker, credentials and", "Home Assistant discovery."] }
].forEach((r) => {
  screens.push({
    id: r.id,
    name: r.name,
    description: `Root-level ${r.crumb} entry. ENTER descends into the ${r.crumb} level.`,
    elements: [
      text("hdr-title", L.headerY, r.crumb.toUpperCase()),
      text("line-1", L.bodyY, r.lines[0], { emphasis: "muted" }),
      text("line-2", L.bodyY + 12, r.lines[1], { emphasis: "muted" }),
      text("prompt", L.valueY + 14, "ENTER to open >", { emphasis: "strong" }),
      text("footer-hint", L.footerY, "UP/DN page  ENTER open", { emphasis: "muted" }),
      scrollbar()
    ],
    flows: [
      btn("f-next", "Next page", "down", "short", A.next, r.next),
      btn("f-prev", "Previous page", "up", "short", A.prev, r.prev),
      btn("f-enter", `Open ${r.crumb} settings`, "enter", "short", A.descend, r.enter),
      btn("f-escape", "Back to main screen", "enter", "long", A.escape, "info-p0-global-status")
    ]
  });
});

// Checked here rather than earlier: it must see EVERY list, and the network lists are declared
// above this point. Placed before the emit calls so a missing editor fails before any screen is
// written rather than after.
assertCoversEverySetting([...DEVICE, ...WIFI_L1, ...MQTT_L1, ...MQTT_SETUP], SENSOR_SETTINGS);

emitLevel({ items: WIFI_L1, ring: WIFI_RING, crumb: "WiFi",
            backId: "net-wifi-back", backName: "W.BACK — Back" });
emitLevel({ items: MQTT_L1, ring: MQTT_RING, crumb: "MQTT",
            backId: "net-mqtt-back", backName: "M.BACK — Back" });
emitLevel({ items: MQTT_SETUP, ring: MQTT_SETUP_RING, crumb: "MQTT Broker",
            backId: "net-mqtt-setup-back", backName: "B.BACK — Back" });

// ── P8 Factory Reset info page ───────────────────────────────────────────────
screens.push({
  id: "info-p8-factory-reset",
  name: "P8 — Factory Reset",
  description: "Info-level entry point for a full factory reset. ENTER opens the confirm screen.",
  elements: [
    text("hdr-title", L.headerY, "P8 FACTORY RESET"),
    text("warning-1", L.bodyY, "Erases all totals, sensor", { emphasis: "muted" }),
    text("warning-2", L.bodyY + 12, "config and LED settings.", { emphasis: "muted" }),
    text("prompt", L.valueY + 14, "ENTER to continue >", { emphasis: "strong" }),
    text("footer-hint", L.footerY, "UP/DN page  ENTER confirm screen", { emphasis: "muted" }),
    scrollbar()
  ],
  flows: [
    btn("f-next", "Next page", "down", "short", A.next, "net-wifi-root"),
    btn("f-prev", "Previous page", "up", "short", A.prev, "info-p7-enter-config"),
    btn("f-enter", "Open confirm screen", "enter", "short", A.descend, "confirm-factory-reset"),
    btn("f-escape", "Back to main screen", "enter", "long", A.escape, "info-p0-global-status")
  ]
});

// ── L1 Confirm screens + acknowledgement toasts ─────────────────────────────
const CONFIRMS = [
  {
    id: "confirm-reset-totals", name: "Reset totals?", title: "RESET TOTALS?",
    warn: "Persistent cumulative volume", warn2: "cannot be recovered.",
    holdMs: 3000, action: "core.action.reset-all-measured", toast: "toast-totals-reset"
  },
  {
    id: "confirm-reset-session", name: "Reset session?", title: "RESET SESSION?",
    warn: "Session totals and max flow", warn2: "return to zero.",
    holdMs: 1500, action: "core.action.reset-session", toast: "toast-session-reset"
  },
  {
    id: "confirm-factory-reset", name: "Factory reset?", title: "FACTORY RESET?",
    warn: "Wipes NVS and reboots.", warn2: "This cannot be undone.",
    holdMs: 30000, action: "core.action.factory-reset", toast: null
  },
  {
    // R8.2a. A 3 s hold, matching "reset totals" rather than the factory reset's 30 s: this is
    // recoverable and destroys nothing, but it does drop the portal to a published default, so it
    // is not a single press either.
    id: "confirm-reset-portal-login", name: "Reset portal login?", title: "RESET PORTAL LOGIN?",
    warn: "Restores admin/admin. Totals,", warn2: "config and calibration kept.",
    holdMs: 3000, action: "core.action.reset-portal-login", toast: "toast-portal-login-reset"
  }
];

for (const c of CONFIRMS) {
  screens.push({
    id: c.id,
    name: c.name,
    description: `Confirm screen. Inverted gestures per §3.2: ENTER-short exits, ENTER held ${c.holdMs} ms confirms.`,
    elements: [
      overlay(),
      text("title", L.bodyY, c.title, { emphasis: "strong" }),
      text("warning-1", L.bodyY + 18, c.warn, { emphasis: "muted" }),
      text("warning-2", L.bodyY + 30, c.warn2, { emphasis: "muted" }),
      { id: "timer-value", kind: "value", x: 104, y: L.valueY + 34, binding: "countdown.value", emphasis: "strong" },
      text("footer-hint", L.footerY, "ENTER exit  hold ENTER confirm", { emphasis: "muted" })
    ],
    flows: [
      btn("f-exit", "Exit without acting", "enter", "short", A.back),
      {
        id: "f-confirm",
        label: c.name.replace("?", ""),
        trigger: { type: "timeout", durationMs: c.holdMs, holdButton: "enter" },
        actionId: c.action,
        ...(c.toast ? { targetScreenId: c.toast } : {})
      }
    ]
  });
}

const TOASTS = [
  { id: "toast-totals-reset", name: "Totals reset", message: "TOTALS RESET" },
  { id: "toast-session-reset", name: "Session reset", message: "SESSION RESET" },
  // R8.2a. Names the credential explicitly rather than saying "done": the operator now has to go
  // and use admin/admin, and a toast that does not say so leaves them guessing what changed.
  { id: "toast-portal-login-reset", name: "Portal login reset", message: "LOGIN: admin/admin" }
];
for (const t of TOASTS) {
  screens.push({
    id: t.id,
    name: t.name,
    description: "Acknowledgement toast. Auto-dismisses after 2 s with no button held (§3.8 auto timeout).",
    elements: [
      overlay(),
      text("message", 58, t.message, { emphasis: "strong" }),
      text("sub", 74, "Returning...", { emphasis: "muted" })
    ],
    flows: [
      {
        id: "f-dismiss",
        label: "Dismiss",
        // No holdButton: absent means auto timeout (§3.8). Emitting an explicit
        // null fails the schema enum, and absence is the clearer encoding anyway.
        trigger: { type: "timeout", durationMs: 2000 },
        actionId: A.back
      }
    ]
  });
}

// ── Merge: replace generated ids, keep hand-tuned info pages ─────────────────
const dataset = JSON.parse(fs.readFileSync(SCREENS, "utf8"));
const generatedIds = new Set(screens.map((s) => s.id));
const RETIRED = new Set([
  // Replaced by confirm screens; enter-config, sensor-save and config-exit are no
  // longer guarded actions at all under the new model.
  "countdown-enter-config", "countdown-reset-session", "countdown-reset-all",
  "countdown-factory-reset", "countdown-sensor-save", "countdown-config-exit",
  // The seven text-setting editors, retired with on-device text entry itself (§6.3). Listed
  // explicitly because `kept` retains anything the generator no longer emits — without this they
  // would survive as orphans: editor screens with a commit flow, bound to config.editor.pending,
  // that no longer have a parent descending into them and no engine behind them.
  "net-wifi-ssid-edit", "net-wifi-psk-edit", "net-mqtt-host-edit", "net-mqtt-user-edit",
  "net-mqtt-password-edit", "net-mqtt-base-topic-edit", "net-mqtt-prefix-edit"
]);

const kept = dataset.screens.filter((s) => !generatedIds.has(s.id) && !RETIRED.has(s.id));

// Bring the retained hand-tuned screens into landscape bounds and give the info
// pages a level indicator. Layouts are otherwise untouched.
// Footer rows, bottom-up. Clamping each stray element independently piled several
// onto one row (legend-led at y=133 and footer-hint at y=226 both landed on 124),
// which the geometry audit caught as fully-superimposed text. Elements below the
// fold are instead assigned successive rows in their original y order.
const FOOTER_ROWS = [L.footerY, L.footerY - 12, L.footerY - 24, L.footerY - 36];
const TEXT_H = { text: 8, value: 10, badge: 12, icon: 10, box: 0, scrollbar: 0 };
const implicitH = (e) => e.height ?? TEXT_H[e.kind] ?? 8;

let moved = 0, resized = 0, barsAdded = 0, badgesMoved = 0, asciiFolded = 0, dividersWidened = 0, legendMoved = 0, textCorrected = 0, descriptionsFixed = 0;
for (const s of kept) {
  for (const e of s.elements) {
    if (e.kind === "box" && e.width === 135 && e.height === 240) { e.width = W; e.height = H; resized += 1; }
  }

  // Everything in the footer zone is re-stacked deterministically, not just what
  // currently overflows. Detecting only overflow is not idempotent: a previous run
  // had already pulled two elements up onto the same row, so on the next run
  // neither overflowed and the collision was invisible.
  const FOOTER_ZONE_TOP = FOOTER_ROWS[FOOTER_ROWS.length - 1];
  const strays = s.elements
    .filter((e) => e.kind !== "box" && e.kind !== "scrollbar"
                   && (e.y >= FOOTER_ZONE_TOP || e.y + implicitH(e) > H - 2))
    .sort((a2, b2) => a2.y - b2.y);
  if (strays.length > FOOTER_ROWS.length) {
    throw new Error(`${s.id}: ${strays.length} footer-zone elements, only ${FOOTER_ROWS.length} rows`);
  }
  // Bottom-most original y takes the bottom row, so relative order is preserved.
  strays.reverse().forEach((e, i) => { e.y = FOOTER_ROWS[i]; moved += 1; });

  // A badge is 12px tall against a text header's 8px, so a badge sharing the
  // header's x and y buries the title under its opaque background. Move badges
  // that collide with the header to the right-hand end of the same row.
  const hdr = s.elements.find((e) => e.id === "hdr-title");
  if (hdr) {
    for (const e of s.elements) {
      if (e === hdr || e.kind !== "badge") continue;
      const overlapsY = e.y < hdr.y + implicitH(hdr) && hdr.y < e.y + implicitH(e);
      if (overlapsY && Math.abs(e.x - hdr.x) < 60) { e.x = 176; badgesMoved += 1; }
    }
  }
  if (s.id.startsWith("info-p") && !s.elements.some((e) => e.kind === "scrollbar")) {
    s.elements.push(scrollbar()); barsAdded += 1;
  }
  // Info pages: ENTER-short now descends into a confirm screen or Configuration.
  const descend = {
    "info-p2-cumulative-liters": "confirm-reset-totals",
    "info-p3-cumulative-m3": "confirm-reset-totals",
    "info-p4-session-liters": "confirm-reset-session",
    "info-p5-session-m3": "confirm-reset-session",
    "info-p6-max-flow": "confirm-reset-session",
    "info-p7-enter-config": "config-c1-modbus-id"
  }[s.id];
  // Every info page gets its ENTER flows rebuilt, not just those with a descent
  // target. P0 and P1 previously kept 0.1-era `ui.action.mode.idle -> state-idle`
  // flows: display-off moved to UP+DOWN (firmware-native, §3.1) and ENTER-long is
  // now unambiguously escape (§3.2), so those were the only two screens
  // contradicting the gesture contract.
  if (s.id.startsWith("info-p")) {
    s.flows = (s.flows ?? []).filter((f) => f.trigger.type !== "button" || f.trigger.button !== "enter");
    if (descend) s.flows.push(btn("f-enter", "Open", "enter", "short", A.descend, descend));
    s.flows.push(btn("f-escape", "Back to main screen", "enter", "long", A.escape, "info-p0-global-status"));
  }

  // Font0 (M5GFX GLCDfont) covers codepoints 0-255 only, and for c >= 176 with
  // cp437 disabled it increments before lookup — so a codepoint above 255 draws a
  // blank 6px cell and U+00B3 prints the *wrong* glyph. Every rendered string must
  // therefore be ASCII. The warning triangle and the legend bullets were invisible
  // on the device; "m³" printed a wrong character.
  const ASCII_FOLD = [
    [/\u2022/g, "-"],   // bullet
    [/\u26A0/g, "!"],   // warning sign
    [/\u2014/g, "-"],   // em dash
    [/\u2013/g, "-"],   // en dash
    [/\u00B3/g, "3"],   // superscript three
    [/\u00B2/g, "2"],
    [/\u2191/g, "UP"],
    [/\u2193/g, "DN"],
    [/\u25C0/g, "<"],
    [/\u25B6/g, ">"],
    [/\u2192/g, "->"]
  ];
  for (const e of s.elements) {
    if (typeof e.content !== "string") continue;
    let next = e.content;
    for (const [re, rep] of ASCII_FOLD) next = next.replace(re, rep);
    // Anything still non-ASCII would render blank; drop it rather than ship a gap.
    next = next.replace(/[^\x20-\x7E]/g, "");
    if (next !== e.content) { e.content = next.replace(/\s{2,}/g, "  ").trim(); asciiFolded += 1; }
  }

  // Dividers were authored at the portrait DISPLAY_WIDTH of 135 on a 240px panel.
  for (const e of s.elements) {
    if (e.kind === "box" && e.height === 1 && e.width === 135) { e.width = W; dividersWidened += 1; }
  }

  // Footer text that still advertises retired durations.
  const footerText = {
    "info-p0-global-status": "UP/DN pages   UP+DN display off",
    "info-p1-instant-flow": "UP/DN pages   UP+DN display off",
    "info-p2-cumulative-liters": "ENTER reset totals (hold 3s)",
    "info-p3-cumulative-m3": "ENTER reset totals (hold 3s)",
    "info-p4-session-liters": "ENTER reset session",
    "info-p5-session-m3": "ENTER reset session",
    "info-p6-max-flow": "ENTER reset session",
    "info-p7-enter-config": "ENTER opens Configuration"
  }[s.id];
  if (footerText) {
    const fh = s.elements.find((e) => e.id === "footer-hint");
    if (fh) fh.content = footerText;
  }

  // §4.3 note 5 and §6 note 8: the LED legend belongs on P0, the landing page. It
  // was on P1-P7 and absent from P0 — the exact inverse.
  if (s.id.startsWith("info-p")) {
    const legend = s.elements.find((e) => e.id === "legend-led");
    if (s.id === "info-p0-global-status") {
      if (!legend) {
        s.elements.push(text("legend-led", L.footerY - 12, "LED: Red=Pulse Grn=Ready Blu=Flow",
                             { emphasis: "muted", binding: "legend.led" }));
        legendMoved += 1;
      }
    } else if (legend) {
      s.elements.splice(s.elements.indexOf(legend), 1);
      legendMoved += 1;
    }
  }

  // P7's on-screen text told the operator to *hold* ENTER for Config, but holding
  // now escapes to P0 and a short press descends.
  // The prompt is split across two elements, so match them by id rather than by
  // looking for both words in one string.
  if (s.id === "info-p7-enter-config") {
    const prompt = { "prompt-line1": "Press ENTER to open", "prompt-line2": "Configuration." };
    for (const e of s.elements) {
      if (prompt[e.id] && e.content !== prompt[e.id]) { e.content = prompt[e.id]; textCorrected += 1; }
    }
  }

  // Screen descriptions still advertised retired countdowns and a dropped
  // propeller animation. They are metadata, but they are what the next person reads.
  const desc = {
    "info-p0-global-status": "P0 landing page: aggregate flow and volume, plus the flow indicator and LED legend.",
    "info-p1-instant-flow": "P1: instantaneous flow for all eight sensors in two columns.",
    "info-p2-cumulative-liters": "P2: cumulative litres. ENTER opens the reset-totals confirm screen (3 s hold).",
    "info-p3-cumulative-m3": "P3: cumulative cubic metres. ENTER opens the reset-totals confirm screen (3 s hold).",
    "info-p4-session-liters": "P4: session litres. ENTER opens the reset-session confirm screen.",
    "info-p5-session-m3": "P5: session cubic metres. ENTER opens the reset-session confirm screen.",
    "info-p6-max-flow": "P6: peak flow since the last session reset. ENTER opens the reset-session confirm screen.",
    "info-p7-enter-config": "P7: entry point to Configuration. ENTER descends to the config root."
  }[s.id];
  if (desc) { s.description = desc; descriptionsFixed += 1; }

  // §5.5: UP returns to the editor and DOWN force-saves, both ascending via the
  // navigation stack. Static targets cannot express "the editor we came from",
  // because which sensor setting failed is runtime state.
  if (s.id === "nyquist-warning") {
    for (const f of s.flows ?? []) {
      if (f.trigger.type === "button" && (f.trigger.button === "up" || f.trigger.button === "down")) {
        delete f.targetScreenId;
        if (f.trigger.button === "up") f.actionId = A.back;
      }
    }
  }
  // P7 now wraps to P8 rather than back to P0.
  if (s.id === "info-p7-enter-config") {
    for (const f of s.flows) {
      if (f.id === "f-next") f.targetScreenId = "info-p8-factory-reset";
    }
  }
  if (s.id === "info-p0-global-status") {
    for (const f of s.flows) {
      // The root ring now ends with the two network entries, so UP from P0 reaches MQTT, not P8.
      if (f.id === "f-prev") f.targetScreenId = "net-mqtt-root";
    }
  }
}

const merged = { ...dataset, screens: [...kept, ...screens] };

const report = {
  kept: kept.length,
  generated: screens.length,
  retired: [...RETIRED].filter((id) => dataset.screens.some((s) => s.id === id)).length,
  total: merged.screens.length,
  footersMoved: moved,
  overlaysResized: resized,
  scrollbarsAdded: barsAdded,
  badgesMoved,
  asciiFolded,
  dividersWidened,
  legendMoved,
  textCorrected,
  descriptionsFixed,
  newActions: [...new Set(screens.flatMap((s) => (s.flows ?? []).map((f) => f.actionId)).filter(Boolean))].sort()
};

if (process.argv.includes("--write")) {
  fs.writeFileSync(SCREENS, `${JSON.stringify(merged, null, 2)}\n`);
  console.log("written:", SCREENS);
}
console.log(JSON.stringify(report, null, 2));
