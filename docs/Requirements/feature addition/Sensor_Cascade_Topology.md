# Requirement: Sensor Cascade Topology

**Version:** 0.2 (proposal — five open questions in §7 need answers before implementation)
**Date:** 2026-08-26

> **0.2** — Q1 decided: an uncalibrated root publishes its branch as UNKNOWN rather than promoting
> its children, which forces the new R2.5 (the total must be able to say "unknown" at all) and opens
> Q1a. **0.1** — first draft. Four decisions were taken by the owner on 2026-08-26 and are normative
> here: the total is the sum of roots, red fires on departure from a commissioned baseline, an
> orphan re-parents to its nearest in-service ancestor, and MQTT stays read-only for topology.
> §5 records the four findings that shaped them, because three of the four reverse what the
> feature looked like when it was first described.

**Depends on:** the per-sensor register block in `Project_document.md` §5, the settings catalogue
and completeness rule in `Loadable_UI_Menu_Packs.md` §3.0.1, the MQTT security position in
`WiFi_MQTT_Connectivity.md` §4.4.1, and project rule **I2** (the catalogue is append-only).

**Blocked on:** `DF24` in `docs/active_work/open_decisions.md` — see §9.

---

## 1. Purpose

Today the eight channels are assumed independent. `SensorStateEngine::update` adds every in-use
channel's session volume and instantaneous flow into one pair of accumulators
(`sensor_state_engine.cpp:72-73`), and that pair is the device's idea of "total". On a parallel
installation — eight separate pipes — it is correct.

It is wrong on a **cascade**: a main meter with others downstream of it. Water measured by a
downstream meter has already been measured by its parent, so the sum double-counts, and the error is
not small — a main plus two sub-meters reads roughly twice the delivered volume.

A cascade also offers something a parallel installation cannot: **verification**. If a parent's
reading should equal the sum of its children's, the difference is evidence. Water entering a branch
that no child measured is either an unmetered outlet or a leak, and the device is the only thing on
site positioned to notice.

This document specifies both: a topology the operator declares, a total that is correct under it, and
a verification that is useful rather than merely arithmetically true.

---

## 2. What is decided

Four decisions, taken 2026-08-26. Each was a genuine fork; §5 says why each went the way it did.

| # | Decision |
| --- | --- |
| 1 | **The upstream meter is authoritative.** The delivered total is the sum of ROOT channels. All skew is booked as unmetered loss inside the branch. |
| 2 | **Red fires on departure from a commissioned baseline**, not on an absolute skew ratio. |
| 3 | **An orphan re-parents to its nearest in-service ancestor**, never to root. |
| 4 | **MQTT is read-only for topology.** It carries the skew, the state and the warning; the parent setting is written only from the panel, RS485 or the portal. |

---

## 3. Behaviour Specification

### 3.1. The topology is a forest of parent pointers

Each channel stores one **parent**: `0` for a root, or `1..8` naming another channel. Multiple roots
mean parallel branches, which is the general case and today's default.

> **R1.1** — The parent is a per-channel SETTING, persisted in NVS, defaulting to `0`. A device
> upgrading from firmware without this feature has no stored key and therefore reads `0` on every
> channel, which is the parallel topology it already had.

> **R1.2** — Depth is limited to **7 edges** — a chain of all eight channels. This is not a
> restriction: an acyclic forest of eight nodes cannot exceed it, so the limit is a consequence of
> R1.3 rather than a rule of its own. It is stated because "max 7 levels" is ambiguous between edges
> and nodes, and reading it as 7 nodes would refuse one physically legal shape for no reason.

> **R1.3** — The stored topology must be a **forest**: acyclic, every parent in range, and no
> channel its own ancestor. A write that would break this is REFUSED and changes nothing (§3.6).

> **R1.4** — A channel's **effective parent** is the nearest ancestor that is in service, found by
> walking up and skipping out-of-service channels. A channel whose walk reaches `0` is an effective
> root. Decision 3 in §2 is this rule; it is what keeps the total correct when a mid-chain meter is
> taken out of service.

