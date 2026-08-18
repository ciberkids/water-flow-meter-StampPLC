# Session Handoff

**Updated:** 2026-08-03
**Branch:** `fix/pipeline-verification-gates` — 55 commits ahead of `main`, pushed, in sync
**CI:** green on `45f9fe6` (all three jobs)
**PR:** not opened

The previous version of this file was 22 commits stale and worse than useless: it told a
reader to protect a validation check that had since been deliberately retired, and listed as
"next" a slice that had shipped fifteen commits earlier. A four-lens documentation audit on
2026-08-01 rated it *badly-stale* and found 50 false claims across 17 documents.

So the rule for this file: **every claim names the command that proves it, or says it is
unverified.** If you are reading this more than a few days after the date above, run §1 before
trusting §3.

---

## 1. Verify the state in four commands

```bash
# 1. Firmware compiles (build the image once; ~5 min)
cd Water-Flow-Meter-PlatformIO && podman build -t stampplc-fw .
podman run --rm -v "$PWD":/workspace:Z -w /workspace stampplc-fw pio run -e m5stack-stamplc

# 2. Host tests + manifest freshness — no PlatformIO, no container, ~2 s
Water-Flow-Meter-PlatformIO/test/host/run.sh

# 3. Web suites
cd web/mockup && npm ci && npx tsc --noEmit && npm run test:unit && npm run test:exporter

# 4. Export gates
npm run export:firmware
```

Last run, 2026-08-03: firmware SUCCESS (RAM 7.8 %, Flash 18.1 %); **356 host checks** across
**ten** suites; 21 unit; 34 exporter; export `ok` with **9/9 gates and no warnings**.

The pack round trip spans both jobs: `npm run test:exporter` writes
`web/mockup/tests/fixtures/default.uipack`, and the firmware host suite reads it back with the
real C++ reader. Run the web suites **before** the host suites on a fresh clone, or `pack_test`
reports a missing fixture.

The Cypress suite is **gone** (2026-08-01). It ran the exporter with
`--screens tests/fixtures/legacy-screens.json` and no `--out` override, so every run
overwrote the committed firmware UI assets with a build from an obsolete fixture; it could not
pass, and it was not in CI. The one property it checked — dataset/IR parity — is now
`tools/exporter/__tests__/dataset_ir_parity.test.ts`, which runs the translation in memory and
writes nothing.

`npm run test:visual` is **unverified**: it gets past `tsc && vite build` but Playwright
browsers are not installed, and the snapshots need regenerating.

---

## 2. What this project is

A water-flow meter on an M5Stack StampPLC (ESP32-S3). It measures up to 8 flow sensors,
publishes over Modbus RTU, and has a 240×135 landscape display driven by **three buttons**.

The distinctive part: the on-device UI is **designed in a web app**, exported to JSON, and
translated into `constexpr` C++ tables the firmware renders. So three vocabularies — screen
ids, action ids and binding ids — must agree across a browser, a Node exporter and a firmware
build. Nearly every serious bug on this branch has been a disagreement between them.

| Layer | Where |
| --- | --- |
| Design tool | `web/mockup/` (React/Vite) |
| Exporter | `web/mockup/tools/exporter/` → `src/ui/generated/GeneratedUi.{h,cpp}` |
| Firmware | `Water-Flow-Meter-PlatformIO/src/` |
| Manifest | generated **from** firmware by `tools/manifest_gen/` (decision D2) |

---

## 3. Where things stand

### Working and tested

- Firmware compiles on two independent toolchains — the container and a clean GitHub runner.
- **A device harness exists.** `test/host/interaction_test.cpp` links the real `UiController`,
  `UiNavigator`, `UiScreenRouter`, `InteractionHandler`, `ButtonInputManager`, `ui_actions`,
  `ui_bindings`, `ui_settings`, `ui_renderer` and `LedController` against the real 48-screen
  generated table, with only `Preferences`, `M5StamPLC` and `ModbusMessage` stubbed
  (`test/host/stubs/`). No hardware. This is the emulator, and it is where new UI behaviour
  belongs.
- The manifest is **generated** from the firmware's own catalogues, and `run.sh` fails when the
  committed copy is stale. Action ids are cross-checked against their handler table by
  `static_assert`.
- CI runs three jobs on every push, and has already caught two bugs invisible locally.

### Fixed 2026-07-31 / 08-01 — each with a test that fails against the previous commit

