#!/bin/bash
# Runs the original (a copy staged under build/origrun/bej2) through origrun on the virtual clock with the pinned seed, scripted
# input and the four board watches, once there is room for another game process (CLAUDE.md: at most 3 at a time).
#   run_original.sh <script.txt> <log> [seed] [timeout]
# Watches: the board's score, level, state and generator index (the port's -trace prints the same four).
script=$1; log=$2; seed=${3:-1}; timeout=${4:-600}
cd "$(dirname "$0")/../../../.." || exit 2
for i in $(seq 1 120); do
  python tools/gamecount.py --limit 3 > /dev/null && break
  sleep 15
done
B='[[0x67ccd8]+0xe68]'
exec build/origrun-bin/origrun.exe --game bejeweled2 --run bej2 --exe "$PWD/build/origrun/bej2/game/Bejeweled2Game.exe" \
  --seed "$seed" --clock virtual --cursor scripted --timeout "$timeout" \
  --watch "$B+0x15e20" --watch "$B+${LEVELOFF:-0x15e4}" --watch "$B+0x170c" --watch "$B+0x15e0" --script "$script" > "$log" 2>&1
