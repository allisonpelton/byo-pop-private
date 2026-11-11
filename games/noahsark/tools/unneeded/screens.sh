#!/bin/bash
# screens.sh <name> <exe>: the screen tour (tour.txt beside this file) on one build of the port, for a before and after comparison
# ("Code the port does not need", GAME_NOTES.md). From the worktree root. Waits for room (three game processes on the machine),
# runs the exe under rundesk with -fast and -seed=1 in a fresh user folder, and writes the screenshots to
# build/unneeded/<name>/port/ and their hashes to build/unneeded/<name>.sha (one line a screenshot). Two builds draw the same
# when their .sha files are the same: cmp build/unneeded/before.sha build/unneeded/after.sha.
cd "$(dirname "$0")/../../../.."
NAME="$1"; EXE="$(cygpath -w "$(cd "$(dirname "$2")" && pwd)/$(basename "$2")")"
D="build/unneeded/$NAME"
rm -rf "$D" "build/unneeded/user_$NAME"; mkdir -p "$D"
python -I games/noahsark/tools/tour.py games/noahsark/tools/unneeded/tour.txt "$D" > /dev/null
until python -I tools/gamecount.py --limit 3 > /dev/null; do sleep 15; done
timeout 300 build/native/rundesk.exe "$EXE" -gamedir="$(pwd -W)/assets/noahsark" -userdir="$(pwd -W)/build/unneeded/user_$NAME" -fast -seed=1 \
  -script="$(pwd -W)/$D/port.txt" > "$D/run.log" 2>&1
echo "$NAME: exit $?, $(ls "$D/port" | wc -l) screenshots"
(cd "$D/port" && sha1sum *.png) > "build/unneeded/$NAME.sha"
