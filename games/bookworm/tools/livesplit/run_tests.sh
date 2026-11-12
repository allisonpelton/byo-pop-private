#!/usr/bin/env bash
# LiveSplit tests for BookWorm: scripted and bot games with the "livesplit" mod, lsprobe.py on one of them,
# and the checks. Run it on an invisible desktop (CLAUDE.md):
#   build/native/rundesk.exe bash games/bookworm/tools/livesplit/run_tests.sh <out dir>
# Needs build/native/bookworm.exe and the game files (assets/bookworm). About 3 minutes; at most three games
# at a time. The out dir is made fresh for each run folder below.
#
# Runs (<out dir>/<run>/block.log is the game's -livesplitlog, probe.log lsprobe.py's). The mod folder is
# the real one (games/bookworm/mods/livesplit), so the split templates are copied too:
#   bot_classic    a Classic bot game (-autoplay=8): GO, seven level clears, a fire tile burns through
#   bot_action     an Action bot game (-autoplay=7): GO, twelve level clears, the countdown runs out
#   bot_both       a Classic game and then an Action game in one process (boards 1 and 2)
#   bot_modded     the Classic bot (-autoplay=1) with a second mod that has a score key, to level 3
#   quit           bot_classic's input to just after level 1, then quit
#   leave          bot_classic's input to level 2, LEAVE GAME, then a new game (the saved one declined), quit
#   resume         bot_classic's input to the middle of level 1, quit (saved); then a second run continues it
#   probe_classic  a Classic bot game at real speed, probed (lsprobe.py) while it runs
#   off            the game without the mod, probed: the block stays inactive
#   state_on       a bot run's state log with the block on (a mod folder holding only the manifest), with no mod, and in the
#                  frozen baseline: the three must be byte-identical
# Checks: check_asl_offsets.py; asltest -compileonly; lsprobe.py --check on the probe; lsdecide.py run (every
# scenario against its expected events and against the real script through asltest).
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
OUT="${1:?usage: run_tests.sh <out dir>}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
EXE="$(cygpath -w "$ROOT/build/native/bookworm.exe")"
ASL="$(cygpath -w "$HERE/BookWormDeluxe-byopop.asl")"
CSC=/c/Windows/Microsoft.NET/Framework/v4.0.30319/csc.exe
status=0

for r in bot_classic bot_action bot_both bot_modded quit leave resume probe_classic off mods mods_keyed mods_off; do
	rm -rf "$OUT/$r"
done

# The mods: LiveSplit (the real folder); LiveSplit and a mod with a score key ("affectsScores"), whose games are
# not vanilla; and none, for "off"
mkdir -p "$OUT/mods" "$OUT/mods_keyed/keyed" "$OUT/mods_off"
cp -r "$ROOT/games/bookworm/mods/livesplit" "$OUT/mods/livesplit"
cp -r "$ROOT/games/bookworm/mods/livesplit" "$OUT/mods_keyed/livesplit"
echo '{ "id": "keyed", "name": "Keyed", "version": "1.0", "affectsScores": true }' > "$OUT/mods_keyed/keyed/mod.json"

game() { # <run> <mods dir> <args...>: the game with its block log and a 400 s limit
	local T="$OUT/$1" M="$2"; shift 2
	mkdir -p "$T/user"
	rm -f "$T/block.log"
	timeout 400 "$EXE" -userdir="$T/user" -modsdir="$M" -livesplitlog="$T/block.log" "$@" > "$T/log.txt" 2>&1
	echo "$(basename "$T"): exit $?"
}
bot() { # <run> <seed> <mode> <mods dir> [args...]: a bot game at -fast
	local name=$1 seed=$2 mode=$3 mods=$4; shift 4
	mkdir -p "$OUT/$name"
	game "$name" "$mods" -fast -seed="$seed" -autoplay="$seed" -autoplaymode="$mode" -autoplaycap=100000 \
		-autoplaylog="$OUT/$name/input.txt" "$@"
}

bot bot_classic 8 classic "$OUT/mods" &
bot bot_action 7 action "$OUT/mods" &
bot bot_both 2 both "$OUT/mods" &
wait

# Scripts from bot_classic's input (its update numbers: the same seed replays it). Level 1 is clear at update
# 886 and its summary's continue is clicked at 1271 (the bot's next click is at 1320); the coordinates below are the 640x480 screen's: LEAVE GAME on the board,
# "Quit!" in its dialog, the Classic book on the selector, "New Game" in the continue dialog, the help
# screen's continue button
IN="$OUT/bot_classic/input.txt"
mkdir -p "$OUT/quit" "$OUT/leave" "$OUT/resume"
{ grep -v '^#' "$IN" | awk '$1 <= 1290'; echo "1500 quit"; } > "$OUT/quit/s.txt"
{ grep -v '^#' "$IN" | awk '$1 <= 1290'; printf '%s\n' "1400 move 111 461" "1401 click 111 461" "1500 move 365 307" "1501 click 365 307" \
	"1900 move 485 89" "1901 click 485 89" "2000 move 506 316" "2001 click 506 316" "2100 move 314 444" "2101 click 314 444" \
	"2500 quit"; } > "$OUT/leave/s.txt"
{ grep -v '^#' "$IN" | awk '$1 <= 700'; echo "800 quit"; } > "$OUT/resume/s1.txt"

