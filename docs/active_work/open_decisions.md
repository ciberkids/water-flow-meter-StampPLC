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

**This file is 287 lines and holds six items.** It was 1,370 the day before, because twenty-nine closed
defects had accumulated in it with their full reasoning. Those are archived, not deleted — see the pointer
near the bottom — on the same principle the 2026-08-12 rewrite used: a register that reads as a thousand lines
of finished work does not get read, which is the failure both cleanups were undoing.

The closed entries are kept verbatim, with their questions, options and reasoning, in
[`../archive/open_decisions-closed-2026-08-12.md`](../archive/open_decisions-closed-2026-08-12.md).
That history is worth keeping — several entries record a decision being *reversed* — but it is
history, not a work list.

Status legend: 🔴 blocks work now · 🟡 blocks a later slice · ⏸️ waiting on something external ·
🟢 blocks nothing, but costs the next reader time

---

## The index — cite the ID, not the heading

**Five open lines, and not one of them is a defect.** `N-c` and `DF22` both closed on 2026-08-20 —
`DF22` was found by building `N-c`, which is the usual way, and was the only 🔴 the board has had since
the register was rewritten. What remains is one queued FEATURE, one missing gate, one piece of residue,
and three things that need the board. Ask for work by ID — "decide N-b", "build I2a" — and this file is
the one place that says what an ID means. Rule **I3** below governs them: append-only, never reused, so
a gap means an item closed, not an item lost.

**`N-c` stays in this file rather than moving to the archive**, because one thing in it is verified by
reading only: `MQTT_EVENT_DATA`'s retain flag, which R4.4.2c depends on entirely and which no host test
can reach. Its ⏸️ block carries the one-command bench check and says what a wrong answer costs. That
makes **three** things waiting on the board, not two.

The **Shape** column is the one that answers *can I just say go ahead?*

| ID | | Shape | What it is |
| --- | --- | --- | --- |
| **G1** | ⏸️ | measurement | The 3.3 kHz polling rate has never been measured on a board; the procedure is written down and waiting |
| **N-d2** | ⏸️ | correction + measurement | Nothing protects the VLF probe's position above `M5StamPLC.begin()`, and whether the RTC survives power loss is unknown |
| **N-b** | 🟡 | feature, queued | Growing the settings catalogue silently invalidates authored menu packs; only the generator notices |
| **I2a** | 🟡 | gate, unbuilt | Nothing enforces I2's append-only catalogue rule; it is honour-system prose |
| **J9** | 🟢 | residue, found 2026-08-20 | `nyquist-warning` is a screen in the dataset that nothing can navigate to — the earlier design of §5.5's prompt, left behind when the in-place one replaced it |

**DF1–DF22 and J1–J8** are fixed and keep their IDs — `DF1`–`DF21` and `J1`–`J8` moved to
[`../archive/open_decisions-closed-2026-08-18.md`](../archive/open_decisions-closed-2026-08-18.md), verbatim,
because I3 makes them append-only and a retired id must still resolve. **I2** and **I3** are standing rules
that never close.

**What this list is NOT.** Nothing here is blocking a build, a test or an export: every gate in the
repository is green (host **1,997 checks across 24 suites**, 220 unit, 51 exporter, 51 visual, 0 audit
findings, and a firmware that compiles at RAM 24.7% / Flash 38.2%, measured 2026-08-20). One is a feature
nobody has started, two need hardware that has never existed for this project, `I2a` is a rule enforced by
prose, and `J9` is residue. That is a
different condition from "twelve things are broken", which is what this register looked like two days ago.

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

## ~~DF22~~ ✅ FIXED 2026-08-20 — the network block's live half is published, and 732 stops lying

Found while building `N-c`, which needed register 565 to actually reach a master, and closed the same
day in two commits because it was two defects wearing one name.

**The eight silent fields.** `ModbusManager::syncGlobalRegisters` republishes the whole 233-register
network block from `NetRegisterMap::publish`, which packs the settings and zeroes everything else — so
501 `kWifiState`, 502 `kWifiRssi`, 503-504 `kWifiIp`, 505-507 `kWifiMac`, 675 `kPortalRemainingS`,
676-691 `kApSsid`, 692-707 `kApPassword` and 708-709 `kApIp` read 0 forever. Fixed with
`NetRegisterMap::publishStatus`, additive and run after `publish`, fed by a `NetStatusSnapshot` that
`firmware.cpp` refreshes once per logic pass and that **the panel reads too** — one snapshot, so an
operator at the device and a master on the bus cannot describe different states.

