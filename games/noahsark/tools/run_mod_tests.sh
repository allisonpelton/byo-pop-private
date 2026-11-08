#!/usr/bin/env bash
# Tests of Noah's Ark's mods (games/noahsark/mods): scripted and bot games with the mods on. Run on an invisible
# desktop (CLAUDE.md), which tools/check.sh noahsark does:
#   build/native/rundesk.exe bash games/noahsark/tools/run_mod_tests.sh <out dir>
# Needs build/native/noahsark.exe and assets/noahsark. About 15 seconds. Prints one "ok ..." or "FAIL ..." line
# per check and exits 1 when any failed.
#
#   undo+helper  a script: H shows a move, H again plays it, Ctrl+Z takes it back (the score is 0 again), then Quit
#                to the menu: the saved game is savegame-<key>.dat, not the original's savegame.dat
#   daily        the first screenshot of a level: the same on one day whatever the game's seed, another on another day
#   tide         the action bot: at 400 % the flood ends the first game early; at 0 % it never does
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
cd "$ROOT" || exit 2
OUT="${1:-build/logs/noahsark-mods}"
EXE="$(cygpath -w "$ROOT/build/native/noahsark.exe")"
MODS="$ROOT/games/noahsark/mods"
rm -rf "$OUT"; mkdir -p "$OUT"
FAILED=0
check() {  # <description> <condition result: 0 is ok>
	if [ "$2" -eq 0 ]; then echo "ok   $1"; else echo "FAIL $1"; FAILED=1; fi
}
# a mods folder with the named mods, one of whose settings has another default: <folder> <mod>... ; then
# set_default <folder> <mod> <old default> <new default>
mods_folder() {
	local d="$1"; shift
	mkdir -p "$d"
	for m in "$@"; do cp -r "$MODS/$m" "$d/"; done
}
set_default() { sed -i "s/\"default\": $3/\"default\": $4/" "$1/$2/mod.json"; }

START='301 click 320 456
360 click 198 188
381 click 320 195
420 click 400 405
700 click 400 240'

# ---- undo + helper
mods_folder "$OUT/mods1" undo helper
cat > "$OUT/undo.txt" <<EOF
$START
900 key H
951 key H
1500 keydown CONTROL
1501 key Z
1502 keyup CONTROL
1600 click 70 450
1660 click 308 331
1700 quit
EOF
timeout 120 "$EXE" -userdir="$OUT/user1" -modsdir="$OUT/mods1" -fast -seed=5 -script="$OUT/undo.txt" > "$OUT/undo.log" 2>&1
L="$OUT/undo.log"
grep -q "mod helper: shows a move" "$L"; check "helper: H shows a move" $?
grep -q "mod helper: plays the move" "$L"; check "helper: H again plays it" $?
grep -q "mod undo: move kept (1 to undo)" "$L"; check "undo: the move was kept" $?
grep -q "mod undo: back to 0 points (0 to undo)" "$L"; check "undo: Ctrl+Z took it back to 0 points" $?
ls "$OUT/user1" | grep -q '^savegame-[0-9a-f]*\.dat$' && [ ! -e "$OUT/user1/savegame.dat" ]; check "saved game is kept apart from the original's" $?

# ---- daily board
for v in a b; do mods_folder "$OUT/mods_d$v" daily-board; done
set_default "$OUT/mods_da" daily-board '""' '"2026-10-25"'
set_default "$OUT/mods_db" daily-board '""' '"2026-10-26"'
cat > "$OUT/daily.txt" <<EOF
$START
900 screenshot $OUT/SHOT.png
901 quit
EOF
for run in a1:a:11 a2:a:22 b1:b:33; do
	IFS=: read name which seed <<< "$run"
	sed "s|SHOT|$name|" "$OUT/daily.txt" > "$OUT/daily_$name.txt"
	timeout 120 "$EXE" -userdir="$OUT/user_d$name" -modsdir="$OUT/mods_d$which" -fast -seed=$seed -script="$OUT/daily_$name.txt" > "$OUT/daily_$name.log" 2>&1
done
cmp -s "$OUT/a1.png" "$OUT/a2.png" && [ -s "$OUT/a1.png" ]; check "daily: one day, two games with other seeds: the same board" $?
! cmp -s "$OUT/a1.png" "$OUT/b1.png" && [ -s "$OUT/b1.png" ]; check "daily: another day: another board" $?

