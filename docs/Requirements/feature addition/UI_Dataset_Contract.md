# The UI dataset contract — what each JSON is, who owns it, and what checks it

**Scope.** Four JSON artefacts describe the panel, and they are not peers: two are authored, two are
generated, and each edge between them is enforced by a gate that fails the build. This document says
which is which, what each field means, and which check catches you if you get it wrong.

Read this before editing any of them. The single most common mistake is editing a generated file —
your change works locally, then vanishes the next time the generator runs, and CI reports a diff you
did not expect.

---

## 1. The pipeline

```
     AUTHORED                                  GENERATED
 ┌──────────────────────────┐
 │ docs/.../screens/*.json  │  one file per screen: the agreed GEOMETRY
 │   the requirement        │  reviewed screen by screen (Display_Per_Screen_Spec.md)
 └───────────┬──────────────┘
             │
 ┌───────────┴──────────────┐   ┌──────────────────────────────────┐
 │ tools/skeleton/           │   │ src/data/actionManifest.json      │
 │   generate.mjs            │◄──┤   the firmware's CATALOGUE        │
 │   owns NAVIGATION         │   │   what values and actions exist   │
 └───────────┬───────────────┘   └───────────▲──────────────────────┘
             │                                │ manifest_gen/run.sh
             ▼                    ┌───────────┴──────────────────────┐
 ┌──────────────────────────┐     │ ui_value_catalogue.cpp           │
 │ src/data/screens.json     │     │ ui_settings_types.cpp            │
 │   THE DATASET             │     │   the firmware source of truth   │
 │   geometry + navigation   │     └──────────────────────────────────┘
 └───────────┬───────────────┘
             │ npm run export:firmware
             ▼
 ┌───────────────────────────────────────────────────────────┐
 │ src/ui/generated/GeneratedUi.{h,cpp}   compiled-in tables  │
 │ src/ui/generated/ui_export_ir.json     debugging IR        │
 │ tests/fixtures/default.uipack          loadable pack       │
 └───────────────────────────────────────────────────────────┘
```

Two rules follow from the shape, and both have bitten this project:

- **`screens.json` is GENERATED.** It was hand-authored once and is not any more. Edit the requirement
  file for geometry, or `generate.mjs` for navigation. CI runs the generator and fails on a diff.
- **`actionManifest.json` is GENERATED FROM FIRMWARE.** A binding cannot be invented in the dataset;
  it has to exist in the firmware catalogue first. `run.sh --check` fails if the committed manifest has
  drifted from the C++.

---

## 2. `docs/Requirements/feature addition/screens/<id>.json` — the requirement

One file per screen, holding the geometry that was reviewed and agreed. The generator reads these and
defers every coordinate to them.

```jsonc
{
  "id": "config-c2-baud-rate",
  "name": "C2 — Baud Rate",
  "description": "Config root entry C2. ENTER descends; UP/DOWN move within the level.",
  "elements": [
    {
      "id": "field-value",
      "kind": "value",
      "x": 2, "y": 24,
      "emphasis": "strong",
      "binding": "config.baudRate",

      // SPEC-ONLY. Stripped by the generator; the dataset schema does not know them.
      "worst": "115200",
      "bound": "widest baud option"
    }
  ]
}
```

### 2.1. The three spec-only fields

| Field | Meaning |
| --- | --- |
| `worst` | The widest string this element can ever render. What the geometry audit measures. |
| `bound` | WHY that is the worst case — a physical limit, an enum, or a fixed literal. |
| `bannerReplaces` | Set on the one element the §2c warning banner is designed to cover: the footer. |

**`worst` must come from a physical bound, never from a format string.** A printf field width is a
MINIMUM: `%7.2f` of a channel clamped to `q_max = 65535` renders `65535.00`, which is eight characters,
not seven. Two real defects came from treating a width as a ceiling, and one from declaring `worst` as
`"?"` — one character — on a row whose binding holds 64 bytes, which made the audit pass on a fiction.

`bound` exists so the next reader can check the claim instead of trusting it. "geometry" and
"fixed literal" are acceptable answers.

---

## 3. `web/mockup/src/data/screens.json` — the dataset

Generated. The schema is `web/mockup/shared/schemaDefinitions.ts`, which is also what the exporter
validates against — one declaration, not a prose copy that can drift.

```jsonc
{
  "screens": [
    {
      "id": "config-s4-multiplier",
      "name": "S4 — Multiplier (F)",
      "description": "...",

      // Screen-level visibility — see §3.3.
      "visibleWhen": { "binding": "config.sensor.calibrationType", "equals": 0 },

      "elements": [ /* §3.1 */ ],
      "flows":    [ /* §3.2 */ ]
    }
  ],
  "theme": { /* themeTokens.json */ }
}
```

