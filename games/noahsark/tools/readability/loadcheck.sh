#!/bin/bash
# loadcheck.sh <name> <exe> <saved game's user folder>: one build of the port opens a saved game through the menu's Continue,
# reports its state twice (the test command `state`: the board's rows among the rest), takes two screenshots and is closed,
# which saves the game again. From the worktree root. Keeps the STATE lines, the screenshots and the saved game written at the
# end in build/readab/<name>/, so that a build from before the two obstacles' numbers were put the original's way and the
# build after it can be compared on a saved game written before:
#     diff -r build/readab/load_old build/readab/load_new
cd "$(dirname "$0")/../../../.."
NAME="$1"; EXE="$(cygpath -w "$(cd "$(dirname "$2")" && pwd)/$(basename "$2")")"; SAVE="$3"
D="build/readab/$NAME"
rm -rf "$D" "build/readab/user_$NAME"; mkdir -p "$D" "build/readab/user_$NAME"
cp "$SAVE"/* "build/readab/user_$NAME/"
W="$(pwd -W)"
cat > "$D/script.txt" <<EOF
301 click 320 456
360 click 198 350
430 state
440 screenshot $W/$D/loaded.png
600 click 400 240
700 state
710 screenshot $W/$D/playing.png
800 quit
EOF
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done
timeout 300 build/native/rundesk.exe "$EXE" -gamedir="$W/assets/noahsark" -userdir="$W/build/readab/user_$NAME" -fast -seed=1 \
	-script="$W/$D/script.txt" > "$D/run.log" 2>&1
echo "$NAME: exit $?"
grep -a '^STATE' "$D/run.log" > "$D/state.txt"
cp "build/readab/user_$NAME/savegame.dat" "$D/savegame_after.dat" 2>/dev/null
rm -f "$D/run.log" "$D/script.txt"
rm -rf "build/readab/user_$NAME"
ls "$D" | tr '\n' ' '; echo
