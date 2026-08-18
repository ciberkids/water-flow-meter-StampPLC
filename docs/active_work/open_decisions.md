# Open Decisions

What is genuinely undecided or unbuilt, and nothing else.

**This file is the single source of truth for open work.** `README.md` § Source of truth says so, and
`MEMORY.md` §6 points here rather than keeping a list of its own — it kept one until 2026-08-18, and the
two drifted, which is the whole argument. If you find an open item recorded anywhere else, move it here,
give it an ID under rule I3, and leave a pointer behind.

The previous register carried 42 entries. **Every one of them had a recorded `Decision:` line**, and
41 of the 42 are implemented — but the status emoji in each heading still said 🔴 *blocks
implementation now* or 🟡 *blocks a later slice*, because the emoji was a second home for a fact that
lived in the Decision line and nobody moved it. A register that says forty things are blocking when
one is does not get read, which is the failure this rewrite is undoing.

The closed entries are kept verbatim, with their questions, options and reasoning, in
[`../archive/open_decisions-closed-2026-08-12.md`](../archive/open_decisions-closed-2026-08-12.md).
That history is worth keeping — several entries record a decision being *reversed* — but it is
history, not a work list.

Status legend: 🔴 blocks work now · 🟡 blocks a later slice · ⏸️ waiting on something external

---

## The index — cite the ID, not the heading

**Nineteen open lines.** No 🔴 — DF17 closed 2026-08-18. Ask for work by ID — "fix J3", "decide
DF10", "DF19 go ahead" — and this file is the one place that says what an ID means. Rule **I3** below governs them; the short version is that
they are append-only and never reused, so a gap means an item closed, not an item lost.

The **Shape** column is the one that answers *can I just say go ahead?*

| ID | | Shape | What it is |
| --- | --- | --- | --- |
| **G1** | ⏸️ | measurement | The 3.3 kHz polling rate has never been measured on a board; the procedure is written down and waiting |
| **N-d2** | ⏸️ | correction + measurement | Nothing protects the VLF probe's position above `M5StamPLC.begin()`, and whether the RTC survives power loss is unknown |
| **DF10** | 🟡 | **decision** | The portal indexes sensors from 0, `ui::writeSetting` from 1 — decide before a `PortalSettingStore` exists, not after |
| **DF14** | 🟡 | **decision** | `configIsValid`'s offset bound is ten times what its own comment promises — tighten the bound or correct the comment |
| **DF15** | 🟡 | **decision** | Three registers §5's table documents do not exist, and 711 is double-booked — build them or move them out of the table |
| **DF18** | 🟡 | **decision** (design) | `nyquist-warning`'s `option-down` sits inside the banner band; fixing it means authoring the screen's first spec file |
| **N-d1** | 🟡 | feature, queued | The clock can be set from no route at all; next step is the Modbus date/time block |
| **N-c** | 🟡 | feature, queued | MQTT is report-only — §4.4.1's command topics are unbuilt, and register 565 reports results that cannot arrive |
| **N-b** | 🟡 | feature, queued | Growing the settings catalogue silently invalidates authored menu packs; only the generator notices |
| **J1** | 🟡 | gate, unbuilt | No export gate proves a level's DOWN ring closes — and paging wraps in the dataset, not in code. Same family as N-b |
| **I2a** | 🟡 | gate, unbuilt | Nothing enforces I2's append-only catalogue rule; it is honour-system prose |
| **DF16** | 🟡 | correction, deferred on scope | The simulator stores `ready` where the device derives it; the fix is known and is a refactor of `sensorConfig.ts`'s public shape |
| **DF19** | 🟡 | correction, one number | Six shipped scrollbars are 104 px, not 100; blocked only on accepting that `--write` regenerates 72 of 80 screens |
| **J2** | 🟡 | correction, one round | `animation` was dropped by C1 and still survives in five layers, including bytes emitted into the firmware header |
| **J6** | 🟡 | gate, unbuilt | Nothing verifies the workspace is accessible — no axe/pa11y/lighthouse in the workspace |
| **DF20** | 🟡 | gate, unbuilt | No snapshot drives a warning state, so the repaired visual suite still never renders the banner |
| **DF21** | 🟡 | gate, unbuilt | CI runs no `test:visual` step — the reason DF17 stayed invisible |
| **J7** | 🟡 | correction, one round | The transition preview is off by decision; its five layers remain, same shape as J2 |
| **J8** | 🟡 | **decision** | Two clamps disagree — the Design panel pins the coordinate, the importer pins the far edge |

**DF1–DF9, DF11–DF13** are fixed and keep their IDs, struck through in place below. **I2** and **I3** are
standing rules that never close.

---

## G1 ⏸️ Polling rate on real hardware is still unmeasured

**Decided, not verified.** M5StamPLC 1.2.0 removed the bulk `IO.getDigitalInput()`, so the sampler
reads the expander per channel. The decision was "measure first, pursue a bulk expander read only if
the measurement demands it" — and the measurement needs a board.

Everything is in place to take it: the achieved rate is published in `REG_POLLING_RATE_KHZ`
(register 0) and on the MQTT diagnostics topic beside the baseline it should be compared against, and
`REG_UNDERSAMPLING_FLAGS` (register 30) names any channel outrunning the sampler.

**What to do when the board arrives.** Flash, read register 0, compare with `baselineKhz`. If the real
rate is materially below it, the bulk-read work becomes real and the per-channel `q_max` limits need
re-checking against what the sampler can actually count.

**Note, 2026-08-17.** `meetsNyquistLimit` now refuses a ceiling of zero or less instead of accepting it,
and `configIsValid` demands a positive multiplier. Neither touches this: both are about configurations
that have no sensible ceiling at all, not about what the ceiling is compared against. The 3.3 kHz is
still assumed, and it is now also the simulator's default dial (`kDefaultPollingRateKhz`), so the
mockup's "inside budget" statements inherit the same assumption.

**Blocks.** Trusting the sensor configuration limits on hardware. Nothing in software.

---

## N-b 🟡 Growing the settings catalogue silently invalidates authored menu packs

`Loadable_UI_Menu_Packs.md` §3.0.1 requires every menu pack to expose an editor for every
`category: "setting"` value. The rule is right and it has never survived growth: the WiFi and MQTT
work took the required editor count from 10 to 24, and this session's Modbus/Display/Sensors
restructure moved every config screen id.

**What actually enforces the rule today:** `assertCoversEverySetting()` in
`web/mockup/tools/skeleton/generate.mjs`, which fires when the default dataset is regenerated. That
is the cheapest possible place to find out, and it is the only place. Neither the export gate of
§5.8 nor the load-time patcher of §3.3.11a exists.

**Recommendation** (Q4 of that document): the manifest carries a catalogue version, and the exporter
*warns* rather than fails for values added after a pack's version. Until that exists, a catalogue
addition means regenerating the default menu — which is fine for the built-in pack and offers nothing
at all to a third-party pack on an SD card.

**Blocks.** Shipping menu packs as a supported extension point. Not the built-in pack.

---

## N-c 🟡 MQTT is a REPORT-ONLY surface, so "every setting is writable from every route" is not true of it

Recorded because the four-surface question keeps being asked as though MQTT were one of the four. It is
not, today. The only subscription is `<prefix>/status` — Home Assistant's birth message — so nothing a
broker sends can change a setting. The command topics are specified in
`../Requirements/feature addition/WiFi_MQTT_Connectivity.md` §4.4.1 and are **not built**, and
`kMqttLastCmdResult` (register 565) exists to report the result of a command that cannot yet arrive.

**This is a feature, queued by the owner, not a defect** — which is exactly why it belongs here rather
than in the defect list below. The three routes that DO write (panel, RS485, portal) agree, and RS485
remains the source of truth.

**One consequence worth stating:** the panel cannot edit text at all (there is no on-device text
editor), so SSID, broker host and topics are portal-or-Modbus only. That is a decision, not a gap, and
it means a device with no Modbus master and an expired provisioning window has no route to its own
network settings. The portal timer is the thing standing between an operator and a reflash.

---

## N-d 🟡 The clock — settable from no route at all, and its one trust signal unprotected

Recorded 2026-08-17, alongside N-c, because it is the same shape: a place where "every setting is
readable and writable over RS485" is not yet true. It is a queued feature, not a defect — but it was
never written down, and the four-surface question keeps being answered without it.

**N-d1 🟡 The clock can be set from no route at all** — the largest live gap against RS485.

`DeviceClock::setTime` has **no production caller**. Grepped 2026-08-17: the only callers are
`device_clock_test.cpp`, `modbus_manager_clock_test.cpp` and `interaction_test.cpp`. There is no
date/time block in `register_map.h` or `net_register_map.h`, no portal page offers one, no panel editor
reaches it (and could not — there is no text editor, per N-c), and NTP exists only in comments
(`wifi_manager.h` cites R7.13). So in the field the clock reads `UNSET` unless the RTC happens to hold a
date it is allowed to trust, and **a Modbus master has no way to set it.**

