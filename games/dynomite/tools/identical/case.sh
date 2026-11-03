#!/bin/bash
# case.sh <name> <seed> <port script> <mode> [extra port arguments]: one comparison. The port plays the script on the
# seed with -fast and logs its state (FossilizedApp::LogState, -statelog), the script is turned into an origrun script
# (toorig.py), the original plays it on the same seed, and the original's values are compared with the port's log,
# change by change with the update each came at (cmp.py). origrun watches four values at a time, so the original runs once
# per four values. <mode> picks the mode's own four values (stomped, panic, fossil or timetrial; "-" leaves them out, for
# a case that plays several modes). PREP, if set, is a command run before each run of the original (to put a saved game in
# its folder, say). ONLY="0 3" runs only those groups of four values. Prints one line per value; exits 1 if any differs.
source "$(dirname "$0")/common.sh"
N="$1"; SEED="$2"; SCRIPT="$3"; MODE="$4"; shift 4
D="$OUT/$N"
mkdir -p "$D"
[ "$SCRIPT" -ef "$D/script.txt" ] || cp "$SCRIPT" "$D/script.txt"
"$TOOLS/port.sh" "$D" -seed=$SEED -script="$(win "$D/script.txt")" -statelog="$(win "$D/state.txt")" "$@" > /dev/null
python "$TOOLS/toorig.py" "$D/script.txt" -1 > "$D/orig.txt"
LAST=$(grep '^at ' "$D/orig.txt" | tail -1 | cut -d' ' -f2)

APP=0x5304f8
B='[0x531ee4]'; G="[$B+0x8c]"; L="[$B+0x5c]"
NAMES=(mti mt0 fibi fibj score type chain longest babies bonus rank pause fired eggs maxcolor bottom ceiling rows pastline
	angle loaded held frozen next)
EXPRS=(0x531490 0x530ad0 0x531498 0x53149c '[0x531edc]+0x68' "$B+0x7c" "$B+0x14c" "$B+0x1d0" "$B+0x1d4" "$B+0x12c"
	"$B+0x1c0" "$B+0x150" "$B+0x15c" "$G+0x46e0" "$G+0x10" "$G+0xc" "$G+4" "$G+8" "$G+0x4684" "$L+0x14" "[$L+0xc]+4"
	"[$L+0xaa8]+4" "$L+0xaa4" "[[$B+0x84]+0x20]+4")
case "$MODE" in
	stomped) NAMES+=(m0 m1 m2 m3); EXPRS+=("$B+0x5b8" "$B+0x398" "$B+0x59c" "$B+0x5b4") ;;
	panic) NAMES+=(m0 m1 m2 m3); EXPRS+=("$B+0x264" "$B+0x270" "$B+0x26c" "$B+0x250") ;;
	fossil) NAMES+=(m0 m1 m2 fell); EXPRS+=("$B+0x250" "$B+0x254" "$B+0x268" "$B+0x270") ;;
	timetrial) NAMES+=(m0 m1 m2); EXPRS+=("$B+0x248" "$B+0x24c" "$B+0x250") ;;
esac
# the port's log calls the Fossil mode's fourth value m3; cmp.py reads it as fell, a byte
[ "$MODE" = fossil ] && sed -i 's/ m3 / fell /' "$D/state.txt"
# the virtual clock plays several thousand updates a second, after a few seconds of start
TO=$((LAST / 2000 + 30))
st=0
for ((i = 0; i < ${#NAMES[@]}; i += 4)); do
	[ -n "$ONLY" ] && [[ " $ONLY " != *" $((i / 4)) "* ]] && continue
	[ -n "$PREP" ] && eval "$PREP"
	"$TOOLS/orig.sh" "$D/orig.txt" $SEED $TO "${EXPRS[@]:i:4}" > "$D/orig$((i / 4)).log" 2>&1
	names="$(IFS=,; echo "${NAMES[*]:i:4}")"
	python "$TOOLS/cmp.py" "$D/state.txt" "$D/orig$((i / 4)).log" "$names" "$LAST" || st=1
done
exit $st
