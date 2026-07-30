# Requirement: Hierarchical Menu Navigation Model

**Version:** 0.2 (proposal)
**Date:** 2026-07-30
**Status:** proposal — supersedes `Display_UI_Requirements.md` §5 and amends §2 and §4.3
**Supersedes decision:** D1 in `docs/active_work/open_decisions.md`

**Changes in 0.2** — incorporates answers to Q1–Q3: UP+DOWN short-press turns the display
off (§3.5), `BACK` confirmed as a screen (§3.1), and reset confirmation moves to a
dedicated screen with **inverted** gesture logic plus a timed acknowledgement toast
(§3.3). Adds §3.8 on the two kinds of timeout, which the schema does not currently
distinguish.

---

## 1. Purpose

Three buttons cannot carry enter / change / exit on one gesture each without
overloading ENTER. The current requirement does exactly that: §4.1 makes ENTER-long the
global Info→Idle gesture, §4.3 gives P2–P6 a reset countdown on the same gesture, and
§5.1 gives it "cancel the edit" — three meanings, one gesture. The firmware cannot
satisfy all three, which is why Configuration mode was never implemented.

This proposal replaces the flat page model with an explicit **navigation tree** in which
every level is a ring of sibling pages ending in a `BACK` entry. One rule governs every
screen:

> **UP/DOWN move within the current level. ENTER-short descends or commits.
> ENTER-long escapes to the main screen. `BACK` ascends one level.**

Two secondary goals fall out of this:

- **Every UI state becomes a real screen.** Today ENTER-short on a config page fires
  `config.action.field.edit`, which flips an invisible mode flag. The web mockup cannot
  render a flag, so the design tool cannot preview the editing state at all — defeating
  the 1:1 preview the tool exists for. Making each editor a screen makes every state
  authorable, previewable and exportable.
- **The "selected sensor" state disappears.** It becomes the level you are in, rather
  than a variable the firmware and the mockup must keep in sync.

---

## 2. Hardware / Interfaces

| Component | Connection / Address | Notes |
| --- | --- | --- |
| UP / DOWN / ENTER buttons | PI4IOE5V6408 expander, `KEYA/B/C` | Unchanged. Long-press threshold stays 1.5 s (`ButtonInputManager::kLongPressThresholdMs`). |
| ST7789V2 display | SPI, 240 × 135 landscape | Per decision D3. Every level must fit one screen without scrolling. |
| Modbus holding registers | see `Project_document.md` §4 | Committing an editor writes the mapped register in the same task cycle. |
| NVS (`Preferences`, namespace `flow-data`) | — | Link settings must load **before** `RS485_SERIAL_PORT.begin()`; see A1. |

---

## 3. Behaviour Specification

### 3.1. Navigation tree

Every level is a **ring**: UP/DOWN step through siblings and wrap around. The last entry
of every non-root level is `BACK`.

```
L0  Info mode ............ P0 P1 P2 P3 P4 P5 P6 P7 P8           (no BACK — this is the root)
     │
     ├─ P2/P3 ENTER-short ──▶ L1  Confirm "Reset totals?"   ─▶ toast ─▶ back to P2/P3
     ├─ P4/P5/P6 ENTER-short▶ L1  Confirm "Reset session?"  ─▶ toast ─▶ back to P4/P5/P6
     ├─ P8 ENTER-short ─────▶ L1  Confirm "Factory reset?"  ─▶ wipe NVS + reboot
     └─ P7 ENTER-short ─────▶ L1  Config root
                                   │  C1 C2 C3 C4 C5 C6 C7 BACK
                                   │
                                   ├─ C1..C6 ENTER-short ──▶ L2  Value editor  (C1.V .. C6.V)
                                   │
                                   └─ C7 "Sensors ▸" ───────▶ L2  Sensor list
                                                                   │  Sensor 1 .. Sensor 8  BACK
                                                                   │
                                                                   └─ ENTER-short ──▶ L3  Sensor settings
                                                                                           │  S1 S2 S3 S4 BACK
                                                                                           │
                                                                                           └─ ENTER-short ──▶ L4  Value editor (S1.V .. S4.V)
```

- **C7 carries no value.** It is a pure descent node labelled "Sensors ▸". The sensor a
  setting applies to is the L2 page you descended from, so no `selectedSensor` state
  exists. This removes the §5.2/§5.3 C5-vs-C7 contradiction (A3) entirely.
