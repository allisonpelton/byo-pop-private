#!/bin/bash
# gap_all.sh: the runs for what the others miss (docs/COVERAGE.md, "Coverage check" in the game's notes), one at a time:
#   ankh     the Ankh asked what it is
#   options  the sliders dragged, the switches, the Tutorial button's question
#   slots    five saved adventures (advsaves: five copies of one adventure save, adv0.sav to adv4.sav, named Bot): the list, the delete, a new game's name box (comma, in use, accepted)
#   rank     classic8's game with Enter in the name box of the high score instead of its OK
# Start it directly from the worktree.
R="$(pwd -W)"; T=games/mummymaze/tools/coverage; D=build/coverage/mummymaze/scripts
for n in ankh options slots; do
	[ -s build/coverage/mummymaze/hits/$n.txt ] && continue
	SAVES="$R/$D/advsaves" bash $T/gap.sh $T/scenarios/$n.txt $n 21 150
done
[ -s build/coverage/mummymaze/hits/rank.txt ] || bash $T/run.sh rank "$R/$D/rank.script.txt" 7 $(( ($(cat $D/classic8.last) + 3000) / 300 + 60 ))
