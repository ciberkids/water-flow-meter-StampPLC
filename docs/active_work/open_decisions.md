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

**This file holds nine items** — seven open plus the two standing rules. It was 287 lines and six items
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

**Seven open lines: two features, three defects and two measurements.** `DF28` and `DF29` opened on
2026-10-29, when writing the wiki's Features and Capabilities page showed the configuration web page is
never started and the board temperature never measured; both closed the same day. `DF30` opened in
`DF29`'s review, when checking that the *Board temperature* entity could appear at all showed that Home
Assistant discovery overflowed the publish queue, and closed on 2026-10-30. `DF31` opened the same day out
of `DF30`'s design study, which read what esp-mqtt does with a message after the firmware hands it over.
`N-g` opened on 2026-10-01, when documenting the SD card showed a pack could not be built, and closed on
2026-10-29: `npm run export:pack` builds one and refuses a pack the device would not load. `N-f` and `DF25` opened on
2026-08-30, both out of one question the owner asked while T1's storage was being decided — *is the
right information surviving a power cycle, and are we burning through the write cycles?* That is the
usual way: `DF23` came from checking whether `G1`'s procedure could be followed, and `DF22` from
building `N-c`. `J9`, `I2a`, `N-b` and N-d2's correction half all closed on 2026-08-21; `N-c` and
`DF22` closed on 2026-08-20, and `DF22` was the only 🔴 the board has had since the register was
rewritten.

**What remains is five software items and three measurements.** `N-e` is under way — T0, T1 and T2 have
landed and T3/T4/T6 are next — `N-f`, `DF23` and `DF25` each carry a decision in the entry, and `DF31`
carries a likely fix that should not be decided before a board has measured it, so its next step waits
on hardware too. The three measurements that need hardware are `G1` (the polling rate),
`N-d2`'s remaining half (whether the RTC survives power loss), and `N-c`'s retain check, which lives in that entry's ⏸️ block rather than on a line of its own
because the rest of `N-c` is built and tested. Cite an ID when you ask about one — this file is the one
place that says what an ID means. Rule **I3** governs them: append-only, never reused, so a gap means an
item closed, not an item lost.

**`N-c` stays in this file rather than moving to the archive**, because one thing in it is verified by
reading only: `MQTT_EVENT_DATA`'s retain flag, which R4.4.2c depends on entirely and which no host test
can reach. Its ⏸️ block carries the one-command bench check and says what a wrong answer costs. With
`DF31`'s measurements that makes **four** things waiting on the board, not two.

The **Shape** column is the one that answers *can I just say go ahead?*