### 3.2. The delivered total is the sum of roots

> **R2.1** — The **delivered total** — volume and flow — is the sum over channels whose *effective*
> parent is `0`. A downstream channel contributes nothing to it directly; its water is counted by
> its root.

> **R2.2** — With every parent at `0`, R2.1 must reduce to today's arithmetic **bit-identically**,
> not merely equivalently: the same index set, in the same ascending order, so the same IEEE-754
> accumulation. This is testable and must be asserted, because it is what makes the feature additive
> for every existing installation.

> **R2.3** — The existing gross accumulators are **not repurposed**. `totalSessionLiters` and
> `aggregateFlowLpm` keep their present meaning and their present consumers; the netted pair is new
> and sits beside them. The reason is in §5.3: three consumers want gross and would misbehave on a
> netted value.

> **R2.5** — The delivered total must be able to say **"unknown"**, because R3.8's decision makes that
> a reachable state: one uncalibrated root means one branch whose volume nobody knows, and the sum of
> roots is then not a figure the device can stand behind.
>
> A float register cannot hold "unknown" by omission — zero is a legal reading and blanking it would
> be indistinguishable from no flow, which is precisely the confusion the `--`-never-means-not-detected
> rule exists to prevent. So the total is published as **NaN** while any branch is unknown, alongside
> a **bitmap naming the unknown branches**. NaN is decodable by any master as "not a number", and it
> propagates: a master that ignores the bitmap and sums it gets NaN rather than a plausible wrong
> figure. The panel shows its own token for the same state, and MQTT omits the key rather than
> publishing `null` into a payload consumers parse as numbers.
>
> See §7 Q1a for the one sub-decision this leaves: whether a partial sum is ALSO published for the
> branches that are known.

