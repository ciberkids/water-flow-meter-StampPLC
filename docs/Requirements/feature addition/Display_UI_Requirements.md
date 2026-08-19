# Requirement: On-Device UI for Display and Buttons

**Version:** 0.2
**Date:** 2026-07-30 · amended 2026-08-17

> **0.2 — hierarchical navigation model.** §3, §4.1, §4.3, §5 and §6 are revised per
> [`NF-20260730-01-menu-navigation-model.md`](../../archive/NF-20260730-01-menu-navigation-model.md) (implemented, so archived).
> Summary of what changed and why:
>
> - **ENTER is no longer overloaded.** 0.1 gave ENTER-long three meanings — global
>   back-to-idle (§4.1), arm a reset countdown (§4.3) and cancel an edit (§5.1). No firmware
>   can satisfy all three, which is why Configuration mode was never implemented.
> - **Navigation is now a tree.** Every level is a ring of sibling pages ending in `BACK`.
> - **Display-off moves to UP + DOWN**, freeing ENTER-long to mean "escape" everywhere.
> - **Factory reset moves from the blind UP+DOWN combo to page P6** with a confirm screen.
> - **Destructive actions get confirm screens and acknowledgement toasts** instead of
>   countdowns armed straight from an info page.
> - **Editing happens on its own screen**, not via an invisible mode flag, so the web design
>   tool can preview every state.
> - Display orientation is **landscape 240 × 135** (decision D3); Modbus slave ID range
>   corrected to **1–247** (decision A2); baud rate is stored as a **list index** (A4).
> - **Amended 2026-08-17 — §3.1's held UP/DOWN repeat is withdrawn.** 0.2 specified a 250 ms
>   repeating navigation step. Nothing ever answered it, and per
>   [`../../active_work/open_decisions.md`](../../active_work/open_decisions.md) the gap cost three
>   bug reports; the ruling was to drop the requirement rather than implement it. See §3.1.1.

---

## 1. Purpose

Define the functional requirements for leveraging the StampPLC’s integrated display and three hardware buttons to present live system metrics and allow local configuration while staying aligned with the Modbus feature set described in `docs/Requirements/Project_document.md`.

---

## 2. Hardware Context & Assumptions

- 1.14″ ST7789V2 display, native resolution 135 × 240, driven **landscape: 240 wide × 135 tall** (`display.setRotation(1)`). See `../../hardware docs/StampPLC specifications.md`. All screen layouts are authored to 240 × 135.
- Three physical momentary buttons mapped to MCU GPIOs with interrupt support via the PI4IOE5V6408 expander (I²C SCL = GPIO15, SDA = GPIO13, INT = GPIO14, RST = GPIO3; key lines `KEYA/B/C`, see `../../hardware docs/StampPLC pin map.md`).
- One tri-colour status LED (RGB) driven from the same expander (`P6/P5/P4`) following the behaviour defined in `RGB_LED_Behavior.md`.
- Firmware already exposes sensor state, session counters, cumulative totals, polling rate, and configuration via Modbus registers.
- Polling task publishes `pollingRate_kHz`; this value must be used for Nyquist-style validation when saving sensor parameters.
- Every screen must fit one display without scrolling. A level with more entries than fit shows a scroll indicator.

---

## 3. Button Interaction Model

### 3.1. Gesture contract

The long-press threshold is **1.5 s** — this is the *gesture* boundary. Durations longer
than that (3 s, 30 s) are always **countdowns** shown on screen, never gesture thresholds.

| Gesture | Meaning |
| :--- | :--- |
| UP / DOWN short | Previous / next sibling at the current level (wraps), or −1 / +1 in a value editor |
| UP / DOWN held | **Nothing on a navigation screen** — see §3.1.1. Accelerating adjust in a numeric editor (§5.4) |
| ENTER short | Descend into the current entry, or commit, or exit — depends on screen type, see §3.2 |
| ENTER long (≥1.5 s) | Escape to the main screen, or discard, or confirm — see §3.2 |
| UP + DOWN short | **Display off, and reset navigation to P0.** Works from any screen at any depth. |
| Any button while idle | Wake the display; the UI is already at P0 (unchanged) |

