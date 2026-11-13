#!/usr/bin/env bash
# Autosplit-block tests for Cosmic Bugs: games with the "livesplit" mod on (the block written, -livesplitlog's lines and the events
# checked) and the proof that the block is inert (one scripted game's state lines with the block on, with it off and with it off
# again must be byte-identical). Run it on an invisible desktop (CLAUDE.md), and never edit this file while it runs (bash reads a
# script as it goes):
#   build/native/rundesk.exe bash games/cosmicbugs/tools/livesplit/run_tests.sh <out dir>
# Needs build/native/cosmicbugs.exe and the game files (assets/cosmicbugs). About 10 minutes; one game at a time. <out dir>/<run>/
# block.log is the game's -livesplitlog, game.log the exe's log with the "livesplit: board ..." line of each event.
#
# Runs (games.py describes the first five):
#   game, mission, left, resume, select   played through real input at -fast
#   inert_on, inert_off1, inert_off2      the same scripted game (-script: its clicks and state lines at fixed updates, the game's
#                                         generator seeded) with the mod on, and with no mod twice: the three state logs match
# CHECK_ONLY=1: only the checks, on the runs an earlier call left in <out dir>
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
OUT="${1:?usage: run_tests.sh <out dir>}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
EXE="$(cygpath -w "$ROOT/build/native/cosmicbugs.exe")"
status=0
fail() { echo "FAIL: $*"; status=1; }
cd "$ROOT"

