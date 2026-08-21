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
// §3.0.1's required set and N-b's fail/warn split, shared with the exporter so the exemptions have
// exactly one home.
import { classifyCoverage } from "../../../../tools/catalogue/coverage.mjs";
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
  // 100, not 104 (DF19). §2c requires every `level-position` scrollbar to stop clear of the warning
  // band at y=116: 14 + 104 = 118, two pixels inside it. The six screens that had no spec file to
  // override this default shipped at 104 while the other 55 came out at 100 from their spec files —
  // one fact with two homes, and the wrong home was the live one.
  scrollbar: { x: 232, y: 14, width: 5, height: 100 }
};

/**
 * The agreed layout for a screen, from the requirement rather than from here.
 *
 * `docs/Requirements/feature addition/screens/<id>.json` is where each screen's geometry is decided and
 * reviewed (Display_Per_Screen_Spec.md). This generator owns NAVIGATION — rings, descents, escapes — which
 * those files deliberately do not carry, and defers every coordinate to them. Before this, the layout table
 * `L` below was the only source, so the requirement and the dataset were two homes for one fact and the
 * dataset was the one that shipped.
 *
 * `worst`, `bound` and `bannerReplaces` are spec metadata — the declared worst-case string, its
 * justification, and the banner declaration — and are stripped: the dataset schema does not know them.
 */
const SPEC_DIR = path.join(ROOT, "..", "..", "docs", "Requirements", "feature addition", "screens");
// The ledger is the only record of WHEN a catalogue id appeared; the manifest describes the catalogue
// as it is now and cannot answer a question about history.
const ledger = JSON.parse(
  fs.readFileSync(path.join(ROOT, "..", "..", "tools", "catalogue", "ledger.json"), "utf8")
);

const specs = new Map();
if (fs.existsSync(SPEC_DIR)) {
  for (const file of fs.readdirSync(SPEC_DIR).filter((f) => f.endsWith(".json"))) {
    const spec = JSON.parse(fs.readFileSync(path.join(SPEC_DIR, file), "utf8"));
    specs.set(spec.id, spec);
  }
}
const SPEC_ONLY_KEYS = ["worst", "bound", "bannerReplaces"];
function specElements(id) {
  const spec = specs.get(id);
  if (!spec) return null;
  return spec.elements.map((element) => {
    const copy = { ...element };
    for (const key of SPEC_ONLY_KEYS) delete copy[key];
    return copy;
  });
}

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

/**
 * The ring lost two entries and the rest moved up.
 *
 * Cumulative litres and session litres were absorbed by their m3 pages: 1 L is exactly 0.001 m3, so one page
 * carries both readings and the device was paginating one fact twice (Display_Per_Screen_Spec.md 5.1). Every
 * flow that referenced a dropped or renamed page is rewritten below, so the ring closes at nine entries.
 */
const RING_RENAMES = {
  "info-p3-cumulative-m3": "info-p2-cumulative-m3",
  "info-p5-session-m3": "info-p3-session-m3",
  "info-p6-max-flow": "info-p4-max-flow",
  "info-p7-enter-config": "info-p5-enter-config",
  "info-p8-factory-reset": "info-p6-factory-reset"
};
const RING_DROPPED = new Set(["info-p2-cumulative-liters", "info-p4-session-liters"]);
/** The nine entries, in order. UP/DOWN are rebuilt from THIS rather than patched target by target. */
const INFO_RING = [
  "info-p0-global-status",
  "info-p1-instant-flow",
  "info-p2-cumulative-m3",
  "info-p3-session-m3",
  "info-p4-max-flow",
  "info-p5-enter-config",
  "info-p6-factory-reset",
  "net-wifi-root",
  "net-mqtt-root"
];
const RING_NAMES = {
  "info-p2-cumulative-m3": "P2 — Cumulative Volume",
  "info-p3-session-m3": "P3 — Session Volume",
  "info-p4-max-flow": "P4 — Max Flow Since Reset",
  "info-p5-enter-config": "P5 — Enter Configuration",
  "info-p6-factory-reset": "P6 — Factory Reset"
};

const screens = [];

// ── L1 Config root: three GROUPS + BACK ─────────────────────────────────────
/**
 * Configuration is three groups, not a flat list of everything.
 *
 * It used to be C1..C8: four Modbus settings, two LED settings, the flow unit and a Sensors descent,
 * all side by side at one level. Eight entries with nothing to say which belonged together, and a
 * root that mixed leaf settings with a descent — so the operator paged past parity to reach the LED
 * brightness. The four link settings are one coherent thing, and grouping them is what the WiFi and
 * MQTT levels already do.
 *
 * The three orphans went to DISPLAY rather than staying at the root: two LED indicator settings and
 * the panel's flow unit are all about how the device PRESENTS itself, as opposed to how it talks
 * (Modbus) or what it measures (Sensors). Every leaf setting now sits one level below a name that
 * describes it.
 *
 * DEPTH is unchanged at 4, which matters because the navigator caps it. Modbus and Display gain a
 * level, but the deepest chain was always the sensor one — root, Sensors, channel, settings, editor —
 * and Sensors was already a descent at the root, so it did not move.
 *
 * Ids name their GROUP (`config-modbus-baud-rate`, not `config-c2-baud-rate`). The old scheme
 * encoded a position, so inserting the flow unit renumbered Sensors from C7 to C8 and every reference
 * to the old id became quietly wrong. A group name survives a reorder.
 */
const CONFIG_GROUPS = [
  { page: "CFG.M", id: "config-modbus", title: "Modbus", body: "Address, baud, framing",
    descendTo: "config-modbus-slave-id" },
  { page: "CFG.D", id: "config-display", title: "Display", body: "LED pulses, flow unit",
    descendTo: "config-display-led-volume" },
  { page: "CFG.S", id: "config-sensors", title: "Sensors", body: "Channels 1-8",
    descendTo: "config-sensor-1" }
];
const CONFIG_RING = [...CONFIG_GROUPS.map((g) => g.id), "config-root-back"];

/** The serial link: how the device TALKS. */
const MODBUS_SETTINGS = [
  { page: "M1", id: "config-modbus-slave-id", title: "Modbus ID", binding: "config.modbusSlaveId" },
  { page: "M2", id: "config-modbus-baud-rate", title: "Baud Rate", binding: "config.baudRate" },
  { page: "M3", id: "config-modbus-parity", title: "Parity", binding: "config.parity" },
  { page: "M4", id: "config-modbus-stop-bits", title: "Stop Bits", binding: "config.stopBits" }
];
const MODBUS_RING = [...MODBUS_SETTINGS.map((m) => m.id), "config-modbus-back"];

