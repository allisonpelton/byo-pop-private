#!/bin/bash
# Runs the original (a copy staged under build/origrun/bej2) through origrun on the virtual clock with the pinned seed, scripted
# input and the four board watches, once there is room for another game process (CLAUDE.md: at most 3 at a time).
#   run_original.sh <script.txt> <log> [seed] [timeout]
# ORIGRUN_ARGS="--dgvoodoo E:/retools/dgVoodoo2_87_5 --3d on" adds origrun options (the 3D, 1024 by 768 original).
# WATCHES="expr expr ..." replaces all four watches (no spaces inside an expression).
# W2=0x67d910 reads the global generator's index in the place of the level (the port's watch 5).
# Watches: the board's score, level, state and generator index (the port's -trace prints the same four).
script=$1; log=$2; seed=${3:-1}; timeout=${4:-600}
cd "$(dirname "$0")/../../../.." || exit 2
for i in $(seq 1 120); do
  python tools/gamecount.py --limit 3 > /dev/null && break
  sleep 15
done
# a run starts as a first run does: the original keeps its players in C:/ProgramData/Steam/Bejeweled2/users and its settings in
# HKCU/Software/Steam/Bejeweled2, and with a player there the title leads to the welcome-back screen and not to the name dialog
# (the user allows changes to PopCap games' registry values and saves, 2026-10-31 and 2026-11-01). KEEP_STATE=1 leaves them.
setup_state() {
  if [ -z "$KEEP_STATE" ]; then
    rm -rf /c/ProgramData/Steam/Bejeweled2/users
    reg delete 'HKCU\Software\Steam\Bejeweled2' //f > /dev/null 2>&1
    # windowed, as the port's driver asks for: under dgVoodoo2 the first run would otherwise be full screen and take no posted click
    reg add 'HKCU\Software\Steam\Bejeweled2' //v ScreenMode //t REG_DWORD //d 0 //f > /dev/null 2>&1
    # PROFILE=<folder of make_profile.py>: the player with the secret modes unlocked is there already
    if [ -n "$PROFILE" ]; then mkdir -p /c/ProgramData/Steam/Bejeweled2 && cp -r "$PROFILE/users" /c/ProgramData/Steam/Bejeweled2/users; fi
  fi
}
B='[[0x67ccd8]+0xe68]'
W=()
if [ -n "$WATCHES" ]; then
  for w in $WATCHES; do W+=(--watch "$w"); done
else
  W=(--watch "$B+0x15e20" --watch "${W2:-$B+0x15e4}" --watch "$B+0x170c" --watch "$B+0x15e0")
fi
# A run that never reaches a board (no value on the state watch) is a flake of the copy, not a difference of the game: it happened to the
# first run after another one, in 2D and 3D alike, and a second run is as the first should have been. Retried up to twice.
for try in 1 2 3; do
  setup_state
  build/origrun-bin/origrun.exe --game bejeweled2 --run bej2 --exe "$PWD/build/origrun/bej2/game/Bejeweled2Game.exe"     --seed "$seed" --clock virtual --cursor scripted --timeout "$timeout" $ORIGRUN_ARGS "${W[@]}" --script "$script" > "$log" 2>&1
  rc=$?
  [ -n "$WATCHES" ] && exit $rc
  grep -qE 'watch 3: .* -> [0-9]+ at update' "$log" && exit $rc
  echo "run_original.sh: no board in the run ($try), trying again" >&2
  sleep 5
done
exit $rc
