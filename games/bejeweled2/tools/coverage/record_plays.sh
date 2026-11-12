#!/bin/bash
# record_plays.sh: the long plays of the coverage check, recorded on the port (origcmp/record_port.sh, 2D at 640 by 480, seed 909, the
# profile player) and turned into origrun scripts (origcmp/to_origrun.py): each to its end, as the identical-play pass played them
# (the play click at update 566 as the recorded tm_* plays have it: a click at 350 is lost in the 2D original; a timed mode idles until the bar runs out and 700 updates more; Classic ends by "no more moves"). The scripts are build/origrun/bej2_<name>.txt,
# the recordings build/logs/bej2/c_<name>.
#   record_plays.sh <name>...     (names: twilight action finity classic hyper)
# twilight and action take 5 to 9 minutes each; finity, classic and hyper run for half an hour or more on a busy machine (the
# game-over code of every mode is the same functions: twilight and action reach it, so the coverage check records only those two).
# run.sh drops a script's `mark` lines, so the idle to the game's end would not be waited for: the last `mark` becomes a mouse move
# off the gems (the original's quit then comes after the game over).
cd "$(dirname "$0")/../../../.." || exit 2
for n in "$@"; do
	case $n in
		twilight) M=twilight; MOVES=40;  IDLE=9000 ;;
		action)   M=action;   MOVES=40;  IDLE=9000 ;;
		finity)   M=finity;   MOVES=19;  IDLE=9000 ;;
		classic)  M=classic;  MOVES=400; IDLE=9000 ;;
		hyper)    M=hyper;    MOVES=40;  IDLE=9000 ;;
		*) echo "record_plays: unknown play $n"; continue ;;
	esac
	D=build/logs/bej2/c_$n
	rm -rf "$D" "$D.user"
	PROFILE=build/logs/bej2/profile LOWRES=1 DRAWEVERY=1 PLAY_AT=566 IDLE_END=$IDLE bash games/bejeweled2/tools/origcmp/record_port.sh $M $MOVES "$D" 909 > "$D.record.txt" 2>&1
	tail -1 "$D.record.txt" | cut -c1-200
	python games/bejeweled2/tools/origcmp/to_origrun.py "$D/events.json" build/origrun/bej2_c_$n.txt > /dev/null && sed -i 's/^at \([0-9]*\) mark$/at \1 move 320 240/' build/origrun/bej2_c_$n.txt && echo "$n: build/origrun/bej2_c_$n.txt $(grep -c '^at' build/origrun/bej2_c_$n.txt) lines"
done
