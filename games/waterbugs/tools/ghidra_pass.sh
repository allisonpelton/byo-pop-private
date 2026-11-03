#!/usr/bin/env bash
# One pass over the analysed WaterBugs.exe (Ghidra project "waterbugs", made by tools/ghidra_import.sh): apply the
# port's symbols (games/waterbugs/docs/symbols/*.jsonl), commit every game function's parameters so that the callers
# pass the ECX of a __thiscall (WaterCommitParams.java), and export the decompilation again into re/waterbugs/gh.
#   bash games/waterbugs/tools/ghidra_pass.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd -W 2>/dev/null || pwd)"
GHIDRA="${GHIDRA_HOME:-E:/retools/ghidra_12.1.4_PUBLIC}"
export JAVA_HOME="${JAVA_HOME:-E:/retools/jdk-21.0.12.1+1}"
export GHIDRA_HEADLESS_MAXMEM="${GHIDRA_HEADLESS_MAXMEM:-4G}"
PROJ="${GHIDRA_PROJECTS:-E:/retools/projects}"
OUT="$ROOT/re/waterbugs/gh"
# The shared scripts and the game's in one folder: a ';'-separated -scriptPath does not survive analyzeHeadless.bat
SCRIPTS="$ROOT/re/waterbugs/ghscripts"
mkdir -p "$SCRIPTS"
cp "$ROOT"/tools/ghidra_scripts/*.java "$ROOT"/games/waterbugs/tools/ghidra/*.java "$SCRIPTS/"
scripts=()
syms=()
for f in "$ROOT"/games/waterbugs/docs/symbols/*.jsonl; do [ -f "$f" ] && syms+=("$f"); done
[ ${#syms[@]} -gt 0 ] && scripts+=(-postScript ApplySymbols.java "${syms[@]}")
scripts+=(-postScript WaterFpuHelpers.java -postScript WaterCommitParams.java -postScript WaterTailCalls.java)
rm -rf "$OUT/decomp"
scripts+=(-postScript ExportAll.java "$OUT")
"$GHIDRA/support/analyzeHeadless.bat" "$PROJ" waterbugs -process WaterBugs.exe -noanalysis \
	-scriptPath "$SCRIPTS" "${scripts[@]}" > "$OUT/pass.log" 2>&1 || true
grep -E "WaterFpuHelpers:|WaterTailCalls:|WaterCommitParams:|ApplySymbols|ExportAll.java>|ERROR|Exception" "$OUT/pass.log" | tail -12
