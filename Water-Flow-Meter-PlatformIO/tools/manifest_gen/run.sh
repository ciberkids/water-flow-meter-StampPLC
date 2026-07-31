#!/usr/bin/env bash
# Generates web/mockup/src/data/actionManifest.json from the firmware's own catalogues.
#
# The manifest describes what the firmware can actually do; keeping it by hand meant the
# design tool validated against a description that had quietly stopped being true. See
# tools/manifest_gen/main.cpp.
#
#   run.sh            write the manifest
#   run.sh --check    exit non-zero if the committed manifest is stale (used by CI)
set -euo pipefail
cd "$(dirname "$0")/../.."
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

g++ -std=gnu++17 -Wall -Wextra -Werror -I src -o "$OUT/manifest_gen" \
  tools/manifest_gen/main.cpp \
  src/ui/core/ui_settings_types.cpp \
  src/ui/core/ui_value_catalogue.cpp

TARGET="../web/mockup/src/data/actionManifest.json"
"$OUT/manifest_gen" > "$OUT/manifest.json"

if [[ "${1:-}" == "--check" ]]; then
  if diff -u "$TARGET" "$OUT/manifest.json"; then
    echo "manifest is up to date with the firmware catalogues"
  else
    echo
    echo "ERROR: the committed manifest does not match the firmware catalogues." >&2
    echo "Run Water-Flow-Meter-PlatformIO/tools/manifest_gen/run.sh and commit the result." >&2
    exit 1
  fi
else
  cp "$OUT/manifest.json" "$TARGET"
  echo "wrote $TARGET"
fi
