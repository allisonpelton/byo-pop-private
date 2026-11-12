#!/usr/bin/env bash
# Autosplit-block tests for Rocket Mania: games with the "livesplit" mod on (the block written, -livesplitlog's lines and the
# events checked) and the proof that the block is inert (a game's state log with the block on, with it off and in the frozen
# baseline must be byte-identical). Run it on an invisible desktop (CLAUDE.md), and never edit this file while it runs (bash reads
# a script as it goes):
#   build/native/rundesk.exe bash games/rocketmania/tools/livesplit/run_tests.sh <out dir>
# Needs build/native/rocketmania.exe, build/baseline-rocketmania/rocketmania.exe and the game files (assets/rocketmania).
# About 8 minutes; at most two games at a time. <out dir>/<run>/block.log is the game's -livesplitlog, stderr.txt holds the
# "livesplit: board ..." line of each event.
#
# Runs:
#   bot_<mode>_<d>     a game played through the mouse by the bot of src/testhooks/UiBot.cpp, which stops moving after 6000 board
#                      updates so that the game ends by itself: GO, the one to three level clears the bot reaches and the game over (arcade:
#                      the top line fills, kind 1; classic: the night runs out, kind 0), on easy (1), normal (2, the game's
#                      medium) and hard (3)
#   script_<mode>_<d>  a game driven by the standard script commands (docs/TEST_COMMANDS.md): "win" and "level <n>" clear 30 levels
#                      in turn, then "lose" ends the game on level 31; strategy has no level end, so its game is a plain "lose"
#   quit               a classic game left through the board's quit button: the block shows the board gone, no game over
#   resume             a classic game saved mid-level (the window closed) and then continued: RESUMED, no GO
#   state_*            three games' state logs (the same games as tools/check.sh rocketmania), each with the block on,
#                      with no mod and in the frozen baseline: the three must be byte-identical
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
OUT="${1:?usage: run_tests.sh <out dir>}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
EXE="$(cygpath -w "$ROOT/build/native/rocketmania.exe")"
BASE="$(cygpath -w "$ROOT/build/baseline-rocketmania/rocketmania.exe")"
status=0
fail() { echo "FAIL: $*"; status=1; }

