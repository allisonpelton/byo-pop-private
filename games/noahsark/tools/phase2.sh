#!/usr/bin/env bash
# One tour of phase two: the port's and the original's shots of the same screens at the same updates, then shotdiff.
#   games/noahsark/tools/phase2.sh <name> <tour file> [port args...]     (files in build/noahtest/p2/<name>/)
# The tour's format is in tour.py. The original runs in Windows Sandbox (SANDBOX=1, one minute or so); the port under rundesk
# with -fast -seed=1. Prints shotdiff's line; the summary is build/shotreview/noahsark/<name>/summary.json.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT"
N="$1"; TOUR="$2"; shift 2
D="build/noahtest/p2/$N"
rm -rf "$D"; mkdir -p "$D"
python -I games/noahsark/tools/tour.py "$TOUR" "$D"
rm -rf "build/noahtest/ud_p2_$N"
games/noahsark/tools/run_script.sh "$D/port.txt" "build/noahtest/ud_p2_$N" 300 -seed=1 "$@" > "$D/port.log" 2>&1
rm -rf "build/origrun/p2_$N"
SANDBOX=1 games/noahsark/tools/run_original.sh "p2_$N" "$D/orig.txt" 1 > "$D/orig.out" 2>&1
mkdir -p "$D/orig"; cp "build/origrun/p2_$N/shots/"*.png "$D/orig/" 2>/dev/null
rm -f "$D/orig/landed.tsv"
python tools/shotdiff/shotdiff.py --game noahsark --orig "$D/orig" --port "$D/port" --run "$N"
