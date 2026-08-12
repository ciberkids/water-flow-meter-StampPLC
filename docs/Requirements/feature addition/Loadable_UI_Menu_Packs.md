# Requirement: Loadable UI Menu Packs

**Version:** 0.2 (proposal — open questions in §7 need answers before implementation)
**Date:** 2026-07-30

> **0.2** — adds §3.0 defining what a menu *is*, with the completeness rule that every menu
> must expose every settable value. Replaces the proposed symlink selection with a pointer
> file, because FAT has no symlinks (verified against `fatfs/src/ff.h`: the API offers
> `f_rename`/`f_unlink`/`f_readdir` and nothing resembling `link`). Splits the "hidden menu"
> into a discoverable root-level page plus a UP+DOWN+ENTER recovery gesture.

**Depends on:** the navigation model in `Display_UI_Requirements.md` §5, and decision **D2**
(generate the manifest from firmware) in `docs/archive/open_decisions-closed-2026-08-12.md`

---

## 1. Purpose

Today one UI is compiled into the firmware. `screens.json` becomes `constexpr` tables in
`GeneratedUi.{h,cpp}`, so changing a single label means a rebuild and a reflash.

This feature makes the **presentation layer** swappable at runtime. The operator selects
one of several menus; the firmware loads it from the microSD card and renders it. A default
menu stays embedded in the firmware as an always-available fallback.

The split that makes this safe already exists:

| Layer | Owner | Swappable? |
| --- | --- | --- |
| **Values and actions** (the catalogue) | Firmware | **No.** A pack can only reference what the firmware already exposes. |
| **Gesture semantics** (short/long per screen type, §3.2) | Firmware | **No.** Constant across every menu, so muscle memory transfers. |
| **Screens, levels, placement, wording, theme** | Menu pack | **Yes.** This is the whole point. |

Decision **D5** already established that the design tool offers settable values as a fixed
catalogue rather than free-form strings. That makes the catalogue an **ABI**, and an ABI is
exactly what a loadable pack needs to bind against.

---

## 2. Hardware / Interfaces

| Component | Connection / Address | Notes |
| --- | --- | --- |
| microSD slot | Built-in (see `../../hardware docs/StampPLC specifications.md`); `M5StamPLC::sd_card_init()` | Card is **optional**. Absent, unreadable or removed-while-running must never fault. |
| Flash (app partition) | 3.3 MB, currently 17.8 % used | Holds the embedded default pack. There is presently **no data partition** — see §7 Q1. |
| RAM | 327 KB, currently 25 KB used | A loaded pack lives in one heap buffer. Estimated pack size for today's 48 screens is ~13 KB; the cap is 64 KB. |
| NVS (`Preferences`, namespace `flow-data`) | — | Stores **only** the load-attempt counter (§3.6). The selection lives on the card as `/ui/active` (§3.1.1), so there is one source of truth. |

---

## 3. Behaviour Specification

### 3.0. What a menu is

> **A menu is the complete set of screens, their content, the connections between them,
> and the settings pages. Every menu must allow every available value to be set.**

Formally, a menu comprises:

| Part | Meaning |
| --- | --- |
| **Screens** | Every page, at every level, including `BACK` entries, confirm screens and toasts. |
| **Content** | Each screen's elements: their kind, placement, wording, and which catalogue value they bind. |
| **Connections** | The navigation graph — which screen each flow leads to, and how levels nest. |
| **Settings pages** | A value editor for **every** settable value in the catalogue, reachable by navigation. |

#### 3.0.1. The completeness rule

**A menu is invalid unless every `category: "setting"` value in the catalogue has a
reachable editor, and every action those editors require is referenced.** With today's
catalogue that means 10 editors: `config.modbusSlaveId`, `.baudRate`, `.parity`,
`.stopBits`, `.ledPulseVolume`, `.ledPulsePeriod`, and the four `config.sensor.*` entries.

This is stronger than "the pack is well-formed", and it is the point. It turns the ABI check
from *"does everything this pack references exist?"* into *"does this pack expose everything
the firmware offers?"* — which makes an entire failure class impossible: **you can never
load a menu that leaves a setting unreachable.** Without it, a pack that simply forgot the
parity editor would load cleanly and silently strand that setting until someone reflashed or
swapped cards.

