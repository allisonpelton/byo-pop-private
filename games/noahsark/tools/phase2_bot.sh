#!/usr/bin/env bash
# Phase two with the port's bot: the bot plays (as in compare_mode.sh), the same clicks are replayed in the original, and both
# take pictures at the given updates, then shotdiff compares them (the board of the first level is the same in both, so the
# pictures of the first level, up to the level-up panel, can be compared; later levels' boards differ without resync).
#   games/noahsark/tools/phase2_bot.sh <name> <menu script> <mode> <cap> <difficulty 0|4|8> <shots: "update:name ...">
# REPLAY_CLICK_DELAY=n sends the bot's clicks to the original n updates later (default 0: a delay of 1 makes a whole game diverge).
# Files go to build/noahtest/p2/<name>/ (port/, orig/, port.txt, orig.txt, .bot); the shotdiff run is <name>.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT"
N="$1"; MENU="$2"; MODE="$3"; CAP="$4"; DIFF="$5"; SHOTS="$6"
D="build/noahtest/p2/$N"
rm -rf "$D"; mkdir -p "$D/port" "$D/orig"
ABS="$(cd "$D" && pwd -W 2>/dev/null || pwd)"
( cat "$MENU"
  for s in $SHOTS; do echo "${s%%:*} screenshot $ABS/port/${s#*:}.png"; done
  echo "$((CAP + 300)) quit" ) | sort -n -s -k1,1 > "$D/port.txt"
rm -rf "build/noahtest/ud_p2_$N"
games/noahsark/tools/run_script.sh "$D/port.txt" "build/noahtest/ud_p2_$N" 300 -seed=1 -drawevery -autoplay=1 -autoplaymenu \
	-autoplaymode="$MODE" -autoplaydiff="$DIFF" -autoplaycap="$CAP" -autoplaylog="$D/bot" > "$D/port.log" 2>&1
REPLAY_CLICK_DELAY=${REPLAY_CLICK_DELAY:-0} python -I games/noahsark/tools/origrun_replay.py script "$MENU" "$D/bot" "$D/orig.base" "$((CAP + 200))"
( grep -v '^quit' "$D/orig.base"
  for s in $SHOTS; do echo "at ${s%%:*} shot ${s#*:}"; done ) > "$D/orig.unsorted"
# the original's script wants ascending counters: sort stable by the number after "at", the quit last
( grep '^at ' "$D/orig.unsorted" | sort -n -s -k2,2; echo quit ) > "$D/orig.txt"
rm -rf "build/origrun/p2_$N"
SANDBOX=1 games/noahsark/tools/run_original.sh "p2_$N" "$D/orig.txt" 1 > "$D/orig.out" 2>&1
cp "build/origrun/p2_$N/shots/"*.png "$D/orig/" 2>/dev/null; rm -f "$D/orig/landed.tsv"
python tools/shotdiff/shotdiff.py --game noahsark --orig "$D/orig" --port "$D/port" --run "$N"
