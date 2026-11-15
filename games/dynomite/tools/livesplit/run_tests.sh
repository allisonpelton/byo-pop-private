#!/usr/bin/env bash
# The autosplit block's tests for Dynomite (src/LiveSplitObserver.cpp): games of each mode with the block on, what it logged,
# and the proof that it is inert.
#   bash games/dynomite/tools/livesplit/run_tests.sh <out dir>
# Every game runs under build/native/rundesk.exe (this script starts each one there; botrec.py starts its own). Needs
# build/native/dynomite.exe, the game files (assets/dynomite) and, for the last check, build/baseline-dynomite/dynomite.exe.
# One game at a time, each waiting for room under the machine's game-process limit (tools/gamecount.py). About 5 minutes.
# The block is turned on by a mod folder holding only a manifest with the feature "livesplit" (<out dir>/mods).
#
# Games (<out dir>/<run>/block.log is the game's -livesplitlog, err.txt its "livesplit:" lines). Each is first played by the
# port's own bot (tools/shots/botrec.py, without the block), and its recorded input then played again with the block on:
#   tutorial    the whole tutorial: GO, its four parts done, the win as it closes to the main menu
#   endless     Endless, the skill the settings keep: GO, then the bot's shots give way to shots straight up after 30 moves: a game over (endless)
#   timetrial   Time Trial, Hard: GO at the first shot, the board cleared: the win
#   fossil      Fossil Challenge, Strategic, from fossil 14 (the level command): fossils 14 and 15 found, the win
#   panic       Fossil Challenge, Panic: GO, a game over (fossil)
#   stomped     Stomped: GO, its rounds cleared, a game over (stomped)
# Scripted (identical/mkscript.py's menu clicks, then test commands):
#   left        Endless (mkscript.py's menu clicks: a click on the mode already chosen goes on, so the game starts at the
#               settings' skill), a few shots, the main menu (left: kind 2), Endless again and a loss
#   saved       Stomped, five shots, saved into the first slot (the game is left), then continued from the main menu: no GO,
#               flag RESUMED
#   off         the endless game again with no mod: no block, no log, no "livesplit:" line
# Then the proof that the block changes nothing: check.sh's three identical-play cases with the block off, with it on and in
# the frozen baseline: the state logs must be identical.
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
OUT="${1:?usage: run_tests.sh <out dir>}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
cd "$ROOT" || exit 1
RUNDESK="$ROOT/build/native/rundesk.exe"
EXE="$(cygpath -w "$ROOT/build/native/dynomite.exe")"
BASE="$(cygpath -w "$ROOT/build/baseline-dynomite/dynomite.exe")"
MKSCRIPT=games/dynomite/tools/identical/mkscript.py
MKHS=games/dynomite/tools/identical/mkhs.py
status=0

mkdir -p "$OUT/mods/livesplit" "$OUT/mods_off"
echo '{ "id": "livesplit", "name": "LiveSplit", "version": "1.0", "author": "test", "description": "test", "affectsScores": false, "features": { "livesplit": true } }' > "$OUT/mods/livesplit/mod.json"

room() { # waits until there is room for one more game process
	local i
	for i in $(seq 1 90); do python tools/gamecount.py --limit 6 > /dev/null && return 0; sleep 10; done
	echo "no room for a game process after 15 minutes" >&2; return 1
}

user() { # <dir>: the settings botrec.py and identical/port.sh give the port (windowed) and mkhs.py's low tables
	mkdir -p "$1"
	printf '%s\n' '[SOFTWARE\PopCap\Dynomite]' CustomCursors=dword:0 InProgress=dword:0 MusicVolume=dword:50 Muted=dword:0 \
		PreferredX=dword:-1 PreferredY=dword:-1 ScreenMode=dword:0 SfxVolume=dword:85 TimesExecuted=dword:5 \
		TimesPlayed=dword:0 WaitForVSync=dword:0 > "$1/registry.ini"
	python "$MKHS" "$1"
}