| ID | | Shape | What it is |
| --- | --- | --- | --- |
| **N-e** | 🟡 | feature, in progress | Sensor cascade topology — a parent per channel, a total that is the sum of roots, and verification against a commissioned baseline. Six decisions taken, four questions open; T0, T1 and T2 landed, **T3/T4/T6 next**, and R2.5's MQTT sentence needs amending |
| **N-f** | 🟡 | feature, specified here | Where the readings live — internal flash or an SD card, human-readable files, persisted session values and rotating telemetry. Five decisions taken, six questions open, not started; question 1 changes a documented register contract and should be answered first |
| **DF31** | 🟡 | defect, found 2026-10-30 | What esp-mqtt does after the hand-over, read in its source: telemetry at QoS (quality of service) 0 drains at about one message a second, the second QoS 0 message of a burst goes out last and can expire, every item expires silently 30 s after it was enqueued, and an enqueue or a subscribe can hold the logic task for as long as esp-mqtt's loop body holds its lock — while connected, a large inbound message's reads, a PUBACK, a resend or retransmit, or a keepalive ping when no write has happened for half the keepalive, each read or send bounded by the 10 s network timeout, and during a connect attempt a name lookup that timeout does not bound — and a subscribe then writes its own packet (read from the source, not measured). Telemetry at QoS 1, already a setting, would avoid the pacing and the misfire by the source's reading; measure on a board, then decide |
| **DF25** | 🟡 | defect, found 2026-08-30 | A write to non-volatile storage that fails is reported by nothing. **Detection and publication landed 2026-08-30** (`DF25a`: register 34, MQTT `storageFault`, 49 host checks) — the panel's warning triangle rides with `N-e`'s T5, and three of its four questions were built on the recommendation rather than decided |
| **DF23** | 🟡 | defect, found 2026-08-21 | `baselineKhz` is published as `0.000` — R2.1.2's radio-off baseline is recorded by nothing, so R2.1.1's 5 % test and half of G1's procedure have no reference |
| **G1** | ⏸️ | measurement | The 3.3 kHz polling rate has never been measured on a board; the procedure is written down and waiting |
| **N-d2** | ⏸️ | measurement | Whether the RTC survives power loss is unknown — the correction half (a gate protecting the VLF probe's position) landed 2026-08-21 |

**DF1–DF22 and J1–J8** are fixed and keep their IDs — `DF1`–`DF21` and `J1`–`J8` moved to
[`../archive/open_decisions-closed-2026-08-18.md`](../archive/open_decisions-closed-2026-08-18.md), verbatim,
because I3 makes them append-only and a retired id must still resolve. **I2** and **I3** are standing rules
that never close.

**What this list is NOT.** Nothing here is blocking a build, a test or an export. Measured 2026-08-30
after T2 and its review: host **2,197 checks across 29 suites**, 0 failures, the register-reference gate green
(and shown to fail on a removed code description), and a firmware that compiles in the container at RAM
24.7% / Flash 39.0% — 81,040 and 1,303,457 bytes against T1's 81,008 and 1,301,925, so DF25a with both
review rounds costs **32 bytes of RAM and 1,532 of flash**. NOT re-run on 2026-08-30 and therefore quoted as the 2026-08-26
figures: 220 unit, 51 exporter, 51 visual, 0 audit findings. (Both compile figures come from a CLEAN dependency
cache; the earlier 38.2% came from a stale container, see `platformio.ini`.) Of the seven open lines one
feature is under way, one has not started, two defects have decisions in them, one defect has a likely
fix that waits on a board measurement, and two need hardware that has never existed for this project. That is a different condition from "twelve things are broken", which
is what this register looked like before the 2026-08-18 rewrite.

---

## ~~N-g~~ ✅ FIXED 2026-10-29 — `npm run export:pack` builds a menu pack, and refuses one the device would not load

Opened 2026-10-01, while answering whether the wiki explains the SD card. It now does (the generated
**SD Card** page: `/ui/<name>.uipack`, the `/ui/active` pointer, the 64 KB and 64-character limits, read
from `ui_pack_storage_sd.h` and `ui_pack_loader.h`). But the firmware half is the only half that exists:

- `web/mockup/tools/exporter/packEmitter.ts` writes the `.uipack` format, and its sole caller is
  `__tests__/pack_emit.test.ts`, which writes `tests/fixtures/default.uipack` for the host round-trip test.
- No npm script, exporter flag or designer button produces a pack, so nobody can put a menu on a card.

**~~To finish~~ — FIXED 2026-10-29, 16 exporter tests and one CI step.** `npm run export:pack` runs
`cli.ts --emit-pack web/mockup/dist-pack/default.uipack`, the flag `Loadable_UI_Menu_Packs.md` §5.6 names.
`--screens` and `--manifest` override the inputs exactly as for `export:firmware`, a second `--emit-pack`
after `--` replaces the default path (`parseArgs` keeps the last value), `--pack-label` sets the header
label, and `--dry-run` checks without writing. Paths resolve from the repository root, like every other
flag of that CLI. `dist-pack/` is gitignored; the report names the file and where it goes on the card.

**One sequence of gates, not two.** The dataset gates moved out of `cli.ts` into
`tools/exporter/exportGates.ts`, and both commands call it: after the schema, the LED legend, the
hold-to-confirm screen, the diagnostics banner (the standing warning), renderable kinds, ring closure, the
three manifest coverage checks and the two firmware-resolver checks — the same ten as before, shown by
re-running `export:firmware` end to end after the move (9 pass, 1 warning, the container compile passing,
`src/ui/generated` identical bar its timestamps). Then five pack
checks in `tools/exporter/packRules.ts`, decided in the pure `packExport.ts::buildPack`, which withholds the
bytes on any failure so a caller cannot write them by forgetting to look at the status:

| Check | Refuses |
| --- | --- |
| `pack-file-name` | a name not ending in `kPackSuffix` (compared case-sensitively, as `listPacks` does), longer than `kMaxNameBytes - 1`, starting with `.`, or using anything but ASCII letters, digits, `.`, `-`, `_` |
| `pack-label` | an explicit `--pack-label` over `PackHeader::kLabelBytes` or outside printable ASCII — refused, not truncated. The label derived from the file name is cut to fit instead (review, below) |
| `pack-size` | more than `kMaxPackBytes`; exactly at it passes, matching the loader's `>` |
| `pack-catalogue-abi` | a manifest with no `catalogueAbi`, which the schema allows and a pack cannot be stamped without |
| `pack-flow-targets` | a flow whose `targetScreenId` names no screen |

**The limits are read, never typed.** `readFirmwarePackLimits` reads `kMaxPackBytes` and `kMaxNameBytes`
from `ui_pack_loader.h`, `kDirectory` and `kPackSuffix` from `ui_pack_storage_sd.h` and `kLabelBytes` from
`ui_pack.h` on every run. It understands a string literal and a product of integer literals (`64 * 1024`),
ignores commented-out definitions, and throws naming the header for anything else, a missing constant or a
duplicate, so a changed header stops the export instead of falling back to a remembered number.
`kPackSuffix` is NEW: the suffix was a literal inside `listPacks` in `ui_pack_storage_sd.cpp`, which no
reader outside that file could see, so it moved into the header and `listPacks` uses it. The wiki's SD
Card page reads it too, and now also `kMaxEntries` from `ui_pack_selector.h`. That corrected the page's
"as many packs as you like" to the seven card packs the Select Menu actually lists.

**Two things found on the way.**

1. *An unresolved flow target was a silent dead button.* The emitter writes `NO_TARGET_SCREEN` for a
   `targetScreenId` it cannot find, and its comment said the screen-coverage gate catches that first. It
   does not: that gate checks the screens the firmware requires, and ring closure covers UP and DOWN only.
   A test changes an ENTER target to a name that does not exist: every `export:firmware` gate passes, and
   `pack-flow-targets` alone refuses it. The C++ tables carry the same typo as a string the router fails
   to resolve, so `export:firmware` has the same gap. It is left as it was, because changing that
   command's gate set was not this item.
2. *The fixture was the library's output, not the command's.* The host `pack_test` and `pack_loader_test`
   read `tests/fixtures/default.uipack`, which the test wrote with `emitPack` directly. Now
   `pack_emit.test.ts` writes it by running the CLI, and is its only writer, because `node --test` runs
   files in parallel. It also asserts the bytes equal `emitPack`'s. So the pack the C++ validates is the
   command's output.

**What changed in the fixture, and why.** With no `--pack-label`, the label is the file name minus
`.uipack`: `default`, where the hand-written test passed `Default`. Before the default was switched, the
command with `--pack-label Default` reproduced the committed fixture byte for byte (`cmp`). After it, the
fixture differs from the previous one in exactly six bytes: the four CRC bytes and the label's first letter,
in the header and in the string table. `pack_test`'s label check expects `default`.

**CI now gates freshness.** The web job's new step runs `npm run export:pack` as an author would, `cmp`s
the result with `tests/fixtures/default.uipack`, and fails on a git diff of the fixture. The firmware-host
job reads the committed fixture without running npm, so until now a dataset change committed without its
pack reached it stale (the DF19 shape). Checked locally by putting the previous fixture back: `cmp` exits 1.

**Not part of this, said so where a reader looks:**

- **§5.8's completeness check is not run.** `export:firmware` does not run it either. The shipped dataset
  satisfies it because `tools/skeleton/generate.mjs` refuses otherwise, but a custom `--screens` is not
  held to it. The SD Card page, `web/mockup/README.md` and the wiki's Features page all say so. It is
  N-b's unbuilt neighbour, with §3.3.11a's load-time repair.
- **§5.5's hard checks are not reimplemented in TypeScript.** A second validator would be a second
  implementation of the format. The host round trip over this command's own output is the check instead.
- **No designer button** (§5.6's UI half, optional here). `packEmitter.ts` uses Node's `Buffer` and the
  gates read firmware source from disk, so a browser button needs either a port or a dev-server endpoint,
  and CI does not run the Playwright suite that could test it.
- The file name rule is stricter than the device on purpose; see `kPortableNameCharacter`.

**Checks, 2026-10-29:** `npx tsc --noEmit` clean; `npm run test:unit` 220 passed; `npm run test:exporter`
**67** passed (51 before: 15 new in `pack_export.test.ts`, 1 in `pack_emit.test.ts`); the host suite, 30
suites each ALL PASSED and exit 0; `npm run export:pack` on the shipped dataset `ok-with-warnings` (the
standing banner warning) and byte-identical to the fixture; `npm run export:firmware` as above;
`tools/wiki/sync.sh` dry run ending "all repo pointers resolve".

**Cost: 0 bytes of firmware**, and 4 bytes of flash after the review below. Container build before and
after `kPackSuffix`: RAM 81,096 and flash 1,319,301 bytes both times (24.7% / 39.5%). The shipped pack is
**27,349 bytes for 79 screens**, against the 65,536 `kMaxPackBytes` allows.

**Review, 2026-10-29: eight findings, seven fixed on the branch and one relayed** (the index row and the
count paragraphs, which this branch deliberately does not edit).

1. *The pack's theme was not the compiled-in table's.* The emitter read `dataset.theme` and converted
   each colour with a hex-only parser of its own. So `--theme` was accepted and ignored: a pack built with
   an override was byte-identical to one built without it. And with no `--theme` at all, the shipped
   `gridMinor` and `gridMajor`, `rgba(124, 162, 206, 0.28)` and `0.55`, were written as opaque black,
   `0xFF000000`, where `GeneratedUi.cpp` holds `0x477CA2CE` and `0x8C7CA2CE`. The emitter now takes the
   theme from the `ExportIR` the gates build, the one `cppEmitter` writes `kThemeColors` from, as a
   required option. §5.2's "both emitters consume the same `ExportIR`" now holds **for the theme block
   only**: screens, elements and flows are still read from the dataset. `ui_pack.cpp` bounds-checks
   `themeOffset` and reads nothing behind it yet, so the wrong colours were never drawn. The fixture
   changed in 12 bytes (`cmp -l`): the 4 CRC bytes and 4 for each grid colour. It is still 27,349 bytes.
   A test compares every colour of the command's pack with `GeneratedUi.cpp`'s table, and another builds
   with `--theme` and reads that file's colours back out of the bytes.
2. *A long file name was refused for its label.* With no `--pack-label`, the label was the whole stem,
   so a stem longer than `kLabelBytes` was refused although the device lists, loads and shows the file
   (the Select Menu shows the file name). `service-menu-for-site-b.uipack` could not be built. The
   derived label is now cut to fit, which is safe because `pack-file-name` holds names to ASCII. An
   explicit `--pack-label` that does not fit is still refused. Nothing is emitted while the label fails,
   so the size and flow-target checks wait for a label that fits rather than measuring another one.
3. *The label field had a second, typed home.* `pack-label` read `kLabelBytes` from `ui_pack.h`, and the
   emitter wrote with its own `20` and `.slice()`, so a larger header value would have been accepted and
   then cut without a word. The field size is now `packEmitter.ts::PACK_LABEL_BYTES`, typed because it is
   layout like the header size, and `buildPack` stops when it differs from the header's. `emitPack` throws
   instead of cutting, including on a label with fewer characters than the field but more bytes, which
   `.slice()` counted in UTF-16 units and `write` then ran past offset 62.
4. *No test held the limits to the headers.* Typing all five back into `readFirmwarePackLimits` left the
   suite green. A test now writes three fake headers whose values differ from the real ones and asserts
   the reader returns exactly those; a missing header, or a missing constant, throws naming its
   repository-relative path.
5. *A leading zero was read as decimal.* `readHeaderConstant` returned 100 for `0100`, which C++ reads as
   octal 64. Only `0` or a number not starting with `0` is accepted now; anything else throws.
6. *The Select Menu's listing rule could not be tested.* It was an expression inside `listPacks`, behind
   `SD.h`, while the exporter copies it. It is now `SdPackStorage::isListablePackName` in
   `ui_pack_storage_sd.h`, a pure move (a bare `.uipack` is still listed), driven by the new host
   `pack_listing_test` (16 checks), which also reads `ui_pack_storage_sd.cpp` as text: `listPacks` must
   call it and compare no suffix itself. The two `char path[96]` buffers are `kPathBytes`, with a
   `static_assert` that it holds the directory, a separator and `kMaxNameBytes`.
7. *`UI_Dataset_Contract.md`'s pipeline diagram* had an unfinished line; it now reads "read by host tests".

Each behavioural fix was reverted once to see its test fail: each of the seven exporter reversions failed
one to three tests, and each of the four firmware ones failed a host check or the build.

**Checks after the review:** `npx tsc --noEmit` clean; `npm run test:unit` 220 passed;
`npm run test:exporter` **77** passed (67 before: 9 new in `pack_export.test.ts`, 1 in
`pack_emit.test.ts`); the host suite, 31 suites each ALL PASSED and exit 0; `npm run export:pack`
byte-identical to the regenerated fixture; `tools/wiki/sync.sh` dry run ending "all repo pointers
resolve". Container build: RAM 81,096 and flash 1,319,305 bytes, against 81,096 and 1,319,301 for the
branch before the review, built the same way on the same day.

**⏸️ Not seen on a board**, like everything else on this device. The first proof is `default.uipack`
copied to `/ui/` with `/ui/active` naming it, and the panel booting into it with no "menu rejected".

Not part of this item: the SD-card WiFi/MQTT credential file, which is `N8b` in `WiFi_MQTT_Connectivity.md`
and still unbuilt. The wiki page says so.

---

## ~~DF28~~ ✅ FIXED 2026-10-29 — the configuration web page is served while the access point is up

Found 2026-10-29 while writing the wiki's **Features and Capabilities** page, by checking that the page
it was about to describe could actually be reached.

**What is missing.** `firmware.cpp` constructs `plc::ArduinoPortalServer portalServer(portalForm)` and
never touches it again. `ArduinoPortalServer` has `begin(apIp)`, `update()` and `end()`
(`src/net/portal_server_arduino.h`), and no file calls any of them. `git log -S"portalServer"` returns
one commit, `31b22383`, whose message says the portal "is served now, but only while the provisioning AP
is up" — and whose diff adds the include and the two declarations and nothing else. So the code that
runs it in `WifiState::ApPortal` was described and never committed.

**What a user sees.** Enable WiFi on a device with no credentials: the access point
`water_flow_meter_<6 digits>` comes up for its ten minutes, a phone joins it, and nothing answers on
port 80 and no captive-portal DNS reply is sent, because the DNS responder lives in the same unstarted
object. Everything `PortalForm` does — the form generated from the settings catalogue, Basic auth, the
clock route through `FirmwarePortalClock` — is unreachable. The login reset at register `710` still
works, because it is a Modbus write; it resets a login nobody can reach.

**Why nothing caught it.** The same blind spot `portal_server_arduino.h` names twice: `PortalForm`'s
host checks exercise the form against strings, and the adapter that would serve it is the part no host
test can construct. `ce67ef8f` fixed the adapter's `/save` routing on 2026-08-14 without noticing that
the adapter itself never runs.

**How the docs were handled.** The wiki's WiFi page had described the page as working, with one routing
defect. While this entry was open it said the page was not served and pointed at Modbus; the fix below
changed it to say the page is served and has not been seen working on a board.

**~~To fix~~ — FIXED 2026-10-29, in two rounds.** The first round started the page. An independent review of
that round found the page could be loaded and could still not do its job, for three older reasons that had
been unreachable while nothing served it. They are lettered here rather than given IDs of their own,
because each is a part of "the configuration page does not work" and the item is not fixed without all
four (the `DF25a` precedent for lead-ins):

- **DF28a — never started.** The rule is `src/net/portal_lifecycle.h`: `plc::drivePortal(server,
  apPortalActive, apIp)` starts the page when the state machine is in `ApPortal` and the access point has
  an address, services it on every pass, and stops it when either goes away. `firmware.cpp` calls it once
  per logic pass, after the provisioning notice. Never on the station interface, because the DNS
  responder answers every name with the device's address. A start that reports failure is retried on the
  next pass; the address-0 guard is only a guard, because `WifiManager` reads the address once.
- **DF28b — the body was never read.** `handlePost` read `arg("plain")`, which WebServer creates only for a
  body that is NOT urlencoded, and a form with no enctype is urlencoded. So every submission reached
  `submit()` as an empty string and the page answered "Nothing changed". Both POST routes now carry a
  second callback, which makes WebServer stream the body raw to `receiveBody()`; it is buffered up to
  `PortalForm::kMaxSubmitBytes` and handed to `submit()`, whose own decoder is the tested one. A body over
  the bound, or carrying a NUL, is refused whole with an error page, never truncated.
- **DF28c — a successful submission reached the radio ten minutes late.** `netSettingsNotifiedRevision =
  netSettings.revision();` sat inside the logic loop's save-success branch since `6c4b8009`, a seed meant
  for `setup()`. The page applies a submission between the notify check and the save, so that line marked
  every submission as already notified, and the access point stayed up for the rest of its ten-minute
  window; only when it expired did the device join the network it had been given. The seed is in
  `setup()` now.
- **DF28d — every submission reported errors.** A disabled checkbox's hidden "0" companion was not
  disabled with it. On the device, which builds the form with no settings store, every per-sensor row is
  read-only, so a browser posted eight `config.sensor.connected@n=0` fields that `submit()` refused, and
  the operator was told "Partly saved ... the device refused these", with eight rows they could not edit,
  after a save that had worked. The companion is disabled with its checkbox now.

**Also from the review.** `ArduinoPortalServer::begin()` returned true on every path, because
`WebServer::begin()` is void and a failed listen is silent; `ListeningWebServer` reads the protected
socket's `_listening` so a failed start is reported. A failed DNS responder no longer fails the page and
is retried from `update()`. The routes are registered once per object, because `WebServer::stop()` does
not clear the handler list and every reopened window would have leaked three handlers. The comment that
said `header("Authorization")` is always empty without `collectHeaders()` was false for the pinned core,
which always collects it; the call stays as belt and braces and says so.

**A third round, from reviewing the second.** The second callback that makes WebServer stream the body is
ALSO its upload callback, so a `multipart/` POST called `receiveBody()` with no raw buffer, and
dereferencing it rebooted the device — before any login check, for anyone in radio range of the access
point. `ListeningWebServer::rawActive()` is asked first now; such a body is not collected, and the
request is answered 400 without leaving any state behind for the next one. The same review
found the body's flags survived from one request to the next, that a failed `String::concat` could drop
a chunk from the middle of a body still submitted as complete, and that a DNS responder that kept failing
was retried, and logged, on every logic pass. The body is now one allocation of the declared
`Content-Length`, refused if over the bound, explicitly terminated and released with its flags on every
path; the DNS retry waits five seconds. One cost is accepted and documented in the adapter: WebServer
asks for a full chunk however few bytes remain, so every save holds the logic task about a second longer
(display and MQTT pause; the sampler and Modbus do not). The review also corrected two descriptions here:
`DF28c`'s device did join, ten minutes late, and `DF28d`'s page said "Partly saved", not "Nothing was
saved". And a comment beside DF28d's fix claimed a boolean's two inputs could come in either order; the
last one wins, so the hidden "0" must come first, and a test now holds that.

**The checks, and that they fire.** `test/host/portal_lifecycle_test.cpp` drives the rule against a fake
server and reads `firmware.cpp` as text, comments blanked: the call must be there exactly once, after
`wifiManager.update(`, with the `ApPortal` condition and the access point's address as its arguments, no
other `portalServer.begin/end/update` beside it, and `netSettingsNotifiedRevision` assigned only inside
the notify check and inside `setup()`'s braces — every assignment counted, whatever its right-hand side or
spacing, after reviews showed an exact-match version passed two rewordings of the stray line and a
later version passed it behind an `else`. Removing
the call fails it; restoring the stray line, any of those rewordings, or a seed moved into `loop()` fails
it. The
source-text helpers it shares with `boot_order_test.cpp` moved to `test/host/firmware_source_text.h`.
`portal_form_test.cpp` gained a browser: it renders the page, submits it the way HTML says a browser
does, and requires `submit()` to accept it on the device's own construction; that check failed before
DF28d's fix. It also requires a checked box to survive the round trip, which fails if the hidden input
and the checkbox swap places. The same pass derives the largest legal body from the page, with and without a settings
store, and fails if `kMaxSubmitBytes` would not hold it.

**Not reachable by any host check**, stated so it is not overclaimed: DF28b lives in the Arduino adapter.
It is verified against the framework source (`WebServer/src/Parsing.cpp`, `detail/RequestHandlersImpl.h`)
and by compiling, not by running.

**Cost**, measured by CI on the pinned espressif32 7.1.3: the first round took flash from 1,304,961 to
1,319,861 bytes (+14,900; the web server and DNS responder were dead code until something called them),
with static RAM unchanged at 81,096 bytes. The second round added 1,144 bytes of flash and 24 of static
RAM (1,321,005 and 81,120 on CI): the body buffer's bookkeeping lives in the global server object.
The third round, measured in the build container against the second round built there (CI and the
container differ by a few hundred bytes of embedded paths, so only same-machine deltas are compared):
8 more bytes of static RAM and 272 fewer of flash, the `String` growth code having gone.

**⏸️ Not seen on a board**, like everything else on this device. The bench check is the whole chain: a
phone joins `water_flow_meter_<6 digits>` and gets the login page, the WiFi credentials are saved with a
"Saved" message, the access point closes within a pass, and register `501` goes to `2` then `3`. If any
step fails, Modbus provisioning is unaffected.

---

## ~~DF29~~ ✅ FIXED 2026-10-29 — the board temperature is read, and published as `null` when there is no reading to trust

Found 2026-10-29, in the same pass as `DF28`. The same shape as `DF23`: **a published field nobody
assigns**.

**What was missing.** `MqttPublisher`'s diagnostics struct had `float boardTemperatureC = 0.0f`
(`src/net/mqtt_publisher.h`), the payload prints it as `tempC`, and `ha_discovery.cpp` announces a
*Board temperature* entity with device class `temperature` and unit °C. Nothing assigned the field. So a
real device published `"tempC":0.0` on every diagnostics message, and Home Assistant drew a flat line at
0 °C — a plausible reading, which is worse than an empty one. The MQTT wiki page's example showed `41.2`,
the same invented-figure pattern that page already confesses for `baselineKhz`.

**The sensor exists.** The StampPLC carries an LM75B at I²C `0x48` on the internal bus. `M5StamPLC.begin()`
calls `lm75b_init`, which probes the address and writes nothing. The library's `M5StamPLC.getTemp()` is
not used, on purpose: `LM75B_Class::read16` ignores `readRegister`'s result, so a failed read decodes an
uninitialised buffer into a confident temperature.

**~~To fix~~ — FIXED 2026-10-29, 65 host checks.** The rule is `src/sensors/board_temperature.h`, which
is Arduino-free. `firmware.cpp` supplies the transaction, `m5::In_I2C.readRegister(0x48, 0x00, buf, 2,
400000)`, whose `bool` is the only thing that can tell a failed read from a cold board, and makes one call
to `plc::sampleBoardTemperature` from the logic task. The reading reaches the snapshot through a new
`MqttSnapshotInputs::boardTemperatureC`, which `fillMqttSnapshot` copies without judging it.

- **The decode is the library's, checked rather than trusted.** 11-bit two's complement in the top bits,
  0.125 °C a step, with the sign extended by hand. `board_temperature_test.cpp` compares it against
  `LM75B_Class::temp()`'s own arithmetic, transcribed, for all 65,536 inputs.
- **Plausibility is the chip's operating range**, −55 to +125 °C, the clamp in
  `LM75B_Class::writeAlertTempHelper`. It catches nonsense codes only. All-zero bytes (0.0 °C) and
  all-one bytes (−0.125 °C), the two patterns a stuck bus is most likely to produce, both pass it, and
  the test asserts that they do, so nobody mistakes the range check for a bus check.
- **One read a minute, and only while MQTT is connected.** R2.1.6 says a device on this bus is polled
  "only as often as its purpose requires", and this reading's only purpose is the snapshot. So with no
  broker there is no bus traffic at all, and the sampler's rate with MQTT off is exactly what `G1` and
  `DF23` will measure. 60 s is `MqttPublisher::kHeartbeatMs`, the staleness bound this firmware already
  sets for subscribers. A good read is about 49 bit periods, roughly 120 µs on the wire at 400 kHz. That
  figure is arithmetic from the frame and was not measured.
- **A failed or implausible read publishes `null` at once.** The previous good value is never kept as a
  fallback, and the next attempt waits the full minute, not less: the costly failure is a stalled bus,
  which the driver waits out up to its stall limit, so retrying sooner would multiply exactly the cost
  the cadence bounds.
- **No snapshot is ever assembled with a reading a full read interval old.** Past that the reading is
  non-finite however it got there. After an MQTT outage of a minute or more, the first pass after the
  reconnect re-reads before filling the snapshot; after a shorter one, the last reading is still inside
  its minute and goes in exactly as it would between two reads.
- **That bounds the snapshot, not the queue.** This said "no published reading" until review replayed
  the publisher. `MqttPublisher` keeps its queue across a disconnect: `pump()` does nothing while the
  link is down, and neither `onDisconnected()` nor `onConnected()` clears it. So a diagnostics message
  queued just before a drop goes out first after the reconnect, with the reading from before the
  outage. In the review's replay (eight channels, discovery off, a ten-minute outage, the board at
  25.0 °C before it and 30.0 °C after) the reconnect sent `"tempC":25.0` and then `"tempC":30.0`, the
  second from the reconnect's own full set, queued in the same pass. When `DF30`'s discovery burst filled
  the queue that set was refused, and in a second replay the next diagnostics message went out one
  publish period (10 s) after the reconnect. Since `DF30`'s fix (2026-10-30) the set is held only while
  the connect's discovery is queued, at most 13 passes, and is never refused. Changing the queue is not
  this item.
- **The first read waits out the chip's 100 ms conversion period** (`LM75B_Class::POWER_NORMAL`),
  because nothing read here says what the register holds before the first conversion, and zero would
  be `DF29` again.
- **The unassigned case is now `null` at every layer.** `MqttDiagnosticsTelemetry::boardTemperatureC`
  and the new input both default to NaN, which `jsonNumber` emits as `null`. So a caller that forgets
  the field, or `fillMqttSnapshot`'s null-input return, publishes "no reading" rather than a number.
- **The console says what happened**: a `[temp]` line on each failed read, at most one a minute, and one
  on the first good reading and on each recovery.

**The wiring is gated, not just the rule.** `board_temperature_test.cpp` reads `firmware.cpp` as text
with comments blanked, the technique `portal_lifecycle_test.cpp` uses. It fails unless all of these hold:
the sampling call appears exactly once, and is exactly `plc::sampleBoardTemperature(boardTemperature,
now, readBoardTemperatureBytes)`; the reader is defined once, its whole body is `return
m5::In_I2C.readRegister(plc::kLm75bAddress, plc::kLm75bTemperatureRegister, out, 2,
plc::kLm75bBusFrequencyHz);`, and nothing else names it; the call sits inside `logicTaskCode`, so never
on the sampler's core, and inside the MQTT connected block; `boardTemperatureC` is named once in the
file's code, as `snapshotInputs.boardTemperatureC = boardTemperature.celsius(now);`, after the call and
before `fillMqttSnapshot`; the chip is read through `m5::In_I2C.readRegister` in one place; and
`getTemp(` and `LM75B.temp(` appear nowhere. Whole statements are compared with all whitespace
removed, so reflowing them does not fail the gate and a changed argument does. The reader is found by
its exact signature, so restyling that one line (`uint8_t *out`) fails the gate rather than passing it.

**Review found the first version of that gate too weak, and it was.** It matched the call and the bus
read by their prefixes only, and five edits that kept the prefixes passed every host suite: the reader
throwing away the bus's `bool` and returning `true` (`LM75B_Class::read16`'s defect, back in the one
place this fix moved the read to), the reader asking for the configuration register, the reader asking
for one byte, a second assignment overwriting the reading with `41.2f`, and the call handed
`[](uint8_t*) { return true; }` instead of the reader. That last one publishes `"tempC":0.0` as a good
reading every minute, which is `DF29` itself: the buffer is zeroed before the read, 0x0000 decodes to
0.0 °C, and 0.0 °C passes the range check. `firmware_source_text.h` said the test guarded "that the
chip is read at all", and it did not. The whole-statement checks above are four new checks, 61 in that
suite instead of 57.