**UP + DOWN clears the navigation stack**, so the display always wakes on P0 rather than
wherever the operator happened to be. This makes waking deterministic and matches §4.1's
"P0 is the landing page from Idle". Any uncommitted edit is discarded.

For the same reason the **120 s inactivity timeout also resets to P0** — otherwise waking
would be predictable after a manual off but arbitrary after an automatic one.

### 3.1.1. A held UP/DOWN does not navigate — amended 2026-08-17

**One press, one step.** Holding UP or DOWN on a navigation screen — info page, config list,
sensor list, `BACK` entry, at any depth — moves nothing. Paging a ring is a **tap**, and each
tap is exactly one sibling. 0.2 specified a 250 ms repeating navigation step; this amendment
drops it.

**Why repeat events still exist in the firmware.** `button_input.cpp` still emits a long press
at 1.5 s and a repeat every 250 ms thereafter, and that is deliberate rather than residue:
§5.4's numeric-editor ramp depends on a held button. The ramp does not read those queued
events at all — `InteractionHandler::handleEditorRepeat` reads the button's pressed **levels**
through `ui_accel.h`'s tiers, on every loop pass, before the queue is drained. Two things, and
only two, respond to a held UP/DOWN: §5.4's editor ramp, and the Select Menu's cursor, which
moves on **any** event kind because the firmware-drawn selector drains the queue and switches
on `event.button` alone (`InteractionHandler::handlePackSelector`; see `Gesture_Reference.md`
§3.6). Everywhere else a repeat is popped, mapped to `FlowGesture::Hold`, matched against a
table that declares no hold flow, and dropped.

**This is a withdrawal, dated 2026-08-17, not an omission and not a deferral.** The
alternative was live and was rejected: declaring `hold` flows on the info ring would have
implemented the repeat. The requirement was withdrawn instead because the firmware never
answered it, because three bug reports were spent discovering that (recorded in
[`../../active_work/open_decisions.md`](../../active_work/open_decisions.md)), and because the
rings are short enough that one-press-one-step is sufficient. The dataset schema still lists
`hold` as a legal gesture value (`web/mockup/shared/schemaDefinitions.ts`) and no screen in the
shipped dataset declares one — do not delete the enum value on the strength of this amendment,
which drops a requirement and not a schema. Anyone about to file "holding UP does nothing" as a
defect: it is this decision.

### 3.2. ENTER depends on screen type — deliberately

| Screen type | ENTER short | ENTER long |
| :--- | :--- | :--- |
| **Navigation** (info pages, menu levels) | Descend into the current entry; on `BACK`, ascend one level | Escape to the main screen (P0) |
| **Value editor** | **Commit** and ascend one level | **Discard** and ascend one level |
| **Confirm** (destructive action) | **Exit** without acting | **Confirm** the action |

Editors put the easy gesture on the common outcome (you usually save); confirm screens put
it on the safe one (you usually back out). The irreversible action therefore always
requires the deliberate gesture.

> Because the mapping is not uniform, **the footer legend must state it on every screen**
> (§6). The three screen types are visually distinct — an editor shows a value, a confirm
> screen asks a question — so the legend is a reminder rather than the only cue.

### 3.3. Retired from 0.1

The **UP + DOWN held 30 s** factory-reset combo is removed. A blind combo is
undiscoverable, gives the operator no indication they are seconds away from wiping the
device, and cannot report what happened. Factory reset is now page **P6** with a confirm
screen (§4.3). LED suppression during the reset countdown is unchanged in behaviour — only
the trigger differs (see `RGB_LED_Behavior.md`, `Project_document.md` §5.3 item 4).

> **Recovery with an unusable display.** The retired combo worked blind. If the display
> fails, use Modbus register 20 (`Master Reset All Configs`).

