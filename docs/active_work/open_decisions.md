# Open Decisions

One entry per decision. Each has **Question → Why it matters → Options → Recommendation
→ Blocks**. Record your answer inline under **Decision:** and date it.

Status legend: 🔴 blocks implementation now · 🟡 blocks a later slice · 🟢 independent

---

## A. Requirements rewrites

### A1 🔴 Modbus register allocation for the link settings (C1–C4)

**Question.** `Display_UI_Requirements.md` §5.2 lets the operator change Modbus slave
ID, baud rate, parity and stop bits from the device UI, and §5.4 says saves must
propagate to holding registers — but `Project_document.md` §4.1 allocates no registers
for them. What is the register layout, and are these writable over Modbus?

**Why it matters.** These are the parameters *of the transport*. A master that writes
baud = 19200 gets its reply at the old rate and then loses the link; changing slave ID
means the next poll addresses a device that no longer answers. Read vs write is
therefore a real safety decision, not a formality.

**Options.**
- **(a) Read-only mirror.** Registers 40–43 report live settings; the UI is the only
  writer. Simple, cannot brick the link, satisfies §5.4 literally.
- **(b) Read/write with explicit apply.** 40–43 stage values, register 44 commits on
  `0x5AA5` after the current response is flushed, then the UART restarts. Standard
  industrial pattern. Wants a rollback-on-silence timer so a bad write cannot
  permanently orphan the device.
- **(c) Not exposed over Modbus at all.** UI + NVS only; §5.4 gets amended.

**Recommendation.** Specify **(b)**'s full layout in the document now, implement
**(a)** in phase 1. The UI is the intended authoring surface, and the rollback timer is
real firmware complexity that is not needed to close the requirements gap.

Proposed block:

| Addr | Name | Type | Phase 1 | Phase 2 | Note |
| --- | --- | --- | --- | --- | --- |
| 40 | Modbus Slave ID | uint16 | R | R/W | see A2 for range |
| 41 | Baud Rate Index | uint16 | R | R/W | see A4 |
| 42 | Parity | uint16 | R | R/W | 0=None 1=Even 2=Odd |
| 43 | Stop Bits | uint16 | R | R/W | 1 or 2 |
| 44 | Link Apply | uint16 | — | W | write `0x5AA5` to commit 40–43 |
| 45 | Link Config Revision | uint16 | R | R | increments on apply, so a master can detect it |
| 46–47 | reserved | | | | |

`config.uartFrameSummary` ("8E1") stays derived — no register.

**Blocks.** Config-mode implementation (D1), and the firmware ordering fix noted below.

> Firmware consequence either way: link settings must load from NVS **before**
> `RS485_SERIAL_PORT.begin()`, but `logicTaskCode` currently calls
> `preferences.begin()` *after* the serial init, with baud and slave ID hardcoded
> (`9600, SERIAL_8N1`, `kDefaultModbusSlaveId`). That ordering has to flip, and
> factory reset must restore link defaults.

**Decision:** ✅ **(b)** full read/write with an explicit apply register, including rollback-on-silence. Specified in `Project_document.md` §4.1.1 (registers 40–47). (2026-07-30)

---

### A2 🟢 Modbus slave ID range

**Question.** §5.2 C1 says 1–255. Modbus RTU reserves 248–255, and 0 is the broadcast
address. Correct to 1–247?

**Recommendation.** Yes — 1–247.

**Decision:** ✅ **yes** — 1–247. Applied to `Display_UI_Requirements.md` §5.2 and `Project_document.md` §4.1.1. (2026-07-30)

---

### A3 🟢 Sensor Select page number contradiction

**Question.** §5.2 lists Sensor Select as **C7**; §5.3 says the sensor is "selected from
**C5**". C5 is LED Pulse Volume. Which is right?

**Recommendation.** C7 (matches the §5.2 table and `config-c7-sensor-select` in the
dataset). Fix the §5.3 cross-reference.

**Decision:** ✅ **C7** is correct. Resolved structurally: C7 is now a pure descent node ("Sensors ▸") with no value, so the C5 cross-reference disappears rather than being corrected. (2026-07-30)

---

### A4 🟢 Baud rate register encoding

**Question.** 115200 does not fit in a uint16 (max 65535). Index into the C2 list, or a
two-register uint32?

**Recommendation.** Index. It matches C2's "cycle list" semantics and keeps the block
compact.

**Decision:** ✅ **index** into the baud list, documented in `Project_document.md` §4.1.1 register 41 with the full list and the uint16 rationale. (2026-07-30)

---

### A5 🟢 Remove the stale out-of-scope clause

**Question.** `Project_document.md` §6.2 line 192 still lists "User interface on the
StampPLC's built-in screen" as out of scope.

