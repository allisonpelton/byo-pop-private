#!/usr/bin/env bash
# One pass over the analysed WordHarmonyGame.exe (Ghidra project "wordharmony_game", made by tools/ghidra_import.sh): create the
# virtual functions of re/wordharmony/vfuncs.txt (games/wordharmony/tools/vtables.py), apply the port's symbols
# (games/wordharmony/docs/symbols/*.jsonl), give the indirect calls their argument counts (WHCallOverrides.java)
# and export the decompilation again into re/wordharmony/gh.
#   build/native/rundesk.exe bash games/wordharmony/tools/ghidra_pass.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd -W 2>/dev/null || pwd)"
GHIDRA="${GHIDRA_HOME:-E:/retools/ghidra_12.1.4_PUBLIC}"
export JAVA_HOME="${JAVA_HOME:-E:/retools/jdk-21.0.12.1+1}"
export GHIDRA_HEADLESS_MAXMEM="${GHIDRA_HEADLESS_MAXMEM:-4G}"
PROJ="${GHIDRA_PROJECTS:-E:/retools/projects}"
OUT="$ROOT/re/wordharmony/gh"
# The shared scripts and the game's in one folder: a ';'-separated -scriptPath does not survive analyzeHeadless.bat
SCRIPTS="$ROOT/re/wordharmony/ghscripts"
mkdir -p "$SCRIPTS"
cp "$ROOT"/tools/ghidra_scripts/*.java "$ROOT"/games/wordharmony/tools/ghidra/*.java "$SCRIPTS/"
scripts=()
[ -f "$ROOT/re/wordharmony/vfuncs.txt" ] && scripts+=(-postScript WHCreateFunctions.java "$ROOT/re/wordharmony/vfuncs.txt")
syms=()
for f in "$ROOT"/games/wordharmony/docs/symbols/*.jsonl; do [ -f "$f" ] && syms+=("$f"); done
[ ${#syms[@]} -gt 0 ] && scripts+=(-postScript ApplySymbols.java "${syms[@]}")
scripts+=(-postScript WHCallOverrides.java)
rm -rf "$OUT/decomp"
scripts+=(-postScript ExportAll.java "$OUT")
"$GHIDRA/support/analyzeHeadless.bat" "$PROJ" wordharmony_game -process WordHarmonyGame.exe -noanalysis \
	-scriptPath "$SCRIPTS" "${scripts[@]}" > "$OUT/pass.log" 2>&1 || true
grep -E "WHCreateFunctions:|ApplySymbols|ExportAll.java>|sig failed|no function|ERROR|Exception" "$OUT/pass.log" | tail -12