---

## 4. Display Info Mode

### 4.1. State Machine

Info mode is the root level (L0) and therefore has no `BACK` entry. `P0` is the landing
page from Idle.

```mermaid
stateDiagram-v2
    direction LR
    Idle --> Info: any button (lands on P0)
    Info --> Idle: UP+DOWN short (clears nav stack)
    Info --> Idle: 120 s inactivity (clears nav stack)

    state Info {
        [*] --> P0
        P0 --> P1: DOWN
        P1 --> P2: DOWN
        P2 --> P3: DOWN
        P3 --> P4: DOWN
        P4 --> P5: DOWN
        P5 --> P6: DOWN
        P6 --> NetWifi: DOWN
        NetWifi --> NetMqtt: DOWN
        NetMqtt --> P0: DOWN
        P0 --> NetMqtt: UP
    }

    Info --> ConfirmResetTotals: ENTER short on P2
    Info --> ConfirmResetSession: ENTER short on P3/P4
    Info --> ConfigRoot: ENTER short on P5
    Info --> ConfirmFactoryReset: ENTER short on P6

    ConfirmResetTotals --> Info: ENTER short (exit)
    ConfirmResetTotals --> ToastTotals: ENTER held 3 s
    ToastTotals --> Info: after 2 s

    ConfirmResetSession --> Info: ENTER short (exit)
    ConfirmResetSession --> ToastSession: ENTER long
    ToastSession --> Info: after 2 s

    ConfirmFactoryReset --> Info: ENTER short (exit)
    ConfirmFactoryReset --> Reboot: ENTER held 30 s

    ConfigRoot --> Info: ENTER long (escape) or BACK
```

**The DATASET's ring is nine entries, not nine info pages.** `net-wifi-root` and `net-mqtt-root` were
appended when WiFi_MQTT_Connectivity landed and this diagram was never updated — it described a
nine-page ring by coincidence while the real one had grown to eleven. Two of those eleven have since
been absorbed (see §4.3), which brings it back to nine: seven info pages and the two network roots.

**The ring the OPERATOR pages through is ten**, as of 2026-08-17. `UiNavigator` splices a
firmware-owned `SELECT MENU` entry onto the end of the root level — `Loadable_UI_Menu_Packs.md` §3.4,
implemented in `src/ui/core/ui_root_tail.h` — so DOWN off `net-mqtt-root` lands on it and DOWN again
wraps to P0. It is in no dataset and no pack, which is what stops a pack removing the only
discoverable route to menu selection, and is also why the diagram above and the mockup cannot show it.

### 4.2. Global Flow Indicator (Animated Dots)

- The **Global Status** page (P0) is the first screen shown from Idle. It shows the aggregate current flow, the
  volume since reset, the largest per-channel peak with the channel that holds it, and a central flow
  indicator. See `Display_Per_Screen_Spec.md` §3 for its exact layout.
- The flow indicator is **four dots in a chase**: one lit at a time, travelling left to right and wrapping, at a
  rate derived from the aggregate flow. When no flow is detected, a single red dot is shown statically in the
  leftmost position.
- Three defects in the current implementation are to be fixed with it, and are recorded in
  `Display_Per_Screen_Spec.md` §3.2: it paints **two** alternating dots rather than four; the inactive dot is
  painted with the theme's *badge* background instead of the panel background, so it renders as a visible disc
  on a screen that has no badges; and the two colours are hardcoded RGB565 literals that bypass the palette
  every other element uses.
- **The rate is not observable at the current repaint cadence.** The phase is computed from `millis()` at up to
  10 Hz while the renderer redraws roughly once a second, so the dots change position between frames but nothing
  walks. The chase needs either a faster repaint for this element or an explicit per-frame step; specifying the
  geometry does not make it animate.
- No propeller animation: it was dropped when the layout was still 135 px wide, and the landscape orientation has not brought it back.

### 4.3. Telemetry Pages

