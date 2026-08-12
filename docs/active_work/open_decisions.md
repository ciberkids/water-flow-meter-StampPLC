# Open Decisions

What is genuinely undecided or unbuilt, and nothing else.

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

## I2 — standing rule, not a decision

**The value catalogue is append-only.** Renumbering or repurposing an existing entry breaks every
authored pack that references it, and breaks it silently — the id still resolves, to something else.

This is not open and never closes; it is recorded here because it constrains every future change and
a rule filed under "archive" is a rule nobody reads. Same class as the wire-encoding rules on
`WifiState` and `kMqttFlags` bit 2.

---

## Three defects found and not yet fixed

None is a decision — each is known, each has a diagnosis, and they are here because this is the file
that gets read before picking up work.

### The provisioning portal drops its own form submission 🔴

`PortalForm` renders `action="/save"`, and its header states the contract: *"the adapter must route
this exact path"*. `portal_server_arduino.h` registers `POST` on `/` only. ESP32's `Uri::canHandle` is
`_uri == requestUri`, so the submission reaches the catch-all, gets a `302` to `/`, and the browser
re-issues it as a `GET` — the configuration is discarded with no error shown.

**Impact.** The web portal is the only provisioning route for a device with no Modbus master. Modbus
provisioning is unaffected and works.

**Why no test caught it.** `PortalForm` has 226 host checks; they exercise the form against strings.
The routing lives in the Arduino adapter, which no host test can construct — the same blind spot its
own `collectHeaders` comment warns about.

**Fix.** Route `PortalForm::kFormAction` for `HTTP_POST` in the adapter. One line, and it wants a way
to be tested that does not need a browser.

### The Select Menu is reachable only by a hidden gesture 🟡

`Loadable_UI_Menu_Packs.md` §3.4 requires the firmware to append a Select Menu page to the end of the
root level, "always reachable by paging with UP/DOWN". §3.4.1 then argues the case explicitly: a hidden
gesture is *"precisely the anti-pattern we retired with the blind UP+DOWN factory-reset combo — nothing
on screen says it exists, nothing confirms you are partway through it, and it cannot be documented on
the device itself."*

**What shipped is the hidden gesture and not the page.** The root ring has nine entries — P0..P6, WiFi,
MQTT — and none is the selector; nothing in `UiNavigator` or `UiScreenRouter` appends one. The
UP+DOWN+ENTER 3 s recovery gesture works and is tested, so the selector is reachable; it is simply
undiscoverable, which is the specific outcome §3.4.1 was written to prevent.

Found 2026-08-12 by someone reviewing the documentation and being unable to locate the UI-selection
menu — which is the failure mode in miniature: if a reader with the source open cannot find it, an
operator with only the panel certainly cannot.

**Decide.** Either append the root-level entry (a dataset row plus the navigator honouring a
firmware-owned entry the packs cannot remove — note it must survive a pack that defines its own root
level, which is the whole reason §3.4 puts it in the firmware), or amend §3.4 and §3.4.1 to record that
the gesture is the only route and accept the discoverability cost. The gesture is now documented in
`../Requirements/Gesture_Reference.md` §3.6 and drawn in `../diagrams/ui_navigation_tree.mermaid`, so
at least it is findable on paper.

**Blocks.** Nothing technically. It blocks an operator finding the feature.

### §3.1's held UP/DOWN navigation step is emitted and answered by nothing 🟡

`button_input.cpp` emits repeat events from 1.5 s, every 250 ms, exactly as specified. `mapGesture`
maps them to `FlowGesture::Hold`, `matchFlow` demands an exact gesture match, and the dataset
declares **zero** hold flows — so every repeat is dropped and a held UP/DOWN navigates nowhere.

This produced three bug reports before anyone found the cause, because the web simulator had invented
the missing half in two different ways and the button legend documented the invention as fact. The
simulator now matches the firmware, and `heldRepeatScopeTests` pins the real behaviour.

**Decide one way or the other:** declare `hold` flows on the info ring so §3.1 is implemented, or
amend §3.1 to drop the repeat. Leaving it half-present is what cost the three reports.

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