It also hardens the action check in a way the pack cannot dodge. An editor is only
operable if the pack references `config.action.value.increment`, `.decrement`, `.commit` and
`.discard`, plus `ui.action.nav.descend` and `.back` to reach and leave it. So the required
action set is **derivable from the catalogue** rather than trusted from the pack's own
declaration.

**Checked in both places, with different consequences:**

- **At export** (§5) — a failure. The exporter will not write a pack that is incomplete.
- **At load** (§3.3) — a *soft* failure: the pack loads, and the firmware appends its own
  built-in editor for each missing setting so reachability is preserved regardless. Refusing
  to load would punish the operator for an authoring mistake by giving them no UI at all;
  filling the gap keeps the device usable and logs what was patched.

> **Open — does this forbid a deliberately reduced menu?** A read-only "operator" or kiosk
> menu that intentionally hides configuration is a legitimate thing to want, and strict
> completeness forbids it. See §7 Q10; the load-time fallback above is what makes a
> `restricted` variant safe if you want one.

### 3.1. The container format — one file per menu

**A menu is a single file, not a directory.** `/ui/<name>.uipack` on the card.

A single file is chosen deliberately over a directory tree:

- **Atomicity.** A half-copied file fails its CRC and is rejected. A half-copied *directory*
  looks structurally valid and fails unpredictably at render time.
- **One read.** No directory walking on an embedded filesystem, no per-file open cost.
- **User-manageable.** Copying one file onto a card is a thing a person can do correctly.
- **Versionable and checksummable** as a unit.

The directory organises the packs and records which one is active:

```
/ui/
  active                    <- pointer file: one line, the selected pack's filename
  production.uipack
  commissioning.uipack
  service.uipack
```

Discovery lists `/ui/*.uipack`, sorted by filename, capped at **8** entries.

#### 3.1.1. Selection is a pointer file, because FAT has no symlinks

The natural way to express "this one is in use" would be a symlink, and that was the
original proposal. **It cannot be built.** The card is FAT-formatted and reached through
FatFs, whose entire API is `f_open`, `f_read`, `f_write`, `f_rename`, `f_unlink`, `f_mkdir`,
`f_readdir`, … — verified against `fatfs/src/ff.h` in the installed toolchain. There is no
`link` and no `symlink`; `f_unlink` is *delete*. Arduino's `SD.h` exposes no link API either.
FAT has no such concept at any level.

The **pointer file** preserves the intent exactly and is the direct FAT analogue:

| Symlink behaviour wanted | Pointer file equivalent |
| --- | --- |
| A name that refers to another file | `/ui/active` contains that file's name |
| Selecting rewrites the link | Selecting rewrites one short line |
| Dangling link → fall back | Named file absent → fall back |
| Missing link → fall back | `/ui/active` absent → fall back |
| Editable from a PC | Editable from a PC, and *more* legible than a symlink |

Format: a single line holding a bare filename, e.g. `production.uipack`. Leading and
trailing whitespace is ignored; anything containing a path separator is rejected, so the
pointer can only ever name a file inside `/ui/`.

**Why the selection lives on the card rather than in NVS.** It means a card can be prepared
on a PC and will boot with the intended menu on any unit, and the choice is visible and
changeable without the device. That is worth the one small write. NVS is still used, but
only for the anti-boot-loop counter (§3.6) — never for the selection, so there is one
source of truth and no chance of the two disagreeing.

**Any of these falls back to the embedded default:** no card, no `/ui/`, no `active` file,
an `active` file that is empty or unparseable or contains a path separator, a named pack
that does not exist, or a pack that fails validation (§3.3). All are logged with the
specific reason, and all are recoverable by fixing the card.

### 3.2. Layout — offset-based and read in place

The pack must be **relocatable**. The current `ui_exporter::Screen` holds `const char*` and
`const Element*`, which are absolute addresses fixed at link time — a pack cannot contain
pointers. Every internal reference is therefore a **byte offset from the start of the
buffer**, and the firmware reads the structure in place rather than parsing it into a graph
of allocated objects. That gives one allocation instead of hundreds, and no fragmentation.