Pages are circular; UP/DOWN step through them with wrap-around. P0 provides a global summary, P1–P4 display one
metric for all eight sensors concurrently, and P5/P6 are text-only entry points. A channel that is not in
service renders `--`; one that is in service without a valid calibration renders `SET?`.

**Two volume pages were absorbed.** The ring previously carried cumulative litres, cumulative m³, session
litres and session m³ — two quantities in two units, with each m³ page derived from its litres page by `/1000`.
Since 1 L is exactly 0.001 m³, one page carries both readings, so cumulative and session are now one page each.
Every wire surface still publishes both units; only the panel picks one. See `Display_Per_Screen_Spec.md`
§2a.1 and §5.

| Page ID | Title | Metric Source | ENTER short | ENTER long |
| :------ | :---- | :------------ | :---------- | :--------- |
| P0 | System Status | Aggregate flow, volume since reset, largest peak and its channel | No action | Escape (already at root) |
| P1 | Instant Flow | Holding registers 101… (`instantFlow`) | No action | Escape |
| P2 | Cumulative Volume | Holding registers 107… (`cumulativeM3`) | Open `Reset totals?` | Escape |
| P3 | Session Volume | Holding registers 113… (`sessionM3`) | Open `Reset session?` | Escape |
| P4 | Max Flow Since Reset | Holding register 115… (`maxFlowSinceReset`) | Open `Reset session?` | Escape |
| P5 | Configuration | Text-only prompt | Enter Configuration (§5) | Escape |
| P6 | **Factory Reset** | Text-only prompt | Open `Factory reset?` | Escape |
| — | `net-wifi-root`, `net-mqtt-root` | See `WiFi_MQTT_Connectivity.md` | — | Escape |

**Layout is specified per screen, not here.** `Display_Per_Screen_Spec.md` owns every coordinate, and it exists
because this section previously described the *content* of each page while nothing stated how much of it fits: an
audit at the firmware's own text metrics found 50 of 79 screens with collisions or overflows. The binding
constraint is that a per-sensor reading is 6–7 px per glyph on a 240 px panel, so **two columns of four** is the
only arrangement that holds eight channels, and only after the per-sensor label and status badge are removed —
both of which restated what the value's own format already says.

P4 is retained rather than merged into P0 because it answers a different question: `MAX` marks a channel whose
peak reached its `q_max` ceiling, which identifies a sensor under-dimensioned for the pipe it is installed on.
P0 reports only the largest peak and its owner.

> Per-sensor register addresses follow `100 + (n−1) × 40 + offset`; see
> `Project_document.md` §4.2. The single addresses above are sensor 1.

#### 4.3.1. Confirm screens and acknowledgement toasts

Destructive actions live behind a **confirm screen**. On a confirm screen ENTER-short
exits without acting and ENTER-long confirms. UP/DOWN have **no effect** — they neither
navigate nor cancel.

Confirming pushes an **acknowledgement toast** that displays for 2 s and then returns
automatically to the page the operator started from, so the operator gets proof the action
happened rather than an unexplained screen change.

| Confirm screen | Reached from | Hold to confirm | Action | Toast |
| :--- | :--- | :--- | :--- | :--- |
| `Reset totals?` | P2 | **3 s** | `core.action.reset-all-measured` | `TOTALS RESET`, 2 s |
| `Reset session?` | P3, P4 | ENTER long (1.5 s), no countdown | `core.action.reset-session` | `SESSION RESET`, 2 s |
| `Factory reset?` | P6 | **30 s** | `core.action.factory-reset` | reboot, no toast |

Hold durations are proportionate to what is at risk. Cumulative litres are persisted in
NVS and are the only value in the system that cannot be recovered, so they get a 3 s hold.
Session values re-accumulate on their own, so a plain long press suffices. Factory reset
wipes NVS and reboots, and P6 is reachable by ordinary page flipping, so its 30 s hold is
the real guard.

