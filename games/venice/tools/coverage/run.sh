#!/bin/bash
# run.sh <name> <idplay scenario>: one coverage run of the original (docs/COVERAGE.md) on the host: games/venice/tools/idplay's
# `original` action (the virtual clock, no sound device, the scenario's inputs) with origrun's --coverage on the list that
# `python -I tools/coverage.py list venice` writes, in its own copy of the game (build/origrun/venice-cov-<name>, a copy of the
# patched game of build/origrun/venice; removed when the run is done) so that it never meets a sandbox run of the shots tours or another coverage
# run. The hits go to build/coverage/venice/hits/<name>.txt, the run's log beside them.
cd "$(dirname "$0")/../../../.." || exit 2
NAME="$1"; SCENARIO="$2"
H=build/coverage/venice/hits; mkdir -p "$H"
export COVERAGE="$(pwd -W)/build/coverage/venice/functions.txt"
export VENICE_RUN="venice-cov-$NAME"
if [ -n "${POWERUP_KIND:-}" ] || [ -n "${EXTRA_PATCH:-}" ]; then
	# every power-up of this run is of one kind (0 wings, 1 power shot, 2 rewind, 3 wild, 4 combo shot, 5 extra try, 6 score multiplier,
	# 7 cupid), so that the catch of a kind the draws seldom give reaches its code: in the Powerup constructor (0x4165b0) the
	# tutorial's "kind 1" store becomes unconditional with this kind (the original then plays on differently from the port, which
	# the run does not mind: it only reads the inputs the port's bot made). EXTRA_PATCH is one more line of origrun's patch list
	# (the basket run's "va:0x4010fd" makes every falling jewel land in a bonus basket that waits)
	P="build/coverage/venice/patches-$NAME.txt"
	{ cat tools/origrun/patches/venice.txt
	  [ -n "${POWERUP_KIND:-}" ] && echo "kind VeniceGame.exe va:0x4166df 740d8b55f0c78248010000.01000000 90908b55f0c78248010000.0${POWERUP_KIND}000000 every power-up of kind ${POWERUP_KIND} (a coverage run)"
	  [ -n "${EXTRA_PATCH:-}" ] && echo "$EXTRA_PATCH"
	  true; } > "$P"
	export VENICE_PATCHES="$(pwd -W)/$P"
fi
R="build/origrun/$VENICE_RUN"
rm -rf "$R"; mkdir -p "$R"; cp -r build/origrun/venice/game "$R/game"  # the sandbox runs' copy of the game, patched
python -I games/venice/tools/idplay/idplay.py original "$SCENARIO" > "$H/$NAME.log" 2>&1  # it then fails to compare with a port run that was not made: nothing to see
cp "$R/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file"; exit 1; }
rm -rf "$R"
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1)"