```
┌─ Header (fixed 64 bytes) ────────────────────────────────────┐
│ magic          char[6]   "WFMUI\0"                           │
│ formatVersion  u16       layout version; mismatch = reject    │
│ catalogueAbi   u16       catalogue version the pack targets   │
│ payloadBytes   u32       must equal fileSize - 64             │
│ crc32          u32       over the payload only                │
│ levelCount     u16                                            │
│ screenCount    u16                                            │
│ levelsOffset   u32                                            │
│ screensOffset  u32                                            │
│ themeOffset    u32                                            │
│ stringsOffset  u32                                            │
│ stringsBytes   u32                                            │
│ label          char[20]  shown in the menu selector           │
└──────────────────────────────────────────────────────────────┘
  Level[]    : labelStr u32, pageCount u16, firstScreenIndex u16
  Screen[]   : idStr u32, nameStr u32,
               elementCount u16, elementsOffset u32,
               flowCount u16,    flowsOffset u32
  Element[]  : kind u8, align u8, emphasis u8, _pad u8,
               x i16, y i16, width i16, height i16,
               contentStr u32, bindingStr u32
  Flow[]     : triggerKind u8, button u8, gesture u8, _pad u8,
               durationMs u32, targetScreenIndex u16, _pad u16,
               actionStr u32
  Theme      : colorCount u16, ThemeColor[] { keyStr u32, argb u32 },
               typographyBase u16, typographyValue u16, typographyBadge u16
  Strings    : NUL-terminated, deduplicated; every *Str is an offset into this block
```

Two consequences worth stating:

- **`targetScreenIndex` is an index, not a string.** Screen-to-screen links are resolved when
  the pack is built, so the firmware bounds-checks one integer instead of doing a string
  lookup on every button press. Cross-pack references become impossible, which is correct.
- **Strings are deduplicated.** Today's dataset repeats `"< BACK"` and
  `"UP/DN page  ENTER edit  hold ENTER exit"` across many screens.

### 3.3. Validation — the firmware must not trust the card

This is the most important section. Today the exporter runs seven gates and a real compile
before anything reaches the device. **A pack loaded from a card has had none of that
verified on the device it is running on.** A malformed pack is an out-of-bounds read in the
renderer; a hostile one is worse. Validation therefore moves into the firmware and is
mandatory before a single element is drawn.

**Hard checks — any failure rejects the pack and falls back to the embedded default:**

1. File size ≥ 64 and ≤ 65536.
2. `magic` equals `"WFMUI\0"`.
3. `formatVersion` equals the version this firmware implements.
4. `catalogueAbi` is compatible with the firmware's catalogue version (see §7 Q4).
5. `payloadBytes == fileSize - 64`.
6. Recomputed CRC32 matches `crc32`.
7. Every offset and every `offset + count * sizeof(entry)` lies inside the buffer.
8. Every screen's element and flow ranges lie inside the buffer.
9. `stringsOffset + stringsBytes` is inside the buffer **and the final byte is NUL**, so no
   string read can run off the end.
10. Every `*Str` offset is `< stringsBytes`.
11. Every `targetScreenIndex < screenCount`; every `firstScreenIndex + pageCount <= screenCount`.

**Completeness check — the pack loads, and the firmware fills the gaps (§3.0.1):**

11a. Every `category: "setting"` value in the catalogue has a reachable editor in the pack.
     For each that does not, the firmware appends its own built-in editor so the setting
     stays reachable, and logs which ones it patched.

**Soft checks — the pack loads, the specific item degrades, and it is logged:**

12. An `actionStr` that the `UiActionRegistry` does not implement: the flow becomes inert.
    The button does nothing rather than dispatching to nothing dangerous.
13. A `bindingStr` the `UiBindingResolver` does not know: the element renders its static
    `contentStr` instead. This is already the existing behaviour for unknown bindings.
14. An `assetId` the renderer has no handler for: the element is skipped.

The distinction matters. Structural corruption (1–11) can crash the device, so it is fatal.
Vocabulary drift (12–14) is expected as firmware evolves, so it degrades gracefully — the
same posture the renderer already takes.

### 3.4. Menu selection is firmware-owned, never part of a pack

**The selector must not live inside a loadable pack.** If the only way to change packs were
a page inside the active pack, then loading a pack that omits that page would trap the
operator with no way out — the same class of problem as the retired blind factory-reset
combo (`Display_UI_Requirements.md` §3.3).

Therefore: **the firmware appends a "Select Menu" page to the end of the root level**,
whatever the active pack defines. A pack cannot remove it, and it is always reachable by
paging with UP/DOWN.

