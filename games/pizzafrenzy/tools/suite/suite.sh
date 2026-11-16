#!/bin/bash
# Pizza Frenzy's check suite (tools/check.sh's suite_pizzafrenzy calls it): <out dir> [exe]
#
# The bot plays by real input through tools/playbymove (-fast, -seed=1, the profile registry.ini beside this file: a
# player, Test, with the first four cities open): the speed mode from launch for SPEED_STEPS moves (through the first
# city's levels and its bonus round), then the memory and the concentration modes from the menu for MODE_STEPS moves
# each. Each run is made twice, and the two transcripts (driver.txt, every line sent and its answer) must be the same.
# When the frozen baseline build/baseline-pizzafrenzy/pizzafrenzy.exe is there, its runs must give the same
# transcripts as the port's. Prints one summary line; exits 0 when every check passed.
set -u
OUT="$1"
EXE="${2:-build/native/pizzafrenzy.exe}"
HERE="$(cd "$(dirname "$0")" && pwd)"
SPEED_STEPS="${PF_SPEED_STEPS:-700}"
MODE_STEPS="${PF_MODE_STEPS:-300}"
BASE=build/baseline-pizzafrenzy/pizzafrenzy.exe
mkdir -p "$OUT"
fail=""

# <run name> <exe> <steps> [first commands...]: one run; prints its summary line's head
play() {
	local name="$1" exe="$2" steps="$3"; shift 3
	local dir="$OUT/$name" firsts=()
	rm -rf "$dir"; mkdir -p "$dir/user"; cp "$HERE/registry.ini" "$dir/user/registry.ini"
	for f in "$@"; do firsts+=(--first "$f"); done
	timeout 280 python -I tools/playbymove/playbymove.py --dir "$dir/run" --steps "$steps" "${firsts[@]}" \
		"$exe" -gamedir=assets/pizzafrenzy -userdir="$dir/user" -seed=1 > "$dir/out.txt" 2>&1
	tail -n 1 "$dir/out.txt" | cut -c1-60
}

# <name> <steps> [first commands...]: the run twice on the port, and once on the baseline when there is one
check() {
	local name="$1" steps="$2"; shift 2
	local line
	line="$(play "$name" "$EXE" "$steps" "$@")"
	play "$name-again" "$EXE" "$steps" "$@" > /dev/null
	case "$line" in *"stop limit"*|*"stop over"*) ;; *) fail="$fail $name($line)";; esac
	cmp -s "$OUT/$name/run/driver.txt" "$OUT/$name-again/run/driver.txt" || fail="$fail $name-rerun"
	if [ -x "$BASE" ]; then
		play "$name-base" "$BASE" "$steps" "$@" > /dev/null
		cmp -s "$OUT/$name/run/driver.txt" "$OUT/$name-base/run/driver.txt" || fail="$fail $name-baseline"
	fi
	echo "$name: $line"
}

check speed "$SPEED_STEPS"
check memory "$MODE_STEPS" "wait 300" "goto memory"
check concentration "$MODE_STEPS" "wait 300" "goto concentration"
[ -x "$BASE" ] && base="baseline identical" || base="no baseline"
if [ -z "$fail" ]; then
	echo "pizzafrenzy: speed, memory and concentration played by the bot; reruns identical; $base"
	exit 0
fi
echo "pizzafrenzy: FAILED:$fail"
exit 1