/** How the device PRESENTS itself: the LED indicator, and the unit the panel shows flows in. */
const DISPLAY_SETTINGS = [
  { page: "D1", id: "config-display-led-volume", title: "LED Pulse Volume", binding: "config.ledPulseVolume" },
  { page: "D2", id: "config-display-led-period", title: "LED Pulse Period", binding: "config.ledPulsePeriod" },
  { page: "D3", id: "config-display-flow-unit", title: "Flow unit", binding: "config.flowUnit" }
];
const DISPLAY_RING = [...DISPLAY_SETTINGS.map((d) => d.id), "config-display-back"];

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

  /**
   * The required set and the fail/warn split both live in `tools/catalogue/coverage.mjs`.
   *
   * They were inline here, with the two exemptions — text and network — spelled out in comments this
   * function owned. They moved because N-b needs the SAME policy at a second call site: a pack stamped
   * with an older catalogue ABI, where a setting added since is a warning rather than a failure. Two
   * copies of an exemption list is the "one fact, two homes" failure this codebase keeps finding, so
   * there is one copy and the reasoning went with it.
   *
   * The skeleton is always built against the CURRENT catalogue, so nothing here can predate it and
   * every gap is a failure — exactly as before. `predating` is asserted empty rather than ignored,
   * because a non-empty one means the manifest and the ledger disagree about what exists.
   */
  const { missing, predating } = classifyCoverage({
    values: manifest.values,
    ledger,
    covered: declared,
    packAbi: manifest.catalogueAbi
  });
  if (predating.length > 0) {
    throw new Error(
      `${predating.map((p) => p.id).join(", ")} record a sinceAbi newer than the manifest's ` +
      `catalogueAbi (${manifest.catalogueAbi}). The ledger and the catalogue disagree about what ` +
      `exists; run node tools/catalogue/check-ledger.mjs.`
    );
  }

  /**
   * A GATED setting is still covered, provided its gate is itself reachable.
   *
   * R7.3 was relaxed for the calibration branch (see SENSOR_SETTINGS), so a setting's editor may
   * leave the level depending on another setting's value. The rule becomes "reachable under SOME
   * value of the setting that gates it" — still statically decidable, because the gate is a setting
   * whose options can be enumerated. What must be checked, and is checked here, is that the gate is
   * not ITSELF gated: a chain of conditions has no such enumeration and would put the rule back where
   * R7.3 refused to have it.
   */
  const gateOf = new Map(
    [...deviceList, ...sensorList].filter((d) => d.visibleWhen).map((d) => [d.binding, d.visibleWhen])
  );
  for (const [binding, gate] of gateOf) {
    const gateValue = manifest.values.find((v) => v.id === gate.binding);
    if (!gateValue || gateValue.category !== "setting") {
      throw new Error(
        `${binding} is gated on "${gate.binding}", which is not a setting. A gate must be a setting ` +
        `so the completeness rule can enumerate its values and prove the gated screen is reachable.`
      );
    }
    if (gateOf.has(gate.binding)) {
      throw new Error(
        `${binding} is gated on "${gate.binding}", which is itself gated. Chained conditions cannot ` +
        `be enumerated, which is exactly what R7.3 forbids — keep every gate ungated.`
      );
    }
    const reachable = (gateValue.options ?? []).some((option) => option.value === gate.equals);
    if (!reachable) {
      throw new Error(
        `${binding} is only visible when ${gate.binding} == ${gate.equals}, which is not one of its ` +
        `options. The setting would be unreachable at the panel.`
      );
    }
  }

  if (missing.length > 0) {
    throw new Error(
      `the skeleton would violate the completeness rule: no editor screen for ` +
      `${missing.join(", ")}. Add each to MODBUS_SETTINGS, DISPLAY_SETTINGS or SENSOR_SETTINGS ` +
      `(per-sensor) in tools/skeleton/generate.mjs, with a title a human can read. ` +
      `(Text and network settings are exempt — they are set via the portal, RS485 or the SD file.)`
    );
  }
}

CONFIG_GROUPS.forEach((g, i) => {
  screens.push({
    id: g.id,
    name: `${g.page} — ${g.title}`,
    description: `Config root entry ${g.page}. ENTER descends into the ${g.title} level; ` +
                 `UP/DOWN move within the root.`,
    elements: [
      text("hdr-title", L.headerY, `Config > ${g.title}`),
      text("group-body", L.bodyY, g.body, { emphasis: "muted" }),
      text("group-open", L.valueY, `${g.title} >`, { emphasis: "strong" }),
      text("footer-hint", L.footerY, "UP/DN pages  ENTER open", { emphasis: "muted" }),
      scrollbar()
    ],
    flows: [
      ...ringFlows(CONFIG_RING, i),
      btn("f-enter", `Open ${g.title}`, "enter", "short", A.descend, g.descendTo)
    ]
  });
});

/**
 * A level of leaf SETTINGS: a page per setting, each descending into its own editor.
 *
 * One emitter for Modbus and Display, because the two levels are the same shape. The old code had
 * this inline in the config-root loop with an `isDescent` branch threaded through it, which is what
 * let a descent and a leaf setting share a level in the first place.
 */
function emitSettingLevel({ items, ring, crumb, backId, backName }) {
  items.forEach((item, i) => {
    const v = byId.get(item.binding);
    if (!v) throw new Error(`catalogue has no value "${item.binding}"`);
    const unit = v.unit ? ` ${v.unit}` : "";
    screens.push({
      id: item.id,
      name: `${item.page} — ${item.title}`,
      description: `${crumb} entry ${item.page}. ENTER edits; UP/DOWN move within the level.`,
      elements: [
        text("hdr-title", L.headerY, `${crumb} > ${item.title}`),
        text("field-label", L.bodyY, `Current${unit}`, { emphasis: "muted" }),
        value("field-value", L.valueY, item.binding, { emphasis: "strong" }),
        text("footer-hint", L.footerY, "UP/DN pages  ENTER edit", { emphasis: "muted" }),
        scrollbar()
      ],
      flows: [
        ...ringFlows(ring, i),
        btn("f-enter", "Edit value", "enter", "short", A.descend, editorId(item.id))
      ]
    });
    screens.push(editorScreen({
      page: item.page, screenId: editorId(item.id), title: item.title,
      binding: item.binding, parentId: item.id
    }));
  });
  emitBackRow({ backId, backName, ring, crumb });
}

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
    btn("f-back", "Back one level", "enter", "short", A.back)
  ]
});

