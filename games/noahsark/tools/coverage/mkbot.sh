#!/bin/bash
# mkbot.sh <name> <mode> <difficulty 0|4|8> <cap> [seed] [quit update]: the port's bot plays a game (as compare_mode.sh does) and
# origrun_replay.py turns its clicks into the original's script: build/coverage/noahsark/scripts/<name>.orig.txt. The menu
# clicks come from menu/<mode>.txt (the title's link, the mode's button, the difficulty and the dialog's close; the difficulty
# is made here). The port's run ends at <cap> updates (the original gets its quit at <quit update>, default <cap> + 300).
# With STOP=<n> in the environment the bot makes no more moves after update n and the run goes on to <cap>: a game it leaves
# alone is lost to the flood, and the bot still answers the game-over screens (the name, the list, the link back). Waits for room.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
N="$1"; MODE="$2"; DIFF="$3"; CAP="$4"; SEED="${5:-1}"; QUIT="${6:-$((CAP + 300))}"
D=build/coverage/noahsark/scripts; mkdir -p "$D"
case "$MODE" in action) B=188; C="400 405";; strategy) B=242; C="400 405";; puzzle) B=296; C="330 335";; esac
case "$DIFF" in 0) Y=195;; 4) Y=240;; 8) Y=285;; esac
printf '301 click 320 456\n360 click 198 %s\n381 click 320 %s\n420 click %s\n' "$B" "$Y" "$C" > "$D/$N.menu.txt"
(cat "$D/$N.menu.txt"; echo "$QUIT quit") > "$D/$N.port.txt"
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done
rm -rf "build/coverage/noahsark/ud_$N"
games/noahsark/tools/run_script.sh "$D/$N.port.txt" "build/coverage/noahsark/ud_$N" 300 -seed="$SEED" -autoplay="$SEED" -autoplaymenu \
	-autoplaymode="$MODE" -autoplaydiff="$DIFF" -autoplaycap="$CAP" ${STOP:+-autoplaystop=$STOP} -autoplaylog="$D/$N.bot" -statelog="$D/$N.state" > "$D/$N.log" 2>&1
python -I games/noahsark/tools/origrun_replay.py script "$D/$N.menu.txt" "$D/$N.bot" "$D/$N.orig.txt" "$((QUIT - 100))"
tail -2 "$D/$N.log" | cut -c1-200
