#!/bin/bash
# run_tests.sh <out dir>: the autosplit block's tests (src/LiveSplitObserver.cpp). Run it under rundesk, from anywhere:
#   build/native/rundesk.exe bash games/mummymaze/tools/livesplit/run_tests.sh build/ls
#
# 1. Inert: three bot games of the check's suite (classic, tips, tutorial) run three times each, with the block off, with it on
#    (the livesplit mod in -modsdir and -livesplitlog), and with the frozen baseline exe (build/baseline-mummymaze, which
#    predates the block): their logs and screenshots must be identical. With the block off there is no block log.
# 2. Events, each game's block log (-livesplitlog: one line for each event, with the block's fields after it):
#      c1  a classic game from pyramid 0 chamber 13: GO, two level clears, the first treasure, a level clear in pyramid 1 (progress 1),
#          then Abandon Hope (game over, kind 0)
#      c2  from pyramid 14 chamber 13: GO, two level clears, the last treasure and the win
#      a1  an adventure from its start: GO, 15 level clears, the first treasure (progress 1) and on
#      a2  a saved adventure with the first 14 pyramids solved (craft_adventure.py), played to the last treasure: the win, RESUMED set
#      r1  a classic game saved before its clock starts and continued (the check's resume run): RESUMED set, no GO
# 3. The block is in the exe (its magic, once) and the web export is named.
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
OUT="${1:?usage: run_tests.sh <out dir>}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
EXE="$ROOT/build/native/mummymaze.exe"
BASE="$ROOT/build/baseline-mummymaze/mummymaze.exe"
GAMEDIR="$(cygpath -w "$ROOT/assets/mummymaze")"
status=0

rm -rf "$OUT/mods" "$OUT/off" "$OUT/on" "$OUT/base" "$OUT/ev"
mkdir -p "$OUT/mods/livesplit" "$OUT/off" "$OUT/on" "$OUT/base" "$OUT/ev"
cp "$ROOT/games/mummymaze/mods/livesplit/mod.json" "$OUT/mods/livesplit/"

# run <exe> <dir> <extra args> -- <bot args>: a bot game, its log and screenshots in <dir>
run() {
	local exe="$1" d="$2" extra="$3"; shift 3
	rm -rf "$d"; mkdir -p "$d/user" "$d/shots"
	( cd "$d" && timeout 900 "$exe" -gamedir="$GAMEDIR" -userdir="$(cygpath -w "$d/user")" -fast $extra "$@" \
		-autoplaylog="$(cygpath -w "$d/log.txt")" -autoplayshots="$(cygpath -w "$d/shots/s")" > "$d/stdout.txt" 2> "$d/stderr.txt" )
}
ON="-modsdir=$(cygpath -w "$OUT/mods")"

echo "== inert: the block off, on, and the frozen baseline"
declare -A GAMES=(
	[classic]="-seed=1 -autoplay=7 -autoplaygames=1 -autoplaylevels=8 -autoplaycap=60000"
	[tips]="-seed=3 -autoplay=5 -autoplaygames=1 -autoplaystart=3,0 -autoplaylevels=4 -autoplaycap=60000"
	[tutorial]="-seed=1 -autoplay=3 -autoplaymode=tutorial -autoplaycap=100000"
)
for k in classic tips tutorial; do
	run "$EXE" "$OUT/off/$k" "" ${GAMES[$k]}
	run "$EXE" "$OUT/on/$k" "$ON -livesplitlog=$(cygpath -w "$OUT/on/$k/block.log")" ${GAMES[$k]}
	[ -f "$BASE" ] && run "$BASE" "$OUT/base/$k" "" ${GAMES[$k]}
	ok=1
	cmp -s "$OUT/off/$k/log.txt" "$OUT/on/$k/log.txt" && diff -rq "$OUT/off/$k/shots" "$OUT/on/$k/shots" > /dev/null || ok=0
	if [ -f "$BASE" ]; then
		cmp -s "$OUT/base/$k/log.txt" "$OUT/on/$k/log.txt" && diff -rq "$OUT/base/$k/shots" "$OUT/on/$k/shots" > /dev/null || ok=0
	fi
	[ -s "$OUT/on/$k/block.log" ] || [ "$k" = tutorial ] || ok=0    # the tutorial is no game: it has no events
	[ ! -e "$OUT/off/$k/block.log" ] || ok=0
	if [ $ok = 1 ]; then echo "PASS inert $k: log and screenshots identical (off, on${BASE:+, frozen baseline})"
	else echo "FAIL inert $k"; status=1; fi
done

