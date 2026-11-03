#!/bin/bash
# case.sh <name> <port script> <seed> [<field> ...]: one comparison case. The port plays the script (-statelog), the
# script is turned into the original's (toorig.py), the original plays it once for every three fields (origrun watches
# at most four values a run, and each run also watches rngindex, whose first deal lines its clock up with the port's),
# and cmp.py compares each field from the first deal on. With no fields, all of them.
# Files go to $OUT/<name>/ (default build/identical/<name>); the last line says whether every field was the same.
# ORIG_TIMEOUT (seconds, default 120, at most 240) bounds each run of the original.
source "$(dirname "$0")/common.sh"
NAME="$1"; SCRIPT="$2"; SEED="$3"; shift 3

# The original's addresses of the values the port's -statelog writes (NingPoApp.cpp, STATE_VALUES): the game is the
# global at 0x475550
declare -A ADDR=([state]=0x47d710 [tilesleft]=0x47d708 [moves]=0x47d70c [penalty]=0x47c06c [hints]=0x47d714
	[undos]=0x47d718 [shuffles]=0x47d71c [finaltime]=0x47d720 [finalraw]=0x47d724 [rngindex]=0x47be4c [clock]=0x475544
	[layout]=0x47d7a4 [undoindex]=0x47c2b8 [tile0]=0x47c504 [tile71]=0x47cde4 [tile143]=0x47d6e4 [best0tiles]=0x480240
	[best0time]=0x480238 [best1tiles]=0x480270 [best1time]=0x480268)
FIELDS=("$@")
[ ${#FIELDS[@]} -gt 0 ] || FIELDS=(state tilesleft moves clock penalty hints undos shuffles finaltime finalraw
	layout undoindex tile0 tile71 tile143 best0tiles best0time best1tiles best1time)

D="$OUT/$NAME"; rm -rf "$D"; mkdir -p "$D"
cp "$SCRIPT" "$D/port.txt"
"$TOOLS/port.sh" "$D/port.txt" "$SEED" "$D/port.log" || { echo "$NAME: the port's run failed"; exit 1; }
python "$TOOLS/toorig.py" "$D/port.txt" "$D/orig.txt" || exit 1
LOGS=""
for ((i = 0; i < ${#FIELDS[@]}; i += 3)); do
	G=(rngindex "${FIELDS[@]:i:3}"); W=()
	for f in "${G[@]}"; do W+=("${ADDR[$f]}"); done
	L="$D/orig$((i / 3)).txt"
	printf '%s\n' "${G[@]}" > "$L.fields"
	# a run whose generator strays from the port's (the original once barely ran its loop and missed the script's keys)
	# is run again, once, and the case's output says so
	for try in 1 2; do
		"$TOOLS/orig.sh" "$D/orig.txt" "$SEED" "${ORIG_TIMEOUT:-120}" "${W[@]}" > "$L" 2>&1
		python "$TOOLS/cmp.py" "$D/port.log" "$L" | grep -q "^run 0 .*rngindex DIFFERS" || break
		[ $try -eq 2 ] || { echo "$NAME: the original's run $((i / 3)) went another way; running it again"; cp "$L" "$L.stray"; }
	done
	grep -q "the game process .* ended\|game exited" "$L" || echo "$NAME: the original's run $((i / 3)) did not end cleanly ($L)"
	# a message box that really appears rings on the user's speakers: no more runs until its cause is found (origrun's
	# hook answers the game's own boxes with no window, and logs them with "[hooked")
	if grep "dialog appeared" "$L" | grep -qv "\[hooked"; then
		echo "$NAME: a dialog appeared in the original's run $((i / 3)); stopping ($L)"; exit 2
	fi
	LOGS="$LOGS${LOGS:+,}$L"
done
python "$TOOLS/cmp.py" "$D/port.log" "$LOGS" > "$D/cmp.txt"; st=$?
cat "$D/cmp.txt"
[ $st -eq 0 ] && echo "$NAME: same" || echo "$NAME: DIFFERS"
exit $st
