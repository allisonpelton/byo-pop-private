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
# ORIG_NOCACHE=1: without the install's cached/ folder, so the game decodes its Ogg sounds again (and writes them there)
[ "${ORIG_NOCACHE:-0}" = 1 ] && rm -rf "$DIR/game/cached"
# ORIG_SOUND=on: BASS's calls (the music) logged to game/basslog.txt by games/bigmoney/tools/bassproxy (built into
# build/bassproxy-bm), the sounds by origrun's DirectSound stand-in (logs/sound.log). Compare with soundcmp.py.
if [ "${ORIG_SOUND:-off}" = on ]; then
	mv "$DIR/game/bass.dll" "$DIR/game/bass_real.dll" && cp "$ROOT/build/bassproxy-bm/bass.dll" "$DIR/game/bass.dll" || exit 1
fi

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
# ORIG_SANDBOX=1: inside Windows Sandbox, the only place origrun takes shots. Its registry starts empty, so the run
# imports settings.txt (port.sh's PORT_SETTINGS gives the port the same), and the input gate of
# tools/origrun/bigmoney.cfg (git-ignored) is passed on, since inside only the run file carries it. ORIG_CLOCK=real
# runs on the wall clock instead of the virtual one.
MODE=()
if [ "${ORIG_SANDBOX:-0}" = 1 ]; then
	{
		printf 'REGEDIT4\r\n\r\n[HKEY_LOCAL_MACHINE\\SOFTWARE\\PopCap\\BigMoney]\r\n'
		grep -v '^#' "$HERE/settings.txt" | while read -r name type value; do
			[ -n "$name" ] || continue
			if [ "$type" = dword ]; then printf '"%s"=dword:%08x\r\n' "$name" "$value"
			else printf '"%s"="%s"\r\n' "$name" "$value"; fi
		done
	} > "$DIR/settings.reg"
	MODE=(--sandbox --reg "$(cygpath -w "$DIR/settings.reg")" --input-eip "$(sed -n 's/^inputeip=//p' "$ROOT/tools/origrun/bigmoney.cfg")")
fi
[ "${ORIG_CLOCK:-virtual}" = virtual ] && MODE+=(--clock virtual)
EXE="$(cygpath -w "$DIR/game/BigMoneyGame.exe")"
"$OR" --game bigmoney --exe "$EXE" --run "$RUN" "${MODE[@]}" --cursor scripted --seed "$SEED" \
	--counter "$(printf '0x%x' $((APP + 0x44c)))" "${WATCHES[@]}" \
	--script "$(cygpath -w "$(cd "$(dirname "$SCRIPT")" && pwd)/$(basename "$SCRIPT")")" --timeout "${ORIG_TIMEOUT:-200}" "$@"