**What the factory reset erases is wider than the screen used to say.** `preferences.clear()` covers the single
`"flow-data"` namespace, which the network settings share — so it also erases the WiFi SSID and PSK, the MQTT
broker, its credentials and the base topic. With on-device text entry removed (§6.3), re-provisioning then
requires the AP portal and someone physically at the device. The confirm screen's warning must say so; see
`Display_Per_Screen_Spec.md` §5b.1.

Additional requirements:

1. Countdown overlays display remaining seconds centrally; releasing ENTER before zero aborts and returns to the confirm screen.
2. During a countdown, UP/DOWN have no effect.
3. A completed reset issues the corresponding Modbus command (register 21 or 22) rather than mutating sensor state directly, so persisted state stays coherent.
4. Every info page carries a footer hint summarising its button gestures.
5. P0 carries the LED's **pulse configuration** — `LED: 1 pulse / 10 L`, from `config.ledPulseVolume` — on one
   row shared with the WiFi and MQTT states. It is not a colour legend: the authored legend text was overridden
   by its own binding on every frame, and the pulse volume is the one thing an operator cannot infer from
   watching the LED. See `Display_Per_Screen_Spec.md` §3.2.

---

## 5. Configuration Mode

### 5.1. Navigation model

Configuration is a **tree**. Every level is a ring of sibling pages whose last entry is
`BACK`. One rule governs every screen:

> **UP/DOWN move within the current level. ENTER-short descends or commits.
> ENTER-long escapes to P0. `BACK` ascends one level.**

```
L0  Info mode ............ P0 P1 P2 P3 P4 P5 P6 + 2 net roots (root, no BACK)
     └─ P5 ENTER-short ─▶ L1  Config root
                               │  C1 C2 C3 C4 C5 C6 C7 BACK
                               │
                               ├─ C1..C6 ENTER-short ──▶ L2  Value editor (C1.V .. C6.V)
                               │
                               └─ C7 "Sensors ▸" ──────▶ L2  Sensor list
                                                              │  Sensor 1 .. Sensor 8  BACK
                                                              │
                                                              └─ ENTER-short ─▶ L3  Sensor settings
                                                                                     │  S1 S2 S3 S4 BACK
                                                                                     │
                                                                                     └─ ENTER-short ─▶ L4  Value editor (S1.V .. S4.V)
```

- **C7 carries no value.** It is a pure descent node labelled “Sensors ▸”. The sensor a
  setting applies to is the L2 page the operator descended from, so there is no
  “selected sensor” state for the firmware and the mockup to keep in sync.
- Maximum depth is 5. ENTER-long returns to P0 from any depth in one press.

### 5.2. Device-Level Settings (L1)

| Page | Label | Editor | Range / Options | Step | Register |
| :--- | :---- | :----- | :-------------- | :--- | :------- |
| C1 | Modbus Slave ID | `C1.V` | **1–247** | ±1, accelerating | 40 |
| C2 | Baud Rate | `C2.V` | 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200 | cycle list | 41 (list index) |
| C3 | Parity | `C3.V` | None, Even, Odd | cycle list | 42 |
| C4 | Stop Bits | `C4.V` | 1, 2 | cycle list | 43 |
| C5 | LED Pulse Volume | `C5.V` | 1 L, 10 L, 100 L | cycle list | 31 |
| C6 | LED Pulse Period | `C6.V` | 100–2000 ms | ±1, accelerating | 32 |
| C7 | Sensors ▸ | — | — | descend only | — |

Notes:

- **Slave ID is 1–247**, not 1–255: address 0 is the Modbus broadcast address and 248–255 are reserved by the specification.
- **Baud rate is stored as a list index**, because 115200 does not fit in a `uint16_t`. The index also matches the cycle-list interaction.
- C1–C4 change the Modbus link itself. Registers 40–44 and the explicit apply step are specified in `Project_document.md` §4.1; a link change must not be applied mid-response.
- C3 and C4 combine into the UART frame summary shown to the operator (e.g. “8E1”). It is derived, not stored.

