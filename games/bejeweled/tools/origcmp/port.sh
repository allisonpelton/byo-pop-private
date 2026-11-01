#!/usr/bin/env bash
# Runs the port on a -script for the identical-play comparison with the original (orig.sh), invisibly and silently
# under rundesk (CLAUDE.md), with -fast, a fresh user folder and no mods.
#   games/bejeweled/tools/origcmp/port.sh <run id> <script.txt> [seed] [extra game args...]
# The script's "screenshot <name>.png" lines write into build/bejcmp/<run>/shots; stdout goes to
# build/bejcmp/<run>/out.txt. BEJ_EXE names another exe (default build/native/bejeweled.exe).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
RUN="$1"; SCRIPT="$2"; SEED="${3:-1}"; shift 2; [ $# -gt 0 ] && shift
EXE_UNIX="${BEJ_EXE:-$ROOT/build/native/bejeweled.exe}"
EXE="$(cygpath -w "$EXE_UNIX")"
DIR="$ROOT/build/bejcmp/$RUN"
rm -rf "$DIR"; mkdir -p "$DIR/shots" "$DIR/user" "$DIR/mods"
cp "$SCRIPT" "$DIR/script.txt"
# Windowed, as the original's runs are (orig.sh, windowed.reg); PORT_REG names a registry.ini to start from instead
if [ -n "${PORT_REG:-}" ]; then cp "$PORT_REG" "$DIR/user/registry.ini"
else printf '[SOFTWARE\\Steam\\Bejeweled]\nScreenMode=dword:0\n' > "$DIR/user/registry.ini"; fi
[ -n "${PORT_SAVE:-}" ] && cp "$PORT_SAVE" "$DIR/user/savegame.dat"   # a saved game to start from (orig.sh's ORIG_SAVE)
cd "$DIR/shots"
export BYOPOP_SOUNDLOG="$(cygpath -w "$DIR/portsound.log")"   # the framework's sound log, for soundcmp.py
timeout "${PORT_TIMEOUT:-300}" "$ROOT/build/native/rundesk.exe" "$EXE" -gamedir="$(cygpath -w "$ROOT/assets/bejeweled")" \
	-userdir="$(cygpath -w "$DIR/user")" -modsdir="$(cygpath -w "$DIR/mods")" -seed="$SEED" -fast \
	-script="$(cygpath -w "$DIR/script.txt")" "$@" > "$DIR/out.txt" 2>&1
status=$?
# A hung run whose rundesk was killed by timeout: stop only this exe, never another checkout's
powershell -NoProfile -Command "Get-CimInstance Win32_Process -Filter \"Name='$(basename "$EXE_UNIX")'\" | Where-Object { \$_.ExecutablePath -eq '$EXE' } | ForEach-Object { Stop-Process -Id \$_.ProcessId -Force; 'stopped leftover ' + \$_.ProcessId }" 2>/dev/null
exit $status
