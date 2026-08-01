# Session Handoff

**Updated:** 2026-08-01
**Branch:** `fix/pipeline-verification-gates` — 42 commits ahead of `main`, pushed, in sync
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

Last run, 2026-08-01: firmware SUCCESS (RAM 7.8 %, Flash 18.1 %); **180 host checks** across
six suites; 21 unit; 27 exporter; export `ok` with **9/9 gates and no warnings**.

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

### Known broken

- **Acknowledgement toasts never appear.** The screens exist; nothing drives their entry timer.
  `Display_UI_Requirements` §6.6 *certifies* this requirement satisfied. It is not.
- **The Nyquist override screen is never pushed**, so §5.5's "save anyway" path is unreachable.
- **D4** — the browser's dataset never reaches the exporter, so Export exports what is on
  *disk*. Also needs theme writeback: `App.tsx` syncs theme one way only.
- **Menu packs: 0 % built.** No SD-card code anywhere. The specification is thorough and its
  factual claims check out; nothing implements it.
- **WiFi/MQTT**: 2 of 9 slices done — the text-entry engine and the text-setting type system.

### Nothing has run on hardware

Not once. The RS485 pin correction, the LED patterns and every gesture and timing are verified
only against the datasheet, the specifications and 180 host checks. **G1** — measuring
`pollingRate_kHz` — is the only item that strictly needs the device, and it is now also a
prerequisite for the WiFi work, whose acceptance criterion is a 5 % budget against a radio-off
baseline that does not exist yet.

---

## 4. Documents you can and cannot trust

| Trust | Do not trust |
| --- | --- |
| `docs/Project definitions/Gesture_Reference.md` — every ✅ names its test | `README.md` — five referenced paths do not exist; its firmware test command cannot work |
| `docs/active_work/open_decisions.md` — the de-facto ADR, 42 entries | `web/mockup/README.md` — describes a portrait display and an empty dataset |
| `Project_document.md` §4.1/§4.2 — the register map, verified line-for-line | `UI_Firmware_Interface.md` — **the most dangerous live document**: lists 4 actions where there are 15 |
| `Loadable_UI_Menu_Packs.md` — as a *specification* | `Implementation_Alignment_Report.md` — audits a file layout that no longer exists |

`open_decisions.md` caveat: ✅ in a heading means **agreed**, not **landed**. The emoji is
severity; status lives in the `Decision:` line.

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

## 6. Mistakes to know about

Early on I ran `git add -A` and swept the user's uncommitted work into commit `af177cf` after
telling them their work was untouched. I corrected the record rather than rewriting history
unsupervised.

Related, on 2026-08-01: a batch of workflow agents wrote 1,235 lines directly into the working
tree when they had been asked only to *design* patches. Recoverable — the diff was preserved
and reviewed before anything was committed — and the reason later runs use
`isolation: 'worktree'`.
