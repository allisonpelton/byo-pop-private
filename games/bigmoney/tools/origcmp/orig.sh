#!/usr/bin/env bash
# Runs the original Big Money Deluxe (the game carved out of WinBM.exe) on an origrun script, on the host under
# rundesk and on origrun's virtual clock, for the identical-play comparison (docs/ORIGRUN.md;
# games/bigmoney/docs/IDENTICAL_PLAY.md).
#   games/bigmoney/tools/origcmp/orig.sh --prepare
#   games/bigmoney/tools/origcmp/orig.sh <run id> <script.txt> [seed] [extra origrun options...]
# The prepared copy lives in build/origrun/bm-master/game; each run gets its own copy of it in
# build/origrun/<run>/game without the install's savegame.dat and hiscores.dat (ORIG_SAVE=<file> stages a saved game).
# The seed patch is tools/origrun/patches/bigmoney.txt (git-ignored). The run watches the board's score, state, RNG
# index and level (WATCH3=mt0 watches the RNG's first state word instead of its index, for long Action games, whose
# index moves every update and would pass origrun's 4000 lines); statecmp.py compares them with the port's -statelog.
# Settings: the copy reads HKLM\SOFTWARE\PopCap\BigMoney (the 32-bit view) as it is: it runs without registry
# virtualization, and since origrun is not elevated, its writes there fail, so every run starts from the same settings.
# port.sh gives the port a copy of that key.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
HERE="$ROOT/games/bigmoney/tools/origcmp"
OR="$ROOT/build/origrun-bin/origrun.exe"
MASTER="$ROOT/build/origrun/bm-master/game"
export MSYS_NO_PATHCONV=1
if [ "${1:-}" = "--prepare" ]; then
	rm -rf "$ROOT/build/origrun/bm-master"
	"$OR" --game bigmoney --run bm-master --copy-only
	python "$ROOT/tools/extract_game_exe.py" "$MASTER/WinBM.exe" -o "$MASTER/BigMoneyGame.exe"
	exit 0
fi
RUN="$1"; SCRIPT="$2"; SEED="${3:-1}"; shift 2; [ $# -gt 0 ] && shift
[ -f "$MASTER/BigMoneyGame.exe" ] || { echo "orig.sh: no prepared copy; run orig.sh --prepare first" >&2; exit 2; }
DIR="$ROOT/build/origrun/$RUN"
rm -rf "$DIR"; mkdir -p "$DIR"
cp -r "$MASTER" "$DIR/game"
rm -f "$DIR/game/savegame.dat" "$DIR/game/hiscores.dat"
[ -n "${ORIG_SAVE:-}" ] && cp "$ORIG_SAVE" "$DIR/game/savegame.dat"

APP=0x52ddc8                       # gWinBMApp
BOARD="[$(printf '0x%x' $((APP + 0x6d0)))]"   # mBoard
W3="[$BOARD+0x88]+0x9c0"           # the board MTRand's mti
[ "${WATCH3:-}" = mt0 ] && W3="[$BOARD+0x88]+0x0"
# ORIG_WATCH="<expr> ..." watches other values (up to four; BOARD stands for the board pointer, APP for the app)
WATCHES=(--watch "$BOARD+0x490" --watch "$BOARD+0xa0" --watch "$W3" --watch "$BOARD+0x5b8")
if [ -n "${ORIG_WATCH:-}" ]; then
	WATCHES=()
	for w in $ORIG_WATCH; do w="${w//BOARD/$BOARD}"; WATCHES+=(--watch "${w//APP/$APP}"); done
fi
EXE="$(cygpath -w "$DIR/game/BigMoneyGame.exe")"
"$OR" --game bigmoney --exe "$EXE" --run "$RUN" --clock virtual --cursor scripted --seed "$SEED" \
	--counter "$(printf '0x%x' $((APP + 0x44c)))" "${WATCHES[@]}" \
	--script "$(cygpath -w "$(cd "$(dirname "$SCRIPT")" && pwd)/$(basename "$SCRIPT")")" --timeout "${ORIG_TIMEOUT:-200}" "$@"
