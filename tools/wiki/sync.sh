#!/usr/bin/env bash
# Publishes the GitHub wiki's ORIENTATION pages, derived from the docs in this repo.
#
#   tools/wiki/sync.sh            print what would be published, change nothing
#   tools/wiki/sync.sh --push     clone the wiki, write the pages, commit and push
#
# WHY THIS IS A SCRIPT AND NOT COMMITTED PAGES.
#
# A wiki copy of a document is a second home for the same text, and this project's recurring defect is
# exactly that — a range hint duplicating a descriptor, a sample table duplicating a resolver, an id
# list duplicating an enum. The second copy always wins on screen and always drifts.
#
# So the wiki carries ORIENTATION and a POINTER, never the content: enough for someone who lands there
# to understand the shape and know which file to open. The pages are generated here rather than stored,
# so there is one home for their text — this script — and re-running it is how they stay current.
#
# FIRST RUN. GitHub does not create `<repo>.wiki.git` until one page exists, so the wiki must be
# initialised once through the web UI (Wiki tab -> Create the first page -> Save). After that this
# script owns it.
set -euo pipefail
cd "$(dirname "$0")/../.."

# Overridable so CI can authenticate differently from a developer's machine. A workstation has an SSH
# host alias; a GitHub Actions runner has a token and no SSH key at all, and hardcoding either means the
# other cannot publish — which is how this script stayed a manual step nobody remembered to run.
WIKI_REMOTE="${WIKI_REMOTE:-github.com-personal:ciberkids/water-flow-meter-StampPLC.wiki.git}"
DOCS="docs/Requirements/feature addition"
STAGE="$(mktemp -d)"
trap 'rm -rf "$STAGE"' EXIT

# ── Home ─────────────────────────────────────────────────────────────────────────────────────────
cat > "$STAGE/Home.md" <<'PAGE'
# Water Flow Meter — StampPLC

An eight-channel water flow monitor on an M5Stack StampPLC (ESP32-S3), with a 240 × 135 panel, Modbus
RTU, MQTT and Home Assistant discovery.

**Documentation lives in the repository and is versioned with the code.** This wiki is orientation
only — every page here points at the file that actually holds the detail, so there is nothing for the
two to disagree about.

> ⚠️ **Every page here is generated, and edits made in this editor will be destroyed.** CI republishes
> the whole wiki on each push to `main`, so a correction typed here survives until the next merge and
> then vanishes with nothing reporting it. Edit the source instead:
> `tools/wiki/sync.sh` for these orientation pages, `tools/wiki/pages/*.md` for the long ones, and
> `tools/wiki/gen-registers.mjs` for the register reference — which reads the firmware headers, so an
> address is corrected by correcting the header.

## Start here

| If you want to | Read |
| --- | --- |
| Edit anything about the panel's screens | [[UI Dataset Contract]] — **read this first**, two of the four JSON files are generated |
| See every screen and how to move between them | [[Screen Navigation]] — the menu tree, generated from the dataset |
| See what every screen looks like | [[Screen Gallery]] — a picture of each, from the web designer |
| Understand the interaction model | `docs/Requirements/feature addition/Display_UI_Requirements.md` |
| See a specific screen's agreed layout | `docs/Requirements/feature addition/Display_Per_Screen_Spec.md` |
| Wire something new into the firmware | `docs/Requirements/feature addition/UI_Firmware_Interface.md` |
| Put a menu pack on the SD card | [[SD Card]] — the card's file layout, and what is not built yet |
| Switch the device to a different UI | Hold **UP + DOWN + ENTER for 3 s** — the Select Menu. It is firmware-drawn and in no screen table, so nothing on the panel advertises it; `docs/Requirements/Gesture_Reference.md` §3.6 |
| Integrate over Modbus | [[Modbus Registers]] — the whole map, generated from the firmware headers |
| Read telemetry into Home Assistant | [[MQTT]] |
| Get a device onto a network | [[WiFi]] |
| Choose between the three | [[Communications]] |

## The one thing worth knowing before you edit

Four JSON artefacts describe the panel and **two of them are generated**:

| File | |
| --- | --- |
| `docs/Requirements/feature addition/screens/<id>.json` | authored — one per screen, the agreed geometry |
| `ui_value_catalogue.cpp` + `ui_settings_types.cpp` | authored (C++) — what the UI may reference |
| `web/mockup/src/data/actionManifest.json` | **generated** from the C++ above |
| `web/mockup/src/data/screens.json` | **generated** from the requirement files + the manifest |