**Where the feature stopped**, in the owner's own ordering: the clock and its trust state are built,
a session reset is dated through `ModbusManager::Dependencies::clock`, and P3 shows the session start.
Next is the **Modbus date/time block** — the one that makes every later item settable — then the portal
page, then NTP on `WifiState::Connected`, with MQTT last because it needs the unbuilt command topics.

**N-d2 ⏸️ Nothing protects the VLF probe's position, and whether the RTC survives power loss is
unknown.** A second, sharper thing on the same subject, and the half that needs the board.
`M5StamPLC.begin()` clears the RX8130CE's whole flag register including VLF, and the library exposes
no reader, so
`plc::readRtcVoltageLowFlag()` must run **before** `M5StamPLC.begin()`. It is the first statement of
`setup()` and that is the only moment in the device's life when "did the clock run across the last power
cut" can be known. **No test or assert protects the ordering** — nothing under `test/` names
`rtc_boot_probe` or `readRtcVoltageLowFlag` (checked 2026-08-17). Move that line, or add anything
touching the RTC above it, and a device with a dead clock reports a healthy one and publishes a
confident year-2000 timestamp to the panel, Modbus and MQTT. Whether the RTC survives power loss at all
is **unknown** — the chip has a backup-supply pin, and M5Stack does not say whether a cell or supercap is
populated. Settle it empirically: set the time, pull power for a minute, boot, read VLF.

**Blocks.** Any timestamp being trustworthy in the field. N-d1 is a queued feature and needs nobody's
decision; N-d2 is a correction (the assert) plus a measurement (the power-cut test).

---

## I2 — standing rule, not a decision

**The value catalogue is append-only.** Renumbering or repurposing an existing entry breaks every
authored pack that references it, and breaks it silently — the id still resolves, to something else.

This is not open and never closes; it is recorded here because it constrains every future change and
a rule filed under "archive" is a rule nobody reads. Same class as the wire-encoding rules on
`WifiState` and `kMqttFlags` bit 2.

**I2a 🟡 Nothing enforces it.** Verified 2026-08-18: no file in the repository contains
`append-only` or `appendOnly` as a check — grepped across `*.ts`, `*.mjs`, `*.cpp`, `*.h` and `*.json`.
The rule lives entirely in prose, in a file the person renumbering a catalogue entry has no reason to
open. What would catch it is a checked-in snapshot of `id → meaning` that CI diffs, the same shape as the
byte-for-byte `screens.json` diff already in CI: a changed line under an existing id fails, a new line
appended passes. Filed here rather than as a defect because the rule and its missing enforcement belong
in one place — but it is real work, and until it exists I2 is honour-system.

---

## I3 — standing rule, not a decision

**Every item in this register carries a stable ID, and the IDs are append-only.** Cite the ID, never
the heading: headings get rewritten as a diagnosis sharpens, and a register described in prose cannot
be pointed at — which is how "go ahead and fix it" stops meaning anything. No count belongs in this
rule; the index carries it.

Same shape as I2, and for the same reason: an identifier that silently moves is worse than none.

**What the prefixes mean.** They are the *thematic groups* of the original 42-entry register, lettered in
the order the groups were opened — not category codes. Every item's own text is in the closed table at
the bottom of this file, verbatim in
[`../archive/open_decisions-closed-2026-08-12.md`](../archive/open_decisions-closed-2026-08-12.md).

| Prefix | Group | Range | Live? |
| --- | --- | --- | --- |
| `A` | requirements rewrites | A1–A6 | closed |
| `B` | bindings | B1–B2 | closed |
| `C` | UI mechanics | C1–C3 | closed |
| `D` | display and dataset | D0–D5 | closed — **and this is why defects are `DF`** |
| `E` | the manifest | E1 | closed |
| `F` | repository hygiene | F1–F7 | closed |
| `G` | hardware risk | G1–G3 | **G1 open**; G2, G3 closed |
| `H` | menu behaviour | H1–H6 | closed |
| `I` | menu packs — and where the standing rules ended up | I1–I3 | **I2, I3 are standing rules**; I1 closed |
| `N-` | the batch opened after the 2026-08-12 rewrite, lettered `a`–`d` so it could not collide with the numbered groups | N-a–N-d2 | **N-b, N-c, N-d1, N-d2 open**; N-a closed |
| `DF` | defect — opened 2026-08-18 | DF1–DF21 | **eight open**, thirteen fixed |
| `J` | residue and hygiene — opened 2026-08-18, consolidated out of `MEMORY.md` §6 and `docs/backlog/` | J1–J8 | **five open**; J3, J4 and J5 fixed |

**`DF`, not `D`, and the reason is this rule's own point.** `D0`–`D5` already mean the display-and-dataset
group — they are in the closed table at the bottom of this file and throughout the archive. The defect
list was first stamped `D1`–`D19`, which made `D1`–`D5` resolve to two different things in one document.
That is precisely I2's complaint (the id still resolves, to something else), so the prefix was corrected
the same day, before anyone cited one. **A retired letter stays retired**: `A`–`I` and `N-` are spent, so
a future batch takes `J` or a new two-letter prefix, never a letter that once meant something else.

- **Never renumber, never reuse.** `DF1`–`DF19` are in the order the defects were found; the twelve that
  are fixed keep theirs, struck through in place. A gap in the open list means an item closed.
- **New items take the next free identifier** — `DF22` onward for defects, `J9` onward for residue, `N-e` onward for entries in
  the upper section. Never fill a gap.
- **A split keeps the parent and adds a suffix** (`N-d` → `N-d1`, `N-d2`), so an ID cited from outside
  this file still resolves. `G1`, `N-b`, `N-c` and `I2` are cited from `README.md`, four requirement
  documents and three source files — renaming one breaks a cross-reference that still looks valid.
- **Sub-IDs are bold lead-ins, not headings.** `###` carries item headings only (`DF`, `J`); a sub-line
  like `I2a` or `N-d2` is a bold lead-in inside its parent, so `grep '^### '` stays a census of items
  and never double-counts one.
- **The index at the top is maintained by hand and must match the headings.** Closing an item is two
  edits in one commit: strike the heading, delete the index row. An index that lists a fixed item is
  the same failure as the status emoji this register was rewritten to fix — one fact with two homes.

---

## Defects found — DF1–DF21, thirteen fixed and eight open

None is a decision — each is known, each has a diagnosis, and they are here because this is the file
that gets read before picking up work.

### DF1 — ~~A channel with no calibration could never be given one~~ ✅ FIXED 2026-08-15

`prepareConfigUpdate` refused every candidate failing `configIsValid`, and every register arm builds its
candidate from the config already in force and changes one field — so from an all-zero configuration both
entry orders were refused (`q_max` alone still fails on `f_multiplier == 0`, the multiplier alone on
`q_max == 0`, and the two cannot arrive in one single-register write). A channel with no valid
configuration could not be given one by any route: not over Modbus, and not from the panel's editors,
which route through the same gate.

It shipped unnoticed because a channel whose NVS already held a valid configuration kept working and
stayed editable. `OFF_CMD_RESET_CALIBRATION` is what made it reachable on purpose — the S.RESET row gave
the operator a way *into* that state, so S.RESET was a one-way door until this was fixed.

**Fixed** by accepting an invalid candidate only when the channel had no valid configuration to lose. A
validly calibrated channel still cannot be demolished field by field, so "not set" remains expressible
only as a command — the property `register_map.h`'s offset-25 note rests on. 54 new host checks;
mutation-tested.

### DF2 — ~~The pulses-per-litre form was flagged for a check it could not pass~~ ✅ FIXED 2026-08-15

`meetsNyquistLimit` computed the formula ceiling only and returned false on `f_multiplier == 0` — the
correct, normal state of a channel calibrated by pulses per litre. So every valid pulses-per-litre
channel failed the sampling check permanently: `evaluateSensorDiagnostics` ORs in `valid && !meets`, so
its bit in `REG_UNDERSAMPLING_FLAGS` stayed lit forever, the panel's warning path was engaged for a
channel inside budget, and the figures could only be installed by writing them twice through the §5.5
override handshake.

Register 30 lying is the half that is verified and asserted. What the engaged warning path actually
PAINTS is the two entries below: the banner is at y=34 rather than §2c's y=116 and is overpainted by the
screen's own rows, and its text reaches no bound element at all. Stated separately on purpose — the
register was wrong regardless of what the panel did with it.

**Fixed** with a per-form ceiling taken from the engine's own inversions: `f_multiplier*q_max + adjust`
for the formula, `K*q_max/60` for pulses per litre. Both directions pinned — in-budget accepted on the
first write with register 30 at 0, out-of-budget still refused and parked.

**Note for G1:** this changes *which* frequency the budget is computed from, not whether the budget is
right. The 3.3 kHz polling rate it is compared against is still unmeasured on hardware.

