#!/usr/bin/env bash
# compare_mode.sh with a resync of the generator (the get-ready of the original ends differently from the port's, which
# moves the generator and with it the next level's board): the port takes over the original's generator state at each level's
# start and at the end of each get-ready (-resync), which a run of the original gives, so every run reaches one level further.
#   games/noahsark/tools/resync_mode.sh <name> <menu script> <mode> <cap> [seed] [difficulty 0|4|8] [rounds]
# Round k: the port plays with the events known so far, the original replays its clicks with the generator's state watched
# (needs the generator patch of tools/origrun/patches/noahsark.txt, rngcave3), resync.py writes the events; the last run
# watches the state, score, level and mode and is compared. Files go to build/noahtest/og/<name>.*.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT"
N="$1"; MENU="$2"; MODE="$3"; CAP="$4"; SEED="${5:-1}"; DIFF="${6:-0}"; ROUNDS="${7:-6}"
D=build/noahtest/og
mkdir -p "$D"
(cat "$MENU"; echo "$((CAP + 300)) quit") > "$D/$N.port.txt"
: > "$D/$N.resync"
run_port() {
	games/noahsark/tools/run_script.sh "$D/$N.port.txt" "build/noahtest/ud_$N" 200 -seed="$SEED" -autoplay="$SEED" -autoplaymenu \
		-autoplaymode="$MODE" -autoplaydiff="$DIFF" -autoplaycap="$CAP" -statelog="$D/$N.state" -autoplaylog="$D/$N.bot" \
		-resync="$D/$N.resync" > "$D/$N.log" 2>&1
	python -I games/noahsark/tools/origrun_replay.py script "$MENU" "$D/$N.bot" "$D/$N.orig.txt" "$((CAP + 200))"
}
prev=-1
for r in $(seq 1 "$ROUNDS"); do
	run_port
	rm -rf "build/origrun/$N"
	WATCHES='0x49c608|0x49c60c|0x49c614|0x49c618' SANDBOX=1 games/noahsark/tools/run_original.sh "$N" "$D/$N.orig.txt" "$SEED" > "$D/$N.out" 2>&1
	cp "build/origrun/$N/run.log" "$D/$N.cave.log"
	python -I games/noahsark/tools/resync.py "build/origrun/$N/run.log" "$D/$N.resync" | sed "s/^/round $r: /"
	n="$(cksum < "$D/$N.resync")"
	[ "$n" = "$prev" ] && break
	prev="$n"
done
run_port
rm -rf "build/origrun/$N"
SANDBOX=1 games/noahsark/tools/run_original.sh "$N" "$D/$N.orig.txt" "$SEED" > "$D/$N.out" 2>&1
# the update of each get-ready end is taken from a run that watches the board state, then the port runs once more
cp "build/origrun/$N/run.log" "$D/$N.std.log"
python -I games/noahsark/tools/resync.py "$D/$N.cave.log" "$D/$N.resync" "$D/$N.std.log" | sed "s/^/final: /"
run_port
rm -rf "build/origrun/$N"
SANDBOX=1 games/noahsark/tools/run_original.sh "$N" "$D/$N.orig.txt" "$SEED" > "$D/$N.out" 2>&1
python -I games/noahsark/tools/origrun_replay.py compare "build/origrun/$N/run.log" "$D/$N.state" | tail -${LINES_SHOWN:-8}
