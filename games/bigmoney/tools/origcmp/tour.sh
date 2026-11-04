#!/usr/bin/env bash
# One "looks identical" comparison (phase two, games/bigmoney/docs/IDENTICAL_PLAY.md): the original runs an origrun
# script inside Windows Sandbox on the virtual clock (orig.sh, ORIG_SANDBOX=1, ORIG_SOUND=on), taking its shots; the
# port replays the same input from the original's log (toport.py) with the same settings and takes its shots at the
# updates the original's landed; then the state (statecmp.py), the shots (tools/shotdiff) and the sound and music
# (soundcmp.py) are compared. Last the web build replays the port's script under Node (webport.sh), compared with the
# native port by state and shots.
#   games/bigmoney/tools/origcmp/tour.sh <name> <origrun script> [seed]
# Output: build/origrun/<name> (the original), build/bmcmp/<name> (the port, with tour.txt, the three comparisons'
# summary) and build/shotreview/bigmoney/<name> (shotdiff's page and summary.json). TOUR_SKIP_ORIG=1 reuses the
# original's run (for a port rebuilt since); BM_EXE as in port.sh. One game process at a time.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
HERE="$ROOT/games/bigmoney/tools/origcmp"
NAME="$1"; SCRIPT="$2"; SEED="${3:-1}"
O="$ROOT/build/origrun/$NAME"; P="$ROOT/build/bmcmp/$NAME"
if [ "${TOUR_SKIP_ORIG:-0}" != 1 ]; then
	ORIG_SANDBOX=1 ORIG_SOUND=on bash "$HERE/orig.sh" "$NAME" "$SCRIPT" "$SEED" > "$ROOT/build/bmcmp/$NAME.orig-out.txt" 2>&1
	grep "RESULT" "$ROOT/build/bmcmp/$NAME.orig-out.txt" | tail -n 1
	grep -q "SOUND LEAK" "$ROOT/build/bmcmp/$NAME.orig-out.txt" && { echo "tour.sh: SOUND LEAK: stop running originals"; exit 3; }
fi
[ -f "$O/shots/landed.tsv" ] || { echo "tour.sh: the original's run took no shots (see build/bmcmp/$NAME.orig-out.txt)"; exit 1; }
python "$HERE/toport.py" "$O" > "$ROOT/build/bmcmp/$NAME.port.txt" || exit 1
PORT_SETTINGS="$HERE/settings.txt" BYOPOP_SOUNDLOG="$(cygpath -w "$P")\\portsound.log" \
	bash "$HERE/port.sh" "$NAME" "$ROOT/build/bmcmp/$NAME.port.txt" "$SEED" || { echo "tour.sh: the port's run failed"; exit 1; }
{
	python "$HERE/statecmp.py" "$O/logs/origrun-sandbox.log" "$P/state.txt" | tail -n 3
	python "$ROOT/tools/shotdiff/shotdiff.py" --game bigmoney --orig "$O/shots" --port "$P/shots" --run "$NAME" 2>&1 | tail -n 3
	python "$HERE/soundcmp.py" "$O" "$P"
	PORT_SETTINGS="$HERE/settings.txt" bash "$HERE/webport.sh" "$NAME" "$ROOT/build/bmcmp/$NAME.port.txt" "$SEED"
} > "$P/tour.txt" 2>&1
cat "$P/tour.txt"