# ---- tide
for v in 0 400; do
	mods_folder "$OUT/mods_t$v" tide
	set_default "$OUT/mods_t$v" tide 100 $v
	timeout 150 "$EXE" -userdir="$OUT/user_t$v" -modsdir="$OUT/mods_t$v" -fast -seed=1 -autoplay=1 -autoplaymode=action \
		-autoplaycap=20000 -autoplaylog="$OUT/tide$v.txt" > "$OUT/tide$v.log" 2>&1
done
grep -q "^game 1 over at update 2612: score 250 level 1 moves 22" "$OUT/tide400.txt"; check "tide 400: the flood ends the first game at update 2612" $?
grep -q "^level 4 done at update 16763: score 6421 mode stampede" "$OUT/tide0.txt"; check "tide 0: no flood, level 4 done at update 16763" $?

# ---- fixes (every fix a toggle, on by default)
mods_folder "$OUT/mods_none"
mods_folder "$OUT/mods_fx" fixes
for fix in snapWater quickScores; do
	mods_folder "$OUT/mods_no_$fix" fixes
	sed -i "/\"id\": \"$fix\"/ s/\"default\": true/\"default\": false/" "$OUT/mods_no_$fix/fixes/mod.json"
done
# with every fix on a bot game plays the lines of the original (the bot never ends a get-ready by a click)
for v in none fx; do
	for m in puzzle action; do
		timeout 150 "$EXE" -userdir="$OUT/user_f${v}_$m" -modsdir="$OUT/mods_$v" -fast -seed=1 -autoplay=1 -autoplaymode=$m \
			-autoplaycap=7000 -autoplaylog="$OUT/fx_${v}_$m.txt" > "$OUT/fx_${v}_$m.log" 2>&1
	done
done
# (the lines of the game: its levels and its end; the game-over screen ends sooner, so the bot's last lines come sooner)
for m in puzzle action; do
	grep -E '^(level|game) ' "$OUT/fx_none_$m.txt" > "$OUT/fx_none_$m.game"; grep -E '^(level|game) ' "$OUT/fx_fx_$m.txt" > "$OUT/fx_fx_$m.game"
	cmp -s "$OUT/fx_none_$m.game" "$OUT/fx_fx_$m.game" && [ -s "$OUT/fx_fx_$m.game" ]; check "fixes on: the $m bot's game is the original's" $?
done
# snapWater: level 2 (the water rises from 0 to its level by steps of 3 and overshoots by one); clicks end the get-ready
{
	echo "$START"
	echo "1000 level 2"
	for u in $(seq 1210 10 1290); do echo "$u click 400 240"; done
	echo "1300 state"
	echo "1301 quit"
} > "$OUT/water.txt"
for v in fx no_snapWater; do
	timeout 120 "$EXE" -userdir="$OUT/user_w$v" -modsdir="$OUT/mods_$v" -fast -seed=5 -script="$OUT/water.txt" > "$OUT/water_$v.log" 2>&1
done
on="$(sed -n 's/.*"update":1300,.*"phase":"playing".*"water":\([0-9]*\),.*/\1/p' "$OUT/water_fx.log")"
off="$(sed -n 's/.*"update":1300,.*"phase":"playing".*"water":\([0-9]*\),.*/\1/p' "$OUT/water_no_snapWater.log")"
[ -n "$on" ] && [ -n "$off" ] && [ $((off - on)) -eq 1 ]; check "snapWater: a click that ends the get-ready leaves the water on its level (water $on, the original's $off)" $?
# quickScores: a lost game (the water brought to the ark at update 1000); the first update the game-over screen asks for
# the name or lists the scores
{ echo "$START"; echo "1000 lose"; for u in $(seq 1025 25 2600); do echo "$u state"; done; echo "2601 quit"; } > "$OUT/over.txt"
for v in fx no_quickScores; do
	timeout 120 "$EXE" -userdir="$OUT/user_o$v" -modsdir="$OUT/mods_$v" -fast -seed=5 -script="$OUT/over.txt" > "$OUT/over_$v.log" 2>&1
done
first() { grep -E '"screen":"(entername|highscores)"' "$1" | head -1 | sed -n 's/.*"update":\([0-9]*\),.*/\1/p'; }
on="$(first "$OUT/over_fx.log")"; off="$(first "$OUT/over_no_quickScores.log")"
[ -n "$on" ] && [ -n "$off" ] && [ $((off - on)) -ge 75 ]; check "quickScores: the game-over screen asks for the name at update $on, the original's at $off" $?

exit $FAILED
