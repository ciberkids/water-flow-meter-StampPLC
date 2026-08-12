# Water Flow Meter — StampPLC

A water-flow meter on an **M5Stack StampPLC** (ESP32-S3). It measures up to eight pulse-output
flow sensors, publishes readings over **Modbus RTU**, and has a 240×135 landscape display driven
by **three buttons**.

The distinctive part, and the thing to understand before changing anything: **the on-device UI
is designed in a web app, exported to JSON, and translated into `constexpr` C++ tables that the
firmware renders.** Nothing about the screens is hand-written in the firmware.

---

## Verify the checkout in four commands

Run these before trusting any document in this repository, including this one.

```bash
# 1. Host tests — no PlatformIO, no container, ~2 seconds
Water-Flow-Meter-PlatformIO/test/host/run.sh

# 2. Web suites
cd web/mockup && npm ci && npx tsc --noEmit && npm run test:unit && npm run test:exporter

# 3. The export gates
npm run export:firmware

# 4. Firmware compiles (build the image once, ~5 min)
cd ../../Water-Flow-Meter-PlatformIO && podman build -t stampplc-fw .
podman run --rm -v "$PWD":/workspace:Z -w /workspace stampplc-fw pio run -e m5stack-stamplc
```

Expected, as of 2026-08-02: **199 host checks** across six suites, 21 unit, 33 exporter, export
`ok` with **9/9 gates and no warnings**, firmware SUCCESS at RAM 7.8 % / Flash 18.1 %.

The `:Z` on the volume mount is required on SELinux hosts (Fedora, RHEL). Without it the
container sees an empty workspace.

**Start with command 1.** It runs the device harness — the real navigator, controller,
interaction handler and action registry against the real 48-screen table, with only three
Arduino headers stubbed. It is the fastest way to find out whether the tree is sound, and it
needs no toolchain at all.

---

## The pipeline

```
web/mockup (React)  ──►  src/data/screens.json  ──►  tools/exporter  ──►  GeneratedUi.{h,cpp}
      designer               the dataset              the translator        constexpr tables
                                                            │
                          tools/manifest_gen  ──►  actionManifest.json
                        (reads the FIRMWARE catalogues)   what the firmware can do
```

Three vocabularies have to agree across a browser, a Node exporter and a firmware build:
**screen ids**, **action ids** and **binding ids**. Nearly every serious bug in this project has
been a disagreement between them — a screen the router looked for and the dataset didn't have, an
action the designer could wire to nothing, a binding that rendered blank.

That is what the gates exist for. `npm run export:firmware` runs nine of them and refuses to
write assets that would not work:

| Gate | Stops |
| --- | --- |
| `manifest-screen-coverage` | A screen the firmware resolves by name being absent or renamed |
| `manifest-action-coverage` | A flow wired to an action no handler implements |
| `manifest-value-coverage` | An element bound to a value that does not exist |
| `firmware-binding-coverage` | A bound value `UiBindingResolver` has no case for — it renders blank |
| `firmware-manifest-resolvable` | The same, for every value the manifest *advertises*, not just those in use |
| `renderable-element-kinds` | An element kind the firmware cannot draw |
| `led-legend`, `countdown-overlay`, `diagnostics-banner` | Required affordances going missing |
| `platformio-compile` | Generated assets that do not compile |

The manifest is **generated from the firmware's own catalogues**, not maintained by hand, and
`test/host/run.sh` fails if the committed copy is stale. Action ids are cross-checked against
their handler table by `static_assert`, so advertising an action with no handler is a build error.

---

## Layout

| Path | What |
| --- | --- |
| `Water-Flow-Meter-PlatformIO/` | Firmware. `src/{input,led,modbus,sensors,ui}`, `tools/manifest_gen/`, `test/host/` |
| `web/mockup/` | Design tool, exporter (`tools/exporter/`), skeleton generator (`tools/skeleton/`) |
| `docs/` | Requirements, decisions, hardware references — see the trust table below |
| `graphics/` | SVG assets and captured previews |
| `.github/workflows/ci.yml` | Three jobs on every push: web + exporter, host tests, firmware compile |