if [ -z "${CHECK_ONLY:-}" ]; then
rm -rf "$OUT"/*/
mkdir -p "$OUT/mods" "$OUT/mods_off"
cp -r "$ROOT/games/cosmicbugs/mods/livesplit" "$OUT/mods/livesplit"
until python tools/gamecount.py --limit 5 > /dev/null; do sleep 10; done
python games/cosmicbugs/tools/livesplit/games.py "$OUT" "$OUT/mods" game mission left resume select || status=1

# the scripted game: Start Game, Play, lines fired at fixed points, the first board won and clicked through, the second lost
inert() { # <run> <mods dir>
	local N="$1" T="$OUT/$1" M="$2" u k
	mkdir -p "$T/user"
	printf '<CP_CONFIG\n    fullscreen="0"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n' > "$T/user/cp.cfg"
	{
		echo "5 seed 777"
		echo "300 down 175 313"; echo "303 up 175 313"
		echo "700 down 400 563"; echo "703 up 400 563"
		echo "900 move 300 250"
		u=1300
		for p in "300 250" "500 400" "200 200" "600 300" "400 350" "250 450" "550 200" "350 300"; do
			echo "$u down $p"; echo "$((u + 3)) up $p"; echo "$((u + 400)) state"; u=$((u + 800))
		done
		echo "$u win"; u=$((u + 100)); echo "$u state"
		for k in 1 2 3 4 5 6 7 8 9 10 11 12; do
			u=$((u + 300)); echo "$u down 400 300"; echo "$((u + 3)) up 400 300"
		done
		u=$((u + 600)); echo "$u state"
		u=$((u + 300)); echo "$u move 300 250"
		u=$((u + 400)); echo "$u lose"; u=$((u + 200)); echo "$u state"
		u=$((u + 300)); echo "$u down 400 300"; echo "$((u + 3)) up 400 300"
		u=$((u + 500)); echo "$u state"
		echo "$((u + 10)) quit"
	} > "$T/script.txt"
	until python tools/gamecount.py --limit 5 > /dev/null; do sleep 10; done
	timeout 900 "$EXE" -gamedir=assets/cosmicbugs -userdir="$(cygpath -w "$T/user")" -modsdir="$(cygpath -w "$M")" \
		-livesplitlog="$(cygpath -w "$T/block.log")" -fast -script="$(cygpath -w "$T/script.txt")" > "$T/stdout.txt" 2> "$T/stderr.txt"
	echo "$N: exit $?"
}
inert inert_on "$OUT/mods"
inert inert_off1 "$OUT/mods_off"
inert inert_off2 "$OUT/mods_off"
fi

# ---- checks
ev() { tr -d '\r' < "$OUT/$1/game.log" | grep -c "livesplit: board [0-9]* $2"; }   # <run> <event pattern>: how many of that event
for r in game mission left resume select; do
	grep -q "RESULT ok" "$OUT/$r/drive.log" || fail "$r: the game did not play through ($(tail -n 1 "$OUT/$r/drive.log" 2>/dev/null))"
	[ -s "$OUT/$r/block.log" ] || fail "$r: no block log"
done
# game: one GO, two level clears, one game over (out of lives, kind 0), no win; mode 0, skill 0 and ingame seen; the board is gone at the end
{ [ "$(ev game 'GO at')" = 1 ] && [ "$(ev game 'level clear')" = 2 ] && [ "$(ev game 'game over (kind 0')" = 1 ] && [ "$(ev game 'win')" = 0 ]; } || fail "game: one GO, two level clears, one game over of kind 0 and no win expected"
grep -q " mode=0 skill=0 ingame=1 " "$OUT/game/block.log" || fail "game: mode 0 skill 0 never seen in game"
grep -q " dlevel=3 " "$OUT/game/block.log" || fail "game: board 3 never seen"
tail -n 1 "$OUT/game/block.log" | grep -q "board=0 " || fail "game: the block's last line shows a board"
# mission: one GO, one level clear, one win, no game over
{ [ "$(ev mission 'GO at')" = 1 ] && [ "$(ev mission 'level clear')" = 1 ] && [ "$(ev mission 'win at')" = 1 ] && [ "$(ev mission 'game over')" = 0 ]; } || fail "mission: one GO, one level clear, one win and no game over expected"
grep -q " dlevel=577 " "$OUT/mission/block.log" || fail "mission: board 577 never seen"
grep -q " state=5 " "$OUT/mission/block.log" || fail "mission: the mission-complete state never seen"
# left: two games, each with a GO; the first left (kind 1), the second lost (kind 0)
{ [ "$(ev left 'GO at')" = 2 ] && [ "$(ev left 'game over (kind 1')" = 1 ] && [ "$(ev left 'game over (kind 0')" = 1 ]; } || fail "left: two GOs, a game over of kind 1 and one of kind 0 expected"
grep -q "deaths=1,1,0,0" "$OUT/left/block.log" || fail "left: deaths 1,1,0,0 never seen"
# resume: one GO (the first game), the continued game has none and is flagged RESUMED (flags bit 2)
{ [ "$(ev resume 'GO at')" = 1 ] && grep -q "starts from another screen" "$OUT/resume/game.log"; } || fail "resume: one GO, and the continued game must have none"
grep -q "flags=\(02\|06\|82\|86\) board=[1-9]" "$OUT/resume/block.log" || fail "resume: RESUMED flag not seen"
# select: likewise for a board chosen on the level selection
{ [ "$(ev select 'GO at')" = 1 ] && grep -q "starts from another screen" "$OUT/select/game.log"; } || fail "select: one GO, and the chosen board must have none"
grep -q "flags=\(02\|06\|82\|86\) board=[1-9]" "$OUT/select/block.log" || fail "select: RESUMED flag not seen"
# inert: the state lines
for r in inert_on inert_off1 inert_off2; do
	tr -d '\r' < "$OUT/$r/stderr.txt" | grep "STATE\|state:" > "$OUT/$r/states.txt"
	tr -d '\r' < "$OUT/$r/stdout.txt" | grep "STATE\|state:" >> "$OUT/$r/states.txt"
	[ -s "$OUT/$r/states.txt" ] || fail "$r: no state lines"
done
cmp -s "$OUT/inert_off1/states.txt" "$OUT/inert_off2/states.txt" || fail "inert: two runs with no mod differ (the script is not deterministic)"
cmp -s "$OUT/inert_on/states.txt" "$OUT/inert_off1/states.txt" || fail "inert: the run with the block on differs from the run with it off"
[ -s "$OUT/inert_on/block.log" ] || fail "inert_on: the block was not written"
[ ! -e "$OUT/inert_off1/block.log" ] || fail "inert_off1: a block log with no mod"
grep -q "level clear" "$OUT/inert_on/stderr.txt" "$OUT/inert_on/stdout.txt" || fail "inert_on: the scripted game won no board"
[ "$status" = 0 ] && echo "livesplit tests: ok" || echo "livesplit tests: FAILED"
exit $status