echo "== events"
ev() { # <name> <bot args>: a bot game with the block on and its log
	run "$EXE" "$OUT/ev/$1" "$ON -livesplitlog=$(cygpath -w "$OUT/ev/$1/block.log")" "${@:2}"
	echo "-- $1"; grep -h "livesplit:" "$OUT/ev/$1/stdout.txt" "$OUT/ev/$1/stderr.txt" 2>/dev/null | sed 's/^.*livesplit: /  /' | sed -n '1,12p'
}
ev c1 -seed=1 -autoplay=7 -autoplaygames=1 -autoplaylevels=3 -autoplaystart=0,13 -autoplaycap=200000
ev c2 -seed=1 -autoplay=7 -autoplaygames=1 -autoplaylevels=3 -autoplaystart=14,13 -autoplaycap=200000
ev a1 -seed=1 -autoplay=5 -autoplaymode=adventure -autoplaygames=1 -autoplaylevels=16 -autoplaycap=600000
# a2: the adventure of a1, 14 pyramids solved
mkdir -p "$OUT/ev/a2/user"
python -I "$HERE/craft_adventure.py" "$OUT/ev/a1/user/adv0.sav" "$OUT/ev/a2/user/adv0.sav" 14 > /dev/null
( cd "$OUT/ev/a2" && timeout 900 "$EXE" -gamedir="$GAMEDIR" -userdir="$(cygpath -w "$OUT/ev/a2/user")" -fast $ON -livesplitlog="$(cygpath -w "$OUT/ev/a2/block.log")" \
	-seed=1 -autoplay=5 -autoplaymode=adventure -autoplaygames=1 -autoplaylevels=40 -autoplaycap=1500000 \
	-autoplaylog="$(cygpath -w "$OUT/ev/a2/log.txt")" > stdout.txt 2> stderr.txt )
echo "-- a2"; grep -h "livesplit:" "$OUT/ev/a2/stdout.txt" "$OUT/ev/a2/stderr.txt" | sed 's/^.*livesplit: /  /' | tail -4
# r1: the check's resume run, with the block on
mkdir -p "$OUT/ev/r1/user"
( cd "$OUT/ev/r1" && timeout 300 "$EXE" -gamedir="$GAMEDIR" -userdir="$(cygpath -w "$OUT/ev/r1/user")" -fast -seed=1 -autoplay=3 -autoplaygames=1 -autoplaysave=3 -autoplaycap=3000 \
	-autoplaylog="$(cygpath -w "$OUT/ev/r1/log1.txt")" > stdout1.txt 2> stderr1.txt
  timeout 300 "$EXE" -gamedir="$GAMEDIR" -userdir="$(cygpath -w "$OUT/ev/r1/user")" -fast $ON -livesplitlog="$(cygpath -w "$OUT/ev/r1/block.log")" -seed=1 -autoplay=3 -autoplaygames=1 \
	-autoplaylevels=1 -autoplaycontinue -autoplaycap=60000 -autoplaylog="$(cygpath -w "$OUT/ev/r1/log2.txt")" > stdout2.txt 2> stderr2.txt )
echo "-- r1"; grep -h "livesplit:" "$OUT/ev/r1/stdout2.txt" "$OUT/ev/r1/stderr2.txt" | sed 's/^.*livesplit: /  /' | head -6

check() { # <label> <condition command>
	if eval "$2"; then echo "PASS $1"; else echo "FAIL $1"; status=1; fi
}
bl() { grep -c "$2" "$OUT/ev/$1/block.log"; }
check "c1: GO, level clears 14 and 15 and 16, treasure 1, game over kind 0" \
	'[ "$(bl c1 "GO at")" = 1 ] && [ "$(bl c1 "level clear")" = 3 ] && [ "$(bl c1 "treasure 1")" = 1 ] && [ "$(bl c1 "game over (Abandon Hope)")" = 1 ] && tail -1 "$OUT/ev/c1/block.log" | grep -q "deaths=1,0,0,0.*progress=1"'
check "c2: the last treasure is the win" '[ "$(bl c2 "win at")" = 1 ] && [ "$(bl c2 "level clear 225")" = 1 ]'
check "a1: GO, 15 level clears and the first treasure" '[ "$(bl a1 "GO at")" = 1 ] && [ "$(bl a1 "treasure 1")" = 1 ] && [ "$(grep -c "level clear" "$OUT/ev/a1/block.log")" -ge 15 ]'
check "a2: the win, a resumed game with no GO" '[ "$(bl a2 "win at")" = 1 ] && [ "$(bl a2 "GO at")" = 0 ] && grep -q "flags=1.6" "$OUT/ev/a2/block.log"'
check "r1: a resumed classic game has no GO and RESUMED set" '[ "$(bl r1 "GO at")" = 0 ] && grep -q "resumed a saved game" "$OUT/ev/r1/block.log" && grep -q "flags=0[67]" "$OUT/ev/r1/block.log"'

echo "== the exe"
check "the block's magic occurs once in the exe" '[ "$(grep -a -o "BYOPOP-LIVESPLIT" "$EXE" | wc -l)" = 1 ]'

# Leftovers of this checkout's exe only
powershell -NoProfile -Command "Get-CimInstance Win32_Process -Filter \"Name='mummymaze.exe'\" | Where-Object { \$_.ExecutablePath -eq '$(cygpath -w "$EXE")' } | ForEach-Object { Stop-Process -Id \$_.ProcessId -Force; 'stopped leftover ' + \$_.ProcessId }" 2>/dev/null
exit $status