Editing a generated file works locally and then vanishes the next time the generator runs, with CI
reporting a diff nobody expected. [[UI Dataset Contract]] says which to edit instead.
PAGE

# ── UI Dataset Contract ──────────────────────────────────────────────────────────────────────────
cat > "$STAGE/UI-Dataset-Contract.md" <<'PAGE'
# UI Dataset Contract

Four JSON artefacts describe the panel, and they are not peers: two are authored, two are generated,
and every edge between them is enforced by a gate that fails the build.

```
docs/.../screens/*.json  ─┐                          authored: the agreed geometry
                          ├─► generate.mjs ─► screens.json ─► export ─► GeneratedUi.{h,cpp}
actionManifest.json ──────┘                                          └─► default.uipack
        ▲
        └── manifest_gen/run.sh ◄── ui_value_catalogue.cpp, ui_settings_types.cpp
```

**The full contract is in the repository, kept current with the code:**

> `docs/Requirements/feature addition/UI_Dataset_Contract.md`

It covers every field of every file, the three spec-only fields (`worst`, `bound`, `bannerReplaces`),
screen-level `visibleWhen` and why it was allowed to relax R7.3, the ten gates and what each one
catches, editing recipes for the common changes, and the conventions that will otherwise look
arbitrary — why the unit lives in the header, why `--` never means "not detected", and why one fact
gets exactly one home.

## Three things that catch people

- **`screens.json` is generated.** Edit the per-screen requirement file for geometry, or
  `web/mockup/tools/skeleton/generate.mjs` for navigation.
- **A binding must exist in the firmware first.** `actionManifest.json` is emitted from the C++
  catalogue; a value invented in the dataset fails `manifest-value-coverage` — a hard failure, because
  the element would render placeholder text on hardware while looking live in the mockup.
- **A catalogue entry without a resolver arm only warns.** It compiles, ships, and renders blank on the
  device. Add the arm in `ui_bindings.cpp` in the same change.

*This page is generated by `tools/wiki/sync.sh`. Edit the repository document, not this page.*
PAGE

# ── Screen Navigation ────────────────────────────────────────────────────────────────────────────
#
# The diagram is NOT written here. It is embedded from docs/diagrams/ui_navigation_tree.mermaid, which
# gen-diagrams.mjs generates from screens.json and CI gates for freshness and for parsing — so the page
# can only show the tree the firmware actually runs, and this script holds the key, not the structure.
{
cat <<'PAGE'
# Screen Navigation

Every screen on the panel and how you get from one to another. It shows **screens only**: what each one
displays and the value editors behind setting pages are left out, so the shape of the menu stays visible.

> This diagram is **generated** from `web/mockup/src/data/screens.json` by `tools/wiki/gen-diagrams.mjs`,
> and published from `docs/diagrams/ui_navigation_tree.mermaid`. It changes when the dataset changes; to
> change the menu, edit the dataset (see [[UI Dataset Contract]]), never this page.

## How to read it

- **A box with a title is a level.** The screens inside it form a ring: **DOWN** steps to the next
  number (`#1`, `#2`, ...), **UP** to the previous one, and both wrap at the ends. Follow the numbers, not
  the position on the page: the layout places boxes to keep arrows short, not in ring order. Those steps
  are not drawn, because they hold everywhere.
- **An arrow is ENTER**, going down into the level it points at.
- **Every level ends with a `Back` screen.** ENTER on it goes up one level. There is no back button.
- **ENTER held for 1.5 s** returns to P0 - Global Status from anywhere in the menu.
- **UP + DOWN together** turns the display off and returns to P0.
- **Screens marked *only when*** appear only for that setting. S3 shows for a pulses-per-litre sensor, S4
  and S5 for a multiplier one.
- **The eight sensors share one settings level.** SEN1 to SEN8 all open the same S1 to S.BACK screens,
  showing the values of the sensor you picked.
- **`Reset ...?` and `Factory reset?` are confirm screens**, and they work the other way round so a slip
  cannot erase data: a short ENTER **leaves** without acting, and only holding ENTER confirms. The hold
  is 1.5 s for the session and the peak flow, 3 s for totals, calibration and the portal login, and
  30 s for a factory reset.
- **The Select Menu** is drawn by the firmware and is in no screen table. It opens from the `SELECT MENU`
  page at the end of the root ring, or by holding **UP + DOWN + ENTER for 3 s** from any screen.

The full gesture rules are in `docs/Requirements/Gesture_Reference.md`. To see what each screen looks
like, open the [[Screen Gallery]].

## The tree

```mermaid
PAGE
# Without the file's %% header: the intro above already says where it comes from, and the header's
# prose has file paths the pointer check below would read as one path with spaces in it.
sed -e '/^%%/d' -e '/./,$!d' docs/diagrams/ui_navigation_tree.mermaid
echo '```'
} > "$STAGE/Screen-Navigation.md"

