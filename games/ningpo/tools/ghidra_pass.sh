#!/usr/bin/env bash
# One pass over the analysed Ningpo.exe (Ghidra project "ningpo", made by tools/ghidra_import.sh): create the
# virtual functions of re/ningpo/vfuncs.txt (games/ningpo/tools/vtables.py), apply the port's symbols
# (games/ningpo/docs/symbols/*.jsonl), give the indirect calls their argument counts (NingpoCallOverrides.java)
# and export the decompilation again into re/ningpo/gh.
#   build/native/rundesk.exe bash games/ningpo/tools/ghidra_pass.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd -W 2>/dev/null || pwd)"
GHIDRA="${GHIDRA_HOME:-E:/retools/ghidra_12.1.4_PUBLIC}"
export JAVA_HOME="${JAVA_HOME:-E:/retools/jdk-21.0.12.1+1}"
export GHIDRA_HEADLESS_MAXMEM="${GHIDRA_HEADLESS_MAXMEM:-4G}"
PROJ="${GHIDRA_PROJECTS:-E:/retools/projects}"
OUT="$ROOT/re/ningpo/gh"
# The shared scripts and the game's in one folder: a ';'-separated -scriptPath does not survive analyzeHeadless.bat
SCRIPTS="$ROOT/re/ningpo/ghscripts"
mkdir -p "$SCRIPTS"
cp "$ROOT"/tools/ghidra_scripts/*.java "$ROOT"/games/ningpo/tools/ghidra/*.java "$SCRIPTS/"
scripts=()
[ -f "$ROOT/re/ningpo/vfuncs.txt" ] && scripts+=(-postScript NingpoCreateFunctions.java "$ROOT/re/ningpo/vfuncs.txt")
syms=()
for f in "$ROOT"/games/ningpo/docs/symbols/*.jsonl; do [ -f "$f" ] && syms+=("$f"); done
[ ${#syms[@]} -gt 0 ] && scripts+=(-postScript ApplySymbols.java "${syms[@]}")
scripts+=(-postScript NingpoCallOverrides.java)
rm -rf "$OUT/decomp"
scripts+=(-postScript ExportAll.java "$OUT")
"$GHIDRA/support/analyzeHeadless.bat" "$PROJ" ningpo -process Ningpo.exe -noanalysis \
	-scriptPath "$SCRIPTS" "${scripts[@]}" > "$OUT/pass.log" 2>&1 || true
grep -E "NingpoCreateFunctions:|ApplySymbols|ExportAll.java>|sig failed|no function|ERROR|Exception" "$OUT/pass.log" | tail -12