| Was | Consequence |
| --- | --- |
| Confirm screens could not be confirmed — 0 of 48 screens matched the arming predicate | the only working factory reset was the blind combo §3.3 had retired |
| That blind UP+DOWN 30 s combo was **live**, on the same two buttons §3.1 uses for display-off | a **hazard**: holding the documented display-off gesture for 3 s started a wipe |
| Going idle cleared neither the nav stack nor the pending edit | the display woke with a live editor; the first ENTER could commit a Modbus write |
| Green LED required all **eight** channels | no realistic installation ever showed green |
| The dataset clamp used **portrait** bounds | 49 of 375 elements mutated on every ingest |
| Holding UP/DOWN on a navigation level stepped nothing | while the browser preview paged happily |
| The value editor opened on config **list** pages | swallowing UP/DOWN paging |
| Everything repainted at 1 Hz | the fast cadence was gated on a retired mode |
| Sensor settings were not persisted at commit | and a disabled channel's calibration never at all |
| Link apply never re-bound the slave ID | and rollback could therefore never fire |

### Closed since, on 2026-08-02/03

- **Acknowledgement toasts work.** Needed a screen-entry timer for the unattended kind of
  timeout, plus `UiNavigator::replaceCurrent` — a toast dismisses itself with `nav.back`, so
  pushing it onto the confirm screen would have ascended back into "RESET TOTALS?".
- **The Nyquist prompt** now fires only for an actual Nyquist refusal (it fired for all six
  reasons `writeSetting` can fail), and UP/DOWN do what the on-screen text says. Previously the
  prompt read "UP=Edit DOWN=Save anyway" while those buttons still adjusted the value.
- **D4** — the exporter now exports what is on screen. It was **two** parts, not three: theme
  writeback already existed at `App.tsx:1053-1062`. I reported otherwise twice, on an agent's
  finding; it was wrong.
- **The white acceptance latch** fires for every reset, not only the one that reboots.
- **README** rewritten — it recommended `pio test -d tests/build` twice and that target does
  not exist.
- **Cypress deleted**, replaced by an in-memory dataset/IR parity test. It ran the exporter with
  an obsolete fixture and no `--out`, overwriting committed firmware assets on every run.

### Menu packs — built, except the wiring

| Slice | State |
| --- | --- |
| `.uipack` format: emitter, reader, round trip, header fuzz | **done** — 20 checks |
| Boot selection ladder + anti-boot-loop guard | **done** — 46 checks |
| SPI arbitration (§4.10) | **done** — 49 checks |
| SD storage adapter | **done**, compiles; unproven on hardware |
| Select Menu page | **done** — 29 checks |
| `firmware.cpp` wiring + UP+DOWN+ENTER recovery (§3.4.1) | **not started** |

The real dataset emits to 16,995 bytes; string dedup saves 6,395 of 12,154 string bytes. The
round trip compares all 375 elements and 175 flows against the generated table, and caught two
real emitter/reader disagreements on its first run.

**WiFi/MQTT**: 2 of 9 slices — the text-entry engine and the text-setting type system. §4.4 of
the requirement still needs the corrected Home Assistant facts: `volume_flow_rate` with `L/s` is
valid but HA infers **0 decimals** for that device class, so a payload without
`suggested_display_precision` renders 0.35 L/s as "0". Floor is HA ≥ 2024.12.

### Nothing has run on hardware

Not once. The RS485 pin correction, the LED patterns, the SD adapter and every gesture and timing
are verified only against the datasheet, the specifications and 1,870 host checks (measured
2026-08-18). **G1** — measuring
`pollingRate_kHz` — is the only item that strictly needs the device, and it is now also a
prerequisite for the WiFi work, whose acceptance criterion is a 5 % budget against a radio-off
baseline that does not exist yet.

---

## 4. Documents you can and cannot trust

**This section no longer keeps its own table.** It duplicated `README.md`'s and the two disagreed —
including about whether `README.md` itself could be trusted. The map of which file answers which
question now lives in **`README.md` § Source of truth**, and the documents known to be wrong are
register items with IDs (**J4** `web/mockup/README.md`, **J5** `UI_Firmware_Interface.md`).

Two corrections this section was carrying, both verified 2026-08-18:

- Its claim that `README.md` **"references five paths that do not exist"** is **stale**. Every path the
  README names resolves — checked by extracting each one and testing it. Its firmware test command is
  also correct: there is no `pio test` target, and `test/host/run.sh` is the suite.
