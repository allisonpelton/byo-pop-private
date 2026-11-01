#!/usr/bin/env bash
# Runs the original Bejeweled Deluxe (the game carved out of WinBej.exe) on an origrun script inside Windows Sandbox,
# for the identical-play comparison (docs/ORIGRUN.md; games/bejeweled/docs/IDENTICAL_PLAY.md).
#   games/bejeweled/tools/origcmp/orig.sh <run id> <script.txt> [seed] [extra origrun options...]
# The prepared copy lives in build/origrun/bej-master/game (made once by "orig.sh --prepare"); each run gets its own
# copy of it in build/origrun/<run>/game, so a run's saves never reach the next one. The update counter, the draw flags
# and the input gate come from tools/origrun/bejeweled.cfg and the seed patch from tools/origrun/patches/bejeweled.txt
# (both git-ignored). Shots land in build/origrun/<run>/shots, logs in build/origrun/<run>/logs.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
OR="$ROOT/build/origrun-bin/origrun.exe"
MASTER="$ROOT/build/origrun/bej-master/game"
if [ "${1:-}" = "--prepare" ]; then
	rm -rf "$ROOT/build/origrun/bej-master"
	"$OR" --game bejeweled --run bej-master --copy-only
	python "$ROOT/tools/extract_game_exe.py" "$MASTER/WinBej.exe" -o "$MASTER/BejeweledGame.exe"
	exit 0
fi
RUN="$1"; SCRIPT="$2"; SEED="${3:-1}"; shift 2; [ $# -gt 0 ] && shift
[ -f "$MASTER/BejeweledGame.exe" ] || { echo "orig.sh: no prepared copy; run orig.sh --prepare first" >&2; exit 2; }
DIR="$ROOT/build/origrun/$RUN"
rm -rf "$DIR"; mkdir -p "$DIR"
cp -r "$MASTER" "$DIR/game"
# ORIG_SOUND=on: BASS's calls (the music) logged to game/basslog.txt by Bookworm Adventures' proxy (build it into
# build/bassproxy-bwa, games/bookwormadv/tools/bassproxy; Bejeweled asks for the same 48 functions), the sounds by
# origrun's DirectSound stand-in (logs/sound.log). Compare with soundcmp.py.
# ORIG_SAVE=<savegame.dat>: a saved game to start from, staged where this copy reads it in the sandbox: the game
# folder's userdata (tried 2026-11-01: one under C:\ProgramData\PopCap Games\Bejeweled\userdata is not offered)
if [ -n "${ORIG_SAVE:-}" ]; then
	mkdir -p "$DIR/game/userdata" && cp "$ORIG_SAVE" "$DIR/game/userdata/savegame.dat"
fi
if [ "${ORIG_SOUND:-off}" = on ]; then
	mv "$DIR/game/bass.dll" "$DIR/game/bass_real.dll" && cp "$ROOT/build/bassproxy-bwa/bass.dll" "$DIR/game/bass.dll" || exit 1
fi

EXE="$(cygpath -w "$DIR/game/BejeweledGame.exe")"
# The input gate is given here: the per-game config's inputeip is read only inside the sandbox.
IEIP="$(sed -n 's/^inputeip=//p' "$ROOT/tools/origrun/bejeweled.cfg")"
# Windowed, as the port's runs are (port.sh): partner.xml makes the first run full screen, which the sandbox cannot open,
# so the original would fall back to a window while its options still said full screen. ORIG_REG names another preset.
REG="${ORIG_REG:-$ROOT/games/bejeweled/tools/origcmp/windowed.reg}"
"$OR" --game bejeweled --exe "$EXE" --run "$RUN" --sandbox --reg "$(cygpath -w "$REG")" --seed "$SEED" --input-eip "$IEIP" \
	--script "$(cygpath -w "$(cd "$(dirname "$SCRIPT")" && pwd)/$(basename "$SCRIPT")")" --timeout "${ORIG_TIMEOUT:-200}" "$@"
