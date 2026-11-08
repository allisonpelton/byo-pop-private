#!/bin/bash
# steady.sh <out dir> [mod|nomod]: the fixes mod's steadyClock checked in real time. The port plays the same TSUNAMI input
# twice on seed 1 in real time (no -fast), with the fixes mod (mod) or without it (nomod), and the two state logs are
# compared: with the mod they must be the same, update for update. Under rundesk; about a minute a run.
source "$(dirname "$0")/common.sh"
D="$1"; WHICH="${2:-mod}"
mkdir -p "$D"
# the menu, TSUNAMI, level 1, the help's PLAY GAME, then three balls released by clicks on the spawn button and two right
# clicks, the game left running to 1800
cat > "$D/script.txt" <<'S'
480 move 447 237
500 click 447 237
540 move 300 200
560 click 300 200
640 move 165 415
660 click 165 415
900 move 678 82
910 click 678 82
1100 click 678 82
1300 click 678 82
1500 rclick 400 300
1700 rclick 400 300
1800 quit
S
for run in 1 2; do
	R="$D/$WHICH$run"; rm -rf "$R"; mkdir -p "$R/user"
	MODS=(-nomods)
	if [ "$WHICH" = mod ]; then rm -rf "$R/mods"; mkdir -p "$R/mods"; cp -r games/wordharmony/mods/fixes "$R/mods/"; MODS=(-modsdir="$(win "$R/mods")"); fi
	"$RUNDESK" "$(win build/native/wordharmony.exe)" -gamedir="$(win assets/wordharmony)" -userdir="$(win "$R/user")" "${MODS[@]}" \
		-seed=1 -script="$(win "$D/script.txt")" -statelog="$(win "$R/state.txt")" > "$R/out.txt" 2>&1
done
if cmp -s "$D/${WHICH}1/state.txt" "$D/${WHICH}2/state.txt"; then
	echo "$WHICH: the two runs' state logs are the same ($(wc -l < "$D/${WHICH}1/state.txt") lines)"
else
	echo "$WHICH: the runs differ from line $(cmp "$D/${WHICH}1/state.txt" "$D/${WHICH}2/state.txt" | awk '{print $NF}')"
fi
