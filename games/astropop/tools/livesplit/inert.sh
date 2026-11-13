#!/bin/bash
# The autosplit block is inert: the same bot games played by the frozen baseline (build/baseline-astropop/astropop.exe), by the
# port with the block off and by the port with it on (-livesplitlog=<file> asks for it), one game process at a time under rundesk
# (tools/playbymove starts it there), and the three runs' driver transcripts (every command, its answer and the state it was
# asked on) must be the same, byte for byte.
#   games/astropop/tools/livesplit/inert.sh <out dir>
# The games are check.sh's astropop suite's: a classic game from level 1, one from level 4 and a survival game, 120 moves each,
# seed 1. Prints one line for each game and the number of lines of its block log.
cd "$(dirname "$0")/../../../.." || exit 2
OUT="${1:?usage: inert.sh <out dir>}"
EXE=build/native/astropop.exe
BASE=build/baseline-astropop/astropop.exe
[ -f "$BASE" ] || { echo "no frozen baseline at $BASE"; exit 2; }
rm -rf "$OUT"; mkdir -p "$OUT"
start=$(date +%s)
until python tools/gamecount.py --limit 6 > /dev/null 2>&1; do
	[ $(( $(date +%s) - start )) -ge 3600 ] && break
	sleep 15
done
run() {  # <name> <exe> <extra game argument or ""> <first command>
	local d="$OUT/$1" extra=()
	[ -n "$3" ] && extra=("$3")
	mkdir -p "$d"
	timeout 600 python tools/playbymove/playbymove.py --dir "$d" --steps 120 --timeout 120 --first "$4" "$2" -seed=1 \
		-userdir="$d/user" "${extra[@]}" > "$d/out.txt" 2>&1
}
st=0
for g in "classic|goto classic 1" "later|goto classic 4" "survival|goto survival 2"; do
	k="${g%%|*}"; c="${g#*|}"
	run "base-$k" "$BASE" "" "$c"
	run "off-$k" "$EXE" "" "$c"
	run "on-$k" "$EXE" "-livesplitlog=$OUT/on-$k/block.log" "$c"
	r="identical"
	cmp -s "$OUT/base-$k/driver.txt" "$OUT/off-$k/driver.txt" || { r="OFF DIFFERS FROM BASELINE"; st=1; }
	cmp -s "$OUT/base-$k/driver.txt" "$OUT/on-$k/driver.txt" || { r="ON DIFFERS FROM BASELINE"; st=1; }
	[ -s "$OUT/base-$k/driver.txt" ] || { r="NO TRANSCRIPT"; st=1; }
	echo "$k: $r; $(wc -l < "$OUT/base-$k/driver.txt") driver lines; block log $(wc -l < "$OUT/on-$k/block.log" 2> /dev/null || echo 0) lines; $(tail -n 1 "$OUT/on-$k/out.txt" | tr -d '\r' | sed 's/,"field":\[[^]]*\]//' | cut -c1-150)"
done
exit $st
