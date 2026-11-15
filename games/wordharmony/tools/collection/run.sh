#!/usr/bin/env bash
# The port on the PopCap Game Collection's files, against the same port on the retail copy's (README.md, "The PopCap Game
# Collection version"). Needs build/native/wordharmony.exe and rundesk, assets/wordharmony staged from the retail copy, and the
# web build (tools/check.sh build-web:wordharmony) for the last step.
#   games/wordharmony/tools/collection/run.sh <the Collection's WordHarmonyDeluxeComponent folder>
# Stages the Collection's files into build/wh-coll/assets/wordharmony, then: tools/check.sh wordharmony on them (WH_GAMEDIR: the
# port's two scripted games end in the states the suite expects and play each update as the frozen baseline does on the retail
# files), the tours of games/wordharmony/tools/shots once on each set of files (port.sh, -fast, seed 1, the original's splash hold:
# the state logs and sound logs must be the same, the pictures are compared by shotdiff) and the web build under Node on the
# Collection's files (the first two tours, each against the same run on the retail files and against the native shots). One game
# process at a time, invisible and silent under rundesk. TOURS="tour1 tour2" and WEBTOURS="tour1" run only some.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
SRC="$1"
T=games/wordharmony/tools/shots
TOURS="${TOURS:-tour1 tour2 tour3 tour4 tour5}"
WEBTOURS="${WEBTOURS:-tour1 tour2}"
OUT=build/wh-coll
COLL="$(pwd -W)/$OUT/assets/wordharmony"
waitslot() { until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done; }
mkdir -p "$OUT"
python -I games/wordharmony/tools/stage_wordharmony_assets.py "$SRC" --out "$OUT/assets/wordharmony" || exit 1

echo "--- the two scripted games on the Collection's files"
waitslot
WH_GAMEDIR="$COLL" tools/check.sh wordharmony | tail -n 1

# tour 4 continues a saved THINK game (the suite's: the chain ORD on a ledge, a W below it)
{
	printf '%s\n' '[SOFTWARE\GameHouse\WordHarmony]'
	printf '%s' 'SolitaireSavedGame=sz:<level mode="1" level="0" score="0" respawnTimer="0" wc="0" hug="0" hul="0" '
	printf '%s' 'ha="1" lp="0" lba="0" bonus="" ilg="4" time="3" ><rope text="ORD" ><bubble letter="O" origin="194.2,66" '
	printf '%s' 'special="" /><bubble letter="R" origin="238.2,66" special="" /><bubble letter="D" origin="282.2,66" '
	printf '%s\n' 'special="" /></rope><bubble letter="W" origin="355.3,198" special="" /><HUD></HUD><spawnqueue></spawnqueue></level>'
} > "$OUT/think.ini"

echo "--- the tours on each set of files"
for t in $TOURS; do
	reg=""; [ "$t" != tour4 ] || reg="$ROOT/$OUT/think.ini"
	rm -rf "$OUT/$t"; mkdir -p "$OUT/$t"
	waitslot
	PORTREG="$reg" bash $T/port.sh $t | tail -n 1
	mv build/ph2/$t/port "$OUT/$t/retail"; mv build/ph2/$t/state.txt "$OUT/$t/retail-state.txt"; mv build/ph2/$t/sound.txt "$OUT/$t/retail-sound.txt" 2>/dev/null
	waitslot
	WH_GAMEDIR="$COLL" PORTREG="$reg" bash $T/port.sh $t | tail -n 1
	mv build/ph2/$t/port "$OUT/$t/coll"; mv build/ph2/$t/state.txt "$OUT/$t/coll-state.txt"; mv build/ph2/$t/sound.txt "$OUT/$t/coll-sound.txt" 2>/dev/null
	cmp -s "$OUT/$t/retail-state.txt" "$OUT/$t/coll-state.txt" && echo "$t: state logs the same ($(wc -l < "$OUT/$t/coll-state.txt") lines)" || echo "$t: STATE LOGS DIFFER"
	cmp -s "$OUT/$t/retail-sound.txt" "$OUT/$t/coll-sound.txt" && echo "$t: sound logs the same" || echo "$t: SOUND LOGS DIFFER"
	# both sides are the port's own pictures: a game name with no shotdiff configuration, so no crop
	python tools/shotdiff/shotdiff.py --game wordharmony-coll --orig "$OUT/$t/retail" --port "$OUT/$t/coll" --run "coll-$t" | tail -n 1
done

echo "--- the web build under Node on each set of files"
for t in $WEBTOURS; do
	[ -d "$OUT/$t/coll" ] || continue
	reg=""; [ "$t" != tour4 ] || reg="$ROOT/$OUT/think.ini"
	waitslot
	PORTREG="$reg" bash $T/web.sh $t | tail -n 1
	rm -rf "$OUT/$t/web-retail"; mv build/ph2/$t/web "$OUT/$t/web-retail"; cp build/ph2/$t/webrun/state.txt "$OUT/$t/web-retail-state.txt"
	waitslot
	WH_GAMEDIR="$COLL" PORTREG="$reg" bash $T/web.sh $t | tail -n 1
	rm -rf "$OUT/$t/web-coll"; mv build/ph2/$t/web "$OUT/$t/web-coll"; cp build/ph2/$t/webrun/state.txt "$OUT/$t/web-coll-state.txt"
	cmp -s "$OUT/$t/web-retail-state.txt" "$OUT/$t/web-coll-state.txt" && echo "web $t: state logs the same ($(wc -l < "$OUT/$t/web-coll-state.txt") lines)" || echo "web $t: STATE LOGS DIFFER"
	python tools/shotdiff/shotdiff.py --game wordharmony-coll --orig "$OUT/$t/web-retail" --port "$OUT/$t/web-coll" --run "web-coll-$t" | tail -n 1
	# the web build against the native port on the same files: its usual differences (the Options box's full screen check box)
	python tools/shotdiff/shotdiff.py --game wordharmony-coll --orig "$OUT/$t/retail" --port "$OUT/$t/web-retail" --run "web-retail-$t" | tail -n 1
done
echo done