play() { # <run> <mods dir> <seed> <script> [user dir to keep]: the script with the block log, a 600 s limit; stderr in err.txt
	local name=$1 mods=$2 seed=$3 script=$4 keep=${5:-}
	local T="$OUT/$name"; rm -rf "$T"; mkdir -p "$T"
	if [ -n "$keep" ]; then cp -r "$keep" "$T/user"; else user "$T/user"; fi
	room || return 1
	timeout 600 "$RUNDESK" "$EXE" -gamedir=assets/dynomite -userdir="$(cygpath -w "$T/user")" -modsdir="$(cygpath -w "$mods")" \
		-livesplitlog="$(cygpath -w "$T/block.log")" -seed="$seed" -fast -drawevery -script="$(cygpath -w "$script")" \
		> "$T/out.txt" 2> "$T/err.txt"
	tr -d '\r' < "$T/err.txt" | grep -o 'livesplit: .*' > "$T/events.txt"
}

record() { # <run> <mode> <seed> [botrec options...]: the bot's game, recorded as a script, then played with the block on
	local name=$1 mode=$2 seed=$3; shift 3
	local R="$OUT/rec-$name"; rm -rf "$R"; mkdir -p "$R"
	room || return 1
	python games/dynomite/tools/shots/botrec.py "$mode" "$seed" "$R/script.txt" "$@" > "$R/botrec.txt" 2>&1 ||
		{ echo "FAIL $name: botrec.py failed ($(tail -n 1 "$R/botrec.txt"))"; status=1; return 1; }
	play "$name" "$OUT/mods" "$seed" "$R/script.txt"
}

count() { grep -c -E "$2" "$OUT/$1/events.txt" 2>/dev/null; }
expect() { # <run> <regex> <count>
	local n; n="$(count "$1" "$2")"
	if [ "$n" != "$3" ]; then echo "FAIL $1: '$2' expected $3 times, found $n"; status=1; fi
}
summary() { echo "$1: $(grep -c . "$OUT/$1/block.log" 2>/dev/null) block lines; $(paste -sd ';' "$OUT/$1/events.txt" | cut -c1-400)"; }

# --- the bot's games
record tutorial tutorial 1
expect tutorial 'tutorial [0-9]+ GO \(mode 5, skill 0\)' 1
expect tutorial 'part done' 4
expect tutorial 'win \(every part' 1
expect tutorial 'game over' 0
summary tutorial

record endless endless 2 --moves 30
expect endless 'GO at tick 0 \(mode 2, skill [1-3]\)' 1
expect endless 'game over \(endless\)' 1
summary endless

record timetrial timetrial 3
expect timetrial 'GO at tick [0-9]+ \(mode 4, skill 2\)' 1
expect timetrial 'win \(cleared\)' 1
expect timetrial 'game over' 0
summary timetrial

record fossil fossil 4 --level 14 --winafter 1
expect fossil 'GO at tick 0 \(mode 3, skill 1\)' 1
expect fossil 'fossil 14 found' 1
expect fossil 'fossil 15 found' 1
expect fossil 'win \(the last fossil\)' 1
expect fossil 'game over' 0
summary fossil

record panic panic 5 --moves 20
expect panic 'GO at tick 0 \(mode 3, skill 2\)' 1
expect panic 'game over \(fossil\)' 1
summary panic

record stomped stomped 6 --moves 30
expect stomped 'GO at tick 0 \(mode 1, skill [1-3]\)' 1
expect stomped 'game over \(stomped\)' 1
summary stomped

# --- scripted
mkdir -p "$OUT/s"
python "$MKSCRIPT" "$OUT/s/left.txt" 25000 7 title menu:endless:hard shots:5:40 wait:100
last=$(tail -n 1 "$OUT/s/left.txt" | cut -d' ' -f1)
printf '%s\n' "$((last + 10)) goto menu" "$((last + 60)) goto endless" "$((last + 100)) move 400 300" \
	"$((last + 110)) click 400 300" "$((last + 160)) lose" "$((last + 700)) quit" >> "$OUT/s/left.txt"
play left "$OUT/mods" 7 "$OUT/s/left.txt"
expect left 'GO at tick' 2
expect left 'game over \(left\)' 1
expect left 'game over \(endless\)' 1
summary left