# ── Screen Gallery ───────────────────────────────────────────────────────────────────────────────
#
# Pictures of every screen, laid out in the navigation tree's levels. The pictures are captured by CI
# (`npm run capture:screens`) before this runs and published into the wiki repository under screens/,
# never committed. gen-gallery.mjs exits non-zero when graphics/screens/ and the dataset disagree,
# which stops the publish: run the capture first.
node tools/wiki/gen-gallery.mjs --out "$STAGE/Screen-Gallery.md"
mkdir -p "$STAGE/screens"
cp graphics/screens/*.png "$STAGE/screens/"

# ── SD Card ──────────────────────────────────────────────────────────────────────────────────────
#
# The paths and limits are READ from the firmware headers, never typed here, so the page cannot describe
# a card layout the reader no longer uses. A constant that cannot be found stops the publish.
SD_H="Water-Flow-Meter-PlatformIO/src/ui/pack/ui_pack_storage_sd.h"
LOADER_H="Water-Flow-Meter-PlatformIO/src/ui/pack/ui_pack_loader.h"
sd_const() { sed -nE "s/.*$2 = \"?([^\";]+)\"?;.*/\1/p" "$1" | head -1; }
SD_DIR="$(sd_const "$SD_H" kDirectory)"
SD_POINTER="$(sd_const "$SD_H" kPointerPath)"
SD_MAX_KB="$(sd_const "$LOADER_H" kMaxPackBytes | sed -E 's/ *\* *1024//')"
SD_MAX_NAME="$(sd_const "$LOADER_H" kMaxNameBytes)"
for v in SD_DIR SD_POINTER SD_MAX_KB SD_MAX_NAME; do
  [ -n "${!v}" ] || { echo "SD Card page: could not read $v from the firmware headers" >&2; exit 1; }
done
cat > "$STAGE/SD-Card.md" <<PAGE
# SD Card

The StampPLC has a microSD slot. Today the firmware uses the card for **one thing: menu packs**, which
swap the panel's whole menu without reflashing. With no card, or nothing usable on it, the device runs
the menu built into the firmware, so a card is never required.

