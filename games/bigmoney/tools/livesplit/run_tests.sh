#!/usr/bin/env bash
# LiveSplit tests for Big Money: scripted and bot games with the "livesplit" mod, lsprobe.py on two of them,
# and the checks. Run it on an invisible desktop (CLAUDE.md):
#   build/native/rundesk.exe bash games/bigmoney/tools/livesplit/run_tests.sh <out dir>
# Needs build/native/bigmoney.exe and the game files (assets/bigmoney). About 3 minutes; at most three games
# at a time.
#
# Runs (<out dir>/<run>/block.log is the game's -livesplitlog, probe.log lsprobe.py's):
#   bot_strategy   a Strategy Easy bot game: GO, three level clears, overflow (also records a replay)
#   bot_puzzle     two Puzzle Easy bot games: each GO, three level clears, no moves
#   bot_seeded     a Strategy Normal bot game on -gameseed=99
#   newgame        a Strategy game, New Game in level 1, then a second game, then quit
#   quit           bot_strategy's input to just after level 1, then quit
#   resume         bot_strategy's input to the middle of level 1, quit (saved); then a second run continues it
#   replay         bot_strategy's replay, watched with the mod on
#   probe_strategy a bot game at real speed, probed (lsprobe.py) while it runs
#   off            the game without the mod, probed: the block stays inactive
# Checks: check_asl_offsets.py; asltest -compileonly; lsprobe.py --check on both probes; lsdecide.py run (every
# scenario against its expected events and against the real script through asltest).
set -uo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
ROOT="$(cd "$HERE/../../../.." && pwd)"
OUT="${1:?usage: run_tests.sh <out dir>}"
mkdir -p "$OUT"
OUT="$(cd "$OUT" && pwd)"
# Every run starts afresh: a user folder left by an earlier run (its high scores and registry.ini) changes
# how the bot's games go
for d in bot_strategy bot_puzzle bot_seeded newgame quit resume replay probe_strategy off; do
	rm -rf "${OUT:?}/${d:?}"
done
EXE="$(cygpath -w "$ROOT/build/native/bigmoney.exe")"
ASL="$(cygpath -w "$HERE/BigMoneyDeluxe-byopop.asl")"
CSC=/c/Windows/Microsoft.NET/Framework/v4.0.30319/csc.exe
status=0

# The mods: LiveSplit (with replays, which changes nothing in play), and replays only for "off"
mkdir -p "$OUT/mods/livesplit" "$OUT/mods/replays" "$OUT/mods_off/replays"
echo '{ "id": "livesplit", "name": "LiveSplit", "version": "1.0", "features": { "livesplit": true } }' > "$OUT/mods/livesplit/mod.json"
echo '{ "id": "replays", "name": "Replays", "version": "1.0", "features": { "replays": { "keep": 50 } } }' > "$OUT/mods/replays/mod.json"
cp "$OUT/mods/replays/mod.json" "$OUT/mods_off/replays/mod.json"

game() { # <run> <args...>: the game with the mod, its block log and a 400 s limit
	local T="$OUT/$1"; shift
	mkdir -p "$T/user"
	rm -f "$T/block.log"
	timeout 400 "$EXE" -userdir="$T/user" -modsdir="$OUT/mods" -replaydir="$T/replays" -livesplitlog="$T/block.log" "$@" > "$T/log.txt" 2>&1
	echo "$(basename "$T"): exit $?"
}
bot() { # <run> <seed> <mode> <skill> <games> [args...]
	local name=$1 seed=$2 mode=$3 skill=$4 games=$5; shift 5
	game "$name" -fast -seed="$seed" -autoplay="$seed" -autoplaymode="$mode" -autoplayskill="$skill" -autoplaygames="$games" \
		-autoplaycap=60000 -autoplaylog="$OUT/$name/input.txt" "$@"
}

bot bot_strategy 61 1 1 1 &
bot bot_puzzle 62 2 1 2 &
bot bot_seeded 63 1 2 1 -gameseed=99 &
wait