**Answered 2026-07-30: in scope now.** Remaining work is the document edit — delete the
line and add a pointer to `Display_UI_Requirements.md`.

**Decision:** ✅ in scope (2026-07-30)

---

### A6 🟢 `eModbus.h` does not exist

**Question.** `Project_document.md` §3.1 lists `eModbus.h` as a core library header. No
such file exists in eModbus 1.7.4; the RTU server header is `ModbusServerRTU.h`.

**Recommendation.** Correct the document. (Code is already fixed.)

**Decision:** ✅ corrected — `Project_document.md` §3.1 now names `ModbusServerRTU.h`. (2026-07-30)

---

## B. Binding semantics

### B1 🟡 Does `sensor.N.<metric>` honour its metric suffix?

**Question.** `UiBindingResolver::resolveSensorBinding` matches the `sensor.N` prefix
and then **ignores** the suffix, choosing the metric from `context.page` instead. So
`sensor.3.sessionLiters` renders whatever the current page's metric is.

**Why it matters.** Today the authored metric is decorative — the mockup and the device
can disagree about what a value means, which is exactly the fidelity the tool exists to
guarantee.

**Options.**
- **(a) Honour the suffix.** Each element declares its own metric; a small
  suffix → metric table replaces the page switch.
- **(b) Keep page-driven.** Current behaviour; the suffix stays informational.

**Answered 2026-07-30: keep page reuse (b).** Counter-argument offered once, for the
record: each of P1–P6 already authors exactly one metric suffix, so per-page screens
(already implemented) are independent of this choice. Honouring the suffix **deletes**
the page switch rather than adding anything — it is the smaller change, not the larger
one. Reconfirm (b) or switch to (a).

**Decision:** ✅ **(a)** honour the metric suffix — "more flexible". Replaces the `context.page` switch in `resolveSensorBinding` with a suffix→metric table. (2026-07-30)

---

### B2 🔴 What config state does `UiRenderContext` expose?

**Question.** All 13 `config.*` bindings (`config.modbusSlaveId`, `.baudRate`,
`.parity`, `.stopBits`, `.ledPulseVolume`, `.ledPulsePeriod`, `.selectedSensor`,
`.sensor.connected/.multiplier/.adjust/.maxFlow/.undersamplingFlag/.nyquistWarning`,
`.uartFrameSummary`) plus `diagnostics.undersampling` are unresolved by the firmware —
they render their static placeholder text on hardware. `UiRenderContext` has no fields
for any of them.

**Why it matters.** These are the entirety of Configuration mode's display. The fields
added here determine the shape of the config-mode state model.

**Recommendation.** Add a `UiConfigContext` sub-struct carrying live link settings,
selected sensor index, the selected sensor's Q/F/Adjust/connected, the focused field,
the tentative edit value, and Nyquist prompt state. Depends on A1 for which link
settings exist.

**Blocks.** Config-mode slice.

**Decision:** ✅ follow recommendation — add a `UiConfigContext` sub-struct. Shape now also constrained by `NF-20260730-01` (navigation stack replaces `selectedSensor`). (2026-07-30)

---

## C. Feature scope

### C1 🟡 `animation` and `scrollbar` elements

**Question.** Both are requested in `Improvement_of_the_web_ui.md` (SVG frame animation
boxes; N-step scrollbars). The editor implements them fully — creation defaults,
viewport rendering, layout metrics, toolbox buttons. Firmware has no
`ui_exporter::ElementType` and no `UiRenderer` case for either.

**Current holding position.** Authorable in the tool; `renderable-element-kinds` blocks
export if used. Nothing unrenderable can reach hardware, but you can also draw
something you cannot ship.

**Options.**
- **(a) Implement in firmware.** Add both element types, IR mapping, emitter support and
  renderer cases. Scrollbar is cheap (a rect plus a position indicator). Animation
  needs an SVG→frame-buffer path and a frame scheduler — the larger piece.
- **(b) Scrollbar only.** Cheap, and it directly serves the "N screens, show current"
  request.
- **(c) Drop both.** Remove from `ElementKind`, delete the editor code, amend the
  proposal doc.

**Recommendation.** (b) now, (a) for animation later — the scrollbar is a small,
well-defined win; SVG animation on a 135×240 panel with a live polling task deserves
its own story.

**Decision:** ✅ **(b)** scrollbar only. Implement `scrollbar` end-to-end; `animation` stays authorable but export-blocked pending its own story. (2026-07-30)

---

### C2 🟡 The compile-time binding emitters

**Question.** `eventEmitter.ts` / `valueEmitter.ts` generate per-screen
`RegisterEvents_` and `UpdateValues_` functions against headers that do not exist
(`FirmwareAction.h`, `ScreenManager.h`, `FirmwareValues.h`) and duplicate the runtime
resolver the firmware actually uses. They are no longer called; their tests are marked
PARKED.

