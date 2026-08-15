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

## Defects found — three fixed, six open

None is a decision — each is known, each has a diagnosis, and they are here because this is the file
that gets read before picking up work.

### ~~A channel with no calibration could never be given one~~ ✅ FIXED 2026-08-15

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

### ~~The pulses-per-litre form was flagged for a check it could not pass~~ ✅ FIXED 2026-08-15

`meetsNyquistLimit` computed the formula ceiling only and returned false on `f_multiplier == 0` — the
correct, normal state of a channel calibrated by pulses per litre. So every valid pulses-per-litre
channel failed the sampling check permanently: `evaluateSensorDiagnostics` ORs in `valid && !meets`, so
its bit in `REG_UNDERSAMPLING_FLAGS` stayed lit forever, the panel wore the warning banner, and the
figures could only be installed by writing them twice through the §5.5 override handshake.

**Fixed** with a per-form ceiling taken from the engine's own inversions: `f_multiplier*q_max + adjust`
for the formula, `K*q_max/60` for pulses per litre. Both directions pinned — in-budget accepted on the
first write with register 30 at 0, out-of-budget still refused and parked.

**Note for G1:** this changes *which* frequency the budget is computed from, not whether the budget is
right. The 3.3 kHz polling rate it is compared against is still unmeasured on hardware.

### The warning banner is drawn where §2c decided it must not be 🔴

**§2c is decided and its firmware half was never done.** `Display_Per_Screen_Spec.md` §2c rules
`bannerY = 116` so the banner covers the footer row, and §8 still lists it as an outstanding one-line
change. `ui_renderer.cpp:497` says `bannerY = 34`.

Everything downstream already assumes 116. `tools/audit/screen-spec.ts` validates every screen against
`kBannerTop = 116` / `kBannerBottom = 134`; the 61 spec files under `screens/` are clean against that
band; each declares its footer hint as the one element the banner may replace (`bannerReplaces`); and
every `level-position` scrollbar was shortened to 100 px so it stops clear of y=116. **So the collision
gate protecting the banner's band is checking a band the firmware does not use** — and the real band at
y 34…52 is unchecked on every one of the dataset's 80 screens. §2c's own argument names the worst case:
on P1 the banner's second row sits at y=44, hiding sensors 2 and 6 *while naming a sensor*.

**§2c calls this one line. It is two.** `drawWarningBanner` is called BEFORE `drawScreen`
(`ui_renderer.cpp:181`), and text elements paint with an opaque background (`bg = backgroundColor_` in
`drawTextElement`), so the screen's own rows stamp background-coloured cells into the banner. At y=34
that already disfigures the band; at y=116 the footer hint at y=124 would punch its text straight through
it, and "the banner replaces the footer" would be false. Moving the banner therefore needs the draw order
moved too, or the footer suppressed while a warning is live.

**Not fixed here, deliberately.** It changes what every screen does while a warning is live, and the
countdown-overlay interaction has not been checked for elements at y≥116. It wants its own round with the
audit re-run.

### The commissioning gap reaches no summary line on the panel 🟡

The uncalibrated-channel work is complete in the firmware and reaches the operator almost nowhere.
`warningSummary` is composed with uncalibrated outranking under-sampling, and both its consumers are
weaker than they look:

- **`drawWarningBanner` returns early on `!context.hasWarnings`**, which means sampling faults only. So
  the two uncalibrated-only strings ("3 channels not calibrated") are composed every pass and never
  painted. Deliberate — `UiRenderContext`'s comment argues that widening `hasWarnings` would make a
  factory-fresh device wear the banner permanently over the very screens that clear it. Sound, and it
  leaves the string with no route out.
- **`telemetry.status` and `legend.warning` are bound by NO dataset element.** Confirmed against
  `screens.json` and `GeneratedUi.cpp`: neither id appears. `telemetry.status`'s five states are
  resolvable and unpainted, and the `legend.warning` colour rule at `ui_renderer.cpp:459` is dead code.
  The exporter says so on every run — the `diagnostics-banner` gate's standing WARNING is exactly this.

**Net effect today:** only the combined case (`uncalibrated > 0 && warnings > 0`) changes anything an
operator sees, because the banner is already up for the sampling half. The gap does reach them by other
routes — the green LED refuses to light (`SensorStateEngine`) and each channel row says `SET?` — but no
summary line says how many or why.

**Decide where the commissioning line lives.** Either bind a row (P0 carried `telemetry.status` before
the §3 redesign dropped it for the walking dots; the owner's ruling then was "the banner is enough for
now", made about *undersampling* and before an uncalibrated count existed), or widen the banner's gate
and accept losing the footer hint until the device is commissioned, or state that the LED and `SET?` are
the intended route and delete the unreachable strings. All three are cheap; leaving it is what ships a
tested string nothing draws.

### ~~The provisioning portal drops its own form submission~~ ✅ FIXED 2026-08-14

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

### `OFF_CMD_RESET_CONFIG` leaves the Nyquist override latched 🟡

Offset 19 wipes `SensorData` and the configuration but does not clear `overridePending_`,
`overrideActive_` or `pendingOverrides_`. A latched override makes `prepareConfigUpdate` accept the FIRST
candidate offered without a sampling check, so the exemption granted to a decommissioned meter is
inherited by whatever replaces it — and `evaluateSensorDiagnostics` ORs both flags in, so the
undersampling bit stays lit on a channel with no configuration to undersample.

Offset 25 (`OFF_CMD_RESET_CALIBRATION`) clears all three, which is what makes this visible as an
inconsistency rather than a design. **Pinned rather than fixed**, deliberately: offset 19's behaviour is
asserted as-is in `modbus_reset_calibration_test.cpp` so narrowing a shipped command later is a
deliberate act with a failing test attached. Any Modbus master already issuing 19 expects what it does.

**Decide:** clear the override state on 19 too (a three-line change, and the test that pins the current
behaviour then has to be inverted), or record that 19 is frozen and 25 is the command to use.

### The simulator's two newest device facts have no test that could fail 🟡

Both are recorded by the commits that added them, and both are the same shape — a green suite over
something only a human can reach:

- **`SimulatedSensor.undersampling` is a mockup-only control** with no automated coverage. All five
  states of `statusSummaryText` and `warningSummaryText` are pinned in `sensorConfig.test.ts`, but they
  construct `{ undersampling: true }` by hand — the checkbox that lets the running app reach that state
  is asserted by nothing, because there is no component-test harness (no `@testing-library`). Removing
  the control would make four firmware strings unreachable in the mockup again with the suite still
  green.
- **`ready` is a stored boolean that nothing recomputes** when the configuration changes, so writing
  `qMaxLpm = 0` through the Values panel leaves the row saying `OK` while the device would say `SET?`.
  Fixing it means deriving `ready` in `normalizeSensor`, which changes what every producer in
  `sensorConfig.ts` means by the field.

**Decide:** add a component-test dependency and pin the controls, or accept that the mockup's inputs are
verified by driving the app and record that as the standing rule.

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