# check.sh's saved game: Stomped, five shots, the side bar's Save into the first slot (back to the main menu); then Continue
python "$MKSCRIPT" "$OUT/s/save.txt" 25000 8 title menu:stomped:pack shots:5:40 wait:100 save:Bot wait:100 quit
play saved1 "$OUT/mods" 8 "$OUT/s/save.txt"
python "$MKSCRIPT" "$OUT/s/cont.txt" 25000 9 title cont shots:3:40 wait:200 quit
play saved2 "$OUT/mods" 9 "$OUT/s/cont.txt" "$OUT/saved1/user"
expect saved1 'GO at tick' 1
expect saved1 'game over \(left\)' 1
expect saved2 'continues a saved game' 1
expect saved2 'GO at tick' 0
n="$(grep -c 'flags=0a' "$OUT/saved2/block.log" 2>/dev/null)"
[ "${n:-0}" -gt 0 ] || { echo "FAIL saved2: no block line with flags 0a (RESUMED and CHOSEN_SEED)"; status=1; }
summary saved1; summary saved2

play off "$OUT/mods_off" 2 "$OUT/rec-endless/script.txt"
expect off 'livesplit' 0
[ ! -e "$OUT/off/block.log" ] || { echo "FAIL off: a block log was written"; status=1; }

# --- inert: check.sh's three identical-play cases (tools/check.sh, suite_dynomite), block off, block on and the baseline
if [ -f "$ROOT/build/baseline-dynomite/dynomite.exe" ]; then
	for a in "en 2 3 title menu:endless shots:80:40 wait:300 text:Zed click:352:369 wait:200 click:400:400 wait:300 shots:80:40 quit" \
		"st 4 5 title menu:stomped:pack shots:45:40 wait:300 text:Sam click:352:369 wait:200 quit" \
		"fp 8 9 title menu:fossil:panic shots:60:40 wait:300 quit"; do
		set -- $a; n=$1; seed=$2; r=$3; shift 3
		for v in off on base; do
			T="$OUT/state-$v-$n"; rm -rf "$T"; mkdir -p "$T"
			# check.sh's settings (dynomite_user: full screen) and mkhs.py's tables, as its dynomite_state
			mkdir -p "$T/user"
			printf '%s\n' '[SOFTWARE\PopCap\Dynomite]' CustomCursors=dword:0 InProgress=dword:0 MusicVolume=dword:50 Muted=dword:0 \
				PreferredX=dword:-1 PreferredY=dword:-1 ScreenMode=dword:1 SfxVolume=dword:85 TimesExecuted=dword:5 \
				TimesPlayed=dword:0 WaitForVSync=dword:0 > "$T/user/registry.ini"
			python "$MKHS" "$T/user"
			python "$MKSCRIPT" "$T/script.txt" 25000 "$r" "$@"
			case $v in off) X="$EXE"; M="$OUT/mods_off";; on) X="$EXE"; M="$OUT/mods";; base) X="$BASE"; M="";; esac
			ARGS=(-gamedir=assets/dynomite -userdir="$(cygpath -w "$T/user")" -fast -drawevery -seed="$seed" \
				-script="$(cygpath -w "$T/script.txt")" -statelog="$(cygpath -w "$T/state.txt")")
			[ -n "$M" ] && ARGS+=(-modsdir="$(cygpath -w "$M")")
			[ $v = on ] && ARGS+=(-livesplitlog="$(cygpath -w "$T/block.log")")
			room || continue
			timeout 300 "$RUNDESK" "$X" "${ARGS[@]}" > "$T/out.txt" 2> "$T/err.txt"
		done
		A="$OUT/state-off-$n/state.txt"; B="$OUT/state-on-$n/state.txt"; C="$OUT/state-base-$n/state.txt"
		if [ -s "$A" ] && cmp -s "$A" "$B"; then r1="block on = off"; else r1="BLOCK CHANGES THE STATE"; status=1; fi
		if [ -s "$C" ] && cmp -s "$A" "$C"; then r2="= baseline"; else r2="DIFFERS FROM BASELINE"; status=1; fi
		echo "state $n: $(grep -c . "$A") lines, $r1, $r2, $(grep -c . "$OUT/state-on-$n/block.log" 2>/dev/null) block lines"
	done
else
	echo "no build/baseline-dynomite/dynomite.exe: the state comparison is skipped"
fi

[ $status -eq 0 ] && echo "livesplit tests: ok" || echo "livesplit tests: FAILED"
exit $status
