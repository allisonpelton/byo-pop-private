#!/bin/bash
# Runs the original (the copy staged by stage_original.sh under build/origrun/apc) through origrun on the virtual clock with the
# pinned seed, scripted input and the four watches, once there is room for another game process (CLAUDE.md: at most 3 at a time).
#   run_original.sh <script.txt> <log> [seed] [timeout]
# The run is inside Windows Sandbox (origrun --sandbox): a first run, with none of this machine's registry, which the original
# reads from HKLM and origrun cannot change (on the host the game starts as the user "Allison"'s machine leaves it, and the
# comparison has no clean state to start from). ORIG_HOST=1 runs on the host instead. The sandbox's watch lines are copied from the
# run's inside log into <log>.
# RUN=name uses another staged copy. ORIGRUN_ARGS adds origrun options. KEEP_STATE=1 keeps the copy's players (a run starts with
# none, as the port's does: the title then leads to the new player box).
# Watches (the port's -trace prints the same four, at the same updates):
#   1 the board's score  2 its level  3 the game generator's position  4 Rand()'s position
# WATCHES="expr expr ..." replaces all four (no spaces inside an expression), e.g. the app's loading flags and screens:
#   [0x5dc220]+0x50c (loading started/completed bytes) [0x5dc220]+0x644 (main menu) [0x5dc220]+0x628 (board) [0x5dc220]+0x690 (user)
script=$1; log=$2; seed=${3:-1}; timeout=${4:-600}
run=${RUN:-apc}
cd "$(dirname "$0")/../../../.." || exit 2
for i in $(seq 1 120); do
  python tools/gamecount.py --limit 3 > /dev/null && break
  sleep 15
done
[ -z "$KEEP_STATE" ] && rm -f build/origrun/$run/game/users/*
B='[[0x5dc220]+0x628]'
W=()
if [ -n "$WATCHES" ]; then
  for w in $WATCHES; do W+=(--watch "$w"); done
else
  W=(--watch "$B+0x1974" --watch "$B+0x1940" --watch "[0x5a4e28]+0x9c0" --watch 0x5a6858)
fi
SBX=--sandbox; [ -n "$ORIG_HOST" ] && SBX=
build/origrun-bin/origrun.exe --game astropop --run "$run" --exe "$PWD/build/origrun/$run/game/AstroPopGame.exe" \
  --seed "$seed" --clock virtual --cursor scripted --timeout "$timeout" $SBX $ORIGRUN_ARGS \
  "${W[@]}" --script "$script" > "$log" 2>&1
st=$?
[ -n "$SBX" ] && cat build/origrun/$run/logs/origrun-sandbox.log >> "$log"
exit $st