The page lists, using the ordinary navigation gestures:

```
  Menu:  0  Built-in            (always present, always first)
         1  production          <- active, from /ui/active
         2  commissioning
         3  service
```

UP/DOWN choose; ENTER-short selects. Entry 0 is the embedded default and is always offered,
so returning to a known-good UI never depends on the card. The active entry is marked, read
from the pointer file.

Selecting entry *n* writes that pack's filename to `/ui/active` (entry 0 deletes the pointer
file) and reboots (§3.5).

#### 3.4.1. "Hidden" versus discoverable — and the recovery gesture

The original proposal called this a *hidden* menu. I would argue against hiding it, and
instead split the requirement in two, because "hidden" is bundling together two different
needs:

**It should be discoverable.** A hidden gesture is precisely the anti-pattern we retired with
the blind UP+DOWN factory-reset combo: nothing on screen says it exists, nothing confirms
you are partway through it, and it cannot be documented on the device itself. Hiding also
buys no protection — anyone who can select a menu can already pull the card out. So the
selector is an ordinary page in the firmware-owned root level.

**But there must be a way in when the UI is unusable.** This is the real need behind
"hidden": if a pack validates yet renders nothing legible, paging to a selector page is no
help. That calls for a gesture that does not depend on anything the pack drew:

> **UP + DOWN + ENTER held for 3 s** opens the selector, from any screen, at any level.

All three buttons at once is the only combination still free — UP/DOWN page, UP+DOWN is
display-off, ENTER-short descends, ENTER-long escapes — and it is effectively impossible to
hit by accident. Because the firmware draws the selector itself, it works even if the active
pack draws nothing at all. The anti-boot-loop counter (§3.6) covers the worse case where a
pack crashes before any input is processed.

So: **a normal page for everyday use, and a recovery gesture for when the UI is broken.**
See §7 Q11 if you would still prefer it hidden outright.

### 3.5. Selecting a menu reboots

ENTER-short on a selection writes the choice to NVS and **reboots**.

Reboot is chosen over an in-place hot swap because the active pack's buffer is referenced by
the router, the renderer and the interaction handler simultaneously; swapping it while those
hold offsets into it is a use-after-free waiting to happen. A reboot is ~1 s, and the LED
boot snake (`RGB_LED_Behavior.md` §3.4) already gives the operator a clear "reloading" signal.
Hot swap is noted as a possible later refinement in §7 Q3.

### 3.6. Boot sequence and anti-boot-loop

```
 1. Read loadAttempts from NVS.
 2. If loadAttempts >= 2 -> force Built-in, delete /ui/active if reachable, log.
 3. Mount SD.                     failure -> Built-in
 4. Read /ui/active.              missing/empty/unparseable/has a separator -> Built-in
 5. Open /ui/<name>.              missing -> Built-in (the "dangling symlink" case)
 6. Increment loadAttempts, commit to NVS.
 7. Read into buffer.             larger than 64 KB -> Built-in
 8. Run the §3.3 hard checks.     failure -> Built-in
 9. Run the §3.0.1 completeness check; patch in built-in editors for any gaps.
10. Activate the pack.
11. After the first successful render, clear loadAttempts.
```

Step 2 together with step 11 is the **anti-boot-loop guard**, and it is not optional. A pack
can pass every structural check and still crash the renderer through some combination the
checks do not model. Without the counter that pack stays selected and the device
reboot-loops forever with no way in. Clearing the counter only after a *successful render*
is what makes the guard meaningful — clearing it straight after validation would prove
nothing.

Note that the counter is incremented at step 6, *after* the cheap "is there anything to
load" checks. A missing card or absent pointer file is a normal state, not a failed attempt,
so it must not burn an attempt and eventually delete a perfectly good selection.

The counter is the only thing kept in NVS. The selection itself lives on the card (§3.1.1),
so there is one source of truth: a card moved to another unit carries its menu choice with
it, and a unit with no card simply runs the embedded default.

### 3.7. The embedded default

The default pack stays compiled into the firmware and is the fallback for every failure
path. It should be generated by the **same emitter pipeline** as SD packs so the two cannot
drift — see §4.3.

---

## 4. Firmware Requirements