### 3.1. Elements — what is drawn

| Field | Notes |
| --- | --- |
| `id` | Unique within the screen. Layout convention, not semantics — never key behaviour off it. |
| `kind` | `text` · `value` · `badge` · `box` · `icon` · `scrollbar` |
| `x`, `y` | Top-left, in device pixels. Origin top-left, 240 × 135 landscape. |
| `width`, `height` | Geometry kinds only. A `badge` sizes to its content when omitted. |
| `content` | Literal text. Ignored when `binding` resolves. |
| `binding` | A value id from the manifest. Must exist there — see §5. |
| `emphasis` | `normal` · `strong` · `muted`. A COLOUR, not a weight: Font0 has no bold. |
| `metadata.assetId` | For `icon`. `drawIconElement` dispatches on `strcmp(assetId, "flow-dots")`, so an icon without one draws **nothing at all**. |

Glyph advances are **6 px** for text and badges, **7 px** for a `value`, and every glyph is 8 px tall.
That gives 40 columns and 17 rows, the last clipped by 1 px. A `value` is wider per character than the
grid, so a long one overhangs — the audit accounts for it.

Text longer than its row is **clipped with a trailing `~`** (`UiRenderer::drawTextElement`, mirrored by
`clipToPanel`). This is not a layout escape hatch: a 64-byte broker host cannot be shown on a 240 px
panel, and a silently shortened hostname reads as the whole one.

### 3.2. Flows — what buttons do

```jsonc
{ "id": "f-enter", "label": "Edit value", "actionId": "ui.action.nav.descend",
  "targetScreenId": "config-c2-baud-rate-edit",
  "trigger": { "type": "button", "button": "enter", "gesture": "short" } }
```

Triggers in use:

| Trigger | Meaning |
| --- | --- |
| `button` + `up`/`down`/`enter` + `short`/`long`/`hold` | A press. `hold` is the auto-repeat of a held button. |
| `timeout` + `durationMs` | Fires unattended after the delay. The toasts use this. |
| `timeout` + `durationMs` + `holdButton` | **Hold-to-confirm.** Runs the on-screen countdown; releasing early abandons it. |

**Dispatch is on `actionId` first, not `targetScreenId`.** The firmware resolves "one level up" from its
own stack, so `ui.action.nav.back` has no target to follow — following the target alone is what made
BACK dead once.

**An unclaimed gesture does nothing.** `findFlow` returns null and the interaction handler makes no
default. That is the whole basis of the gesture contract: long-ENTER is not a global escape, it is a
flow that a screen either declares or doesn't. It is declared on exactly two screen kinds — confirms
(hold = confirm) and editors (discard, because an editor has no `< BACK` row to page to, its UP/DOWN
being spent on the value).

`Flow.guard` exists in the schema and is emitted into `GeneratedUi`. **Nothing evaluates it.** Use
`visibleWhen` instead; the field is retained only because removing it is an ABI change.

### 3.3. `visibleWhen` — conditional screens

```jsonc
"visibleWhen": { "binding": "config.sensor.calibrationType", "equals": 0 }
```

The screen is part of its level only while that setting holds that value. Absent means unconditional,
which is every screen but six.

It exists because a meter is calibrated one of two ways and never both — a datasheet prints either
`450 pulses/L` or `F = 6*Q - 8` — so showing the rows for both leaves half the sensor menu permanently
inapplicable.

**It relaxes R7.3**, which forbade runtime-hidden rows because the completeness rule (every settable
value has a reachable editor) must be statically decidable. It still is, and that is the only reason
this is allowed: the gate is a SETTING whose options can be enumerated, so the rule becomes *reachable
under some value of the gate*. `assertCoversEverySetting` enforces three things and throws on each:

1. the gate is a setting (not a reading, not derived);
2. the gate is not **itself** gated — chained conditions cannot be enumerated, and that is exactly what
   R7.3 refused;
3. `equals` is one of the gate's declared options, or the gated setting would be unreachable.

A guard on RUNTIME state — flow, connection, a warning bit — is still forbidden, for R7.3's original
reason.

Both the navigator and the ring-position report step over hidden screens through **one** function
(`nextVisibleSibling`), so the scrollbar and `nav.position` cannot disagree with where UP/DOWN goes.

---

## 4. `web/mockup/src/data/actionManifest.json` — the catalogue

Generated from `ui_value_catalogue.cpp` and `ui_settings_types.cpp` by
`Water-Flow-Meter-PlatformIO/tools/manifest_gen/run.sh`. Never hand-edit: `--check` is what CI runs, and
a hand edit fails it as stale.

