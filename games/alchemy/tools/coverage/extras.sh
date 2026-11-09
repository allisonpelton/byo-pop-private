#!/bin/bash
# extras.sh: the short runs of GAME_NOTES.md's "Coverage check" for what the tours miss (run from the worktree root, in the
# background, after all.sh's tours: it needs build/coverage/alchemy/scripts/ and the saved game of build/coverage/alchemy/save).
# origrun scripts written here, one per run, on the host, the virtual clock, no sound device. A `quit` line is origrun's close of
# the window ("ask the window to close, and end the run"), which the original answers by ending at once, with or without a
# question open. The positions are the port's `state` answers for the same screens (the quit question with a game in progress has
# its buttons at y 284, the other questions at 204; the options dialog is taller in the original, its Done button at 338).
source "$(dirname "$0")/../identical/common.sh"
G=build/coverage/alchemy; S=$G/scripts; mkdir -p "$S"
RUN="$(dirname "$0")/run.sh"
PRE=$'at 200 focus regain\nat 300 move 390 244\nat 310 click 390 244'
# cut <tour> <update>: the host script of a tour up to that update (all.sh made it), then the window's close
cut() { awk -v u="$2" '$1 == "at" && $2 + 0 > u + 0 { exit } { print }' "$S/$1.txt"; echo quit; }
# the saved game's start: the title, the selector's start button and the load question answered yes
LOAD=$'at 3000 click 320 441\nat 3009 click 527 442\nat 3039 click 325 244'

# 1. the saved game: the forge gauge, the hint, options and help in a game, the tutorial question answered no and yes (a game
#    in progress is replaced), the new game question answered no and yes
cat > "$S/ex-saved.txt" <<EOT
$PRE
$LOAD
at 3099 click 75 370
at 3129 click 33 423
at 3149 click 395 204
at 3169 click 72 241
at 3189 click 318 237
at 3209 click 290 434
at 3219 click 465 204
at 3229 click 520 432
at 3249 click 73 123
at 3259 click 465 204
at 3279 click 73 123
at 3289 click 325 204
at 3329 move 320 300
quit
EOT
# 2. the tutorial question answered yes in a game: the tutorial replaces the game
cat > "$S/ex-tutq.txt" <<EOT
$PRE
$LOAD
at 3169 click 72 241
at 3189 click 318 237
at 3209 click 290 434
at 3219 click 325 204
at 3400 click 73 449
at 3420 move 320 300
quit
EOT
# 3. quitting a game in progress: the question answered no, then yes (the game is saved as the window ends)
cat > "$S/ex-quit.txt" <<EOT
$PRE
$LOAD
at 3139 click 73 449
at 3159 click 465 284
at 3179 click 73 449
at 3199 click 325 284
at 3300 move 320 300
quit
EOT
# 4. the window closed in each state: the title, the selector, the options dialog, the help, the high scores, the quit question,
#    a game in progress, the game over, the name entry and the table after it
printf '%s\nat 1500 move 320 300\nquit\n' "$PRE" > "$S/ex-close-title.txt"
printf '%s\nat 3000 click 320 441\nat 3200 move 320 300\nquit\n' "$PRE" > "$S/ex-close-selector.txt"
printf '%s\nat 3000 click 320 441\nat 3040 click 318 372\nat 3043 click 72 241\nat 3100 move 320 300\nquit\n' "$PRE" > "$S/ex-close-options.txt"
printf '%s\nat 3000 click 320 441\nat 3040 click 318 372\nat 3043 click 72 241\nat 3069 click 318 237\nat 3150 move 320 300\nquit\n' "$PRE" > "$S/ex-close-help.txt"
printf '%s\nat 3000 click 320 441\nat 3040 click 318 372\nat 3043 click 72 241\nat 3109 click 470 237\nat 3200 move 320 300\nquit\n' "$PRE" > "$S/ex-close-scores.txt"
printf '%s\nat 3000 click 320 441\nat 3040 click 318 372\nat 3147 click 72 448\nat 3200 move 320 300\nquit\n' "$PRE" > "$S/ex-close-quitq.txt"
cut tour2 5200 > "$S/ex-close-play.txt"
cut tour2 15200 > "$S/ex-close-over.txt"
cut tour2 15484 > "$S/ex-close-name.txt"
cut tour2 15530 > "$S/ex-close-table.txt"

# the saved game of tour 6 for the first three runs, put back before each
for n in saved tutq quit; do cp "$G/save/user/savegame.dat" "$OG_GAME/savegame.dat"; KEEP_SAVE=1 bash "$RUN" ex-$n "$S/ex-$n.txt" 1 120; done
for n in close-title close-selector close-options close-help close-scores close-quitq close-play close-over close-name close-table; do
	bash "$RUN" ex-$n "$S/ex-$n.txt" 1 200
done
echo "extras done"
