#!/usr/bin/env bash
# The autosplit block's tests for Alchemy (src/LiveSplitObserver.cpp): scripted and bot games with the block on, what it
# logged, and the proof that it is inert. Run it on an invisible desktop (CLAUDE.md):
#   build/native/rundesk.exe bash games/alchemy/tools/livesplit/run_tests.sh <out dir>
# Needs build/native/alchemy.exe, the game files (assets/alchemy) and, for the last check, build/baseline-alchemy/alchemy.exe.
# One game at a time, each waiting for room under the machine's game-process limit (tools/gamecount.py). About 6 minutes.
# The block is turned on by a mod folder holding only a manifest with the feature "livesplit" (<out dir>/mods).
#
# Runs (<out dir>/<run>/block.log is the game's -livesplitlog, err.txt its "livesplit:" lines):
#   strategic1  Strategic, skill 2: the first board is won (the cleared panel comes up), the panel's button, then the forge
#               overflows: GO, one board cleared, a game over of kind 0
#   strategic7  the same with seven boards won first: seven boards cleared
#   timetrial_full     Time Trial: a board won, then the forge filled and left full: kind 1
#   timetrial_discard  Time Trial: a board won, then a discard overflows the forge: kind 3
#   tutorial    the bot plays the tutorial: GO, a win at its last step, no game over (the button leaves for the selector)
#   left        a game, then the selector (the game is left: kind 2), a second game and its game over
#   resume      a bot game quit half way (saved), then a second run that continues it: no GO, flag RESUMED
#   off         the same script as strategic1 with no mod: no block, no log, no "livesplit:" line
# Then the proof that the block changes nothing: three mouse-played games (check.sh's) with the block off, with it on and in the
# frozen baseline: the state logs must be identical (the baseline's has no "rand" field).
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
OUT="${1:?usage: run_tests.sh <out dir>}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
EXE="$(cygpath -w "$ROOT/build/native/alchemy.exe")"
BASE="$(cygpath -w "$ROOT/build/baseline-alchemy/alchemy.exe")"
status=0
cd "$ROOT" || exit 1

mkdir -p "$OUT/mods/livesplit" "$OUT/mods_off"
echo '{ "id": "livesplit", "name": "LiveSplit", "version": "1.0", "author": "test", "description": "test", "affectsScores": false, "features": { "livesplit": true } }' > "$OUT/mods/livesplit/mod.json"

room() { # waits until there is room for one more game process
	local i
	for i in $(seq 1 90); do python tools/gamecount.py --limit 6 > /dev/null && return 0; sleep 10; done
	echo "no room for a game process after 15 minutes" >&2; return 1
}

run() { # <run> <mods dir> <args...>: the game with its block log, a 400 s limit; its stderr in err.txt
	local name=$1 mods=$2; shift 2
	local T="$OUT/$name"; rm -rf "$T"; mkdir -p "$T"
	room || return 1
	timeout 400 "$EXE" -userdir="$(cygpath -w "$T/user")" -modsdir="$(cygpath -w "$mods")" -livesplitlog="$(cygpath -w "$T/block.log")" "$@" \
		> "$T/out.txt" 2> "$T/err.txt"
}

count() { grep -c -E "$2" "$OUT/$1/err.txt" 2>/dev/null; }
check() { echo "$1: $2"; }
expect() { # <run> <regex> <count>
	local n; n="$(count "$1" "$2")"
	if [ "$n" != "$3" ]; then echo "FAIL $1: '$2' expected $3 times, found $n"; status=1; fi
}

# A script: the title's link, a game through the selector's own buttons, then the lines given
script() { # <file> <goto args> [lines...]
	local f=$1 g=$2; shift 2
	{ echo "300 click 320 441"; echo "350 goto $g"; for l in "$@"; do echo "$l"; done; } > "$f"
}
winboards() { # <first update> <boards>: win a board, then the panel's button 150 updates later, every 300 updates
	local u=$1 n=$2 i
	for i in $(seq 0 $((n - 1))); do echo "$((u + i * 300)) win"; echo "$((u + i * 300 + 150)) click 395 416"; done
}

# --- the scenarios
mkdir -p "$OUT/s"
script "$OUT/s/strategic1.txt" "strategic 2" "$(winboards 450 1)" "800 lose" "1500 quit"
script "$OUT/s/strategic7.txt" "strategic 2" "$(winboards 450 7)" "2700 lose" "3400 quit"
script "$OUT/s/timetrial_full.txt" "timetrial 1" "$(winboards 450 1)" "800 set forge 3" "1300 quit"
script "$OUT/s/timetrial_discard.txt" "timetrial 3" "$(winboards 450 1)" "800 lose" "1500 quit"
script "$OUT/s/left.txt" "strategic 3" "500 goto menu" "700 goto strategic 1" "800 lose" "1500 quit"

run strategic1 "$OUT/mods" -fast -script="$OUT/s/strategic1.txt"
expect strategic1 'GO at tick 1 \(mode 0, skill 2\)' 1
expect strategic1 'board cleared 1 at' 1
expect strategic1 'game over \(strategic forge\)' 1
check strategic1 "$(grep -c . "$OUT/strategic1/block.log") block lines"