> ⚠️ **Not finished yet — tracked as \`N-g\` in \`docs/active_work/open_decisions.md\`.** The device can
> read and select packs, but there is **no command yet that builds a \`.uipack\` from the designer**: the
> pack writer (\`web/mockup/tools/exporter/packEmitter.ts\`) is only called by a test. Until that lands,
> this page describes the format the firmware reads, not something you can produce.

## File layout

Format the card as **FAT32** and put everything in one folder:

\`\`\`
${SD_DIR}/
├── active            which pack to load: a text file holding one file name
├── default.uipack    a menu pack
└── other.uipack      as many packs as you like
\`\`\`

| File | What it is |
| --- | --- |
| \`${SD_DIR}/<name>.uipack\` | One complete menu in a single binary file. At most **${SD_MAX_KB} KB**, and the file name must be under **${SD_MAX_NAME} characters**. Only files ending in \`.uipack\` are listed |
| \`${SD_POINTER}\` | Plain text: the file name of the pack to load, e.g. \`default.uipack\`. A trailing newline from a text editor is fine. The Select Menu writes this file for you |

## Choosing a pack

Open the **Select Menu** from the \`SELECT MENU\` page at the end of the root ring, or by holding
**UP + DOWN + ENTER for 3 s** from any screen. It lists the packs on the card. UP and DOWN move,
ENTER selects one and reboots into it, and a held ENTER leaves without changing anything. See
[[Screen Navigation]].

## When something is wrong

The device never gets stuck on a bad card; it boots the built-in menu instead. A pack is refused when
it is missing, larger than the limit, fails its checksum, or was built for a different firmware
version. If loading the selected pack fails on two boots in a row, the device also deletes
\`${SD_POINTER}\`, so the next boot starts clean.

## Not on the card yet

**WiFi and MQTT settings from a file on the card** are planned (item \`N8b\` in
\`docs/Requirements/feature addition/WiFi_MQTT_Connectivity.md\`) but **not built**. Set them up with
the configuration portal or over Modbus instead; see [[WiFi]].

The full design, including the binary format, is in
\`docs/Requirements/feature addition/Loadable_UI_Menu_Packs.md\`.
PAGE

# ── The communication pages ──────────────────────────────────────────────────────────────────────
#
# These are FILES rather than heredocs, unlike the two above, for one reason each way: the orientation
# pages are a few paragraphs that belong next to the pointer logic, while the communication reference
# runs to hundreds of lines and is edited as prose. Keeping the long ones in `pages/` means a reviewer
# reads a Markdown diff instead of a shell diff.
cp tools/wiki/pages/*.md "$STAGE/"

# The register reference is GENERATED from the firmware headers on every run — see the header of
# gen-registers.mjs for why a hand-maintained register table was never an option. It exits non-zero if
# the headers and its descriptions disagree, which takes the whole publish down with it, deliberately.
node tools/wiki/gen-registers.mjs --out "$STAGE/Modbus-Registers.md"

echo "── pages to publish ──────────────────────────────────────────"
for f in "$STAGE"/*.md; do
  echo
  echo "### $(basename "$f")  ($(wc -l < "$f") lines)"
  sed 's/^/    /' "$f"
done
echo

# Sanity: every repo path the pages mention must exist, or the wiki sends people nowhere.
missing=0
while read -r path; do
  [ -e "$path" ] || { echo "BROKEN POINTER: $path" >&2; missing=1; }
done < <({
  grep -ohE '(docs|web|tools)/[A-Za-z0-9_./ -]+\.(md|json|mjs|sh)' "$STAGE"/*.md
  # Firmware citations are written `src/net/foo.cpp` in the prose, so resolve them against the
  # PlatformIO tree. Worth checking rather than trusting: these are the paths that actually move, and
  # a wiki page that cites a file somebody renamed is how a reader concludes the doc is stale and
  # stops trusting the rest of it.
  grep -ohE '\bsrc/[a-z0-9_/]+\.(h|cpp)' "$STAGE"/*.md | sed 's|^|Water-Flow-Meter-PlatformIO/|'
} | sed 's/`//g' | sort -u)
[ "$missing" -eq 0 ] || { echo "refusing to publish with broken pointers" >&2; exit 1; }
echo "all repo pointers resolve."

if [ "${1:-}" != "--push" ]; then
  echo
  echo "Dry run. Re-run with --push to publish."
  exit 0
fi

WIKI="$(mktemp -d)"
trap 'rm -rf "$STAGE" "$WIKI"' EXIT
if ! git clone --quiet "$WIKI_REMOTE" "$WIKI" 2>/dev/null; then
  cat >&2 <<'HELP'
Could not clone the wiki repository.

GitHub does not create <repo>.wiki.git until the wiki has at least one page, and it cannot be created
over the API. Initialise it once by hand:

  1. open https://github.com/ciberkids/water-flow-meter-StampPLC/wiki
  2. "Create the first page", save anything at all
  3. re-run: tools/wiki/sync.sh --push

This script will then overwrite that page with the generated Home.
HELP
  exit 1
fi

cp "$STAGE"/*.md "$WIKI/"
# Replaced wholesale, so a screen removed from the menu loses its picture too.
rm -rf "$WIKI/screens"
cp -r "$STAGE/screens" "$WIKI/screens"
cd "$WIKI"
git add -A
if git diff --cached --quiet; then
  echo "wiki already up to date."
  exit 0
fi
# Identity on the command rather than in config: a runner has none, and a workstation already has
# one that this script has no business overwriting.
git -c "user.name=${WIKI_AUTHOR_NAME:-$(git config user.name || echo 'wiki sync')}" \
    -c "user.email=${WIKI_AUTHOR_EMAIL:-$(git config user.email || echo 'wiki-sync@users.noreply.github.com')}" \
    commit --quiet -m "docs: regenerate wiki orientation pages from the repository docs"
git push --quiet
echo "published: $(cd "$WIKI" && ls *.md | sed 's/\.md$//' | paste -sd, -)"