- Its **"356 host checks"** and the README's "199" were both wrong. Measured: **1,870 checks across 23
  suites, 0 failures**, plus a fresh manifest. Both files now carry the measured figure and its date.

The one judgement worth keeping here: **any undated status claim in this repository is untrustworthy,
including in this file.** Re-measure rather than quote.

---

## 5. The pattern worth carrying forward

Every significant bug on this branch was **a check that was not checking**, or a claim nothing
verified:

- Two prior handoffs claimed "aligned, tests passing" while the firmware did not compile.
- Three requirement documents said the blind factory-reset combo was removed. It was not.
- A code comment and decision H4 both asserted idle clears the navigation stack. It did not.
- Three unit tests asserted a **mirrored Y axis**; two asserted **portrait** display bounds.
  Both suites were green because they encoded the bug they should have caught.
- `run.sh` had no `-Werror`, so the `-Wswitch` guarantee protecting the manifest generator was
  one flag away from being an unread warning.
- The gesture reference's own status column was wrong within **hours** of being written.

So: prefer a check that executes over a sentence that asserts; negative-test every new gate by
breaking it deliberately and watching it fail; and treat a green suite over untested code as no
evidence at all. Before this branch, `interaction_handler.cpp`, `ui_controller.cpp` and
`ui_actions.cpp` were compiled by **no test** while nine gates reported success.

---

## 6. Where to pick up

**The item list is not here any more.** Every open item has a stable ID and lives in the index at
the top of `docs/active_work/open_decisions.md` — twenty-two lines with status and shape, governed by
rule **I3**. Cite the ID (`DF10`, `J3`, `N-d1`). What belongs in this section is only the ordering
advice an index cannot carry:

1. ~~**Wire the loader into `firmware.cpp`**~~ — **shipped, and this entry outlived it by several
   rounds.** Verified 2026-08-18: `packStorage`, `packLoader` and `packOutcome` are declared at
   `firmware.cpp:158–174`, §3.6's attempt-counter ladder is at :1058–1096, and §3.4.1's recovery
   route reaches `UiController::openPackSelector` (`ui_controller.cpp:273`). A stale instruction at
   the top of a pick-up list is exactly the failure §5 is about.
2. ~~**`DF17` first** — the register's only 🔴.~~ **Fixed 2026-08-18: 44 passed, 0 failed, exit 0,
   twice.** All eleven non-snapshot failures were the SPEC being stale — portrait bounds, removed
   features, renamed selectors, a legacy fixture the gates now rightly reject — and none was an app
   regression, which is what earned the baseline refresh. **The register has no 🔴 now.** Two
   consequences it did NOT close are filed rather than glossed: the banner still has no pixel-level
   verification because no snapshot drives a warning state (**DF20**), and CI still runs no
   `test:visual` step (**DF21**), which is why it could rot unseen for months.
3. **Then WiFi slice N0** — the polling-rate spike, still the recommended next slice. Its
   acceptance criterion budgets 5 % against a radio-off baseline that does not exist yet; that
   baseline is **G1** and needs the board.
4. **The six "smaller and still open" items that used to be item 3 of this list** were verified on
   2026-08-18 and moved into the register with IDs: **J1** (no export gate for ring closure), **J2**
   (`animation` residue in five layers), **J3** (`carea/`, 18 tracked files), **J4**
   (`web/mockup/README.md`), **J5** (`UI_Firmware_Interface.md` — 4 actions listed against 19 in the
   catalogue), and **I2a** (nothing enforces the append-only rule, folded into I2 itself).
5. **One of the six got no ID, deliberately:** *the simulator's missing nav stack*. Probed
   2026-08-18 — no `navStack`, `backStack` or `history` in the mockup, but the device has no nav
   stack either (BACK resolves from the tree), so "missing" may describe a divergence that no longer
   exists. It needs someone to say what the simulator should do on BACK before it can be called a
   defect. Recorded under "Not promoted" at the end of the register's J section.

---

## 7. Mistakes to know about

Early on I ran `git add -A` and swept the user's uncommitted work into commit `af177cf` after
telling them their work was untouched. I corrected the record rather than rewriting history
unsupervised.

Related, on 2026-08-01: a batch of workflow agents wrote 1,235 lines directly into the working
tree when they had been asked only to *design* patches. Recoverable — the diff was preserved
and reviewed before anything was committed — and the reason later runs use
`isolation: 'worktree'`.