// ── L2 The two leaf-setting levels ──────────────────────────────────────────
emitSettingLevel({ items: MODBUS_SETTINGS, ring: MODBUS_RING, crumb: "Modbus",
                   backId: "config-modbus-back", backName: "M.BACK — Back" });
emitSettingLevel({ items: DISPLAY_SETTINGS, ring: DISPLAY_RING, crumb: "Display",
                   backId: "config-display-back", backName: "D.BACK — Back" });

// ── L3 Value editors ────────────────────────────────────────────────────────
function editorScreen({ page, screenId, title, binding, parentId, visibleWhen }) {
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
    ...(visibleWhen ? { visibleWhen } : {}),
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
      text("footer-hint", L.footerY, "UP/DN adjust  ENTER save  hold=cancel", { emphasis: "muted" })
    ],
    flows: [
      btn("f-inc", "Increase", "up", "short", A.inc),
      btn("f-dec", "Decrease", "down", "short", A.dec),
      btn("f-commit", "Save and go back", "enter", "short", A.commit, parentId),
      btn("f-discard", "Discard and go back", "enter", "long", A.discard, parentId)
    ]
  };
}

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
      btn("f-enter", `Open sensor ${n} settings`, "enter", "short", A.descend, "config-s1-connected")
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
    btn("f-back", "Back one level", "enter", "short", A.back)
  ]
});

// ── L3 Sensor settings: S1..S4 + BACK, and L4 their editors ─────────────────
/**
 * The per-sensor settings, with the INACTIVE calibration form hidden.
 *
 * A meter is specified one of two ways and never both — a datasheet prints either `F = 6*Q - 8` or
 * `450 pulses/L` — so S2 asks which, and the rows belonging to the other form leave the level
 * entirely. Showing both left half the sensor menu permanently inapplicable, with the operator having
 * to remember which half.
 *
 * `visibleWhen` RELAXES R7.3, which forbade runtime-hidden rows on the grounds that the completeness
 * rule must be statically decidable. It still is, and that is the whole reason this shape is
 * acceptable: the gate is a SETTING whose own editor is ungated, so the rule becomes "reachable under
 * some value of the setting that gates it" — provable by enumerating that setting's options.
 * `assertCoversEverySetting` below does exactly that. What R7.3 could not tolerate was a guard on
 * RUNTIME state, where no such enumeration exists; that is still forbidden.
 *
 * `config.sensor.calibrationType` stores 0 for Formula and 1 for Pulses/L (kCalibrationOptions).
 */
const CAL_FORMULA = { binding: "config.sensor.calibrationType", equals: 0 };
const CAL_PULSES = { binding: "config.sensor.calibrationType", equals: 1 };

const SENSOR_SETTINGS = [
  { page: "S1", id: "config-s1-connected", title: "Connected", binding: "config.sensor.connected" },
  { page: "S2", id: "config-s2-calibration", title: "Calibration", binding: "config.sensor.calibrationType" },
  { page: "S3", id: "config-s3-pulses-per-l", title: "Pulses per litre", binding: "config.sensor.pulsesPerLiter",
    visibleWhen: CAL_PULSES },
  { page: "S4", id: "config-s4-multiplier", title: "Multiplier (F)", binding: "config.sensor.multiplier",
    visibleWhen: CAL_FORMULA },
  { page: "S5", id: "config-s5-adjust", title: "Adjust", binding: "config.sensor.adjust",
    visibleWhen: CAL_FORMULA },
  // Q is the flow variable in BOTH forms — it is the channel's ceiling either way — so it is never
  // hidden. Neither is Connected, and neither is the question itself.
  { page: "S6", id: "config-s6-max-flow", title: "Max Flow (Q)", binding: "config.sensor.maxFlow" }
];
/**
 * The reset row is NOT in SENSOR_SETTINGS, and that is the point.
 *
 * Everything in that table is a setting: the loop below emits a value page AND an `-edit` editor for
 * each, and `assertCoversEverySetting` reads the table as the answer to "which manifest setting is
 * editable where". This row edits nothing — it descends into a hold-to-confirm, like P4's peak reset —
 * so it joins the RING without joining the table, exactly as the back row does.
 *
 * Hence the `S.RESET` name rather than an `S7` one: the S-numbers mean "setting n of this level", and
 * `config-s7-*` would also promise a spec file for an `-edit` screen that does not and must not exist.
 */
const SENSOR_RESET_CAL_ID = "config-sensor-settings-reset-cal";
const S_RING = [...SENSOR_SETTINGS.map((s) => s.id), SENSOR_RESET_CAL_ID, "config-sensor-settings-back"];

SENSOR_SETTINGS.forEach((s, i) => {
  const v = byId.get(s.binding);
  const unit = v?.unit ? ` ${v.unit}` : "";
  screens.push({
    id: s.id,
    name: `${s.page} — ${s.title}`,
    // The editor inherits its parent's gate: an editor whose list page has left the level is
    // unreachable anyway, and stating it means the completeness check sees one consistent story.
    ...(s.visibleWhen ? { visibleWhen: s.visibleWhen } : {}),
    description: `Sensor settings entry ${s.page}, scoped to the sensor of the current level.`,
    elements: [
      text("hdr-title", L.headerY, `Sensor > ${s.title}`),
      value("sensor-index", L.headerY, "config.selectedSensor", { emphasis: "muted", x: 200 }),
      text("field-label", L.bodyY, `Current${unit}`, { emphasis: "muted" }),
      value("field-value", L.valueY, s.binding, { emphasis: "strong" }),
      value("nyquist-warning", L.savedValueY, "config.sensor.nyquistWarning", { emphasis: "muted" }),
      text("footer-hint", L.footerY, "UP/DN pages  ENTER edit", { emphasis: "muted" }),
      scrollbar()
    ],
    flows: [
      ...ringFlows(S_RING, i),
      btn("f-enter", "Edit value", "enter", "short", A.descend, editorId(s.id))
    ]
  });
  screens.push(editorScreen({
    page: s.page, screenId: editorId(s.id), title: s.title,
    binding: s.binding, parentId: s.id, visibleWhen: s.visibleWhen
  }));
});
/**
 * S.RESET — the meter swap, one channel at a time.
 *
 * A broken sensor gets replaced by one with different characteristics, and the operator needs the old
 * meter's figures gone before the new one's can go in. What must NOT go is the volume: the old meter was
 * measuring truthfully right up to the failure, so the totals are real and carry on from where they
 * stopped. Every row above edits one calibration field; this one returns all of them to unset at once,
 * which is the same thing as returning the channel to `SET?`.
 *
 * The label says "calibration", never "values" or "sensor". The request was phrased as resetting the
 * sensor's values, but the accumulated values are precisely what survives, and a row promising to reset
 * what it keeps would be the panel lying to the person holding the datasheet. The confirm below then
 * says "Totals are kept" outright rather than leaving it to be inferred from a title.
 */
