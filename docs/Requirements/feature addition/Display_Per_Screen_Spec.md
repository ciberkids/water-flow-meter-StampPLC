# Display — Per-Screen Specification

**Status:** in progress, one screen at a time, in ring order.
**Companion to:** `Display_UI_Requirements.md` (§4.3 says *what data* each page carries; this says *what it
draws and where*), `RGB_LED_Behavior.md`, `UI_Firmware_Interface.md`.

## 1. Why this document exists

`Display_UI_Requirements.md` §4.3 gives a per-page content table — page, title, data source, ENTER
behaviour. It contains no coordinate, no field width, and no statement of how much text fits on the panel.
The dataset was therefore authored by eye, and an audit against the firmware's own text metrics found
**50 of 79 screens with defects: 63 text collisions and 44 panel overflows.**

Neither class was catchable before: the web mockup sizes badges to their content, so the overpaint never
appeared there, and its layout diagnostics test out-of-bounds only — never element-versus-element. The two
tools added alongside this document close that gap:

```
npx tsx tools/audit/screen-geometry.ts            # every screen: collisions, overflows, unknown bindings
npx tsx tools/audit/screen-ascii.ts <screen-id>   # one screen, drawn at the device's real metrics
```

Both resolve their strings through the same code the simulator uses, so they measure what renders rather
than what was authored.

**Where each screen's requirement lives, and how this document stays true.** The agreed geometry for a
screen is a JSON file in `docs/Requirements/feature addition/screens/`, beside this prose — not in the web
tree, because it is a requirement rather than a build artefact, and it sits upstream of everything:

```
screens/<id>.json  ->  tools/skeleton/generate.mjs  ->  web/mockup/src/data/screens.json
                                                    ->  exporter  ->  src/ui/generated/GeneratedUi.*
```

Every ASCII block and element table below is **generated** from that JSON by
`npx tsx tools/audit/screen-spec.ts "<path>"`, and marked as generated. The first draft of this document
hand-maintained them beside the JSON and an audit found three mutually exclusive geometries for one element
— the same two-homes-for-one-fact defect this project keeps removing from its code, reproduced in its own
requirements. Regenerate; never hand-edit a generated block.

Worst-case widths come from each value's **physical bound**, declared per element in the JSON with its
justification. A printf field width is not a bound: `%7.2f` is a *minimum*, so a channel clamped to
`q_max = 65535 L/min` renders `65535.00` — eight characters, not seven.

## 2. The panel budget — the numbers that were missing

From `ui_renderer.cpp`, not from the mockup:

| Quantity | Value | Source |
| --- | --- | --- |
| Panel | 240 × 135 px | `web/mockup/src/utils/layout.ts` (`DISPLAY_WIDTH`/`DISPLAY_HEIGHT`) |
| Glyph width, `text` / `badge` / `scrollbar` | **6 px** | `ui_renderer.cpp:16` |
| Glyph width, `value` | **7 px** | `ui_renderer.cpp:17` |
| Glyph height, all kinds | 8 px | `ui_renderer.cpp:292`, as the badge box's text height |
| Badge box | text width + 3 px each side; 8 + 2×2 tall | `ui_renderer.cpp:285-293` |
| **Box** fallback when width is 0 | 40 × 12 px | `ui_renderer.cpp:314-315` — boxes only; an icon has no fallback |

Therefore:

- **40 characters** per row of `text`, from x = 0. From the usual x = 8 margin, **38**.
- **34 characters** per row of `value`.
- **17 rows** of 8 px; the last (y = 128…135) has 7 px of room, so it is clipped by **1 px**.
- A per-sensor reading in the device's own format — `%u: %6.2f %s` → `1:   2.34 L/s` — is 13 characters
  and therefore **91 px**. Two such columns plus labels and badges need 242 px on a 240 px panel, which is
  why every two-column sensor page collides. This arithmetic, not taste, is what forces their redesign.

**Rules for every screen from here on.**

1. An element must not overlap another element's box.
2. An `icon` is not decorative. `drawFlowDots` paints two discs of `radius = min(w,h)/4` inside its box —
   about 13% of a 120 × 40 area, not the whole of it — but discs are pixels, and text drawn across them is
   as broken as two overlapping labels. P0's own defect was exactly that.
3. **An `icon` must carry an `assetId`.** Dispatch is `if (element.assetId && strcmp(element.assetId,
   "flow-dots") == 0)` (`ui_renderer.cpp:324`), so an icon without one draws *nothing at all* — silently,
   and neither the mockup nor an ASCII render shows the absence.
4. A row's **worst case** must fit, taken from the value's physical bound rather than its format string.
5. An element's authored `content` must never contradict its `binding`: on the device the binding wins.
6. Only the footer hint may sit in y 116…134, the banner's row — see §2c.

**Character size is fixed.** The renderer selects one font globally — `display.setFont(&fonts::Font0)` with
`setTextSize(1)` (`ui_renderer.cpp:58`, :300) — so there is no per-element font and 6 px is the floor. The
only size lever available is the element kind: `value` draws at 7 px, `text` and `badge` at 6 px. A smaller
face would need a font attribute on the element, plus renderer, exporter, schema and mockup support; that is
deliberately not in scope.

## 2a. Units — flow is litres per minute, everywhere

**Decision:** every flow quantity is **litres per minute**. The panel writes `L/m`; MQTT and Home Assistant
write `L/min`, because HA validates the unit string against its own enumeration for `volume_flow_rate` and
`L/m` is not in it (`ha_discovery.cpp:269`). Same quantity, two spellings, one of them imposed externally.

This resolves a three-way split in which the same fact had two units and three conversion points:

| Surface | Was | Evidence |
| --- | --- | --- |
| Internal state | L/s | `SensorData::instantFlow_L_s`, `maxFlowSinceReset` |
| Display | L/s | `"%u: %6.2f %s"` with `L/s` |
| Modbus holding registers | L/s, **and documented as such** | `modbus_manager.cpp:338` writes `instantFlow_L_s`; `docs/Requirements/Project_document.md:169` publishes register 101 as "Instant Flow (L/s) … Liters per Second", :174 the same for 115 |
| MQTT | L/min | `flowLPerMin`, `maxFlowLPerMin` |
| Home Assistant | L/min | `ha_discovery.cpp:269` |
| `config.sensor.maxFlow` (q_max) | L/min | `ui_settings_types.cpp:56` |

Consequences worth stating plainly:

- The change **removes** conversions rather than adding them. `SensorStateEngine::update` already computes
  `flowLpm` and then divides by 60 to store L/s (`sensor_state_engine.cpp:27-33`), and the q_max clamp and
  the Nyquist limit are already evaluated in L/min. Storing L/min makes the engine, the setting and the
  limit share one unit.
- Today an operator sets `150 L/min` on a sensor and the panel reports its peak as `2.50 L/s`. That
  division-by-60 in the head is the user-visible symptom of the split.
- **Changing `OFF_INSTANT_FLOW` and `OFF_MAX_FLOW` is a breaking change for any Modbus master already
  integrated.** No hardware has shipped, so this is the cheapest moment it will ever be. The register units
  ARE documented — `Project_document.md:169` and :174 state Liters per Second explicitly — so that table is
  part of this change, not an afterthought. It is also the file an integrator reads.
