#!/usr/bin/env bash
# Runs the port on a -script for the identical-play comparison with the original (orig.sh), invisibly and silently
# under rundesk (CLAUDE.md), with -fast, a fresh user folder with the original's settings, no mods, and -statelog.
#   games/bigmoney/tools/origcmp/port.sh <run id> <script.txt | -> [seed] [extra game args...]
# ("-" runs no script, for a bot game: -autoplay=... as extra arguments)
# Writes build/bmcmp/<run>/state.txt (the -statelog lines, for statecmp.py) and out.txt. BM_EXE names another exe
# (default build/native/bigmoney.exe); PORT_SAVE=<file> starts from a saved game (orig.sh's ORIG_SAVE);
# PORT_SETTINGS=<settings.txt> gives the sandbox runs' settings instead of a copy of the original's HKLM key.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
HERE="$ROOT/games/bigmoney/tools/origcmp"
RUN="$1"; SCRIPT="$2"; SEED="${3:-1}"; shift 2; [ $# -gt 0 ] && shift
EXE_UNIX="${BM_EXE:-$ROOT/build/native/bigmoney.exe}"
EXE="$(cygpath -w "$EXE_UNIX")"
DIR="$ROOT/build/bmcmp/$RUN"
rm -rf "$DIR"; mkdir -p "$DIR/user" "$DIR/mods"
SARG=()
if [ "$SCRIPT" != - ]; then cp "$SCRIPT" "$DIR/script.txt"; SARG=(-script="$(cygpath -w "$DIR/script.txt")"); fi
# the settings the original starts from (regini.py): its HKLM key, or PORT_SETTINGS for the sandbox runs
if [ -n "${PORT_SETTINGS:-}" ]; then python "$HERE/regini.py" --settings "$PORT_SETTINGS" > "$DIR/user/registry.ini"
else python "$HERE/regini.py" --hklm > "$DIR/user/registry.ini"; fi
[ -n "${PORT_SAVE:-}" ] && cp "$PORT_SAVE" "$DIR/user/savegame.dat"
mkdir -p "$DIR/shots"; cd "$DIR/shots"   # the script's screenshots land here
FAST=(-fast); [ "${PORT_REALTIME:-0}" = 1 ] && FAST=()   # PORT_REALTIME=1: real time (the stats panel's game time)
timeout "${PORT_TIMEOUT:-300}" "$ROOT/build/native/rundesk.exe" "$EXE" -gamedir="$(cygpath -w "$ROOT/assets/bigmoney")" \
	-userdir="$(cygpath -w "$DIR/user")" -modsdir="$(cygpath -w "$DIR/mods")" -seed="$SEED" "${FAST[@]}" \
	"${SARG[@]}" -statelog="$(cygpath -w "$DIR/state.txt")" "$@" > "$DIR/out.txt" 2>&1
status=$?
# A hung run whose rundesk was killed by timeout: stop only this run's game (its user folder is on its command line)
USERDIR_W="$(cygpath -w "$DIR/user")"
powershell -NoProfile -Command "Get-CimInstance Win32_Process -Filter \"Name='$(basename "$EXE_UNIX")'\" | Where-Object { \$_.ExecutablePath -eq '$EXE' -and \$_.CommandLine.Contains('$USERDIR_W') } | ForEach-Object { Stop-Process -Id \$_.ProcessId -Force; 'stopped leftover ' + \$_.ProcessId }" 2>/dev/null
exit $status