1. `ui::MenuPack` — an immutable view over a validated buffer, exposing the same shape the
   router and renderer already consume (`screenCount`, `screenAt(i)`, `elementAt(...)`), so
   `UiScreenRouter`, `UiRenderer` and `InteractionHandler` need no knowledge of where the
   data came from. **The embedded default is presented through the same interface**, so there
   is exactly one code path, not a loaded path and a compiled path.
2. `ui::MenuPackLoader` — SD mount, discovery of `/ui/*.uipack`, read into a single heap
   buffer, and the §3.3 validation. Returns a `MenuPack` or a typed failure reason.
3. Every accessor is bounds-checked against the buffer length. No accessor may compute an
   address it has not validated, even on a pack that passed load-time validation — a
   defence-in-depth requirement, because load-time checks are the thing most likely to have a
   gap.
4. One allocation for the pack buffer. No per-screen or per-element allocation.
5. SD access happens **only** at boot. Nothing reads the card during rendering or button
   handling, so removing the card mid-operation cannot fault.
6. The "Select Menu" page (§3.4) is firmware-native and appended to the root level.
7. NVS holds only `ui_pack_try` (u8), the load-attempt counter. The selection is
   `/ui/active` on the card (§3.1.1) — never duplicated into NVS, so the two can never
   disagree.
7a. Writing `/ui/active` is the only write the firmware makes to the card. A partial write
    leaves unparseable content, which falls back to the embedded default rather than
    misbehaving.
7b. The required-editor set for the §3.0.1 completeness check is derived from the firmware's
    own catalogue, not read from the pack, so a pack cannot declare itself complete.
7c. A built-in editor screen per settable value, used both by the embedded default menu and
    as the patch for an incomplete pack. One implementation, two callers.
7d. The UP+DOWN+ENTER 3 s recovery gesture (§3.4.1) opens the firmware-drawn selector from
    any state, without consulting the active pack.
8. Load outcome is reported over Modbus so a supervisory system can see which UI is running —
   see §7 Q7 for whether this warrants registers.
9. Failures are logged with the specific reason, and the reason is shown briefly on screen at
   boot. A silent fallback would leave the operator believing their pack loaded.


### 4.10. SPI arbitration — no artifacts, by construction

Decided 2026-08-03. Implemented in `src/bus/spi_arbiter.{h,cpp}`, 40 host checks.

§2 warns that the card and the LCD share MOSI 8, MISO 9 and SCLK 7 with separate chip selects.
The requirement is **no visible artifacts**, which is strictly stronger than "no corruption", and
that difference decides the design:

> A mutex around each SPI transaction prevents corruption and still permits a **torn frame**. The
> renderer draws a screen as a sequence of transactions, so releasing the bus partway leaves half
> a picture on the panel. Interleaving at transaction granularity cannot be made clean.

The bus is therefore handed over at **frame granularity**, as a state machine rather than a lock:

```
DisplayOwns ──requestCard──► CardRequested ──frame closes──► CardOwns
     ▲                                                          │
     └──── repaint done ──── DisplayResuming ◄──releaseCard─────┘
```

| Rule | Why |
| --- | --- |
| The **display owns the bus by default**; the card asks and waits. | The display is the operator's feedback channel and is never preempted. |
| A grant happens **only** when no frame is open — `startWrite()`/`endWrite()` are the boundary. | This makes a torn frame impossible rather than unlikely. |
| While the card holds the bus the renderer draws **nothing**, not something partial. | A skipped frame is invisible; half a frame is an artifact. |
| On release the display owes **one full repaint**. | What is on the panel describes state from before the handover; an incremental update would leave it. |
| At boot the grant is **immediate** — no frame has ever opened. | Which is why §4.5's "card access at boot only" stays preferred: no contention to arbitrate. |
| A frame open for 500 ms yields anyway. | A wedged renderer must not lock out menu selection forever, and nothing that is not progressing can be torn. |

**The LEDs carry the status while the card holds the bus**, because the display cannot:
`LedOverride::CardBusy`, an amber/blue alternation at 400 ms (`led_patterns.h::cardBusyState`).
Deliberately unlike everything in `RGB_LED_Behavior` §3 — never solid, never accelerating, never a
single channel — so it cannot be mistaken for reset acceptance or a countdown. An operator facing
a dark panel with no other sign of life is one who pulls the card mid-write.

The fallback is unchanged: the embedded default menu of §3.7, which the loader already selects for
every failure rung of §3.6.