- `RGB_LED_Behavior.md:57` specifies the blue LED against "aggregated instantaneous flow … exceeds 0.0 L/s",
  and `drawFlowDots` gates on `aggregateFlowLps > 0.001` (`ui_renderer.cpp:332`). Both are thresholds in the
  old unit and both need restating.
- Volumes are unaffected: litres and m³ throughout.

### 2a.1. The panel is a view; the wire is the record

**Decision:** the panel shows cubic metres only, at 2 decimals. Every wire surface carries **both** litres
and cubic metres, at full stored precision.

That asymmetry is the point, not a compromise. A 240 × 135 panel with 6–7 px glyphs is a human-readable
summary — it can afford to round, because nothing downstream reads it. Modbus and MQTT are the record, and
they already hold more than the panel can show: `setDouble` for cumulative litres *and* cumulative m³,
`setFloat` for both session variants (`modbus_manager.cpp:339-342`).

Consequences to hold onto:

- **The panel's resolution is 10 L** (0.01 m³). A 123 L reading shows as `0.12 m^3`. For a lifetime
  totalizer that is a summary; for a *session* being watched during commissioning, a 5 L bucket test reads
  `0.00` on the panel while the register holds `5.0`. See §5.2 for the session page's decimals.
- **The conversion needs exactly one implementation.** It currently has four — `firmware.cpp:954`,
  `ui_bindings.cpp:290` and `:296`, `modbus_manager.cpp:340` (into a double) and `:342` (into a float),
  each with a bare `1000` literal. Publishing both units everywhere doubles the number of places that
  division happens, so a single `litresToCubicMeters()` comes first.
- **MQTT is currently inconsistent with itself**: cumulative goes out as m³ (`totalCubicMeters`) and session
  as litres (`sessionLiters`) (`mqtt_publisher.h:124-128`), and Home Assistant mirrors that split
  (`ha_discovery.cpp:239-240`). Both-units-everywhere fixes it rather than adding to it.

## 2b. Aggregates are published, not re-derived

**Decision:** the four aggregate facts P0 displays get their own Modbus registers. The device already
computes all four for the panel and publishes none of them — `syncGlobalRegisters`
(`modbus_manager.cpp:358-380`) writes the polling rate, the connected bitmap, the reset commands and the LED
settings, and no aggregate at all, while `aggregateFlowLpsCache` (`modbus_manager.h:36`) holds one of them
already and never reaches a register.

Without this, every master re-derives them from the eight sensor blocks: summing for two, argmaxing for the
third, and guessing the tie-break for the fourth. That is one rule with as many implementations as there are
integrators, and they will disagree — the device samples at its own cadence and resolves ties by a rule
written down nowhere.

Global addresses 46-99 are free and existing groups are spaced by ten, so the block starts at 50:

| Register | Words | Type | Content | Unit |
| --- | --- | --- | --- | --- |
| 50 | 2 | float | Total current flow, summed over `inUse` sensors | L/min |
| 52 | 2 | float | Total session volume, summed over `inUse` sensors | L |
| 54 | 2 | float | Max flow since reset — the peak ONE sensor reached | L/min |
| 56 | 1 | uint16 | Which sensor holds that peak: **0 = none, 1..8** | — |

Two conventions that belong in `register_map.h` beside the addresses, not in a reader's head:

- **The sensor number is 1-based and 0 means none.** It matches the panel's `S1`..`S8` and `UiNavigator`'s
  own `sensorIndex_ = 0` sentinel. It also avoids spreading a hazard the tree already carries: the web
  portal names per-sensor fields `@0`..`@7` while `ui::readSetting` takes a 1-based index where 0 is
  invalid (`portal_form.h:165`), so one careless adapter drops sensor 1 and misroutes the rest.
- **Ties resolve to the lowest sensor number.** A master cannot reproduce a rule it was never told, and
  not having to reproduce the rule is the entire point of register 56.

Edge cases, matching what the panel shows:

| State | Reg 54 | Reg 56 | Panel |
| --- | --- | --- | --- |
| No sensor enabled | 0.0 | 0 | `Max Flow: --` |
| Enabled, nothing has flowed yet | 0.0 | 0 | `Max Flow: 0.00 L/m` |
| Peak on sensor 3 | 140.40 | 3 | `Max Flow:  140.40 L/m (S3)` |

**Deferred, pending context** — whether these four also become Home Assistant entities. They are cheap to
publish over MQTT, but `kHaMaxEntities = kNumSensors * 3 + 4` (`ha_discovery.h:120`) budgets exactly four
globals, so adding four more changes that constant and the discovery payload's size.

## 2c. The warning banner lives in the footer row  *(decided)*

`drawWarningBanner` paints `fillRect(0, bannerY, 240, 18)` whenever `context.hasWarnings` is set
(`ui_renderer.cpp:422-435`), followed by `!` at x=4 and the summary at x=16. **`bannerY` was 34**, which put
18 px of full-width overlay mid-panel — the one place every screen keeps its content — on all 79 screens, and
no requirement document mentioned it.

**Decision: `bannerY = 116`, so the banner covers the footer row.** The footer hint is the least valuable row
on any screen: a gesture reminder an operator reads once. A warning replacing it costs nothing while a warning
is live, and no screen has to reserve a band or nominate an element to sacrifice. It is a one-line firmware
change.

The case for moving rather than reserving is P1. With the banner at y=34, its second row sits at y=44, so a
per-sensor undersampling warning would **hide sensors 2 and 6 while naming a sensor** — and no arrangement of
eight sensors on four rows avoids that band.

What comes with the decision:

- **The banner inherits the footer's budget**: 40 characters at 6 px. `!` at x=4 and the summary at x=16 leave
  **37 characters**, which holds the widest summary the device can produce (`! S1,2,3,4,5,6,7,8` is 18).
- **Exactly one element per screen may sit in y 116…134** — the footer hint, marked `bannerReplaces` in the
  screen's JSON. Anything else there is reported as a collision, so this is not a blanket exemption.
- `level-position` shortens from 104 px to **100 px** (y 14…114) on every screen that carries it: at 104 it
  grazed the band by 2 px.

## 3. P0 — System Status  *(agreed)*

`info-p0-global-status`. The landing page from Idle, and the root of the info ring. Requirements entry:
"Global aggregate flow and volume | ENTER: no action | ENTER-long: escape (already at root)".

### 3.1. Layout

<!-- generated by tools/audit/screen-spec.ts info-p0-global-status.json — do not hand-edit -->

Worst case on every row, from the physical bound of each value rather than its format string:

```
     +----------------------------------------+   240 x 135 px = 40 cols x 17 rows
y   0 |System Status                           |
y   8 |                                        |
y  16 |Total Current Flow (L/m) 524280.00>>   ||
y  24 |                                       ||
y  32 |          ++++++++++++++++++++         ||
y  40 |          +                  +         ||
y  48 |          +                  +         ||
y  56 |          +                  +         ||
y  64 |          ++++++++++++++++++++         ||
y  72 |                                       ||
y  80 |Since reset: 999999.99 L               ||
y  88 |                                       ||
y  96 |Max Flow: 65535.00 L/m (S8)            ||
y 104 |                                       ||
y 112 |WiFi RETRY  MQTT OFF  LED 1p/100L       |
y 120 |                                        |
y 128 |UP/DN pages   UP+DN off                 |
     +----------------------------------------+
```