**Options.** (a) implement the firmware side and switch to compile-time binding;
(b) delete emitters and tests.

**Recommendation.** (b). The runtime resolver works, is already wired, and is
compatible with page reuse (B1). Two mechanisms for one job is what produced this mess.

**Decision:** ✅ **(b)** delete `eventEmitter.ts`, `valueEmitter.ts` and their tests. The runtime resolver is the surviving mechanism. (2026-07-30)

---

### C3 🟡 Which countdown screens are actually needed?

**Question.** The dataset defines six (`countdown-enter-config`,
`-reset-session`, `-reset-all`, `-factory-reset`, `-sensor-save`, `-config-exit`) plus
`nyquist-warning`. Only `countdown-factory-reset` is reachable, because the factory
reset combo is the only countdown the firmware produces.

**Recommendation.** Keep all six — §4.3 and §5.3 require each. They become reachable
with the countdown state machine (next slice). Confirm none are redundant.

**Decision:** ✅ keep all six. They become reachable via the confirm-screen model in `NF-20260730-01` §3.3, plus new toast screens. (2026-07-30)

---

## D. Architecture

### D0 🔴 Countdown trigger semantics: hold-to-confirm, or arm-then-wait?

**Question.** The requirements and the dataset describe two different state machines,
and they cannot both be implemented.

- **Requirements** (`Display_UI_Requirements.md` §4.3 note 1): "Countdown overlays
  display remaining seconds centrally; **release ENTER before zero aborts**." §5.3 rule 3
  is the same shape: "Long pressing ENTER … starts a 3 s countdown. **If held to zero** …",
  rule 4: "Long ENTER release before countdown completion cancels without saving."
  → **hold-to-confirm**: the button must stay down for the whole countdown.
- **Dataset** (`screens.json`): the info page's `button/enter/short` flow navigates to the
  countdown screen with no action; the countdown screen then has a **`timeout`** flow that
  fires the action, plus a `button/enter/short` flow that cancels.
  → **arm-then-wait**: a short press starts it, it completes on its own, and a second
  press cancels.

The same §4.3 table also says "ENTER (short) → Start 30 s countdown", which reads as
arming, not holding — so the requirement document is internally inconsistent too.

**Why it matters.** These need different firmware. Hold-to-confirm needs button *state*
polling driven off `isPressed()` (like the existing factory-reset combo). Arm-then-wait
needs a timer plus a timeout-flow evaluator in `InteractionHandler`, which does not exist
yet — it currently only matches `FlowTrigger::Button` and ignores `Timeout` entirely.

**Options.**
- **(a) Hold-to-confirm.** Matches the safety intent for a 30 s destructive reset, matches
  the already-working factory-reset combo, and means an accidental press cannot wipe
  totals. Requires rewriting the countdown flows in the dataset.
- **(b) Arm-then-wait.** Matches the dataset as authored. Needs a timeout-flow evaluator.
  A 30 s unattended countdown that completes by itself is a worse fit for "Reset All
  Measured", though the cancel flow mitigates it.
- **(c) Hybrid.** Arm on short press, then require the hold; cancel on release *or* on a
  second press.

**Recommendation.** **(a)**, and correct the §4.3 table wording plus the dataset flows.
Holding a button for 30 s to erase persistent totals is the safer, more conventional
interaction, and it reuses the factory-reset pattern already in `InteractionHandler`.

**Also needed either way:** every countdown flow in `screens.json` has **no
`timeoutMs`**. The durations from §4.3/§5 are: reset-all 30 s, reset-session 3 s,
enter-config 3 s, factory-reset 30 s, sensor-save 3 s, config-exit 3 s.

**Blocks.** The countdown state machine, and therefore reachability of
`core.action.reset-session`, `core.action.reset-all-measured` and
`core.action.factory-reset` (handlers now implemented but not yet reachable), plus
`ui.action.mode.configuration` from P7.

**Decision:** ✅ **(a)** hold-to-confirm. Implemented in `b93a514`. Refined by `NF-20260730-01` §3.3: the countdown now lives on a dedicated confirm screen reached by ENTER-short, rather than being armed by ENTER-long from the info page. (2026-07-30)
---

### D1 🔴 How do config pages map to screens? — **SUPERSEDED**

**Superseded 2026-07-30** by
[`NF-20260730-01-menu-navigation-model.md`](../new%20feature%20proposal/NF-20260730-01-menu-navigation-model.md),
which replaces the flat page model with a navigation tree: every level is a ring of
sibling pages ending in `BACK`, ENTER-short descends or commits, ENTER-long escapes to
the main screen. Answer that proposal instead of this entry.

