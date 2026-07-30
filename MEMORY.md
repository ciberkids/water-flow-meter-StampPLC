# Session Handoff

**Updated:** 2026-07-30
**Branch:** `fix/pipeline-verification-gates` (9 commits, **local only — not pushed**)
**Base:** `main` @ `03ed89c`

Every claim below is backed by a command that was run. Where something is unverified it
says so. The previous version of this file claimed "fully realigned" and "tests passing"
while the firmware did not compile at all — please keep this one honest.

---

## 1. How to verify the current state

```bash
# Firmware toolchain (build the image once; ~5 min, downloads the ESP32 toolchain)
cd Water-Flow-Meter-PlatformIO
podman build -t stampplc-fw .
podman run --rm -v "$PWD":/workspace:Z -w /workspace stampplc-fw pio run -e m5stack-stamplc

# Web + exporter
cd web/mockup
npx tsc --noEmit          # 0 errors
npm run test:unit         # 18/18
npm run test:exporter     # 27/27
npm run export:firmware   # ok-with-warnings (see §3)
```

Last measured: firmware **SUCCESS**, RAM 7.8%, Flash 17.8% with all 48 screens.

`npm run test:visual` is **UNVERIFIED** — it gets past `tsc && vite build` but Playwright
browsers are not installed (`npx playwright install chromium`). Snapshots likely need
refreshing: the design toolbox gained and lost buttons this session.

---

## 2. What this session did

Started as "read the docs and report deviations". The report became
`docs/active_work/open_decisions.md` (31 decisions, all now answered).

**The pipeline was fundamentally broken and reported success.** Fixed, in order:

1. **The firmware had never compiled.** `platformio.ini` declared `framework = espidf`
   while every source is Arduino-style, so `setup()`/`loop()` were never called and
   `M5StamPLC.h` would not resolve. Also: sources use C++17 (`std::string_view`,
   `inline constexpr`, `std::clamp`) but the Arduino core defaults to gnu++11; `eModbus.h`
   does not exist (it is `ModbusServerRTU.h`); `eModbus` was absent from `lib_deps`;
   `M5StamPLC 1.2.0` removed `IO.getDigitalInput()`.
2. **`GeneratedUi.h` could not compile** — `struct Flow` referenced `KeyValue` before its
   declaration. The exporter's own compile gate would have caught it, except a missing
   PlatformIO degraded to "warning" while the export still said `status: "ok"`.
3. **The display would have been blank and every button dead.** `UiScreenMap` defaulted to
   screen IDs (`info-overview`, `configuration`, `countdown`) that exist nowhere in the
   dataset, so `screenForMode()` returned nullptr and `InteractionHandler` dropped every
   event.
4. **The design tool and the exporter read different fields.** The tool wrote
   `element.dataSourceId`; the exporter emits firmware bindings from `element.binding`
   only. Anything bound *through the UI* rendered in the mockup and was silently dropped
   from firmware. Unified on `binding`.
5. **The Modbus task was unpinned.** `modbus.begin()` without a core ID gives
   `tskNO_AFFINITY` at priority 8 — free to run on core 0 and preempt the priority-2
   polling task, contradicting `Project_document.md` §3.2. Now pinned to core 1.

Then: requirements rewritten for a hierarchical navigation model, and 38 screens generated
from the firmware value catalogue (dataset 27 → 48 screens).

---

## 3. The one thing that is deliberately NOT finished

**8 actions the dataset uses have no firmware handler**, so those buttons do nothing:

```
ui.action.nav.descend            config.action.value.increment
ui.action.nav.back               config.action.value.decrement
ui.action.nav.escape             config.action.value.commit
                                 config.action.value.discard
                                 config.action.value.commit-override
```

This is **reported on every export** by the `firmware-action-coverage` check, which is why
the export status is `ok-with-warnings` rather than `ok`. It cannot silently read as
complete. Do not "fix" the warning by deleting the check.

**Next slice = implement these.** In `Water-Flow-Meter-PlatformIO/src/`:

- A navigation stack in `ui_controller.h` (`UiNavNode { levelId, pageIndex }`, max depth 5)
  replacing the flat `UiPage`, with descend / back / escape.
- `UiEditorState { setting, pending, saved, holdStartMs, accelTier }` and the three
  acceleration tiers from `Display_UI_Requirements.md` §5.4, driven off
  `ButtonInputManager::pressedDuration()`.
- `UiRenderContext` gains the config fields the `config.*` bindings need (decision B2).
- Per-level screen tables in `ui_screen_router.cpp`, each `static_assert`ed against its
  page-count enum — the same pattern as the existing `kInfoScreenIds`.