`>` marks a value's 7 px glyphs overhanging the 6 px grid. `·` marks two elements in one cell.

| Element | Kind | x, y | Binding | Worst case | Bound |
| --- | --- | --- | --- | --- | --- |
| `hdr-title` | text | 2, 2 | — | 13 ch = 78 px, x 2..80 | fixed literal |
| `total-flow-label` | text | 2, 14 | — | 24 ch = 144 px, x 2..146 | fixed literal |
| `total-flow-value` | value | 152, 14 | `telemetry.totalFlowLpm` | 9 ch = 63 px, x 152..215 | 8 channels each clamped to q_max=65535 L/min |
| `flow-dots` | icon | 60, 30 | — | 120 × 40 px | geometry; assetId required by ui_renderer.cpp:324 |
| `session-total` | text | 2, 80 | `telemetry.totalVolumeLiters` | 24 ch = 144 px, x 2..146 | float32 session litres, 6 integer digits |
| `max-flow` | text | 2, 92 | `telemetry.maxFlowLpm` | 27 ch = 162 px, x 2..164 | one channel clamped to q_max=65535 |
| `net-led-status` | text | 2, 108 | `legend.status` | 33 ch = 198 px, x 2..200 | wifiStateText max RETRY(5), mqttStateText max OFF(3), volume max 100 |
| `level-position` | scrollbar | 232, 14 | — | 5 × 100 px | geometry; 100px so it stops clear of the banner row at y=116 |
| `footer-hint` | text | 2, 124 | — | 23 ch = 138 px, x 2..140 | fixed literal |

Rows inked 17 of 17. Narrowest right margin 25 px.

> Accepted overlap: footer-hint is the row the banner replaces by design (§2c)

**No collisions, no overflow, every icon addressable, and 1 banner overlap(s) declared below.**

### 3.2. What each row means

- **Total Current Flow** — the live aggregate, summed over `inUse` sensors only
  (`sensor_state_engine.cpp:49`, inside the `if (sensor.inUse)` arm). A disconnected sensor contributes
  nothing, which is what makes switching one off visible here. The label is a 6 px `text` element and the
  number a 7 px `value` sharing its row: the label ends at x=146, the value runs 152..201, so the headline
  number is deliberately the largest glyphs on the screen.
- **Flow dots** — **four** dots in a chase, one lit at a time travelling left to right, wrapping. Today
  `drawFlowDots` paints **two** alternating dots with `radius = min(width, height) / 4`
  (`ui_renderer.cpp:329-336`); four is a firmware change of roughly twenty lines inside that one function,
  with the count as a constant beside the glyph widths. Geometry in the 120 × 40 box at (60, 30):
  `spacing = width / count = 30`, `radius = min(spacing, height) / 3 = 10`, centres at x = 75, 105, 135, 165.
  Zero flow stays a single red dot, in the leftmost position.
  - **Two defects to fix in the same change.** The inactive dot is painted `badgeBackgroundColor_`
    (`ui_renderer.cpp:341`, :347, :350) — the theme's *badge* fill, not the panel background
    (`ui_renderer.cpp:62`, :66) — so an "off" dot renders as a visible disc in the wrong colour, and P0 has no
    badges at all. And the two colours are hardcoded RGB565 literals, `0xF800` red and `0x001F` blue, so a
    theme change moves every element except these.
  - **Two consequences of §2a, both to fix with the unit change.** The animation rate comes from
    `aggregateFlowLps` clamped to 0.1..10.0 with `periodMs = 1000 / flow` (`ui_renderer.cpp:345-349`), and the
    active/inactive test is `aggregateFlowLps > 0.001` (:332). Both constants are calibrated for litres per
    SECOND: fed litres per minute, every non-trivial flow saturates the clamp and the dots run at a fixed
    maximum, conveying nothing.
  - **And the animation is unobservable regardless of unit until the repaint cadence is addressed.** The
    renderer redraws on a ~1 s cadence, so a phase computed from `millis()` at up to 10 Hz is sampled once a
    second: the dots change position between frames, but nothing walks. Four dots in a chase need either a
    faster repaint for this element or an explicit per-frame step; specifying the geometry does not make it
    animate.
- **Since reset** — `totalSessionLiters`, likewise summed over `inUse` sensors. This is **session** volume,
  cleared by the session reset on P4/P5/P6 — not the lifetime cumulative total. The old label said "Total",
  which is why the number looked wrong for what it is; the quantity was always the one wanted.
- **Max Flow** — the highest peak any single sensor has registered since reset, **with the sensor that
  registered it**. It is the largest of the eight `maxFlowSinceReset` values, not a peak of the aggregate:
  the value shown is one sensor's, and `(S3)` says whose.
  - Ties resolve to the **lowest** sensor number.
  - No sensor enabled → `Max Flow: --`.
  - Enabled but every peak still 0 → `Max Flow: 0.00 L/m`, with no sensor tag: nothing has flowed, so no
    sensor owns the peak.
- **WiFi / MQTT / LED** — one row, deliberately: none of it is vital, and combining them frees a row. It is a
  `text` element at 6 px rather than a `value` at 7 px, which is the only size reduction the renderer offers.
  - The real state strings, from `wifiStateText` (`wifi_manager.cpp`): `OFF`, `CONN`, `OK`, `RETRY`, `AP`,
    `FAIL`, `?` — longest **`RETRY`**, five characters. `AuthF` is not a string the device can produce.
    `mqttStateText` emits `OK` or `OFF`. So the worst case is `WiFi RETRY  MQTT OFF  LED 1p/100L`.
  - The LED half is the red pulse volume from `config.ledPulseVolume` (1 / 10 / 100 L), phrased as one pulse
    per N litres — `1p/10L`. The pulse **period** is deliberately absent: it is a blink duration, unlabelled
    it reads as meaningless, and it stays editable and labelled on C6.
  - Worst case `WiFi AuthF  MQTT OK  LED 1p/100L` = 32 ch = 192 px.

### 3.3. Navigation

| Gesture | Result |
| --- | --- |
| DOWN | P1 `info-p1-instant-flow` |
| UP | previous page in the ring |
| ENTER short | nothing (§4.3: no action) |
| ENTER long | escape — already at root, so a no-op |
| UP+DOWN tapped | display off, nav stack cleared, wakes on P0 |

**Open defect, not P0's to fix:** §4.1's state machine describes a **9-page** ring (`P0 --> P8: UP`), but the
real ring is **11** — `net-wifi-root` and `net-mqtt-root` were appended when the WiFi/MQTT feature landed
and §4.1 was never updated. P0's UP therefore lands on `net-mqtt-root`. Either the diagram or the ring is
wrong; that decision belongs with the ring as a whole, not with this screen.