screens.push({
  id: SENSOR_RESET_CAL_ID,
  name: "S.RESET — Reset calibration",
  description: "Sensor settings action row: opens the reset-calibration confirm for the sensor of the current level.",
  elements: [
    text("hdr-title", L.headerY, "Sensor > Reset cal."),
    value("sensor-index", L.headerY, "config.selectedSensor", { emphasis: "muted", x: 200 }),
    text("field-label", L.bodyY, "Calibration back to unset", { emphasis: "muted" }),
    text("keeps-note", L.valueY, "Totals are kept", { emphasis: "strong" }),
    text("footer-hint", L.footerY, "UP/DN pages  ENTER opens", { emphasis: "muted" }),
    scrollbar()
  ],
  flows: [
    ...ringFlows(S_RING, S_RING.length - 2),
    btn("f-enter", "Open reset confirm", "enter", "short", A.descend, "confirm-reset-calibration")
  ]
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
    btn("f-back", "Back one level", "enter", "short", A.back)
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
/**
 * CONSOLIDATED. The network levels are PAGINATED INFORMATION PAGES, not one screen per value.
 *
 * There used to be fifteen screens between them — W1..W4, M1..M2, B1..B9 — each showing a single
 * read-only row and leaving two thirds of the panel empty. Once the owner ruled that the panel only
 * READS WiFi and MQTT, one screen per value bought nothing: paging fourteen times to see fourteen
 * facts, when four fit on a page and the scrollbar already says where you are in the level.
 *
 * So the values stay exposed and the screens collapse. Four rows to a page, the shape §7.1 gave the
 * information pages, and the level ring paginates them.
 */
const WIFI_INFO_PAGES = [
  {
    id: "net-wifi-info", page: "W.I1", title: "WiFi", rows: [
      { label: "State", binding: "net.wifi.state" },
      { label: "Enabled", binding: "config.wifi.enabled" },
      { label: "Network", binding: "config.wifi.ssid" },
      // Masked by formatSettingText. Present so an operator can see that a passphrase IS set
      // without it being readable off a wall panel.
      { label: "Passphrase", binding: "config.wifi.psk" }
    ]
  },
  {
    id: "net-wifi-info-2", page: "W.I2", title: "WiFi link", rows: [
      { label: "Address", binding: "net.wifi.ip" },
      { label: "Signal (dBm)", binding: "net.wifi.rssi" }
    ]
  }
];

const MQTT_INFO_PAGES = [
  {
    id: "net-mqtt-info", page: "M.I1", title: "MQTT", rows: [
      { label: "State", binding: "net.mqtt.state" },
      { label: "Enabled", binding: "config.mqtt.enabled" },
      { label: "Broker", binding: "config.mqtt.host" },
      { label: "Port", binding: "config.mqtt.port" }
    ]
  },
  {
    id: "net-mqtt-info-2", page: "M.I2", title: "MQTT broker", rows: [
      { label: "Username", binding: "config.mqtt.user" },
      { label: "Password", binding: "config.mqtt.password" },
      { label: "Topic", binding: "config.mqtt.baseTopic" },
      { label: "HA prefix", binding: "config.mqtt.discoveryPrefix" }
    ]
  },
  {
    id: "net-mqtt-info-3", page: "M.I3", title: "MQTT publish", rows: [
      { label: "HA discovery", binding: "config.mqtt.haDiscovery" },
      { label: "Period", binding: "config.mqtt.publishPeriod" },
      { label: "QoS", binding: "config.mqtt.qos" },
      // R4.4.2d — the panel half of "a refusal must be visible, not merely logged". On M.I3 because
      // it is the one MQTT page with a free row; a fourth page for one value would add a ring hop
      // between the operator and everything else, which §2c's paging exists to keep short.
      { label: "Last cmd", binding: "net.mqtt.lastCommandResult" }
    ]
  }
];

/**
 * The one WiFi row that is NOT a value: reaching the provisioning portal (R8.2a).
 *
 * It survives the consolidation because it is an ACTION, not a reading — and because §6.3 left the
 * panel with no other way to influence the portal at all. Removing it would leave a device whose
 * WiFi can only be configured by something the operator standing in front of it cannot reach.
 */
const WIFI_ACTIONS = [
  { page: "W4", id: "net-wifi-portal-reset", title: "Reset portal login", binding: null,
    descendTo: "confirm-reset-portal-login" }
];

const WIFI_RING = [...WIFI_INFO_PAGES.map((p) => p.id), ...WIFI_ACTIONS.map((a) => a.id),
                   "net-wifi-ap-info", "net-wifi-back"];
const MQTT_RING = [...MQTT_INFO_PAGES.map((p) => p.id), "net-mqtt-back"];

/**
 * Emits one navigation level: an entry screen per item, an editor for each that binds a setting,
 * and the closing BACK page.
 *
 * Factored rather than copied because there are now five levels with identical structure, and the
 * ring wiring is exactly the part that is silently wrong when hand-repeated.
 */

/**
 * An INFORMATION page: several read-only rows, no ENTER, no editor below it.
 *
 * Emitted directly rather than through emitLevel() because that builds a single label/value pair for
 * one setting, and these show four derived values at once. §7.1 always described them as their own
 * shape; they were deferred until N4 because none of the values they bind existed yet.
 *
 * No ENTER-short flow at all — there is nothing below to descend to, and a flow pointing at a
 * nonexistent screen is what the export gate rejects.
 */
function emitInfoPage({ id, page, title, crumb, ring, index, rows, footer }) {
  const y = [24, 46, 68, 90];
  const elements = [text("hdr-title", L.headerY, `${crumb} > ${title}`)];
  rows.forEach((row, i) => {
    elements.push(text(`row${i}-label`, y[i], row.label, { emphasis: "muted" }));
    elements.push(value(`row${i}-value`, y[i] + 10, row.binding, { emphasis: "strong" }));
  });
  elements.push(text("footer-hint", L.footerY, footer ?? "UP/DN pages",
                     { emphasis: "muted" }));
  elements.push(scrollbar());
  screens.push({
    id,
    name: `${page} — ${title}`,
    description: `${crumb} information page. Read-only; the values come from the live radio and broker.`,
    elements,
    flows: [
      ...ringFlows(ring, index)
    ]
  });
}

/**
 * Emits the `< BACK` row that closes a level.
 *
 * Split out of emitLevel because the MQTT level no longer has ANY bound items — the consolidation
 * turned all of them into information pages — and emitLevel used to be the only thing that emitted
 * the back row. A level whose ring names `net-mqtt-back` while nothing emits it is a flow pointing
 * at a screen that does not exist, which is exactly what the export gate rejects.
 */
function emitBackRow({ backId, backName, ring, crumb }) {
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
      btn("f-back", "Back one level", "enter", "short", A.back)
    ]
  });
}

