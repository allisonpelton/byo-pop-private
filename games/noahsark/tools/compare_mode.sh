#!/usr/bin/env bash
# One series of the phase-one comparison: the port's bot plays a game (menus by a script, -autoplaymenu), the same clicks are
# replayed in the original (Windows Sandbox, virtual clock, generator pinned by the seed patch) and the watched state, score,
# level and mode are compared.
#   games/noahsark/tools/compare_mode.sh <name> <menu script> <mode> <cap> [seed] [difficulty 0|4|8]
# <menu script> is a -script file of "<update> click x y" lines (the title, the mode, the difficulty, the tutorial or the
# mode's dialog). Files go to build/noahtest/og/<name>.*; the result is the last line.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT"
N="$1"; MENU="$2"; MODE="$3"; CAP="$4"; SEED="${5:-1}"; DIFF="${6:-0}"
D=build/noahtest/og
mkdir -p "$D"
(cat "$MENU"; echo "$((CAP + 300)) quit") > "$D/$N.port.txt"
games/noahsark/tools/run_script.sh "$D/$N.port.txt" "build/noahtest/ud_$N" 200 -seed="$SEED" -autoplay="$SEED" -autoplaymenu \
	-autoplaymode="$MODE" -autoplaydiff="$DIFF" -autoplaycap="$CAP" -statelog="$D/$N.state" -autoplaylog="$D/$N.bot" \
	> "$D/$N.log" 2>&1
python -I games/noahsark/tools/origrun_replay.py script "$MENU" "$D/$N.bot" "$D/$N.orig.txt" "$((CAP + 200))"
rm -rf "build/origrun/$N"
SANDBOX=1 games/noahsark/tools/run_original.sh "$N" "$D/$N.orig.txt" "$SEED"
python -I games/noahsark/tools/origrun_replay.py compare "build/origrun/$N/run.log" "$D/$N.state" | tail -${LINES_SHOWN:-8}