### 3.4. Changes this requires

**Dataset** — take the geometry from `screens/info-p0-global-status.json`; it is the requirement and §3.1 is
generated from it. In prose: relabel `total-flow-label`; `total-flow-value` becomes `kind: "value"` bound to
`telemetry.totalFlowLpm`; add `session-total` and `max-flow`; give `flow-dots` its **`assetId: "flow-dots"`**
without which it draws nothing (§2 rule 3); **replace `net-status` and `legend-led` with one `net-led-status`
row**, which also **deletes `legend-led`'s authored content** (`LED: Red=Pulse Grn=Ready Blu=Flow`) that its
binding has always overridden; shorten the footer hint to `UP/DN pages   UP+DN off`. Dataset edits go through
`tools/skeleton/generate.mjs`, which CI diffs.

Note that removing `legend.led`'s only binding leaves a catalogue value with no user. That is safe for the
value-coverage gate, which fails only on the reverse — an element binding an id the manifest lacks — but it
means `legend.led` should either be deleted from the catalogue in the same change or documented as retained
deliberately.

**Firmware formats** — `telemetry.totalFlowLpm`: `%7.2f`, so four digits and two decimals are guaranteed
rather than hoped for (`%.2f` was unbounded). `legend.status` replaces `legend.led`'s
`"LED: %uL pulses | %ums"` with `"WiFi %s  MQTT %s  LED 1p/%uL"`, folding in what `net.status` used to
render on its own row.

**New firmware values** — **three**, not two. The catalogue has `telemetry.totalFlowLps`; under §2a the
L/m-valued binding is a new id:
- `telemetry.totalFlowLpm` — the aggregate in litres per minute.
- `telemetry.maxFlowLpm` — an argmax over `SensorSnapshot::maxFlow` (`ui_controller.h:24`), which the render
  context already holds. No new stored state, no new register, no new topic: the per-sensor peak already
  exists in RAM (`sensor_types.h:26`), on Modbus (`OFF_MAX_FLOW = 15`) and over MQTT (`firmware.cpp:955`).
  One format cannot serve its three states, so the resolver arm must branch: `Max Flow: --` (nothing
  enabled), `Max Flow: 0.00 L/m` (enabled, no peak yet), `Max Flow: %7.2f L/m (S%u)` (a peak with an owner).
- `legend.status` — the combined network-and-LED row.

Each needs a `kSimpleValues` entry, a resolver arm and a manifest regeneration
(`Water-Flow-Meter-PlatformIO/tools/manifest_gen/run.sh`, then commit the result; `--check` is what CI runs).
Adding values needs no catalogue ABI bump — only a removal or rename does.

**A gap that would let this ship broken:** nothing hard-fails a catalogue value that has no resolver arm. The
exporter's `firmware-manifest-resolvable` check returns `warning`, and only `fail` escalates — so a value can
be advertised, bound by an element, and render blank on hardware while every gate passes. Add the arms and
the tests in the same change, and treat that warning as fatal for these three.

**Unit change** — every flow format moves to L/m per §2a, which for P0 means the current-flow value and the
max-flow row. Volumes are untouched.

**Modbus** — the aggregates block of §2b, which is where P0's four numbers come from on the wire. The
display resolver and the register writer must read the same computation rather than each doing their own.

## 4. P1 — Instant Flow  *(agreed)*

`info-p1-instant-flow`. Requirements entry: "Instant Flow | Holding registers 101… (`instantFlow_L_s`) |
ENTER: no action | ENTER-long: escape".

### 4.1. The finding that determines the layout

The authored row carries **three elements for two facts**. The per-sensor value's own format already encodes
the sensor number *and* the status, because `resolveSensorMetric` returns the status word in place of the
reading (`ui_bindings.cpp:266-279`):

| Sensor state | `sensor.N.instantFlow` renders | `sensor.N.status` badge renders |
| --- | --- | --- |
| in service, calibrated | `3:  140.40 L/m` | `OK` |
| in service, not calibrated | `3: SET?` | `WAIT` |
| not in service | `3: --` | `--` |

So `S3` restates the value's own `3:` prefix, and the badge restates the value's own status word in every
state — a number *means* OK. Across P1–P6 that is **48 redundant labels and 48 redundant badges**. Removing
both leaves one self-describing element per sensor, and that is what makes two columns fit.

### 4.2. Layout

<!-- generated by tools/audit/screen-spec.ts info-p1-instant-flow.json — do not hand-edit -->

Worst case on every row, from the physical bound of each value rather than its format string:

```
     +----------------------------------------+   240 x 135 px = 40 cols x 17 rows
y   0 |Instant Flow (L/m)                      |
y   8 |                                        |
y  16 |                                       ||
y  24 |1: 65535.00>>      5: 65535.00>>       ||
y  32 |                                       ||
y  40 |                                       ||
y  48 |2: 65535.00>>      6: 65535.00>>       ||
y  56 |                                       ||
y  64 |3: 65535.00>>      7: 65535.00>>       ||
y  72 |                                       ||
y  80 |                                       ||
y  88 |4: 65535.00>>      8: 65535.00>>       ||
y  96 |                                       ||
y 104 |                                       ||
y 112 |                                        |
y 120 |                                        |
y 128 |UP/DN pages   UP+DN off                 |
     +----------------------------------------+
```

`>` marks a value's 7 px glyphs overhanging the 6 px grid. `·` marks two elements in one cell.

| Element | Kind | x, y | Binding | Worst case | Bound |
| --- | --- | --- | --- | --- | --- |
| `hdr-title` | text | 2, 2 | — | 18 ch = 108 px, x 2..110 | fixed literal; the unit lives here, not on eight rows |
| `s1-value` | value | 2, 24 | `sensor.1.instantFlow` | 11 ch = 77 px, x 2..79 | clamped to q_max = 65535 L/min by the state engine |
| `s2-value` | value | 2, 44 | `sensor.2.instantFlow` | 11 ch = 77 px, x 2..79 | clamped to q_max = 65535 L/min by the state engine |
| `s3-value` | value | 2, 64 | `sensor.3.instantFlow` | 11 ch = 77 px, x 2..79 | clamped to q_max = 65535 L/min by the state engine |
| `s4-value` | value | 2, 84 | `sensor.4.instantFlow` | 11 ch = 77 px, x 2..79 | clamped to q_max = 65535 L/min by the state engine |
| `s5-value` | value | 114, 24 | `sensor.5.instantFlow` | 11 ch = 77 px, x 114..191 | clamped to q_max = 65535 L/min by the state engine |
| `s6-value` | value | 114, 44 | `sensor.6.instantFlow` | 11 ch = 77 px, x 114..191 | clamped to q_max = 65535 L/min by the state engine |
| `s7-value` | value | 114, 64 | `sensor.7.instantFlow` | 11 ch = 77 px, x 114..191 | clamped to q_max = 65535 L/min by the state engine |
| `s8-value` | value | 114, 84 | `sensor.8.instantFlow` | 11 ch = 77 px, x 114..191 | clamped to q_max = 65535 L/min by the state engine |
| `footer-hint` | text | 2, 124 | — | 23 ch = 138 px, x 2..140 | fixed literal |
| `level-position` | scrollbar | 232, 14 | — | 5 × 100 px | geometry; 100px so it stops clear of the banner row at y=116 |

