#!/bin/bash
# case.sh <name> <mode> <bot seed> <game seed> [extra port arguments]: one comparison. The port's bot plays a game through
# the mouse (-autoplayui; mode strategic, timetrial), its input is written as a script, the original plays the same input
# on the same game seed, and the original's values are compared with the port's state log. The original is run three
# times (origrun watches four values at a time). Prints the game's last line and one line per value.
source "$(dirname "$0")/common.sh"
N="$1"; MODE="$2"; BS="$3"; GS="$4"; shift 4
D="$OUT/$N"
"$TOOLS/port.sh" "$D" -seed=$GS -autoplay=$BS -autoplayui -autoplaystart=450 -autoplaymode=$MODE -autoplaycap=30000 \
	-autoplaylog="$D/log.txt" -autoplayscript="$D/script.txt" -statelog="$D/state.txt" "$@" > /dev/null
grep "over at" "$D/log.txt" | tail -1
python "$TOOLS/toorig.py" "$D/script.txt" -1 > "$D/orig.txt"
B='[0x516f04]'   # the app's board pointer (app 0x5160b0 + 0xe54)
"$TOOLS/orig.sh" "$D/orig.txt" $GS 40 "$B+0x60" "$B+0xd18" "$B+0xaf0" "$B+0xaf4" > "$D/origA.log" 2>&1
"$TOOLS/orig.sh" "$D/orig.txt" $GS 40 "$B+0xe0c" "[$B+0xae8]+0x68" "[$B+0xae8]+0x6c" "$B+0xdf8" > "$D/origB.log" 2>&1
"$TOOLS/orig.sh" "$D/orig.txt" $GS 40 "$B+0xe00" "$B+0xd90" "$B+0xaed" > "$D/origC.log" 2>&1
st=0
python "$TOOLS/cmp.py" "$D/state.txt" "$D/origA.log" state,score,moves,discards || st=1
python "$TOOLS/cmp.py" "$D/state.txt" "$D/origB.log" boards,shape,color,trialpause || st=1
python "$TOOLS/cmp.py" "$D/state.txt" "$D/origC.log" fullcount,overtimer,timetrial || st=1
exit $st