run strategic7 "$OUT/mods" -fast -script="$OUT/s/strategic7.txt"
expect strategic7 'GO at tick' 1
expect strategic7 'board cleared [1-7] at' 7
expect strategic7 'board cleared 7 at' 1
expect strategic7 'game over \(strategic forge\)' 1

run timetrial_full "$OUT/mods" -fast -script="$OUT/s/timetrial_full.txt"
expect timetrial_full 'GO at tick 1 \(mode 1, skill 1\)' 1
expect timetrial_full 'board cleared 1 at' 1
expect timetrial_full 'game over \(time trial full\)' 1

run timetrial_discard "$OUT/mods" -fast -script="$OUT/s/timetrial_discard.txt"
expect timetrial_discard 'GO at tick 1 \(mode 1, skill 3\)' 1
expect timetrial_discard 'board cleared 1 at' 1
expect timetrial_discard 'game over \(time trial discard\)' 1

run tutorial "$OUT/mods" -fast -autoplay=1 -autoplaymode=tutorial -autoplaycap=20000
expect tutorial 'GO at tick [0-9]+ \(mode 2, skill 0\)' 1
expect tutorial 'win \(tutorial done\)' 1
expect tutorial 'game over' 0

run left "$OUT/mods" -fast -script="$OUT/s/left.txt"
expect left 'GO at tick' 2
expect left 'game over \(left\)' 1
expect left 'game over \(strategic forge\)' 1

# a bot game quit half way is saved; the second run continues it
run resume1 "$OUT/mods" -fast -autoplay=1 -autoplaymode=strategic -autoplaycap=700
cp -r "$OUT/resume1/user" "$OUT/resume_user" 2>/dev/null
room && timeout 400 "$EXE" -userdir="$(cygpath -w "$OUT/resume_user")" -modsdir="$(cygpath -w "$OUT/mods")" -livesplitlog="$(cygpath -w "$OUT/resume2.log")" \
	-fast -autoplay=1 -autoplaymode=strategic -autoplaycap=1500 > "$OUT/resume2.out" 2> "$OUT/resume2.err"
n="$(grep -c 'continues a saved game' "$OUT/resume2.err")"; [ "$n" = 1 ] || { echo "FAIL resume: 'continues a saved game' $n times"; status=1; }
n="$(grep -c ' GO at tick' "$OUT/resume2.err")"; [ "$n" = 0 ] || { echo "FAIL resume: $n GO lines in a continued game"; status=1; }
n="$(grep -c 'flags=06' "$OUT/resume2.log")"; [ "$n" -gt 0 ] || { echo "FAIL resume: no block line with flags 06 (RESUMED and VANILLA)"; status=1; }
check resume "$n block lines with flags 06 (RESUMED and VANILLA), no GO"

run off "$OUT/mods_off" -fast -script="$OUT/s/strategic1.txt"
expect off 'livesplit' 0
[ ! -e "$OUT/off/block.log" ] || { echo "FAIL off: a block log was written"; status=1; }

# --- inert: the state logs of mouse-played games, block off, block on and the frozen baseline
if [ -f "$ROOT/build/baseline-alchemy/alchemy.exe" ]; then
	for a in "strategic 7 1 -1" "timetrial 1 1 -1" "strategic 3 4 2"; do
		read -r m bs gs sk <<< "$a"
		for v in off on base; do
			T="$OUT/state-$v-$m-$gs"; rm -rf "$T"; mkdir -p "$T"; room || continue
			case $v in off) X="$EXE"; M="$OUT/mods_off";; on) X="$EXE"; M="$OUT/mods";; base) X="$BASE"; M="$OUT/mods_off";; esac
			ARGS=(-userdir="$(cygpath -w "$T/user")" -fast -seed="$gs" -autoplayui -autoplaystart=450 -autoplay="$bs" -autoplaymode="$m" -autoplayskill="$sk" -autoplaycap=20000 -statelog="$(cygpath -w "$T/state.txt")")
			[ $v = base ] || ARGS+=(-modsdir="$(cygpath -w "$M")")
			[ $v = on ] && ARGS+=(-livesplitlog="$(cygpath -w "$T/block.log")")
			timeout 600 "$X" "${ARGS[@]}" > "$T/out.txt" 2> "$T/err.txt"
		done
		A="$OUT/state-off-$m-$gs/state.txt"; B="$OUT/state-on-$m-$gs/state.txt"; C="$OUT/state-base-$m-$gs/state.txt"
		if [ -s "$A" ] && cmp -s "$A" "$B"; then r1="block on = off"; else r1="BLOCK CHANGES THE STATE"; status=1; fi
		if [ -s "$C" ] && cmp -s <(grep -v ' rand ' "$A" | tr -d '\r') <(tr -d '\r' < "$C"); then r2="= baseline"; else r2="DIFFERS FROM BASELINE"; status=1; fi
		echo "state $m $gs: $(grep -c . "$A") lines, $r1, $r2, $(grep -c . "$OUT/state-on-$m-$gs/block.log") block lines"
	done
else
	echo "no build/baseline-alchemy/alchemy.exe: the state comparison is skipped"
fi

[ $status -eq 0 ] && echo "livesplit tests: ok" || echo "livesplit tests: FAILED"
exit $status
