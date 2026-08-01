# Gesture Reference

**Version:** 1.1
**Date:** 2026-08-01
**Status:** reference — describes both what is *specified* and what is *built*, and these are
not yet the same thing.

> Every timing in this document is quoted from the source, not from a requirement. Where a
> requirement and the code disagree, the disagreement is recorded rather than smoothed over,
> because a reference that hides a gap is worse than no reference: it tells the next reader a
> gesture works when the device will not respond to it.
>
> **1.1** — v1.0 named five broken gestures. All five were repaired within hours of it being
> written, so the table it was proud of was wrong by the end of the same day, and one entry in
> this note was itself wrong on the first attempt. That is the honest lesson of this file:
> **a hand-maintained status column has a half-life measured in hours.** Every ✅ below now
> names the test that earns it, so the column is checkable by running `test/host/run.sh`
> instead of by trusting this paragraph. The one remaining ❌ (§8.1) is the acknowledgement
> toast, which v1.0 mentioned but did not tabulate.

**Normative sources.** `Display_UI_Requirements.md` §3 (gesture contract), §5 (navigation
tree); `NF-20260730-01-menu-navigation-model.md`.
**Implementation.** `src/input/button_input.{h,cpp}` (event model),
`src/input/interaction_handler.{h,cpp}` (dispatch), `src/ui/core/ui_accel.h` (acceleration),
`src/ui/core/ui_text_editor.h` (text entry).

---

## 1. The device has three buttons

| Button | Library name | Role |
| --- | --- | --- |
| **UP** | `M5StamPLC.BtnA` | Previous sibling, or increase |
| **DOWN** | `M5StamPLC.BtnB` | Next sibling, or decrease |
| **ENTER** | `M5StamPLC.BtnC` | Descend, commit, confirm — meaning depends on screen type (§4) |

There is no fourth input, no touch and no rotary encoder. Every interaction below is built
from these three, which is the single constraint that shapes the whole UI. `BACK` is not a
button: it is a **screen** you navigate onto and press ENTER on (§5).

---

## 2. Timings, as coded

| Constant | Value | Where | Meaning |
| --- | --- | --- | --- |
| `kLongPressThresholdMs` | **1500 ms** | `button_input.h:41` | Boundary between a short and a long press |
| `kRepeatIntervalMs` | **250 ms** | `button_input.h:42` | Repeat cadence once a hold passes the threshold |
| `kDisplayOffComboMaxMs` | **1000 ms** | `interaction_handler.h:66` | UP+DOWN must be released within this to count as "short" |
| `kIdleTimeoutMs` | **120 000 ms** | `ui_controller.h:156` | Inactivity before the display sleeps |
| `kEnterIdleHoldMs` | 3000 ms | `interaction_handler.h:55` | — |
| `kFactoryResetHoldMs` | 30 000 ms | `interaction_handler.h:52` | The **retired** blind combo (§7) |

A short press fires on **release**, not on press. That is deliberate: it means a press that
turns out to be a hold never also registers as a tap. A long press fires **once** at the
threshold, while the button is still down.

### 2.1. Acceleration, in a numeric editor only

`ui_accel.h:28`. Applies to UP/DOWN held **in a value editor**, never to navigation.

| Held for | Step | Repeat every |
| --- | --- | --- |
| < 700 ms | ±1 | 250 ms |
| 700–1500 ms | ±5 | 150 ms |
| ≥ 1500 ms | ±25 | 150 ms |

A deliberate tap never reaches the first interval, so **a short press is always exactly ±1**.

---

## 3. The gesture table

Status is measured, not assumed: ✅ means a host test in `test/host/` exercises it,
⚠️ means it is implemented but unverified, ❌ means the device will not do it today.

### 3.1. On a navigation screen (info pages, config lists, sensor lists, BACK entries)

| Gesture | Effect | Status |
| --- | --- | --- |
| UP short | Previous sibling at this level, wrapping | ✅ `interaction_test` |
| DOWN short | Next sibling at this level, wrapping | ✅ `interaction_test` |
| UP/DOWN held | Repeats the sibling step every 250 ms | ✅ `interaction_test` |
| ENTER short | Descend into the current entry; on a `BACK` entry, ascend one level | ✅ `interaction_test` |
| ENTER long (≥1.5 s) | Escape to the main screen (P0), clearing the whole stack | ⚠️ |
| UP+DOWN short | Display off **and** reset navigation to P0, from any screen at any depth | ✅ `interaction_test` |

### 3.2. In a value editor