Rows inked 17 of 17. Narrowest right margin 49 px.

> Accepted overlap: footer-hint is the row the banner replaces by design (§2c)

**No collisions, no overflow, every icon addressable, and 1 banner overlap(s) declared below.**

Columns at x 2 and x 114 with four rows at 20 px pitch — **the grid every telemetry page shares**, so paging
moves values in place rather than relaying them out. Chosen over one column of eight because the panel is
landscape, and 12 px pitch reads worse at distance than 20 px.

### 4.3. Format change, shared by every telemetry page

The per-sensor format becomes **`%u: %7.2f %s`** (was `%6.2f`). Under §2a a single channel can reach
`9999.99 L/m`, which `%6.2f` cannot hold. This is the format shared by all six telemetry pages, so it lands
on P2–P6 at the same time and their rows all become 14 characters wide.

**The unit lives in the header, on every telemetry page.** Per-row units were the first choice here, on the
argument that a bare number gets misread — but they are impossible on the volume pages, where
`8: 99999999.99 m^3` is 18 characters and overflows two columns by 19 px. Rather than let two pages disagree
with the other two for a purely arithmetic reason, the unit sits once in the title, which is two rows above
and always visible. On the flow pages that also frees 28 px per row.

### 4.4. `SET?` replaces `WAIT`, and why the state model changes

`WAIT` implies warming up. The real condition is "this channel has no valid calibration yet" — `q_max` or
`f_multiplier` still zero. `SET?` says that; `WAIT` does not.

Behind it sits a redundancy in the firmware's own model, confirmed from source:

- **`inUse` / "connected" detects nothing.** It is bit *n* of the persisted `connectedSensorsBitmap`, set by
  the operator. No hardware presence detection exists, and none is possible: a passive pulse sensor that is
  idle is indistinguishable from one whose wire has fallen off. The panel must therefore not imply
  detection — `--` means "not in service", not "not detected".
- **`isReady` is a cache of a pure predicate.** Every write of a true value is literally
  `isReady = configIsValid(candidate)` (`modbus_manager.cpp:287`, :300, :313); the only other writes are
  false at boot (`firmware.cpp:688`) and false on disable (`modbus_manager.cpp:105`). The bit carries nothing
  the stored configuration does not.
- **That cache is already broken.** Boot restores the configuration from NVS and forces `isReady = false`,
  and only a configuration *write* recomputes it — so after any reboot a calibrated, enabled channel reports
  not-ready indefinitely: no flow computed, registers zero, panel showing `WAIT`. The engine hints at the
  distrust itself, testing `sensor.isReady && config.f_multiplier != 0.0f`
  (`sensor_state_engine.cpp:26`) — belt and braces around a bit it cannot rely on.

**Proposed model:** `enabled` stays as the one persisted bit; `calibrated` becomes derived,
`configIsValid(config)`, never stored. Two flags that can disagree become one flag and one predicate, and
the reboot defect disappears with the cache. The four display states are then exactly those in §4.1 plus
`3:    0.00 L/m` for a calibrated channel with no pulses — which, per the point above, is also what a fallen
wire looks like, and the device cannot say otherwise.

**Status: the trace is complete and the defect is CONFIRMED — and it is data loss, not a display fault.**

- `firmware.cpp:681-682` promises "let the state engine decide readiness from the restored config rather than
  forcing everything off", then `:688` writes `isReady = false` and nothing recomputes it. The engine only
  reads the bit; `refreshDiagnostics` does not touch it.
- Pulses are still counted — the polling mask comes from `inUse` (`firmware.cpp:617-618`) — and then
  **discarded**: `sensor.pulseCount = 0` (`sensor_state_engine.cpp:24`) runs BEFORE the `isReady` gate at
  `:26`, and the else-branch only zeroes flow, so `cumulativeLiters += litersInterval` never executes.
- The register block is gated on `inUse && isReady` (`modbus_manager.cpp:337`), so the else-branch publishes
  `setDouble(OFF_CUMULATIVE_LITERS, 0.0)` — **a Modbus master reads the lifetime total as zero.**
- The NVS copy survives, because the periodic writer only saves when the value changes and it never moves
  (`firmware.cpp:741`). So deriving `calibrated` recovers the true total rather than a lost one.
- `evaluateSensorDiagnostics` already computes `configIsValid(cfg)` (`modbus_manager.cpp:393`) and never
  stores it back. The recompute is nearly there.
- No host test covers boot restore; `test/host/sensor_state_test.cpp:90` sets `isReady` by hand.

## 5. P2 — Cumulative Volume, and P3 — Session Volume  *(agreed)*

### 5.1. Two pages absorbed

The ring carried **four** volume pages: cumulative litres, cumulative m³, session litres, session m³ — two
quantities in two units, with the m³ form derived from the litres one by `/1000`
(`ui_bindings.cpp:290`, `:296`). The device stored one fact and paginated it twice.

Because 1 L is exactly 0.001 m³, a m³ page carries the litres reading too: the decimal point moves and
nothing is lost that the panel could have shown anyway. So **cumulative litres and cumulative m³ become one
page, and session litres and session m³ become one page.** The ring drops from 11 pages to 9.

This is the same duplication removed at §4.1 (a label restating the value's prefix) and §4.1 again (a badge
restating its status word), one level up: two whole pages restating each other's number ÷ 1000.

Both litres and m³ remain on **every wire surface** — see §2a.1. Nothing is lost to an integrator; only the
panel picks one.

### 5.2. Bound and resolution

**8 integer digits and 2 decimals**, so the widest row is `8: 99999999.99`:

- maximum `99,999,999.99 m³` = 99,999,999,990 L — **1,268 years** at 150 L/min continuous, so the counter
  cannot realistically overflow. The value reset therefore exists for operational reasons (a new billing
  period, a sensor swap), not for wrap-around.
- resolution `0.01 m³` = **10 L**.

The unit is in the header, not on the rows: at 4 characters × 8 rows it would cost 224 px to restate one
fact, and it would push the row to 18 characters — which overflows two columns by 19 px.

**Open on the session page:** at 10 L resolution a 5 L bucket test reads `0.00` while the register holds
`5.0`. Three decimals would give 1 L resolution for one more character (`8: 99999999.999` = 15 ch = 105 px,
still fits), at the cost of the two pages no longer being dimensionally identical.

### 5.3. The reset each page reaches — and what it does NOT touch

The owner's requirement was that a value reset must not disturb calibration. That separation already exists
in the register map, and each page reaches the right scope:

| Command | Clears | Touches calibration? |
| --- | --- | --- |
| `REG_MASTER_RESET_ALL_MEASURED` (21) | `sessionLiters`, `cumulativeLiters`, `maxFlowSinceReset` on every in-use channel, then persists to NVS (`modbus_manager.cpp:175-189`) | **No** |
| `REG_MASTER_RESET_ALL_SESSION` (22) | session values only | **No** |
| `OFF_CMD_RESET_SESSION` / `OFF_CMD_RESET_ALL` (17 / 18) | the same, one channel | **No** |
| `OFF_CMD_RESET_CONFIG` (19) | that channel's `q_max` / `f_multiplier` / `adjust` | **Yes — deliberately separate** |
| Factory reset (P8) | NVS wholesale, including link and network configuration | Yes |

