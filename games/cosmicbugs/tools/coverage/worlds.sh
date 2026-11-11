#!/bin/bash
# worlds.sh [end ms [world numbers 0..8, default all]]: coverage runs of the original from a save with every board unlocked (the port's runner writes it:
# `unlock all`), one run a world of the select-world screen (docs/README.md, "Test commands"): Start Game, the world's
# button, a click on the slider page, "click here when done" (its box centre, 563), then the random play of gen.py (a click on a random cell every
# 1.5 to 3.5 seconds, the aim switched at random) up to <end ms> of game time (default 400000); each menu click has the mouse hover
# over its button 300 updates earlier (hover.py). With UP=<n> the runs are cov-wu<k> (another seed) and the slider page's up arrow (690, 312) is clicked n times
# instead (each click steps the level one up, to the world's last), which starts the world at a later level. A held drag of the slider's
# handle (origrun's down, move and up) did not move it in the original: gSliding stayed 0.
# The runs are run.sh's.
cd "$(dirname "$0")/../../../.."
END="${1:-400000}"
shift
KS="${*:-0 1 2 3 4 5 6 7 8}"
mkdir -p build/cbw/save577 build/coverage/cosmicbugs
until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 20; done
[ -f build/cbw/save577/cb.dat ] || {
	printf '500 cmd unlock all\n1500 end\n' > build/cbw/save577.txt
	CB_NOWARP=1 timeout 120 build/native/cosmicbugs_run.exe assets/cosmicbugs build/cbw/save577 build/cbw/save577.txt > build/cbw/save577.out 2>&1
}
W=("293 106" "434 122" "545 209" "607 344" "564 478" "426 524" "296 464" "203 354" "188 208")
for k in $KS; do
	NAME=cov-w$k; HOV=4; SEED=$((k + 11))
	[ -n "$UP" ] && { NAME=cov-wu$k; HOV=3; SEED=$((k + 31)); }
	P=build/coverage/cosmicbugs/prefix-$NAME.txt
	printf '3000 click 175 313\n5500 click %s\n8000 click 400 310\n10000 click 400 563\n' "${W[$k]}" > $P
	python games/cosmicbugs/tools/play/gen.py $SEED $END $NAME $P
	if [ -n "$UP" ]; then
		# the first click gets its hover (hover.py counts three menu clicks)
		L="at 7500 click 690 312 30"; for ((i = 1; i < UP; i++)); do L="$L\nat $((7500 + 100 * i)) click 690 312 30"; done
		sed -i "s/^at 7981 click 400 310 30\$/$L/" build/origrun/$NAME.txt
	fi
	python -I games/cosmicbugs/tools/coverage/hover.py build/origrun/$NAME.txt $HOV
	SAVE=build/cbw/save577/cb.dat bash games/cosmicbugs/tools/coverage/run.sh $NAME build/origrun/$NAME.txt 900
done