- **Info mode (L0) has no `BACK`** because it is the root. ENTER-long there is available
  for Idle — see §3.5.

### 3.2. Gesture contract

| Gesture | Navigation level | Value editor | Confirm screen |
| --- | --- | --- | --- |
| UP / DOWN short | previous / next sibling (wraps) | −1 / +1 (or previous / next enum) | no effect |
| UP / DOWN held | repeat sibling stepping every 250 ms | accelerating adjust, see §3.4 | no effect |
| ENTER short | descend into the current entry; on `BACK`, ascend one level | **commit** and ascend one level | **exit** without acting |
| ENTER long (≥1.5 s) | escape to the main screen (P0) | **discard** and ascend one level | **confirm** the action |
| UP + DOWN short | **display off** (§3.5) | display off | display off |

**Gesture meaning differs by screen type, deliberately.** Editors put the easy gesture on
the common outcome (you usually save), confirm screens put it on the safe one (you usually
back out). That optimises for expected frequency, and the irreversible action always
requires the deliberate gesture:

| Screen type | ENTER short | ENTER long |
| --- | --- | --- |
| Navigation | descend | escape to P0 |
| Value editor | commit | discard |
| Confirm | exit | confirm |

Because the mapping is not uniform, **the footer legend must state it on every screen**
(§5). The three screen types are visually distinct — an editor shows a value, a confirm
screen asks a question — so the legend is a reminder rather than the only cue.

> **Proposed change to the original model (R1).** The original sketch put a 3 s
> hold-to-discard countdown on the editor's ENTER-long. This proposal discards
> **immediately**. Discarding an unsaved edit is harmless — the operator re-enters and
> tries again — so a countdown buys no safety while adding friction. Countdowns are
> reserved for irreversible operations (§3.3).

### 3.3. Confirm screens and acknowledgement toasts

Destructive actions live behind a **confirm screen** reached by ENTER-short from the
relevant info page. On a confirm screen ENTER-short **exits** and ENTER-long **confirms**.
UP/DOWN have no effect (§4.3 note 2).

Confirming pushes an **acknowledgement toast** — a screen showing e.g.
`TOTALS RESET` — which displays for 2 s and then returns automatically to the info page
the operator started from. This satisfies §6 note 4 ("feedback within 1 s of an action")
and gives the operator proof the action happened rather than an unexplained screen change.

| Confirm screen | Reached from | On confirm | Toast |
| --- | --- | --- | --- |
| `Reset totals?` | P2, P3 (ENTER-short) | `core.action.reset-all-measured` | `TOTALS RESET`, 2 s |
| `Reset session?` | P4, P5, P6 (ENTER-short) | `core.action.reset-session` | `SESSION RESET`, 2 s |
| `Factory reset?` | **P8** (ENTER-short) | `core.action.factory-reset` | reboot, no toast |
| Nyquist override | sensor editor commit failure | see §3.6 | `SAVED (WARNING)`, 2 s |

Editors should use the same toast pattern on commit (`SAVED`, 2 s) so feedback is
consistent across the UI.

> **Proposed change (Q3).** In the current requirement P2–P6 arm their countdown with
> ENTER-**long** directly from the info page, which collides with ENTER-long as the escape
> gesture. Routing through a confirm screen restores one meaning per gesture per screen
> type, makes the confirmation an authorable screen, and resolves decision **H1**.
>
> **Open: does the confirm screen still need a hold countdown?** §4.3 specifies 30 s for
> "Reset All Measured" and 3 s for session resets. Those durations were sized for a reset
> armed *directly* from an info page, where an accidental press was the risk. A dedicated
> confirm screen already removes that risk, so a 30 s hold is now punitive.
>
> **Recommendation:** keep a **3 s** hold-to-confirm on `Reset totals?` — cumulative
> litres are persisted in NVS and are the one value in the system that cannot be
> recovered — and use a plain ENTER-long (1.5 s) for `Reset session?`, which is
> non-persistent and re-accumulates on its own. `Factory reset?` keeps a **30 s** hold: it
> wipes NVS and reboots, and unlike the other two it is now reachable by ordinary page
> flipping (§3.7), so the long hold is the only thing standing between a browsing operator
> and a wiped device. See §7 item 9.

### 3.4. Value editors

One editor screen per editable setting. Each shows the label, the **pending** value
(highlighted per §6 note 2), the unit, and the currently saved value, so the operator can
see both what they are about to commit and what is in force.

