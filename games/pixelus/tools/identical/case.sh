#!/bin/bash
# case.sh <name> <bot seed> <game seed> <puzzles> <delta> [extra port arguments]: one comparison. The port's bot plays N
# puzzles through the mouse (-autoplayui), its input is written as a script, the original plays the same input on the same
# seed, and the original's values (the screens that exist, the move count, the time, the tiles' places...) are compared with
# the port's state log (testhooks/Autoplay.cpp, StateLog), change by change with the update it came at. origrun watches four
# values at a time, so the original runs once per four values. DELTA is how many updates later than the port's the
# original's Play button works (its loading screen takes longer); the first run finds it (see IDENTICAL_PLAY.md).
source "$(dirname "$0")/common.sh"
N="$1"; BS="$2"; GS="$3"; PZ="$4"; DELTA="$5"; shift 5
D="$OUT/$N"
"$TOOLS/port.sh" "$D" -seed=$GS -autoplay=$BS -autoplayui -autoplaypuzzles=$PZ -autoplaycap=200000 \
	-autoplaylog="$D/log.txt" -autoplayscript="$D/script.txt" -statelog="$D/state.txt" "$@" > /dev/null
tail -1 "$D/log.txt"
python "$TOOLS/toorig.py" "$D/script.txt" "$DELTA" > "$D/orig.txt"

# the original's app is a static object at 0x542db0 (+0x678 the board, +0x680.. the screens); the grid is the board's +0xe8
G='[[0x543428]+0xe8]'
E0="[[[[0x543428]+0xe8]+0xb040]]"
E1="[[[[0x543428]+0xe8]+0xb040]+4]"
E2="[[[[0x543428]+0xe8]+0xb040]+8]"
E3="[[[[0x543428]+0xe8]+0xb040]+0xc]"
NAMES=(load menu story island temple board completed help moves time done supply cursor side empty stray reveal moving wanted
	o0x o0y o0state o1x o1y o1state o2x o2y o2state o2variant o0variant o1variant o3variant)
EXPRS=(0x543434 0x543438 0x543450 0x54343c 0x543430 0x543428 0x543448 0x54342c "$G+0xb060" "$G+0xb064" "$G+0xb05c" "$G+0xb050"
	"$G+0xb054" "$G+0xb058" "$G+0xb164" "$G+0xb168" "$G+0xb100" "$G+0xb0a8" "$G+0xb16c" "$E0+0x8" "$E0+0xc" "$E0+0x44"
	"$E1+0x8" "$E1+0xc" "$E1+0x44" "$E2+0x8" "$E2+0xc" "$E2+0x44" "$E2+0x1c" "$E0+0x1c" "$E1+0x1c" "$E3+0x1c")
LAST=$(tail -1 "$D/script.txt" | cut -d' ' -f1)
TO=$(( (LAST + DELTA) / 300 + 20 ))
st=0
for ((i = 0; i < ${#NAMES[@]}; i += 4)); do
	"$TOOLS/orig.sh" "$D/orig.txt" $GS $TO "${EXPRS[@]:i:4}" > "$D/orig$((i / 4)).log" 2>&1
	names="$(IFS=,; echo "${NAMES[*]:i:4}")"
	python "$TOOLS/cmp.py" "$D/state.txt" "$D/orig$((i / 4)).log" "$names" "$DELTA" $((LAST + DELTA)) || st=1
done
exit $st
