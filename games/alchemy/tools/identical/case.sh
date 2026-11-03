#!/bin/bash
# case.sh <name> <mode> <bot seed> <game seed> [extra port arguments]: one comparison. The port's bot plays a game through
# the mouse (-autoplayui; mode strategic or timetrial; -autoplayskill=<0..2>, -autoplaymiss=<n> as extra arguments), its
# input is written as a script, the original plays the same input on the same game seed, and the original's board values are
# compared with the port's state log (testhooks/Autoplay.h, StateLog), change by change with the update it came at. origrun
# watches four values at a time, so the original runs once per four values. Prints the game's last line and one line per value.
source "$(dirname "$0")/common.sh"
N="$1"; MODE="$2"; BS="$3"; GS="$4"; shift 4
D="$OUT/$N"
"$TOOLS/port.sh" "$D" -seed=$GS -autoplay=$BS -autoplayui -autoplaystart=450 -autoplaymode=$MODE -autoplaycap=30000 \
	-autoplaylog="$D/log.txt" -autoplayscript="$D/script.txt" -statelog="$D/state.txt" "$@" > /dev/null
grep "over at" "$D/log.txt" | tail -1
python "$TOOLS/toorig.py" "$D/script.txt" -1 > "$D/orig.txt"

# the original's board is the pointer at 0x516f04 (the app global at 0x5160b0, +0xe54); the piece to place is at board +0xae8
B='[0x516f04]'
NAMES=(state score moves discards boards shape color trialpause fullcount overtimer timetrial multimoves run longest pause
	numshapes numcolors bombchance skullchance elapsed totalseconds numvalid lastshape tutorial tutstep tutcount tutwaiting hinton hinttimer)
EXPRS=("$B+0x60" "$B+0xd18" "$B+0xaf0" "$B+0xaf4" "$B+0xe0c" "[$B+0xae8]+0x68" "[$B+0xae8]+0x6c" "$B+0xdf8" "$B+0xe00"
	"$B+0xd90" "$B+0xaed" "$B+0xaf8" "$B+0xe04" "$B+0xe08" "$B+0xdf4" "$B+0xd98" "$B+0xd9c" "$B+0xda0" "$B+0xda4" "$B+0xdec"
	"$B+0xdf0" "$B+0xd00" "$B+0xe60" "$B+0xdd0" "$B+0xdd4" "$B+0xdd8" "$B+0xe44" "$B+0xe30" "$B+0xe34")
# the virtual clock plays about 1000 updates a second, after about 5 seconds of start
LAST=$(tail -1 "$D/script.txt" | cut -d' ' -f1)
TO=$((LAST / 700 + 10))
st=0
for ((i = 0; i < ${#NAMES[@]}; i += 4)); do
	"$TOOLS/orig.sh" "$D/orig.txt" $GS $TO "${EXPRS[@]:i:4}" > "$D/orig$((i / 4)).log" 2>&1
	names="$(IFS=,; echo "${NAMES[*]:i:4}")"
	python "$TOOLS/cmp.py" "$D/state.txt" "$D/orig$((i / 4)).log" "$names" || st=1
done
exit $st