**Mutation-tested on exit codes, 24 mutations, every one caught**, all re-run in one script on the
reviewed tree: each mutation applied to a fresh copy, then `board_temperature_test`, `mqtt_snapshot_test`
and `mqtt_publisher_test` built with the suite's flags and run, and a mutation counted as caught only
when one of them exits non-zero. The 13 of the first round, re-created from its list: dropping the
assignment, dropping the read, reading through the library, moving the read into `pollingTaskCode`,
moving it out of the connected block, dropping the snapshot copy, putting either default back to
`0.0f`, keeping the last good value after a failure, removing the maximum age, removing the sign
extension, ignoring the bus's `bool` in `noteRead`, and removing the range check. Review's five, above.
And six more of the same shapes: `|| true` after the bus read, another bus speed, an overwrite of
`snapshot.diagnostics.boardTemperatureC` after the fill, the reader also called from `pollingTaskCode`,
the call handed `0` instead of `now`, and the assignment reading `celsius(0)`. **On the tests before this round, the 13 were caught and the other
11 all passed.** **One finding from the first round worth keeping:** the "no fallback after a failed
read" mutation was caught only by the implausible-read case. At the real cadence a failed read lands
exactly when the previous value reaches its maximum age, so the two rules agree and the bus-failure
checks could not tell which one produced the `null`. A separate check now records a failure one second
after a good read.