**And 732 `kLastError`, which was worse than silent.** `applyHoldingWrite` writes it when an apply is
refused, because §5.1 requires a block write across the region to succeed rather than except, so the
refusal has nowhere else to go — and the next sync zeroed it. `0` is `NetApplyError::None`, so the
register did not go blank, it reported success. Fixed by carrying the value across the block write: it
is a RECORD, not a reading, so it belongs to neither `publish` nor a snapshot.

**A timing claim in this entry was wrong.** It said the erase happened "a few milliseconds later"
because the sync "runs on the logic loop". `syncGlobalRegisters` is not called per pass — it runs from
`sensorStateEngine.update`, which the loop gates behind `now - lastCalcTime >= 1000`, plus on a
master's reset, a link restart and a rollback. So the window was up to **one second**, not
milliseconds. The defect is identical in substance and the correction is here rather than quietly
edited away, because "how often does this actually happen" was the question that decided the fix's
shape: at 1 Hz, a `publishStatus` inside the existing sync is fresh enough for a radio state, and no
separate periodic writer was needed.

**What made it findable, and what hid it.** Three places already recorded the silent half —
`wifi_manager.h`, `wifi_manager_test.cpp` and `ui_renderer.cpp` all say some form of "`publish()` does
not write 501" — each stating it as a *reason* rather than as a gap, and none of them in this register.
What hid the 732 half is sharper: the existing assertion in `modbus_write_multiple_test.cpp` read the
register in the same breath as the write, so it passed over the erase completely. The new one syncs
first and says why.

**The cost, stated as it was found.** `WiFi.md` calls the Modbus route "the fully remote path, and
currently the reliable one" and ends it with three verification reads — `kRevision`, `kWifiState`,
`kLastError` — of which one worked. A master could not tell a successful association from a refused
apply, on a device whose only other remote route is a portal window that expires. That page now says
so, including for anyone reading it against older firmware.

**One thing the fix corrected on the way past:** `wifi_manager.h` said the MAC is "published at
registers 503–508", which is the IP's window and one register too many. It is 505-507. Nothing
published it at all until now, which is why the wrong span went unnoticed.

**Verified:** 27 new host checks across two suites — the packing (exact register values, both word
orders, the span boundaries, and that `publishStatus` before `publish` is erased by it) and the
integration (that the manager calls it, in the right order, and that a second sync does not lose it).
Negative-tested by reversing the call order (8 failures), flipping the IP word order (caught), and
writing `kLastError` back as zero (caught). A null snapshot leaves the registers alone, which is the
boot pass. Host 1,997 checks; RAM 24.6 % → 24.7 %, the cost of one cached snapshot.

---

## ~~N-c~~ ✅ FIXED 2026-08-20 — MQTT accepts commands, with both safeguards (one ⏸️ on hardware)

Recorded because the four-surface question kept being asked as though MQTT were one of the four. It now
is, for measurements: §4.4.1's three command topics are built, discovered as Home Assistant `button`
entities, and a reset arrives through the same holding-register write a Modbus master performs (R4.4.3),
so a reset from HA, from the panel's confirm screen and from the bus are one implementation.

**What landed.** `MqttCommandRouter` (Arduino-free, host-tested, 49 checks) owns every decision: the
magic payload, the retain check, both halves of the rate limit, and the command→register mapping.
`firmware.cpp` is a five-line adapter on esp-mqtt's task that latches and returns, plus a consumer on the
logic task that has the clock, NVS and the Modbus manager. Three `button` entities carry
`payload_press` — without it Home Assistant sends `PRESS`, the router answers `bad-payload`, and the
button ships broken, which is the one thing in this slice that was going to be wrong.

**Three things it taught, worth keeping:**

