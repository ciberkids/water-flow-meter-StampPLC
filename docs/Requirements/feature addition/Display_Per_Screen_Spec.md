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

## 2. The panel budget — the numbers that were missing

From `ui_renderer.cpp`, not from the mockup:

| Quantity | Value | Source |
| --- | --- | --- |
| Panel | 240 × 135 px | `utils/layout.ts`, `ui_pages.h` |
| Glyph width, `text` / `badge` / `scrollbar` | **6 px** | `ui_renderer.cpp:16` |
| Glyph width, `value` | **7 px** | `ui_renderer.cpp:17` |
| Glyph height, all kinds | 8 px | `ui_renderer.cpp:292` |
| Badge box | text width + 3 px each side; 8 + 2×2 tall | `ui_renderer.cpp:285-293` |
| Box / icon fallback when width is 0 | 40 × 12 px | `ui_renderer.cpp:314-315` |

Therefore:

- **40 characters** per row of `text`, from x = 0. From the usual x = 8 margin, **38**.
- **34 characters** per row of `value`.
- **17 rows** of 8 px; the last (y = 128…135) is clipped by 3 px.
- A per-sensor reading in the device's own format — `%u: %6.2f %s` → `1:   2.34 L/s` — is 13 characters
  and therefore **91 px**. Two such columns plus labels and badges need 242 px on a 240 px panel, which is
  why every two-column sensor page collides. This arithmetic, not taste, is what forces their redesign.