**The MQTT page** now shows `"tempC":null` in its example, because no board has reported a figure and
any number there would be invented. It says what a reader will see: °C to one decimal, read once a
minute while connected, and `null` (which Home Assistant shows as `unknown`, per its MQTT sensor
documentation) on a failed, implausible or expired reading. The same edit corrected a sentence that
said all four device-wide entities carry `entity_category: diagnostic`. Three do, and *Board
temperature* deliberately does not (`haEntityIsDiagnostic`). README and the Features and Capabilities
page make no claim about the temperature, so neither changed.

**Verified:** host **2,284 checks across 31 suites**, 0 failures, against 2,219 across 30 at `04e59da2`
(2,280 before the review round; the count is the sum of the figures the suites print, and three suites
that print none add 157 `ok` lines on top, unchanged), and the manifest check green. The wiki dry run
ends "all repo pointers resolve". The firmware compiles in the container.

**Cost: 24 bytes of RAM and 588 of flash.** RAM goes from 81,096 to 81,120 bytes (24.7 % to 24.8 %) and
flash from 1,319,301 to 1,319,889 bytes (39.5 % either way). Measured by building `04e59da2` and this
change in the same container from the same copied dependency cache. The review round changed only
comments in firmware code, and the build after it is the same size to the byte: 81,120 and 1,319,889.

**Not done, because each is the owner's decision:** there is no Modbus register for the temperature,
which sits awkwardly with RS485 being the source of truth, but adding one changes the register contract.
The panel does not show it either.

**⏸️ Not seen on a board**, like everything else on this device. The first proof is a
`[temp] board temperature … C` line on the console after MQTT connects, and a number, not `unknown`, on
the Home Assistant entity. **Until `DF30` was fixed, the Home Assistant half of that check was reliable
only with four or fewer channels in service:** from five, the *Board temperature* discovery message was
refused on every connect. Since 2026-10-30 every config is handed to the MQTT client with any number of
channels, which is not the same as delivered (`DF31`). The console line and `tempC` on
`<base>/diagnostics/state` do not depend on discovery, so they remain the check that does not.

---

## ~~DF30~~ ✅ FIXED 2026-10-30 — Home Assistant discovery is queued as a job across logic passes, and a channel change no longer stops telemetry

Found 2026-10-29 in `DF29`'s review, by asking whether the *Board temperature* entity that `DF29`'s bench
check reads could appear at all on a fully populated device. Not caused by `DF29`: `publishDiscovery()`
was older than it and outside its diff. It was worse than this entry first said, and worse than its 🟡:
a design study on 2026-10-30 — three fact-finders, five analysts each designing one fix and five reviewers
told to refute them, all replaying the real classes on a host — found telemetry stopping until a reboot.

**What happened.** `publishDiscovery()` (`src/firmware.cpp`) enqueued every discovery message for the
connected sensors in one loop, with no `pump()` between enqueues. `HaDiscovery::enumerateEntities`
orders them three per channel in service, then the four device-wide entities with *Board temperature*
first, then the three buttons: 3k + 7 messages for k channels, which is `kHaMaxEntities`' shape.
`MqttPublisher::kQueueCapacity` is 16, and R4.1.3 never evicts a Discovery message, so once the queue
held only availability and discovery, every further message was **refused**. On a connect two
availability messages were already queued, one from `MqttPublisher::onConnected()` and a second, byte
for byte the same, from `firmware.cpp`'s connect latch, so 14 discovery messages fitted:

| Channels in service | Discovery messages | Queued on a connect | Refused |
| --- | --- | --- | --- |
| 0, 1, 2 | 7, 10, 13 | all | none |
| 3 | 16 | 14 | the *Reset lifetime totals* and *Republish discovery* buttons |
| 4 | 19 | 14 | *Under-sampling*, *WiFi RSSI* and all three buttons |
| 5 | 22 | 14 | channel 5's *Lifetime volume*, all four device-wide entities and all three buttons |
| 8 | 31 | 14 | channel 5's *Lifetime volume*, channels 6–8 entirely, the four device-wide entities and the buttons |

The counts were measured by replaying that connect order against the real `MqttPublisher` for every k
from 0 to 8, and the study's own replay reproduced them row by row; the names follow from
`enumerateEntities`' order. A bench would have read `[mqtt] discovery: 14 of 31 entities queued` at every
connect with all eight channels in service.

**A per-pass storm, which this entry missed.** `HaRepublishPolicy::onSensorBitmap` answered
`SensorsChanged` whenever the bitmap differed from the last one FULLY published, and `notePublished` was
reached only when a whole set had been accepted. So once any complete publish had happened, a channel
change whose set no longer fitted made `publishDiscovery()` run on EVERY connected pass: 3k + 7 payloads
built, the same first four messages handed over, the rest refused, and two console lines. Telemetry was
0 messages in 130 s of replay, at k = 4 and at k = 8, and only a reboot or the bitmap returning to the
recorded value ended it. **Ordinary commissioning reached it:** enabling channels one at a time while
connected published 1, 2 and 3 fully and stormed at the fourth. With discovery switched off the same
comparison printed the "connected sensors changed" line on every pass, 70,000 lines in 70 s of replay.

**Three sentences in this entry were wrong**, and are corrected here rather than quietly rewritten:

- "`onSensorBitmap` answers `None` until it has been [published]" was true only of a device never fully
  published. Once one had been, it answered `SensorsChanged` on every pass: the storm above.
- "A device commissioned with all its channels at once shows the losses in the table; one whose channels
  were added later may show none of them" was incomplete, not merely optimistic: adding channels one at
  a time while connected was the route INTO the storm. One, two and three channels each published
  fully. At the fourth every entity still existed — the buttons from the complete set at three channels,
  retained, and the four device-wide entities from the fourth's first attempt, which queued 16 of 19 —
  but telemetry stopped. From the fifth, the new channels' entities never appeared. 19 distinct configs
  were ever handed over on that route, replayed against `65b36f0e`'s classes; "only entities 1–16", which
  this bullet first said, is the figure for a jump from two channels straight to more.
- "with three or more channels, the first diagnostics message went out 10 s after the reconnect" described
  the first connect after boot. On a RECONNECT the change-detection baselines survive (esp-mqtt reconnects
  by itself and `configure()` does not run), so with k ≥ 3 the unchanged sensor and total topics waited
  for the 60 s heartbeat, and only diagnostics, whose `uptimeS` moves, went at 10 s. The `cmd/republish`
  acknowledgement (R4.4.2d, R4.4.4) waited 10 s from k = 3 for the same reason.

**And what the entry said that was right**, kept here because closing it first dropped these with the
wrong ones — the paragraphs "No retry gets further than 16" and "It also costs the reconnect's
telemetry" went whole, although only one sentence in them was wrong:

- *No retry gets further than 16.* Every route that republished — a reconnect, Home Assistant's birth,
  `cmd/republish` — ran the same loop from the first entity again. A reconnect had the same 14 slots;
  a republish outside a connect, with the queue otherwise empty, had all 16, enough for three channels'
  16 and not for four's 19.
- *It also costs the reconnect's telemetry*, measured in a replay at the default 10 s period: with
  discovery on and two channels, channel 1's state topic first went out 60 s after the reconnect, at the
  next heartbeat, because the burst left one free slot and each later message in the set evicted the one
  before it. "With discovery off, channel 1's state and the diagnostics both went out within 2 ms of the
  reconnect" — the figure `DF31` says was the hand-over to esp-mqtt, not the wire.

**Four more, found on the way**, each fixed below: the bitmap was read twice, to enumerate the set and to
record it, while the priority-8 Modbus task can write it in between; the connect latch's second
availability message cost one slot of sixteen and said nothing new; `onMqttData` latched a birth without
looking at the retain flag, so a broker holding a retained `homeassistant/status online` replayed it at
our subscribe and doubled every connect's set; and when a connect and a disconnect were both latched by
the time the logic task reached them, the connect was consumed first, which for the usual real order — a
drop, then esp-mqtt's reconnect 10 s later behind a long pass — left the policy and the ladder believing a
working link was down, and a later change of the connected sensors unpublished for the rest of the
session.

**Why nothing caught it.** The comment on `kQueueCapacity` in `mqtt_publisher.h` said "the caller pumps as
it enqueues", and the caller did not. `ha_discovery_test` checked every payload and `mqtt_publisher_test`
the queue's policy, each on its own; nothing ran the firmware's connect order against the real queue,
because that order lived in `firmware.cpp`, which no host test links. The same blind spot as `DF28` and
`DF29`. And one check in `ha_discovery_test` asserted the storm's own rule: "keeps asking until the
publish is noted — a -1 publish must not be forgotten".

**~~Likely fix, not decided~~ — FIXED 2026-10-30.** The study recommended the second fix this entry named,
a cursor across passes, with amendments, and the owner took it with the decisions marked below. The rule
is `src/net/ha_discovery_job.h`, header-only and Arduino-free:

- **A job, not a burst.** `HaDiscoveryJob` captures the bitmap once when a set starts and keeps the index
  of the next entity; each topic and payload is built at the moment it is enqueued, so nothing is stored
  but the cursor. At most `MqttPublisher::kMessagesPerPass` builds per pass — 4, the `pump()` figure,
  which `firmware.cpp` carried as a literal and which now has a name — counted as attempts, as `pump()`
  counts them.
- **An admission window.** Before EVERY enqueue there must be a free slot — `enqueue()` evicts telemetry
  and still returns true, so its result cannot be the test — and fewer than `kAdmissionWindow`
  availability-plus-discovery messages queued. Derived, with a `static_assert`: 16 −
  `kFullTelemetrySetMessages` (10) − one availability message per connect = **5**, so a full telemetry
  set and the next connect's availability always fit beside whatever discovery is queued. One
  availability message per connect, because the duplicate is gone.
- **Discovery first on a connect (owner decision B).** Stepping the job before `tick()` puts only the
  first slice ahead of telemetry, so the full set `onConnected()` arms is HELD — `tick()` is not called —
  until the set that connect started has queued its last entity; then every config is ahead of every
  state in a first-in, first-out queue. The hold is that one set's: a rerun, a birth or a stream of
  `cmd/republish` does not extend it. It is bounded at `kConnectHoldPassLimit` = 4 + 8 + 1 = **13** passes —
  four for `pump()` to drain a queue the previous session left full, eight to queue the largest set at four
  a pass, one for the connect pass — so a stuck set can never silence telemetry. The longest hold on a
  healthy connect in the host test is 11 passes, a reconnect onto a queue the previous session left full
  of telemetry. (This said 9 until the review round: the route it came from forced one telemetry set and
  then ran a whole pass, whose `pump()` took four, so it never filled the queue it was named for.)
- **The policy compares against the REQUESTED bitmap.** `HaRepublishPolicy::noteRequested` records it at
  every trigger — connect, birth, `cmd/republish`, bitmap change, retry — including while discovery is
  switched off, and `onSensorBitmap` compares against that, so a change is answered once. It compares
  the CHANNEL bits only, since the review round: register 10 keeps all sixteen bits a master writes, and
  `enumerateEntities` ignores bits 8–15, so a write that flipped only those republished a byte-identical
  set. The mask is in that comparison and nowhere else; the register and the panel keep every bit.
- **What "published" means (owner decision A).** A set is recorded with `notePublished` only when every
  entity was accepted, no Discovery-class message is left in the publisher queue, and a new counter,
  `MqttPublisher::discoveryPublishFailures()`, has not moved since the set started: `pump()` pops a
  message even when the sink returns −1, so that counter is the only evidence one was lost. That is
  **handed to esp-mqtt, not delivered**, and the code says so — esp-mqtt can still expire it silently at
  30 s, and the transport discards `MQTT_EVENT_PUBLISHED` (`DF31`).
- **A bounded retry.** A set that ends incomplete — a config too long for its buffer, or a sink failure —
  is retried after 60 s, `kHeartbeatMs`: never every pass, and never only "at the next reconnect".
- **Coalescing.** A birth, `cmd/republish` or bitmap change while a set is being queued lets it finish and
  then runs ONE more, for the latest bitmap; while the set has handed nothing over yet it is retargeted.
- **A disconnect pauses the set**, and the next connect starts it again from entity 0.
- **The latches are consumed disconnect first**, in `consumeMqttLinkLatches`, which moved into the header so
  a host test drives both orders. Really disconnect-then-connect ends connected, as it is. Really
  connect-then-disconnect ends with the policy, publisher and ladder believing a down link is up. The
  policy and the job are harmless there, because everything that publishes runs in the connected block,
  gated on the transport's own `connected()` and on a session the logic task started (below). The two
  subscribes were not: they ran on the report alone, and `esp_mqtt_client_subscribe` takes the client's
  lock before it checks the state (esp-mqtt `bb9c8af` `mqtt_client.c:1708`, then `:1709`), which a
  reconnecting client holds through its whole connect attempt: a name lookup that `network_timeout_ms`
  does not bound, the TCP (transmission control protocol) connect, the CONNECT write and the CONNACK (the
  broker's answer to a connect) read. They now run only on a connect consumed while the transport still
  said connected (`link.sessionStarted`); the next real CONNECTED subscribes. The ladder makes no attempt
  while it believes it is connected, which is harmless **only
  because esp-mqtt's auto-reconnect is on**: the transport never sets
  `disable_auto_reconnect`, although owner decision 3B says it should. If 3B is ever honoured, that case
  needs revisiting.
- **A teardown drops the old session.** `firmware.cpp`'s two `end()` sites are one `stopMqttClient()`, which
  cancels the job, clears the WHOLE publisher queue (`MqttPublisher::discardQueue()`: everything in it
  carries the old base topic and prefix, and `configure()` keeps the queue) and clears all four latches and
  the session flag AFTER `end()`, so a CONNECTED latched before or during `end()` cannot queue an `online`
  under the old topic. During, because esp-mqtt can still deliver a CONNECTED or a DISCONNECTED while
  `end()` runs — a connect attempt in progress finishes under the lock `esp_mqtt_client_stop` waits for
  (`mqtt_client.c:1455`, `:1472-1490`, `:1656`), and a poll error on the last iteration aborts the
  connection (`:1563-1567`) — though never after it returns (`:1571-1573`); that order is load-bearing,
  and three comments that said `end()` dispatches nothing were corrected in the review round. Every
  settings apply bumps the revision and comes through here, which is how discovery being switched off, or
  its prefix changed, reaches the job.
- **Retained births are refused**, in `HaRepublishPolicy::onStatusMessage`, which now takes the flag. MQTT
  3.1.1 §3.3.1.3: a broker sets RETAIN on a message sent because a subscription was just made and clears it
  when forwarding to an existing one, so a real Home Assistant restart is still answered. This rests on
  esp-mqtt's `retain` field — present in the installed 4.4.7 `mqtt_client.h`, and already passed by the
  transport — which R4.4.2c rests on too, with the same bench check in `N-c`'s ⏸️ block. If that flag were
  wrong, R4.4.7 would stop answering births, silently.
- **The connected block runs only for a session the logic task started**, and not while a CONNECTED or a
  DISCONNECTED is latched but unconsumed. esp-mqtt sets the transport's flag and then the latch from its own task, so one
  landing after this pass read the latches would otherwise run the block for a session the pass had not
  started: `tick()` would queue the already-armed full set ahead of the connect's `online` and its
  discovery, which breaks decision B, and on the first connect after boot Home Assistant has no configs.
  The gate this entry first closed with, `connected() && !mqttConnectLatched`, did not close it: between
  `EspMqttTransport::onEvent`'s store of `connected_` and its latch, a few instructions and a call,
  `connected()` is true and the latch still clear, and no order of reading the two helps, as the review
  round found. Swapping the two stores would let a consume see the latch before `connected_` and skip
  the subscribes on a link that then stays up, so they stay as they are. The gate is now
  `plc::mqttConnectedPassMayRun(connected(), mqttSessionStarted, mqttConnectLatched,
  mqttDisconnectLatched)`. `mqttSessionStarted` belongs to the logic task alone: `consumeMqttLinkLatches`
  sets it when it consumes a CONNECTED while the transport — read once, AFTER the connect latch, so a
  CONNECTED is never consumed against a stale `false` — says connected, and clears it when it consumes a
  DISCONNECTED, before the connect is handled; `stopMqttClient()` clears it too. The DISCONNECTED term
  closes the one case the flag cannot: the flag is the dropped session's until a DISCONNECTED is
  consumed, so a drop AND esp-mqtt's reconnect landing between a pass's consume and its gate, read
  between the reconnect's `connected_` store and its latch, would open the gate for a session nobody had
  consumed and hand that pass's telemetry to the new connection ahead of its `online`. The drop's
  DISCONNECTED is latched by then — `onEvent` latches it before esp-mqtt begins its 10 s reconnect wait —
  and it is consumed at the top of the next pass, so it shuts the gate for that one pass at most. The
  cost is one pass either way.
- **`step()` keeps its buffers in a frame of its own**, which the review round found it did not. Defined `inline` in
  the header and called once, it and `serviceMqttConnectedPass` were folded into `logicTaskCode` at -Os,
  so the entity table and the 128- and 1,152-byte build buffers sat in LogicTask's frame for the whole
  pass and every other deep chain — the UI, the portal, WiFi — started about 1.1 KB deeper. Measured with
  `-fstack-usage` in the build container: `logicTaskCode` was 1,328 bytes at `65b36f0e` and 2,464 at
  `d5efac64`. `step()` is now `noinline`, and `logicTaskCode` is 1,376 with `step()` out of line at
  1,440, against `publishDiscovery()`'s 1,424 at `65b36f0e`. Behaviour is identical either way, so no
  route can see the attribute go — a re-review removed it and every suite stayed green. Two things pin it
  now: a source-text check that the attribute stands immediately before `step()`'s one definition, and a
  CI step after the firmware compile, which passes `-fstack-usage` to that same build and fails if
  `logicTaskCode`'s static frame is over 1,600 bytes (`tools/stack_budget/check.sh`, which gives the
  budget's basis: 224 bytes above today's frame, 864 below the inlined one). `-fstack-usage` only
  reports: the RAM and flash figures are the same with it and without it.
