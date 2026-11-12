#!/bin/bash
# all.sh: every coverage run of the first round, one after another (one game process at a time, and the port's bot between);
# the summary lines go to build/coverage/bookworm/summary.txt. OG_STAGE=C:/byo-og/bwp must be staged (setup.sh).
export OG_STAGE="${OG_STAGE:-C:/byo-og/bwp}"
cd "$(dirname "$0")/../../../.." || exit 1
R=games/bookworm/tools/coverage/run.sh; S=build/coverage/bookworm/summary.txt; mkdir -p build/coverage/bookworm
{
	bash "$R" tour menus menus 1 300
	bash "$R" case c1 classic 1 1
	bash "$R" case c2 both 2 7
	bash "$R" case m1 classic,action,classic 7 9
	bash "$R" save s1 classic 3 11 3000
	bash "$R" save s2 action 5 13 4000
	bash "$R" tour classic classic 7 400
	bash "$R" tour action action 8 600
	bash "$R" tour focusa focusa 1 300
	bash "$R" tour focusc focusc 1 300
	echo "all done"
} >> "$S" 2>&1