| Page | Setting | Type | Range / options | Step behaviour | Register |
| --- | --- | --- | --- | --- | --- |
| `C1.V` | Modbus Slave ID | numeric | 1–247 (A2) | ±1, accelerating | 40 (A1) |
| `C2.V` | Baud Rate | enum | 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200 | cycle, no acceleration | 41 index (A4) |
| `C3.V` | Parity | enum | None, Even, Odd | cycle | 42 |
| `C4.V` | Stop Bits | enum | 1, 2 | cycle | 43 |
| `C5.V` | LED Pulse Volume | enum | 1 L, 10 L, 100 L | cycle | 31 |
| `C6.V` | LED Pulse Period | numeric | 100–2000 ms | ±1, accelerating | 32 |
| `S1.V` | Connected | boolean | on / off | toggle | bit *n* of 10 |
| `S2.V` | Multiplier (F) | numeric | int16 | ±1, accelerating | 121 + 40·(n−1) |
| `S3.V` | Adjust | numeric | int16 | ±1, accelerating | 122 + 40·(n−1) |
| `S4.V` | Max Flow (Q) | numeric | 0–65535 L/min | ±1, accelerating | 120 + 40·(n−1) |

**Acceleration** (numeric only, unchanged from §5.3 rule 2):

| Hold duration | Step | Interval |
| --- | --- | --- |
| 0–700 ms | ±1 | 250 ms |
| 700 ms – 1.5 s | ±5 | 150 ms |
| > 1.5 s | ±25 | 150 ms |

A short press is always exactly ±1 regardless of what preceded it. Numeric values clamp
at their range ends; enums and booleans wrap.

### 3.5. Reaching Idle, and retiring the UP+DOWN combo

**Answer to Q1.** A short **UP + DOWN** press turns the display off, from any screen and
any level. This is better than overloading ENTER-long, because it works at every depth
without competing with the escape gesture, and it leaves ENTER-long meaning exactly one
thing everywhere.

Waking is unchanged: §4.1 already says "Idle → Info: any button". So the gesture is
effectively **display off**, not a true toggle — see §7 item 10.

Idle is therefore reached two ways:

1. **Automatically** after 120 s without a button press (unchanged, §4.1).
2. **Manually** by UP + DOWN short press, from anywhere.

This settles **H1** (ENTER-long is unambiguously escape) and **H3** (no back-to-idle
countdown — going idle is reversible by any button press).

**Consequence: §2's UP+DOWN factory-reset combo is retired.** With factory reset promoted
to a navigable page (P8, §3.1) and UP+DOWN reassigned to display-off, the blind 30 s combo
and its 3 s warning overlay are no longer needed. That is a net gain:

- A blind button combo is **undiscoverable** — nothing on screen tells an operator it
  exists, and nothing tells them they are 12 seconds into triggering it.
- A menu entry is self-documenting, shows a confirm screen, and reports what happened.
- It deletes a bespoke state machine from `InteractionHandler` (`FactoryResetState`, the
  overlay delay, the restart scheduling) in favour of the same confirm-screen path every
  other destructive action uses.

The one thing lost is a recovery route when the UI is unusable — a blind combo works even
if the display is broken. See §7 item 11.

### 3.6. Committing, validation and the Nyquist path

ENTER-short in an editor performs, in order:

1. Clamp the pending value to its range.
2. Write the mapped Modbus holding register **within the same task cycle** (§5.4).
3. For sensor settings, evaluate the Nyquist condition
   `pollingRate_kHz * 1000 >= 2 * max(0, Q * F + Adjust)`.
   - **Pass:** persist to NVS, clear bit *n* of register 30, ascend one level.
   - **Fail:** do **not** ascend. Push the Nyquist override screen showing
     "Sampling too slow. UP=Edit values, DOWN=Save anyway."
     - **UP** returns to the editor with the pending value intact.
     - **DOWN** forces the save, sets bit *n* of register 30, and ascends one level.
4. For device settings that change the Modbus link (C1–C4), stage the value and require
   the operator to commit the link change explicitly — see A1.

### 3.7. Screen and page identifier scheme

**Proposed change (R2).** The original proposal named sub-menus `D0`, `E0`, `F0`, …. With
eleven editors that exhausts the alphabet and collides with identifiers already in use
(`P`/`C`/`S` in the requirements, `A`–`H` in the decisions register). Instead, an editor's
ID is **derived** from its parent page:

