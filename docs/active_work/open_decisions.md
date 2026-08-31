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

**This file holds eight items** — six open plus the two standing rules. It was 287 lines and six items
when it was rewritten on 2026-08-18, and 1,370 the day before that, because twenty-nine closed defects
had accumulated in it with their full reasoning. **Its own live line count is deliberately no longer
quoted here:** the figure went stale twice while this paragraph was being edited on 2026-08-30, which is
the same argument this register makes about every other number that has no command behind it. The item
count stays, because I3 already requires the index to track it. Those are archived, not deleted — see the pointer
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

**Six open lines: two features, two defects and two measurements.** `N-f` and `DF25` opened on
2026-08-30, both out of one question the owner asked while T1's storage was being decided — *is the
right information surviving a power cycle, and are we burning through the write cycles?* That is the
usual way: `DF23` came from checking whether `G1`'s procedure could be followed, and `DF22` from
building `N-c`. `J9`, `I2a`, `N-b` and N-d2's correction half all closed on 2026-08-21; `N-c` and
`DF22` closed on 2026-08-20, and `DF22` was the only 🔴 the board has had since the register was
rewritten.

**What remains is four software items and three measurements.** `N-e` is under way — T0 and T1 have
landed and T2 is next — and `N-f`, `DF23` and `DF25` each carry a decision in the entry. The three that
need hardware are `G1` (the polling rate), `N-d2`'s remaining half (whether the RTC survives power
loss), and `N-c`'s retain check, which lives in that entry's ⏸️ block rather than on a line of its own
because the rest of `N-c` is built and tested. Cite an ID when you ask about one — this file is the one
place that says what an ID means. Rule **I3** governs them: append-only, never reused, so a gap means an
item closed, not an item lost.

**`N-c` stays in this file rather than moving to the archive**, because one thing in it is verified by
reading only: `MQTT_EVENT_DATA`'s retain flag, which R4.4.2c depends on entirely and which no host test
can reach. Its ⏸️ block carries the one-command bench check and says what a wrong answer costs. That
makes **three** things waiting on the board, not two.

The **Shape** column is the one that answers *can I just say go ahead?*