### DF3 — ~~A pulses-per-litre calibration written over Modbus was lost at every reboot~~ ✅ FIXED 2026-08-17

`saveSensorConfig` wrote three keys — `cfg_q`, `cfg_f`, `cfg_a` — and `SensorCharacteristics` has five
fields. `calibration` and `pulses_per_litre` were never persisted, so a master that wrote registers 123
and 124 saw them accepted and read them back correctly until the next power cycle, after which the
channel returned as `Formula` with no K figure, failed `configIsValid`, and reported `SET?` with 0.0 for
every measured register.

The litres were never lost — `cml_%u` round-trips. The zero at register 103 is DERIVED, by
`syncSensorToHolding`'s `inUse && configIsValid(...)` gate, which is correct and was left alone.

**Fixed** by moving the serializer to `modbus/sensor_config_nvs.h`, templated on the store, so it can be
host-tested at all: it lived in firmware.cpp, which is in no link set. 20 checks, mutation-proven, plus a
`static_assert` on the struct's size so a sixth field is a build failure.

### DF4 — ~~FC16 refused every address in the network block~~ ✅ FIXED 2026-08-17

`handleWriteMultiple` pre-validated with `isWritableAddress`, which knows only the sensor and link
registers — so every address from 500 up was refused and a block write anywhere in the network region
excepted on its first word, while FC6 at the same address worked. §5.2's whole "no AP, no portal, no site
visit" story was unreachable for any master that writes text as blocks.

The host check named for that requirement was green throughout: it drives `NetRegisterMap::stageWrite`,
one layer below the handler, and nothing in `test/` called the handler at all. `deps_.net` is null in the
one test that constructs the manager, which makes the entire network branch unreachable.

**Fixed**, with NET_APPLY lifted out of the write loop so a zero passed over at 730 cannot except.
28 checks through the real handler, mutation-proven.

**Found while testing, and now documented in §5:** FC16 caps one frame at 123 registers, because the
byte count is a single byte. The region cannot be written in one frame, so §5.1's "block write across the
whole region" is necessarily a sequence.

### DF5 — ~~12 of the portal password's 32 bytes were unreachable over RS485~~ ✅ FIXED 2026-08-17

`kPortalPassword` was at 720 claiming 16 registers, and `kApply` is at 730. So bytes 0..19 were
writable, a write aimed at byte 20 **committed the block**, and 731/732 were ignored — while the store,
`netFieldCapacity` and the web portal all said 32 bytes, and the portal really accepts 32.

**Fixed** by moving the field to 736 (the apply trio keeps the addresses §5 publishes) and leaving
720-729 reserved so a master built against the old prose is ignored rather than landing inside another
field. The real fix is three `static_assert`s: a text field that runs past the block, overlaps another
field, or overlaps a scalar is now a build failure. The audit had to check all ten fields by hand.

### DF6 — ~~A master could install a negative multiplier, or an offset cancelling the whole span~~ ✅ FIXED 2026-08-17

Both were accepted, both reported `OK` with register 30 clear. A negative multiplier is a negative
DIVISOR, so the channel read 0.00 at every frequency; an offset cancelling the span reached
`meetsNyquistLimit`'s `theoreticalFrequency <= 0.0` arm, which returned **true**, and the engine then read
full scale on a dry pipe.

The multiplier half needed no decision — `ui_settings_types.cpp` bounds the panel editor at 1..32767 and
the register wiki published that range — but the bound lived only in the editing stepper, so no route
enforced it. It is now in `configIsValid`, the one choke point all surfaces share.

**Partially fixed, and the remainder is the entry below.** Both refusals PARK rather than reject, so
§5.5's override still installs these figures on a second identical write.

### DF7 — ~~The warning banner is drawn where §2c decided it must not be~~ ✅ FIXED 2026-08-17

**§2c was decided and its firmware half had never been done.** `Display_Per_Screen_Spec.md` §2c rules
`bannerY = 116` so the banner covers the footer row; `ui_renderer.cpp` said `bannerY = 34`. Everything
downstream already assumed 116 — `tools/audit/screen-spec.ts` validates the 61 spec files under
`screens/` against `kBannerTop = 116` / `kBannerBottom = 134`, each declaring its footer hint as the one
element the banner may replace. So the gate protecting the band was checking a band the firmware did not
use, while the real band at y 34…52 was unchecked on all 80 screens.

**§2c called it one line. It was two, and the second is the interesting one.** `drawWarningBanner` ran
BEFORE `drawScreen`, and text elements paint with an opaque background, so the screen's own footer hint
punched a background-coloured hole straight through the banner. "The banner replaces the footer" was
false as a matter of draw order, not of coordinates.

**Fixed** by moving the coordinate AND the order: `drawScreen` → `drawWarningBanner` → countdown
overlay. `kPanelHeight = 135` is now named beside `kPanelWidth` and
`static_assert(bannerY + bannerH <= kPanelHeight)` makes "does the band still end on the panel" a build
failure. The countdown overlay stays LAST on purpose, so a modal's own `hold ENTER confirms` — the abort
gesture — outranks the banner while a destructive hold is running.

**The swap is NOT a no-op, and must not be recorded as one.** Eleven screens change: the five `toast-*`
screens and the six `confirm-*` screens *viewed before their hold begins* carry a full-screen
`overlay-bg` and are drawn through the ordinary path, so their box used to erase the banner and no longer
does. The toasts have no footer hint and lose nothing; a pre-hold confirm screen loses its hint, which is
the footer row §2c chose to spend. Whether the banner belongs on a 2-second acknowledgement toast at all
is a presentation question nobody has answered — it is now visible there for the first time.

**The simulator had no banner at all** until this round, while §2c documented its pixels. It has one now
(`web/mockup/src/utils/warningBanner.ts` plus a layer in `DisplayViewport`), painting above the screen's
elements exactly as the firmware now does. The mockup still has no countdown-overlay pass, so during a
hold the device covers the band and the mockup does not — left alone deliberately, because inventing the
missing half is the defect class this register exists to catch.

**And the gate that was missing now exists.** `tools/audit/screen-geometry.ts` — the only audit run over
the shipped `src/data/screens.json` — had no banner-band check at all, so `bannerY` could move with every
gate green. It has one now. See the two new entries at the end of this section for what it immediately
found.

### DF8 — ~~The commissioning gap reaches no summary line on the panel~~ ✅ RESOLVED 2026-08-17

The uncalibrated-channel work was complete in the firmware and reached the operator almost nowhere: the
two uncalibrated-only strings were composed every pass and never painted, because `drawWarningBanner`
returned early on `!hasWarnings`, which means sampling faults only.

**Decided 2026-08-17: widen the banner to cover it** — not bind a P0 status row, which was the other
option offered. The gate is now `UiRenderContext::bannerActive()`:

```
hasWarnings || (uncalibratedCount > 0 && !editorActive)
```

`hasWarnings` was NOT widened; it still means `REG_UNDERSAMPLING_FLAGS != 0`, which is what
`ui_bindings.cpp` reads it as. One shared predicate rather than two spellings of the condition, because
the banner and the `legend.warning` row colour print the same string and must agree about whether there
is anything to say.

**The `editorActive` term is a second owner decision, and it is the original objection resurfacing.**
`UiRenderContext`'s old comment argued that a factory-fresh device must not wear the banner over the very
screens that clear it. That was correct at y=34. Moving the banner to the footer row defused it for the
telemetry pages but NOT for the editors: **thirteen** `config-*-edit` screens carry
`UP/DN adjust  ENTER save  hold=cancel` at y=124, the only place the abort gesture is documented
anywhere, and with `uncalibratedCount` at 8 on a fresh device the banner would have hidden it
permanently, while the operator was calibrating — the one activity that clears the condition. So the
uncalibrated half suppresses itself while an editor is open. (The decision note said eighteen editor
screens. It is thirteen, counted 1:1 against the dataset.)

**Sampling faults are exempt from that suppression, on purpose.** `hasWarnings` says a reading is WRONG —
the number on the panel is not the flow — which outranks a gesture reminder on every screen. The
uncalibrated half says a setup is UNFINISHED, which can wait for the operator to look up from the setting
they are finishing.

**The widest summary was 50 characters against the 37 the band affords, and nothing had ever caught it.**
The sampling-only branch read `Sampling warning on sensors 1, 2, …, 8`; the 28-character prefix plus a
3k−2 list passed 37 at FOUR flagged channels. It overflowed silently, because `drawWarningBanner` prints
straight to the panel and gets none of `drawTextElement`'s `~` clipping — and the existing
`legendLen <= 37` guard passed for the wrong reason, its 0xFF/0xFF state routing to the *combined* branch.
Decided 2026-08-17: **trade the channel list for a count.** Every branch is now bounded — combined 33
(the widest), sampling-only 29, uncalibrated-only 25 — with the k=8 case pinned in both languages.
Channel identity is carried where it already was: the flagged rows are drawn in the warning colour and an
uncalibrated row says `SET?` itself.