1. **Change detection does not rescue a changed value.** `MqttPublisher::tick` returns early on
   `!heartbeat && !rateLimitCleared` *before* formatting anything, so a refusal left to change detection
   alone is invisible for up to a full publish period. Every evaluated command now arms a full publish.
   The test says so, because I had written the opposite and it passed for the wrong reason.
2. **The persisted guard is only as good as the clock**, and that is stated rather than hidden: with no
   trusted time it cannot fire and the `millis()` half carries it alone. All three clock routes exist
   now (N-d1), so the degraded case is a device nobody commissioned.
3. **R4.4.2d says "the status topic"**, which reads literally as `<base>/status` — R4.5.1's retained
   `online`/`offline` will message. A JSON object there would break every entity's availability, so the
   result rides `<base>/diagnostics/state` beside `rssi` and `uptimeS`. Written down in
   `mqtt_publisher.h` because the next reader will check.

**⏸️ ONE THING IS NOT VERIFIED, AND IT IS THE SAFEGUARD THAT MATTERS MOST.**

The retain check (R4.4.2c) rests entirely on `event->retain` being populated for `MQTT_EVENT_DATA` in
this build, and **nothing that has run touches that field.** `mqtt_transport_esp.h` is not
host-compiled — which is why the host suite went green the instant the callback grew the parameter — so
the 1,997 checks cover the router's *decisions* and none of the adapter: the latch, the retain flag, the
R4.4.5 re-check, the persist-on-accept, the `requestFullPublish` arming and the boot seed are all
verified by reading only.

**Why this one matters more than the others.** The rate limit does not cover the case R4.4.2c exists
for. Trace it: a retained `RESET` sits on `<base>/cmd/reset-totals`; the device reboots, so the
`millis()` guard is clear; the broker redelivers on subscribe; the persisted guard passes because
`nowEpoch - persistedEpoch >= 60` for any downtime over a minute — **accepted, lifetime totals wiped,
every single boot.** That is precisely the loop R4.4.2 was designed around, and in this one scenario the
retain check is the *only* thing standing in it. `mqtt_command_router.h` says the retain check "is no
longer what the safety rests on"; for a retained message surviving a reboot, it is.

**Evidence so far, short of a board.** The SDK in use — `framework-arduinoespressif32` 3.20017.0,
`esp32s3` sdk — documents `retain` in `mqtt_client.h` as part of `MQTT_EVENT_DATA`'s context
(line 57, beside `topic`, `data_len` and `qos`), and declares it `bool retain` on `esp_mqtt_event_t`.
So it is specified as populated for INCOMING data in this exact version, not only meaningful on publish.
That is a documentation reading, not a measurement.

**What to do when the board arrives** — one command and a power cycle:

```
mosquitto_pub -r -t <base>/cmd/reset-totals -m RESET      # leave a retained command on the broker
# power-cycle the device, let it reconnect, then read M.I3's "Last cmd" row or register 565
mosquitto_pub -r -t <base>/cmd/reset-totals -m ""         # clear it, or the broker keeps serving it
```

`retained-ignored` means the chain works. **`accepted` means it does not, and the lifetime totals have
just been destroyed** — so run it on a device whose totals do not matter. While the board is out, the
same session should confirm the rest of the adapter: press each button from a Home Assistant dashboard
(so `payload_press` is exercised as HA sends it), press one twice inside a minute for `rate-limited`,
publish to `<base>/cmd/nonsense` for `unknown-command`, and check that an accepted reset publishes the
new totals immediately rather than at the next period.

**One consequence worth restating:** the panel cannot edit text at all (there is no on-device text
editor), so SSID, broker host and topics are portal-or-Modbus only. That is a decision, not a gap, and
it means a device with no Modbus master and an expired provisioning window has no route to its own
network settings. The portal timer is the thing standing between an operator and a reflash. And nothing
that changes *configuration* is reachable over MQTT, deliberately: a reset is recoverable by re-reading
the meter, a repointed broker is not.

---

## N-d ⏸️ The clock — ~~settable from no route at all~~ (N-d1 ✅), and its one trust signal unprotected (N-d2)

Recorded 2026-08-17, alongside N-c, because it is the same shape: a place where "every setting is
readable and writable over RS485" is not yet true. It is a queued feature, not a defect — but it was
never written down, and the four-surface question keeps being answered without it.