The proposal also supersedes or resolves: **A3** (the C5-vs-C7 contradiction disappears
because C7 becomes a pure descent node with no value), **H1** (ENTER-long is the escape
gesture; Idle is manual from P0 only), **H3** (no back-to-idle countdown), and **H6**
(footer hints and LED legend placement).

<details>
<summary>Original entry</summary>

### D1 (original) How do config pages map to screens?

**Question.** `UiScreenRouter` resolves Configuration mode to a single screen
(`config-c1-modbus-id`). The requirements need C1–C7 plus the per-sensor sub-menu
S1–S4, with ENTER-short entering field edit and ENTER-long cancelling.

**Options.**
- **(a) Mirror the info-page model.** A `UiConfigPage` enum plus a
  `kConfigScreenIds[]` table, and a second table for `kSensorScreenIds[]`, with a
  "current sub-menu" flag. Consistent with what already works.
- **(b) One flat page list** covering C1–C7 and S1–S4 together, with S-pages skipped
  unless a sensor is selected.

**Recommendation.** (a). It matches the info-mode structure, keeps the `static_assert`
guard, and models the sub-menu scoping §5.3 describes.

**Blocks.** Config-mode slice. Depends on A1 and B2.

**Decision:** superseded — see the proposal linked above.

</details>

---

### D5 ✅ Settable elements are a fixed catalogue, not free-form bindings

**Decided 2026-07-30 (user).** The web design tool must offer the settable entities as
**fixed, pre-declared items** rather than free-form elements carrying arbitrary binding
strings. What the dataset controls is **screen order, sub-level nesting, and the placement
and wording of text** — not which values exist.

**Why this matters.** Today a designer types a binding string and nothing checks it against
firmware until export (and until this session, not even then). That is how 13 `config.*`
bindings and `diagnostics.undersampling` ended up rendering placeholder text on hardware
while looking live in the mockup. A catalogue makes the connection **structural** rather
than a name that has to match: you cannot place a value the firmware does not expose,
because the palette only contains values it does expose.

**Consequences to honour when the web UI is next touched:**

1. The design tool's element palette splits in two: **layout elements** the designer places
   freely (text, box, scrollbar) and **bound elements** chosen from the firmware catalogue
   (values, settings, actions). A bound element cannot be created with a hand-typed ID.
2. The catalogue is the firmware manifest, which makes **D2 (generate the manifest from
   firmware) a prerequisite rather than an optimisation** — a hand-maintained catalogue can
   still over-claim, which is exactly the current failure with six unimplemented actions.
3. The manifest's value entries need the descriptor fields from
   `Display_UI_Requirements.md` §8 item 6 — `min`, `max`, `step`, `enum`, `unit` — so one
   declaration drives the editor behaviour in both the simulator and the firmware.
4. `manifest-value-coverage` and `manifest-action-coverage` (added this session) become
   belt-and-braces rather than the primary guard: the UI should make an unknown binding
   impossible to author in the first place.
5. Screen order and nesting become first-class dataset concepts — see
   `NF-20260730-01` §3.1/§5.7 for the level-and-ring model the dataset must express.

**Blocks.** Nothing immediately; it constrains the web-UI slice. Recorded so it is not
rediscovered later.

**Decision:** ✅ fixed catalogue (2026-07-30)

---

### D2 🟡 Should the manifest be generated from firmware?

**Question.** `actionManifest.json` is hand-maintained and must mirror three firmware
facts: `kDefaultBindings` (actions), the binding resolver's vocabulary (values), and
`kInfoScreenIds` (screens). The exporter can now check dataset ↔ manifest, but nothing
checks manifest ↔ firmware. That remaining gap is why six declared actions have no
handler and the export still passes.

**Why it matters.** `spike-report-SI-20251111-05.md` already evaluated this and
recommended **Approach A** (a parallel `constexpr kActionDescriptors[]` next to
`kDefaultBindings`, guarded by `static_assert`, plus a `manifest_gen` host binary). The
implementation steps were never carried out.

**Recommendation.** Yes, implement Approach A, extended to cover values and screens.
Until then, treat the manifest as an *intent* document and accept that it can over-claim.

**Decision:** ✅ **yes** — implement the spike's Approach A, extended to values and screens. Promoted to a prerequisite by **D5**: a catalogue-driven design tool needs a manifest that cannot over-claim. (2026-07-30)

---

### D3 🔴 Display orientation

**Question.** `screens.json` is authored to exactly 135 wide × 240 tall. The firmware
calls `display.setRotation(1)` and `drawWarningBanner` hardcodes a 240-px-wide banner.
Both cannot be right.

**Options.** (a) portrait 135×240 — drop the rotation, fix the banner width to 135;
(b) landscape 240×135 — re-author every screen and change `DISPLAY_WIDTH/HEIGHT` in the
web tool.

