#!/bin/bash
# gen.sh <out dir>: the bot's input for pairs.sh: the port's bot plays a strategic game (game seed 11, bot seed 1) through
# the mouse under rundesk with -fast, to update 7400, and its clicks go to <out dir>/bot.txt as -script lines
# (identical/toorig.py turns them into the original's). Run from the worktree root.
cd "$(dirname "$0")/../../../.." || exit 2
D="$1"
mkdir -p "$D/gen"
OUT="$D" bash games/alchemy/tools/identical/port.sh "$D/gen" -seed=11 -autoplay=1 -autoplayui -autoplaystart=450 -autoplaymode=strategic \
	-autoplaycap=7400 "-autoplaylog=$D/gen/log.txt" "-autoplayscript=$D/gen/script.txt"
cp "$D/gen/script.txt" "$D/bot.txt"
tail -1 "$D/gen/log.txt"