**Standing, and not an oversight:** `telemetry.status` and `legend.warning` are still bound by NO dataset
element, so the exporter's `diagnostics-banner` check still emits its WARNING on every run. Rule I2 makes
the catalogue append-only, so deleting the two ids was never available, and the check was not rewritten to
pass on something it does not verify. One consequence worth stating: the banner's gate is true only when a
count is non-zero, so `All sensors nominal` and `No channels in use` are unreachable through the banner by
construction and reach only those two unbound ids.

**Carried forward:** `drawWarningBanner` still has no clipping. There are four columns of slack and a host
assertion on the composition, but nothing on the renderer side would catch a future over-long phrasing.

### DF9 — ~~The provisioning portal drops its own form submission~~ ✅ FIXED 2026-08-14

`PortalForm` renders `action="/save"`, and its header states the contract: *"the adapter must route
this exact path"*. `portal_server_arduino.h` registers `POST` on `/` only. ESP32's `Uri::canHandle` is
`_uri == requestUri`, so the submission reaches the catch-all, gets a `302` to `/`, and the browser
re-issues it as a `GET` — the configuration is discarded with no error shown.

**Impact.** The web portal is the only provisioning route for a device with no Modbus master. Modbus
provisioning is unaffected and works.

**Why no test caught it.** `PortalForm` has 226 host checks; they exercise the form against strings.
The routing lives in the Arduino adapter, which no host test can construct — the same blind spot its
own `collectHeaders` comment warns about.

**Fixed** by routing `PortalForm::kFormAction` for `HTTP_POST` in the adapter, alongside the existing `/`
handler. Firmware compiles; the host suite is unchanged because it cannot reach this code.

**Still open, and the reason this defect lasted:** routing lives in the Arduino adapter, which no host
test can construct, so nothing verifies it. A seam that let a fake server double be driven would have
caught it — and would catch the next one. That is a real gap, not a nicety: two of the three portal
defects found this year were in the adapter rather than the form.

### DF10 — The portal names sensors from 0 and the settings API counts them from 1 🟡

Found while auditing whether the three writing routes agree, and verified by reading both sides
2026-08-17. `portal_form.cpp` renders per-sensor fields as `<bindingId>@<n>` over
`0 .. sensorCount-1` (the loop at :726) and its parser refuses `n >= sensorCount_` (:829), so the
portal's index is **0-based**. `ui::writeSetting`'s `sensorIndex` is **1-based with 0 meaning
invalid** — `sensorSlot` in `ui_settings.cpp` returns `sensorCount` (the "no such slot" answer) for 0
and `sensorIndex - 1` otherwise.

**Latent today, and only by an accident of wiring.** No production `PortalSettingStore` is injected —
`firmware.cpp:244` passes `nullptr`, and its comment says that is the supported "not yet" rather than an
oversight — so every per-sensor portal row renders disabled and no such write can happen. But
`portal_form.h`'s `PortalSettingStore` invites the adapter to forward to `ui::writeSetting`, and the
first adapter written that way will drop sensor 1's write on the floor (index 0 is invalid) and land
every other calibration **one channel low**.

**Decide before the store is implemented, not after:** either make the portal 1-based so the two
agree, or state in `PortalSettingStore`'s header that the adapter owns the +1 and say so where the
implementer will read it. This is cheap now and is a data-corruption bug once a store exists — a
calibration silently applied to the wrong meter is exactly the class of error no operator can see.

### DF11 — ~~The Select Menu is reachable only by a hidden gesture~~ ✅ FIXED 2026-08-17

`Loadable_UI_Menu_Packs.md` §3.4 requires the firmware to append a Select Menu page to the end of the
root level, "always reachable by paging with UP/DOWN". §3.4.1 then argues the case explicitly: a hidden
gesture is *"precisely the anti-pattern we retired with the blind UP+DOWN factory-reset combo — nothing
on screen says it exists, nothing confirms you are partway through it, and it cannot be documented on
the device itself."*

**What shipped was the hidden gesture and not the page.** The root ring had nine entries — P0..P6, WiFi,
MQTT — and none was the selector; nothing in `UiNavigator` or `UiScreenRouter` appended one. The
UP+DOWN+ENTER 3 s recovery gesture worked and was tested, so the selector was reachable; it was simply
undiscoverable, which is the specific outcome §3.4.1 was written to prevent.

Found 2026-08-12 by someone reviewing the documentation and being unable to locate the UI-selection
menu — which is the failure mode in miniature: if a reader with the source open cannot find it, an
operator with only the panel certainly cannot.

**Decided 2026-08-17: build the root-level entry, and have the NAVIGATOR synthesise it** so packs cannot
remove or shadow it — the second half was asked and answered separately, because a dataset row is exactly
what a pack could drop.

`UiNavigator::rawNextWithTail` splices `ui::kSelectMenuScreen` (`src/ui/core/ui_root_tail.h`, id
`ui-select-menu`) between the last dataset member of the root level and the root, **at depth 0 only**. So
DOWN off `net-mqtt-root` lands on it, DOWN again wraps to P0, and UP from P0 arrives on it directly. The
root level is now TEN members; the dataset's own ring is still nine. ENTER-short on it raises the same
`InteractionResult::openPackSelector` flag the §3.4.1 gesture raises, through a one-shot on
`UiController`, so both routes converge on the single existing consumer in `firmware.cpp` rather than a
second code path.

**Two negative facts that matter more than the positive one**, because each is a trap the next reader will
otherwise walk into:

- It is deliberately **not** in `kRequiredScreens`. `web/mockup/tools/exporter/validation.ts` fails an
  export whose dataset omits any required screen, so listing it would FORCE it into every pack — the
  precise outcome this decision rules out. There is a comment above the array saying so.
- It is deliberately **not** in `src/ui/generated/`. The export gate regenerates that directory from the
  dataset and CI fails on any hand edit, so an entry added there would be both erased and pack-shadowable.

The root-level `ringPosition` anchor changed with it: it now anchors on the root rather than the
lowest-addressed member, because one member of the ring lives in a different translation unit and link
order could otherwise decide which is index 0 — which would make a host assertion about the index no
evidence about the device.

Covered by `rootEntryTests` (24 checks) in `test/host/interaction_test.cpp` and the extended `[info ring]`
block in `nav_test.cpp`. **A permanent consequence to know about:** the mockup, the 80-screen geometry
audit and the spec audit all keep showing a NINE-member root ring, because `screens.json` is untouched by
design. That discrepancy is correct, and the host test is the only gate this screen's geometry will ever
have.

### DF12 — ~~§3.1's held UP/DOWN navigation step is emitted and answered by nothing~~ ✅ RESOLVED 2026-08-17

`button_input.cpp` emits repeat events from 1.5 s, every 250 ms, exactly as specified. `mapGesture`
maps them to `FlowGesture::Hold`, `matchFlow` demands an exact gesture match, and the dataset
declares **zero** hold flows — so every repeat is dropped and a held UP/DOWN navigates nowhere.

This produced three bug reports before anyone found the cause, because the web simulator had invented
the missing half in two different ways and the button legend documented the invention as fact. The
simulator now matches the firmware, and `heldRepeatScopeTests` pins the real behaviour.

**Decided 2026-08-17: amend §3.1 and withdraw the repeat.** The owner was offered the info-ring-only
implementation, the everywhere implementation, and the amendment, and chose the amendment — so this is a
**withdrawal of the requirement, not a deferral**. Nothing in the firmware or the simulator changed:
`heldRepeatScopeTests` already pinned the real behaviour and still passes with every assertion unmodified.

`Display_UI_Requirements` §3.1.1 now records three things, because stating only the first is what let this
be re-filed as a defect three times: that a held UP/DOWN does not navigate — one press, one step; **why
repeat events still exist in the firmware** (§5.4's editor ramp is the only thing a hold does, and it
reads pressed LEVELS rather than queued events, so deleting the emission would break it); and that the
dataset schema keeps `hold` as a legal trigger even though no screen declares one.
`Gesture_Reference.md` v1.3 corrects two rows that stated the unimplemented behaviour as fact.

**One thing the amendment now asserts that no test pins.** §3.1.1 records that the Select Menu's cursor
moves on a repeat, because `handlePackSelector` drains the queue and switches on `event.button` alone.
`heldRepeatScopeTests` covers the info ring, a config setting page and the editor ramp — **not the
selector** — and `recoveryGestureTests` pins the 3 s opening gesture rather than the cursor's tolerance of
an event kind. Add an `isRepeat` guard to `handlePackSelector` later and nothing fails while §3.1.1
silently becomes wrong. Closing it means extending one of those two functions with a held UP inside an
open selector.

### DF13 — ~~`OFF_CMD_RESET_CONFIG` leaves the Nyquist override latched~~ ✅ FIXED 2026-08-17

