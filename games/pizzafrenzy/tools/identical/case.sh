#!/bin/bash
# case.sh <name> <seed> <bot moves> [playbymove options...]: one comparison.
# 1. The port's bot plays from a first launch (an empty user folder) for that many moves by tools/playbymove (held,
#    -fast), and record.py turns its transcript into an input file (build/identical/<name>/input.txt).
# 2. The port plays that file again as real input (-realinput, -fast) and writes its state log (state.txt).
# 3. The original plays the same input (toorig.py) on the same seed, watching four values a run, and cmp.py compares
#    each with the port's log, change by change with the frame each came at. A last run peeks the random state at
#    every update, and randseq.py compares the order of its values.
# FIELDS=<name,...> picks the values; INPUT=<file> plays a given input file instead of recording one (step 1 skipped).
# Prints one line per value; exits 1 when one differs.
source "$(dirname "$0")/common.sh"
N="$1"; SEED="$2"; MOVES="$3"; shift 3
D="$OUT/$N"
mkdir -p "$D"; rm -f "$D"/orig*.log
if [ -n "$INPUT" ]; then
	cp "$INPUT" "$D/input.txt"
else
	rm -rf "$D/bot"; mkdir -p "$D/bot/user"
	timeout 280 python -I tools/playbymove/playbymove.py --dir "$D/bot/run" --steps "$MOVES" "$@" \
		build/native/pizzafrenzy.exe -gamedir=assets/pizzafrenzy -userdir="$D/bot/user" -seed="$SEED" > "$D/bot/out.txt" 2>&1
	python -I "$TOOLS/record.py" "$D/bot/run/driver.txt" > "$D/input.txt"
fi
"$TOOLS/port.sh" "$D/port" -seed="$SEED" -realinput="$(win "$D/input.txt")" -statelog="$(win "$D/state.txt")" \
	> /dev/null
python -I "$TOOLS/toorig.py" "$D/input.txt" > "$D/orig.txt" || exit 1
QUIT=$(tail -1 "$D/input.txt" | cut -d" " -f1)
# the game's values are compared from the frame the port's first level starts (the stats are made)
export MODE_FROM=$(awk '$2 == "score" && $3 != "-" { print $1; exit }' "$D/state.txt")

# the original's game is the object whose address is at 0x532900; its stats at +0x120, its mode's game at +0x110;
# rand's state is mirrored at 0x541a04 (setup.sh)
G='[0x532900]'; ST="[$G+0x120]"; M="[$G+0x110]"
declare -A EX=(
	[rand]=0x541a04 [state]="[$G+0x30]" [city]="$G+0x4c" [level]="$G+0x48"
	[score]="$ST+0xc" [elapsed]="$ST+0x10" [delivered]="$ST+0x20" [ordered]="$ST+0x24" [happiness]="$ST+0x28"
	[served]="$ST+0x2c" [tipmoney]="$ST+0x34" [waves]="$ST+0x3c" [combo]="$ST+0x40"
	[wavetimer]="$M+0xd8" [spawntimer]="$M+0xdc" [deliverytimer]="$M+0x114" [customersleft]="$M+0xe8"
	[ordersbegun]="$M+0xec" [streak]="$M+0xf0")
ALL=(state city level score elapsed delivered ordered happiness served tipmoney waves combo wavetimer spawntimer
	deliverytimer customersleft ordersbegun streak)
if [ -n "$FIELDS" ]; then IFS=, read -ra NAMES <<< "${FIELDS//rand/}"; else NAMES=("${ALL[@]}"); fi
NAMES=($(printf "%s\n" "${NAMES[@]}" | grep .))
TO=$((QUIT / 100 + 40))
st=0
for ((i = 0; i < ${#NAMES[@]}; i += 4)); do
	E=(); for n in "${NAMES[@]:i:4}"; do E+=("${EX[$n]}"); done
	"$TOOLS/orig.sh" "$D/orig.txt" "$SEED" "$TO" "${E[@]}" > "$D/orig$((i / 4)).log" 2>&1
	names="$(IFS=,; echo "${NAMES[*]:i:4}")"
	python -I "$TOOLS/cmp.py" "$D/state.txt" "$D/orig$((i / 4)).log" "$names" $((QUIT - 2)) || st=1
done
# the random state is peeked at every update (a watch can miss a value) and compared by the order of its values
if [ -z "$FIELDS" ] || [[ ",$FIELDS," == *,rand,* ]]; then
	awk -v q="$QUIT" 'BEGIN { k = 1 } $1 == "at" { while (k <= $2 + 0 && k <= q) print "at " k++ " peek 0x541a04" }
		{ print }' "$D/orig.txt" > "$D/orig_rand.txt"
	"$TOOLS/orig.sh" "$D/orig_rand.txt" "$SEED" "$TO" > "$D/orig_rand.log" 2>&1
	python -I "$TOOLS/randseq.py" "$D/state.txt" "$D/orig_rand.log" || st=1
fi
exit $st
