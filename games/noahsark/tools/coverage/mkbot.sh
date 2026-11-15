#!/bin/bash
# mkbot.sh <name> <mode> <difficulty 0|4|8> <cap> [seed] [quit update]: the port's bot plays a game (as compare_mode.sh does) and
# origrun_replay.py turns its clicks into the original's script: build/coverage/noahsark/scripts/<name>.orig.txt. The menu
# clicks come from menu/<mode>.txt (the title's link, the mode's button, the difficulty and the dialog's close; the difficulty
# is made here). The port's run ends at <cap> updates (the original gets its quit at <quit update>, default <cap> + 300).
# With STOP=<n> in the environment the bot makes no more moves after update n and the run goes on to <cap>: a game it leaves
# alone is lost to the flood, and the bot still answers the game-over screens (the name, the list, the link back). Waits for room.
# MENU_AT=<update> (default 301) is the update of the title's click; the other three menu clicks follow it as before. The original's
# loading thread runs on the wall clock while its updates run ahead on the virtual one, so on a loaded machine its play link comes
# up thousands of updates late (3160 and 956 in two runs of 2026-11-14) and a click at 301 is lost: the game stays on its title.
# A later MENU_AT moves the whole game later in both games (nothing draws a random number before a game starts). <cap> and
# <quit update> still count from the run's start, so add the shift to them. The original's script also waits for the link first.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
N="$1"; MODE="$2"; DIFF="$3"; CAP="$4"; SEED="${5:-1}"; QUIT="${6:-$((CAP + 300))}"
D=build/coverage/noahsark/scripts; mkdir -p "$D"
case "$MODE" in action) B=188; C="400 405";; strategy) B=242; C="400 405";; puzzle) B=296; C="330 335";; esac
case "$DIFF" in 0) Y=195;; 4) Y=240;; 8) Y=285;; esac
T="${MENU_AT:-301}"
printf '%s click 320 456\n%s click 198 %s\n%s click 320 %s\n%s click %s\n' "$T" "$((T + 59))" "$B" "$((T + 80))" "$Y" "$((T + 119))" "$C" > "$D/$N.menu.txt"
(cat "$D/$N.menu.txt"; echo "$QUIT quit") > "$D/$N.port.txt"
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done
rm -rf "build/coverage/noahsark/ud_$N"
games/noahsark/tools/run_script.sh "$D/$N.port.txt" "build/coverage/noahsark/ud_$N" 300 -seed="$SEED" -autoplay="$SEED" -autoplaymenu \
	-autoplaymode="$MODE" -autoplaydiff="$DIFF" -autoplaycap="$CAP" ${STOP:+-autoplaystop=$STOP} -autoplaylog="$D/$N.bot" -statelog="$D/$N.state" > "$D/$N.log" 2>&1
python -I games/noahsark/tools/origrun_replay.py script "$D/$N.menu.txt" "$D/$N.bot" "$D/$N.orig.txt" "$((QUIT - 100))"
# the play link's flag (the title screen at app+0x414, its link shown at +0x68): the script holds until it is up
sed -i '1i waitfor [[0x4d125c]+0x414]+0x68 != 0 120000' "$D/$N.orig.txt"
tail -2 "$D/$N.log" | cut -c1-200
