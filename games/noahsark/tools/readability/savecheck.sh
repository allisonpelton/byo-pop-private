#!/bin/bash
# savecheck.sh <name>: a strategy game on hard (crates and blocks on the board) played by the bot for 2500 updates and left by
# closing the app, which saves the game in progress. From the worktree root. Keeps the saved game, the bot's log and the state
# log in build/readab/<name>/, so that two states of the source can be compared file by file: the readability pass's proof that
# putting the two obstacles' numbers the original's way round leaves the saved game's bytes and the play unchanged.
#     diff -r build/readab/save_before build/readab/save_after
cd "$(dirname "$0")/../../../.."
D="build/readab/$1"
rm -rf "$D"; mkdir -p "$D"
W="$(pwd -W)"
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done
timeout 300 build/native/rundesk.exe "$W/build/native/noahsark.exe" -gamedir="$W/assets/noahsark" -userdir="$W/$D/user" -fast -seed=1 \
	-autoplay=1 -autoplaymode=strategy -autoplaydiff=8 -autoplaycap=2500 -autoplaylog="$W/$D/bot.txt" -statelog="$W/$D/state.txt" \
	> "$D/run.log" 2>&1
echo "$1: exit $?; $(find "$D/user" -type f | wc -l) files in the user folder; $(wc -l < "$D/bot.txt") bot lines, $(wc -l < "$D/state.txt") state lines"
find "$D/user" -type f | sort | while read f; do echo "$(sha1sum < "$f" | cut -c1-12) $(stat -c %s "$f") ${f#$D/}"; done
