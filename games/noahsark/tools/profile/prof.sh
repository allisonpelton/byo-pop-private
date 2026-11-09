#!/bin/bash
# prof.sh <name> <seconds> <port script> [game args]: a sampling CPU profile of the port (games/bejeweled/tools/profile/cpuprof.py: the
# threads' instruction pointers about every millisecond, named from the exe's symbols) for the performance pass (GAME_NOTES.md, section 10).
# From the worktree root. Waits for room (three game processes on the machine), runs the port under rundesk with the script (which must
# end in a quit, or the profiler leaves the game running) and writes build/perf2/prof_<name>.txt. Real time unless the game args
# say -fast.
cd "$(dirname "$0")/../../../.."
NAME="$1"; SECS="$2"; SCRIPT="$3"; shift 3
W=$(pwd -W)
mkdir -p build/perf2
until python -I tools/gamecount.py --limit 3 > /dev/null; do sleep 15; done
rm -rf "build/perf2/user_$NAME"
timeout 600 build/native/rundesk.exe python -I games/bejeweled/tools/profile/cpuprof.py "build/perf2/prof_$NAME.txt" "$SECS" -- \
  "$W/build/native/noahsark.exe" -gamedir="$W/assets/noahsark" -userdir="$W/build/perf2/user_$NAME" -seed=1 -script="$W/$SCRIPT" "$@" \
  > "build/perf2/prof_$NAME.log" 2>&1
echo "$NAME: exit $?"