- **A config the publish queue refuses despite the window is printed**, on every pass it happens, where
  the review round found the count was kept and read by nothing. Unreachable today — the free-slot and window checks and the builder's bounds
  keep every config inside what `enqueue()` accepts — and not guarded against: if it became reachable,
  the set would try the same entity every pass, never reach a verdict and so never get the retry's
  backoff, and on a connect only the hold's bound would release telemetry. The line is how that would be
  seen.
- `firmware.cpp` keeps the wiring: one read of the bitmap per pass, each latch read and cleared only if it
  was set, the transport read after them, `consumeMqttLinkLatches` before `serviceMqttCommand`, and one
  `serviceMqttConnectedPass` — triggers, one step, `tick()` unless held, `pump(kMessagesPerPass)` — after
  the snapshot is filled.

**The checks, and that they fire.** `test/host/ha_discovery_job_test.cpp`, **262 checks** (137 until the
review round), links the real `mqtt_publisher.cpp`, `ha_discovery.cpp`, `mqtt_command_router.cpp` and
`mqtt_reconnect.cpp` and drives the header's entry points in the firmware's order, for every k from 0 to
8, on every route: first connect; reconnect with baselines kept; a drop 1, 2, 3, 5, 8 and 12 passes into
the set; three connects in a row that drop before their first `pump()`; a reconnect onto a queue left
full of telemetry; a pass whose `pump()` finds the link down mid-set, then a drop and a reconnect; HA
birth, and a birth while a set awaits its verdict and while one awaits its retry; a retained birth;
`cmd/republish`; `cmd/republish` on every pass for 2,000 passes; channels commissioned one at a time
0 → 8; every jump 0 → k; 2 → k while disconnected; 8 → k; a change mid-set, a change while a set awaits
its verdict, and a birth, a birth and a change inside the connect's set; a bit above the channels flipped
ten times, and a connect and a birth that record the bitmap with such a bit set; discovery switched
off by a teardown and in place; a teardown that moves the base topic and
the prefix; both latch orders; a CONNECTED landing between the latch read and the block's gate, and one
caught half-stored there on a first connect and on a reconnect; a drop and a half-stored reconnect both
between a pass's consume and its gate; a CONNECTED consumed after its link dropped again, and one
consumed with its DISCONNECTED in the same pass, followed by a reconnect caught half-stored; truncation; and a sink failure, those two also from 20 s, 90 s and 150 s before
`millis()` wraps. It asserts that every one of the 3k + 7 configs reaches the sink; that
`publishedBitmap()` equals the bitmap; one set per change and none in the 5 s after, and exactly one extra
set for any number of requests mid-set; on every connect, each config before its entity's first state of
that session, a started session reported and the subscribes made (that they come before the session's
first hand-over is pinned by a firmware.cpp text check: after the consume, before the gate), and the hold ended on the pass its own set was queued and never at its bound; the full
telemetry set within 4 passes of the hold; nothing refused or evicted, and on a reconnect onto a full
queue nothing evicted but the slot its `online` takes; at most 4 builds and 4 sink calls in any pass,
the builds counted by the cursor as well as by the job's own report; no step, on any pass of any route,
admitting past the window; one step against a hand-filled queue admitting exactly 1 with 4 reliable
messages queued, 0 with 5, and 0 with 15 telemetry and 1 availability; a retry once a minute across the
wrap; nothing reaching the sink while a CONNECTED is half-stored, and the session flag left clear by a
CONNECTED consumed on a dead link; `consumeMqttLinkLatches` itself reporting and setting the session for
a connect with the link up, and neither with it down; and a stuck job released at exactly 13 passes. Truncation is reached through a real seam, the job's payload-limit constructor argument: at 463
bytes exactly *Board temperature* and *Polling rate* do not fit. It also reads `firmware.cpp` as text,
with whole statements compared: no `publishDiscovery`, no `mqttPublisher.enqueue(`, `tick(` or `pump(` of
its own, both entry points called once with their whole argument lists, the transport read after the
connect latch, the service call after `fillMqttSnapshot`, `mqttTransport.end()` and `endMqttSession` in
one place each inside `stopMqttClient`, whose whole body is compared, the retain flag handed to the birth
test, the gate by its exact text, both subscribes inside `link.sessionStarted`, the refused line in the
console report, and a whitelist: `haDiscoveryJob`, `haRepublish` and `mqttSessionStarted` may be named
only where those statements name them. `board_temperature_test` finds the connected block by its gate's
text too, and was updated with it. Not checked, as `firmware_source_text.h` says it cannot be: whether
a call sits inside a conditional. Any call moved behind an `if` still passes a presence check — the
conditions around the two teardown calls, and the consume and subscribe block itself, are the cases the
mutations below show. It also reads `ha_discovery_job.h` for one thing: `__attribute__((noinline))`
immediately before `step()`'s one definition.

- **It fails against the code it replaces.** Built with `-DDF30_TODAY`, which swaps the glue for a
  transcription of `firmware.cpp` at `65b36f0e` and links that commit's sources — the recipe is in the
  test's header — **126 of its 214 behavioural checks fail**, and **26 of its 27 text checks** when it is
  run from `65b36f0e`'s tree, so that they read that commit's `firmware.cpp`; run from this tree, they read
  this one's and pass. The other 20 checks — the hold, the window, the job's phases, the consume's
  session — have no
  counterpart there. Among the failures: the tail from k = 3, the storm from the fourth channel, the line
  on every pass with discovery off, the set doubled by a retained birth, the ladder left disconnected by
  the latch order, a set recorded although a config was refused, telemetry ahead of the `online` on a
  half-stored CONNECTED, and a set per write of a bit above the channels. (The review round measured the
  first version the same way: 88 of 109 behavioural and 19 of 20 text, which this entry had quoted
  together as "107 of 129".)
- **Twenty-one mutations, every one caught by a named check**, each applied to a fresh copy, three suites
  built with `run.sh`'s flags, and a mutation counted only on a non-zero exit. The policy put back to the
  published bitmap: 15 checks fail, among them "one channel at a time 0 -> 8: exactly one set and one
  sensors-changed answer per change", and 2 in `ha_discovery_test`. No admission window: 5, among them "drop 1-12
  passes into the set, then reconnect: nothing refused, no telemetry evicted". The step moved after
  `tick()`: 2, among them "cmd/republish every pass for 2000 passes: every config still reaches the sink
  during it". No hold: 27, "each config reaches the sink before its entity's first state" on every connect
  route. No queue clear at teardown: "teardown mid-set: nothing under the old base topic or prefix reaches
  the sink after it". The teardown call taken out of `stopMqttClient`: its source check. And twelve more of
  the same shapes: the CONNECTED consumed first, retained births latched again, `false` handed for the flag,
  a set recorded despite a failure, a set recorded before its last config left the queue, no retry, no
  coalescing, no bound on the hold, the duplicate availability restored, a direct `end()` restored, the
  global bitmap read again, and the bitmap trigger not recorded as a request. And three on the two gates:
  the connected block's latch condition removed from `firmware.cpp` (its source checks) and from the
  test's own model of the block ("a CONNECTED landing between the latch read and the gate: no channel's
  state ever reaches the sink before a config for it"), and the subscribes' link check removed. Two
  needed the routes sharpened first: a verdict that did not wait for the queue to drain survived, because
  the refused config was long gone by then, and latched retained births were caught only by the policy's
  unit test, because the birth arrived in the connect pass, where a set that has handed nothing over
  absorbs it. The routes now also refuse the last config, and deliver the retained birth two passes in,
  as a broker's round trip does.