P2's ENTER opens `confirm-reset-totals`; P3's opens the session equivalent. What the spec adds is the
statement that these are the *same operations* as registers 21 and 22 — nothing documented which scope a
screen's reset had.

### 5.4. P2 layout

<!-- generated by tools/audit/screen-spec.ts info-p2-cumulative-m3.json — do not hand-edit -->

Worst case on every row, from the physical bound of each value rather than its format string:

```
     +----------------------------------------+   240 x 135 px = 40 cols x 17 rows
y   0 |Cumulative (m3)                         |
y   8 |                                        |
y  16 |                                       ||
y  24 |1: 99999999.99>>   5: 99999999.99>>    ||
y  32 |                                       ||
y  40 |                                       ||
y  48 |2: 99999999.99>>   6: 99999999.99>>    ||
y  56 |                                       ||
y  64 |3: 99999999.99>>   7: 99999999.99>>    ||
y  72 |                                       ||
y  80 |                                       ||
y  88 |4: 99999999.99>>   8: 99999999.99>>    ||
y  96 |                                       ||
y 104 |                                       ||
y 112 |                                        |
y 120 |                                        |
y 128 |ENTER reset totals (hold 3s)            |
     +----------------------------------------+
```

`>` marks a value's 7 px glyphs overhanging the 6 px grid. `·` marks two elements in one cell.

| Element | Kind | x, y | Binding | Worst case | Bound |
| --- | --- | --- | --- | --- | --- |
| `hdr-title` | text | 2, 2 | — | 15 ch = 90 px, x 2..92 | fixed literal; the unit lives here, not on eight rows |
| `s1-value` | value | 2, 24 | `sensor.1.cumulativeM3` | 14 ch = 98 px, x 2..100 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `s2-value` | value | 2, 44 | `sensor.2.cumulativeM3` | 14 ch = 98 px, x 2..100 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `s3-value` | value | 2, 64 | `sensor.3.cumulativeM3` | 14 ch = 98 px, x 2..100 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `s4-value` | value | 2, 84 | `sensor.4.cumulativeM3` | 14 ch = 98 px, x 2..100 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `s5-value` | value | 114, 24 | `sensor.5.cumulativeM3` | 14 ch = 98 px, x 114..212 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `s6-value` | value | 114, 44 | `sensor.6.cumulativeM3` | 14 ch = 98 px, x 114..212 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `s7-value` | value | 114, 64 | `sensor.7.cumulativeM3` | 14 ch = 98 px, x 114..212 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `s8-value` | value | 114, 84 | `sensor.8.cumulativeM3` | 14 ch = 98 px, x 114..212 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `footer-hint` | text | 2, 124 | — | 28 ch = 168 px, x 2..170 | fixed literal |
| `level-position` | scrollbar | 232, 14 | — | 5 × 100 px | geometry; 100px so it stops clear of the banner row at y=116 |

Rows inked 17 of 17. Narrowest right margin 28 px.

> Accepted overlap: footer-hint is the row the banner replaces by design (§2c)

**No collisions, no overflow, every icon addressable, and 1 banner overlap(s) declared below.**

### 5.5. P3 layout

<!-- generated by tools/audit/screen-spec.ts info-p3-session-m3.json — do not hand-edit -->

Worst case on every row, from the physical bound of each value rather than its format string:

```
     +----------------------------------------+   240 x 135 px = 40 cols x 17 rows
y   0 |Session (m3)                            |
y   8 |                                        |
y  16 |                                       ||
y  24 |1: 99999999.99>>   5: 99999999.99>>    ||
y  32 |                                       ||
y  40 |                                       ||
y  48 |2: 99999999.99>>   6: 99999999.99>>    ||
y  56 |                                       ||
y  64 |3: 99999999.99>>   7: 99999999.99>>    ||
y  72 |                                       ||
y  80 |                                       ||
y  88 |4: 99999999.99>>   8: 99999999.99>>    ||
y  96 |                                       ||
y 104 |                                       ||
y 112 |                                        |
y 120 |                                        |
y 128 |ENTER reset session (hold 3s)           |
     +----------------------------------------+
```

`>` marks a value's 7 px glyphs overhanging the 6 px grid. `·` marks two elements in one cell.

| Element | Kind | x, y | Binding | Worst case | Bound |
| --- | --- | --- | --- | --- | --- |
| `hdr-title` | text | 2, 2 | — | 12 ch = 72 px, x 2..74 | fixed literal; the unit lives here, not on eight rows |
| `s1-value` | value | 2, 24 | `sensor.1.sessionM3` | 14 ch = 98 px, x 2..100 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `s2-value` | value | 2, 44 | `sensor.2.sessionM3` | 14 ch = 98 px, x 2..100 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `s3-value` | value | 2, 64 | `sensor.3.sessionM3` | 14 ch = 98 px, x 2..100 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `s4-value` | value | 2, 84 | `sensor.4.sessionM3` | 14 ch = 98 px, x 2..100 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `s5-value` | value | 114, 24 | `sensor.5.sessionM3` | 14 ch = 98 px, x 114..212 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `s6-value` | value | 114, 44 | `sensor.6.sessionM3` | 14 ch = 98 px, x 114..212 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `s7-value` | value | 114, 64 | `sensor.7.sessionM3` | 14 ch = 98 px, x 114..212 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `s8-value` | value | 114, 84 | `sensor.8.sessionM3` | 14 ch = 98 px, x 114..212 | display maximum 8 integer digits + 2 decimals of m3 (0.01 m3 = 10 L resolution) |
| `footer-hint` | text | 2, 124 | — | 29 ch = 174 px, x 2..176 | fixed literal |
| `level-position` | scrollbar | 232, 14 | — | 5 × 100 px | geometry; 100px so it stops clear of the banner row at y=116 |

Rows inked 17 of 17. Narrowest right margin 28 px.

> Accepted overlap: footer-hint is the row the banner replaces by design (§2c)

**No collisions, no overflow, every icon addressable, and 1 banner overlap(s) declared below.**

Both on P1's grid — columns at x 2 and 114, four rows at 20 px pitch — so all telemetry pages share one
layout and paging moves values in place instead of relaying them out.

### 5.6. Defects in the authored pages, beyond the eight collisions each

- **`divider-1` is a `box` at (0, 65) with no width or height**, so it takes the 40 × 12 fallback
  (`ui_renderer.cpp:314-315`) and paints a small filled rectangle at the middle-left of the panel. A
  full-width divider was intended.
- **`undersampling-badge` at (176, 4) duplicates the banner.** It binds `diagnostics.undersampling`, which
  renders `OK` or `! S1,3` — exactly what the banner now reports from the footer row (§2c). It also reads
  `OK` permanently today, because nothing can set the bit.

## 5a. P4 — Max Flow Since Reset  *(agreed)*

