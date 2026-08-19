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

**The item list is not here.** Every open item has a stable ID and lives in the index at the top of
`docs/active_work/open_decisions.md` — **ten lines**, no 🔴, governed by rule **I3**. Cite the ID
(`DF18`, `J1`, `N-d1`). This section carries only what an index cannot: the ordering, and the state of
the working tree.

### Session of 2026-08-18 — ten items closed, four opened

Closed, each with its reasoning in the register entry rather than only in the diff: **DF17** (visual
suite, 32 of 46 failing — every non-snapshot failure was the SPEC being stale, not one app regression),
**DF10** (portal now one-based), **DF14** (a negative offset made a dry pipe read flow), **DF15**
(register table marked and corrected), **DF16** (`ready` derived), **DF19** (six 104 px scrollbars),
**J3** (`carea/`), **J4** (nine false claims, not three), **J5** (action table generated + CI gate),
**J8** (one clamp rule).

Opened by that work, because a repaired gate finds things: **DF20** (no snapshot drives a warning
state, so the banner still has no pixel verification), **DF21** (CI runs no `test:visual`), **J6**
(nothing verifies accessibility), **J7** (the transition preview's five layers survive its removal).

### THE WORKING TREE IS NOT CLEAN, and that is deliberate

40 modified files and 3 untracked ones are the **§2c banner round, unfinished and not mine**:
`DisplayViewport.tsx`, `warningBanner.ts` + its test (untracked), `ui_root_tail.h` (untracked),
`Display_Per_Screen_Spec.md`, `screen-geometry.ts`'s band check, and the rest. Nothing from
2026-08-18's ten closures is uncommitted; everything is pushed.

**Two of my changes could NOT be committed and sit in that tree deliberately:**

0. **`tests/visual/warning-banner.spec.ts`** and its `.display-surface` baseline (untracked), plus three
   `data-testid`s on the banner in `DisplayViewport` — DF20's whole deliverable, waiting on the §2c round.
0. The **audit's own comment** in `screen-geometry.ts`, which said non-zero was the correct state of the
   banner-band line. It is zero now, by repair — the comment says so, and says a future non-zero is real.
0. **`ui_root_tail.h`'s `Screen` initializer**, one pair shorter since J2 narrowed the generated struct.
   Without it the firmware does not compile — so whoever commits that file must keep this edit.
1. The **scrollbar band rule** in `tools/audit/screen-geometry.ts` — reports any scrollbar reaching the
   band, 6 before DF19's fix and 0 after. It builds on the band check from the uncommitted §2c work, so
   it belongs to that round.
2. Two comment tweaks whose anchors exist only in the working tree (`warningBanner.test.ts`, and one
   line in `sensorConfig.test.ts`).

**Staging discipline this tree forces, learned the hard way** — see §7. A commit that names one of those
40 files sweeps someone else's work in. `git add <path>` is not enough; check `git status` for the
SPECIFIC file first, and if it is dirty, stage HEAD-plus-your-hunks:

```bash
git show HEAD:path > /tmp/base && python3 - <<'EOF'   # apply only your replacement to /tmp/base
EOF
blob=$(git hash-object -w /tmp/base) && git update-index --cacheinfo 100644,$blob,path
```

**And then VERIFY THE INDEX, not the working tree.** A reconstruction is a different artefact from the
file you tested, and commit `97132ea` proved it: three `/**` openers were dropped by a helper that started
at a marker phrase on a comment's second line, so HEAD did not typecheck while the working tree passed 220
tests. Repaired in `54f191d`. The check that catches it:

```bash
tree=$(git write-tree) && mkdir /tmp/chk && git archive $tree | tar -x -C /tmp/chk
cd /tmp/chk/web/mockup && ln -s <repo>/web/mockup/node_modules node_modules
npx tsc --noEmit && npm run test:unit && npm run test:exporter
```

### Two local gates fire for a reason that is not a fault

The catalogue's uncommitted 19th action (`ui.action.pack.select-menu`) means
`node tools/wiki/gen-actions.mjs --write` shows a one-row diff locally, and the committed action table
carries **18** rows. Whoever commits that catalogue entry runs `--write` in the same commit; the CI gate
says so if they forget. Do not "fix" the table by committing the 19-row version — CI would fail on a
clean checkout, where the catalogue has 18.

### Next, in the order I would take them

1. ~~**`J2` and `J7` together**~~ — **done 2026-08-18**, and doing them together paid: J2's emitter change
   broke on a positional `ui_exporter::Screen` initializer in the untracked `ui_root_tail.h`, which J7 alone
   would not have surfaced. Flash fell 656 bytes. J7 turned out to be a NARROWING, not a deletion: its
   entry claimed nothing read the state, and `ScreenSelector`'s `.preview-target` ring does.
2. ~~**`DF18`**~~ — **done 2026-08-18.** The spec file exists, the audit reads 0 findings because the
   overlap is gone, and it surfaced a gate that was not checking: the host round-trip compared element
   positions but not SIZES, so DF19's six scrollbar heights had left `default.uipack` stale and passing.
   Fixed and negative-tested. Look for that shape elsewhere — a gate reading a committed snapshot has to
   compare every field it can.
3. **`DF20` is WRITTEN and cannot be committed; `DF21` is next in line.** `tests/visual/warning-banner.spec.ts`
   exists, passes, and is negative-tested — but `warningBanner` is not in HEAD at all (zero references in
   `App.tsx` and `DisplayViewport.tsx`; `utils/warningBanner.ts` is untracked), so it lands with the §2c
   round. Wire `test:visual` into CI only after that, or CI locks in a gate that still misses the band.
4. **`J1`** and **`J6`** — export gates that `Loadable_UI_Menu_Packs.md` and SI-04 assume and that do
   not exist.
5. **WiFi slice N0** — still the recommended next feature slice, and still blocked on **G1**, which
   needs the board.

## 7. Mistakes to know about

Early on I ran `git add -A` and swept the user's uncommitted work into commit `af177cf` after
telling them their work was untouched. I corrected the record rather than rewriting history
unsupervised.

Related, on 2026-08-01: a batch of workflow agents wrote 1,235 lines directly into the working
tree when they had been asked only to *design* patches. Recoverable — the diff was preserved
and reviewed before anything was committed — and the reason later runs use
`isolation: 'worktree'`.