> **R2.4** — Liveness must not be derived from a netted value. The blue LED channel
> (`led_controller.cpp:100-101`) and P0's flow-dot animation (`ui_renderer.cpp:425`, `aggregateFlowLpm
> > 0.01`) must read "any in-service channel is flowing", not the netted aggregate — otherwise a dead
> root with a flowing child shows the device as idle, which is exactly the condition the feature
> exists to surface.

### 3.3. Verification compares accumulated volume, not flow

> **R3.1** — Skew is computed on **accumulated volume over a window**, never on instantaneous flow.
> Channels are read per-channel rather than simultaneously, and a pipe has transit delay, so an
> instantaneous comparison flaps between samples on a correctly plumbed installation. Volume
> integrates both away.

> **R3.2** — For each channel with at least one in-service child, the skew is
> `parent_volume − Σ children_volume` over the window, and the ratio is that difference divided by
> the parent's volume over the same window. The **sign is retained**: positive means water entered
> the parent that no child measured (an unmetered outlet, or a leak); negative means the children
> measured more than the parent (a miscalibration, or a child fed from elsewhere). Both are
> actionable and they are not the same fault.

> **R3.3** — Below a **minimum absolute volume floor**, the ratio is not published as a percentage
> and no breach can be raised. A percentage of a near-zero parent volume is noise, and the first
> litres after a reset would otherwise always breach. The floor is a setting.

> **R3.4** — The device publishes, per verifying channel: the current ratio, the **absolute litre
> difference**, and the **peak ratio since the window opened**. The litre difference and the peak
> exist because of §5.2: a ratio decays as throughput grows while the missing litres stay in the
> numerator, so a bounded overnight leak is invisible in the ratio alone by morning.

### 3.4. Red means "different from how it was commissioned"

> **R3.5** — A channel is marked red when its skew ratio departs from a **commissioned baseline** by
> more than the margin — not when the ratio exceeds an absolute threshold. The baseline is recorded
> by an explicit operator action once the installation is understood.
>
> This is decision 2, and §5.2 is the argument: in a real installation the dominant term is the
> unmetered fraction, not meter tolerance. One outside tap can exceed 10 % of household consumption,
> and accumulated skew *converges* on that ratio rather than decaying. An absolute threshold
> therefore measures the plumbing and would sit red from commissioning day, correctly and uselessly.

> **R3.6** — Until a baseline is recorded, verification **reports but does not judge**: the skew,
> the litre difference and the peak are all published, and nothing is marked red. A device that has
> never been commissioned must not claim a fault.

> **R3.7** — The comparison window's basis is **invalidated when the participant set changes** — a
> re-parent, a child entering or leaving service, or a baseline being recorded. Without this,
> re-parenting a channel charges its whole accumulated history to a new parent's children-sum and
> produces an immediate, permanent, false red.

### 3.5. Degraded nodes

> **R3.8** — A channel that is in service but has **no valid calibration** contributes a frozen
> volume and a zero flow: `sessionLiters` and `cumulativeLiters` are only assigned inside
> `if (configIsValid(config))` and the else-arm zeroes only `instantFlow_L_min`
> (`sensor_state_engine.cpp:60-71`), while line 72 still adds the frozen value. As a **root**, such a
> channel would silently zero its entire branch's contribution to the delivered total while its
> calibrated children meter real water. The trigger is ordinary — a meter swap clears the
> calibration.
>
> **DECIDED 2026-08-26: the branch is published as UNKNOWN.** Its children are NOT promoted to roots.
> The owner's reasoning, and it is the stronger one: a total that silently shrinks is a total that
> lies, and a metering device should refuse to state a figure it cannot support rather than state a
> smaller one. Promotion would have kept a number on the screen at the cost of that number being
> wrong by the whole branch.
>
> This has a consequence that has to be specified rather than discovered — see R2.5.

> **R3.9** — A channel that is undersampling (`REG_UNDERSAMPLING_FLAGS`) is reading LOW by
> definition, so any skew computed from it is not evidence of anything. Verification involving such a
> channel must be reported as unavailable rather than as a breach.

### 3.6. Validation is a refusal, and a topology commits atomically

> **R4.1** — An invalid topology write is **REFUSED and changes nothing**. The house style applies:
> a per-sensor configuration refusal is a Modbus exception, as `prepareConfigUpdate` does for a
> Nyquist violation and `DeviceClock::setTime` does for an implausible date. The precedent is
> deliberate — `DF14` chose a hard refusal over an override for a negative offset, on the owner's
> explicit decision, and a parent pointer is less recoverable than an offset.

> **R4.2** — The following are refused: a channel naming itself; a parent index outside `1..8`; any
> write that would create a cycle of any length; and any write exceeding R1.2's depth.

> **R4.3** — Cycle detection must consider the STORED topology, not the effective one. A validator
> that walks effective parents can be defeated by ordering: write two parents, take the middle
> channel out of service so the walk short-circuits, close the loop, then put it back — and the
> stored topology now holds a genuine cycle that every subsequent walk trusts.

> **R4.4** — A topology is **staged and applied atomically**, using the project's `0x5AA5` idiom
> (`REG_LINK_APPLY`, `NET_APPLY`, `REG_CLOCK_APPLY`). Rebuilding a tree takes several writes and the
> intermediate states are frequently invalid; a per-write validator would refuse the legal end state
> because of the illegal path to it. One FC16 frame commits a whole tree or refuses it whole.

> **R4.5** — The **threshold, the margin and the floor are NOT part of the staged topology object.**
> They are tuning parameters and must be writable without committing a topology. If they share the
> atomic commit, the natural response to a false red — nudge the margin — invalidates the comparison
> basis on every branch (R3.7), and the operator can never converge because every adjustment destroys
> the measurement it was made against.

---

## 4. Surfaces

> **R5.1 — Panel.** One per-channel row showing the parent, on the existing per-sensor config level,
> which is already scoped to a single channel. A tree view is not attempted: 40 columns of Font0
> cannot draw one legibly. The completeness rule (`Loadable_UI_Menu_Packs.md` §3.0.1) will REQUIRE an
> editor for this setting — it is numeric and not network-prefixed, so it qualifies for neither
> exemption in `tools/catalogue/coverage.mjs`.

> **R5.2 — Modbus.** The parent lives in the **per-sensor block** at a free offset (`0..25` are in
> use, `SENSOR_BLOCK_SIZE` is 40), declared with `registerOffset` rather than an absolute address.
> `ui_settings_types.h` documents exactly why: a per-sensor setting has no single absolute address
> because it depends on which channel the navigation level selected, and recording one is untrue.
> The verification results are read-only registers.

> **R5.3 — HTTP portal.** See §7 Q2. Serving this setting from the portal means injecting a settings
> store into `PortalForm`, which turns on every per-sensor setting at once — calibration included.
> That is a scope decision, not a detail.

> **R5.4 — MQTT.** Read-only, per decision 4. The diagnostics payload gains the per-channel skew,
> the litre difference, the peak and the state; the topology itself is not writable. §4.4.1's
> security position is explicit that a reset is recoverable by re-reading the meter while a changed
> calibration is not — and a parent pointer is less recoverable than a calibration, because it makes
> every total wrong rather than one channel.

> **R5.5** — A **legacy master must be able to tell**. A master that sums the eight per-sensor blocks
> gets a double-counted figure on a cascaded device and has no way to know. The per-sensor status word
> has free bits; see §7 Q6.

---

## 5. The findings that shaped this

Recorded because three of the four decisions in §2 reverse what the feature looked like when it was
first described, and the reasoning is the valuable part.

### 5.1. Netting cannot corrupt Home Assistant history — the fear was misplaced

The initial concern was that changing the total would corrupt HA's long-term statistics. It cannot.
The `total_increasing` entity that feeds the Water dashboard is **per-channel**
(`ha_discovery.h:105` — "this is the Water dashboard one"), fed from raw per-channel
`cumulativeLiters`, and **no HA entity subscribes to the aggregate topic at all**. There is also no
aggregate Modbus register today, so nothing on the bus can shift meaning either.

The hazard becomes real only if an aggregate `total_increasing` entity is ever added: a later topology
edit steps its value down, and HA reads a decrease as a counter reset. That is a reason not to add
one, and a reason to verify HA's reset arithmetic first if anyone ever wants to.

### 5.2. An absolute percentage threshold measures the plumbing, not a leak

The obvious threshold is derived from meter tolerance: ±2 % parts compared against a sum give roughly
a ±4 % envelope, so 5–10 % looks defensible. **It is the wrong model.** In a real installation the
dominant term is the *unmetered fraction* — one outside tap, one bypass, one branch nobody metered. A
single garden tap can exceed 10 % of household consumption, and an accumulated-volume skew converges
on that steady-state ratio rather than decaying toward zero. So an absolute threshold marks the main
channel red on commissioning day and keeps it red, correctly.

Worse, the same arithmetic hides the fault the feature is for: a **bounded** leak — a cistern
overnight — puts a fixed number of litres in the numerator while the denominator grows with normal
consumption, so the ratio *decays*. By morning it is under any threshold. Hence R3.5 (compare against
a commissioned baseline) and R3.4 (publish the litre difference and the peak, not only the ratio).

### 5.3. Three consumers want the GROSS total, so it must not be repurposed

- The **red LED** pulses once per N litres of accumulated volume (`led_controller.cpp:106`,
  `deltaLiters = totalSessionLiters - lastTotalLiters_`). Its step is persisted and Modbus-writable, so
  a site whose commissioning notes say "one blink per 100 L" would quietly change meaning.
- The **blue LED** derives liveness from `aggregateFlowLpm` (`led_controller.cpp:100-101`).
- **P0's flow dots** animate on `context.aggregateFlowLpm > 0.01` (`ui_renderer.cpp:425`).

The last two are the sharp ones: under a netted aggregate, a root reading zero while a child meters
real flow gives a netted zero, so the panel and the LED both report "no water" during exactly the
condition being detected. Hence R2.3 and R2.4.

### 5.4. An uncalibrated root deletes its branch

Verified at `sensor_state_engine.cpp:60-71`: `sessionLiters` and `cumulativeLiters` are assigned only
inside `if (configIsValid(config))`, and the else-arm assigns only `instantFlow_L_min = 0.0f`. Line 72
then adds the frozen `sessionLiters` regardless. A root in this state contributes a frozen volume and
zero flow while its children meter real water, so the branch's netted contribution is zero or stuck.
No candidate root predicate considered calibration quality. Hence R3.8 and §7 Q1.

---

## 6. Test Considerations

1. **The reduction proof is the first test.** With every parent `0`, the netted pair must be
   bit-identical to the gross pair — asserted in a binary that links the real engine, not argued in a
   comment.
2. **The topology logic belongs in an Arduino-free unit** with its own host binary, as `NtpPolicy`
   and `MqttCommandRouter` are split from their adapters. Parenthood, effective-parent resolution,
   depth, cycle detection at every cycle length, orphan re-parenting, the skew arithmetic and every
   state transition are all decisions, and decisions in a file no test links are decisions nobody
   checks.
3. **Cycles must be testable by forcing the stored array directly**, independently of the validator —
   otherwise the walk's boundedness is only ever proved by the thing that is supposed to prevent
   cycles existing.
4. **Do not put the aggregation in `firmware.cpp`.** It is in no host link set, and that is precisely
   how `DF24` and the NVS sensor serializer both shipped broken. Move the snapshot assembly out first
   (§9).
5. The panel row, the portal field and the MQTT keys each need a case; the binding must appear in the
   `ui_walk_test` sweep automatically once it is on a screen.
6. Negative-test every refusal in R4.2 by attempting it over the real `applyHoldingWrite` path.

---

## 7. Open Questions

| Q | Question | Recommendation |
| --- | --- | --- |
| ~~1~~ | ~~An in-service root with no valid calibration (R3.8)~~ | **DECIDED: publish the branch as unknown** |
| 1a | Is a PARTIAL sum also published for the known branches? | Yes, on its own register, never in place of the total |
| 2 | Should the portal serve non-network settings generally? | Panel + RS485 only for v1 |
| 3 | Does a skew breach latch or self-clear? | Latch, cleared by a named command |
| 4 | Does a per-channel session reset reset its subtree? | Channel only, plus a separate verification reset |
| 5 | Does the LIFETIME total need a netted counterpart? | Yes, before anyone bills from it |
| 6 | Should a downstream channel be marked for legacy masters? | Yes — a free status bit |

1. ~~**An in-service root with no valid calibration.**~~ **DECIDED 2026-08-26: publish the branch as
   unknown**, against my recommendation to promote the children. The owner's reasoning is the stronger
   one and is now R3.8: a device should refuse to state a total it cannot support rather than state a
   smaller one. Promotion keeps a number on the screen at the cost of that number being wrong by a
   whole branch, and a wrong number is harder to notice than a missing one.

1a. **Is a partial sum also published?** R2.5 makes the total NaN while any branch is unknown. An
   integrator with seven good branches and one uncalibrated root still has a use for the seven.
   *Recommendation: yes, on its own clearly-named register and MQTT key — never in place of the total,
   and never as the value the panel shows for "total". The failure to avoid is a partial sum being read
   as a complete one, which is why it gets its own address rather than a flag beside the real total.*

2. **Should the HTTP portal serve non-network settings generally?** `PortalForm` is network-only
   today. Injecting a settings store to reach the parent turns on every per-sensor setting including
   calibration; a parent-only store would be a second settings path, which is its own defect shape.
   *Recommendation: panel and RS485 for v1. Revisit the portal as its own decision, because "the
   portal can now edit calibration" deserves to be chosen rather than inherited.*

3. **Latch or self-clear?** A self-clearing breach is invisible the morning after — §5.2's decaying
   ratio makes this concrete. *Recommendation: latch, with an explicit clear command, and keep the
   peak alongside so the trace survives even after clearing.*

4. **Does a per-channel session reset reset the subtree?** Resetting a parent without its children
   leaves a window whose two halves cover different periods, which is a guaranteed false skew.
   *Recommendation: a per-channel reset resets only that channel but INVALIDATES the verification
   basis for its branch (R3.7 already requires this), plus a dedicated verification reset that touches
   no measurement. Destroying measurement to clear a diagnostic is the wrong trade.*

5. **Does the lifetime total need a netted counterpart?** Session-only netting leaves the lifetime
   figure double-counted on a cascade — and the lifetime figure is the one Home Assistant records per
   channel and the one anyone would bill from. *Recommendation: yes, and specify it in v1 even if it
   ships second, so the register and topic layout leaves room.*

6. **Should a downstream channel be marked for legacy masters?** R5.5. *Recommendation: yes — one
   free bit in the per-sensor status word. It is nearly free and it is the only way a master written
   before this feature can tell that summing the blocks is now wrong.*

---

## 8. Risks

| Risk | Mitigation |
| --- | --- |
| A false red on commissioning day makes the whole feature distrusted, permanently. | R3.5's baseline and R3.6's report-don't-judge default. The first impression is the feature's only chance. |
| The netted total is correct and the panel still shows the gross one, or vice versa, because two accumulators exist. | R2.2's bit-identity assertion plus explicit per-consumer decisions in R2.3/R2.4. Never let "which total is this?" be answerable only by reading the call site. |
| A cycle stored over Modbus makes the total quietly wrong. | R4.3 validates the stored topology, and R4.4 commits atomically. Tested by forcing the array, not only through the validator. |
| The catalogue ABI bump makes new packs unloadable on older firmware. | Accepted, and inherent to any catalogue addition since 2026-08-21. `check-ledger.mjs` enforces the bump. |
| The aggregation lands in `firmware.cpp` and is untestable, repeating `DF24`. | §6.4 and §9's precursor slice. |
| One uncalibrated root makes the headline total unavailable (R2.5), and an operator reads that as the device being broken. | The commissioning warning already names the channel, and §2c's banner already carries this class. The panel must say WHICH branch is unknown, not merely that the total is. |
| Home Assistant keeps double-counting anyway because all eight per-channel entities are configured as water sources on the dashboard. | Out of the firmware's control. Worth telling the operator plainly in the wiki: netting fixes the device's total, not a dashboard that adds sub-meters to their parent. |

---

## 9. Implementation slices

Ordered so each is verifiable when it lands.

| Slice | Content | State |
| --- | --- | --- |
| **T0** | **Precursor: fix `DF24` and move the MQTT snapshot assembly out of `firmware.cpp`** into a host-linkable unit. The cascade's aggregation must not land where no test can reach it. | ⛔ do first |
| **T1** | The topology module: parent storage, effective-parent resolution, depth, cycle detection, forest validation. Arduino-free, own host binary. | |
| **T2** | The netted aggregation beside the gross one, with R2.2's bit-identity assertion and R2.4's liveness repointing. | |
| **T3** | The skew arithmetic: window, floor, sign, baseline, peak, litre difference, and the states. Same module as T1. | |
| **T4** | Modbus: the per-sensor parent offset, the staged apply, the read-only results, R4.5's separate tuning registers, and the wiki reconciliation rows. | |
| **T5** | Panel: the per-channel parent row and editor, the skew row, the warning-banner class, and the catalogue ABI bump with its ledger append. | |
| **T6** | MQTT: the diagnostics keys, read-only. Home Assistant entities deliberately NOT added — see §5.1. | |
| **T7** | The requirements this document leaves open (§7) resolved, the wiki page, and the register entry closed. | |

**T0 is not optional.** Every slice after it writes into an aggregate whose assembly is currently
untested and demonstrably wrong.