if [ -z "${CHECK_ONLY:-}" ]; then   # CHECK_ONLY=1: only the checks, on the runs an earlier call left in <out dir>
rm -rf "$OUT"/*/
mkdir -p "$OUT/mods" "$OUT/mods_off"
cp -r "$ROOT/games/rocketmania/mods/livesplit" "$OUT/mods/livesplit"
MODS_ON="$(cygpath -w "$OUT/mods")"
MODS_OFF="$(cygpath -w "$OUT/mods_off")"

play() { # <run> <exe> <mods dir> <mode> <difficulty> <bot seed> <game seed> <extra args...>: a bot game at -fast
	local N="$1" T="$OUT/$1" X="$2" M="$3" mode="$4" diff="$5" bs="$6" gs="$7"; shift 7
	mkdir -p "$T/user"
	rm -f "$T/block.log"
	timeout 900 "$X" -userdir="$(cygpath -w "$T/user")" -modsdir="$M" -livesplitlog="$(cygpath -w "$T/block.log")" -fast -drawevery \
		-seed="$gs" -uibot="$bs" -uistart=300000 -uimodes="$mode" -uidiffs="$diff" -uigames=1 \
		-statelog="$(cygpath -w "$T/state.txt")" -uiscript="$(cygpath -w "$T/script.txt")" "$@" > "$T/stdout.txt" 2> "$T/stderr.txt"
	echo "$N: exit $?"
}
bot() { # <run> <mode> <difficulty>: a bot game that ends by itself
	play "$1" "$EXE" "$MODS_ON" "$2" "$3" 2 777 -uicap=100000 -uipassive=6000
}
scripted() { # <run> <mode name> <difficulty> <levels cleared before the loss>: the standard commands
	local N="$1" T="$OUT/$1" mode="$2" diff="$3" count="$4" u=5000 k
	mkdir -p "$T/user"
	{
		echo "5 startgame"
		echo "$u goto $mode $diff"; u=$((u + 30))
		for ((k = 1; k <= count; k++)); do
			echo "$u win"; u=$((u + 30))
			echo "$u level $((k + 1))"; u=$((u + 30))
		done
		echo "$u lose"; u=$((u + 400))
		echo "$u quit"
	} > "$T/script.txt"
	timeout 900 "$EXE" -userdir="$(cygpath -w "$T/user")" -modsdir="$MODS_ON" -livesplitlog="$(cygpath -w "$T/block.log")" -fast -seed=777 \
		-script="$(cygpath -w "$T/script.txt")" > "$T/stdout.txt" 2> "$T/stderr.txt"
	echo "$N: exit $?"
}

bot bot_arcade_1 0 1 &
bot bot_classic_1 1 1 &
wait
bot bot_arcade_2 0 2 &
bot bot_classic_2 1 2 &
wait
bot bot_arcade_3 0 3 &
bot bot_classic_3 1 3 &
wait
scripted script_arcade_1 arcade 1 30 &
scripted script_arcade_2 arcade 2 30 &
wait
scripted script_arcade_3 arcade 3 30 &
scripted script_classic_1 classic 1 30 &
wait
scripted script_classic_2 classic 2 30 &
scripted script_classic_3 classic 3 30 &
wait
scripted script_strategy_2 strategy 2 0 &
play quit "$EXE" "$MODS_ON" 1 1 2 777 -uicap=1500 &
wait

# resume: the window is closed at update 301100 (a game in progress is saved); the second run continues the save
play resume1 "$EXE" "$MODS_ON" 1 1 2 777 -uicap=100000 -uistop=301100
mkdir -p "$OUT/resume2"
cp -r "$OUT/resume1/user" "$OUT/resume2/user"
play resume2 "$EXE" "$MODS_ON" 1 1 2 777 -uicap=800 -uicontinue=1

# inert: the same games as tools/check.sh rocketmania, with the block on, with no mod and in the frozen baseline
for g in "0 2 2 777" "1 2 1 12345" "2 3 3 4242"; do
	read -r m df bs gs <<< "$g"
	play "state_on_$m" "$EXE" "$MODS_ON" "$m" "$df" "$bs" "$gs" -uicap=3000 &
	play "state_off_$m" "$EXE" "$MODS_OFF" "$m" "$df" "$bs" "$gs" -uicap=3000 &
	wait
	play "state_base_$m" "$BASE" "$MODS_OFF" "$m" "$df" "$bs" "$gs" -uicap=3000
done

fi

# ---- checks
ev() { grep -c "livesplit: board [0-9]* $2" "$OUT/$1/stderr.txt"; }   # <run> <event pattern>: how many of that event
for r in bot_arcade_1 bot_arcade_2 bot_arcade_3 bot_classic_1 bot_classic_2 bot_classic_3; do
	[ -s "$OUT/$r/block.log" ] || { fail "$r: no block log"; continue; }
	[ "$(ev $r 'GO at')" = 1 ] || fail "$r: not exactly one GO"
	[ "$(ev $r 'game over')" = 1 ] || fail "$r: not exactly one game over"
	[ "$(ev $r 'level clear')" -ge 1 ] || fail "$r: no level clear (raise the bot's -uipassive)"
done
for r in script_arcade_1 script_arcade_2 script_arcade_3 script_classic_1 script_classic_2 script_classic_3; do
	[ -s "$OUT/$r/block.log" ] || { fail "$r: no block log"; continue; }
	{ [ "$(ev $r 'GO at')" = 1 ] && [ "$(ev $r 'level clear')" = 30 ] && [ "$(ev $r 'game over')" = 1 ]; } || fail "$r: one GO, 30 level clears and one game over expected"
done
{ [ "$(ev script_strategy_2 'GO at')" = 1 ] && [ "$(ev script_strategy_2 'game over')" = 1 ]; } || fail "script_strategy_2: one GO and one game over expected"
for r in bot_arcade_1 bot_arcade_3 script_arcade_1 script_arcade_3; do
	grep -q "game over (kind 1)" "$OUT/$r/stderr.txt" || fail "$r: arcade's game over is kind 1"
done
for r in bot_classic_1 bot_classic_3 script_classic_1 script_classic_3; do
	grep -q "game over (kind 0)" "$OUT/$r/stderr.txt" || fail "$r: classic's game over is kind 0"
done
grep -q "game over (kind 2)" "$OUT/script_strategy_2/stderr.txt" || fail "strategy's game over is kind 2"
# the mode and skill fields: arcade 0, classic 1, strategy 2; easy 0, normal 1, hard 2
for r in arcade_1:0:0 arcade_2:0:1 arcade_3:0:2 classic_1:1:0 classic_2:1:1 classic_3:1:2; do
	IFS=: read -r n mode skill <<< "$r"
	for kind in bot script; do
		grep -q " mode=$mode skill=$skill ingame=1 " "$OUT/${kind}_$n/block.log" || fail "${kind}_$n: mode $mode skill $skill never seen in game"
	done
done
grep -q " mode=2 skill=1 ingame=1 " "$OUT/script_strategy_2/block.log" || fail "script_strategy_2: mode 2 skill 1 never seen in game"
# a scripted game reaches displayLevel 31, where it is lost
grep -q " dlevel=31 .* ingame=1 " "$OUT/script_classic_2/block.log" || fail "script_classic_2: level 31 never seen"
# quit: a board, a GO and no game over; the block shows no board after it
{ [ "$(ev quit 'GO at')" = 1 ] && [ "$(ev quit 'game over')" = 0 ]; } || fail "quit: one GO and no game over expected"
grep -q " board=0 go=1/1 lc=[0-9/]* over=0/0 " "$OUT/quit/block.log" || fail "quit: no board-less line after the GO without a game over"
# resume: the continued game is flagged and has no GO
[ "$(ev resume1 'GO at')" = 1 ] || fail "resume1: one GO expected"
{ [ "$(ev resume2 'GO at')" = 0 ] && grep -q "continues a saved game" "$OUT/resume2/stderr.txt"; } || fail "resume2: a continued game must have no GO"
grep -q "flags=\(06\|86\) board=[1-9]" "$OUT/resume2/block.log" || fail "resume2: RESUMED flag not seen"
# inert: the state logs
for m in 0 1 2; do
	for k in on off; do
		{ [ -s "$OUT/state_${k}_$m/state.txt" ] && cmp -s "$OUT/state_${k}_$m/state.txt" "$OUT/state_base_$m/state.txt"; } || fail "state_${k}_$m differs from the baseline's"
	done
	[ -s "$OUT/state_on_$m/block.log" ] || fail "state_on_$m: the block was not written"
	[ ! -e "$OUT/state_off_$m/block.log" ] || fail "state_off_$m: a block log with no mod"
done
[ "$status" = 0 ] && echo "livesplit tests: ok" || echo "livesplit tests: FAILED"
exit $status