| ID | | Shape | What it is |
| --- | --- | --- | --- |
| **N-e** | 🟡 | feature, in progress | Sensor cascade topology — a parent per channel, a total that is the sum of roots, and verification against a commissioned baseline. Six decisions taken, four questions open; T0, T1 and T2 landed, **T3/T4/T6 next**, and R2.5's MQTT sentence needs amending |
| **N-f** | 🟡 | feature, specified here | Where the readings live — internal flash or an SD card, human-readable files, persisted session values and rotating telemetry. Five decisions taken, six questions open, not started; question 1 changes a documented register contract and should be answered first |
| **DF25** | 🟡 | defect, found 2026-08-30 | A write to non-volatile storage that fails is reported by nothing. **Detection and publication landed 2026-08-30** (`DF25a`: register 34, MQTT `storageFault`, 49 host checks) — the panel's warning triangle rides with `N-e`'s T5, and three of its four questions were built on the recommendation rather than decided |
| **DF23** | 🟡 | defect, found 2026-08-21 | `baselineKhz` is published as `0.000` — R2.1.2's radio-off baseline is recorded by nothing, so R2.1.1's 5 % test and half of G1's procedure have no reference |
| **G1** | ⏸️ | measurement | The 3.3 kHz polling rate has never been measured on a board; the procedure is written down and waiting |
| **N-d2** | ⏸️ | measurement | Whether the RTC survives power loss is unknown — the correction half (a gate protecting the VLF probe's position) landed 2026-08-21 |

**DF1–DF22 and J1–J8** are fixed and keep their IDs — `DF1`–`DF21` and `J1`–`J8` moved to
[`../archive/open_decisions-closed-2026-08-18.md`](../archive/open_decisions-closed-2026-08-18.md), verbatim,
because I3 makes them append-only and a retired id must still resolve. **I2** and **I3** are standing rules
that never close.

**What this list is NOT.** Nothing here is blocking a build, a test or an export. Measured 2026-08-30
after `DF25a` and its review: host **2,168 checks across 29 suites**, 0 failures, the register-reference gate green
(and shown to fail on a removed code description), and a firmware that compiles in the container at RAM
24.7% / Flash 39.0% — 81,040 and 1,303,457 bytes against T1's 81,008 and 1,301,925, so DF25a with both
review rounds costs **32 bytes of RAM and 1,532 of flash**. NOT re-run on 2026-08-30 and therefore quoted as the 2026-08-26
figures: 220 unit, 51 exporter, 51 visual, 0 audit findings. (Both compile figures come from a CLEAN dependency
cache; the earlier 38.2% came from a stale container, see `platformio.ini`.) Of the six open lines one
feature is under way, one has not started, two defects have decisions in them and two need hardware that
has never existed for this project. That is a different condition from "twelve things are broken", which
is what this register looked like before the 2026-08-18 rewrite.

---

## G1 ⏸️ Polling rate on real hardware is still unmeasured

**Decided, not verified.** M5StamPLC 1.2.0 removed the bulk `IO.getDigitalInput()`, so the sampler
reads the expander per channel. The decision was "measure first, pursue a bulk expander read only if
the measurement demands it" — and the measurement needs a board.

Everything is in place to take it: the achieved rate is published in `REG_POLLING_RATE_KHZ`
(register 0) and on the MQTT diagnostics topic beside the baseline it should be compared against, and
`REG_UNDERSAMPLING_FLAGS` (register 30) names any channel outrunning the sampler.

**What to do when the board arrives.** Flash, read register 0 — and compare it against **3.3 kHz as a
design assumption**, not against `baselineKhz`, which is published as `0.000` because nothing records it
(`DF23`). If the real rate is materially below 3.3 kHz, the bulk-read work becomes real and the
per-channel `q_max` limits need re-checking against what the sampler can actually count.

That substitution is a downgrade and worth naming: §2.1 intended this measurement to be *self-checking*,
comparing the live rate against a baseline the device took of itself, which is what R2.1.1's 5 % rule
needs. Against a written-down assumption it is still a useful measurement — it answers "can this sampler
count what the datasheet limits assume" — but it cannot answer "did enabling the radio cost us
anything", which was the other half. `DF23` is what closes that gap.

**Note, 2026-08-17.** `meetsNyquistLimit` now refuses a ceiling of zero or less instead of accepting it,
and `configIsValid` demands a positive multiplier. Neither touches this: both are about configurations
that have no sensible ceiling at all, not about what the ceiling is compared against. The 3.3 kHz is
still assumed, and it is now also the simulator's default dial (`kDefaultPollingRateKhz`), so the
mockup's "inside budget" statements inherit the same assumption.

**Blocks.** Trusting the sensor configuration limits on hardware. Nothing in software.

---

## ~~N-b~~ ✅ FIXED 2026-08-21 — a catalogue addition is distinguishable from a pack's omission

`Loadable_UI_Menu_Packs.md` §3.0.1 requires every menu pack to expose an editor for every
`category: "setting"` value. The rule is right; what it could not do was tell two situations apart. A
pack missing an editor for a setting that existed when it was authored is a BUG in the pack. A pack
missing an editor for a setting added to the firmware since is not — it is an old pack, and failing it
would mean every firmware release invalidated every card in the field. From the pack alone the two look
identical.

**What makes them distinguishable.** Three things that did not exist before today:

1. **The manifest carries `catalogueAbi`**, emitted from `ui::kUiCatalogueAbi` — so the number a pack is
   stamped with and the number `MenuPack::validate` compares against are one constant. It is a
   different number from the `version` beside it and the generator says so: `version` is this file's
   shape, `catalogueAbi` is the vocabulary it lists.
2. **Every id records the ABI it appeared at**, in `I2a`'s ledger. The manifest describes the catalogue
   as it is NOW and cannot answer a question about history.
3. **Additions bump the ABI** (owner's decision, reversing what `kUiCatalogueAbi`'s own comment said).
   Without that, every id sits at the same ABI and the distinction has nothing to work with.

**`tools/catalogue/coverage.mjs` is the one home for the policy.** The required set — settings, minus
text and minus network, both exemptions decided by a STATIC property so the rule still proves that
every setting an operator can change at the panel has an editor there — moved out of
`assertCoversEverySetting` because N-b needs the same policy at a second call site. Two copies of an
exemption list is the failure this codebase keeps finding. Verified by regenerating: the dataset comes
out byte-for-byte identical, so the policy moved without changing behaviour.

**The pack emitter no longer invents its ABI.** `pack_emit.test.ts` passed a literal `1`, so the
stamped number and the firmware's constant were unrelated facts that happened to agree; the day someone
bumped, the fixture would have kept claiming the old vocabulary and the C++ round-trip would have kept
passing. It reads the manifest now, and asserts the value reaches header offset 8 where `validate`
looks. Proved by bumping the constant to 7 and watching the fixture's stamp follow, then back.

**WHAT A WARNING DOES NOT MEAN, and this is the honest limit.** It does not mean the firmware covers the
gap. §3.3.11a specifies a load-time patcher that appends built-in editors for missing settings, and
**that still does not exist.** So a warning today means precisely: *this pack cannot reach that setting,
and the operator needs the portal, RS485 or a newer pack.* It is information, not absolution. The module
says so at the top, because "warns rather than fails" reads like the problem has been handled.

**The warn branch cannot fire against the live catalogue** — every id is at `sinceAbi: 1` and the
firmware is at ABI 1 — so it is exercised with fixtures in `coverage.test.mjs` rather than left
unexecuted. Seven cases: the exemptions (including that network is exempt even when it is a NUMBER), a
gap at the current ABI failing, the same gap on an older pack warning, the `>` versus `>=` boundary, a
covered setting appearing in neither list, an id absent from the ledger failing STRICTER rather than
looser, and the live catalogue satisfying the rule so the fixtures cannot drift away from the policy the
project actually applies.

**Still not built, and still N-b's neighbours rather than N-b:** §5.8's export gate and §3.3.11a's
load-time patcher. A third-party pack on an SD card is now *diagnosable* — the exporter can say which
settings it predates — but nothing yet repairs it on the device.

---

## N-e 🟡 Sensor cascade topology — specified 2026-08-26, four questions open, T1 and T2 landed 2026-08-30

The owner's feature: channels may be wired in CASCADE (a main meter with others downstream) rather
than only in parallel. That makes the present total wrong — a downstream channel's water was already
measured by its parent, so summing double-counts — and it makes VERIFICATION possible, because a
parent's reading should equal the sum of its children's and the difference is evidence.

**Specified in full:** [`../Requirements/feature addition/Sensor_Cascade_Topology.md`](../Requirements/feature%20addition/Sensor_Cascade_Topology.md).
Nine sections, numbered requirements, six open questions with recommendations, and eight slices. Cite
requirement ids from there (`R2.2`, `R3.5`, …) rather than quoting this summary.

**Four decisions taken 2026-08-26**, each a real fork:

| # | Decision |
| --- | --- |
| 1 | The UPSTREAM meter is authoritative — the total is the sum of roots, and skew is booked as unmetered loss in the branch |
| 2 | Red fires on departure from a COMMISSIONED BASELINE, not on an absolute skew ratio |
| 3 | An orphan re-parents to its nearest IN-SERVICE ancestor, never to root |
| 4 | MQTT stays READ-ONLY for topology — §4.4.1's security position is not reopened |

**Three of those four reverse how the feature was first described, and the reasoning is the valuable
part** (§5 of the spec):

- **The Home Assistant fear was misplaced.** Netting cannot corrupt HA history: the
  `total_increasing` entity is per-CHANNEL, fed from raw `cumulativeLiters`, and no HA entity
  subscribes to the aggregate topic at all. There is also no aggregate Modbus register. The hazard
  only exists if somebody later adds an aggregate `total_increasing` entity — which is a reason not
  to.
- **An absolute percentage threshold measures the plumbing, not a leak.** Meter tolerance suggests
  5-10 %; the dominant term in a real install is the UNMETERED FRACTION — one outside tap can exceed
  10 % of consumption, and accumulated skew converges on that ratio rather than decaying. So 10 %
  would sit red from commissioning day, correctly and uselessly. Worse, the same arithmetic HIDES a
  bounded overnight leak: fixed litres in the numerator, growing consumption in the denominator, so
  the ratio decays under any threshold by morning. Hence the baseline, and hence publishing the litre
  difference and the peak rather than only a ratio.
- **Three consumers want the GROSS total**, so it must not be repurposed: the red LED's litre step,
  the blue LED's liveness, and P0's flow dots. The last two are the sharp ones — under a netted
  aggregate, a root reading zero while a child meters real flow gives zero, so the panel and the LED
  both report "no water" during exactly the condition being detected.

**And one defect-shaped finding, verified:** an in-service root with no valid calibration contributes
a FROZEN volume and zero flow — `sessionLiters` and `cumulativeLiters` are assigned only inside
`if (configIsValid(config))` and the else-arm zeroes only the flow
(`sensor_state_engine.cpp:60-71`), while line 72 adds the frozen value regardless. As a root it
silently zeroes its whole branch. A meter swap triggers it. That is Q1 of the six.

**Two more decisions taken 2026-08-26**, both narrowing v1 deliberately:

- **An uncalibrated root publishes its branch as UNKNOWN**, against my recommendation to promote its
  children. The owner's reasoning is the stronger one: a device should refuse to state a total it
  cannot support rather than state a smaller one, because a wrong number is harder to notice than a
  missing one. It forces a new requirement — the total must be able to SAY "unknown" (NaN plus a
  bitmap of unknown branches), since zero is a legal reading and blanking would be indistinguishable
  from no flow.
- **The HTTP portal is out of v1.** Reaching the parent setting there means injecting a settings store,
  which turns on every per-sensor setting including calibration — a security decision that deserves to
  be chosen rather than inherited from a topology feature.

**So two of the four surfaces originally asked for are deliberately deferred.** The feature was
described as needing the setting on the panel, Modbus, HTTP and MQTT; MQTT is read-only and HTTP is
out. Both stay open as their own decisions. Recording it here because a narrowing the owner chose
should not later read as one nobody noticed.

**~~Blocked on `DF24`~~ — T0 is DONE, 2026-08-26.** The cascade's aggregate was about to be built on
`<base>/total/state`, whose two dead fields proved the snapshot assembly in `firmware.cpp` was
reachable by no test. That assembly now lives in `net/mqtt_snapshot.h` with 23 host checks, so the
cascade's own aggregation has somewhere testable to land.

**T1 is DONE, 2026-08-30 — the topology module, 37 host checks.** `sensors/sensor_topology.h` holds the
forest and `sensors/sensor_topology_nvs.h` its trip to flash. Four things in it are decisions rather
than transcriptions:

- **`apply()` is the only mutator**, taking all eight parents at once. §3.6 requires a write that would
  break the forest to change nothing, and a per-channel setter would let a master write four parents,
  fail the fifth, and leave the device metering a shape nobody asked for — which looks exactly like a
  shape somebody did.
- **The in-service set is passed as a BITMAP**, not a bool array, because that is the form the device
  already keeps it in: `conn_map` in flash and `connectedSensorsBitmap` in `firmware.cpp`. A second
  representation of one fact is how two of this project's defects started.
- **R1.2's depth limit is not a check.** An acyclic forest of eight nodes cannot exceed seven edges, so
  a depth rule could only reject a shape `validateForest` has already accepted. It is asserted from the
  legal side instead — a legal eight-channel chain measures exactly 7.
- **A stored set that is not a forest becomes ALL ROOTS and says so** (`SensorTopologyLoad::stored`).
  Flash can hold anything a corrupt page wrote, and the parallel topology is the one substitute that
  cannot over-count water. Repairing a cycle by cutting an edge would guess which meter feeds which.

**The keys are `parent_0` … `parent_7`, spelled out.** The platform caps a key at 15 characters and
`parent_7` is eight, so a new key has no excuse to be an abbreviation. The `cfg_*` calibration keys stay
frozen for the reason `sensor_config_nvs.h` gives — renaming one silently discards a calibrated channel
on upgrade — and the new file says so, because "tidy the old keys to match" is the next reader's
plausible-looking mistake.

**Separate from the calibration serializer, deliberately.** `SensorCharacteristics` is tripwired at
`sensor_config_nvs.h:56` so a sixth field cannot be added without someone deciding whether it persists;
for `parent` the answer is "persist it, but not there". Calibration describes a METER and topology
describes PLUMBING — a meter swap changes one and not the other — and
`SensorCharacteristics::operator==` is the 60-second dirty check, so folding topology in would make a
re-plumb look like a recalibration.

**And the new module carries the same tripwire, which has a known first customer.**
`static_assert(sizeof(SensorTopology) == kNumSensors, …)`: the class is one byte per channel and nothing
else, so a ninth byte means somebody added state — and T3's commissioned baseline is already scheduled
to want exactly that (§9's slice row now says so). Verified by adding a byte: the build fails naming the
line. A guard that has been shown to fire is worth more than one that has only been written.

**NOTHING CALLS IT YET, and the module says so at the top.** The boot-time load lands with T2, which is
the first consumer of an effective root, and the save with T4, which is the first thing that can change
a parent. Stated rather than left to be discovered: a setting with no author is precisely the shape of
`DF22`, `DF23` and `DF24`.

**Verified:** 37 checks, and mutation-tested on exit codes. Six mutations, five of which compiled:
committing a refused topology fails 1 assertion, inverting the in-service test fails 6, reporting a
corrupt load as ok fails 3, an off-by-one in the cycle bound fails 6, counting nodes instead of edges
fails 3, and saving only non-root parents fails 2. The sixth — deleting the in-service test outright —
would not compile under `-Werror` because the parameter went unused, which is the flag doing its job.
Host **2,095 checks across 28 suites**, 0 failures.

**T2 is DONE, 2026-08-30 — the delivered aggregate, 24 host checks.** `SensorStateEngine` now computes a
DELIVERED pair beside the gross one, in the same ascending loop, out of the same two float fields — which
is what earns R2.2's bit-identity rather than hoping for it. Five things in it are decisions:

- **The predicate is `isEffectiveRoot` and nothing else.** Adding `configIsValid` would look tidy and
  would silently narrow R2.2 to the all-calibrated case: the gross sum includes an in-service
  uncalibrated channel's FROZEN volume (§5.4), so the delivered sum must too, or the two disagree on the
  state a meter swap produces every time. What that means for a ROOT is R2.5's business, answered with
  NaN rather than by dropping an addend.
- **The in-service set is built in its own tiny pass**, because whether channel 0 is a delivery point
  depends on channel 7's service state, so the mask cannot be assembled as the main loop walks. Eight
  bool reads and no destructive ones — `pulseCount` is consumed in the main loop and must be touched
  exactly once, which is also why the per-channel conversion was not factored out and called twice.
- **A null topology means all-roots**, so every existing host test kept asserting the same numbers and a
  caller with no opinion about topology gets R2.2's reduction for free.
- **NaN goes only to the delivered pair.** R2.3 said the gross pair keeps its meaning; that is now a HARD
  constraint rather than a preference, because `LedController::update` keeps a `lastTotalLiters_`
  baseline and one NaN through it poisons the red volume LED for the rest of the boot — fixing the
  calibration would not clear it.
- **The unknown bitmap is NOT `uncalibratedFlags`.** That one names every uncalibrated channel; this one
  names uncalibrated EFFECTIVE ROOTS. On a parallel topology the two sets are identical, so reusing it
  would look correct until the first cascade was declared and no test on the default topology could tell
  them apart.

**T2 is also T1's first production caller**, so the boot-time load landed with it — and with it
`StorageFault::Topology` (code 4) stops being reserved-and-unreported: a stored set that is not a forest
raises it, says which channel, and reverts every channel to a root.

**A claim I wrote in this slice was wrong, and the mutation test is what caught it.** The comment beside
the accumulation said computing `delivered = gross - downstream` would fail the cascade oracle. It does
not — that mutation passes every assertion in the suite. Measured: over four million plausible
eight-channel tuples the two forms are **bit-identical**, and they diverge by a single ulp only when one
channel's volume sits about nine decades below another's (0.0108 L beside 8,963,428 L). So no test at
realistic magnitudes can enforce the distinction, and both comments now say so. The reason to sum the
roots is structural — it computes what R2.1 defines instead of deriving it from an accumulator R2.3
leaves to other consumers — and the numerical claim is only true for `cumulativeLiters`, which is a
`double`, where the same subtraction differs in 99.5 % of cases (§7 Q5's territory).

**And R2.2's "same ascending order" clause is UNFALSIFIABLE for this pair**, which is worth knowing
before someone strengthens the suite with a test that cannot fail. The addends are `float` (24-bit
mantissa) and the accumulator is `double` (53-bit), so summing eight of them is exact and every
permutation yields the identical double by construction — verified over 200,000 tuples × 40 shuffles, 0
mismatches. Order only re-emerges for double addends, i.e. `cumulativeLiters`.

**Verified:** 24 new checks in `sensor_state_test.cpp`; mutation-tested — adding `configIsValid` to the
predicate fails 3 assertions, using the stored parent instead of the effective one fails 1, NaN reaching
the gross pair fails 1, marking every uncalibrated channel unknown fails 2, and removing the NaN
substitution fails 2. Host **2,192 checks across 29 suites**, 0 failures; firmware SUCCESS at RAM 24.7 % /
Flash 39.0 %.

**Nothing publishes the delivered pair yet** — the Modbus block is T4, the panel T5, the MQTT keys T6 —
and that is said out loud because a value with no reader is `DF22`-`DF25`'s shape.

Slice **T4** or **T6** is next, and T3 (the skew arithmetic) is the other candidate. **One spec amendment
is owed before T6:** R2.5 says MQTT should omit the key rather than publish `null` for an unknown total,
and `jsonNumber` publishes `null` for every non-finite value as a documented rule with a stated reason
(`nan` is invalid JSON, so Home Assistant drops the whole message silently) plus a standing assertion.
No HA entity reads the total topic, so omission buys nothing a consumer can see while costing conditional
payload assembly in a file built on fixed format strings. Recommend amending R2.5's sentence rather than
the formatter — the unknown-branch bitmap is what distinguishes a deliberate unknown from an arithmetic
slip.

**A count in this entry was wrong, twice over.** The heading said six open questions and the index row
said five; §7's table has **four** (`1a`, `4`, `5`, `6`). The residue inside Q3's decided entry — the
peak needs a defined window — is a consequence to specify at implementation time, not a fifth open
question, and counting it as one is how the number drifted.

**Blocks.** Nothing today — a parallel installation is correctly served by the current firmware, and
the spec's `R2.2` requires the new arithmetic to reduce to it bit-identically. This is a feature, not
a repair.

---

## N-f 🟡 Where the readings live — internal flash or an SD card, and what a card makes affordable

Asked 2026-08-30 by the owner, out of the audit that T1's storage decision provoked: *is the right
information surviving a power cycle, and are we burning through the write cycles?*

**The audit's answer first, because it decides what this entry is.** What persists today is the
per-channel lifetime volume (`cml_0`…`cml_7`), the calibration, the in-service bitmap, the RS485 link
settings, the network settings, the flow unit, the two reset-command guard epochs, the LED step and
period, and the menu-pack attempt counter. Session volume and peak flow are deliberately NOT persisted,
and the requirements say so explicitly — `Project_document.md:204` documents register 111 as **"not
persistent"** and register 22 is named "Master Reset All Session Values" with the gloss "session
(**non-persistent**) values". So the split is correct as built.

**The wear budget is bounded, and bounded by the right thing.** Only the litres are written on a timer:
every 60 s, and only for channels whose value actually moved (`firmware.cpp:1044-1063`), with the shadow
copies seeded from the loaded values at `:996-999` so a reboot causes no redundant write. Worst case is
therefore 8 writes per minute — the comment at `:1042` already sizes the unconditional version at ~525k
writes per key per year — and the changed-value check makes wear proportional to **water actually
metered rather than to uptime**, which for a domestic install is a large factor. An absolute lifetime
figure needs the flash part number off the module and is not claimed here.

**So this is a feature, not a repair, and it must not be justified by a wear risk nobody has measured.**
What a card actually buys is three things: session values that survive a power cycle, files a human can
read, and room for telemetry.

**Decided 2026-08-30 by the owner:**

| # | Decision |
| --- | --- |
| 1 | The device detects an SD card; the storage menu entry appears only when one is present |
| 2 | Internal or external storage is an operator setting, not a build-time choice |
| 3 | External files are ASCII and human-readable, with names that say what they hold |
| 4 | On external storage the SESSION values are persisted too, since a card tolerates far more writes |
| 5 | Rotating telemetry and metrics logging to the card, for troubleshooting, behind its own flag |

**The card path already half-exists**: the menu-pack feature reads from a card
(`ui/pack/ui_pack_storage_sd.cpp`), and there is an SPI arbiter with its own host suite, so this is not a
new bus.

**Open questions.**

1. **Decision 4 changes a documented contract, and that is this entry's sharpest edge.** Register 111 is
   published as non-persistent and a master may reasonably reboot a device to clear a session. Persisting
   it on a card makes the same register behave differently depending on which store an operator picked,
   which is worse than either answer alone. *Recommendation: if yes, say so in the register map and give
   the state its own status bit, so a master can tell which store it is talking to. A storage backend
   must not silently redefine a register.*
2. **What happens when the card is removed while it is the selected store?** The lifetime total is the
   one number that must never go backwards — Home Assistant records it per channel and anyone billing
   reads it. *Recommendation: fall back to internal and refuse to publish a total that would decrease;
   this needs a decision before any of it is built.*
3. **Is internal still written when external is selected?** A mirror doubles the writes the card was
   chosen to avoid; abandoning it means a removed card has lost everything since the switch.
4. **ASCII means a parse at boot, and a truncated last line is the NORMAL case** after a power cut, not
   the exotic one. What a half-written record means has to be decided before the format is.
5. **Telemetry rotation**: how many files, how large, what happens when the card fills, and whether the
   writer may take the SPI bus while the display and the pack loader want it.
6. **Does the card path share `DF25`'s error surface?** *Recommendation: yes — one warning glyph and one
   code sequence. An operator seeing a triangle should not have to know which store failed to look up
   the code.*

**One thing T1 already settled in this entry's favour.** `sensor_topology_nvs.h` and
`sensor_config_nvs.h` are both templated on the store, so an SD-backed store is an adapter rather than a
rewrite. That is a seam, not a finished decision: a human-readable ASCII file is a different
serialization shape from a key-value store, so the adapter has real work in it.

**Blocks.** Nothing. Not started, and it should not start before question 1 is answered.

---

## DF25 🟡 A write to non-volatile storage that fails is reported by nothing

Found 2026-08-30 while auditing what survives a power cycle (`N-f`). It is the fourth instance of one
shape and the shape is now worth naming: **a value with a home, a writer and no error path.** `DF22` was
registers nobody filled, `DF23` a field nobody assigns, `DF24` two fields never written — and this is a
write that can simply fail.

**What is discarded.** `preferences.begin("flow-data", false)` returns a bool saying whether the store
opened; it is discarded at `firmware.cpp:910`. Arduino's `Preferences::put*` return the number of bytes
written and **0 on failure**; every call site discards that too, and `sensor_config_nvs.h` documents the
discard as deliberate ("the return is deliberately ignored, as it was before") — which was an honest
description of the code it replaced and is now the thing to change.

**So a device whose store has failed looks perfect.** The live flow, the session volume and the lifetime
total all keep publishing from RAM, correctly, on Modbus and on MQTT and on the panel. The failure is
invisible until a power cycle discards the lifetime total — the one number Home Assistant records per
channel and the one anyone would bill from. Causes are not exotic: a store that fails to initialise, a
corrupt page, or a full partition (20 KB, five sectors, shared with the WiFi stack's own data —
`default_8MB.csv`, no custom table in `platformio.ini`).

**Decided 2026-08-30 by the owner:** use the return value, and when writes start failing show a warning
on the panel — a danger triangle with a CODE — with the codes documented on a wiki page as the start of
a numbered error-code sequence. **What counts as failing: N CONSECUTIVE failures on the same key**, so
the triangle means "this device is no longer keeping your totals" rather than "one write was retried".

**~~DF25a~~ ✅ DETECTION AND PUBLICATION LANDED 2026-08-30 — 49 host checks.** (A bold lead-in, not a
heading: I3 says sub-IDs are lead-ins so the heading census stays a census of items. This paragraph was
briefly a `###` and broke that rule in the same round the rule was corrected.)

`storage/nvs_write_health.h` owns the code space and the counting policy; N = 3, which at one pass a
minute means roughly three minutes of not keeping totals. Published on **Modbus register 34**
(`REG_STORAGE_FAULT_CODE`) and as **`storageFault`** on the MQTT diagnostics topic. Twelve writers now
report: the store's own `begin()`, the once-a-minute litre and calibration pass, the connected bitmap,
the topology, the link settings (including §4.1.1's unattended rollback), the live and factory-reset flow
unit, the LED settings through all five of their call sites, the network settings, the command epochs,
a master's cumulative-litre reset, and the factory reset's `clear()`.

**AND THE SECOND HALF OF THE DEFECT, which was worse than the first.** The once-a-minute pass advanced
its shadow copies unconditionally, immediately after the write nobody checked
(`persistedCumulative[i] = …` right after `saveCumulativeData`). So the dirty check said "already saved"
on the next pass and **a failed write was never attempted again** — which is what turned a transient
failure into a permanent loss. The shadows now advance only on success, and that makes the pass its own
retry: exactly one attempt per minute per key, bounded by the pass, so no retry storm. That is the
answer to question 2 and it cost less than a retry loop would have.

**The trap that would have shipped a fix worse than the defect.** `Preferences::putString` returns
`strlen(value)`, not a fixed width — so a SUCCESSFUL write of an empty string returns 0, and
`net_settings_nvs.h` writes an empty string on every save for any unset field, which for an unset MQTT
password is most devices in the field. Testing `returned != 0` there would have raised a storage alarm
on healthy hardware. `nvsPutStringOk` exists solely for that case and the suite asserts it. Every other
`put*` is checked as `returned == expected` rather than `!= 0`, which also catches a short write.

**The wiki table is GENERATED from the enum**, by `tools/wiki/gen-registers.mjs`, and reconciled in both
directions: a code appended to the header without a description fails the build, and so does a
description for a code that no longer exists. Verified by removing one — the generator names both halves
and exits 1. That is the same argument the register table itself is generated on, and it is what makes
the owner's "a sequence of error codes in the wiki page" a thing that cannot drift.

**Every code now has an author.** `Topology` (4) was the last one waiting, and `N-e`'s T2 gave it one
sooner than expected: T2 loads the topology at boot, so a stored set that is not a forest raises code 4
and names the channel. `PackAttemptCounter` (11) was the other and is wired too:
`NvsPackAttemptCounter` records its own outcome rather than returning it, so `PackLoader` — Arduino-free
and knowing nothing about storage — did not have to grow a health parameter to carry one bool through two
call sites.

**Three answers were BUILT ON THE RECOMMENDATION rather than decided**, each one line to change:
question 2 (the retry is the next pass), question 3 (one code space for every writer) and question 4
(RAM only — a store that cannot be written cannot record its own failure). They are asked properly in
the same round this landed.

**REVIEWED THE SAME DAY, and the review found twelve real things — four of which changed behaviour.**
Five independent lenses over the commit, each finding then verified by an adversary told to refute it:
twelve confirmed, eight refuted. Recorded in full because the two worst were the same defect this entry
is about, wearing a different coat.

1. **The network settings had the identical shadow-advance bug.** `netSettingsSavedRevision` advanced
   immediately after a write nobody had checked, so a failed apply was never re-attempted, the counter
   parked at 1, and the device came back after a power cycle on a MIXTURE of old and new credentials with
   register 34 reading 0. I had fixed exactly this in the litre pass and not looked for it anywhere else.
   The shadow now advances only on success, and the retry is RATE-LIMITED to once a minute — the naive
   fix would have written flash every tick, because that block runs on the logic loop rather than in the
   once-a-minute pass, and every write stalls the pulse sampler (§2.1.3).
2. **N = 3 was applied to writers that nothing retries.** An operator sets a baud rate once; a master
   writes the flow unit once. For those there is no next pass, so a single failed write parked a count of
   1 that could never be published — the failure was recorded and unreportable, which is this entry's own
   shape. **The cadence is now a property of the group**, in `storageFaultIsPeriodic`, beside the policy
   it changes rather than at twelve call sites: the four groups the once-a-minute pass re-attempts count
   to three, and the one-shot writers raise on the FIRST failure. The cost is stated in the header — a
   transient failure on a one-shot write shows the fault until the next successful write of that group,
   and a per-group retry queue would distinguish the two. That is the richer answer and it is not built.
3. **A partial failure across eight channels was recorded as a clean success.** `saveCumulativeToNvs`
   reported per channel, so channel 5's success zeroed the run channels 0-4's failures had built — and
   because `syncGlobalRegisters` runs after the loop, the alarm never reached register 34 in any frame.
   It now returns its outcome and the three reset loops OR the failures and report once.
4. **A factory reset that could not erase rebooted away its own evidence**, one second later, with the
   fault code held only in RAM. The restart is now held when code 12 is up, with a line on the serial
   console saying why. Rebooting a device whose reset did not happen accomplishes nothing.

And four things that were true of the documentation rather than the code, which in this repository is the
same class of defect:

5. **The "cannot drift" gate could drift.** The enum parser matched only `Name = N,` — so an enumerator
   with an implicit value, a hex value, or (the likeliest edit) no trailing comma on the last line was
   skipped in silence, producing a green build with the code missing from the operator's table. Hex and
   the optional comma are handled now, and anything still unparsed is named as a problem. Proven by
   appending a code in the demonstrated form and watching the generator fail.
6. **The generated page told operators the panel shows the code beside a warning triangle.** It does not;
   that is DF25's unbuilt second half. The page now says so, and states the per-group threshold honestly
   instead of claiming a flat three failures.
7. **`MQTT.md`'s diagnostics example did not carry the new key**, so the only integrator-facing
   description of the payload gave an integrator no key to alarm on while the register page told them to
   alarm. Added, with the sentence that matters: every other figure keeps arriving correctly from RAM
   while the writes fail, so this is the only remote warning.
8. **A `check(true, …)`** whose only failure mode was a crash, correctly called vacuous. Replaced with
   the two claims its own label made.

**A SECOND REVIEW PASS then found three defects in the two fixes written by hand** rather than the ones
review had handed over, which is its own lesson about where to look:

- **the held restart printed its notice on every logic pass** — `restartScheduled` stays true once set,
  so a fix meant to keep a fault readable emitted roughly a thousand console lines a second at
  `vTaskDelay(1)`. Latched. (The suppression itself is correctly narrow, and now says why rather than
  asserting it: `InteractionResult::restartScheduled` is assigned from the factory-reset state and
  nothing else, so it is the only scheduled reboot the firmware has.)
- **a storage failure delayed WiFi provisioning by up to a minute.** `noteProvisioningComplete` sat
  inside the write's new retry gate, so a failed save armed a 60 s timer and the operator's next
  credentials did not reach the radio until it expired. The block's own comment already said the radio
  does not depend on flash; the code now agrees.
- **the retry deadline was not rollover-safe.** `now >= deadline` against `millis()`; every other timer
  in the file uses `now - last >= interval`, which survives the ~49-day wrap. Matched.

**And the one review fix that was host-testable now has its test.** The `saveCumulativeToNvs` change
landed in `modbus_manager.cpp`, which four suites link, and it was verified only by "the suite still
passes". The shared `Preferences` stub gained an OPT-IN failure knob (default off, so the five suites
that depend on writes succeeding are untouched), and the FC16 suite now asserts that one dead channel out
of eight is recorded as one group failure, that three such resets publish code 2, and that a clean reset
clears it. Mutation-tested: restoring per-channel reporting fails 2 assertions.

Verified after all of it: host **2,168 checks across 29 suites**, 0 failures; firmware SUCCESS in the
container at RAM 24.7 % / Flash 39.0 % (81,040 and 1,303,457 bytes); the register-reference gate green
and shown to fail on both a removed description and an unparsable enumerator.

---

**Split in two, deliberately, and the second half rides with `N-e`'s T5.** The detection half is cheap
and host-testable; it landed as `DF25a` above.

**A CLAIM IN THIS ENTRY WAS WRONG, and the correction makes the remaining work cheaper.** It said the
panel half needs a new indicator "in the dataset, the exporter, the generated tables, the renderer and
the value catalogue". It does not. A warning banner already exists at every layer that matters — the
firmware renderer, the host tests, the mockup mirror, the unit and visual suites and both geometry
audits — and it is deliberately OUTSIDE the dataset/exporter/generated-table pipeline: no element
declares it, no binding reaches it, the exporter never sees it (`warningBanner.ts:7` against this
entry's own list). So the work is to extend `drawWarningBanner` and `bannerActive()` plus their five
mirrors, and it needs **no dataset, schema, exporter, generated-table or ABI work at all**. The ABI and
ledger leg only becomes DF25's cost if the numeric code is also exposed as a bindable catalogue value,
which is a separate decision — and on that route the one loud gate that would catch a missing resolver
arm (`everyScreenBindingResolvesTests`) never fires, because nothing binds it.

Still worth landing with T5 rather than alone: T5 is already editing the banner's class and the panel's
per-channel rows, and a second pass over the same five mirrors is the kind of duplication this
repository keeps paying for.

**The one thing the panel leg must decide explicitly:** whether a storage failure is exempt from the
editor suppression that `hasWarnings` obeys. A device that has stopped keeping totals arguably outranks
a `hold=cancel` prompt, and the existing comment's asymmetry argument is where the answer belongs.

**Open questions.**

1. **What counts as failing?** A single refused write can succeed on the next pass once the store
   compacts. *Recommendation: N consecutive failures for the same key, N small, so the triangle means
   "this device is no longer keeping your totals" and not "one write was retried".*
2. **Does a failed write retry, and how often?** A retry storm against a full partition is its own
   hazard, and the writer runs on the logic loop.
3. **One code space or several?** The link settings, the network settings, the LED and the pack counter
   all write to the same store. *Recommendation: one sequence — the operator sees one triangle, so the
   code has to be the thing that disambiguates.*
4. **Does the code survive a reboot?** A store that cannot be written cannot record its own failure,
   which is a small joke with a real consequence: the panel must be able to say it from RAM alone.

**Blocks.** Nothing that a test can see, which is the argument for filing it rather than waiting for a
bench. It is recorded now because `N-f` would otherwise be built on top of it.

---

## ~~DF26~~ ✅ FIXED 2026-08-30 — register 0 was documented as a `uint16` and is published as a `float32`

Found by the `DF25a` review while checking whether register 34 was genuinely free. It was — but the
sweep turned up that `REG_POLLING_RATE_KHZ` is written with `setFloat`, which occupies **registers 0 AND
1**, while the generated register reference described it as a one-register `uint16`.

**Why this is worth an ID for a one-word fix.** An integrator decodes real hardware against that table.
Reading register 0 alone as a `uint16` returns the high half of an IEEE-754 float — a plausible number
(16467 for 3.31 kHz) that moves in the right direction when the real rate changes, so it looks like a
working integration and is wrong by orders of magnitude. That is precisely the failure the register
reference is generated to prevent, and it survived because the reconciliation gate matches constant
NAMES and cannot see a multi-register SPAN.

Fixed by declaring the row `f32`, which makes the page state "2 registers, IEEE-754, high word first"
and correctly claims 0 and 1.

**And the gate now catches it, which took two checks rather than one.** The obvious one — walk the
documented rows in address order and fail when a span reaches the next address — was written first and
**would not have caught DF26**: describing a `float32` as a `uint16` makes the documented span NARROWER,
and narrower never collides. Under-declaration is the error that actually shipped, and the only authority
on width is the WRITER. So `checkWriterEncodings` reads every
`set{Uint16,Float,Double}(REG_NAME` out of `src/` and reconciles it against the row's encoding, the same
way the addresses are reconciled against the header. Both checks are in, and the second was proven by
re-introducing DF26 and watching the build fail naming both halves. The span check stays because it
catches the opposite error, an over-wide row overlapping its neighbour.

That "the first fix would not have caught the defect it was written for" is worth keeping. It is the same
lesson as this round's others: the check has to be aimed at the failure that actually happened.

---

## ~~DF24~~ ✅ FIXED 2026-08-26 — the aggregate topic carries real numbers, and the assembly is testable

`<base>/total/state` published `"total":0.000000,"sensors":0` for the life of the topic:
`MqttTotalTelemetry` has four fields and `firmware.cpp` assigned two.

**The fix is a MOVE, not an assignment.** `net/mqtt_snapshot.h` now owns the whole assembly — the
per-sensor loop, `uncalibratedFlags`, the aggregates and the diagnostics — and `firmware.cpp` is
reduced to filling an inputs struct and calling `fillMqttSnapshot`. That is what
`verification-blind-spots.md` prescribes for this shape and the second time it has been followed:
`modbus/sensor_config_nvs.h` was the first, extracted after the NVS serializer silently dropped two of
five calibration fields. Assigning the two fields in place would have fixed the symptom and left the
next one unreachable.

**One new semantic decision, and it is a decision rather than a transcription.** Nothing had ever
computed an aggregate LIFETIME volume, so `totalCubicMeters` had no precedent to copy. It is the sum
over IN-SERVICE channels, matching `sessionLiters` beside it and the engine's own `if (sensor.inUse)`
gate, so a subscriber comparing `total` against the per-sensor topics it receives gets an answer that
agrees. It can be argued the other way — an out-of-service meter's litres are still litres that
flowed — and the choice is stated at the site because changing it later is wire-visible.

Summed as LITRES and converted once, not as eight already-divided values: the test asserts the two
orders give different doubles here (`0.00266666666666666658` against `...701`), so the check
discriminates rather than passing on rounding.

**`activeSensors` is the count of in-service channels** — the same predicate as `present`, so it can
never disagree with the number of per-sensor topics a subscriber actually receives. That invariant is
the whole value of the field and it is asserted directly. When `N-e` lands, "how many meters feed the
total" stops being the same question, and it will want its own field rather than a redefinition.

**`DF23` was deliberately NOT fixed here**, and the test asserts `baselineKhz == 0` to say so. R2.1.2
wants a measured radio-off baseline recorded once per firmware update, which carries an unanswered
decision about a first boot with WiFi already enabled; folding it into a refactor would bury that.

**Verified:** 23 new host checks in `mqtt_snapshot_test.cpp`, wired into both the build and the run
list. Mutation-tested on exit codes: restoring the DF24 state (leaving `totalCubicMeters` unassigned)
fails 3 assertions, and counting every channel instead of the in-service ones fails 4. Host 2,058
checks across 27 suites; firmware SUCCESS at RAM 24.7 % / Flash 39.0 %.

**One process note worth keeping.** The first mutation run reported no failures at all, and I nearly
believed it: the grep filtered out `error:` and `head` truncated the pipeline before the assertions
printed. Third time this session that a piped grep has swallowed a signal. Mutations were re-run
capturing `$?` and counting both `FAIL$` and `error:` separately, which is the only form worth
trusting.

---

## DF23 🟡 The radio-off baseline is published as 0.000, so R2.1.1 and G1 have nothing to compare against

Found 2026-08-21 while checking that G1's procedure could actually be followed. It cannot, quite.

`MqttDiagnosticsTelemetry::baselineRateKhz` is declared, formatted into every
`<base>/diagnostics/state` payload as `baselineKhz`, and **assigned by nothing** — the only two
mentions in `src/` are its declaration and the `snprintf` that publishes it. So it goes out as `0.000`
on every publish, and `mqtt_publisher.h`'s own comment — "Carries the R2.1.2 pair so a polling
regression is visible in HA" — describes a pair with one half missing.

**The test does not catch it, for a reason worth naming.** `mqtt_publisher_test.cpp` sets
`snap.diagnostics.baselineRateKhz = 4.75f` by hand and asserts the payload says `4.750`. That is a
correct test of the FORMATTER, and it passes while production publishes zero, because nothing tests who
fills the struct. Same shape as DF22's silent registers: a value with a home, a publisher and no author.

**What R2.1.2 actually asks for**, and it is not a constant: *"The device must record the radio-off
baseline once, at the first boot after a firmware update, and expose both the baseline and the live
rate."* So it is a MEASUREMENT the device takes of itself, persisted, and compared against later — which
is what makes R2.1.1's "must not reduce the measured rate by more than 5 % from its radio-off baseline"
checkable at all, and what §2.1's own note calls "also the answer to open decision G1 — the same
measurement serves both."

**The decision it needs**, which is why this is recorded rather than fixed in the same round it was
found: what happens when the first boot after an update has WiFi ENABLED? There is then no radio-off
rate to record. The tidy answer is to defer — record no baseline until a boot with the radio off, which
§7 makes the default since WiFi is never enabled automatically — and that is a real choice about whether
a device that is never booted with WiFi off simply never has a baseline. *Recommendation: defer, and
publish `0.000` as "not yet measured" with the panel and the wiki saying so, because a fabricated
baseline would silently pass R2.1.1 forever.*

**Until it exists**, G1's comparison target is the 3.3 kHz DESIGN ASSUMPTION and not a measured figure —
G1's entry now says so rather than pointing at a register that reads zero.

**The published payload docs said `3.400`.** `tools/wiki/pages/MQTT.md`'s diagnostics example carried a
plausible non-zero baseline — the kind of figure that makes a document harder to trust than a blank
would be. Corrected 2026-08-21 to `0.000` with a paragraph saying the field is published, documented and
empty, and pointing here. Same treatment `WiFi.md` got when `DF22` was found: the page states what a
reader will actually see, and the firmware is what has to catch up.

**Blocks.** R2.1.1's acceptance test, and half of G1's procedure. Nothing on the device: the live rate
is real and published, on register 0 and in the same payload.

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

**One thing declined on purpose, and written down at the declaration:** the cache is written on the
logic task and read from whichever task calls `syncGlobalRegisters` — five of those calls are inside
`applyHoldingWrite`, which runs on the eModbus server task at priority 8 and preempts the logic task.
So a master's WRITE can read a half-assigned struct. It is unguarded because a master's READ cannot
race it at all (`handleReadHolding` never syncs), every scalar is word-sized and individually
consistent, the only field that genuinely tears is an AP string — bounded by its register span, so a
wrong string and never an overflow — and the next sync a second later repairs it. A double buffer would
cost more than that. The note says a guard was declined rather than forgotten, so a future field that
cannot tolerate a mixed read has somewhere to argue from.

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

**N-d2 ⏸️ — the ordering is protected as of 2026-08-21; the power-cut question still needs the board.**

`M5StamPLC.begin()` clears the RX8130CE's whole flag register including VLF, and the library exposes no
reader, so `plc::readRtcVoltageLowFlag()` must run **before** it. That single line is the only moment in
the device's life when "did the clock run across the last power cut" can be known. Move it, or add
anything touching the RTC above it, and a device with a dead clock reports a healthy one and publishes a
confident year-2000 timestamp to the panel, Modbus and MQTT.

**✅ The ordering half is done.** `test/host/boot_order_test.cpp`, nine checks in the host suite. A
runtime assertion cannot express this: the invariant is the order of two LINES IN A FILE, and by the
time either has run the flag register reads clean whichever way round they were — which is precisely the
failure. So it reads `firmware.cpp` as text, the technique `gen-actions.mjs` uses on the action
catalogue. It pins the probe before `begin()`, `readRtcEpoch()` after it (the calendar needs the bus
up), the probe inside `setup()`, and no `M5StamPLC.` call earlier in `setup()` at all. Negative-tested
both ways a refactor breaks it: the probe moved below `begin()` (2 failures) and an RTC read inserted
above it (1).

**Two things that first run corrected, both worth keeping:**

1. **A raw text scan reported the ordering broken on correct code.** The comment above the probe
   explains that "`M5StamPLC.begin()` clears the RX8130CE's flag register" — so the scan found a
   `begin()` six lines ABOVE the probe. The prose about an invariant must not be able to violate it, so
   the test blanks comments and string literals first, preserving byte positions so a failure still
   names a real line.
2. **"No `M5StamPLC.` call before the probe" was too strong as first written.** It failed on
   `M5StamPLC.readPlcInput` at line 817 — the sampler's pin-read helper, DEFINED above `setup()` and
   CALLED from a task that starts long after `begin()`. File position and execution order are different
   things, and a gate that confuses them fails on correct code, which is how gates get deleted. The
   check is scoped to `setup()`.

**What it cannot see, so the coverage is not overclaimed:** it does not parse C++. A probe moved inside
a conditional, or into a function called from `setup()` after `begin()`, would satisfy every assertion.
What it catches is the ordinary refactor — a line moved, or an RTC read inserted above it.

**⏸️ The other half needs the board.** Whether the RX8130CE survives power loss at all is **unknown**:
the chip has a backup-supply pin and M5Stack does not say whether a cell or supercap is populated.
Settle it empirically — set the time, pull power for a minute, boot, read VLF. No amount of software
answers this one, and building scaffolding around it would be worse than the honest gap.

**Blocks.** Any timestamp being trustworthy in the field — **and N-d1 no longer does**: RS485 can set the
clock as of 2026-08-18. What is left is N-d2, a correction (the assert protecting the VLF probe's position)
plus a measurement (the power-cut test), and it needs the board.

---

## ~~J9~~ ✅ FIXED 2026-08-21 — the orphan screen is retired, and the prompt that works is annotated

`nyquist-warning` sat in the dataset with plausible UP/DOWN flows and no route to it: no flow named it
as a `targetScreenId`, no `ui_pages.h` table named it, so `UiScreenRouter` could never resolve it.
Retired through the mechanism the generator already had for exactly this — the `RETIRED` set, whose own
comment says "`kept` retains anything the generator no longer emits, without this they would survive as
orphans". 80 screens to 79.

**The trap this had, and it is worth remembering.** `nyquist-warning` is BOTH a screen id and an
element id. Nineteen `nyquist-warning` ELEMENTS live on the sensor-settings screens, bound to
`config.sensor.nyquistWarning` — and those are the prompt's actual rendering surface, the mechanism that
WORKS. A grep-and-delete would have removed §5.5's prompt while leaving the thing that could not be
reached. All nineteen are still there and the count was asserted before and after.

**§5.5's prompt is not on a screen and never needed one.** `ui_actions.cpp`'s `consumedByPrompt`
reinterprets UP and DOWN on the editor screen itself while a commit is parked awaiting an override, and
its comment gives the reason: no new screen id, so §3.0.1's completeness rule stays satisfied and no
dataset change is needed. That comment now also records that a screen for this once existed and why it
does not, so the next reader does not rediscover the question.

**Four counts moved with it**, which is the real cost of residue: `ui_renderer.cpp` said "Twelve of the
eighty generated screens carry that full-screen overlay-bg, and eleven of them are reachable" — it is
now eleven of seventy-nine and *every one* is reachable, which is a simpler sentence than the one that
had to explain an exception. Also `ui_renderer.cpp`'s other "80 generated screens", `README.md`'s
"80-screen table" and `web/mockup/README.md`'s "80 screens".

**Verified:** dataset 79 screens, orphan absent, nineteen elements present; geometry audit 79 screens
0 findings; 220 unit, 51 exporter, 51 visual with NO baseline change — it was unreachable, so nothing
rendered it; host 1,997 checks; firmware SUCCESS at RAM 24.7 % / Flash 38.1 %.

**✅ GENERALISED 2026-08-25 — `test/host/ui_walk_test.cpp`.** J9 was found by hand, which is the wrong
way to find an unreachable screen. The walk now BFSs the real generated table from `kRequiredScreens` —
the firmware's own list of screens it resolves by name — over flow targets and submenu entries,
honouring gate satisfiability, and fails on any screen it cannot reach. It also refuses a dangling flow
target, an action id the catalogue does not advertise, the same button and gesture bound twice on one
screen, a dead end, and a gate naming a value its setting cannot take.

**Its first run found a second one, and it was NOT a defect** — which is the more useful outcome.
`state-idle` is unreachable and stays: idle is a MODE, `UiRenderer::update` blanks the panel and returns
so none of its elements is ever drawn, and it is kept because the MOCKUP reads its wake flows as a spec
(`App.tsx`) and deliberately refuses to draw its `- Display off -` label, which
`DisplayViewport.tsx` calls "the dataset being unfaithful". `tools/audit/screen-geometry.ts` exempts it
from the banner rule for the same reason. **That fact lived in three prose comments in three files and
was asserted nowhere.** It is now one exemption entry with its reason, checked in BOTH directions: an
unexplained unreachable screen fails, and an exemption that has become reachable — or that names a
screen which no longer exists — fails as stale. Negative-tested both ways.

---

## I2 — standing rule, not a decision

**The value catalogue is append-only.** Renumbering or repurposing an existing entry breaks every
authored pack that references it, and breaks it silently — the id still resolves, to something else.

This is not open and never closes; it is recorded here because it constrains every future change and
a rule filed under "archive" is a rule nobody reads. Same class as the wire-encoding rules on
`WifiState` and `kMqttFlags` bit 2.

**~~I2a~~ ✅ ENFORCED 2026-08-21.** `tools/catalogue/ledger.json` is the append-only record —
126 values and 19 actions, each with the ABI it appeared at — and
`tools/catalogue/check-ledger.mjs` is the gate, run in CI's *Generated docs* job.

**Why it is not the manifest diff that already existed.** `test/host/run.sh` fails if
`actionManifest.json` differs from a fresh generation, which looks like the same check and is not: it
enforces FRESHNESS. Rename a catalogue id, regenerate, and the two agree again — the rename passes with
a green build. A rule about history needs a record of history, which only ever grows.

**What is pinned: `category`, `type`, `unit`, `readOnly`.** Those four are the contract a pack binds
against. Deliberately NOT pinned — because a gate that cries wolf gets bypassed inside a month —
`description` (a wording improvement must never fail CI), `min`/`max`/`step` (the firmware owns the
domain, and this project has already widened one), and `register` (a Modbus concern that
`gen-registers.mjs` reconciles in both directions). What it CANNOT see is an action keeping its id and
quietly doing something else; nothing machine-readable captures that, and the script says so rather
than implying otherwise.

**It also enforces the bump rule the owner decided the same day** (see N-b): a new id whose
`catalogueAbi` is not higher than every `sinceAbi` already recorded fails, with the fix named. Every id
existing on 2026-08-21 is recorded at `sinceAbi: 1` — the ABI they were actually authored under.
Back-dating them across the WiFi/MQTT and N-c rounds was considered and rejected: no pack has ever been
stamped with anything but 1, so those numbers would have been invented history.

**Negative-tested through the real path** — editing the firmware catalogue and regenerating, not by
hand-editing the ledger. A renamed id fails as REMOVED *and* names the missing bump; a changed category
fails as REPURPOSED quoting the before and after; an addition without a bump fails and names the
constant to raise; the same addition with a bump passes and records `sinceAbi: 2` appended at the end;
and a reworded description passes, which is the case that decides whether anyone will keep the gate.

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
- **Sub-IDs are bold lead-ins, not headings.** A sub-line like `I2a` or `N-d2` is a bold lead-in inside
  its parent, never a heading of its own, so the census never double-counts one. **This rule said `###`
  until 2026-08-30 and every heading in the file is `##`** — so the command it named,
  `grep '^### '`, matched nothing at all, which is a rule describing a check that could not run. `##` is
  also the right level: the index and the two closed tables are `##` peers of the items, so demoting
  items to `###` would nest them under nothing. The census that works, and excludes the four structural
  sections by matching the ID itself:

      grep -cE '^## (~~)?(DF|J|N-|G[0-9]|I[0-9])' docs/active_work/open_decisions.md

  It reports **13** today: six open, five struck through in place, and the two standing rules.
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
