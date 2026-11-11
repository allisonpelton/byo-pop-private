#!/bin/bash
# Runs the original (the copy staged by stage_original.sh under build/origrun/apc) through origrun on the virtual clock with the
# pinned seed, scripted input and the four watches, once there is room for another game process (CLAUDE.md: at most 3 at a time).
#   run_original.sh <script.txt> <log> [seed] [timeout]
# The run is inside Windows Sandbox (origrun --sandbox): a first run, with none of this machine's registry, which the original
# reads from HKLM and origrun cannot change (on the host the game starts as the user "Allison"'s machine leaves it, and the
# comparison has no clean state to start from). ORIG_HOST=1 runs on the host instead. The sandbox's watch lines are copied from the
# run's inside log into <log>.
# PROFILE=file starts the original with that profile (users/data1.usr: the port's record.py --make-profile makes one).
# RUN=name uses another staged copy. ORIGRUN_ARGS adds origrun options. KEEP_STATE=1 keeps the copy's players (a run starts with
# none, as the port's does: the title then leads to the new player box).
# Watches (the port's -trace prints the same four, at the same updates):
#   1 the board's score  2 its level  3 the game generator's position  4 Rand()'s position
# WATCHES="expr expr ..." replaces all four (no spaces inside an expression), e.g. the app's loading flags and screens:
#   [0x5dc220]+0x50c (loading started/completed bytes) [0x5dc220]+0x644 (main menu) [0x5dc220]+0x628 (board) [0x5dc220]+0x690 (user)
#
# The original loads on a thread of its own, in real time, while its counter runs on the virtual clock; on a busy machine the title
# is not yet loaded at the script's first click, and a click before that is lost: the run then never gets a board. A run that
# shows no board (no line of the log matches NEED, "watch 2: - -> " by default: the level watch's first value; set NEED for other
# WATCHES, or NEED=none for no check) is made again, up to TRIES times (5 by default).
script=$1; log=$2; seed=${3:-1}; timeout=${4:-600}
run=${RUN:-apc}
tries=${TRIES:-5}
need=${NEED:-'watch 2: - -> '}
cd "$(dirname "$0")/../../../.." || exit 2
B='[[0x5dc220]+0x628]'
W=()
if [ -n "$WATCHES" ]; then
  for w in $WATCHES; do W+=(--watch "$w"); done
else
  W=(--watch "$B+0x1974" --watch "$B+0x1940" --watch "[0x5a4e28]+0x9c0" --watch 0x5a6858)
fi
SBX=--sandbox; [ -n "$ORIG_HOST" ] && SBX=
st=0
for try in $(seq 1 "$tries"); do
  for i in $(seq 1 120); do
    python tools/gamecount.py --limit 3 > /dev/null && break
    sleep 15
  done
  [ -z "$KEEP_STATE" ] && rm -f build/origrun/$run/game/users/*
  [ -n "$PROFILE" ] && cp "$PROFILE" build/origrun/$run/game/users/data1.usr
  # the inside log of the run before (a run that never got its sandbox would otherwise be read as that one)
  rm -f build/origrun/$run/logs/origrun-sandbox.log
  # other sessions use the one Windows Sandbox in turn: the run waits in origrun's queue for up to an hour
  build/origrun-bin/origrun.exe --game astropop --run "$run" --exe "$PWD/build/origrun/$run/game/AstroPopGame.exe" \
    --seed "$seed" --clock virtual --cursor scripted --timeout "$timeout" --watch-max "${WATCH_MAX:-400000}" $SBX \
    ${SBX:+--sandbox-wait 3600} $ORIGRUN_ARGS "${W[@]}" --script "$script" > "$log" 2>&1
  st=$?
  if [ -n "$SBX" ]; then
    if [ ! -f build/origrun/$run/logs/origrun-sandbox.log ]; then
      echo "run_original.sh: try $try of $tries: the run got no sandbox (origrun exit $st: $(tail -1 "$log" | cut -c1-120)); again" >&2
      continue
    fi
    cat build/origrun/$run/logs/origrun-sandbox.log >> "$log"
  fi
  # a step the original did not reach within 30 s of real time (the machine was busy) was done at an earlier counter: the run is no
  # comparison of the script, so it is made again (SLOW_OK=1 keeps it)
  # (the last step of the script, the mark that ends it, may be out of reach for the same reason: the port's bot waited a long
  # time on a screen it could not leave; the original has shown the same game by then)
  lastat=$(grep -o '^at [0-9]*' "$script" | tail -1 | cut -d" " -f2)
  if [ -z "$SLOW_OK" ] && grep "not reached within" "$log" | grep -qv "counter $lastat "; then
    echo "run_original.sh: try $try of $tries: the original was too slow for a step of the script (\"not reached within\" in the log); again" >&2
    continue
  fi
  [ "$need" = none ] && break
  grep -q -- "$need" "$log" && break
  echo "run_original.sh: try $try of $tries showed no board (no line matches '$need'); the click on the title probably came before the game had loaded" >&2
done
exit $st