### 5.3. Sensor Settings (L3)

Reached via C7 → sensor list → the chosen sensor. Register addresses below are for
sensor *n*, following `100 + (n−1) × 40 + offset`.

| Page | Label | Editor | Type | Range | Step | Register |
| :--- | :---- | :----- | :--- | :---- | :--- | :------- |
| S1 | Connected | `S1.V` | boolean | on / off | toggle | bit *n* of register 10 |
| S2 | Multiplier (F) | `S2.V` | int16 | signed 16-bit | ±1, accelerating | 121 + 40·(n−1) |
| S3 | Adjust | `S3.V` | int16 | signed 16-bit | ±1, accelerating | 122 + 40·(n−1) |
| S4 | Max Flow (Q) | `S4.V` | uint16 | 0–65535 L/min | ±1, accelerating | 120 + 40·(n−1) |

### 5.4. Value Editors

Each editable setting has its own screen, showing the label, the **pending** value
(highlighted), the unit, and the currently saved value — so the operator can see both what
they are about to commit and what is currently in force.

ENTER-short commits and ascends one level; ENTER-long discards and ascends one level.
Discarding is immediate: an unsaved edit is harmless to lose, so a countdown would add
friction without adding safety.

**Acceleration** applies to numeric editors only. Enum and boolean editors cycle with no
acceleration.

| Hold duration | Step | Interval |
| :--- | :--- | :--- |
| 0–700 ms | ±1 | 250 ms |
| 700 ms – 1.5 s | ±5 | 150 ms |
| > 1.5 s | ±25 | 150 ms |

A short press is always exactly ±1 regardless of what preceded it. Numeric values clamp at
their range ends; enums and booleans wrap.

### 5.5. Committing, Validation and the Nyquist Path

ENTER-short in an editor performs, in order:

1. Clamp the pending value to its range.
2. Write the mapped Modbus holding register **within the same task cycle**.
3. For sensor settings, evaluate the Nyquist condition
   `pollingRate_kHz * 1000 >= 2 * max(0, Q * F + Adjust)`.
   - **Pass:** persist to NVS (reusing the existing `Preferences` namespace), clear bit *n* of register 30, show a `SAVED` toast, ascend one level.
   - **Fail:** do **not** ascend. Push the Nyquist override screen: “Sampling too slow. UP = Edit values, DOWN = Save anyway.”
     - **UP** returns to the editor with the pending value intact.
     - **DOWN** forces the save, sets bit *n* of register 30, shows a `SAVED (WARNING)` toast, and ascends one level.
4. The UI shall respect `isConfigValid()`; invalid entries are blocked with a contextual message and cannot be saved except via the Nyquist override path above.

### 5.6. Exit Behaviour

- `BACK` ascends exactly one level.
- ENTER-long escapes to P0 from any depth, discarding any uncommitted edit.
- UP+DOWN turns the display off and clears the navigation stack to P0 (§3.1), so the device never wakes deep inside Configuration.
- Leaving Configuration does **not** by itself turn the display off.

### 5.7. Screen and Page Identifier Scheme

An editor’s identifier is **derived** from its parent page, so no lookup table is needed
and the exporter can mechanically verify that every setting page has its editor.

**A page number belongs in the identifier, never in the title an operator reads.** The factory-reset screen
was titled `P8 FACTORY RESET`, so renumbering the ring made its own title wrong. Screen ids carry the
position; titles carry the meaning.

| Level | Page ID | Screen ID |
| :--- | :--- | :--- |
| Info page | `P6` | `info-p6-factory-reset` |
| Confirm | `P6.C` | `confirm-factory-reset` |
| Toast | `P2.T` | `toast-totals-reset` |
| Config root | `C1` | `config-c1-modbus-id` |
| Value editor | `C1.V` | `config-c1-modbus-id-edit` |
| Sensor list | `SEN3` | `config-sensor-3` |
| Sensor settings | `S2` | `config-s2-multiplier` |
| Value editor | `S2.V` | `config-s2-multiplier-edit` |
| BACK entry | `<parent>.BACK` | `config-<level>-back` |

