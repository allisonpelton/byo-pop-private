#!/bin/bash
# Bejeweled 3's check suite (tools/check.sh's suite_bejeweled3 calls it): <out dir> [exe]
#
# One script of test commands (suite_test.txt beside this file), run twice: the menu, a seeded Classic game played by the
# bot for 150 moves, a board with a Flame Gem, a Star Gem and a Hypercube put on it, then a board with no move and the
# game-over dialog. The two runs' state lines and their screenshots (all but the menu's) must be the same, and the state lines must
# match the frozen hash below (update it, with the reason in the commit, when a translation changes the game's state).
set -u
OUT="$1"
EXE="${2:-build/native/bejeweled3.exe}"
HERE="$(cd "$(dirname "$0")" && pwd)"
FROZEN="bfe5c9c6bf24"
SHOTS="classic play specials gameover"  # compared; menu.png is only kept to look at: the menu opens after the
# loading, which runs at the machine's speed, so its dialog and balls stand at a different point of their animation each run
mkdir -p "$OUT"
fail=""

run() {  # <name>: the script into $OUT/<name>; prints the exe's exit status
	local dir="$OUT/$1" e=0 windir
	rm -rf "$dir"; mkdir -p "$dir/user"
	windir="$(cygpath -m "$(cd "$dir" && pwd)")"
	sed "s|@DIR@|$windir|" "$HERE/suite_test.txt" > "$dir/script.txt"
	timeout 240 "$EXE" -gamedir=assets/bejeweled3 -userdir="$dir/user" -fast -script="$dir/script.txt" > "$dir/stdout.txt" 2>&1 || e=$?
	grep -oE '(OK|ERROR|STATE) .*' "$dir/stdout.txt" > "$dir/states.txt"
	echo "$e"
}

e="$(run game)"; [ "$e" -eq 0 ] || fail="$fail exit($e)"
grep -q '^ERROR' "$OUT/game/states.txt" && fail="$fail command($(grep -m1 '^ERROR' "$OUT/game/states.txt" | cut -c1-80))"
[ "$(grep -c '^STATE ' "$OUT/game/states.txt")" = 3 ] || fail="$fail statelines"
grep '^STATE ' "$OUT/game/states.txt" | tail -n 1 | grep -q '"gameover":1' || fail="$fail nogameover"
e="$(run rerun)"; [ "$e" -eq 0 ] || fail="$fail rerun-exit($e)"
cmp -s "$OUT/game/states.txt" "$OUT/rerun/states.txt" || fail="$fail rerun-states"
for s in $SHOTS; do
	[ -s "$OUT/game/$s.png" ] || { fail="$fail noshot-$s"; continue; }
	cmp -s "$OUT/game/$s.png" "$OUT/rerun/$s.png" || fail="$fail rerun-$s"
done
sh="$(sha256sum "$OUT/game/states.txt" | cut -c1-12)"
[ -z "$FROZEN" ] || [ "$sh" = "$FROZEN" ] || fail="$fail STATES CHANGED($sh; frozen $FROZEN)"
if [ -z "$fail" ]; then
	echo "bejeweled3: menu, a seeded Classic game (bot, 150 moves), the specials and the game over; rerun identical (states and 4 shots); states $sh"
	exit 0
fi
echo "bejeweled3: FAILED:$fail"
exit 1
