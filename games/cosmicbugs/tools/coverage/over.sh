#!/bin/bash
# over.sh <name> <seed> [world]: a coverage run of the original that ends a game. The original's new game is patched to start with
# one life and every game over to earn a high-score entry (run.sh, LIVES1=1 and HS=1: the saved game a port's runner could write does not stick, as the profile's furthest level stays 0).
# Without a world it is a fresh profile: Start Game, the instructions page's Play, the board of level 1; with a world (0..8) it starts
# from the save with every board unlocked (the port's runner: `unlock all`) as worlds.sh does, through that world's button, a click
# on the slider page and "click here when done". Then 30 s of the random play of gen.py and the original's own play on until the last
# life is lost (origrun's waitfor on the lives, 0x460040), the game over's click, the high-score entry (two letters of its keyboard,
# "click here when done"), the Hall of Fame and the title. Each menu click has the mouse hover over its button 300 updates earlier
# (hover.py); a click on a font item lands on its box (the item's y plus 9 or 10). The runs are run.sh's.
cd "$(dirname "$0")/../../../.."
N=$1; SEED=$2; WORLD=$3
mkdir -p build/cbw build/coverage/cosmicbugs
W=("293 106" "434 122" "545 209" "607 344" "564 478" "426 524" "296 464" "203 354" "188 208")
P=build/coverage/cosmicbugs/prefix-$N.txt
if [ -z "$WORLD" ]; then
	printf '3000 click 175 313\n4000 click 400 563\n' > $P
	HOV=2; SAVE_ARG=
else
	until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done
	[ -f build/cbw/save577/cb.dat ] || {
		mkdir -p build/cbw/save577
		printf '500 cmd unlock all\n1500 end\n' > build/cbw/save577.txt
		CB_NOWARP=1 timeout 120 build/native/cosmicbugs_run.exe assets/cosmicbugs build/cbw/save577 build/cbw/save577.txt > build/cbw/save577.out 2>&1
	}
	printf '3000 click 175 313\n5500 click %s\n8000 click 400 310\n10000 click 400 563\n' "${W[$WORLD]}" > $P
	HOV=4; SAVE_ARG=build/cbw/save577/cb.dat
fi
python games/cosmicbugs/tools/play/gen.py $SEED 30000 $N $P
S=build/origrun/$N.txt
python -I games/cosmicbugs/tools/coverage/hover.py $S $HOV
head -n -2 $S > $S.t
cat >> $S.t <<EOT
waitfor 0x460040 == 0 900000
mark
at +3000 click 400 300 30
at +4000 move 100 300
at +5000 click 100 300 30
at +5500 move 150 300
at +6000 click 150 300 30
at +6500 move 400 520
at +7000 click 400 520 30
at +8500 move 400 545
at +9000 click 400 545 30
at +11000 move 0 0
quit
EOT
mv $S.t $S
SAVE=$SAVE_ARG LIVES1=1 HS=1 bash games/cosmicbugs/tools/coverage/run.sh $N $S 900
