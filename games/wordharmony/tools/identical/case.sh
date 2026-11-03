#!/bin/bash
# case.sh <name> <port script> <seed> [extra port arguments]: one comparison. The port plays the script with -fast and writes
# its state log; the original plays the same input (toorig.py) on the same seed, and its watched values are compared with the
# port's log, change by change with the frame each came at. origrun watches four values at a time, so the original runs once
# per four values (FIELDS=<name,...> picks some). REG=<file.reg> presets the original's registry and PORTREG=<file> the port's
# registry.ini. Prints one line per value.
source "$(dirname "$0")/common.sh"
N="$1"; S="$2"; SEED="$3"; shift 3
D="$OUT/$N"
mkdir -p "$D"; rm -f "$D"/orig*.log
cp "$S" "$D/script.txt"
rm -rf "$D/user"; if [ -n "$PORTREG" ]; then mkdir -p "$D/user"; cp "$PORTREG" "$D/user/registry.ini"; fi
KEEP_USER=1 "$TOOLS/port.sh" "$D" -seed=$SEED -script="$(win "$D/script.txt")" -statelog="$(win "$D/state.txt")" "$@" > /dev/null
python "$TOOLS/toorig.py" "$D/script.txt" > "$D/orig.txt"
LAST=$(tail -1 "$D/state.txt" | cut -d" " -f1); QUIT=$(grep quit "$D/script.txt" | cut -d" " -f1)
# the ball system's values are compared from the frame the port's first level starts (state 2 or more; the original leaves
# them unset before)
export MODE_FROM=$(awk '$2 == "state" && $3 >= 2 { print $1; exit }' "$D/state.txt")

# the original's game is the singleton at 0x4995b8, its ball system the pointer at +0x700, the generator's state 0x49729c
G='[0x4995b8]'; B="[$G+0x700]"
declare -A EX=(
	[rand]=0x49729c [state]="$G+0x1d8" [nextstate]="$G+0x1dc" [timer]="$G+0x1e0" [score]="$G+0x2c" [level]="$G+0x308"
	[mode]="$G+0x708" [template]="$G+0x6fc" [time]="$G+0x30" [levelpoints]="$G+0x38" [lettersleft]="$G+0x3c"
	[lettersused]="$G+0x40" [bonus]="$G+0x44" [wildcards]="$G+0x48" [hints]="$G+0x4c" [hintsused]="$G+0x50"
	[levelhints]="$G+0x54" [leveltime]="$G+0x18c" [ballcount]="$B+0x4" [ballsleft]="$B+0x8" [steptime]="$B+0xfc"
	[spawntime]="$B+0x100" [nextslot]="$B+0x10c" [tocome]="$B+0x120" [water]="$B+0x124" [fill]="$B+0x128"
	[hintindex]="$B+0x1f4" [ball0x]="[$B+0x1c]+0x2c" [ball0y]="[$B+0x1c]+0x30" [ball0state]="[$B+0x1c]+0xd8"
	[ball0letter]="[$B+0x1c]+0xf0" [ball3x]="[$B+0x1c]+0x320" [ball3y]="[$B+0x1c]+0x324")
ALL=(rand state nextstate timer score level mode template time levelpoints lettersleft lettersused bonus wildcards hints
	hintsused levelhints leveltime ballcount ballsleft steptime spawntime nextslot tocome water fill hintindex ball0x ball0y
	ball0state ball0letter ball3x ball3y)
if [ -n "$FIELDS" ]; then IFS=, read -ra NAMES <<< "$FIELDS"; else NAMES=("${ALL[@]}"); fi
# the virtual clock plays several hundred frames a second, after a few seconds of start
TO=$((LAST / 100 + 30))
st=0
for ((i = 0; i < ${#NAMES[@]}; i += 4)); do
	E=(); for n in "${NAMES[@]:i:4}"; do E+=("${EX[$n]}"); done
	"$TOOLS/orig.sh" "$D/orig.txt" $SEED $TO "${E[@]}" > "$D/orig$((i / 4)).log" 2>&1
	names="$(IFS=,; echo "${NAMES[*]:i:4}")"
	python "$TOOLS/cmp.py" "$D/state.txt" "$D/orig$((i / 4)).log" "$names" $((QUIT - 2)) || st=1
done
exit $st