| Level | Page ID | Screen ID |
| --- | --- | --- |
| Info page | `P8` | `info-p8-factory-reset` |
| Confirm | `P8.C` | `confirm-factory-reset` |
| Toast | `P2.T` | `toast-totals-reset` |
| Config root | `C1` | `config-c1-modbus-id` |
| Value editor | `C1.V` | `config-c1-modbus-id-edit` |
| Sensor list | `SEN3` | `config-sensor-3` |
| Sensor settings | `S2` | `config-s2-multiplier` |
| Value editor | `S2.V` | `config-s2-multiplier-edit` |
| BACK entry | `<parent>.BACK` | `config-<level>-back` |

No lookup table is needed, and the exporter can verify mechanically that every setting
page has a matching `-edit` screen and every confirm screen a matching toast.

### 3.8. Two kinds of timeout

The schema has one `trigger.type: "timeout"`, but this model needs two behaviours that
must not be conflated:

| Kind | Requires a button held | Aborts on release | Used by |
| --- | --- | --- | --- |
| **Hold countdown** | yes (ENTER) | yes | confirm screens (§3.3) |
| **Auto timeout** | no | n/a | acknowledgement toasts (§3.3), idle timeout |

Today `InteractionHandler` treats every `FlowTrigger::Timeout` as a hold countdown, so a
2 s toast authored with the current schema would require the operator to keep ENTER pressed
for it to dismiss — the opposite of intended.

**Proposal:** add a discriminator to the timeout trigger, e.g.
`{ "type": "timeout", "durationMs": 2000, "holdButton": null }`, where `holdButton` is
`"enter"` for a hold countdown and `null` for an auto timeout. The exporter emits it as a
`FlowButton` on the flow (`FlowButton::None` meaning auto), so the firmware can dispatch on
one field without a second trigger enum.

---

## 4. Firmware Requirements

1. Replace `UiPage` with a **navigation stack**: `UiNavNode { levelId, pageIndex }` with a
   fixed maximum depth of 5. `screenForMode()` resolves the current level + page index to
   an exporter screen ID.
2. Descend on ENTER-short by pushing the target level; ascend on `BACK` or editor commit
   or discard by popping; ENTER-long clears the stack to L0/P0.
3. Add `UiEditorState { const FirmwareValueDescriptor* setting; int32_t pending; int32_t saved;
   uint32_t holdStartMs; uint8_t accelTier; }`, populated on entering an editor.
4. Implement the three acceleration tiers of §3.4 from `pressedDuration()`, so the tier
   follows the physical hold rather than repeat-event counting.
5. Extend `UiRenderContext` with the fields the `config.*` bindings need (decision B2):
   live link settings, current level and page, the editor's pending and saved values, the
   sensor index implied by the navigation stack, and Nyquist prompt state.
6. Commit path per §3.6, including register write, NVS persistence, and register 30
   maintenance.
7. Countdown machinery is reused from the existing hold-to-confirm implementation; the
   arming site moves from the info page to the confirm screen.
8. Add an **auto-timeout** path alongside the hold countdown (§3.8): a timer started on
   screen entry that fires its flow without requiring a button, used by toasts.
9. `UiScreenRouter` gains per-level screen tables, each guarded by `static_assert` against
   its page-count enum, matching the existing `kInfoScreenIds` pattern.
10. **Retire `FactoryResetState`** and the UP+DOWN combo machinery from
    `InteractionHandler`. Replace with: UP+DOWN short press → `UiController::enterIdle()`.
    Factory reset becomes an ordinary confirm-screen action via `core.action.factory-reset`,
    which is already implemented and registered.
11. No dynamic allocation; the navigation stack and editor state are fixed-size members.

---

## 5. UI / UX Requirements

- Every level fits 240 × 135 without scrolling. A level with more entries than fit shows a
  scroll indicator (`scrollbar` element, decision C1(b)).
- Header shows the current level and page (e.g. "Config ▸ Sensor 3 ▸ Multiplier").
- Footer shows the gesture legend for the current screen type, e.g.
  `↑↓ adjust · ENTER save · hold ENTER discard`.
- The pending value uses the highlight colour; the saved value is muted.
- `BACK` renders as a distinct entry (e.g. `◀ BACK`) so it is not mistaken for a setting.
- Confirm screens show remaining seconds centrally and the hint "Release to cancel".
- Every info page carries a footer hint, and the LED legend moves to P0 (decision H6).
- All strings route through a single table for future localisation (§7).

---

## 6. Test Considerations

