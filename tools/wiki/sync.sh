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

REPO_SSH="github.com-personal:ciberkids/water-flow-meter-StampPLC.wiki.git"
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

## Start here

| If you want to | Read |
| --- | --- |
| Edit anything about the panel's screens | [[UI Dataset Contract]] — **read this first**, two of the four JSON files are generated |
| Understand the interaction model | `docs/Requirements/feature addition/Display_UI_Requirements.md` |
| See a specific screen's agreed layout | `docs/Requirements/feature addition/Display_Per_Screen_Spec.md` |
| Wire something new into the firmware | `docs/Requirements/feature addition/UI_Firmware_Interface.md` |
| Work on SD-card menu packs | `docs/Requirements/feature addition/Loadable_UI_Menu_Packs.md` |
| Integrate over Modbus | `docs/Requirements/Project_document.md` §4, the register map |

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
done < <(grep -ohE '(docs|web|tools)/[A-Za-z0-9_./ -]+\.(md|json|mjs|sh)' "$STAGE"/*.md \
         | sed 's/`//g' | sort -u)
[ "$missing" -eq 0 ] || { echo "refusing to publish with broken pointers" >&2; exit 1; }
echo "all repo pointers resolve."

if [ "${1:-}" != "--push" ]; then
  echo
  echo "Dry run. Re-run with --push to publish."
  exit 0
fi

WIKI="$(mktemp -d)"
trap 'rm -rf "$STAGE" "$WIKI"' EXIT
if ! git clone --quiet "$REPO_SSH" "$WIKI" 2>/dev/null; then
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
cd "$WIKI"
git add -A
if git diff --cached --quiet; then
  echo "wiki already up to date."
  exit 0
fi
git commit --quiet -m "docs: regenerate wiki orientation pages from the repository docs"
git push --quiet
echo "published: Home, UI-Dataset-Contract"
