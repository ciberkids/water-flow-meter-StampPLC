# Requirement: Hierarchical Menu Navigation Model

**Version:** 0.1 (proposal)
**Date:** 2026-07-30
**Status:** proposal — supersedes `Display_UI_Requirements.md` §5 and amends §2 and §4.3
**Supersedes decision:** D1 in `docs/active_work/open_decisions.md`

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
L0  Info mode ................ P0 P1 P2 P3 P4 P5 P6 P7          (no BACK — this is the root)
     │
     ├─ P2/P3 ENTER-short ──▶ L1  Confirm: Reset Totals         (hold-to-confirm, 30 s)
     ├─ P4/P5/P6 ENTER-short▶ L1  Confirm: Reset Session        (hold-to-confirm, 3 s)
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
| ENTER short | descend into the highlighted entry; on `BACK`, ascend one level | **commit** and ascend one level | no effect |
| ENTER long (≥1.5 s) | escape to the main screen (P0) | **discard** and ascend one level | — |
| ENTER held | — | — | run the countdown; release aborts |
| UP + DOWN held 30 s | factory reset (unchanged, §2) | factory reset | factory reset |

> **Proposed change to the original model (R1).** The original proposal put a 3 s
> hold-to-discard countdown on the editor's ENTER-long. This proposal discards
> **immediately** instead. Discarding an unsaved edit is harmless — the operator simply
> re-enters and tries again — so a 3 s hold buys no safety while adding friction, and it
> inverts the convention used everywhere else, where holding *commits* the dangerous
> action. Countdowns are reserved for irreversible operations: §3.3.

### 3.3. Countdowns are only for irreversible actions

A hold-to-confirm countdown appears on a **confirm screen**, never on a navigation or
editor screen. Releasing ENTER before zero aborts; UP/DOWN have no effect (§4.3 note 2).

| Confirm screen | Reached from | Duration | Action at zero |
| --- | --- | --- | --- |
| Reset Totals | P2, P3 (ENTER-short) | 30 s | `core.action.reset-all-measured` |
| Reset Session | P4, P5, P6 (ENTER-short) | 3 s | `core.action.reset-session` |
| Factory Reset | UP+DOWN combo, any screen | 30 s | `core.action.factory-reset` |
| Nyquist override | sensor editor commit failure | — | see §3.6 |

> **Proposed change (Q3).** In the current requirement P2–P6 arm their countdown with
> ENTER-**long**, which collides with ENTER-long as the escape gesture. Moving the
> countdown behind an ENTER-short descent into a confirm screen restores one meaning per
> gesture, and it makes the confirmation visible as an authorable screen. It also
> resolves decision **H1**.

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

### 3.5. Reaching Idle

**Proposed answer to Q1/H1.** ENTER-long is the escape gesture, so it cannot also mean
Idle. Idle is reached two ways:

1. **Automatically** after 120 s without a button press (unchanged, §4.1).
2. **Manually** from `P0` only, where ENTER-long has no level to escape to and therefore
   means "display off".

At any deeper level ENTER-long escapes to P0; a second ENTER-long there goes Idle. This
keeps one meaning per gesture and makes manual idle reachable from anywhere in two
presses. It also settles **H3**: no back-to-idle countdown, because going idle is
reversible by any button press.

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
| Config root | `C1` | `config-c1-modbus-id` |
| Value editor | `C1.V` | `config-c1-modbus-id-edit` |
| Sensor list | `SEN3` | `config-sensor-3` |
| Sensor settings | `S2` | `config-s2-multiplier` |
| Value editor | `S2.V` | `config-s2-multiplier-edit` |
| BACK entry | `<parent>.BACK` | `config-<level>-back` |

No lookup table is needed, and the exporter can verify mechanically that every setting
page has a matching `-edit` screen.

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
7. Countdown machinery is reused unchanged from the existing hold-to-confirm
   implementation; only the arming site moves from the info page to the confirm screen.
8. `UiScreenRouter` gains per-level screen tables, each guarded by `static_assert` against
   its page-count enum, matching the existing `kInfoScreenIds` pattern.
9. No dynamic allocation; the navigation stack and editor state are fixed-size members.

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

1. **Q1 answered by proposal** (§3.5): ENTER-long escapes; Idle is manual from P0 only.
   Confirm or override.
2. **Q2 answered by proposal** (§3.1): `BACK` is a page in the ring, not a highlighted row
   in a list, because the existing dataset is already one-setting-per-page and no cursor
   concept exists. Confirm.
3. **Q3 answered by proposal** (§3.3): info-mode resets move from ENTER-long to
   ENTER-short-into-a-confirm-screen. Confirm.
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
8. **Documents to amend once accepted:** `Display_UI_Requirements.md` §2 (gesture table),
   §4.1 (state machine), §4.3 (P2–P7 ENTER semantics), §5 (replaced wholesale), §6;
   `docs/diagrams/ui_state_machine.mermaid`; `docs/diagrams/ui_config_layout.mermaid`;
   `docs/diagrams/ui_sensor_submenu.mermaid`.