Offset 19 wiped `SensorData` and the configuration but left `overridePending_`, `overrideActive_` and
`pendingOverrides_` set. Offset 25 cleared all three, which is what made it visible as an inconsistency
rather than a design.

**The report was wrong about its own harm, and that is the part worth keeping.** This entry claimed a
latched override made `prepareConfigUpdate` accept the first candidate without a sampling check, so a
replacement meter inherited the exemption. That is **unreachable** — and was unreachable through offset 25
as well. `prepareConfigUpdate`'s invalid-candidate branch clears all three flags for ANY candidate failing
`configIsValid`, and from an all-zero configuration the first single-register write is necessarily such a
candidate, because no single write can supply the two fields `configIsValid` demands. The claim had been
copied into four other places, all corrected in the same round.

**The reachable harm was the other one, and it was real.** `evaluateSensorDiagnostics` ORs both flags in
for every `inUse` channel, and offset 19 deliberately preserves `inUse` — so a decommissioned channel
nobody would ever reconfigure kept its `REG_UNDERSAMPLING_FLAGS` bit lit for the life of the device, on
RS485 and on the MQTT diagnostics topic, with nothing that could ever clear it.

**Fixed** 2026-08-17 by owner decision rather than kept pinned: offset 19 clears the same three, so both
per-channel reset arms agree. Offset 19 was in fact the **last** site that invalidated a configuration
without clearing the override describing it — the connected-bitmap enable/disable arms and
`REG_MASTER_RESET_ALL_SENSORS` already did — so the rule is now uniform, which is a better statement than
"the two arms agree". Offset 19's measurement wipe is unchanged and still pinned as-is; 25 still exists for
the meter swap, whose reason is the measurement wipe and not the override.

The pinning test was **inverted rather than deleted**, which is the deliberate act it was written to
require, and a second case was added for the parked-but-unconfirmed flag — the confirmed-override scenario
structurally cannot observe it, so without that case deleting one of the three lines would still pass.
Stated plainly because it would otherwise rot: `pendingOverrides_` is **unobservable at every one of its
five clearing sites**, since its only reader is guarded by `overridePending_ &&`. Deleting that line kills
no assertion, in this arm or in offset 25's.

**Found while sweeping citations, and left alone: nothing in this repo gates citation freshness.** Ten
`modbus_manager.cpp` line references in the mockup were correct before this change and would have been
broken by it, so they were re-grepped rather than arithmetically shifted — and three of them turned out to
have been **already** wrong, pointing into unrelated functions. Separately,
`../Requirements/feature addition/Display_Per_Screen_Spec.md` carries **nine** such references that were
all stale before this round began. Deliberately not swept: a docs-wide citation sweep is its own round.
The durable lesson is the one this round keeps proving — a line number in prose is a second home for a
fact, and prefer naming the symbol, or a `static_assert`, over citing a coordinate.

### DF14 — `configIsValid`'s offset bound is ten times the frequency the channel can reach 🟡

**The one thing this round found and deliberately did not change**, because it is a decision about what
the predicate promises rather than a repair.

The bound is `|adjust| <= q_max * multiplier * 10`. The comment directly above it says the offset "may not
exceed the frequency the channel can actually reach", which is `q_max * multiplier` — **ten times
smaller**. So the code and its own stated rule disagree by a factor of ten, and the code is the looser of
the two.

**What the sampling fix already catches.** An offset large enough to cancel the whole span drives the
ceiling to zero or below, and `meetsNyquistLimit` now refuses that. So `m=200, a=-30000, q=150` — the
audit's case — is flagged and parked instead of installed silently.

**What it does not catch, and why this is still open.** A SMALL negative offset passes everything:

| | |
| --- | --- |
| `m=10, q=150, a=-1400` | ceiling 100 Hz — comfortably samplable, so no flag |
| what the engine reads at ZERO pulses | `(0 + 1400) / 10` = **140 L/min** on a dry pipe |

`configIsValid` says valid, register 30 stays clear, the row says `OK`. Pinned in
`web/mockup/src/utils/__tests__/nyquist.test.ts` ("still admits a SMALL negative offset") so it cannot be
mistaken for solved, and mirrored in `sensorConfig.ts` on purpose — a mockup stricter than the device
would show a warning hardware will not.

**Decide what the bound means.** Either tighten it to `q_max * multiplier` and accept that some real
datasheet fits become invalid (nobody has checked which, and that needs meters, not reasoning), or state
that a negative offset is the operator's business and correct the comment instead. Both are cheap; the
present state is the only one that is indefensible, because the comment and the code disagree.

**Also still true after this round:** every sampling refusal PARKS. A master that writes the same
degenerate figures twice installs them through §5.5's override handshake, by design — the handshake exists
for meters the gate is wrong about. Asserted in `sensor_config_gate_test.cpp` rather than glossed over.

### DF15 — Three registers §5's table documents do not exist 🟡

Found while relocating `kPortalPassword`. The table in
`../Requirements/feature addition/WiFi_MQTT_Connectivity.md` §5 does not match `net_register_map.h`, and
in one place does not match itself:

| Table says | Header says |
| --- | --- |
| `674 NET_PORTAL_ENABLED` — the STA-side config page (R7.9) | no such constant |
| `711 NET_AP_REQUEST` — raise or drop the provisioning AP (R5.4a) | no such constant |
| `708–711 NET_AP_IP` | `kApIp = 708`, **two** registers (708-709) |

So the table double-books 711 between `NET_AP_IP`'s range and `NET_AP_REQUEST`, and publishes two
addresses an integrator would read as available. Both missing registers are **unbuilt features** with
requirement numbers attached (R7.9, R5.4a), which is why this is recorded rather than fixed: adding them
is a feature, and deleting them from the table would discard a requirement.

**Decide:** build the two registers, or move them out of the layout table into the requirements text so
the table describes only what answers. The `static_assert`s added this round cover overlaps between
things that EXIST; they cannot catch an address that exists only on paper.

### DF16 — The simulator's `ready` is stored where the device derives it — DEFERRED ON SCOPE 🟡

Was two facts; one closed this round. Both were the same shape — a green suite over something only a
human can reach:

- ~~**`SimulatedSensor.undersampling` is a mockup-only control**~~ ✅ **closed 2026-08-17.** The flag is
  now DERIVED from the configuration and a polling-rate dial, exactly as `evaluateSensorDiagnostics`
  derives it, and `nyquist.test.ts` pins both ceilings and the margin factor against the C++ source. What
  remains a hand-set input is `samplingOverride`, which is genuinely an input — it models the operator's
  half of §5.5.
- **`ready` is a stored boolean that nothing recomputes** when the configuration changes, so writing
  `qMaxLpm = 0` through the Values panel leaves the row saying `OK` while the device would say `SET?`.
  Fixing it means deriving `ready` in `normalizeSensor`, which changes what every producer in
  `sensorConfig.ts` means by the field.

**Why it survived this round.** The `undersampling` half was closed by DERIVING it, which is the same
move `ready` needs — but `ready` is read by `resolveSensorBinding` and by the row that renders
`!connected ? "--" : ready ? "OK" : "SET?"`, so deriving it changes what every producer in
`sensorConfig.ts` means by the field. The firmware has no such field at all: readiness is
`configIsValid(configs[n])`, evaluated where it is asked, and the `SensorData::isReady` cache was deleted
precisely because a stored answer goes stale. The mockup is currently one refactor behind that lesson.

**This is NOT an open question, and it is filed honestly as such.** It is a correction: it needs no
hardware, reverses no owner decision, and is a simulator-lies divergence by the description above —
writing `qMaxLpm = 0` through the Values panel leaves the row saying `OK` where the device says `SET?`.
The right answer is known: derive `ready` in `normalizeSensor`, which is the same move that closed the
`undersampling` half this round, and which makes the mockup agree with the firmware.

**It is DEFERRED ON SCOPE, not undecided.** `ready` is read by `resolveSensorBinding` and by the row
renderer, and every producer in `sensorConfig.ts` sets it, so deriving it is a refactor of the module's
public shape rather than the one-line change the other three fixes were. It wants its own round, with
`sensorConfig.test.ts`'s 48 checks re-run against the new meaning of the field.

### DF17 — ~~`npm run test:visual` has been failing 32 of its 46 tests, and nothing noticed~~ ✅ FIXED 2026-08-18

**Measured both ways on 2026-08-17, on this branch.** With the banner round applied: 32 failed, 14
passed. With the round stashed and the tree at HEAD: **32 failed, 14 passed, and the failing test names
are identical** — compared as sets, not as counts. So this is pre-existing and the round neither caused
nor cured it. It is recorded because it was discovered by accident and would otherwise stay invisible.

**It is not only stale snapshots.** 21 of the failures are `toHaveScreenshot`, but the rest are not:
nine `toBeVisible`, five `toHaveValue`, three `toHaveCount`, and one exporter CLI failing
`Post-export validation failed` on `tests/fixtures/legacy-screens.json`. Assertions about the workspace's
own controls are failing, which is a broken suite rather than a moved baseline — so
`--update-snapshots` is NOT the fix and would bake in whatever the app now does.