**N-d1 ✅ FIXED 2026-08-18 — the clock is settable over RS485.** A block at **50-55**: staged epoch halves at
50-51, `0x5AA5` to 52 to apply with source `operator`, the current time read-only at 53-54, the source at 55.
Epoch rather than broken-down fields (the clock's API and its floor are `uint32` seconds, and date arithmetic
in firmware is somewhere to be wrong about February); staged rather than applied on the low word (two
registers are not atomic under FC6, and a half-composed epoch is a timestamp nobody chose); and a refusal is a
**Modbus exception** rather than a status register, because `setTime` already refuses 2020…2100 outliers and
changes nothing, so there is no half-applied state to describe. 53-55 are read-only: a master able to write the
published time could make the device disagree with its own clock.

Documented in **§4.1.2** of `Project_document.md` and in `gen-registers.mjs`, whose reconciliation refused the
six registers until they were described — the gate working. Five host cases, negative-tested by applying on
the low word. Firmware compiles at RAM 24.6% / Flash 38.0%.

**One correction the clock made to me.** I expected the first write to BACKDATE an undated session by the
millis gap; `device_clock.cpp:107-116` records the sync epoch instead, because the session demonstrably began
before that moment and that is the tightest available bound. Backdating would look more precise and be a
fabrication, since nothing recorded a trustworthy anchor at the reset. The test asserts the bound and says so.

**The portal page followed the same day** — the clock's second route, specified as **R7.9d** in
`WiFi_MQTT_Connectivity.md`. It shows the time as `YYYY-MM-DD HH:MM:SS UTC` and who set it, and takes an epoch
computed by the BROWSER rather than a `datetime-local` string, because that widget would put date parsing and
a timezone guess in the firmware — the two things the Modbus block was designed to avoid. One code path, one
validation, one floor. It is not a catalogue setting: those are `int32_t` and an epoch outgrows that in 2038,
which would have put a Y2038 bug in the subsystem whose subject is being right about time.

**NTP followed too** (R7.13, built the same day): `NtpPolicy` — host-tested, Arduino-free — decides when to
ask, and `firmware.cpp` makes the only two calls it cannot: `configTime` to start SNTP and `time()` to read
what arrived. Ask on every association and every six hours (the RX8130CE drifts ±3 ppm, so that bounds the
error under a second); retry two minutes after a failure; fifteen seconds of silence counts as one. The device
knows SNTP answered because the SYSTEM clock becomes plausible — `time()` starts at 1970 and only SNTP moves
it, while `DeviceClock` keeps its own base, so it cannot be an echo of what the operator or RTC supplied.

**What is left of the chain:** nothing. All four routes are built — Modbus (N-d1), the portal (R7.9d), NTP
(R7.13), and MQTT, whose command topics landed with N-c on 2026-08-20. The two that work without a network
were the first two, deliberately.

**The gap it closed, for the record:**

`DeviceClock::setTime` has **no production caller**. Grepped 2026-08-17: the only callers are
`device_clock_test.cpp`, `modbus_manager_clock_test.cpp` and `interaction_test.cpp`. There is no
date/time block in `register_map.h` or `net_register_map.h`, no portal page offers one, no panel editor
reaches it (and could not — there is no text editor, per N-c), and NTP exists only in comments
(`wifi_manager.h` cites R7.13). So in the field the clock reads `UNSET` unless the RTC happens to hold a
date it is allowed to trust, and **a Modbus master has no way to set it.**

**Where the feature stopped**, in the owner's own ordering: the clock and its trust state are built,
a session reset is dated through `ModbusManager::Dependencies::clock`, and P3 shows the session start.
Next is the **Modbus date/time block** — the one that makes every later item settable — then the portal
page, then NTP on `WifiState::Connected`, with MQTT last because it needed N-c's command topics. All four
landed, in that order.

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

