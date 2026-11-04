#!/usr/bin/env bash
# Import and analyse any executable in its own Ghidra project, recover RTTI classes, and export the
# decompilation (same pipeline as Bejeweled). At most GHIDRA_SLOTS (default 3) runs execute at once
# machine-wide, so several agents can call this in parallel without exhausting memory.
#   tools/ghidra_import.sh <exe> <project name> <export dir>
# VS.NET 2002 (MSVC 7.0) binaries (docs/POPCAP_GAMES_SURVEY.md 3.4, "Findings that apply to many games"):
# MSVC70=1 disables the noreturn discovery analyzer (DisableNoReturnDiscovery.java) and skips
# RecoverClassesFromRTTIScript, which finds no classes there; SYMBOLS=<file.jsonl> applies symbols (e.g. from tools/rtti_msvc70.py) before export.
set -euo pipefail
EXE="$1"; NAME="$2"; OUT="$3"
ROOT="$(cd "$(dirname "$0")/.." && pwd -W 2>/dev/null || pwd)"
GHIDRA="${GHIDRA_HOME:-E:/retools/ghidra_12.1.4_PUBLIC}"
export JAVA_HOME="${JAVA_HOME:-E:/retools/jdk-21.0.12.1+1}"
export GHIDRA_HEADLESS_MAXMEM="${GHIDRA_HEADLESS_MAXMEM:-4G}"
PROJ="${GHIDRA_PROJECTS:-E:/retools/projects}"
SLOTS="${GHIDRA_SLOTS:-3}"
mkdir -p "$PROJ" "$OUT"

slot=""
while [ -z "$slot" ]; do
	for i in $(seq 1 "$SLOTS"); do
		if mkdir "$PROJ/.slot$i" 2>/dev/null; then slot="$PROJ/.slot$i"; break; fi
	done
	[ -z "$slot" ] && sleep 10
done
trap 'rmdir "$slot" 2>/dev/null || true' EXIT

# RecoverClassesFromRTTIScript can run for hours on large binaries (Bookworm Adventures: 45+ minutes
# without finishing); CLASS_RECOVERY=0 skips it. Auto-analysis still names the RTTI classes and vtables.
PRE=()
POST=()
if [ "${MSVC70:-0}" = "1" ]; then
	PRE+=(-preScript DisableNoReturnDiscovery.java)
	CLASS_RECOVERY="${CLASS_RECOVERY:-0}"
fi
[ "${CLASS_RECOVERY:-1}" != "0" ] && POST+=(-postScript RecoverClassesFromRTTIScript.java)
if [ -n "${SYMBOLS:-}" ]; then
	POST+=(-postScript ApplySymbols.java "$(cd "$(dirname "$SYMBOLS")" && pwd -W 2>/dev/null || pwd)/$(basename "$SYMBOLS")")
fi
POST+=(-postScript ExportAll.java "$OUT")

"$GHIDRA/support/analyzeHeadless.bat" "$PROJ" "$NAME" -import "$EXE" -overwrite \
	-scriptPath "$ROOT/tools/ghidra_scripts" ${PRE[@]+"${PRE[@]}"} "${POST[@]}" -analysisTimeoutPerFile 5400 > "$OUT/headless.log" 2>&1 || true
grep -E "DisableNoReturn|ApplySymbols.java>|ExportAll.java>|Total number of virtual|REPORT: (Analysis|Import)|ERROR.*Exception" "$OUT/headless.log" | tail -12
