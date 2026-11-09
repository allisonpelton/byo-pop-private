#!/bin/bash
# all.sh: the coverage runs of GAME_NOTES.md's "Coverage check", one after another (run it from the worktree root, in the
# background). The shot tours of phase two on the host (make.py's "host" side has no shots), the tutorial and the saved game
# among them, and one real-clock run with sound.
source "$(dirname "$0")/../identical/common.sh"
G=build/coverage/alchemy; mkdir -p "$G/scripts"
host() { python games/alchemy/tools/shots/make.py "games/alchemy/tools/shots/$1.txt" "$G/scripts" host > "$G/scripts/$1.txt"; }
for t in 1 2 3 4 5 6 7; do host tour$t; done
# the saved game of tour 6: the port's own, written when it quits at update 1500 of a strategic game
if [ ! -f "$G/save/user/savegame.dat" ]; then
	rm -rf "$G/save"; mkdir -p "$G/save"
	"$TOOLS/port.sh" "$G/save" -seed=1 -autoplay=1 -autoplaymode=strategic -autoplaycap=1500 > /dev/null
fi
RUN="$(dirname "$0")/run.sh"
bash "$RUN" tour1 "$G/scripts/tour1.txt" 1
bash "$RUN" tour2 "$G/scripts/tour2.txt" 1 300
bash "$RUN" tour3 "$G/scripts/tour3.txt" 3 300
bash "$RUN" tour4 "$G/scripts/tour4.txt" 1 300
bash "$RUN" tour5 "$G/scripts/tour5.txt" 1
cp "$G/save/user/savegame.dat" "$OG_GAME/savegame.dat"
KEEP_SAVE=1 bash "$RUN" tour6 "$G/scripts/tour6.txt" 1
bash "$RUN" tour7 "$G/scripts/tour7.txt" 2 300
# real clock and the silent stand-in: the first 60 s of a bot game, at 100 updates a second
if [ -f build/perf/orig-play.txt ]; then REAL=1 bash "$RUN" real-minute build/perf/orig-play.txt 11 150; fi
echo "all done"
