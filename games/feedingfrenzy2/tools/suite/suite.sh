#!/bin/bash
# Feeding Frenzy 2's check suite (tools/check.sh's suite_feedingfrenzy2 calls it): <out dir> [exe]
#
# Three tours from scripts of test commands (-fast; the profile registry.ini beside this file: one player, Alye, at
# the start of the story mode):
#   stage1  the first stage played from recorded mouse moves (the player eats, grows and scores; 6270 updates)
#   bonus   every stage unlocked, the first bonus stage (Herring Herder) started: a time bubble rises to the player and
#           is eaten (the clock up 3 seconds, its text rising), the clock cut to 2 seconds, the bonus stage's count,
#           Continue, and the next stage reached
#   lose    the first stage lost three times, the continue offer's Yes, three more, Yes, three more, and the game over's
#           Done back to the menu
# Each tour is run twice; the two runs must give the same state lines and screenshots, and their hashes must be the
# frozen ones below. When the frozen baseline build/baseline-feedingfrenzy2/feedingfrenzy2.exe is there, its tours must
# give the same too. One game process at a time. Prints one summary line; exits 0 when every check passed.
set -u
OUT="$1"
EXE="${2:-build/native/feedingfrenzy2.exe}"
HERE="$(cd "$(dirname "$0")" && pwd)"
BASE=build/baseline-feedingfrenzy2/feedingfrenzy2.exe
TOURS="stage1 bonus lose"
# The first 12 hex digits of the sha256 of each tour's screenshots' hashes and of its state lines: the frozen baseline
FROZEN_stage1="6bd10893e4f6 26bf7d79e7d5"
FROZEN_bonus="9d60e42341e8 84d108ed4c34"
FROZEN_lose="9254a944ebb5 7de3b7096536"
mkdir -p "$OUT"
fail=""
notes=""

# <run dir> <exe> <tour>: one tour; prints the exe's exit status
run() {
	local dir="$1" exe="$2" tour="$3" e=0
	rm -rf "$dir"; mkdir -p "$dir/user"; cp "$HERE/registry.ini" "$dir/user/registry.ini"
	sed "s|@DIR@|$dir|" "$HERE/$tour.txt" > "$dir/script.txt"
	timeout 300 "$exe" -userdir="$dir/user" -fast -script="$dir/script.txt" \
		> "$dir/stdout.txt" 2>&1 || e=$?
	grep -E '^(INFO: )?(OK|ERROR|STATE) ' "$dir/stdout.txt" | sed 's/^INFO: //' > "$dir/states.txt"
	echo "$e"
}

# <run dir>: the hashes of its screenshots (in name order) and of its state lines
digest() {
	echo "$(for f in "$1"/*.png; do sha256sum "$f" | cut -c1-64; done | sha256sum | cut -c1-12) $(sha256sum "$1/states.txt" | cut -c1-12)"
}

for t in $TOURS; do
	e="$(run "$OUT/$t" "$EXE" "$t")"
	[ "$e" = 0 ] || { fail="$fail $t(exit $e)"; continue; }
	grep -q '^ERROR' "$OUT/$t/states.txt" && { fail="$fail $t($(grep -m1 '^ERROR' "$OUT/$t/states.txt"))"; continue; }
	n="$(grep -c screenshot "$HERE/$t.txt")"
	[ "$(ls "$OUT/$t"/*.png 2>/dev/null | wc -l)" = "$n" ] || { fail="$fail $t(screenshots missing)"; continue; }
	e="$(run "$OUT/$t-again" "$EXE" "$t")"
	[ "$e" = 0 ] || { fail="$fail $t-rerun(exit $e)"; continue; }
	d="$(digest "$OUT/$t")"
	[ "$d" = "$(digest "$OUT/$t-again")" ] || { fail="$fail $t-rerun(differs)"; continue; }
	eval "frozen=\$FROZEN_$t"
	[ "$d" = "$frozen" ] || { fail="$fail $t(CHANGED: $d; frozen $frozen)"; continue; }
	if [ -x "$BASE" ]; then
		e="$(run "$OUT/$t-base" "$BASE" "$t")"
		[ "$d" = "$(digest "$OUT/$t-base")" ] || fail="$fail $t-baseline(differs)"
	fi
done

# What each tour must have reached
last() { grep '^STATE' "$OUT/$1/states.txt" | tail -n 1; }
case "$(last stage1)" in *'"score":60,'*) ;; *) fail="$fail stage1(score not 60)";; esac
grep -q '"lives":4\|"time":42.66' "$OUT/bonus/states.txt" || fail="$fail bonus(no time bubble eaten)"
case "$(last bonus)" in *'"level":4,'*) ;; *) fail="$fail bonus(the next stage not reached)";; esac
case "$(last lose)" in *'"run":"none"'*|*'"player":null'*) ;; *) fail="$fail lose(not back at the menu)";; esac

[ -x "$BASE" ] && notes="; baseline identical"
if [ -n "$fail" ]; then
	echo "FAIL:$fail"
	exit 1
fi
echo "the first stage played from recorded moves (score 60), the first bonus stage with a time bubble eaten, its count and the next stage, the continue offer twice and the game over back to the menu; reruns identical; screens and states as frozen$notes"
exit 0
