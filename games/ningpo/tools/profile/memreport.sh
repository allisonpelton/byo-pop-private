#!/bin/bash
# memreport.sh <out dir> [exe]: the port's memreport (docs/TEST_COMMANDS.md) at the loading screen, at the title and in a
# game, from fresh settings, under rundesk with -fast; waits for room for a game process first. The lines are in
# <out dir>/log.txt (grep MEMREPORT).
source "$(dirname "$0")/../identical/common.sh"
D="$1"; EXE="${2:-build/native/ningpo.exe}"
rm -rf "$D"; mkdir -p "$D/user"
printf '5 memreport\n150 click game 320 425\n250 memreport\n400 click game 340 440\n700 memreport\n800 quit\n' > "$D/s.txt"
until python -I tools/gamecount.py --limit 3 > /dev/null; do sleep 15; done
"$RUNDESK" "$(win "$ROOT/$EXE")" -gamedir=assets/ningpo -userdir="$D/user" -seed=7 -fast -realinput="$(win "$PWD/$D/s.txt")" > "$D/log.txt" 2>&1
grep MEMREPORT "$D/log.txt" | cut -c1-600