---

## 5. Exporter / Translator Requirements

1. New emitter `emitPack(ir)` producing the §3.2 binary, alongside the existing `emitCpp(ir)`.
2. **Both emitters consume the same `ExportIR`.** This is a hard requirement, not a
   convenience: the embedded default and the SD packs must be incapable of describing
   different UIs from the same source.
3. A validation check asserting the two emitters agree on screen count, element count and
   flow count for the same IR. This is the guard that keeps requirement 2 true over time.
4. The exporter stamps `formatVersion` and `catalogueAbi` from the manifest, and writes the
   `label` from a dataset field.
5. The exporter runs the §3.3 hard checks against its own output before writing the file, so
   a pack that would be rejected on-device never reaches a card.
6. CLI: `--emit-pack <path>` and a UI affordance producing a downloadable `.uipack`, since the
   browser cannot write to the card itself. The user copies the downloaded file to `/ui/`.
7. String deduplication when building the string table.
8. **The exporter refuses to write an incomplete pack** (§3.0.1): every settable value in the
   manifest must have a reachable editor, and every action those editors need must be
   referenced. Reported as a named validation check so the failure says which editors are
   missing, not merely that the pack is invalid.
9. The required-action set is derived from the manifest's settable values, so adding a new
   setting to the catalogue automatically makes every existing pack incomplete — which is the
   correct and intended consequence.

---

## 6. Test Considerations

- **Round trip.** A pack built from a dataset, then read back by a host-side reader, must
  reproduce the same screens, elements, flows and theme.
- **Fuzzing the loader is the priority test.** Take a valid pack and, programmatically:
  truncate at every length; flip each header field to 0, 1 and `0xFFFFFFFF`; corrupt the CRC;
  point every offset outside the buffer; strip the string table's terminating NUL. Every case
  must reject cleanly and fall back — no crash, no out-of-bounds read. This is testable on the
  host without hardware and should gate the feature.
- **Anti-boot-loop.** A pack that validates but faults during render must fall back to the
  embedded default by the third boot.
- **Card absent, card removed mid-run, empty `/ui/`, 9+ packs present, non-`.uipack` files.**
- **Vocabulary drift.** A pack referencing an action and a binding the firmware does not
  implement must load, degrade those two items, and log — not fail.
- **Memory.** Confirm the loaded pack's actual footprint against the 64 KB cap, and that
  repeated selection and reboot does not leak.

---

## 7. Open Questions

**Accepted 2026-07-30: all recommendations below are adopted.** They are retained with their
reasoning rather than collapsed into bare statements, because the reasoning is what a future
reader needs. Summary of what that settles:

| Q | Decision |
| --- | --- |
| 1 | SD for v1, loader source abstracted (`MenuPackSource`) so flash can be added later |
| 2 | Uncompressed; the version field allows compression later |
| 3 | Selection reboots rather than hot-swapping |
| 4 | Accept `packAbi <= firmwareAbi`, and **the catalogue is append-only** — a project rule |
| 5 | Packs can never add values or actions |
| 6 | Gesture semantics stay firmware-owned and absent from packs |
| 7 | One Modbus register for load status and pack ABI |
| 8 | Header `label`, falling back to the filename |
| 9 | Implement after the navigation stack and after D2 |
| 10 | Strict completeness by default; a `restricted` flag if the kiosk case is wanted |
| 11 | Discoverable page **plus** the UP+DOWN+ENTER recovery gesture |
| 12 | The pointer file is explicitly human-writable |

> **Q4 creates a standing project rule worth repeating outside this document: the value and
> action catalogue is append-only.** Renaming or removing a catalogue entry silently breaks
> every menu pack authored against it, and unlike a compile error that breakage only appears
> on a device with a card in it.

1. **SD only, or also a flash partition?** SD matches the stated intent and is
   user-swappable. But there is currently **no data partition** in the flash layout, so a
   flash-resident pack store would need a custom partition table — and it would let a pack be
   uploaded over serial or Modbus with no card at all, which is useful for a DIN-rail device
   in a cabinet. *Recommendation: SD for v1, design the loader so its source is abstracted
   (`MenuPackSource`) so flash can be added without touching the format.*
2. **Does "expanded by the firmware" mean compressed?** A ~13 KB pack compresses to perhaps
   6 KB, saving little while adding a decompressor to the most safety-critical path in the
   feature. *Recommendation: uncompressed for v1. The format has a version field, so
   compression can arrive later without breaking anything.*