**Recommendation.** (a). The hardware spec is 135×240, the dataset and all 27 screens
are already built for it, and the mockup's bounds checking assumes it. The rotation call
and the 240 literal look like leftovers.

**Blocks.** Anything that trusts on-screen geometry. Cheap to fix, high value —
worth doing in the next slice.

**Decision:** ✅ **landscape 240 × 135.** Notation clarified: the first number is width, so the panel's native 135×240 is portrait and `setRotation(1)` gives landscape — meaning the firmware was already landscape and the *dataset* was authored portrait. 38 of 232 elements need repositioning and 105 px of width is currently unused. (2026-07-30)

---

### D4 🟡 How does the browser save the dataset?

**Question.** The app imports `screens.json` statically and keeps edits in React state.
"Export to Firmware" runs a CLI that reads `screens.json` **from disk**. There is no
`/api/save`, so the real loop is: edit → download JSON → manually overwrite the file →
export. Clicking Export without that manual step exports the previous dataset.

**Options.** (a) add `POST /api/save` writing `src/data/screens.json`;
(b) have `/api/export` accept the dataset in the request body;
(c) keep manual, but make the panel warn when in-memory state differs from disk.

**Recommendation.** (b) — it removes the stale-export trap entirely and keeps one
round-trip. (a) is fine too if you want the file to be the durable record.

**Decision:** ✅ **(b)** POST the dataset in the export request body, **plus a checked-in baseline** dataset so a reset never starts from scratch. (2026-07-30)

---

## E. Data correctness

### E1 🔴 `actionManifest.json` register numbers are wrong

**Question.** Per-sensor stride is 40 (`SENSOR_BLOCK_SIZE`, spec §4.2:
`100 + (n-1)*40`), but the manifest increments by 1: `sensor.2.instantFlow` = 102 where
it should be 141. Every sensor ≥ 2 is wrong, and `sensor.3.instantFlow` (103) collides
with `sensor.1.cumulativeLiters` (103). Many derived entries have `register: null`.

**Recommendation.** Fix the arithmetic from `sensorBaseAddress(n) + offset`. If D2 is
adopted, generate it instead of hand-fixing. Left untouched pending your call because
it sits inside the A1/D2 discussion.

**Decision:** ✅ fix the arithmetic to `sensorBaseAddress(n) + offset` (stride 40). If **D2** lands first, generate it instead of hand-fixing. (2026-07-30)

---

### E2 🟢 Mock actions in the firmware manifest

