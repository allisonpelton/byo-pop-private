#!/usr/bin/env bash
# Phase two from a saved game: both start from the same savegame.dat (the port writes the original's format), the menu script
# clicks Continue, the port's bot plays on and its clicks are replayed in the original, both shoot at the given updates.
# The saved game carries the generator's state, so the two play the same boards.
#   games/noahsark/tools/phase2_save.sh <name> <savegame.dat> <menu script> <mode> <cap> <difficulty> <shots: "update:name ...">
# Files go to build/noahtest/p2/<name>/ (port/, orig/, port.txt, orig.txt, bot); the shotdiff run is <name>.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT"
N="$1"; SAVE="$2"; MENU="$3"; MODE="$4"; CAP="$5"; DIFF="$6"; SHOTS="$7"
D="build/noahtest/p2/$N"
rm -rf "$D"; mkdir -p "$D/port" "$D/orig"
ABS="$(cd "$D" && pwd -W 2>/dev/null || pwd)"
( cat "$MENU"
  for s in $SHOTS; do echo "${s%%:*} screenshot $ABS/port/${s#*:}.png"; done
  echo "$((CAP + 300)) quit" ) | sort -n -s -k1,1 > "$D/port.txt"
U="build/noahtest/ud_p2_$N"; rm -rf "$U"; mkdir -p "$U"; cp "$SAVE" "$U/savegame.dat"
games/noahsark/tools/run_script.sh "$D/port.txt" "$U" 300 -seed=1 -drawevery -autoplay=1 -autoplaymenu \
	-autoplaymode="$MODE" -autoplaydiff="$DIFF" -autoplaycap="$CAP" -autoplaylog="$D/bot" > "$D/port.log" 2>&1
python -I games/noahsark/tools/origrun_replay.py script "$MENU" "$D/bot" "$D/orig.base" "$((CAP + 200))"
( grep -v '^quit' "$D/orig.base"
  for s in $SHOTS; do echo "at ${s%%:*} shot ${s#*:}"; done ) > "$D/orig.unsorted"
( grep '^at ' "$D/orig.unsorted" | sort -n -s -k2,2; echo quit ) > "$D/orig.txt"
R="build/origrun/p2_$N"; rm -rf "$R" "C:/byo-og/p2_$N"
build/origrun-bin/origrun.exe --game noahsark --run "p2_$N" --copy-only > "$D/stage.log" 2>&1
mkdir -p "$R/game/data"; cp "$SAVE" "$R/game/data/savegame.dat"
build/origrun-bin/origrun.exe --game noahsark --run "p2_$N" --clock virtual --seed 1 --sandbox \
	--counter "[0x4d125c]+0x368" --watch "[[0x4d125c]+0x408]+0x310" --watch "[0x4d125c]+0x474" --watch "[0x4d125c]+0x43c" --watch "[0x4d125c]+0x444" \
	--script "$D/orig.txt" --timeout 250 > "$D/orig.out" 2>&1
cp "$R/shots/"*.png "$D/orig/" 2>/dev/null
python tools/shotdiff/shotdiff.py --game noahsark --orig "$D/orig" --port "$D/port" --run "$N"