**Blocks.** Any timestamp being trustworthy in the field — **and N-d1 no longer does**: RS485 can set the
clock as of 2026-08-18. What is left is N-d2, a correction (the assert protecting the VLF probe's position)
plus a measurement (the power-cut test), and it needs the board.

---

## J9 🟢 `nyquist-warning` is an orphan screen in the dataset — residue of a design that was replaced

Found 2026-08-20 while deciding whether some stale local branches could be deleted. Recorded because
`ui_renderer.cpp:208` already states it and this file's own preamble says an open item recorded anywhere
else belongs here.

The dataset carries a screen `nyquist-warning` with two correct-looking flows (UP = back,
DOWN = `config.action.value.commit-override`) and **nothing navigates to it**: no flow names it as a
`targetScreenId`, and no `ui_pages.h` table names it, so `UiScreenRouter` can never resolve it.

**This is NOT a broken override path.** §5.5's prompt works, and works better than the screen would:
`ui_actions.cpp`'s `consumedByPrompt` reinterprets UP and DOWN on the editor screen itself while a prompt
is pending, and its comment gives the reason — "no new screen id, so the menu-pack completeness rule
stays satisfied and no dataset change is needed". The orphan screen is the earlier design, left behind
when the in-place prompt replaced it. Two local worktree branches held that earlier implementation and were
deleted on 2026-08-20 as superseded — not lost work, and their second claim, that register 30 stops
clearing, does not apply either, because `evaluateSensorDiagnostics` rebuilds `flags` from zero on every
call. The tips are `bd8a179` (the screen made reachable) and `fe8a952` (that plus two review commits:
scoping the latched override to the config it was granted for, and guarding `dialTo` against a closed
editor). Both are unreachable now and survive only in the reflog, so `git show` them within about ninety
days if the earlier design is ever worth reading.

**Why it is worth an ID rather than a shrug.** It costs one screen of the eighty in every generated table
and every `.uipack`, it is the twelfth screen carrying a full-screen `overlay-bg` that the §2c banner
round had to reason about, and — the real cost — the next person to read the dataset finds a screen with
plausible flows and no route to it, and has to redo the investigation above to learn it is deliberate.

**Two options, and this one is genuinely a preference.** Delete the screen and let §5.5 live entirely in
`consumedByPrompt`; or keep it and have the exporter's ring gate (`J1`) grow a *reachability* check that
would have named it automatically. **Recommendation: delete it**, and note in `consumedByPrompt`'s comment
that a screen for this once existed and why it does not now — a reachability gate is worth building, but
for its own sake rather than to justify keeping one orphan.

**Blocks.** Nothing. It is residue.

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
| `N-` | the batch opened after the 2026-08-12 rewrite, lettered `a`–`d` so it could not collide with the numbered groups | N-a–N-d2 | **N-b, N-c, N-d2 open**; N-a and N-d1 closed |
| `DF` | defect — opened 2026-08-18 | DF1–DF21 | **none open**; all twenty-one fixed |
| `J` | residue and hygiene — opened 2026-08-18, consolidated out of `MEMORY.md` §6 and `docs/backlog/` | J1–J8 | **all eight closed** the same day |

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

## Closed 2026-08-15 … 08-18 — DF1–DF21 and J1–J8, all of them

**Every defect and every residue item is closed**, between 2026-08-15 and 2026-08-18. They are kept verbatim,
with their diagnoses and what closed each, in
[`../archive/open_decisions-closed-2026-08-18.md`](../archive/open_decisions-closed-2026-08-18.md) — moved
there so this file is a work list again rather than a thousand lines of finished work, which is the same
reason the 2026-08-12 rewrite archived its predecessor.

Worth re-reading from that archive before writing the next diagnosis, because each was wrong in a way that is
easy to repeat:

| | |
| --- | --- |
| **DF10** | the recorded mismatch was real, and the same bound hid a SECOND defect nobody had noticed |
| **DF14** | tightening the bound as written would not have fixed the case the entry was about |
| **DF19** | the deferral rested on a cost — "72 screens" — that turned out to be six lines when measured |
| **J7** | the entry said nothing read the value; something did, and checking is why that behaviour survives |
| **DF18** | fixing it exposed a gate comparing positions but not sizes, which had let a stale artefact pass |

**Two things deliberately never given an ID**, recorded at the end of that archive: the simulator's
"missing nav stack", which could not be confirmed as a divergence at all, and SI-04's claim about Help-tab
contextual links, which was already stale when it was read.

## Closed earlier — A1–I1 and N-a, for reference

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
