# Requirement: Loadable UI Menu Packs

**Version:** 0.1 (proposal — open questions in §7 need answers before implementation)
**Date:** 2026-07-30
**Depends on:** the navigation model in `Display_UI_Requirements.md` §5, and decision **D2**
(generate the manifest from firmware) in `docs/active_work/open_decisions.md`

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
| NVS (`Preferences`, namespace `flow-data`) | — | Stores the selected pack name and the load-attempt counter (§3.6). |

---

## 3. Behaviour Specification

### 3.1. The container format — one file per menu

**A menu is a single file, not a directory.** `/ui/<name>.uipack` on the card.

A single file is chosen deliberately over a directory tree:

- **Atomicity.** A half-copied file fails its CRC and is rejected. A half-copied *directory*
  looks structurally valid and fails unpredictably at render time.
- **One read.** No directory walking on an embedded filesystem, no per-file open cost.
- **User-manageable.** Copying one file onto a card is a thing a person can do correctly.
- **Versionable and checksummable** as a unit.

The directory then only *organises* packs:

```
/ui/
  production.uipack
  commissioning.uipack
  service.uipack
```

Discovery lists `/ui/*.uipack`, sorted by filename, capped at **8** entries.

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
         1  production
         2  commissioning
         3  service
```

UP/DOWN choose; ENTER-short selects. Entry 0 is the embedded default and is always offered,
so returning to a known-good UI never depends on the card.

### 3.5. Selecting a menu reboots

ENTER-short on a selection writes the choice to NVS and **reboots**.

Reboot is chosen over an in-place hot swap because the active pack's buffer is referenced by
the router, the renderer and the interaction handler simultaneously; swapping it while those
hold offsets into it is a use-after-free waiting to happen. A reboot is ~1 s, and the LED
boot snake (`RGB_LED_Behavior.md` §3.4) already gives the operator a clear "reloading" signal.
Hot swap is noted as a possible later refinement in §7 Q3.

### 3.6. Boot sequence and anti-boot-loop

```
1. Read selectedPack + loadAttempts from NVS.
2. If loadAttempts >= 2  -> force Built-in, clear selectedPack, log. (see below)
3. If selectedPack empty -> Built-in.
4. Increment loadAttempts, commit to NVS.
5. Mount SD.            failure -> Built-in (keep selection; the card may return)
6. Open the file.       failure -> Built-in (keep selection)
7. Read into buffer.    too large -> Built-in + clear selection
8. Run §3.3 checks 1-11. failure -> Built-in + clear selection
9. Activate the pack.
10. After the first successful render, clear loadAttempts.
```

Step 2 with step 10 is the **anti-boot-loop guard**, and it is not optional. A pack can pass
every structural check and still crash the renderer through some combination the checks do
not model. Without the counter, that pack is selected in NVS and the device reboot-loops
forever with no way in. Clearing the counter only after a *successful render* is what makes
the guard meaningful — clearing it right after validation would prove nothing.

Note the asymmetry in steps 5–8: a missing card keeps the selection (the card may be
reinserted), while a corrupt pack clears it (it will never become valid on its own).

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
7. NVS keys in the existing namespace: `ui_pack` (string) and `ui_pack_try` (u8).
8. Load outcome is reported over Modbus so a supervisory system can see which UI is running —
   see §7 Q7 for whether this warrants registers.
9. Failures are logged with the specific reason, and the reason is shown briefly on screen at
   boot. A silent fallback would leave the operator believing their pack loaded.

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

These need answers before implementation. My recommendation is given for each.

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

## 8. Risks

| Risk | Mitigation |
| --- | --- |
| Every compile-time gate we built stops protecting the loaded path. | Validation moves into the firmware (§3.3) and the exporter runs the same checks before writing (§5.5). The fuzz suite (§6) is the real guard. |
| A pack that validates but crashes the renderer bricks the UI. | Anti-boot-loop counter cleared only after a successful render (§3.6). |
| The embedded default and SD packs drift apart. | Both from one IR, with a check asserting they agree (§5.2–5.3). |
| Format churn invalidates packs users have already made. | `formatVersion` is checked and refused rather than misread; append-only catalogue rule (§7 Q4). |
| Scope creep into assets, compression, hot swap. | Explicitly out of scope for v1: bitmap/SVG assets inside packs, compression, hot swap, and any pack-defined values, actions or gestures. |