bot bot_modded 1 classic "$OUT/mods_keyed" -autoplaycap=3000 &
game quit "$OUT/mods" -fast -seed=8 -script="$OUT/quit/s.txt" &
game leave "$OUT/mods" -fast -seed=8 -script="$OUT/leave/s.txt" &
wait

# resume: the first run saves the game in progress; only the second run's block log counts (the bot there
# answers the continue question with yes)
T="$OUT/resume"
mkdir -p "$T/user"
timeout 400 "$EXE" -userdir="$T/user" -modsdir="$OUT/mods" -fast -seed=8 -script="$T/s1.txt" > "$T/log1.txt" 2>&1
game resume "$OUT/mods" -fast -seed=7 -autoplay=7 -autoplaymode=classic -autoplaycontinue=1 -autoplaycap=2500

# probe_classic: real speed, probed (no other game starts while a probe waits for one)
mkdir -p "$OUT/probe_classic" "$OUT/off/user"
python "$HERE/lsprobe.py" --exe "$EXE" --log "$OUT/probe_classic/probe.log" > "$OUT/probe_classic/lsprobe.txt" 2>&1 &
sleep 1
game probe_classic "$OUT/mods" -seed=64 -autoplay=64 -autoplaymode=classic -autoplaycap=6500
wait
# off: the game without the mod, probed for 17 s of real time
python "$HERE/lsprobe.py" --exe "$EXE" --log "$OUT/off/probe.log" > "$OUT/off/lsprobe.txt" 2>&1 &
sleep 1
printf '%s\n' "1700 quit" > "$OUT/off/s.txt"
timeout 120 "$EXE" -userdir="$OUT/off/user" -modsdir="$OUT/mods_off" -seed=1 -script="$OUT/off/s.txt" > "$OUT/off/log.txt" 2>&1
wait
cat "$OUT/probe_classic/lsprobe.txt" "$OUT/off/lsprobe.txt"

# state_on: the block changes nothing in play. The game's state log (-statelog: the board at every update) of a bot run
# with the block on, in a mod folder that holds only the livesplit manifest, against the same run with no mod and, when it
# is there, the frozen baseline's (built before the block existed)
rm -rf "$OUT/state_on"
mkdir -p "$OUT/state_on/mods/livesplit" "$OUT/state_on/mods_off"
echo '{ "id": "livesplit", "name": "LiveSplit", "version": "1.0", "author": "byo-pop", "description": "Block only.", "affectsScores": false, "features": { "livesplit": true } }' \
	> "$OUT/state_on/mods/livesplit/mod.json"
state_run() { # <run> <exe> <mods dir>
	mkdir -p "$OUT/state_on/$1/user"
	timeout 900 "$2" -userdir="$OUT/state_on/$1/user" -modsdir="$3" -fast -statelog="$OUT/state_on/$1/state.txt" \
		-seed=9 -autoplay=7 -autoplaymode=classic,action,classic > "$OUT/state_on/$1/log.txt" 2>&1
}
BASE="$ROOT/build/baseline-bookworm/bookworm.exe"
state_run on "$EXE" "$OUT/state_on/mods" &
state_run off "$EXE" "$OUT/state_on/mods_off" &
[ -f "$BASE" ] && state_run base "$(cygpath -w "$BASE")" "$OUT/state_on/mods_off" &
wait

# Leftovers of this checkout's exe only
powershell -NoProfile -Command "Get-CimInstance Win32_Process -Filter \"Name='bookworm.exe'\" | Where-Object { \$_.ExecutablePath -eq '$EXE' } | ForEach-Object { Stop-Process -Id \$_.ProcessId -Force; 'stopped leftover ' + \$_.ProcessId }"

echo "== check_asl_offsets"
python "$HERE/check_asl_offsets.py" "$HERE/BookWormDeluxe-byopop.asl" "$ROOT/games/bookworm/src/LiveSplitBlock.h" "$ROOT/build/native/bookworm.exe" || status=1
echo "== asltest"
"$CSC" -nologo -out:"$(cygpath -w "$OUT/asltest.exe")" -r:Microsoft.CSharp.dll -r:System.Windows.Forms.dll \
	"$(cygpath -w "$HERE/asltest/asltest.cs")" | grep -v "^This compiler\|^$"
"$OUT/asltest.exe" "$ASL" -compileonly || status=1
echo "== lsprobe --check"
python "$HERE/lsprobe.py" --check "$OUT/probe_classic/probe.log" "$OUT/probe_classic/block.log" || status=1
echo "== state with the block on"
S="$OUT/state_on"
if [ -s "$S/on/state.txt" ] && grep -q "livesplit: block at" "$S/on/log.txt" && cmp -s "$S/on/state.txt" "$S/off/state.txt" \
	&& { [ ! -f "$BASE" ] || cmp -s "$S/on/state.txt" "$S/base/state.txt"; }; then
	echo "state log identical with the block on and off$([ -f "$BASE" ] && echo ' and in the frozen baseline') ($(wc -l < "$S/on/state.txt") lines)"
else
	echo "FAILED: the state log differs with the block on, or the block was never written"; status=1
fi
echo "== lsdecide"
python "$HERE/lsdecide.py" run "$OUT" --asltest "$(cygpath -w "$OUT/asltest.exe")" --asl "$ASL" || status=1
exit $status