**A third reason, verified 2026-08-18: CI never runs it.** Neither `test:visual` nor `playwright`
appears anywhere in `.github/workflows/` — the pipeline runs typecheck, unit, exporter, build, the export
gates, the skeleton-reproduces-dataset check, the committed-assets check, host tests and the generated-docs
gate. A suite that no automation runs and whose exit status is easy to lose locally is a suite that can
fail for months. Note also that `docs/backlog/SI-20251111-04-help-docs.md` describes this as *stale
baselines*; that framing is superseded here — 11 of the 32 failures are not snapshot assertions at all.

**Two further reasons it stayed hidden, both worth knowing.** `Display_Per_Screen_Spec.md` and this register both
cite the suite as a live gate, and `web/mockup/tests/` carries a comment warning that a bare
`npx playwright test` serves a stale `dist/` — so the recorded hazard is about *how* to run it, which
reads as though running it correctly works. And the exit status is easy to lose: any invocation that
pipes the run (`| tail`) or appends a command (`; echo`) reports the last process's status, so the suite
looks green at exit 0 while printing 32 failures. Both mistakes were made in the course of finding this.

**Consequence for §2c, stated plainly: the banner's relocation to y=116 has NO pixel-level
verification.** The band is checked by the new `screen-geometry.ts` rule, by a host assertion on the fill
rect and both cursor positions, and by `warningBanner.test.ts` — but nothing renders it and looks. This
suite is the only gate that would, and it cannot be trusted until it is repaired.

**Do not repair it by regenerating baselines.** Triage the non-snapshot failures first: they say the app
and the spec disagree about the workspace, and that disagreement is either a real regression from an
earlier round or a spec that was never updated. Only once those pass does a snapshot refresh mean
anything.

### How it was fixed, 2026-08-18 — **44 passed, 0 failed, exit 0, twice**

**Correcting this entry's own evidence first.** The 21/11 split was right; the per-type counts were not —
they counted assertion mentions, not failing tests. Measured: **21 snapshot, 3 `toBeVisible`, 2
`toHaveValue`, 1 `toHaveCount`, 5 timeouts/CLI**.

**All eleven were the SPEC being stale. Not one was an app regression** — which is what earned the baseline
refresh the paragraph above rightly withheld until it was known.

| Failure | What it asserted | Why it was wrong |
| --- | --- | --- |
| blank canvas by default | 1 screen named *Blank Canvas* | The workspace ships and loads 80 screens. Now compares against the shipped dataset's own length, so growth cannot re-break it |
| edge coordinates | filled a Content box, and portrait axes | The toolbox renders a *Bound value* select instead of Content for any bindable kind; `maxX` used the **portrait** bound on the landscape axis. Rewritten — and it found J8 |
| transition preview | `.transition-overlay` renders on DOWN | **Deleted.** `App.tsx` passes `pendingTransition={undefined}` with six lines saying why: the firmware draws no transition, so previewing one made the simulator less faithful. Residue is **J7** |
| per-element clamp / clamp all | 200 px overflows, clamps to **135** | 135 is the portrait width. Landscape is 240 (D3), so 200 fits and the app was right not to offer a clamp. Now overflows with 300 and expects 240 |
| landscape orientation | clicked a button named *Landscape* | The toggle is a disabled indicator reading *Landscape 240x135* — the same rename that once broke every test through `beforeEach` |
| event binding action | a `flow-action-select` test id, on a screen with no events | The panel rewrite dropped the id, and the fixture declares flows, not events. Id restored as `event-action-select`; the test adds an event first |
| animation inspector | an `animation-upload` control | **Deleted.** C1 dropped animation and the inspector went with it. Residue is **J2** |
| value placeholder traces | `.value-editor-panel`, a *Save value* button, *Value edited* / *Value saved* traces | None of the four exists. `handleMemoryWrite` applies an edit straight into memory with no trace and no save step, deliberately. Now asserts the write lands |
| out-of-bounds import alert | the alert on the **Design** tab, *"were clamped"* | Layout diagnostics moved to **Simulation**, and one element is *"was clamped"*. Verified by driving the app: `x: 220 -> 160, y: 260 -> 115` — the landscape bound working |
| exported IR | built the exporter and ran the CLI over `legacy-screens.json` | The CLI **rightly refuses** that fixture (9 required screens missing, 8 actions and 10 values absent from the manifest), and it wrote into `src/ui/generated/`, so a test run dirtied the firmware tree. Now compares the **committed** IR with the **committed** dataset and writes nothing |

**Two app changes, both restoring a test contract the app had dropped:** `data-element-id` on rendered
viewport elements (the suite used to locate an element by the text it renders, which coupled geometry to
content), and `data-testid="event-action-select"`.

**Then the 21 baselines were regenerated,** once the assertions passed. The diffs are changes already
decided elsewhere — the orientation toggle gone, the values panel rewritten, the unit change — and the old
baselines still showed a PORTRAIT button and *SAVE VALUE* buttons. Green twice at exit 0; `tsc`, 213 unit,
44 exporter and the host suite all still pass.

**Two things this did NOT fix, stated plainly because the entry above claims them:** the banner still has no
pixel-level verification (**DF20**), and CI still does not run the suite (**DF21**).

### DF18 — `nyquist-warning` puts an option row inside the banner band 🟡

**Found by the new gate, on its first run.** Teaching `tools/audit/screen-geometry.ts` the banner band
took it from "0 findings" to "1 finding" — and the finding is correct. `option-down`
("DOWN = Save anyway") sits at y 112…120, four pixels inside the band, so the banner paints edge to edge
over it while a warning is live.

**A gate that finally checks the real band should report the real overlap. Do not make the audit read 0
again by deleting the check** — the reason is written into the tool's own summary comment as well as here.
The tool never calls `process.exit` and CI does not run it, so nothing is broken by the 1.

**Why it is not fixed here.** The screen has no spec file under `screens/`, so the generator's
deterministic footer re-stack reproduces y=112 on every `--write`. The only fix is authoring
`../Requirements/feature addition/screens/nyquist-warning.json`, which is a fresh design decision about
where two option rows go on a screen that must stay legible with a warning live.

**Deferred on the grounds that the screen is unreachable on the device** — verified this round: zero flows
target it, zero submenus reference it, and it appears in no `ui_pages.h` table, so the live prompt is the
in-place `config.sensor.nyquistWarning` row on each edit screen and this overlap is a mockup-only artefact
today. But note the irony before deciding: it is the ONE screen where the banner and the content are
guaranteed on-screen together, so it may deserve to be the screen that RESERVES the band rather than
sacrificing it.

### DF19 — Six shipped scrollbars are 104 px where §2c requires 100 🟡

**§2c's claim that "every `level-position` scrollbar was shortened to 100 px so it stops clear of y=116"
is not true of the shipped dataset.** Six screens carry y=14 height=104, bottom y=118 — two pixels inside
the band: `confirm-reset-totals-back`, `confirm-reset-session-back`, `confirm-reset-max-flow-back`,
`confirm-reset-calibration-back`, `confirm-factory-reset-back`, `confirm-reset-portal-login-back`.

Every `*-back` screen WITH a spec file gets 100 from the spec-override loop. These six have none, so the
**root cause is `web/mockup/tools/skeleton/generate.mjs`'s own layout table** (`L.scrollbar.height: 104`) —
one fact with two homes, in the one place that was still live, which is the hazard that file's own comment
complains about.

**The fix is that single number plus `node tools/skeleton/generate.mjs --write`**, which is the only legal
route because CI diffs `screens.json` byte-for-byte. Not done this round: `--write` rewrites 72 of the 80
screens from scratch and no one had accepted a wholesale dataset regeneration as part of a banner change.

**It does not appear as an audit finding, which is exactly why it is recorded here.** The new band check
exempts `kind === "scrollbar"` — a 5 px fixture losing 2 of 104 px hides no glyph, and the collision loop
already treats one as decorative. Without that exemption the audit would report 7 rather than 1.

### DF20 — No snapshot drives a warning state, so the visual suite never renders the banner 🟡

Found while fixing DF17, and the reason that repair does not close §2c's verification gap. Every snapshot
captures a workspace with no warning live, so `warningBanner` is empty in all 21 baselines and the band at
y=116 is never painted in any of them.

**What it needs:** one case that drives the simulator into a warning — the values panel's per-sensor
controls are the lever — and snapshots the panel with the banner up. That is the only gate that would catch
the banner drifting back over the footer, which is what DF7 fixed. Until it exists, the banner's position is
asserted by host tests and the geometry audit but seen by nothing.

### DF21 — CI runs no `test:visual` step 🟡

Verified 2026-08-18: neither `test:visual` nor `playwright` appears anywhere in `.github/workflows/`. The
pipeline runs typecheck, unit, exporter, build, the export gates, the skeleton-reproduces-dataset check, the
committed-assets check, host tests and the generated-docs gate — none of which renders a pixel.