function emitLevel({ items, ring, crumb, backId, backName, ringOffset = 0 }) {
  items.forEach((item, itemIndex) => {
    // Where this item sits in the RING, which is no longer the same as where it sits in `items`:
    // the network levels put information pages ahead of the action rows.
    const i = itemIndex + ringOffset;
    const isDescent = item.binding === null;
    const v = item.binding ? byId.get(item.binding) : null;
    if (item.binding && !v) throw new Error(`catalogue has no value "${item.binding}"`);
    const unit = v?.unit ? ` ${v.unit}` : "";
    /**
     * EVERY bound row in a network level is read-only — the panel reads WiFi and MQTT, it does not
     * set them.
     *
     * This used to be `v?.type === "string"`, exempting only the seven text settings because there is no
     * on-device text entry. The owner extended it to the whole of both levels: configuring a broker
     * through three buttons is, in their words, too painful, and the surfaces that were always the
     * real ones — the web portal, the RS485 register block, the SD credential file — already do it
     * better. So a numeric row like the broker port is read-only for the same reason the passphrase
     * always was, not for a different one.
     *
     * The rows themselves stay. An operator at the device still needs to see which network and which
     * broker it is pointed at, and that is the diagnostic half of §7.1's information pages at zero
     * input cost. Secrets render as "********" via formatSettingText, so a wall panel is safe.
     */
    const isReadOnly = !isDescent;
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
          isDescent ? "UP/DN pages  ENTER open"
                    : isReadOnly ? "Set via web portal or RS485"
                                 : "UP/DN pages  ENTER edit",
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
        ])
      ]
    });
    if (item.binding && !isReadOnly) {
      screens.push(editorScreen({
        page: item.page, screenId: editorId(item.id), title: item.title,
        binding: item.binding, parentId: item.id
      }));
    }
  });
  if (backId) emitBackRow({ backId, backName, ring, crumb });
}

// The two root-level entries. Their ring is the INFO ring, so they page with P0..P8.
[
  { id: "net-wifi-root", name: "WIFI — WiFi", crumb: "WiFi", enter: "net-wifi-info",
    prev: "info-p6-factory-reset", next: "net-mqtt-root",
    lines: ["Radio, network name and", "passphrase."] },
  { id: "net-mqtt-root", name: "MQTT — MQTT", crumb: "MQTT", enter: "net-mqtt-info",
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
      btn("f-enter", `Open ${r.crumb} settings`, "enter", "short", A.descend, r.enter)
    ]
  });
});

// Checked here rather than earlier: it must see EVERY list, and the network lists are declared
// above this point. Placed before the emit calls so a missing editor fails before any screen is
// written rather than after.
// Checked here rather than earlier: it must see EVERY list, and the network lists are declared
// above this point. Placed before the emit calls so a missing editor fails before any screen is
// written rather than after. The network settings are exempt by surface (see the function), so the
// consolidated info pages do not need to be passed in — they contain no editors at all.
assertCoversEverySetting([...MODBUS_SETTINGS, ...DISPLAY_SETTINGS], SENSOR_SETTINGS);

// The WiFi level: two paginated information pages, the portal action, the AP page, then BACK.
WIFI_INFO_PAGES.forEach((page, i) => {
  emitInfoPage({
    id: page.id, page: page.page, title: page.title, crumb: "WiFi",
    ring: WIFI_RING, index: i, rows: page.rows,
    footer: "UP/DN pages  hold=exit"
  });
});
emitLevel({ items: WIFI_ACTIONS, ring: WIFI_RING, crumb: "WiFi",
            backId: "net-wifi-back", backName: "W.BACK — Back",
            ringOffset: WIFI_INFO_PAGES.length });
emitInfoPage({
  id: "net-wifi-ap-info", page: "W6", title: "AP info", crumb: "WiFi",
  ring: WIFI_RING, index: WIFI_INFO_PAGES.length + WIFI_ACTIONS.length,
  rows: [
    { label: "AP network", binding: "net.ap.ssid" },
    // Shown in clear on purpose — R5.3. The device is broadcasting this network, so anyone in range
    // already sees it, and an operator at the panel has to read the key off to join.
    { label: "AP key", binding: "net.ap.password" },
    { label: "Browse to", binding: "net.ap.ip" },
    { label: "Closes in (s)", binding: "net.portal.remaining" }
  ],
  footer: "Join the AP and browse to the address"
});

// The MQTT level: three paginated information pages, then BACK. Everything B1..B9 and M1..M2 showed
// is on them.
MQTT_INFO_PAGES.forEach((page, i) => {
  emitInfoPage({
    id: page.id, page: page.page, title: page.title, crumb: "MQTT",
    ring: MQTT_RING, index: i, rows: page.rows,
    footer: "Set via web portal or RS485"
  });
});
emitBackRow({ backId: "net-mqtt-back", backName: "M.BACK — Back", ring: MQTT_RING, crumb: "MQTT" });

