#!/bin/bash
# sound_logs.sh <out folder> [run ...]: the port's sound log (BYOPOP_SOUNDLOG) of bot games, for the site's soundboard presets
# (tools/soundpreset.py: pass --ms 4, Venice's update is 4 ms, 250 a second). Each run is play.sh (this folder) under rundesk with -fast,
# one game process at a time, waiting for room under the 6-process limit. The log of a run is <out folder>/<run>.log, the driver's
# last line <out folder>/<run>.out. Run from anywhere; with no run named, all of them.
#   journey         a new player: the tutorial, level 1 won, the map, level 2 lost three times (about 5 minutes)
#   trickshot       Trick Shot from its first level, 2500 moves: shots made and missed (about 3 minutes)
#   flood           Flood from its first level, 2500 steps (the explaining screen needs --nulls 60)
#   survival        Survival, 2500 moves: power-ups, wings, minute markers, combos (about 7 minutes)
#   survival_lost   Survival's board lost on purpose (`lose`): rise, sink, the results and the rising water
#   cupid           a cupid (`cupid` test command) flies in and catches the one shot fired at its hover place (Journey level 3)
#   final_victory   the win scene (`goto win_sequence`): final_victory looping, then the high scores and the main menu
cd "$(dirname "$0")/../../../.." || exit 2
OUT="$1"; shift
[ -z "$OUT" ] && { echo "usage: sound_logs.sh <out folder> [journey|trickshot|flood|survival|survival_lost|cupid|final_victory ...]"; exit 2; }
mkdir -p "$OUT"; OUT="$(cd "$OUT" && pwd -W)"
RUNS=("$@"); [ ${#RUNS[@]} -eq 0 ] && RUNS=(journey trickshot flood survival survival_lost cupid final_victory)
for run in "${RUNS[@]}"; do
	export BYOPOP_SOUNDLOG="$OUT/$run.log"; rm -f "$BYOPOP_SOUNDLOG"
	P=games/venice/tools/realinput/play.sh
	case "$run" in
		journey) bash $P journey 2d 2500 ;;
		trickshot) bash $P trickshot 2d 2500 ;;
		flood) bash $P flood 2d 2500 --nulls 60 ;;
		survival) bash $P survival 2d 2500 ;;
		survival_lost) bash $P survival 2d 200 --nulls 20 --first "wait 1500" --first "lose" ;;
		cupid)
			# the level's GO click first (a click before it only starts the level), then the cupid, and a shot at the place it
			# hovers at (x 40) about 100 updates after it comes in: the flight in takes 301 updates and only then can it catch
			bash $P journey 2d 5 --nulls 3 --first "unlock all" --first "level 3" --first "wait 600" \
				--first "move game 400 300" --first "wait 3" --first "down game 400 300" --first "wait 3" --first "up game 400 300" \
				--first "wait 500" --first "cupid" --first "wait 100" --first "move game 40 300" --first "wait 40" \
				--first "down game 40 300" --first "wait 3" --first "up game 40 300" --first "wait 600" ;;
		final_victory) bash $P journey 2d 30 --nulls 5 --first "unlock all" --first "goto win_sequence" --first "wait 3000" ;;
		*) echo "unknown run $run"; continue ;;
	esac > "$OUT/$run.out" 2>&1
	tail -1 "$OUT/$run.out" | cut -c1-200
done