**Question.** `ui.mock.value-edit` and `ui.mock.value-save` are simulation-only helpers
living in the firmware manifest alongside real actions. `config.action.field.cancel` is
declared but unused by the dataset (it is needed by §5.1's ENTER-long-cancels-edit).

**Recommendation.** Move the two `ui.mock.*` entries to a separate simulation catalogue
so the firmware manifest only claims things firmware can do. Keep
`config.action.field.cancel` — it becomes used in the config slice.

**Decision:** ✅ follow recommendation — move `ui.mock.*` to a separate simulation catalogue; keep `config.action.field.cancel`. (2026-07-30)

---

## F. Hygiene and process

### F1 🟢 `web/mockup/node_modules` is committed

10,615 files — was 10,615 of the repo's 10,917 tracked files before this branch. The
`.gitignore` already lists `node_modules/`, but they were tracked before the rule
existed. The tracked copy is also **stale**: it lacks `vitest`, so a fresh clone cannot
run the tests. `dist-exporter/`, `dist/`, `.pio/`, logs and `test-results/` were
untracked on this branch already.

**Recommendation.** `git rm -r --cached web/mockup/node_modules`. One noisy commit, then
clones get smaller and `npm ci` becomes the real install path.

**Decision:** ✅ follow recommendation — `git rm -r --cached web/mockup/node_modules`. Extra justification found: the tracked copy is **stale** (no `vitest`), so a fresh clone cannot run the tests. (2026-07-30)

---

### F2 🟢 Stray `carea/` directory

An untracked bare git repository at the repo root (`HEAD`, `config`, `hooks/`,
`info/exclude`) — almost certainly a mistyped `git clone`/`git init`.

**Recommendation.** Delete. Confirm you have nothing in it first.

**Decision:** ✅ delete `carea/`. Verified empty of project content — a stray bare repo. (2026-07-30)

---

### F3 🟢 Playwright browsers not installed

`npm run test:visual` now gets past `tsc && vite build` but cannot launch a browser.
Restoring `animation`/`scrollbar` re-enables two DesignToolbox buttons, so the
`workspace-design-*.png` snapshots very likely need regenerating.

**Recommendation.** `npx playwright install chromium` (~150 MB), run the suite, and
refresh the affected baselines. Until then treat visual regression as unverified.

**Decision:** ✅ use the container if possible, otherwise install Playwright locally. (2026-07-30)

---

### F4 🟢 `active_work_tracker.md` is not a record of state

All 37 items marked `[x]`, and every link points at `../Implemented stories _ IGNORE/`
or `../missing implementation/` — folders since renamed to `archive/` and `backlog/`.
Every link is broken and the completion claims contradict the code.

**Recommendation.** Rewrite from the real state, and point the SI-20251111/20260123
entries at their actual `backlog/` paths.

**Decision:** ✅ execute the clean-up — rewrite from real state, fix the broken links. (2026-07-30)

---

### F5 🟢 `MEMORY.md` and `README.md` contain stale claims

`MEMORY.md` says "fully realigned", "Tests Passing", "All 21 tests pass" — true only of
`test:exporter`, while the firmware did not compile. `README.md` references five paths
that do not exist (`web/backups/`, `web/Water Flow Meter PlatformIO/`,
`docs/missing implementation`, `docs/stories to implement`, `pio test -d tests/build`).

**Recommendation.** Rewrite both. Keep handoff docs claim-free unless a command backs
the claim.

**Decision:** ✅ rewrite both. (2026-07-30)

---

### F6 🟢 No CI

There is no `.github/workflows/`. Everything found in this review would have been caught
by one.

**Recommendation.** Add a workflow running `npm ci`, `npm run test:unit`,
`npm run test:exporter`, `npm run build`, and the containerised
`pio run -e m5stack-stamplc`. That last one is the important one.

**Decision:** ✅ follow recommendation, using containers for the firmware build step. (2026-07-30)

---

### F7 🟢 Six agent-tool config directories

`.antigravity/`, `.antigravitycli/`, `.beads/`, `.codex/`, `.kiro/`, `.shirika/` plus
`AGENTS.md`. `AGENTS.md` mandates a `bd`-based workflow ("Landing the Plane") that this
session did not follow, because it also mandates pushing.

**Recommendation.** Decide which tool is authoritative and prune the rest; reconcile
`AGENTS.md` with how you actually want sessions to end (in particular whether an agent
should push unprompted).

**Decision:** ✅ remove everything, keep only Claude Code. `.beads/issues.jsonl` verified **empty**, so no issue data is lost. Note `AGENTS.md` mandates the `bd` workflow and must go with it. (2026-07-30)

---

## I. Loadable menu packs

### I1 ✅ Menu-pack design accepted

**Decided 2026-07-30.** Full specification in
[`Loadable_UI_Menu_Packs.md`](../Requirements/feature%20addition/Loadable_UI_Menu_Packs.md)
0.2, with all twelve §7 recommendations adopted.

The two shaping decisions:

- **A menu must be complete** — every settable value in the catalogue needs a reachable
  editor. This makes the required action set derivable from the catalogue rather than
  declared by the pack, so a pack cannot claim completeness it does not have.
- **Selection is a pointer file, not a symlink.** FAT has no symlinks; verified against
  `fatfs/src/ff.h`, whose API offers `f_rename`/`f_unlink`/`f_readdir` and nothing resembling
  `link`. `/ui/active` holds one line naming the selected pack, which keeps the choice on the
  card so a card prepared on a PC boots the intended menu on any unit.

### I2 🔴 The catalogue is append-only — standing rule

**Consequence of I1/Q4.** Once menu packs exist, **no value or action may be renamed or
removed from the catalogue** — only added. A pack authored against an older catalogue is
accepted when `packAbi <= firmwareAbi`, which is only sound if the vocabulary strictly grows.

This is easy to violate accidentally and expensive to detect: the breakage appears on a
device with a card in it, not at compile time. Worth a check in the exporter comparing the
catalogue against its previous committed state.

**Decision:** ✅ append-only (2026-07-30)

---

## H. Menu behaviour refinement

Found while implementing D0(a). These are all requirement-level: the spec is either
self-contradictory or silent, so the firmware cannot be "correct" until they're settled.

### H1 ✅ ENTER-long is overloaded — P2–P7 can no longer reach Idle

**Resolved 2026-07-30** by
[`NF-20260730-01`](../new%20feature%20proposal/NF-20260730-01-menu-navigation-model.md) §3.5:
a short **UP + DOWN** press turns the display off from any screen, so ENTER-long means
"escape to the main screen" and nothing else. Consequence: §2's UP+DOWN 30 s factory-reset
combo is retired and factory reset becomes page **P8** with a confirm screen. H3 is
resolved with it (no back-to-idle countdown).

<details>
<summary>Original entry</summary>

### H1 (original) ENTER-long is overloaded

**Question.** §4.1 makes ENTER-long the global Info → Idle gesture ("Info → Idle: ENTER
long (3s)"). §4.3's table gives P2–P6 a reset countdown and P7 an enter-config countdown.
With D0(a) hold-to-confirm, ENTER-long on P2–P7 now arms those countdowns, so **manual
idle is only reachable from P0 and P1**. The 120 s auto-idle still covers everything.

**Options.**
- **(a) Accept.** Idle from P0/P1 plus auto-idle. Simplest; costs nothing to implement.
- **(b) Two-stage hold.** Holding ENTER on P2–P7 arms the countdown; continuing to hold
  past it (say 5 s) goes idle instead. Discoverable? Doubtful.
- **(c) A dedicated idle page.** Add P8 "Display Off" whose ENTER-hold goes idle. Costs a
  page in the ring but is explicit and consistent.
- **(d) Move the resets off ENTER.** e.g. UP-long on the relevant page. Frees ENTER-long
  to be globally "idle", but invents a gesture the requirements don't have.

**Recommendation.** (a) for now, (c) if you want manual idle from anywhere. Worth noting
the display is off for burn-in reasons, and auto-idle already handles that.

**Decision:** superseded — resolved by a fifth option not listed here: move the gesture off
ENTER entirely and onto UP+DOWN. See the proposal linked above.

</details>

---

### H2 🟡 Long-press threshold is 1.5 s, but the spec says 3 s

**Question.** `ButtonInputManager::kLongPressThresholdMs = 1500`, and §2's button table
says "Long Press (≥1.5 s)". But §4.1 and §4.3 both specify "ENTER long (3 s)" for
back-to-idle, and §5.5 says "Releasing before 1.5 s cancels; holding to 3 s exits".

So there are two different long-press meanings: a 1.5 s *gesture* threshold and a 3 s
*hold-to-confirm* duration. Today only the 1.5 s one exists, so "ENTER long (3 s)" fires
at 1.5 s.

**Recommendation.** Keep 1.5 s as the gesture threshold (it arms), and express every 3 s
requirement as a countdown duration in the dataset — which is what the countdown
machinery now does. Then add a 3 s back-to-idle countdown (see H3) so §4.1 is honoured.

**Decision:** ✅ resolved by `Display_UI_Requirements.md` §3.1: **1.5 s is the gesture boundary**, and every longer duration (3 s, 30 s) is an on-screen countdown, never a gesture threshold. The two meanings of "long" are now separated. (2026-07-30)
---

### H3 ✅ Back-to-idle has no countdown

**Resolved 2026-07-30** by `NF-20260730-01` §3.5: option (b), accept immediate idle. Going
idle is reversible by any button press, so a countdown protects nothing. The gesture is now
UP+DOWN rather than ENTER-long.

<details>
<summary>Original entry</summary>

### H3 (original) Back-to-idle has no countdown

**Question.** §4.3 gives P0 and P1 "ENTER (long) → Back-to-idle countdown (3 s)", but the
dataset fires `ui.action.mode.idle` immediately on ENTER-long, and there is no
`countdown-idle` screen (C3 confirmed the existing six, none of which is one).

**Options.** (a) add a `countdown-idle` screen and let the existing machinery handle it;
(b) accept immediate idle — it is non-destructive and any button press wakes the display.

**Recommendation.** (b), and correct §4.3's wording. A countdown to protect a reversible,
harmless action is friction for no safety gain.

**Decision:** ✅ (b) — see the proposal linked above.

</details>

---

### H4 🟡 §5.5 contradicts itself on where config-exit lands

**Question.** §5.5: "holding to 3 s **exits to Info mode** and **turns off the display
(transition to Idle)**." Info and Idle are different states. The dataset hedges the same
way: `countdown-config-exit` has `actionId: ui.action.mode.info` but
`targetScreenId: state-idle`.

**Recommendation.** Exit to **Info** (P0). Idle is what the 120 s timeout is for, and
dropping straight to a dark screen after a deliberate save/exit reads as a fault.

**Decision:** ✅ resolved by §5.6: leaving Configuration returns to **P0 (Info)** and does *not* turn the display off. Display-off is UP+DOWN or the 120 s timeout, both of which also clear the navigation stack. (2026-07-30)
---

### H5 🟢 UP/DOWN during a countdown — fixed to match the spec

§4.3 note 2 says UP/DOWN have "no effect" during a countdown. The first cut of the
countdown machine cancelled on UP/DOWN; corrected to ignore them. Only releasing ENTER
aborts. No decision needed — recorded so the behaviour is traceable.

**Decision:** ✅ ignore UP/DOWN, only ENTER release aborts (2026-07-30)

---

### H6 🟡 Per-page helper text and the LED legend are incomplete

**Question.** §4.3 note 4 wants per-page helper text summarising the button hints, note 5
and §6 note 6 want the RGB LED legend visible in info mode. Today `legend.led` appears on
`info-p1-instant-flow` only, and every `footer-hint` sits at y≈226 — out of bounds once
the display is landscape (D3).

**Recommendation.** Fold into the D3 re-layout: give every info page a footer hint, and
put the LED legend on P0 (the landing page) rather than P1.

**Decision:** ✅ resolved by §4.3 notes 4–5 and §6: **every** info page carries a footer hint, and the LED legend moves to P0. Element repositioning happens as part of the D3 landscape re-layout. (2026-07-30)
---

### G2 ✅ Modbus task affinity broke the dedicated-core guarantee — FIXED

**Found 2026-07-30** while reviewing whether the two-core split still holds.

`firmware.cpp` called `modbus.begin(RS485_SERIAL_PORT)` without a core ID.
`ModbusServerRTU::doBegin()` defaults `coreID = -1`, which becomes
**`tskNO_AFFINITY`**, and the task is created at **priority 8**. So the Modbus server
task was free to be scheduled on **core 0**, where it preempts the priority-2 polling
task — directly contradicting `Project_document.md` §3.2: "pulse counting is never
delayed or interrupted by other application logic, such as Modbus communication delays."

**Fixed:** `modbus.begin(RS485_SERIAL_PORT, 1)` pins it to core 1. Priority 8 also means
Modbus still preempts the logic/UI task (priority 1) on core 1, so a slow redraw cannot
delay a Modbus response — the ordering is now correct on both cores.

Resulting layout:

| Core | Task | Priority |
| --- | --- | --- |
| 0 | `PollingTask` (+ IDLE0) | 2 |
| 1 | eModbus server | 8 |
| 1 | `LogicTask` (sensors, LED, UI) | 1 |

**Decision:** ✅ fixed (2026-07-30)

---

### G3 ✅ Configuration mode redrew on every loop tick — FIXED

`UiRenderer::update()` throttled to 1 Hz only when `context.mode == UiMode::Info`, so
Configuration and countdown screens redrew on **every logic-loop iteration (~1 ms)** — a
full 240x135x16bpp SPI transfer each time. Harmless today because Configuration mode is
unreachable, but it would have surfaced the moment the config slice landed.

**Fixed:** interactive modes throttle to 80 ms (fast enough for §7's "acknowledge within
100 ms"), Info stays at 1 s.

**Decision:** ✅ fixed (2026-07-30)

---

## G. Hardware risk

### G1 🔴 Polling rate regression from the M5StamPLC 1.2.0 API

**Question.** 1.2.0 removed the bulk `IO.getDigitalInput()`. The only public input API
is `readPlcInput(ch)`, one I2C expander read per channel — so an 8-channel sample is 8
I2C transactions where it used to be 1. That lowers achievable `pollingRate_kHz` roughly
8-fold, and `pollingRate_kHz` is exactly what `meetsNyquistLimit()` budgets against
(`pollingRate_kHz * 1000 >= 2 * f_theoretical`).

**Why it matters.** "No missed pulses" is the core requirement. This silently shrinks
the sensor configurations the device will accept, and the Nyquist check will start
rejecting configs that used to pass.

**Options.**
- **(a) Measure first.** Flash, read register 0–1, and see what the real rate is. It may
  still be far above what 8 YF-B10 sensors need.
- **(b) Bulk read the expander directly.** One I2C transaction for all 8 bits.
  `M5StamPLC` keeps `_io_expander_b` private, so this means a local PI4IOE5V6408 access
  path and careful I2C arbitration against the library.
- **(c) Pin to M5StamPLC 1.1.x** if it still exposes the bulk accessor.

**Recommendation.** (a) first — it is one flash and one register read, and it decides
whether (b) is needed at all. Do not invest in (b) before the measurement.

**Blocks.** Trusting the sensor configuration limits on real hardware.

**Decision:** ✅ **(a) then (b)** — measure the real polling rate on hardware first; pursue the bulk expander read only if the measurement demands it. (a) needs you to flash and read register 0–1. (2026-07-30)

---

## Suggested order

1. **D0** — smallest question, biggest unblock: three action handlers are implemented
   but unreachable until the countdown state machine exists.
2. **A1** (+A2, A4) — unblocks A5/A6 doc edits and the whole config slice.
3. **D3** — cheap, and everything visual depends on it.
4. **G1** — one measurement, and it may quietly invalidate config assumptions.
5. **B2 + D1** — the config-mode state model, once A1 is settled.
5. **C1, C2, C3** — feature scope, once the above is moving.
6. **D2 + E1** — make the manifest generated rather than asserted.
7. **D4** — remove the stale-export trap.
8. **F1–F7** — hygiene, any time; **F6 (CI)** pays for itself immediately.