`info-p4-max-flow`. Requirements entry: "Max Flow Since Reset | Holding register 115… | ENTER: open
`Reset session?` | ENTER-long: escape".

### 5a.1. What the page is FOR

The owner's purpose, which is sharper than "see the peaks": **it tells you whether a sensor is maxing out, and
therefore under-dimensioned for the pipe it is installed on.**

That is derivable rather than a judgement call, because the state engine clamps to the ceiling by assignment:

```
if (flowRateLpm > config.q_max) {
  flowRateLpm = config.q_max;          // sensor_state_engine.cpp:32-33
}
```

So `maxFlowSinceReset == q_max` means this channel reached its ceiling at least once — the sensor is too small
for the flow the line actually carries. The page marks those rows **`MAX`**, so the operator does not have to
remember that S2's q_max is 150 and S8's is 96.2.

Three properties of the marker:

- **Derived, never stored.** It is a comparison, not a bit. Same lesson as §4.4: do not cache what a comparison
  answers.
- **It depends on §2a.** Flow is stored in L/s today and the ceiling in L/min, so the round-trip
  `L/min → ÷60 → L/s → ×60` can miss exact equality. Storing L/min makes `maxFlowLpm == qMaxLpm` bit-exact, so
  the marker and the unit change ship together.
- **It is a different fault from undersampling.** `MAX` means the *sensor* is too small for the flow; the
  Nyquist warning means the *polling rate* is too slow for the sensor. Distinct causes, and the vocabulary must
  not conflate them.

`q_max` itself is not printed per row: `8: 65535.00/65535 MAX` is 21 characters and overflows two columns by
61 px. The ceiling is on the sensor's own settings page, and the marker is what the operator needs at a glance.

### 5a.2. When the peak resets — and why volatility is accepted

Every site that clears `maxFlowSinceReset`:

| Trigger | Scope | Also clears |
| --- | --- | --- |
| `OFF_CMD_RESET_SESSION` (17) | one channel | `sessionLiters` |
| `OFF_CMD_RESET_ALL` (18) | one channel | session + cumulative |
| `REG_MASTER_RESET_ALL_SESSION` (22) | all in-use | `sessionLiters` |
| `REG_MASTER_RESET_ALL_MEASURED` (21) | all in-use | session + cumulative |
| `REG_CONNECTED_SENSORS_BITMAP` (10) | the toggled channel | session + cumulative + **calibration** |
| **any reboot** | all channels | — the peak is never persisted to NVS |

So "since reset" means **since the last session reset or the last power-on**. The owner has accepted the
volatility deliberately: the peak is an observation window, not a service record, and a `MAX` marker earned
during the current run is what matters for sizing. No NVS write is therefore added.

One consequence to state rather than discover: `MAX` says the ceiling was reached, not how often or for how
long. A valve slam and a chronically undersized pipe look identical. Distinguishing them would need a clip
counter or a time-at-ceiling accumulator — new state, deliberately not proposed.

### 5a.3. Layout

<!-- generated by tools/audit/screen-spec.ts info-p4-max-flow.json — do not hand-edit -->

Worst case on every row, from the physical bound of each value rather than its format string:

```
     +----------------------------------------+   240 x 135 px = 40 cols x 17 rows
y   0 |Max Flow (L/m)                          |
y   8 |                                        |
y  16 |                                       ||
y  24 |1: 65535.00 MAX>>> 5: 65535.00 MAX>>>  ||
y  32 |                                       ||
y  40 |                                       ||
y  48 |2: 65535.00 MAX>>> 6: 65535.00 MAX>>>  ||
y  56 |                                       ||
y  64 |3: 65535.00 MAX>>> 7: 65535.00 MAX>>>  ||
y  72 |                                       ||
y  80 |                                       ||
y  88 |4: 65535.00 MAX>>> 8: 65535.00 MAX>>>  ||
y  96 |                                       ||
y 104 |                                       ||
y 112 |                                        |
y 120 |                                        |
y 128 |MAX = at sensor ceiling                 |
     +----------------------------------------+
```

`>` marks a value's 7 px glyphs overhanging the 6 px grid. `·` marks two elements in one cell.

| Element | Kind | x, y | Binding | Worst case | Bound |
| --- | --- | --- | --- | --- | --- |
| `hdr-title` | text | 2, 2 | — | 14 ch = 84 px, x 2..86 | fixed literal; the unit lives here, not on eight rows |
| `s1-value` | value | 2, 24 | `sensor.1.maxFlowSinceReset` | 15 ch = 105 px, x 2..107 | q_max = 65535 L/min is the clamp ceiling; MAX appears when the peak reached it |
| `s2-value` | value | 2, 44 | `sensor.2.maxFlowSinceReset` | 15 ch = 105 px, x 2..107 | q_max = 65535 L/min is the clamp ceiling; MAX appears when the peak reached it |
| `s3-value` | value | 2, 64 | `sensor.3.maxFlowSinceReset` | 15 ch = 105 px, x 2..107 | q_max = 65535 L/min is the clamp ceiling; MAX appears when the peak reached it |
| `s4-value` | value | 2, 84 | `sensor.4.maxFlowSinceReset` | 15 ch = 105 px, x 2..107 | q_max = 65535 L/min is the clamp ceiling; MAX appears when the peak reached it |
| `s5-value` | value | 114, 24 | `sensor.5.maxFlowSinceReset` | 15 ch = 105 px, x 114..219 | q_max = 65535 L/min is the clamp ceiling; MAX appears when the peak reached it |
| `s6-value` | value | 114, 44 | `sensor.6.maxFlowSinceReset` | 15 ch = 105 px, x 114..219 | q_max = 65535 L/min is the clamp ceiling; MAX appears when the peak reached it |
| `s7-value` | value | 114, 64 | `sensor.7.maxFlowSinceReset` | 15 ch = 105 px, x 114..219 | q_max = 65535 L/min is the clamp ceiling; MAX appears when the peak reached it |
| `s8-value` | value | 114, 84 | `sensor.8.maxFlowSinceReset` | 15 ch = 105 px, x 114..219 | q_max = 65535 L/min is the clamp ceiling; MAX appears when the peak reached it |
| `footer-hint` | text | 2, 124 | — | 23 ch = 138 px, x 2..140 | fixed literal; explains the marker rather than repeating the gestures |
| `level-position` | scrollbar | 232, 14 | — | 5 × 100 px | geometry; 100px so it stops clear of the banner row at y=116 |

Rows inked 17 of 17. Narrowest right margin 21 px.

> Accepted overlap: footer-hint is the row the banner replaces by design (§2c)

**No collisions, no overflow, every icon addressable, and 1 banner overlap(s) declared below.**

The footer earns its row here by explaining the marker instead of repeating the gestures every page carries.

## 5b. P5 — Enter Configuration, and P6 — Factory Reset  *(agreed)*

Both are text prompts per §4.3, and both were geometrically clean. Their defects were in what they said.

### 5b.1. The factory-reset warning was materially untrue