---

## Working on it

### Firmware

```bash
cd Water-Flow-Meter-PlatformIO
test/host/run.sh                     # host tests + manifest freshness
podman run --rm -v "$PWD":/workspace:Z -w /workspace stampplc-fw pio run -e m5stack-stamplc
```

A local `pio run` works if you have PlatformIO installed; the container is what CI mirrors.
There is **no** `pio test` target — the unit tests are the host suite in `test/host/`, built
straight with `g++` and `-Werror`. That `-Werror` is deliberate: the manifest generator relies on
`-Wswitch` to make "added a setting kind without teaching the generator" a build failure rather
than a warning nobody reads.

Adding UI behaviour? Put the test in `test/host/interaction_test.cpp`. It drives the real
firmware with a fake button source, so gestures, navigation, editors and countdowns are all
testable without hardware.

### Design tool and exporter

```bash
cd web/mockup
npm ci                               # not npm install — the lockfile is authoritative
npm run dev
```

Then edit screens in the browser and press Export. The dataset you see is what gets exported —
it is POSTed with the request rather than re-read from disk.

Commit the dataset and the regenerated assets **together**. CI fails if they disagree.

`tools/skeleton/generate.mjs` regenerates the default menu from the catalogue and refuses to emit
one that leaves any setting unreachable. Run it after adding a firmware setting.

### Requirements

Node **22** (CI pins it; `engines` allows ≥20). PlatformIO or Podman/Docker for firmware.

---

## Which documents to trust

Documentation in this repository has been wrong in both directions — requirements describing
machinery that does not exist, and status files reporting finished work as pending. An audit on
2026-08-01 found 50 false claims across 17 documents. Current state:

| Trust | Treat with care |
| --- | --- |
| `docs/Requirements/Gesture_Reference.md` — every ✅ names the test that earns it | `web/mockup/README.md` — describes a portrait display and an empty dataset |
| `docs/active_work/open_decisions.md` — the de-facto decision record | `docs/Requirements/feature addition/UI_Firmware_Interface.md` — lists 4 actions where there are 15 |
| `docs/Requirements/Project_document.md` §4.1–§4.2 — the register map, verified against the headers | `docs/Requirements/Implementation_Alignment_Report.md` — audits a file layout that no longer exists |
| `MEMORY.md` — session handoff, rewritten 2026-08-01 | Any undated status claim |

`open_decisions.md` was rewritten on 2026-08-12 and now lists only what is genuinely open — two
defects and two unbuilt things. The caveat that used to sit here (✅ meaning *agreed* rather than
*landed*, with the heading emoji disagreeing with the `Decision:` line) is gone with the old file,
which is preserved at `docs/archive/open_decisions-closed-2026-08-12.md`.

`Loadable_UI_Menu_Packs.md` **is built**, contrary to what this paragraph used to say: the `.uipack`
format, its reader, the SD storage adapter, the loader, the firmware-drawn selector, the boot-time
selection ladder and the SPI arbitration all exist under `src/ui/pack/` and `src/bus/`. What is *not*
built is the versioning that keeps a third-party pack valid as the catalogue grows — see **N-b** in
`docs/active_work/open_decisions.md`.

---

## Nothing has run on hardware

Not once. The firmware compiles on two independent toolchains and passes 199 host checks, but the
RS485 pin assignment, the LED behaviour and every gesture and timing are verified only against
the datasheet, the specifications and those tests.

The first bring-up should measure `pollingRate_kHz` (published on holding register 0 and shown on
the diagnostics screen) — it is open decision **G1**, and it is also a prerequisite for the WiFi
work, whose acceptance criterion budgets against a radio-off baseline that does not exist yet.
