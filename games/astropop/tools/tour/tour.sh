#!/bin/bash
# One tour of phase two: the port's and the original's shots of the same screens at the same updates, then shotdiff.
#   games/astropop/tools/tour/tour.sh <name> <tour file> [tour.py options]     (files in build/logs/ap/tour/<name>/)
# The tour's format is in tour.py. The port runs under rundesk with -fast -drawevery -seed=1 and real SDL input (-realinput), from
# a player-less user folder and a registry that says Is3D=0 and ScreenMode=0 (the original in Windows Sandbox has no Direct3D and runs in a window: its Fullscreen box is clear); the original in
# Windows Sandbox from the staged copy (origcmp/stage_original.sh), as run_original.sh runs it. Prints shotdiff's line; the
# summary is build/shotreview/astropop/<name>/summary.json.
# Env: PORT_ARGS (more port arguments), SKIP_ORIG=1 / SKIP_PORT=1 (reuse the other side's shots), PROFILE (a profile file the
# original starts with, as run_original.sh), PORT_USER (a user folder to copy instead of the empty one), SEED (default 1), PORT_EXE
# (a copy of astropop.exe beside SDL2.dll, so that the build can go on while the tour runs: tools/tour/freeze.sh).
# ORIG_WATCHES (the original's watch expressions, as run_original.sh's WATCHES; default: the update counter alone).
set -uo pipefail
cd "$(dirname "$0")/../../../.."
N="$1"; TOUR="$2"; shift 2
D="build/logs/ap/tour/$N"
SEED=${SEED:-1}
mkdir -p "$D"
[ -z "${SKIP_PORT:-}${SKIP_ORIG:-}" ] && rm -rf "$D"/*
mkdir -p "$D"
python -I games/astropop/tools/tour/tour.py "$TOUR" "$D" "$@" || exit 2
wait_room() { for i in $(seq 1 240); do python tools/gamecount.py --limit 6 > /dev/null && return; sleep 15; done; }
if [ -z "${SKIP_PORT:-}" ]; then
  rm -rf "$D/port" "$D/user"; mkdir -p "$D/port" "$D/user"
  if [ -n "${PORT_USER:-}" ]; then cp -r "$PORT_USER"/. "$D/user"/; else printf '[SOFTWARE\\PopCap\\AstroPop]\nIs3D=dword:0\nScreenMode=dword:0\n' > "$D/user/registry.ini"; fi
  wait_room
  build/native/rundesk.exe "$(cygpath -w "$PWD/${PORT_EXE:-build/native/astropop.exe}")" -fast -drawevery -seed=$SEED -userdir="$(cygpath -w "$PWD/$D/user")" -realinput="$(cygpath -w "$PWD/$D/port.txt")" ${PORT_ARGS:-} > "$D/port.log" 2>&1
  echo "port: exit $?, $(ls "$D/port" | wc -l) shots"
fi
if [ -z "${SKIP_ORIG:-}" ]; then
  rm -rf "$D/orig"; mkdir -p "$D/orig"
  rm -f build/origrun/apc/shots/*.png build/origrun/apc/shots/landed.tsv
  NEED=none WATCHES="${ORIG_WATCHES:-[0x5dc220]+0x4a0}" bash games/astropop/tools/origcmp/run_original.sh "$D/orig.txt" "$D/orig.log" $SEED 900 > "$D/orig.out" 2>&1
  echo "original: exit $?, $(ls build/origrun/apc/shots/*.png 2>/dev/null | wc -l) shots"
  cp build/origrun/apc/shots/*.png "$D/orig"/ 2>/dev/null
  cp build/origrun/apc/shots/landed.tsv "$D/orig.landed.tsv" 2>/dev/null
fi
python tools/shotdiff/shotdiff.py --game astropop --orig "$D/orig" --port "$D/port" --run "$N"
