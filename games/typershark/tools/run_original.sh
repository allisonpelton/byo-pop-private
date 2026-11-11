#!/usr/bin/env bash
# Run the user's original Typer Shark Deluxe under origrun on the virtual clock (invisible, silent, as fast as it can
# update; CLAUDE.md, "Running games in tests"), with the seed pinned, and keep origrun's log.
#   games/typershark/tools/run_original.sh <script.txt> <log> [seed] [timeout seconds] [extra origrun options...]
# The script is an origrun script (docs/ORIGRUN.md, "Script commands"): `at <N> click|key|text ...` at the game's update
# counter, `at <N> peek <expr>...` to log values from its memory at update N, `quit` last. Waits (up to 30 minutes) until
# the machine has room for another game process (tools/gamecount.py), as every test run must.
# The copy is build/origrun/typershark/game: the install's files plus the real exe carved from WinTS.exe (TyperSharkGame.exe),
# patched by tools/origrun/patches/typershark.txt (the registry check, the seed, a registry key that an unelevated run cannot
# make); both the patch list and tools/origrun/typershark.cfg are git-ignored.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
SCRIPT="$1"; LOG="$2"; SEED="${3:-1}"; LIMIT="${4:-300}"; shift 4 2>/dev/null || shift $#
n=0
until python "$ROOT/tools/gamecount.py" --limit 3 > /dev/null; do
	n=$((n+1)); [ $n -gt 90 ] && { echo "run_original: no room for a game process after 30 minutes" > "$LOG"; exit 3; }
	sleep 20
done
"$ROOT/build/origrun-bin/origrun.exe" --exe "$ROOT/build/origrun/typershark/game/TyperSharkGame.exe" --run typershark \
	--script "$SCRIPT" --clock virtual --seed "$SEED" --counter "[0x565380]+0x4b8" \
	--patches "$ROOT/tools/origrun/patches/typershark.txt" --timeout "$LIMIT" "$@" > "$LOG" 2>&1
echo "exit=$?" >> "$LOG"