**This is why DF17 was invisible for months**, and it is deliberately NOT bundled into DF17's fix: the suite
takes ~1.6 minutes locally plus a browser download, snapshots are sensitive to the container's font
rendering (these baselines are `chromium-linux`), and a flake policy has to be chosen. Decide those, then
add the step.

---

## Residue and hygiene — J1–J8, opened 2026-08-18 (J3, J4, J5 closed the same day)

J1–J5 were a **single unnumbered sentence in `MEMORY.md` §6** — "smaller and still open: the export
gate for ring closure, the I2 append-only catalogue check, `animation` residue across five layers, the
simulator's missing nav stack, `carea/` (tracked, 18 files), and rewrites of `web/mockup/README.md` and
`UI_Firmware_Interface.md`" — which is a second open-items register with no ids, no status and no
diagnosis. Each was verified against the source on 2026-08-18 before it was given an id; the two that did
not survive verification are recorded at the end of this section rather than promoted.

### J1 — No export gate proves a level's DOWN ring closes 🟡

`runExportValidations` in `web/mockup/tools/exporter/validation.ts` runs four coverage checks
(`checkManifestActionCoverage`, `…ValueCoverage`, `…ScreenCoverage`) plus `checkRenderableElementKinds`.
Grepped for `ring` and `closure` on 2026-08-18: neither appears in the file.

**Why it matters more than it looks.** Paging wraps in the **dataset**, not in code — `UiNavigator`
follows each screen's own DOWN flow and resolves the target by linear search, so a ring is closed only if
the authored data closes it. A pack whose last member points nowhere, or points back into the middle,
strands the operator on the device with no way out and no gate that would have said so. The built-in pack
is closed because the generator emits it that way; a third-party pack has no such guarantee.

**Same family as N-b:** both are export-time gates that `Loadable_UI_Menu_Packs.md` assumes and that do
not exist. Worth building in one round.

### J2 — `animation` was dropped by C1 and still survives in five layers 🟡

C1 chose the scrollbar and **dropped animation**. What is still there, verified 2026-08-18:

| Layer | Evidence |
| --- | --- |
| dataset type | `web/mockup/src/types.ts:168` — `animations?: ScreenAnimation[]` |
| shared schema | `shared/schemaDefinitions.ts:153,170,248` — `animationKeyframeSchema`, `animationSchema`, the `animations` array, and `animation` in the theme's `required` list |
| exporter IR | `tools/exporter/types.ts:76` and `ir.ts:121,133` — carried through the intermediate representation |
| C++ emitter | `tools/exporter/cppEmitter.ts:138,537–540` — **emits an `Animation*` array into the generated firmware header** |
| firmware | `theme_palette.cpp:41`, `theme_palette.h:21`, `GeneratedUi.h:165` — `animationEasing()` exists and returns a token |
| user-facing docs | `HelpPanel.tsx:156` documents `animations` to the operator as a dataset feature |

**Nothing renders one — grepped, not assumed** (2026-08-18, because a diagnosis quoted without checking
its mechanism is how offset 19 got its harm wrong). `GeneratedUi.h:136–137` gives every screen struct
`const Animation* animations` and `std::size_t animationCount`; searching `src/` for `->animations`,
`.animations` and `animationCount` returns **no reader outside `generated/`**, and `ui_renderer.{cpp,h}`
does not contain the string `animation` at all, case-insensitive. `animationEasing()` likewise has no
caller beyond its own definition and declaration. So a dropped feature costs schema surface, IR surface, generated
firmware bytes and a documented promise the device does not keep.

**Decide the shape before cutting:** the theme's `animation` easing token is in the schema's `required`
list and the mockup's own CSS transitions may legitimately use it, so this is not one delete — it is a
`required`-list change, a regenerated dataset, and a `GeneratedUi.h` shape change, which makes it a round
of its own rather than a tidy-up.

### J3 — ~~`carea/` is 18 tracked files of a stray git directory~~ ✅ FIXED 2026-08-18

`git ls-files carea/` returns 18 files, 80 KB on disk, and they are **git internals**: `carea/HEAD`,
`carea/config`, `carea/description`, `carea/hooks/*.sample`. This is the debris of a `git init` or a
`--separate-git-dir` that landed in the working tree and was committed.

**Why it is not a one-line `git rm`.** It is harmless to the build and it has been tracked long enough
that nothing is known about what pointed at it. Removing it is right, but do it deliberately: confirm no
tooling path references `carea/`, then remove in a commit that does nothing else, so the deletion is
reviewable on its own.

**Done that way, 2026-08-18.** `config` said `bare = true` and there were no `objects/` or `refs/`
directories at all — the skeleton a `git init --bare` leaves behind, with zero content. Grepped every
`md`/`json`/`mjs`/`ts`/`yml`/`sh` in the tree: the only references were the register entries describing it.
Deleted in a commit that does nothing else.

**Worth recording, because it is the reason this was open at all:** archive entry **F2** reads *"✅ delete
`carea/`. Verified empty of project content — a stray bare repo. (2026-07-30)"*. The decision was taken
nineteen days earlier and the directory was still tracked. A decision recorded as done, but never
executed, is indistinguishable from a decision never taken — which is why I3's index rule ties closing an
item to the edit that closes it.

### J4 — ~~`web/mockup/README.md` describes a display, a dataset and a feature that are all wrong~~ ✅ FIXED 2026-08-18

Verified by reading it 2026-08-18. Three false claims, in one paragraph each:

- **"the 135×240 display"** — portrait. D3 settled landscape **240 × 135** and the whole dataset is built
  that way. This is the claim most likely to be believed, because it carries numbers.
- **"You start on an empty Blank Canvas dataset"** — the workspace ships `src/data/screens.json` with 80
  screens.
- **"easing presets"** in the design panel — the animation feature C1 dropped, which is J2's residue
  surfacing as a user-facing promise.

`MEMORY.md` §4 already lists this file as untrustworthy, which is the right label and no substitute for
fixing it. Small, self-contained, and needs no decision.

**Rewritten 2026-08-18, and there were nine false claims, not three.** Reading the whole file rather than
the three known lines was the point — a document labelled untrustworthy earns that label everywhere, not
only where someone last looked:

| Claim | Truth |
| --- | --- |
| "the 135×240 display" | 240 × 135 landscape (**D3**). The new text explains that `135` in this codebase is the panel's native portrait dimension, never a bound on the x axis — the confusion that put portrait bounds in the visual suite for months (**DF17**) |
| "You start on an empty **Blank Canvas** dataset" | It loads `src/data/screens.json`: **80 screens** plus a theme block |
| "The bundled file is intentionally empty" | Same claim again, in the Screen-layout section. It is also **generated** by `tools/skeleton/generate.mjs`, and CI diffs it byte-for-byte — so hand-editing and committing it is a failing build, which the file never said |
| "easing presets" in the Design panel | Animation was dropped by **C1**. The token survives and nothing reads it (**J2**) |
| "Simulation view includes a live JSON snapshot" | `LiveJsonEditorPanel` renders under `activePanel === "design"` (`App.tsx:3436`) |
| `npm install` | `npm ci` — the root README already says the lockfile is authoritative |
| "Generated files land in `Water Flow Meter PlatformIO/…`" | `Water-Flow-Meter-PlatformIO/` — hyphens. A path a reader would try and fail with |
| "Extend the JSON schema to cover animations. ✅" | A tick beside the feature C1 dropped |
| "(Story SI-20250517-05)", "(story SI-20250517-02)" | Neither exists. `docs/backlog/` holds SI-20251111-03 and -04; the SI-20250517 series is in `docs/archive/` |

**What the rewrite adds, beyond removing falsehoods:** the keyboard section now says the arrow keys are
taken over by element nudging in the Design tab, the four tabs are listed with what each actually owns, the
export section names the flags that exist (`--screens`, `--manifest`, `--dry-run`,
`--allow-missing-toolchain`) and warns that `export:firmware` rewrites **three** `generated/` timestamps —
counted: `GeneratedUi.cpp`, `ui_export_ir.json`, `ui_export_metadata.json`. The visual-suite section records
why a bare `npx playwright test` lies and that CI does not run the suite (**DF21**).