### 5.8. Diagnostic Feedback

- The firmware exposes a read-only holding register at global address **30** (`Undersampling Flags`). Bit *n* (0–7) mirrors whether sensor *n* failed the Nyquist validation during the most recent save.
- Info pages display a warning badge next to any sensor whose bit is high; sensor settings pages surface the same state before edits are committed.
- The flag clears automatically when a sensor passes validation on the next save. Forcing a save via the override path sets it.

---

## 6. Visual & UX Guidelines

1. Consistent header/footer across every screen: header shows the current position in the tree (e.g. “Config ▸ Sensor 3 ▸ Multiplier”); footer shows the gesture legend.
2. **The footer legend must state the ENTER mapping for the current screen type** (§3.2), because it differs between navigation, editor and confirm screens. For example `↑↓ adjust · ENTER save · hold ENTER discard`.
3. Use a contrasting highlight colour for the pending value in an editor; show the saved value muted alongside it.
4. `BACK` renders as a distinct entry (e.g. `◀ BACK`) so it is not mistaken for a setting.
5. Display units alongside numeric values (“L/min”, “L”, “m³”, “ms”).
6. Provide success or error feedback within 1 s of an action — the acknowledgement toasts of §4.3.1 satisfy this.
7. All eight sensors fit without overlap using a two-column layout (sensors 1–4 left, 5–8 right).
8. Surface the LED status legend on P0.

---

## 7. Non-Functional Requirements

- **Responsiveness:** Button events must be acknowledged within 100 ms to feel immediate.
- **Low Power:** When the display is off, backlight current draw must fall below the platform’s documented standby target.
- **Localization-ready:** All user-facing strings route through a central table to enable future translations.
- **Reliability:** Countdown confirmations must survive transient Modbus activity; configuration saves cannot be interrupted by external requests.
- **No dynamic allocation:** the navigation stack (max depth 5) and editor state are fixed-size.

---

## 8. Follow-Up

1. Capture UX copy for the factory-reset confirm screen and the acknowledgement toasts (multilingual).
2. Validate the coarse-adjust acceleration tiers with real sensors to confirm they balance speed and precision.
3. Implement automated tests that verify the `Undersampling Flags` register clears when conditions improve.
4. Prototype RGB LED transitions (red pulse, green ready, blue flow) to ensure they remain visible without distracting from the UI.
5. **Two kinds of timeout.** A hold countdown requires ENTER held and aborts on release; an auto timeout fires regardless and drives the toasts. The dataset schema currently has one `timeout` trigger for both — see `NF-20260730-01` §3.8 for the proposed discriminator.
6. **Per-setting descriptors have no home yet.** Ranges, steps and enum lists in §5.2/§5.3 are specified here but are not expressed anywhere machine-readable. Proposal: extend the firmware manifest’s value entries with `min`, `max`, `step`, `enum` and `unit`, so one declaration drives both the web mockup and the firmware.
7. **Settable elements should be a fixed catalogue in the web design tool.** The design tool should offer the settable entities as fixed, pre-declared items rather than free-form elements with arbitrary binding strings, so the connection back to firmware is structural instead of a name that has to match. What the dataset then controls is screen order, sub-level nesting, and the placement and wording of text — not which values exist. This is the same single source of truth as item 6.
8. ~~Amend the diagrams to match this revision.~~ **Done 2026-08-12.** `ui_config_layout.mermaid` and the new `ui_navigation_tree.mermaid` are now GENERATED from the dataset by `tools/wiki/gen-diagrams.mjs`, so they cannot drift from it again. `ui_state_machine.mermaid` and `ui_sensor_submenu.mermaid` are retired — both described structure the generated pair now covers, and both had been wrong in every structural detail for months.