// ── P8 Factory Reset info page ───────────────────────────────────────────────
screens.push({
  id: "info-p6-factory-reset",
  name: "P6 — Factory Reset",
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
    btn("f-prev", "Previous page", "up", "short", A.prev, "info-p5-enter-config"),
    btn("f-enter", "Open confirm screen", "enter", "short", A.descend, "confirm-factory-reset")
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
    /**
     * P4's own reset, and the cheapest one here: 1500 ms, matching the session reset rather than the
     * 3 s of the totals.
     *
     * The peak is volatile — never written to NVS, so a power cycle already clears it — which is exactly
     * why it earns a route of its own. Before this it was reachable only as a side effect of the session
     * or measured resets, so clearing a spike after fixing the pipe that caused it meant giving up real
     * data. A hold is still required, because it is still a reset and the panel has one vocabulary for
     * those; it is simply the shortest one.
     */
    id: "confirm-reset-max-flow", name: "Reset peak flow?", title: "RESET PEAK FLOW?",
    warn: "Clears the peak on every", warn2: "channel. Totals are kept.",
    holdMs: 1500, action: "core.action.reset-max-flow", toast: "toast-max-flow-reset"
  },
  {
    /**
     * 3000 ms, and per CHANNEL rather than per device — the only confirm here that is.
     *
     * Against the existing vocabulary: 1500 is the tier for state a power cycle already clears, which
     * is why the session volume and the peak sit there. The calibration is not that. It is persisted
     * (firmware.cpp's 60 s dirty check writes `cfg_q/cfg_f/cfg_a` to NVS), and while it is unset the
     * channel measures nothing at all — pulses arrive and are discarded, because `configIsValid` is
     * false. So it belongs with the 3 s group, beside the totals reset and the portal login: persistent
     * state, gone until someone does something about it. It is emphatically not the 30 s tier, which is
     * reserved for wiping the whole device.
     *
     * `showSensor` puts the channel number on the screen. Every other confirm acts on all eight, so
     * none of them needed to name one; this one does, and a hold-to-destroy screen that will not say
     * WHICH channel it is about is the one place that ambiguity actually costs something.
     */
    id: "confirm-reset-calibration", name: "Reset calibration?", title: "RESET CALIBRATION?",
    warn: "Channel returns to SET? until", warn2: "new figures. Totals are kept.",
    holdMs: 3000, action: "core.action.reset-calibration", toast: "toast-calibration-reset",
    showSensor: true
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

/**
 * Each confirm is a TWO-ENTRY LEVEL: the confirm itself, and a `< BACK` row.
 *
 * ENTER-short on the confirm is now UNATTACHED, and therefore ignored — `findFlow` returns null and
 * the interaction handler does nothing. That is the harmonisation: long-ENTER stops being a hidden
 * global gesture and becomes an ordinary per-screen flow that fires only where a screen declares one.
 *
 * It replaces `f-exit`, an ENTER-short that meant "leave without acting" and sat one slip away from
 * the hold that acts. Leaving is now the same motion as leaving any other level — page to `< BACK`,
 * press ENTER — so the only gesture that does anything destructive on this screen is the deliberate
 * hold, and the only gesture that leaves it is the one that leaves everywhere else.
 *
 * The hold itself is unchanged: it runs the on-screen countdown and releasing early abandons it.
 */
for (const c of CONFIRMS) {
  const backId = `${c.id}-back`;
  const ring = [c.id, backId];
  screens.push({
    id: c.id,
    name: c.name,
    description: `Confirm screen. ENTER-short is unattached and ignored; ENTER held ${c.holdMs} ms ` +
                 `confirms, and releasing early abandons the countdown. UP/DOWN pages to < BACK.`,
    elements: [
      overlay(),
      // Only for a confirm that acts on ONE channel. Spread rather than conditionally pushed so the
      // device-wide confirms emit byte-for-byte what they emitted before this field existed.
      ...(c.showSensor
        ? [value("sensor-index", L.headerY, "config.selectedSensor", { emphasis: "muted", x: 200 })]
        : []),
      text("title", L.bodyY, c.title, { emphasis: "strong" }),
      text("warning-1", L.bodyY + 18, c.warn, { emphasis: "muted" }),
      text("warning-2", L.bodyY + 30, c.warn2, { emphasis: "muted" }),
      { id: "timer-value", kind: "value", x: 104, y: L.valueY + 34, binding: "countdown.value", emphasis: "strong" },
      text("footer-hint", L.footerY, "hold ENTER confirms  UP/DN back", { emphasis: "muted" })
    ],
    flows: [
      ...ringFlows(ring, 0),
      {
        id: "f-confirm",
        label: c.name.replace("?", ""),
        trigger: { type: "timeout", durationMs: c.holdMs, holdButton: "enter" },
        actionId: c.action,
        ...(c.toast ? { targetScreenId: c.toast } : {})
      }
      // No f-escape: long-ENTER here means CONFIRM, and nothing else claims it.
    ]
  });
  emitBackRow({ backId, backName: `${c.name.replace("?", "")} — Back`, ring, crumb: c.title });
}

const TOASTS = [
  { id: "toast-totals-reset", name: "Totals reset", message: "TOTALS RESET" },
  { id: "toast-session-reset", name: "Session reset", message: "SESSION RESET" },
  // R8.2a. Names the credential explicitly rather than saying "done": the operator now has to go
  // and use admin/admin, and a toast that does not say so leaves them guessing what changed.
  { id: "toast-max-flow-reset", name: "Peak flow reset", message: "PEAK RESET" },
  { id: "toast-portal-login-reset", name: "Portal login reset", message: "LOGIN: admin/admin" },
  // Says CAL, not "DONE": the operator's next move is to walk back up the level and type the new
  // meter's figures in, and a toast that only acknowledges leaves them wondering whether the totals
  // went with it. "CAL CLEARED" names the one thing that changed.
  { id: "toast-calibration-reset", name: "Calibration reset", message: "CAL CLEARED" }
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

// The ring change, applied before anything else reads an id: drop the two absorbed litres pages, rename the
// five that moved up, and rewrite every flow target that pointed at either.
dataset.screens = dataset.screens.filter((s) => !RING_DROPPED.has(s.id));
for (const s of dataset.screens) {
  if (RING_RENAMES[s.id]) {
    s.id = RING_RENAMES[s.id];
    if (RING_NAMES[s.id]) s.name = RING_NAMES[s.id];
  }
  for (const flow of s.flows ?? []) {
    if (flow.targetScreenId && RING_RENAMES[flow.targetScreenId]) {
      flow.targetScreenId = RING_RENAMES[flow.targetScreenId];
    }
  }
}
// Rebuild UP/DOWN across the ring from INFO_RING. Renaming targets alone left P1 pointing DOWN at the
// absorbed litres page: a dropped screen has no rename, so patching cannot express "the page that took over".
for (const s of dataset.screens) {
  const at = INFO_RING.indexOf(s.id);
  if (at === -1) continue;
  const prev = INFO_RING[(at - 1 + INFO_RING.length) % INFO_RING.length];
  const next = INFO_RING[(at + 1) % INFO_RING.length];
  s.flows = (s.flows ?? []).filter((f) => !(f.trigger?.type === "button"
    && (f.trigger.button === "up" || f.trigger.button === "down") && f.trigger.gesture === "short"));
  s.flows.unshift(btn("f-prev", "Previous page", "up", "short", A.prev, prev));
  s.flows.unshift(btn("f-next", "Next page", "down", "short", A.next, next));
}
const generatedIds = new Set(screens.map((s) => s.id));
const RETIRED = new Set([
  /**
   * §5.5's prompt as a SCREEN — the design that lost, retired 2026-08-21 (J9).
   *
   * The prompt is real and works; it just does not live on a screen of its own. `ui_actions.cpp`'s
   * `consumedByPrompt` reinterprets UP and DOWN on the editor screen itself while a commit is parked
   * awaiting an override, and its comment gives the reason: no new screen id, so the §3.0.1
   * completeness rule stays satisfied and no dataset change is needed. The `nyquist-warning` ELEMENT
   * on every sensor-settings screen — bound to `config.sensor.nyquistWarning`, emitted a few hundred
   * lines above — is that prompt's rendering surface and stays exactly where it is.
   *
   * What retires here is only the screen with that id, which no flow named as a `targetScreenId` and
   * no `ui_pages.h` table named either, so `UiScreenRouter` could never resolve it. It carried
   * plausible-looking UP/DOWN flows, which is what made it cost the next reader an investigation.
   */
  "nyquist-warning",
  // Replaced by confirm screens; enter-config, sensor-save and config-exit are no
  // longer guarded actions at all under the new model.
  "countdown-enter-config", "countdown-reset-session", "countdown-reset-all",
  "countdown-factory-reset", "countdown-sensor-save", "countdown-config-exit",
  // The seven text-setting editors, retired with on-device text entry itself (§6.3). Listed
  // explicitly because `kept` retains anything the generator no longer emits — without this they
  // would survive as orphans: editor screens with a commit flow, bound to config.editor.pending,
  // that no longer have a parent descending into them and no engine behind them.
  "net-wifi-ssid-edit", "net-wifi-psk-edit", "net-mqtt-host-edit", "net-mqtt-user-edit",
  "net-mqtt-password-edit", "net-mqtt-base-topic-edit", "net-mqtt-prefix-edit",
  // config.mqtt.tls, removed to honour Q3/R8.3: TLS is out of scope for this version, and a toggle
  // that does nothing implies protection that is not there. §6.1 had listed it, contradicting the
  // decision; the decision wins. Retired explicitly because `kept` preserves anything the generator
  // stops emitting, and these would otherwise survive bound to a catalogue id that no longer exists.
  "net-mqtt-tls", "net-mqtt-tls-edit",
  // The six NUMERIC network editors, retired with the decision that the panel only reads WiFi and
  // MQTT (see isReadOnly in emitLevel). Listed explicitly for the same reason the text editors are:
  // `kept` preserves anything the generator stops emitting, so without this they would survive as
  // orphans — editors with a commit flow and no parent descending into them.
  "net-wifi-enabled-edit", "net-mqtt-enabled-edit", "net-mqtt-port-edit",
  "net-mqtt-period-edit", "net-mqtt-qos-edit", "net-mqtt-ha-discovery-edit",
  // The fifteen one-value-per-screen network rows, replaced by the paginated information pages.
  // Every value they showed is still on the panel — W.I1/W.I2 and M.I1/M.I2/M.I3 carry all of them —
  // so this is a consolidation, not a removal. Listed explicitly because `kept` preserves anything
  // the generator stops emitting, and these would otherwise survive as unreachable orphans.
  "net-wifi-enabled", "net-wifi-ssid", "net-wifi-psk",
  "net-mqtt-enabled", "net-mqtt-setup", "net-mqtt-setup-back",
  "net-mqtt-host", "net-mqtt-port", "net-mqtt-user", "net-mqtt-password",
  "net-mqtt-base-topic", "net-mqtt-prefix", "net-mqtt-period",
  "net-mqtt-ha-discovery", "net-mqtt-qos",
  // The old sensor-settings numbering. S2..S4 were Multiplier/Adjust/Max Flow; the calibration
  // branch inserts Calibration at S2 and Pulses per litre at S3, so the three formula rows moved to
  // S4..S6. Same settings, new ids — retired explicitly so the old ids do not survive in `kept` as
  // duplicates bound to the same settings.
  // The flat C1..C8 config root, replaced by three grouped levels. Ids named a POSITION, which is
  // exactly why inserting the flow unit renumbered Sensors from C7 to C8 and every reference to the
  // old id went quietly wrong; the new ids name their group instead. Retired explicitly because
  // `kept` preserves anything the generator stops emitting.
  "config-c1-modbus-id", "config-c1-modbus-id-edit",
  "config-c2-baud-rate", "config-c2-baud-rate-edit",
  "config-c3-parity", "config-c3-parity-edit",
  "config-c4-stop-bits", "config-c4-stop-bits-edit",
  "config-c5-led-pulse-vol", "config-c5-led-pulse-vol-edit",
  "config-c6-led-pulse-period", "config-c6-led-pulse-period-edit",
  "config-c7-flow-unit", "config-c7-flow-unit-edit",
  "config-c7-sensor-select", "config-c8-sensor-select",
  "config-s2-multiplier", "config-s2-multiplier-edit",
  "config-s3-adjust", "config-s3-adjust-edit",
  "config-s4-max-flow", "config-s4-max-flow-edit"
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

let specApplied = 0;
let moved = 0, resized = 0, barsAdded = 0, badgesMoved = 0, asciiFolded = 0, dividersWidened = 0, legendMoved = 0, textCorrected = 0, descriptionsFixed = 0;
for (const s of kept) {
  /**
   * A SPECIFIED screen's layout is the requirement, and needs none of the repair below.
   *
   * The repairs exist for screens authored before Display_Per_Screen_Spec.md: re-stacking a footer zone,
   * widening portrait dividers, moving a badge off a title. Running them over a specified layout would move
   * elements the spec placed deliberately — and the footer re-stack proved it by refusing P0 outright, whose
   * agreed layout has more elements in its lower half than the repair's four rows allow.
   */
  const fromSpec = specElements(s.id);
  const hasSpec = Boolean(fromSpec);
  if (hasSpec) {
    s.elements = fromSpec;
  }
  if (!hasSpec) for (const e of s.elements) {
    if (e.kind === "box" && e.width === 135 && e.height === 240) { e.width = W; e.height = H; resized += 1; }
  }

  // Everything in the footer zone is re-stacked deterministically, not just what
  // currently overflows. Detecting only overflow is not idempotent: a previous run
  // had already pulled two elements up onto the same row, so on the next run
  // neither overflowed and the collision was invisible.
  const FOOTER_ZONE_TOP = FOOTER_ROWS[FOOTER_ROWS.length - 1];
  const strays = hasSpec ? [] : s.elements
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
  const hdr = hasSpec ? null : s.elements.find((e) => e.id === "hdr-title");
  if (hdr) {
    for (const e of s.elements) {
      if (e === hdr || e.kind !== "badge") continue;
      const overlapsY = e.y < hdr.y + implicitH(hdr) && hdr.y < e.y + implicitH(e);
      if (overlapsY && Math.abs(e.x - hdr.x) < 60) { e.x = 176; badgesMoved += 1; }
    }
  }
  if (!hasSpec && s.id.startsWith("info-p") && !s.elements.some((e) => e.kind === "scrollbar")) {
    s.elements.push(scrollbar()); barsAdded += 1;
  }
  // Info pages: ENTER-short now descends into a confirm screen or Configuration.
  /**
   * P4 descends into a confirm of ITS OWN, which is a reversal worth recording.
   *
   * It used to descend into `confirm-reset-session` — the same confirm P3 opens — and that was removed
   * for two good reasons: an operator looking at max flow got a screen titled RESET SESSION? warning
   * about session totals, which reads as the wrong screen arriving; and the peak is volatile, so there
   * was nothing persistent on the page for a reset to act on. The conclusion drawn at the time was that
   * P4 should claim no gesture at all.
   *
   * The second half of that reasoning was backwards. Volatile is not a reason to withhold a reset; it is
   * what makes one CHEAP. A channel that spiked once keeps showing that spike until the next power cycle,
   * and the only ways to clear it were the session and measured resets — both of which destroy real data
   * to get at a number that costs nothing. So P4 now has its own confirm, its own action and its own
   * register, and the first objection is answered by the screen being P4's rather than P3's.
   */
  const descend = {
    "info-p2-cumulative-m3": "confirm-reset-totals",
    "info-p3-session-m3": "confirm-reset-session",
    "info-p4-max-flow": "confirm-reset-max-flow",
    "info-p5-enter-config": "config-modbus"
  }[s.id];
  // Every info page gets its ENTER flows rebuilt, not just those with a descent
  // target. P0 and P1 previously kept 0.1-era `ui.action.mode.idle -> state-idle`
  // flows: display-off moved to UP+DOWN (firmware-native, §3.1) and ENTER-long is
  // now unambiguously escape (§3.2), so those were the only two screens
  // contradicting the gesture contract.
  if (s.id.startsWith("info-p")) {
    s.flows = (s.flows ?? []).filter((f) => f.trigger.type !== "button" || f.trigger.button !== "enter");
    if (descend) s.flows.push(btn("f-enter", "Open", "enter", "short", A.descend, descend));
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
  if (!hasSpec) for (const e of s.elements) {
    if (e.kind === "box" && e.height === 1 && e.width === 135) { e.width = W; dividersWidened += 1; }
  }

  // Footer text that still advertises retired durations.
  // Only for screens without a spec: a specified screen's footer comes from its requirement file.
  const footerText = specs.has(s.id) ? undefined : {
    "info-p0-global-status": "UP/DN pages   UP+DN off"
  }[s.id];
  if (footerText) {
    const fh = s.elements.find((e) => e.id === "footer-hint");
    if (fh) fh.content = footerText;
  }

  // §4.3 note 5 and §6 note 8: the LED legend belongs on P0, the landing page. It
  // was on P1-P7 and absent from P0 — the exact inverse.
  // Skipped for a specified screen: P0's spec folded the LED text into net-led-status, and an
  // "add it if missing" repair reads that deliberate removal as an omission and undoes it.
  if (!hasSpec && s.id.startsWith("info-p")) {
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
  if (s.id === "info-p5-enter-config" && !hasSpec) {
    const prompt = { "prompt-line1": "Press ENTER to open", "prompt-line2": "Configuration." };
    for (const e of s.elements) {
      if (prompt[e.id] && e.content !== prompt[e.id]) { e.content = prompt[e.id]; textCorrected += 1; }
    }
  }

  // Screen descriptions still advertised retired countdowns and a dropped
  // propeller animation. They are metadata, but they are what the next person reads.
  const desc = specs.get(s.id)?.description ?? {
    "info-p0-global-status": "P0 landing page: current aggregate flow, volume since reset, the peak and its channel.",
    "info-p1-instant-flow": "P1: instantaneous flow for all eight sensors, two columns of four.",
    "info-p2-cumulative-m3": "P2: cumulative volume in m3. ENTER opens the reset-totals confirm screen (3 s hold).",
    "info-p3-session-m3": "P3: session volume in m3. ENTER opens the reset-session confirm screen.",
    "info-p4-max-flow": "P4: peak flow per channel since the last session reset; MAX marks a channel at its ceiling.",
    "info-p5-enter-config": "P5: entry point to Configuration. ENTER descends to the config root."
  }[s.id];
  if (desc) { s.description = desc; descriptionsFixed += 1; }

  // The §5.5 block that used to sit here repaired the flows of a screen that is now RETIRED (J9):
  // the prompt lives on the editor screen, via ui_actions.cpp's consumedByPrompt, and has no flows
  // of its own to repair.
  // P7 now wraps to P8 rather than back to P0.
  if (s.id === "info-p5-enter-config" && !hasSpec) {
    for (const f of s.flows) {
      if (f.id === "f-next") f.targetScreenId = "info-p8-factory-reset";
    }
  }
  if (s.id === "info-p0-global-status") {
    // The main-screen connection indicator, which was the owner's very first request for this
    // feature. y=98 is the free row between the flow icon (70) and the LED legend (112), so it groups
    // with the other status lines rather than displacing telemetry.
    if (!hasSpec && !s.elements.some((e) => e.id === "net-status")) {
      s.elements.push(value("net-status", 98, "net.status", { emphasis: "muted" }));
    }
    for (const f of s.flows) {
      // The root ring now ends with the two network entries, so UP from P0 reaches MQTT, not P8.
      if (f.id === "f-prev") f.targetScreenId = "net-mqtt-root";
    }
  }
}

const merged = { ...dataset, screens: [...kept, ...screens] };

/**
 * One pass over EVERY screen: a specified layout wins, whether the screen was kept or generated.
 *
 * The emitters above build 69 of the 77 screens from their own tables, and those never pass through the
 * repair loop — so overriding inside that loop reached only the 8 hand-tuned info pages and left every
 * config and net screen on the generator's own geometry. Layout has one source; this is where it is applied.
 */
for (const screen of merged.screens) {
  const fromSpec = specElements(screen.id);
  if (!fromSpec) continue;
  screen.elements = fromSpec;
  const spec = specs.get(screen.id);
  if (spec?.description) screen.description = spec.description;
  specApplied += 1;
}
const unspecified = merged.screens.filter((s) => !specs.has(s.id)).map((s) => s.id);

const report = {
  kept: kept.length,
  generated: screens.length,
  retired: [...RETIRED].filter((id) => dataset.screens.some((s) => s.id === id)).length,
  total: merged.screens.length,
  layoutsFromSpec: specApplied,
  unspecifiedScreens: unspecified,
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