**Every claim in the replacement was verified against the source**, including the ones inherited from the
old text: the `backups/ui/<timestamp>/` path, the export endpoint in `vite.config.ts`, the fixture list, the
component inventory, and the absence of a port override (so 5173 is Vite's default and correct).

It closes with a pointer to this register rather than a "Next steps" list of its own — the second list is
how the first goes stale, which is what I3 and the consolidation of 2026-08-18 are about.

### J5 — ~~`UI_Firmware_Interface.md` lists 4 actions where the catalogue has 19~~ ✅ FIXED 2026-08-18

Counted 2026-08-18: the document's action table carries four rows — `ui.action.page.next` /
`.previous`, `ui.action.mode.configuration` / `.info`, `ui.action.mode.idle`, `core.action.save-config`.
`grep -oE '"[a-z]+\.action\.[a-z.-]+"' src/ui/core/ui_action_catalogue.h | sort -u` returns **19**
distinct ids. (`MEMORY.md` says "4 where there are 15" — it was 15 when that was written; the drift
continued.)

**Why this one is the dangerous one.** It is the document an implementer reads to learn what actions
exist, so it does not merely omit — it teaches a wrong catalogue, and every id it omits looks unavailable.
The fix is mechanical (regenerate the table from the catalogue header, or delete the table and point at
it), and the mechanical option that cannot drift again is the right one.

**Fixed 2026-08-18 by generating it, and it was worse than recorded.** Two of the four rows named actions
that **do not exist**: `ui.action.mode.configuration` and `ui.action.mode.info`, replaced by
`ui.action.nav.descend` / `.back` / `.escape` without the table being told. Omitting fifteen makes them
look unavailable; advertising two that no handler implements sends an implementer to wire a flow the
firmware will refuse. A third row's description was also wrong — `core.action.save-config` was described
as persisting *"the LED configuration via `LedController::saveToPreferences`"*, where the catalogue says
the configuration block to NVS and the Modbus registers.

- **`tools/wiki/gen-actions.mjs`** reads `kActionCatalogue` and replaces a marked region in the document.
  A scanner, not a regex, because two entries defeat one: descriptions split across adjacent C++ string
  literals, and the `//` comment above `reset-calibration`. Escapes are decoded, so `\u00a75.5` reads as
  §5.5.
- **A parse that finds no actions, or a document missing its markers, exits 1** rather than writing an
  empty table — an empty section is precisely what a silent failure looks like.
- **CI gates it by diff**, the same shape as the diagram freshness check.
- **Negative-tested in a clean worktree**, per §5 of `MEMORY.md`: quiet on an unmodified checkout, fires
  and names the fix when an action is added to the catalogue, and exits 1 with a message when the markers
  are removed.

The catalogue was already reconciled with the handler table by `static_assert`, so that guarantee now
reaches the document rather than depending on someone copying nineteen rows and remembering to come back.

**The committed table has EIGHTEEN rows, not nineteen.** `ui.action.pack.select-menu` is part of this
branch's uncommitted Select Menu work, so generating from the working tree would have documented an action
`HEAD` does not declare and CI would have failed on the pushed branch. Whoever commits that catalogue
entry runs `--write` in the same commit — and the new gate is what says so if they forget.

### J6 — Nothing verifies the workspace is accessible 🟡

`docs/backlog/SI-20251111-04-help-docs.md` lists "accessibility verified" among its acceptance criteria
and marks it not delivered. Verified 2026-08-18: `web/mockup/package.json` contains no `axe`, `jest-axe`,
`pa11y` or `lighthouse`, so **no automated check exists**, and CI runs no `test:visual` either (see DF17),
so neither the accessible-name tree nor the rendered result is checked by anything.

The hand-written attributes are real — `HelpPanel.tsx` carries `aria-labelledby` on its sections, an
`aria-label` on the element-kind table and on each tab-jump button — which is the reason this is a
verification gap rather than an absence. Nothing tells you when one of them stops being true.

**Kept deliberately small.** This is an internal design tool for an embedded device, not a shipped web
product, and the scope here is exactly what the story claimed and did not do: one axe pass over the
workspace's tabs, wired into the unit suite. Anything broader is a new decision, not this item.

### J7 — The transition preview is off by decision, and all five of its layers remain 🟡

`App.tsx` passes `pendingTransition={undefined}` behind a six-line comment explaining that the firmware
draws no transition, so previewing one made the simulator less faithful. The feature is off. Still present:
the `transitionPreview` state and its 1500 ms expiry effect, the `previewTransition` callback fired on every
answered press, the `TransitionPreviewState` type, `.transition-overlay*` in `App.css`, and the render branch
in `DisplayViewport`.

**Identical shape to J2** — a dropped feature whose layers survive — and worth deciding in the same round:
delete the five layers, or restore the prop behind a toggle. What it must not stay is a callback firing on
every keypress into a value nothing reads.

### J8 — Two clamps disagree: the Design panel pins the coordinate, the importer pins the far edge 🟡

Found by DF17's corner test, measured both ways on 2026-08-18:

| Route | A box pushed to the right edge | Result |
| --- | --- | --- |
| Design panel x/y inputs | 40x20 box, requested x = 9999 | **x = 240** — the panel's own width, so the box sits entirely outside the visible area and renders with `clientWidth` 0 |
| dataset import (`datasetClamp`) | 80x20 box at x = 220 | **x = 160** = 240 − 80, the far edge exactly on the boundary, and the clamp notice says so |

The same geometry is legal through one route and corrected through the other, and the panel's answer is the
one that hides an element. `datasetClamp`'s rule is the defensible one — an element you cannot see is not a
placement.

**Decide which rule the panel follows**, then make the inputs agree. The corner test pins today's behaviour
(`expect(geometry.x).toBe(layoutBounds.width)`), so this cannot change silently.

### Not promoted, and why

- **"The simulator's missing nav stack" — not verified, so no id.** Probed 2026-08-18: no `navStack`,
  `backStack` or `history` in `App.tsx` or `src/utils/`, and the parent lookups that do exist
  (`findParentScreenId`, `App.tsx:670`) serve the **design** tree, not a runtime BACK. But the device has
  no nav stack either — `UiNavigator::current_` is a bare pointer and BACK is resolved from the tree — so
  "missing" may describe a divergence that no longer exists. It needs someone to state what the simulator
  is supposed to do on BACK before it can be called a defect. Left in `MEMORY.md` §6 marked unverified.
- **"The I2 append-only catalogue check" — folded into I2 as I2a**, not given a J id. The rule and the
  fact that nothing enforces it belong in one place.
- **SI-04's "no contextual links from Help into the Simulation and Design panels" — stale, so no id.**
  They are built and wired: `HelpPanel.tsx` renders `→ Open Simulation trace panel` and `→ Open Simulation
  tab` behind an `onNavigateToTab` prop, and `App.tsx:3555` passes `setActivePanel` into it, so the buttons
  render. The story's residue section has been corrected in place.

---

## Closed, for reference

41 of 42 entries. Grouped as the archive groups them, with what closed each:

| | Closed by |
| --- | --- |
| **A1–A6** requirements rewrites | `Project_document.md` §4.1.1 and the staged link block, registers 40–45 |
| **B1** metric suffixes | a suffix→metric table in `ui_bindings.cpp` |
| **B2** config state in the render context | **by different means** — the resolver holds the settings store and calls `readSetting`/`readSettingText`. No `UiConfigContext` sub-struct was added, and none is needed |
| **C1** animation vs scrollbar | scrollbar shipped; animation dropped |
| **C2** compile-time binding emitters | `eventEmitter.ts` and `valueEmitter.ts` deleted; the runtime resolver survives |
| **C3** countdown screens | replaced by four confirm screens with BACK rows plus three toasts |
| **D0** countdown semantics | hold-to-confirm, on a dedicated confirm screen |
| **D1** config pages to screens | superseded by the hierarchical navigation model |
| **D2** manifest generation | `manifest_gen` generates it from the firmware catalogues; CI fails on drift |
| **D3** display orientation | landscape 240 × 135 throughout |
| **D4** dataset saving | the dataset is POSTed with the export request, with a checked-in baseline |
| **D5** settable elements | a fixed catalogue, not free-form bindings |
| **E1** manifest register arithmetic | closed by D2 — it is computed, not asserted |
| **F1–F7** hygiene | `node_modules` untracked, CI added, agent tooling removed, stale claims corrected |
| **G2, G3** hardware risk | Modbus task affinity and the Configuration-mode redraw both fixed |
| **H1–H6** menu behaviour | resolved across `Display_UI_Requirements.md` §3.1, §4.3, §5.6 and §6 |
| **I1** menu-pack design | accepted; the format, loader, selector and boot ladder all shipped |
| **N-a** text settings | `SettingKind::Text` plus `readSettingText`/`writeSettingText`, the numeric API untouched |

---

## What to read instead of this file

This register is for genuinely open questions. Current state lives with the code:

| | |
| --- | --- |
| What the panel does, screen by screen | `../Requirements/feature addition/Display_Per_Screen_Spec.md` |
| What every gesture does, as built | `../Requirements/Gesture_Reference.md` |
| How the four JSON artefacts relate, and the ten gates | `../Requirements/feature addition/UI_Dataset_Contract.md` |
| WiFi, MQTT, Home Assistant, the network registers | `../Requirements/feature addition/WiFi_MQTT_Connectivity.md` |
| The Modbus register map | `../Requirements/Project_document.md`, and `../../tools/wiki/gen-registers.mjs` for the generated reference |
