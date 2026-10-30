#!/usr/bin/env bash
# Fails if logicTaskCode's static stack frame is over its budget, read from a -fstack-usage build.
#
#   check.sh [firmware.cpp.su]   default: .pio/build/m5stack-stamplc/src/firmware.cpp.su, relative to
#                                Water-Flow-Meter-PlatformIO/; a path given is taken from where you run it
#
# WHY (`DF30`). `HaDiscoveryJob::step()` keeps an entity table and two build buffers, 128 and 1,152
# bytes, on the stack. Defined inline in a header and called once, GCC folded it into logicTaskCode
# at -Os, and those buffers sat in LogicTask's frame for the WHOLE pass, so every other deep chain —
# the UI renderer, the portal's POST handler, WiFi — started about 1.1 KB deeper, in a 10,000-byte
# task whose high-water mark has never been measured. `__attribute__((noinline))` on step() fixed it,
# and nothing a host test runs can see the attribute go: behaviour is identical either way. A
# source-text check in ha_discovery_job_test pins the attribute; this pins what it is for, the frame.
#
# THE BUDGET, 1,600 bytes, from three measurements with -fstack-usage in the build container:
#   1,328  logicTaskCode at 65b36f0e, before DF30
#   1,376  with step() out of line, as now
#   2,464  with step() inlined (d5efac64, and the attribute removed again in DF30's re-review)
# 1,600 leaves the pass's own locals 224 bytes to grow before anyone has to look, and sits 864 bytes
# below the inlined figure, so step()'s buffers cannot come back unnoticed. Raise it deliberately, in
# the same change that explains why the frame grew — it is the depth every other call in the pass
# starts from.
#
# -fstack-usage only REPORTS: the RAM and flash figures of a build with it equal those without it
# (measured for DF30, 81,184 and 1,322,517 bytes either way), so CI passes it to its one firmware
# build through PLATFORMIO_BUILD_FLAGS, which PlatformIO adds to platformio.ini's build_flags.
set -euo pipefail
# A path given on the command line means the caller's directory, so resolve it before moving.
SU=""
if [[ $# -ge 1 ]]; then SU="$(realpath -m -- "$1")"; fi
cd "$(dirname "$0")/../.."
SU="${SU:-.pio/build/m5stack-stamplc/src/firmware.cpp.su}"
FUNCTION="void logicTaskCode(void*)"
BUDGET=1600

if [[ ! -f "$SU" ]]; then
  echo "ERROR: $SU not found. Was the firmware built with PLATFORMIO_BUILD_FLAGS=-fstack-usage?" >&2
  exit 1
fi

# Each line: <file>:<line>:<column>:<function>\t<bytes>\t<static|dynamic|bounded>
matches="$(awk -F'\t' -v f="$FUNCTION" '{ n = split($1, a, ":"); name = a[4]; for (i = 5; i <= n; ++i) name = name ":" a[i]; if (name == f) print $2 "\t" $3 }' "$SU")"
count="$(printf '%s' "$matches" | grep -c . || true)"
if [[ "$count" -ne 1 ]]; then
  echo "ERROR: expected one '$FUNCTION' line in $SU, found $count." >&2
  exit 1
fi
bytes="${matches%%$'\t'*}"
kind="${matches#*$'\t'}"

if [[ "$kind" != "static" ]]; then
  echo "ERROR: $FUNCTION's frame is '$kind', not static: its size is no longer known at compile time." >&2
  exit 1
fi
if (( bytes > BUDGET )); then
  echo "ERROR: $FUNCTION's frame is $bytes bytes, over its $BUDGET-byte budget." >&2
  echo "Something was folded into the logic task's frame — HaDiscoveryJob::step() lost its noinline" >&2
  echo "attribute, or a new local is that large. See tools/stack_budget/check.sh for the budget's basis." >&2
  exit 1
fi
echo "$FUNCTION: $bytes-byte static frame, inside its $BUDGET-byte budget"