- The commit path of §5.5, including the Nyquist prompt.

---

## 4. Also queued (all decided, none started)

| Item | What |
| --- | --- |
| **A1 firmware** | Link registers 40–47 with staged writes + `0x5AA5` apply + 60 s rollback. Needs `preferences.begin()` moved *before* `Serial.begin()` in `logicTaskCode` — it is currently after, with baud and slave ID hardcoded. |
| **LED §3.4/§3.5** | Boot snake (R→G→B, 150 ms, red-blink after 10 s) and reset ramp (accelerating white → solid white on acceptance, no flash on abort). Needs `UiCountdownState` to carry `totalMs` — whole seconds cannot drive a 60 ms period. |
| **D2** | Generate the manifest from firmware (spike Approach A: `kActionDescriptors` + `static_assert` + `manifest_gen`). Promoted to a prerequisite by **D5**. |
| **D4** | `POST` the dataset in the export body + a checked-in baseline, so "Export to Firmware" can never ship stale JSON. |
| **D5** | Catalogue-driven palette: layout elements placed freely, bound elements chosen from the catalogue only — never a hand-typed ID. |
| **F1** | `git rm -r --cached web/mockup/node_modules` (10,615 files, and the tracked copy is stale — no `vitest`, so a fresh clone cannot run the tests). |
| **F2** | Delete `carea/` — stray bare git repo, verified empty of project content. |
| **F4/F5** | Rewrite `active_work_tracker.md` (all 37 items marked done, every link broken). README references five paths that do not exist. |
| **F6** | CI: `npm ci`, `test:unit`, `test:exporter`, `build`, and the containerised `pio run`. |
| **F7** | Remove `.antigravity/ .antigravitycli/ .beads/ .codex/ .kiro/ .shirika/` and `AGENTS.md`; keep Claude Code. `.beads/issues.jsonl` verified **empty**, so no issue data is lost. |
| **G1** | **Needs you at the hardware.** Flash and read register 0–1 for the real `pollingRate_kHz`. `readPlcInput()` is 8 I²C reads where the old bulk call was 1, so the rate dropped ~8×. Arithmetic says ~2–4 kHz against the ~660 Hz a 50 L/min YF-B10 needs, so there should be 3–6× headroom — but measure it. |

---

## 5. Unfinished verification

An adversarial workflow over the generated dataset was **stopped mid-run** at pause time
(4 lenses: ring closure, geometry, requirements conformance, binding correctness). Resume,
reusing cached results for agents that already finished:

```
Workflow({
  scriptPath: "/home/matteo/.claude/projects/-home-matteo-Documents-projects-personal-water-flow-meter/0e8c4cef-8c97-4929-9d99-eee863e13f91/workflows/scripts/verify-generated-nav-dataset-wf_78bd8161-e4e.js",
  resumeFromRunId: "wf_78bd8161-e4e"
})
```

That path is in session scratch, so it may not survive. The script is short enough to
re-author from the four lens descriptions inside it. **The generated dataset has not been
independently audited** — the export gates pass, but ring closure and element overlap were
never machine-checked.

---

## 6. Working tree

`git status` should show only **your own** uncommitted work, which I left untouched
throughout:

- `web/mockup/src/App.tsx` — one line, `globalValues={firmwareLoopValues}`
- `web/mockup/src/components/DisplayViewport.tsx` — disabled-sensor `--` rendering

⚠️ That `DisplayViewport` work reads `element.dataSourceId`, which **no longer exists** —
it is now `element.binding` (§2 item 4). It needs that one rename to work. It never worked
before either, because nothing in the dataset ever set `dataSourceId`.

Untracked: `carea/` (delete, F2).

**Nothing has been pushed.** When you want it on the remote:
`git push -u origin fix/pipeline-verification-gates`

---

## 7. Key documents

| Document | Why |
| --- | --- |
| `docs/active_work/open_decisions.md` | 31 decisions with rationale. Start here. |
| `docs/Requirements/feature addition/Display_UI_Requirements.md` (0.2) | The authoritative UI spec — navigation tree, gesture contract, editors. |
| `docs/new feature proposal/NF-20260730-01-menu-navigation-model.md` (0.2) | Why the model is shaped this way, and what was rejected. |
| `docs/Requirements/Project_document.md` | §4.1.1 link registers, §5.3 LED items 5–6. |
| `docs/Requirements/feature addition/RGB_LED_Behavior.md` (0.2) | §3.4 boot snake, §3.5 reset ramp. |
| `web/mockup/tools/skeleton/generate.mjs` | The one-shot scaffold generator, re-runnable with `--write`. |