**Rules for every screen from here on.** An element must not overlap another element's box; an `icon` is
not decorative (`drawFlowDots` paints its whole area, and P0's own defect was text drawn across it); a
row's worst-case string — not its typical one — must fit; and an element's authored `content` must never
contradict its `binding`, because on the device the binding always wins.

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
| Modbus holding registers | L/s, **documented nowhere** | `modbus_manager.cpp:338` writes `instantFlow_L_s` |
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
  integrated.** No hardware has shipped, so this is the cheapest moment it will ever be — but the register
  units must be *documented* this time, since no document currently states them.
- Volumes are unaffected: litres and m³ throughout.

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

## 3. P0 — System Status  *(agreed)*

`info-p0-global-status`. The landing page from Idle, and the root of the info ring. Requirements entry:
"Global aggregate flow and volume | ENTER: no action | ENTER-long: escape (already at root)".

### 3.1. Layout

Generated by `npx tsx tools/audit/screen-ascii.ts info-p0-global-status --proposal
tools/audit/proposals/info-p0-global-status.json`, showing each row's WORST CASE, not its typical one:

```
     +----------------------------------------+   40 cols x 17 rows
y   2 |System Status                           |
y  16 |Total Current Flow (L/m) 9999.99>      ||
y  32 |            ++++++++++++++++           ||
y  40 |            +   (o)  (o)   +           ||   flow-dots, 96x32 at (72,28)
y  56 |            ++++++++++++++++           ||
y  72 |Since reset: 9999.99 L                 ||
y  80 |Max Flow: 9999.99 L/m (S8)             ||
y  96 |WiFi AuthF  MQTT OK  LED 1p/100L       ||
y 124 |UP/DN pages   UP+DN off                 |
     +----------------------------------------+
```

With typical values, and the dots drawn as the firmware draws them:

```
     +----------------------------------------+
y   2 |System Status                           |
y  16 |Total Current Flow (L/m)  1123.20      ||
y  40 |              (o)      ( )             ||   left dot lit, right dark
y  72 |Since reset: 987.60 L                  ||
y  80 |Max Flow:  140.40 L/m (S3)             ||
y  96 |WiFi OK  MQTT OK  LED 1p/10L           ||
y 124 |UP/DN pages   UP+DN off                 |
     +----------------------------------------+
```

| Element | Kind | x, y | Binding | Format | Worst case |
| --- | --- | --- | --- | --- | --- |
| `hdr-title` | text | 2, 2 | — | `System Status` | 13 ch = 78 px |
| `total-flow-label` | text | 2, 16 | — | `Total Current Flow (L/m)` | 24 ch = 144 px |
| `total-flow-value` | **value** | 152, 16 | `telemetry.totalFlowLpm` | `%7.2f` | 7 ch = 49 px, ends x=201 |
| `flow-dots` | icon | 72, 28 | — | TWO alternating dots, 96 × 32 | r = min(w,h)/4 = 8 |
| `session-total` | text | 2, 68 | `telemetry.totalVolumeLiters` | `Since reset: %7.2f L` | 22 ch = 132 px |
| `max-flow` | text | 2, 80 | `telemetry.maxFlowLpm` *(new)* | `Max Flow: %7.2f L/m (S%u)` | 26 ch = 156 px |
| `net-led-status` | text | 2, 96 | `legend.status` *(new)* | `WiFi %s  MQTT %s  LED 1p/%uL` | 32 ch = 192 px |
| `footer-hint` | text | 2, 124 | — | `UP/DN pages   UP+DN off` | 23 ch = 138 px |
| `level-position` | scrollbar | 232, 14 | — | 5 × 104 | — |

Every worst case leaves ≥ 84 px spare and no two elements share a pixel.

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
  - **Consequence of §2a that must be fixed with the unit change:** the animation rate is derived from
    `aggregateFlowLps` clamped to 0.1..10.0 with `periodMs = 1000 / flow` (`ui_renderer.cpp:345-349`). Those
    constants are calibrated for litres per SECOND; fed litres per minute every non-trivial flow saturates
    the clamp and the dots alternate at a fixed maximum rate, losing all information.
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
  - `wifiStateText` is capped at five characters (`wifi_manager.h:66`) and MQTT's states are `OK` / `OFF`, so
    the network half cannot exceed 21 characters whatever happens to the link.
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

**Dataset** — relabel `total-flow-label` to `Total Current Flow (L/m)`; `total-flow-value` becomes
`kind: "value"` bound to `telemetry.totalFlowLpm`; add `session-total` and `max-flow`; move `flow-dots` to
(72, 44) at 96 × 16; **replace `net-status` and `legend-led` with one `net-led-status` row** at (2, 96),
which also **deletes `legend-led`'s authored content** (`LED: Red=Pulse Grn=Ready Blu=Flow`) that its
binding has always overridden. Dataset edits go through `tools/skeleton/generate.mjs`, which CI diffs.

**Firmware formats** — `telemetry.totalFlowLpm`: `%7.2f`, so four digits and two decimals are guaranteed
rather than hoped for (`%.2f` was unbounded). `legend.status` replaces `legend.led`'s
`"LED: %uL pulses | %ums"` with `"WiFi %s  MQTT %s  LED 1p/%uL"`, folding in what `net.status` used to
render on its own row.

**New firmware values** — two:
- `telemetry.maxFlowLpm`, an argmax over `SensorSnapshot::maxFlow` (`ui_controller.h:24`), which the render
  context already holds. No new stored state, no new register, no new topic: the per-sensor peak already
  exists in RAM (`sensor_types.h:26`), on Modbus (`OFF_MAX_FLOW = 15`) and over MQTT (`firmware.cpp:955`).
- `legend.status`, the combined network-and-LED row.

Each needs a `kSimpleValues` entry, a resolver arm and a manifest regeneration. `net.status` and
`legend.led` lose their only users on P0 but stay in the catalogue for the net pages.

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

Generated by `npx tsx tools/audit/screen-ascii.ts info-p1-instant-flow --proposal
tools/audit/proposals/info-p1-instant-flow.json`. `%7.2f` pads, so **every row is exactly 14 characters wide
whatever the magnitude** — which is what keeps the two columns aligned:

```
     +----------------------------------------+   40 cols x 17 rows
y   2 |Instant Flow                            |
y  24 |1: 9999.99 L/m    5: 9999.99 L/m       ||
y  44 |2: 9999.99 L/m    6: 9999.99 L/m       ||
y  64 |3: 9999.99 L/m    7: 9999.99 L/m       ||
y  84 |4: 9999.99 L/m    8: 9999.99 L/m       ||
y 124 |UP/DN pages   UP+DN off                 |
     +----------------------------------------+
```

With a mixed, realistic fleet — two channels not in service, one uncalibrated:

```
     +----------------------------------------+
y   2 |Instant Flow                            |
y  24 |1:  140.40 L/m    5:    0.00 L/m       ||
y  44 |2:  138.90 L/m    6: --                ||
y  64 |3: --             7: --                ||
y  84 |4: SET?           8:   96.20 L/m       ||
y 124 |UP/DN pages   UP+DN off                 |
     +----------------------------------------+
```

| Element | Kind | x, y | Binding | Worst case |
| --- | --- | --- | --- | --- |
| `hdr-title` | text | 2, 2 | — | `Instant Flow`, 12 ch = 72 px |
| `s1-value` … `s4-value` | value | 2, 24 / 44 / 64 / 84 | `sensor.1..4.instantFlow` | 14 ch = 98 px, x 2..100 |
| `s5-value` … `s8-value` | value | 110, 24 / 44 / 64 / 84 | `sensor.5..8.instantFlow` | 14 ch = 98 px, x 110..208 |
| `footer-hint` | text | 2, 124 | — | 23 ch = 138 px |
| `level-position` | scrollbar | 232, 14 | — | 5 × 104 |

Column gap 10 px, right margin 32 px, 20 px row pitch. No collisions. Chosen over one column of eight
because the panel is landscape: a single column uses 100 px of 240 and leaves 58% of the width empty, and
12 px pitch reads worse at distance than 20 px.

### 4.3. Format change, shared with P2–P6

The per-sensor format becomes **`%u: %7.2f %s`** (was `%6.2f`). Under §2a a single channel can reach
`9999.99 L/m`, which `%6.2f` cannot hold. This is the format shared by all six telemetry pages, so it lands
on P2–P6 at the same time and their rows all become 14 characters wide.

The unit stays on **every row** rather than once in the header: both fit, and a bare number on a plant-room
panel is how a gauge gets misread.

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

*Status: the collapse is specified but not yet agreed to implement — a blast-radius trace over the engine,
the status register, the LED green-ready rule, MQTT presence and the mockup is in flight.*

## 5. Queue

In ring order, with the audit's current findings. Each becomes a section here as it is agreed.

| # | Screen | Findings today |
| --- | --- | --- |
| 1 | `info-p0-global-status` | 3 collisions — **specified, §3** |
| 2 | `info-p1-instant-flow` | 20 collisions — **specified, §4** |
| 3 | `info-p2-cumulative-liters` | 8 collisions |
| 4 | `info-p3-cumulative-m3` | 8 collisions |
| 5 | `info-p4-session-liters` | 8 collisions |
| 6 | `info-p5-session-m3` | 8 collisions |
| 7 | `info-p6-max-flow` | 8 collisions |
| 8 | `info-p7-enter-config` | — |
| 9 | `info-p8-factory-reset` | — |
| 10 | `net-wifi-root` … `net-mqtt-*` (14 screens) | 14 overflows |
| 11 | `config-c1…c7`, `config-s1…s4`, `config-sensor-1…8` (29 screens) | 29 overflows |

The 43 config and net overflows share one cause: footer hints and option lists written as prose against a
budget nobody had stated. The worst is the baud-rate list at **58 characters (348 px)**, 108 px past the
edge; `UP/DN adjust  ENTER save  hold ENTER discard` is 44. Every one of them fits once shortened to 38.