- **And the review round's, 36 of 36 caught** by the same method, four suites this time
  (`board_temperature_test` added), each run from its copy. The independent review had found sixteen that
  the suite above passed, and every one is caught now, by the checks the round added: a request lost while
  a set awaits its verdict or its retry ("HA birth while the set awaits its verdict…", "bitmap change while
  the set awaits its verdict…", "HA birth while a set awaits its retry…"); the window compared with `<=`
  (under both reviewers' names) or written as 16 − 10 ("4 reliable messages queued: one step admits
  exactly 1", "no step, on any pass of any route, admitted a message past the admission window");
  the free-slot check deleted, twice ("16 queued, 1 of them reliable: the step admits nothing…", and the
  full-queue reconnect's eviction check); the retry test written in either of two ways that are not
  wrap-safe, three times ("truncation: retried once a minute…" from before the wrap); two reruns for
  three requests ("…exactly one extra set runs"); the hold carried on into the rerun, or held to the
  verdict ("the hold ends with the connect's own set, never at its bound"); a truncated entity not
  counted as a build ("the cursor moves at most 4 entities in any pass of one set"); and in `firmware.cpp`
  a second consume and a job request on every pass (the whitelist, and "consumeMqttLinkLatches is called
  once"). The five mutations this entry names first — the policy reverted, no window, the step after
  `tick()`, no hold, no queue clear at teardown — were run again on this code and caught. And eleven for
  the round's own fixes: the gate without the session flag, in the header (the half-stored routes) and
  in `firmware.cpp` (its source checks, and `board_temperature_test`'s); the gate without its DISCONNECTED
  term, in the header ("a drop and a half-stored reconnect between the consume and the gate: nothing
  reaches the sink on that pass") and with `false` handed for it in `firmware.cpp` (the same source
  checks); the flag not cleared on a
  consumed DISCONNECTED ("reconnect caught half-stored at the gate: nothing reaches the sink on that
  pass"), or in `stopMqttClient`; the transport read before the latches; the subscribes back on
  `link.connected`; a session started on a dead link ("…the connect starts no session, so nothing
  subscribes"); no channel mask ("a bit above the channels flipped ten times…"); and the refused line
  gone. A re-review then found four more the suite passed, each caught now: the consume never reporting
  a started session, so that nothing would ever subscribe on a board ("…the connect starts a session and
  subscribes before its first hand-over", on every connect route, and the direct check of
  `consumeMqttLinkLatches`); the channel mask on the live side of the comparison only, a retained-config
  flood with telemetry still flowing ("a connect recorded with a bit above the channels…", and the
  policy's own check in `ha_discovery_test`); `step()` without `noinline` (the header's source-text
  check, and CI's frame-budget step would fail too); and the session flag set by a CONNECTED
  consumed on a dead link ("connect then drop inside one pass: the session flag is left clear").
  A last review of that round found the route model's "before its first hand-over" true by
  construction, so the subscribe block moved below the connected block, after the pump, passed; it is
  caught now by a source-text check of where the block sits ("and after the consume, before the
  connected block's gate: written before this pass's pump").
  Seven of the review's mutations still pass, each for a stated reason. Two are the presence-check
  limit `firmware_source_text.h` documents, and are NOT equivalent: the consume and the subscribe block
  moved behind `if (connectLatched)`, which would read and clear a DISCONNECTED arriving on its own pass
  without ever consuming it — the lost DISCONNECTED `mqtt_reconnect.h` was rewritten for — and a teardown
  made conditional on `false`. One is a no-op: an unused second read of the bitmap. Four are equivalent
  under the current wiring: a consumed DISCONNECTED cancelling the job (the connect restarts it from
  entity 0 anyway), `endMqttSession` telling neither the publisher nor the policy (`onConnected` re-arms
  both), the connect recording its request only with discovery on (with discovery off the policy's
  answer starts nothing and prints nothing), and `step()` not pausing while the link is down (its only
  caller runs behind the `connected()` gate).
- `ha_discovery_test`'s policy block is rewritten for the requested bitmap, the retained birth and a
  request recorded with a bit above the channels (269 checks, 262 before), and `mqtt_publisher_test` checks the new counts, the counter and `discardQueue()`
  (202, 192 before).

**Not reachable by any host check**, stated so it is not overclaimed: what esp-mqtt does after the
hand-over — pacing, the QoS 0 misfire, the 30 s expiry, lock waits — is `DF31`; the retain flag is
`N-c`'s; how deep LogicTask's stack really goes, of which only the frame sizes above are measured; and
`firmware.cpp` itself is checked by compiling it and by reading it, not by running it.

**What it does not do**, each read from the code and none measured:

- a channel enabled while connected is not held: its first state goes out on the next `tick()`, a few
  passes before the set reaches its three configs, so Home Assistant shows it from the next publish — 10 s
  if a value moves, otherwise the 60 s heartbeat. Owner decision B was taken for the connect, and the other
  routes were given the admission window only;
- a channel taken out of service mid-set still has its configs handed over, retained, by the set being
  queued — the ghost entities the spec already accepts as a cost of retain;
- esp-mqtt's outbox holds more than it did. For the closest design it replayed, the study's model put the
  peak of one connect at 19,896 bytes (43 entries) and a connect with a coalesced rerun at 37,704 (74),
  against 8,424 (16) when 14 of 31 were refused (MODEL). Free heap has never been measured on this board;
- ~~the gate leaves one window open~~ — CLOSED the same day, after the review round: a drop and esp-mqtt's reconnect
  both landing after a pass consumed the latches and before its gate, read between the reconnect's
  `connected_` store and its latch, opened the gate for one pass, because the session flag was still the
  dropped session's. The gate now also waits while a DISCONNECTED is latched (above), and the host test
  has that interleaving as a route;
- a config the publish queue refused would be retried on every pass with no backoff (above): printed,
  not prevented.

**Cost: 32 bytes of RAM and 1,728 of flash.** RAM 81,152 → 81,184 bytes and flash 1,320,789 → 1,322,517
(24.8 % either way; 39.5 % → 39.6 %), measured by building `65b36f0e` and the commit that added the
gate's DISCONNECTED term in one build container from one dependency cache, espressif32 7.1.3. The same
way, the first version measured 81,176 and 1,322,533, and the review round before that term 81,184 and
1,322,497: the term costs 20 bytes of flash and no RAM, and leaves the frames above as they were. CI's figures differ by a few hundred bytes of embedded paths. The job is 20 bytes,
the Discovery failure counter 4 and the policy's requested bitmap 4 — `requested_` and
`requestedBitmap_` take `HaRepublishPolicy` from 4 bytes to 8, alignment 2 — so 28 by layout, where the
linked image of the first version grew 24; the review round adds `mqttSessionStarted`, 1 byte, and the
image grew 32 in all.

**Verified:** host **2,699 checks across 34 suites**, 0 failures, against 2,420 across 33 at `65b36f0e`
(the sum of the figures the suites print; three suites that print none add 55 `ok` lines either way), and
the manifest check green. The wiki dry run ends "all repo pointers resolve". The firmware compiles in the
container.

**The wiki** said "31 entities on a fully populated device" on the MQTT page and "every entity appears
without any YAML" on Features and Capabilities. Both now say what is announced, that every config is
handed to the MQTT client with any number of channels, and that what happens to it after that is `DF31`'s.

**⏸️ Not seen on a board**, like everything else on this device. With all eight channels in service the
console should read `[mqtt] discovery: all 31 entities handed to the MQTT client` once per connect, Home
Assistant should list 31 entities, and enabling a channel while connected should print one "connected
sensors changed" line, not one a pass. And LogicTask's stack: `uxTaskGetStackHighWaterMark(LogicTask)`
read during a full 31-entity connect, a portal POST and a walk through the UI, on this build and on
`65b36f0e`'s, compared. The task has 10,000 bytes and no high-water figure exists for it; the frame sizes
above say `step()` no longer deepens the rest of the pass, not how much room the pass leaves.

---

## DF31 🟡 What esp-mqtt does with a message after the hand-over: QoS 0 telemetry is paced, reordered and silently expired — measure on a board, then choose

Found 2026-10-30 by `DF30`'s design study, which read the esp-mqtt this firmware links to say what "handed
over" is worth. Not caused by `DF30` and not fixed by it: everything here happens after
`esp_mqtt_client_enqueue` has returned, which is where every host check in this repository stops. Each fact
says how it is known: READ from the source at the cited line; MEASURED in a host replay against esp-mqtt's
real outbox code; MODEL, that replay with an assumed broker round trip.

**Which code.** espressif32 7.1.3 resolves Arduino-ESP32 2.0.17 on ESP-IDF v4.4.7 (`38eeba213a`), whose
esp-mqtt submodule is commit `bb9c8af`. Its `include/mqtt_client.h` is byte-identical to the installed
header, and every error string of the branches its `sdkconfig` compiles in is in the shipped `libmqtt.a`.
The line numbers below are that commit's. None of it can be changed from `platformio.ini`: the library is
prebuilt, with REPORT_DELETED, MSG_ID_INCREMENTAL and the custom outbox settings all off.

- **QoS 0 drains at about one message a second.** READ. Each pass of esp-mqtt's task, while connected,
  sends at most ONE queued item (`mqtt_client.c:1512-1516`), then waits in `esp_transport_poll_read` for up
  to `MQTT_POLL_READ_TIMEOUT_MS`, 1,000 ms (`:1563-1568`, `lib/include/mqtt_config.h:16`) — a `select()`
  that wakes only on data to read. A QoS 1 message's PUBACK (the broker's acknowledgement of a QoS 1
  publish) wakes it; a QoS 0 message gets no reply. So a 10-message telemetry set at the default QoS 0
  takes about 10 s on the wire (MODEL), and the "within 2 ms" `DF30`'s entry quoted for telemetry (kept
  in its list of what it said that was right) was the hand-over, not the wire.
- **The QoS 0 misfire.** READ, and confirmed in the shipped binary's disassembly and in a replay. After a QoS
  0 item is sent, `mqtt_resend_queued` deletes it (`:1398-1404`), and `:1515` then marks
  `pending_msg_id` TRANSMITTED — an id of 0, which every QoS 0 message has, so `outbox_get(0)`
  (`lib/mqtt_outbox.c:56-65`) finds the NEXT unsent QoS 0 item and marks it transmitted without sending it.
  That item goes out only through the retransmit path, which runs when nothing is queued, at most once per
  1,000 ms (`:1518-1524`). In a burst the second QoS 0 message goes out last — MEASURED, five went out in the
  order 1, 3, 4, 5, 2 — and if queued items keep arriving about once a second for 30 s, it never gets a turn
  and expires.
- **Every item expires 30 s after it was ENQUEUED, silently.** READ. `mqtt_delete_expired_messages` runs
  before each send (`:1509`) and, with REPORT_DELETED off, takes the branch that dispatches no
  `MQTT_EVENT_DELETED` (`:1424`, `lib/mqtt_outbox.c:193-207`); `OUTBOX_EXPIRED_TIMEOUT_MS` falls back to
  30,000 (`lib/include/mqtt_config.h:85-89`). Any state, any QoS: QoS 1 here means "retried for at most
  30 s from enqueue". `EspMqttTransport` discards `MQTT_EVENT_PUBLISHED`, the only delivery signal, so
  nothing on the device can tell a delivered message from an expired one.
- **An enqueue or a subscribe can block the logic task for whole network timeouts.** READ — upper bounds
  from the source, none measured. `esp_mqtt_client_enqueue` takes the client's lock with `portMAX_DELAY`
  (`:1939`), and esp-mqtt's task holds it for its loop body (`:1455-1562`). While connected, one body can
  hold it across, in order:
  - one inbound message processed (`:1503`). For a PUBLISH larger than the 1,024-byte input buffer
    (`EspMqttTransport::kInBufferBytes`), the rest is read in chunks, each read bounded by
    `network_timeout_ms` (`:1036-1049`). Then a PUBACK write, only if the sender published at QoS 1 or
    more (`:1310-1326`) — the granted QoS is the lower of the two, so this firmware's QoS 1 subscriptions
    are not enough on their own;
  - then one resend (`:1512-1516`) or one retransmit (`:1518-1524`), never both;
  - then a PINGREQ (the client's keepalive ping) (`:1526` → `:625` → `:1694`), sent when half the
    keepalive interval has passed since the last successful write (`process_keepalive`, `:609-635`).
    Every successful write in the body, the PUBACK included, resets that timer (`mqtt_write_data`,
    `:957-967`), so a ping follows only a body that wrote nothing before it.
  Each write is `esp_mqtt_write` (`:637-656`), which loops over partial writes and gives each send a
  fresh `network_timeout_ms`, 10 s here, so even one write is not bounded by one timeout. During a connect
  attempt the body is the whole attempt (`:1472-1486`): a name lookup, `getaddrinfo` with no timeout (IDF
  4.4.7 `esp_tls.c:148`), so `network_timeout_ms` does not bound it; then the TCP connect, the CONNECT
  write and the CONNACK read, each bounded by it. An enqueue waits for that body.
  `esp_mqtt_client_subscribe` waits for it too and then writes its own packet in the caller's task under
  the lock (`:1708`, the write at `:1732`), and the connect latch calls it twice. No total is given: the
  terms depend on what arrives and what is queued. All of it against §7's 100 ms, on the pass that
  consumes a connect and on any `pump()`. (This said "up to about 10 s … the bound is the network
  timeout" until `DF30`'s review read it again, and "one blocking socket write … about two timeouts
  while connected" until its re-review did.)
- **A steady QoS 0 backlog.** ARITHMETIC, then MODEL. With channels flowing, telemetry arrives at
  (k + 2) / P messages a second against a drain of about one: with eight channels that is any publish period
  under 10 s, and the default 10 s is the edge. The study's model put the steady outbox at about 20.6 KB at
  P = 10 s up to 76 KB at P = 1 s with every value changing, and a Home Assistant birth's set queued behind
  that backlog lost most or all of its configs to expiry, depending on the period and the phase — while the
  device recorded the set as handed over, which is all `DF30`'s "published" claims.

**What would avoid most of it, read from the source and not measured.** Telemetry's QoS is already a
setting, `config.mqtt.qos` (§6.1, default 0; owner decision 8A puts discovery and availability at 1
whatever it says). At QoS 1 every telemetry message gets a non-zero id and a PUBACK: the PUBACK wakes the
task, so the one-a-second pacing goes, and the misfire, which keys on id 0, cannot happen. The cost is that
every message stays in the outbox, on the heap, until it is acknowledged or expires; and ids are random, so
with 33 in flight two collide about 0.8 % of the time (COMPUTED, the birthday product over 65,534 ids), and
one PUBACK then settles the wrong item. Whether the default should change is the decision this item is
for, and it should be taken on measurements.

**To measure on a board**, before deciding: `esp_mqtt_client_get_outbox_size()` (bytes; the only
back-pressure signal the 4.4 API has) and `esp_get_free_heap_size()` and
`heap_caps_get_largest_free_block()` at a connect with eight channels, and in steady flow at P = 10 s and
P = 1 s; the time from handing over a 10-message set to its arrival at the broker, at QoS 0 and at QoS 1;
whether `sensor/2/state` arrives last at QoS 0; whether any config of a connect's set expires on a slow
link; and the longest single `pump()` and connect-latch pass.
None of these is read anywhere on this device yet, and the free heap never has been.

**Blocks.** Nothing in software. It bounds what `DF30`'s fix can claim — handed over, not delivered — and
what any telemetry figure can.

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

**Verified:** 27 checks in `sensor_state_test.cpp`; mutation-tested — adding `configIsValid` to the
predicate fails 3 assertions, using the stored parent instead of the effective one fails 1, NaN reaching
the gross pair fails 1, marking every uncalibrated channel unknown fails 2, and removing the NaN
substitution fails 2. Host **2,197 checks across 29 suites**, 0 failures; firmware SUCCESS at RAM 24.7 % /
Flash 39.0 %.

**REVIEWED, and it found eight things — one of them not about T2 at all** (`DF27` above, the store that
opened after `setup()` had read it). Of the seven that were:

- **a READ failure was reporting a WRITE code.** The boot load raised `StorageFault::Topology` (4) when
  the stored parents read back intact and were not a forest — nothing had failed to persist, and the wiki
  row for code 4 tells an operator the topology "is not reaching flash", which would send them looking at
  the hardware. Appended **code 13, `TopologyNotAForest`**, with its own description saying the flash is
  fine and the topology needs re-entering. Two claims in `nvs_write_health.h` were false as a result and
  are corrected: that a code "can only ever name which group of keys stopped persisting", and that "two
  conditions" earn an immediate raise (it is three).
- **a fixture neutered its own assertion.** The "nothing in service" case passed `kAllCalibrated`, so no
  channel COULD have been marked unknown whatever the code did — the check passed for the wrong reason
  while its text claimed to test that a root must be in service. Fixed by passing an all-uncalibrated
  array, which is the fixture the claim needs.
- **nothing pinned the unknown bitmap to any bit but 0**, so an index-to-bit mis-mapping survived the
  suite. Added a case where channel 2 becomes a delivery point by skipping an out-of-service ancestor and
  must set `0x0004`.
- **R2.2's flow half could not fail** — with no pulses both sides were 0.0. It is now asserted on metered
  flow (105 L/min across three channels).
- **and two of my floating-point claims were simply wrong**, which is worth more than the fixes. "Float
  addends into a double sum exactly by construction" is false beyond about seven decades of spread:
  measured over 200,000 tuples, ascending versus descending summation disagrees **0** times across
  1e-2…1e5 L, **180** times across 1e-2…1e7, and **13,590** times across 1e-2…1e9. And the "99.5 % of
  cases" figure for double addends is **51 %**. Both are now stated with the sweep that produced them,
  and the `gross - downstream` divergence with them: 481 cases in 200,000 with one channel downstream,
  1,453 with seven. The conclusions survive — an order assertion is still not worth writing, and summing
  the roots is still right for structural reasons — but a number nobody can reproduce is exactly what
  this register's opening rule is about.
- the eighth was a mutation attribution crediting assertions that do not hold those mutations, corrected
  because a comment that over-states coverage is how coverage gets over-estimated.

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

## ~~DF27~~ ✅ FIXED 2026-08-30 — the store opened AFTER `setup()` had already read it, so stored WiFi and MQTT settings never took effect

Found by the `T2` review, which was looking at the boot ordering of the new topology load and noticed the
line above it. **This is the most consequential defect this register has carried**, and it had been in the
firmware since 2026-08-05.

`preferences.begin("flow-data", false)` lived inside `logicTaskCode`. `setup()` does not create that task
until its last lines — so every NVS read in `setup()` ran against a CLOSED store, and `Preferences` guards
each accessor on `_started` and hands back the caller's default without a word.

**Two consequences, both silent, both permanent:**

1. **`loadNetSettings` saw no keys at all.** A configured device came up on DEFAULTS at every boot: the
   radio never associated, MQTT never published. The stored block was intact in flash and unreachable.
   Worse than a blank log — the console printed `[net] settings restored (revision 0), wifi=off mqtt=off`,
   which reads exactly like a device nobody had ever configured. And the destruction was deferred rather
   than avoided: `netSettingsSavedRevision` was seeded from those defaults, so the once-a-minute pass saw
   no drift and left flash alone, until the operator's first edit in some boot bumped the revision and
   wrote defaults-plus-that-one-edit over all fourteen keys.
2. **§4.7's anti-boot-loop guard was dead.** The menu pack's attempt counter could neither be read nor
   incremented, so `attempts.read()` returned 0 forever and the give-up check could never fire. A pack
   that validates and then takes the renderer down was retried at **every** boot rather than being given
   up on at the third — which is the whole point of the counter, and it is documented in §3.6 step 11 as
   working.

**Fixed by opening the store at the top of `setup()`, before any reader**, and by NOT opening it again in
the logic task: `Preferences::begin` returns false when the store is already open
(`Preferences.cpp:33-35`), so a second call would have raised fault code 1 on every healthy boot — which
is a trap worth naming, because "add a begin() to be safe" is the obvious wrong fix. §4.1.1's requirement
that the link settings load before the RS485 port opens still holds and still passes.

**What let it live for eight weeks.** `firmware.cpp` is in no host link set, so nothing could reach it; the
symptom is indistinguishable from an unconfigured device; and the log line asserted the opposite of what
had happened. `DF25`'s work is what made it findable at all — the review was tracing which writers report
their outcomes when it hit the reader that had nothing to read.

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
| `N-` | the batch opened after the 2026-08-12 rewrite, lettered `a`–`d` so it could not collide with the numbered groups | N-a–N-g | **N-d2, N-e, N-f open**; N-a, N-b, N-c, N-d1 and N-g closed |
| `DF` | defect — opened 2026-08-18 | DF1–DF31 | **DF23, DF25, DF31 open**; the rest fixed |
| `J` | residue and hygiene — opened 2026-08-18, consolidated out of `MEMORY.md` §6 and `docs/backlog/` | J1–J9 | **all closed**; J1–J8 the same day, J9 on 2026-08-21 |

**`DF`, not `D`, and the reason is this rule's own point.** `D0`–`D5` already mean the display-and-dataset
group — they are in the closed table at the bottom of this file and throughout the archive. The defect
list was first stamped `D1`–`D19`, which made `D1`–`D5` resolve to two different things in one document.
That is precisely I2's complaint (the id still resolves, to something else), so the prefix was corrected
the same day, before anyone cited one. **A retired letter stays retired**: `A`–`I` and `N-` are spent, so
a future batch takes `J` or a new two-letter prefix, never a letter that once meant something else.

- **Never renumber, never reuse.** `DF1`–`DF31` are in the order the defects were found; the fixed ones
  keep theirs, struck through in place. A gap in the open list means an item closed.
- **New items take the next free identifier** — `DF32` onward for defects, `J10` onward for residue, `N-h` onward for entries in
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

  It reports **20** on 2026-10-30, after `DF30` closed and `DF31` opened: seven open, eleven struck
  through in place, and the two standing rules. (It said 13, six open and five struck through, and
  had not been recounted as items were added after `DF25a`, which last set it.)
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
