#!/usr/bin/env bash
# Run the original Noah's Ark through origrun (host, invisible, virtual clock) on a script, with the four watches that
# games/noahsark/tools/origrun_replay.py compares with the port's -statelog.
#   games/noahsark/tools/run_original.sh <run id> <script> <seed> [extra origrun options]
# Needs build/origrun-bin (tools/origrun/build.sh), build/native/rundesk.exe and tools/origrun/patches/noahsark.txt
# (git-ignored; it pins the generator's seed, see docs/IDENTICAL_PLAY.md). The log is build/origrun/<run id>/run.log.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
RUN="$1"; SCRIPT="$2"; SEED="$3"; shift 3
rm -rf "C:/byo-og/$RUN"   # a fresh copy: the original keeps a saved game in its own folder, which changes the menus
mkdir -p "$ROOT/build/origrun/$RUN"
# SAVE=<savegame.dat> starts the original from a saved game (the menu's Continue): the copy is staged, the file put in its data folder, and that copy run
if [ -n "${SAVE:-}" ]; then
	"$ROOT/build/origrun-bin/origrun.exe" --game noahsark --run "$RUN" --stage-root C:/byo-og --copy-only > "$ROOT/build/origrun/$RUN/stage.log" 2>&1
	mkdir -p "C:/byo-og/$RUN/game/data" && cp "$SAVE" "C:/byo-og/$RUN/game/data/savegame.dat"
fi
# WATCHES="<expr>|<expr>|..." replaces the four default watches (state, score, level, mode): up to four values,
# e.g. board cells "[[[0x4d125c]+0x408]+0x5c+4*36]+0x18" (the type of the piece at x*9+y)
WATCHES="${WATCHES:-[[0x4d125c]+0x408]+0x310|[0x4d125c]+0x474|[0x4d125c]+0x43c|[0x4d125c]+0x444}"
W=(); IFS='|' read -ra EXPRS <<< "$WATCHES"; for e in "${EXPRS[@]}"; do W+=(--watch "$e"); done
# SANDBOX=1 runs inside Windows Sandbox instead (slower, one at a time on the machine; the host run's input did not reach
# the game on a busy machine); the sandbox's log is appended to run.log, where origrun_replay.py reads the watch lines.
if [ -n "${SANDBOX:-}" ]; then
	"$ROOT/build/origrun-bin/origrun.exe" --game noahsark --run "$RUN" --clock virtual --seed "$SEED" --sandbox 		--counter "[0x4d125c]+0x368" "${W[@]}" --script "$SCRIPT" --timeout 200 "$@" > "$ROOT/build/origrun/$RUN/run.log" 2>&1
	cat "$ROOT/build/origrun/$RUN/logs/origrun-sandbox.log" >> "$ROOT/build/origrun/$RUN/run.log" 2>/dev/null
	exit 0
fi
"$ROOT/build/origrun-bin/origrun.exe" --game noahsark --run "$RUN" --stage-root C:/byo-og --clock virtual --seed "$SEED" 	--counter "[0x4d125c]+0x368" "${W[@]}" --script "$SCRIPT" --timeout 200 "$@" > "$ROOT/build/origrun/$RUN/run.log" 2>&1
