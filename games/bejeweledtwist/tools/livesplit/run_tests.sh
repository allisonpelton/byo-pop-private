#!/usr/bin/env bash
# Autosplit-block tests for Bejeweled Twist (src/LiveSplitObserver.cpp): seeded autoplay games with the "livesplit" mod on,
# their -livesplitlog lines checked, and the proof that the block is inert (the screens of check.sh's Classic and Blitz
# games with the block on must be the suite's frozen hashes). Run it on an invisible desktop (CLAUDE.md):
#   build/native/rundesk.exe bash games/bejeweledtwist/tools/livesplit/run_tests.sh <out dir>
# Needs build/native/bejeweledtwist.exe and the game files (assets/bejeweledtwist). About 2 minutes; at most three games at
# a time. <out dir>/<run>/block.log is the game's -livesplitlog.
#
# Runs:
#   inert_classic, inert_blitz   check.sh's Classic and Blitz games (the same seed, screenshots and quit), the block on
#   classic   Level 10 (Classic): -autoplay's Classic game, the test command `level` ending levels 1 to 10, then `lose`
#   zen       Level 10 (Zen): the same in Zen
#   blitz     -autoplay's Blitz game to its end (in this seed a counter runs out first: game over kind 0)
#   challenge Enigma% and Challenge 100%: `autoplaychallenge`, a click on Detonator's planet, -autoplay meets Detonator 1
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
OUT="${1:?usage: run_tests.sh <out dir>}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
cd "$ROOT"
EXE="build/native/bejeweledtwist.exe"
status=0
fail() { echo "FAIL: $*"; status=1; }
eval "$(grep -oE '^BT_(CLASSIC|BLITZ)="[^"]*"' tools/check.sh)"

rm -rf "$OUT"/*/
mkdir -p "$OUT/mods"
cp -r games/bejeweledtwist/mods/livesplit "$OUT/mods/livesplit"

game() {  # <run> <autoplay mode> <script lines...>: a seeded 2D autoplay game with the block on
	local N="$1" T="$OUT/$1" mode="$2"
	shift 2
	mkdir -p "$T/user" "$T/shots"
	printf '[SOFTWARE\\Steam\\BejeweledTwist]\nScreenMode=dword:0\nArtRes=dword:768\n' > "$T/user/registry.ini"
	printf '%s\n' "$@" | sed "s|@SHOTS@|$(cygpath -m "$T/shots")|" > "$T/script.txt"
	timeout 600 "$EXE" -gamedir=assets/bejeweledtwist -userdir="$T/user" -modsdir="$OUT/mods" -livesplitlog="$T/block.log" \
		-nod3d9 -fast -seed=1234 -autoplay="$mode" -script="$T/script.txt" > "$T/stdout.txt" 2> "$T/stderr.txt"
	echo "$N: exit $?"
}
shots() {  # <updates...>: script lines for check.sh's screenshots
	local n
	for n in "$@"; do echo "$n screenshot @SHOTS@/$n.png"; done
}
hashes() {  # <run> <updates...>
	local T="$OUT/$1" n
	shift
	for n in "$@"; do sha256sum "$T/shots/$n.png" 2>/dev/null | cut -c1-12; done | paste -sd" " -
}
levels() {  # script lines: `level 2` to `level 11` 1000 updates apart from update 1500, then `lose` and a quit (a
	# `level` that lands while the board is busy is refused and the next one skips a level, so only level 10's clear is checked)
	local u=1500 k
	for ((k = 2; k <= 11; k++)); do echo "$u level $k"; u=$((u + 1000)); done
	echo "$((u + 500)) lose"; echo "$((u + 1100)) quit"
}

mapfile -t L < <(shots 656 2156 3656 5156; echo "5166 quit"); game inert_classic 1 "${L[@]}" &
mapfile -t L < <(shots 2656 5156 7156 7606 8156; echo "8166 quit"); game inert_blitz blitz "${L[@]}" &
game blitz blitz "8166 quit" &
wait
mapfile -t L < <(levels)
game classic 1 "${L[@]}" &
game zen zen "${L[@]}" &
game challenge 1 "5 unlock all" "6 autoplaychallenge" "2210 click 500 95" "7600 quit" &
wait

[ "$(hashes inert_classic 656 2156 3656 5156)" = "$BT_CLASSIC" ] && echo "inert: Classic's screens as frozen" ||
	fail "inert: Classic's screens differ with the block on"
[ "$(hashes inert_blitz 2656 5156 7156 7606 8156)" = "$BT_BLITZ" ] && echo "inert: Blitz's screens as frozen" ||
	fail "inert: Blitz's screens differ with the block on"

expect() {  # <run> <count> <pattern>: the run's block.log has that many lines matching
	local got
	got="$(grep -cE "$3" "$OUT/$1/block.log" 2>/dev/null)"
	[ "${got:-0}" = "$2" ] && echo "$1: $2 x '$3'" || fail "$1: '$3' $got times, not $2"
}
expect classic 1 "GO at tick .* \(mode 1,"
expect classic 1 "level clear 10 "
expect classic 1 "game over \(kind 3\)"
expect zen 1 "GO at tick .* \(mode 4,"
expect zen 1 "level clear 10 "
expect zen 1 "game over \(kind 3\)"
expect blitz 1 "GO at tick .* \(mode 3,"
expect blitz 1 "game over \(kind 0\)"
expect challenge 1 "GO at tick .* \(mode 2,"
expect challenge 1 "win \(challenge 1 of category 0\)"
expect challenge 0 "game over"
[ $status -eq 0 ] && echo "livesplit: all passed" || echo "livesplit: FAILED"
exit $status