# Scripts from bot_strategy's input (its update numbers: the same seed replays it)
IN="$OUT/bot_strategy/input.txt"
mkdir -p "$OUT/newgame" "$OUT/quit" "$OUT/resume" "$OUT/replay"
{ grep -v '^#' "$IN" | awk '$1 <= 600'; printf '%s\n' "700 move 84 140" "701 click 84 140" "800 move 308 175" "801 click 308 175" \
	"1200 move 400 448" "1201 click 400 448" "1900 quit"; } > "$OUT/newgame/s.txt"
{ grep -v '^#' "$IN" | awk '$1 <= 1589'; echo "1900 quit"; } > "$OUT/quit/s.txt"
{ grep -v '^#' "$IN" | awk '$1 <= 700'; echo "800 quit"; } > "$OUT/resume/s1.txt"
printf '%s\n' "4 move 308 451" "4 click 308 451" "200 move 308 226" "201 click 308 226" "900 quit" > "$OUT/resume/s2.txt"
echo "1500 quit" > "$OUT/replay/s.txt"
REPLAY="$(ls "$OUT"/bot_strategy/replays/*.bmr 2>/dev/null | head -1)"

game newgame -fast -seed=61 -script="$OUT/newgame/s.txt" &
game quit -fast -seed=61 -script="$OUT/quit/s.txt" &
game replay -fast -seed=5 -replay="$(cygpath -w "$REPLAY")" -script="$OUT/replay/s.txt" &
wait

# resume: the first run saves; only the second run's block log counts
T="$OUT/resume"
mkdir -p "$T/user"
timeout 400 "$EXE" -userdir="$T/user" -modsdir="$OUT/mods" -fast -seed=61 -script="$T/s1.txt" > "$T/log1.txt" 2>&1
game resume -fast -seed=7 -script="$T/s2.txt"
# probe_strategy: real speed, probed (no other game starts while a probe waits for one)
mkdir -p "$OUT/probe_strategy" "$OUT/off/user"
python "$HERE/lsprobe.py" --exe "$EXE" --log "$OUT/probe_strategy/probe.log" > "$OUT/probe_strategy/lsprobe.txt" 2>&1 &
PROBE1=$!
sleep 1
game probe_strategy -seed=64 -autoplay=64 -autoplaymode=1 -autoplayskill=3 -autoplaygames=1 -autoplaycap=6000 &
wait
# off: the game without the mod, probed for 15 s of real time
python "$HERE/lsprobe.py" --exe "$EXE" --log "$OUT/off/probe.log" > "$OUT/off/lsprobe.txt" 2>&1 &
sleep 1
printf '%s\n' "1500 quit" > "$OUT/off/s.txt"
timeout 120 "$EXE" -userdir="$OUT/off/user" -modsdir="$OUT/mods_off" -seed=1 -script="$OUT/off/s.txt" > "$OUT/off/log.txt" 2>&1
wait
cat "$OUT/probe_strategy/lsprobe.txt" "$OUT/off/lsprobe.txt"

# Leftovers of this checkout's exe only
powershell -NoProfile -Command "Get-CimInstance Win32_Process -Filter \"Name='bigmoney.exe'\" | Where-Object { \$_.ExecutablePath -eq '$EXE' } | ForEach-Object { Stop-Process -Id \$_.ProcessId -Force; 'stopped leftover ' + \$_.ProcessId }"

echo "== check_asl_offsets"
python "$HERE/check_asl_offsets.py" "$HERE/BigMoneyDeluxe-byopop.asl" "$ROOT/games/bigmoney/src/LiveSplitBlock.h" "$ROOT/build/native/bigmoney.exe" || status=1
echo "== asltest"
"$CSC" -nologo -out:"$(cygpath -w "$OUT/asltest.exe")" -r:Microsoft.CSharp.dll -r:System.Windows.Forms.dll \
	"$(cygpath -w "$HERE/asltest/asltest.cs")" | grep -v "^This compiler\|^$"
"$OUT/asltest.exe" "$ASL" -compileonly || status=1
echo "== lsprobe --check"
python "$HERE/lsprobe.py" --check "$OUT/probe_strategy/probe.log" "$OUT/probe_strategy/block.log" || status=1
echo "== lsdecide"
python "$HERE/lsdecide.py" run "$OUT" --asltest "$(cygpath -w "$OUT/asltest.exe")" --asl "$ASL" || status=1
exit $status