| Gesture | Effect | Status |
| --- | --- | --- |
| UP short / DOWN short | ±1 (exactly one step, always) | ⚠️ |
| UP/DOWN held | Accelerating adjust per §2.1 | ⚠️ |
| ENTER short | **Commit** and ascend one level | ⚠️ |
| ENTER long | **Discard** and ascend one level | ⚠️ |

An editor now opens **only** on the ten `-edit` screens, identified by their
`config.editor.pending` element. It used to open on the config *list* pages too, which
swallowed UP/DOWN paging through C1–C6 and S1–S4.

### 3.3. In a text editor (WiFi SSID, passphrase, MQTT fields)

`ui_text_editor.h`. The two editing commands are **members of the character ring**, not
gestures, because the gesture space is full — overloading UP+DOWN would break the display-off
combo that works from every screen at every depth.

| Gesture | Effect | Status |
| --- | --- | --- |
| UP short / DOWN short | Previous / next character in a 97-position ring | ✅ |
| UP/DOWN held | Accelerating scrub through the ring, per §2.1 | ✅ |
| ENTER short | Accept the character and advance; on `DEL` backspace; on `END` **commit** | ✅ |
| ENTER long | **Discard** and ascend — same meaning as every other editor | ✅ |
| UP+DOWN | Reserved for display-off. **Not** backspace. | ✅ |

The ring is space (0x20) through tilde (0x7E), 95 characters, plus `DEL` and `END`. It opens
on `END`, so leaving a value unchanged costs one press. Cost is honest and measured:
**381 short presses for a 14-character passphrase** by single-stepping, hence the
acceleration. A secret shows `*` for committed characters and the character under the cursor
in clear.

### 3.4. On a confirm screen (destructive actions)

The logic **inverts** here, deliberately: a slip must not destroy data.

| Gesture | Effect | Status |
| --- | --- | --- |
| ENTER short | **Exit** without acting | ✅ `interaction_test` |
| ENTER held for the screen's declared duration | **Confirm** the action | ✅ `interaction_test` |
| Release before the duration elapses | Abort; the exit flow runs instead | ✅ `interaction_test` |

Declared durations, from the generated table: reset totals **3000 ms**, reset session
**1500 ms**, factory reset **30 000 ms**.

### 3.5. While the display is off

| Gesture | Effect | Status |
| --- | --- | --- |
| Any button | Wake. The press wakes only — it does **not** also act on the screen. | ✅ |

The UI is always at P0 on wake, because going idle clears the stack and discards any pending
edit. That was **not** true until 2026-08-01; see §8.5.

---

## 4. Why ENTER means different things

This is the most common source of confusion, and it is intentional. With three buttons, ENTER
has to be overloaded, so its meaning is a property of the **screen type**:

| Screen type | ENTER short | ENTER long |
| --- | --- | --- |
| Navigation | Descend (or ascend, on `BACK`) | Escape to P0 |
| Value editor | Commit and ascend | Discard and ascend |
| Text editor | Accept character / commit at `END` | Discard and ascend |
| Confirm | **Exit** without acting | **Confirm** the action |

The pattern: **short is the safe action, long is the committing action** — except on a
confirm screen, where short is the safe action *because* it does nothing. Reading it as
"long always commits" is wrong in the one place it matters.

---

## 5. `BACK` is a screen, not a button

Each navigation level's ring ends with a `BACK` entry. You reach it by cycling UP/DOWN like
any sibling, then press ENTER to ascend. This exists because there is no fourth button, and
because ENTER-long is already spent on "escape to the top" — so a one-level ascent needed
somewhere to live.

Consequence for anyone authoring a menu: **every level must include a `BACK` entry**, or that
level is a trap reachable only by escaping all the way to P0.

---

## 6. What each screen class actually declares

Counted from the generated table (`src/ui/generated/GeneratedUi.cpp`), so this is what the
device really has:

| Screen class | Screens | Declared flows |
| --- | --- | --- |
| Info pages | 9 | UP/DOWN short, ENTER short (7 of 9), ENTER long |
| Config lists + editors | 31 | UP/DOWN short, ENTER short, ENTER long |
| Confirm | 3 | ENTER short (exit) + a Timeout flow carrying ENTER and a duration |
| Toast | 2 | A Timeout flow with **no** button — an automatic dismissal |
| **Hold-gesture flows** | — | **0 across all 48 screens** |

Two notes a menu author needs:

- **`info-p0-global-status` and `info-p1-instant-flow` declare no ENTER-short flow.** ENTER
  does nothing on the two most-visited screens. That may be intended (nothing to descend
  into) but it is worth knowing before wondering why the button feels dead.
- **No screen anywhere declares a `hold` gesture**, which is why §8.2 exists.

---

## 7. Retired gestures

| Gesture | Was | Status |
| --- | --- | --- |
| UP+DOWN held 30 s | Blind factory reset | **Retired and removed** (2026-08-01). Three documents had claimed this for weeks while the code still had it — and it was a hazard, not merely dead: §3.1 tells the operator to press UP+DOWN to blank the display, and holding it 3 s raised a factory-reset countdown on the same two buttons. `interaction_test` now holds UP+DOWN for 36 s and asserts nothing is wiped. |
| `UiMode::Configuration` | A separate mode for the config UI | Retired in favour of the navigation tree. The renderer's fast-repaint gate no longer depends on it — it is driven from actual interactivity instead. |

---

## 8. Gaps, and what closed

### 8.1. Still broken: acknowledgement toasts never appear ❌

Two screens (`toast-totals-reset`, `toast-session-reset`) declare a `Timeout` flow with **no**
button — an automatic dismissal after 2 s. Nothing drives it. There is no screen-entry timer,
so the flow never fires.

`Display_UI_Requirements` §4.3.1 and §6.6 describe the toast as delivering "proof the action
happened rather than an unexplained screen change", and §6.6 certifies that requirement
satisfied. It is not. Completing a reset currently ascends straight to the parent, so the
operator gets the screen change without the proof.

Needs two things that do not exist: a screen-entry timer, and a navigator replace-at-depth
primitive — without the latter the toast's own `nav.back` would return to the confirm screen
rather than to the page the operator started from.

### 8.2. Unverified, not broken ⚠️

ENTER-long escape-to-P0, and the numeric editor's adjust/commit/discard. All implemented and
all plausible; none has a host test yet, so they carry ⚠️ rather than ✅. The harness exists,
so this is a gap in coverage rather than in capability.

### 8.3. Closed on 2026-08-01 ✅

Five defects, all listed as ❌ in v1.0 of this document and all repaired the same day:

| Was | Now |
| --- | --- |
| Confirm screens could not be confirmed — zero of 48 screens matched the arming predicate, which still looked for a two-screen page→overlay model | Arms on ENTER down against the confirm screen's own `Timeout` flow |
| The blind UP+DOWN 30 s factory reset was live, and armed on the same buttons §3.1 uses for display-off | Removed, with a test that holds UP+DOWN for 36 s and asserts nothing is wiped |
| Holding UP/DOWN on a navigation level stepped nothing, while the browser preview paged | Both resolve a repeat as a repetition of the Short flow |
| The value editor opened on config **list** pages, swallowing UP/DOWN paging | Requires a `config.editor.pending` element |
| Everything repainted at 1 Hz, because the fast cadence was gated on a retired mode | Driven from actual interactivity |

Also closed the day before: going idle — by gesture **or** by the 120 s timeout — did not clear
the navigation stack or the pending edit. The rendered screen comes from the navigator, not
from the page, so the display woke on whatever screen it was left on with a **live editor**:
the first ENTER could commit a Modbus write nobody confirmed. §3 and the code's own comment
both claimed otherwise.

---

## 9. Testing a gesture

`test/host/run.sh` runs the device harness: the real `UiController`, `UiNavigator`,
`InteractionHandler`, `ButtonInputManager` and `ui_actions` against the real 48-screen table,
with only `Preferences`, `M5StamPLC` and `ModbusMessage` stubbed. No hardware, no PlatformIO.

```cpp
Device dev;
dev.boot();
dev.tap(ButtonInputManager::Button::Down);   // one short press
dev.tapUpDown();                             // the display-off combo
dev.press(ButtonInputManager::Button::Enter, true);
dev.tick(1600);                              // past the 1.5 s threshold
```

Four traps, each of which produced a confident false pass while this harness was being built:

1. **Seed the navigator** (`navigator().reset(...)`, as `firmware.cpp:327` does). Without it
   `current()` is null and every gesture is a silent no-op — "20 down then 20 up returns to
   the start" passes over a frozen device.
2. **Call `controller.update()`**, not just the input calls. `updateIdleState` lives there, so
   a harness that only drives buttons can never fail an idle test.
3. **Wire `deps.modbus`.** `handleFlowEvent` returns early without it, swallowing every event.
4. **ENTER on P0 does not descend.** P7 is the configuration entry.