The authored warning read `Erases all totals, sensor config and LED settings.` The reset is
`preferences.clear()` (`firmware.cpp:462`) over the single `"flow-data"` namespace opened at `:653` — and
`loadNetSettings`/`saveNetSettings` take **that same `Preferences` object** (`firmware.cpp:1154`, `:981`).

So a factory reset also erases **the WiFi SSID and PSK, the MQTT broker, its credentials and the base topic**.
The warning predates the WiFi/MQTT feature and was never updated. With on-device text entry removed (§6.3 of
`Display_UI_Requirements`), re-provisioning then requires the AP portal — someone physically at the device. An
operator clearing calibration would have silently lost the network provisioning.

The new warning names all of it, and says what recovery costs.

### 5b.2. Three smaller defects, all in the same family

- **`P8 FACTORY RESET` embedded its page number in its title** — which the renumbering to P6 invalidates, and
  which no other screen does. Titles do not carry page numbers.
- **P5's footer duplicated its own body**: `Press ENTER to open Configuration.` above
  `ENTER opens Configuration`. The body now says what Configuration *contains*; the footer says the gesture.
- **P5's `divider-1` sat at y 120…121** — inside the banner row §2c claims, and the only decorative divider on
  any prompt screen. Removed.

Both screens also used x=8 / x=10 left margins where all five telemetry pages use x=2; they now match.

### 5b.3. P5 layout

<!-- generated by tools/audit/screen-spec.ts info-p5-enter-config.json — do not hand-edit -->

Worst case on every row, from the physical bound of each value rather than its format string:

```
     +----------------------------------------+   240 x 135 px = 40 cols x 17 rows
y   0 |Configuration                           |
y   8 |                                        |
y  16 |                                       ||
y  24 |                                       ||
y  32 |Sensor calibration, Modbus             ||
y  40 |link, LED pulses, network.             ||
y  48 |                                       ||
y  56 |                                       ||
y  64 |                                       ||
y  72 |                                       ||
y  80 |                                       ||
y  88 |                                       ||
y  96 |                                       ||
y 104 |                                       ||
y 112 |                                        |
y 120 |                                        |
y 128 |ENTER opens Configuration               |
     +----------------------------------------+
```

`>` marks a value's 7 px glyphs overhanging the 6 px grid. `·` marks two elements in one cell.

| Element | Kind | x, y | Binding | Worst case | Bound |
| --- | --- | --- | --- | --- | --- |
| `hdr-title` | text | 2, 2 | — | 13 ch = 78 px, x 2..80 | fixed literal |
| `body-1` | text | 2, 28 | — | 26 ch = 156 px, x 2..158 | fixed literal |
| `body-2` | text | 2, 40 | — | 26 ch = 156 px, x 2..158 | fixed literal |
| `footer-hint` | text | 2, 124 | — | 25 ch = 150 px, x 2..152 | fixed literal |
| `level-position` | scrollbar | 232, 14 | — | 5 × 100 px | geometry; 100px so it stops clear of the banner row at y=116 |

Rows inked 17 of 17. Narrowest right margin 82 px.

> Accepted overlap: footer-hint is the row the banner replaces by design (§2c)

**No collisions, no overflow, every icon addressable, and 1 banner overlap(s) declared below.**

### 5b.4. P6 layout

<!-- generated by tools/audit/screen-spec.ts info-p6-factory-reset.json — do not hand-edit -->

Worst case on every row, from the physical bound of each value rather than its format string:

```
     +----------------------------------------+   240 x 135 px = 40 cols x 17 rows
y   0 |Factory Reset                           |
y   8 |                                        |
y  16 |                                       ||
y  24 |                                       ||
y  32 |Erases totals, calibration,            ||
y  40 |LED, Modbus link and WiFi              ||
y  48 |                                       ||
y  56 |and MQTT credentials.                  ||
y  64 |                                       ||
y  72 |Re-provisioning needs the AP           ||
y  80 |                                       ||
y  88 |portal, at the device.                 ||
y  96 |                                       ||
y 104 |                                       ||
y 112 |                                        |
y 120 |                                        |
y 128 |ENTER opens confirm screen              |
     +----------------------------------------+
```

`>` marks a value's 7 px glyphs overhanging the 6 px grid. `·` marks two elements in one cell.

| Element | Kind | x, y | Binding | Worst case | Bound |
| --- | --- | --- | --- | --- | --- |
| `hdr-title` | text | 2, 2 | — | 13 ch = 78 px, x 2..80 | fixed literal; no page number — renumbering must not invalidate a title |
| `warning-1` | text | 2, 28 | — | 27 ch = 162 px, x 2..164 | fixed literal |
| `warning-2` | text | 2, 40 | — | 25 ch = 150 px, x 2..152 | fixed literal |
| `warning-3` | text | 2, 52 | — | 21 ch = 126 px, x 2..128 | fixed literal |
| `warning-4` | text | 2, 72 | — | 28 ch = 168 px, x 2..170 | fixed literal |
| `warning-5` | text | 2, 84 | — | 22 ch = 132 px, x 2..134 | fixed literal |
| `footer-hint` | text | 2, 124 | — | 26 ch = 156 px, x 2..158 | fixed literal |
| `level-position` | scrollbar | 232, 14 | — | 5 × 100 px | geometry; 100px so it stops clear of the banner row at y=116 |

Rows inked 17 of 17. Narrowest right margin 70 px.

> Accepted overlap: footer-hint is the row the banner replaces by design (§2c)

**No collisions, no overflow, every icon addressable, and 1 banner overlap(s) declared below.**

## 6. Queue

The ring is **9 pages**, not 11: two volume pages were absorbed at §5.1.

| # | Screen | State |
| --- | --- | --- |
| 1 | `info-p0-global-status` | **specified, §3** |
| 2 | `info-p1-instant-flow` | **specified, §4** |
| 3 | `info-p2-cumulative-m3` | **specified, §5** — absorbs the old cumulative-litres page |
| 4 | `info-p3-session-m3` | **specified, §5** — absorbs the old session-litres page |
| 5 | `info-p4-max-flow` | **specified, §5a** — kept: summary vs detail, not duplication |
| 6 | `info-p5-enter-config` | **specified, §5b** |
| 7 | `info-p6-factory-reset` | **specified, §5b** — its warning was materially untrue |
| 8–9 | `net-wifi-root`, `net-mqtt-root` | 14 net screens behind them, 14 overflows |
| — | `config-c1…c7`, `config-s1…s4`, `config-sensor-1…8` | 29 screens, 29 overflows |

Renumbering follows from the absorption and needs confirming: the old P4/P5/P6 shift up, so max-flow becomes
P4, enter-config P5, factory-reset P6. `Display_UI_Requirements.md` §4.1's state machine and §4.3's page table
both need rewriting for the 9-page ring — and §4.1 was already stale, since it describes a 9-page ring by
coincidence while the real one had grown to 11 with the network pages (§3.3).

The 43 config and net overflows share one cause: footer hints and option lists written as prose against a
budget nobody had stated. The worst is the baud-rate list at **58 characters (348 px)**, 108 px past the edge;
`UP/DN adjust  ENTER save  hold ENTER discard` is 44. Every one fits once shortened to 38.
