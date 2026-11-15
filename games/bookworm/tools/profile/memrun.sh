#!/bin/bash
# memrun.sh <scenario> <out dir>: the port plays a scenario with `memreport` lines in it (docs/TEST_COMMANDS.md: "<update> memreport
# mem/<name>.txt"), -fast, under rundesk, in <out dir>; the MEMREPORT lines are in <out dir>/game.log and the detail in <out dir>/mem/.
# From the worktree root. Waits for a free game slot first.
SCEN=$1; OUT=$2
ROOT=$(pwd)
win() { cygpath -w "$1"; }
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done
rm -rf "$OUT"; mkdir -p "$OUT/mem" "$OUT/user" "$OUT/mods"
( cd "$OUT" && "$ROOT/build/native/rundesk.exe" "$(win "$ROOT/build/native/bookworm.exe")" -gamedir="$(win "$ROOT/assets/bookworm")" -userdir=user -modsdir=mods -fast -seed=1 -script="$(win "$ROOT/$SCEN")" > game.log 2>&1 )
grep -c MEMREPORT "$OUT/game.log"
