#!/usr/bin/env bash
# Run build/native/alchemy.exe on a -script file, invisibly and silently (CLAUDE.md: through
# build/native/rundesk.exe), with -fast and a time limit, in a fresh or given user dir.
#   games/alchemy/tools/run_script.sh <script.txt> <user dir> [timeout seconds] [extra game args...]
# The script must end with a "quit" line. After the run, any alchemy.exe left over from *this*
# checkout's build folder is stopped (never processes of other checkouts or sessions).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd)"
SCRIPT="$1"; USERDIR="$2"; LIMIT="${3:-120}"; shift 3 2>/dev/null || shift $#
EXE_UNIX="$ROOT/build/native/alchemy.exe"
EXE="$(cygpath -w "$EXE_UNIX" 2>/dev/null || echo "$EXE_UNIX")"
grep -q "quit" "$SCRIPT" || { echo "run_script: $SCRIPT has no quit line" >&2; exit 2; }
mkdir -p "$USERDIR"
timeout "$LIMIT" "$ROOT/build/native/rundesk.exe" "$EXE" -userdir="$USERDIR" -fast -script="$SCRIPT" "$@"
status=$?
# Leftovers (a hung run whose rundesk was killed by timeout): only this checkout's exe
powershell -NoProfile -Command "Get-CimInstance Win32_Process -Filter \"Name='alchemy.exe'\" | Where-Object { \$_.ExecutablePath -eq '$EXE' } | ForEach-Object { Stop-Process -Id \$_.ProcessId -Force; 'stopped leftover ' + \$_.ProcessId }" 2>/dev/null
exit $status