- **Exporter:** every setting page has a matching `-edit` screen; every level's ring is
  closed (each page reachable from its siblings) and terminates in `BACK`; no screen
  declares two flows with the same trigger + button + gesture (this currently exists — see
  §7 item 4).
- **Simulator parity:** the web mockup must reproduce the tree, the acceleration tiers and
  the commit/discard paths. A dataset-driven test should walk every level and assert the
  simulator and the exported flow tables agree.
- **Firmware:** navigation stack depth never exceeds 5; ENTER-long from depth 5 lands on
  P0; commit writes the expected register; Nyquist failure does not ascend.
- **Acceleration:** verify tier boundaries at 700 ms and 1.5 s, and that a short press is
  always ±1.
- **Boundaries:** numeric clamping at both ends; enum wrap-around; int16 negative values
  for S3 Adjust.
- **Interruption:** a countdown must survive concurrent Modbus traffic (§7 reliability).

---

## 7. Open Questions / Follow-Up

1. ✅ **Q1 answered** (§3.5): UP+DOWN short press turns the display off. ENTER-long is
   therefore unambiguously "escape" at every level.
2. ✅ **Q2 answered** (§3.1): `BACK` is a screen in the ring, not a highlighted row.
3. ✅ **Q3 answered** (§3.3): resets move to a confirm screen with inverted gestures
   (short = exit, long = confirm) plus a 2 s acknowledgement toast.
4. **Latent dataset bug (R7):** `config-s1-connected` declares two `button/enter/long`
   flows. `InteractionHandler::matchFlow` returns the first, so the second — §5.3 rule 5's
   "exit immediately for an inactive sensor" — is dead. Fix the dataset and add the
   duplicate-trigger exporter check from §6.
5. **Per-setting descriptors have no home yet (R4).** The dataset has no min, max, step or
   enum list for any setting. Proposal: extend the manifest's `FirmwareValue` with
   `min`, `max`, `step`, `enum` and `unit`, so one declaration drives the simulator and the
   firmware. This ties into decisions **D2** (generate the manifest from firmware) and
   **E1** (fix the register arithmetic).
6. **§5.3 rule 5** ("ENTER long at S1 for an inactive sensor exits immediately") is
   redundant under this model — `BACK` and ENTER-long both already leave. Propose deleting
   it.
7. **Depth cost:** reaching `S2.V` from P0 is five ENTER presses. ENTER-long-to-P0 makes
   the return one press. Confirm the depth is acceptable, or consider hoisting the most
   used sensor settings.
8. **Documents to amend once accepted:** `Display_UI_Requirements.md` §2 (gesture table —
   the UP+DOWN row changes meaning entirely), §4.1 (state machine), §4.3 (P2–P8 ENTER
   semantics, new P8 row), §5 (replaced wholesale), §6. Also `Project_document.md` §5.3
   item 4 and `RGB_LED_Behavior.md` line 70, both of which name the retired "UP+DOWN 30 s"
   combo when describing LED suppression during the reset countdown — the behaviour
   survives, the trigger changes. Diagrams: `ui_state_machine.mermaid`,
   `ui_config_layout.mermaid`, `ui_sensor_submenu.mermaid`.
9. **Hold duration on confirm screens (§3.3).** Recommendation: 3 s for `Reset totals?`,
   plain ENTER-long for `Reset session?`, 30 s for `Factory reset?`. Confirm or set your
   own durations.
10. **Is UP+DOWN a toggle or off-only?** §4.1's "any button wakes" already covers turning
    the display on, so the gesture only ever needs to mean "off". If you want a true
    toggle, "any button wakes" has to be narrowed, which costs the operator the ability to
    wake the device with whichever button is nearest. Recommendation: off-only.
11. **Recovery when the display is unusable.** The retired UP+DOWN combo worked blind; a
    menu entry does not. If a bricked-display recovery route matters, options are: keep a
    blind combo as a hidden fallback (e.g. all three buttons held 30 s), or rely on the
    Modbus master (register 20 already does "Master Reset All Configs"). Recommendation:
    rely on register 20, since a device with a dead display is already a bench repair.
12. **Should P8 live at the info level or inside Config?** As specified it is a sibling of
    P7, so ordinary page flipping surfaces "Factory Reset" to any operator. Putting it in
    the Config root instead adds one deliberate descent. Neither is access control — the
    30 s hold is the real guard. Recommendation: follow the answer given (info level), and
    make the page title unambiguous (`FACTORY RESET`, not `Reset`).