It carries, per value: `id`, `category`, `type`, `unit`, `register`, `readOnly`, `description`, and for a
setting also `min`, `max`, `step`, `options`, `perSensor`, `registerOffset`, `maxLength`, `writeOnly`.

**The descriptor is the single home for a setting's domain.** Two display strings are DERIVED from it
rather than authored, and both had been static placeholders that were wrong on every screen but one:

- `config.editor.range` — `1 to 247`, `None / Even / Odd`, `1200..115200 (8)`. Formatted by
  `formatSettingRange` on the device and `rangeHintFor` in the mockup.
- `config.editor.pending` — the value an open editor has dialled up, formatted by the same descriptor
  as the saved one.

A per-sensor derived value may declare **no register** (`kNoRegister`), and one does:
`sensor.<n>.pulsesPerLitre` is register 24 on a pulses-calibrated channel but computed from the
multiplier on a formula-calibrated one, so advertising an address would send a master to read a zero.

---

## 5. The gates, and what each one catches

Run by CI in three jobs. All of them run locally too, and the host suite is the one most easily
forgotten — it caught four breaks that `tsc`, the unit tests and a PlatformIO compile all passed.

| Check | Fails when |
| --- | --- |
| `manifest-value-coverage` | An element binds an id the manifest lacks. **Hard fail** — the element would render its placeholder on hardware and look live in the mockup. |
| `firmware-binding-coverage` | A bound value has no case in `UiBindingResolver`. Warning only, so treat it as fatal yourself. |
| `firmware-manifest-resolvable` | The firmware advertises a value it cannot resolve. |
| `manifest-screen-coverage` | A screen id the firmware resolves by name is missing. Catches renames — `kInfoScreenIds` in `ui_pages.h`. |
| generator idempotence | `generate.mjs` no longer reproduces the committed `screens.json`. |
| asset freshness | `src/ui/generated` differs from a fresh export (timestamps excluded). |
| `screen-geometry.ts` | Collisions, overflow past 240 × 135, an icon with no `assetId`. Uses `worst`. |
| `screen-values.ts` | An unresolved `{{binding}}`, a value outside its descriptor's domain, a range hint on a screen with no setting. Checks the TEXT, which nothing did before. |
| host `pack_test` round trip | The emitted pack and the compiled table disagree — **field by field**, including `visibleWhen`. A round-trip test is only as good as the fields it knows to look at. |
| host `interaction_test` | The completeness rule, the gesture contract, the reset paths. |

---

## 6. Editing recipes

**Move an element / change its text** → the requirement file in `docs/.../screens/`, then
`node tools/skeleton/generate.mjs --write`, then `npm run export:firmware`.

**Add a screen to a level** → `generate.mjs` (it owns rings, descents and BACK rows), plus a requirement
file for its geometry.

**Add a bindable value** → firmware first: a `kSimpleValues` or `kSensorMetrics` entry in
`ui_value_catalogue.cpp` AND a resolver arm in `ui_bindings.cpp`. Then `manifest_gen/run.sh`. A
catalogue entry without an arm renders blank on hardware and only warns.

**Add a setting** → a descriptor in `ui_settings_types.cpp`, a `SettingTarget`, read/write arms in
`ui_settings.cpp`, and an answer in `portal_form.cpp`'s three switches — those are `-Werror=switch`, and
that is deliberate: adding a target is a build failure until somebody says where the portal sends it.
Then list it in `DEVICE` or `SENSOR_SETTINGS` so the completeness rule is satisfied.

**Change the pack format** → both sides and the version: `packEmitter.ts`, `ui_pack.{h,cpp}`,
`kFormatVersion`. Bump it, so an older pack is refused rather than read at the wrong stride. Then extend
the round-trip test to compare the new field, or nothing will notice it going missing.

---

## 7. Conventions worth knowing before you argue with the code

- **The panel is a view; the wire is the record.** The panel shows m³ at two decimals; Modbus and MQTT
  carry litres AND m³ at full precision. A display preference must never change a register.
- **Every flow is L/min**, storage included (§2a). L/s survives only as a derived reading for consumers
  that ask for it.
- **The unit lives in the header**, not the row (§4.3). Per-row units are impossible on the volume
  pages, where `8: 99999999.99 m^3` overflows two columns by 19 px.
- **`--` means not in service, never "not detected."** No presence detection exists and none is
  possible: an idle passive pulse sensor is indistinguishable from one whose wire fell off.
- **`SET?` means no valid calibration**, and replaced `WAIT`, which implied warming up when the real
  condition needs an operator.
- **One home per fact.** This project's recurring defect is two, and the second one always wins on
  screen: a range hint duplicating a descriptor, a sample table duplicating a resolver, an id list
  duplicating an enum. When you find yourself copying a value, delete the copy and derive it.