3. **Reboot on selection, or hot swap?** §3.5 argues for reboot. *Recommendation: reboot.*
4. **How strict is the catalogue ABI?** Exact match is safest but means every firmware
   release invalidates every pack. A monotonic version where the firmware accepts
   `packAbi <= firmwareAbi` works only if the catalogue is strictly additive — values and
   actions may be added, never removed or renamed. *Recommendation: accept `packAbi <=
   firmwareAbi`, and make "the catalogue is append-only" an explicit project rule. This is
   where **D2** matters: the firmware must know its own catalogue version, which means
   generating the manifest from firmware rather than maintaining it by hand.*
5. **Confirm: a pack can never add values or actions?** I have assumed yes throughout — this
   is what "keeping the values constant" implies, and it is what makes validation tractable.
6. **Confirm: gesture semantics stay firmware-owned and absent from the pack?** Also assumed
   yes. It means muscle memory transfers between menus, and it removes a whole category of
   pack-induced unusability. Worth being explicit, because it does mean a pack cannot
   redefine what ENTER-long does.
7. **Should the active pack be visible over Modbus?** e.g. a register holding the label and
   the load outcome, so a SCADA system can tell which UI an operator is looking at.
   *Recommendation: yes, one register for load status and the pack's ABI — cheap, and useful
   when diagnosing "the operator says the screen shows X".*
8. **What labels the pack in the selector — the header `label` or the filename?** *Recommendation:
   the header `label`, falling back to the filename when it is empty, so a pack renamed on the
   card still identifies itself.*
9. **Sequencing.** This feature serialises the navigation tree, so the tree must exist in
   firmware first. *Recommendation: implement after the navigation stack and after D2.
   Designing the format now is right; building it before the thing it serialises works is
   not.*

---

10. **Does the completeness rule forbid a deliberately reduced menu?** A read-only operator
    or kiosk menu that intentionally hides configuration is a reasonable thing to want, and
    §3.0.1 as written forbids it. *Recommendation: keep strict completeness as the default,
    and if you want the kiosk case, add a `restricted` flag to the header. A restricted pack
    may omit editors, and the firmware then appends its own built-in config level rather than
    individual editors — so settings stay reachable without the pack having to carry them.
    The invariant to protect is "no setting is ever unreachable", not "every pack must
    contain every screen".*
11. **Selector discoverable, or genuinely hidden?** §3.4.1 argues for a normal page plus a
    UP+DOWN+ENTER recovery gesture, on the grounds that hiding repeats the blind-combo
    anti-pattern and protects nothing when the card is physically accessible anyway.
    *Recommendation: as written. If you want it hidden, drop the root-level page and keep
    only the gesture — but then it must be documented outside the device, because nothing on
    screen will ever mention it.*
12. **Should the pointer file be human-written too?** As specified, a person can create
    `/ui/active` in a text editor to preselect a menu. *Recommendation: yes, explicitly
    support it — it is the main advantage of keeping the selection on the card, and it costs
    nothing beyond tolerating trailing whitespace and a trailing newline.*

---

## 8. Risks

| Risk | Mitigation |
| --- | --- |
| Every compile-time gate we built stops protecting the loaded path. | Validation moves into the firmware (§3.3) and the exporter runs the same checks before writing (§5.5). The fuzz suite (§6) is the real guard. |
| A pack that validates but crashes the renderer bricks the UI. | Anti-boot-loop counter cleared only after a successful render (§3.6). |
| The embedded default and SD packs drift apart. | Both from one IR, with a check asserting they agree (§5.2–5.3). |
| Format churn invalidates packs users have already made. | `formatVersion` is checked and refused rather than misread; append-only catalogue rule (§7 Q4). |
| Scope creep into assets, compression, hot swap. | Explicitly out of scope for v1: bitmap/SVG assets inside packs, compression, hot swap, and any pack-defined values, actions or gestures. |
| A pack omits an editor, stranding a setting. | The completeness rule (§3.0.1): a hard failure at export, and at load the firmware patches in its own editor for each gap. |
| The firmware writes to the card and a power loss corrupts the pointer. | Unparseable content falls back to the embedded default. The pointer is one short line and is the only write the firmware makes. |
