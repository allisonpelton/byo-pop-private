#!/usr/bin/env bash
# One pass over the analysed MahjongEscapeAC.exe (Ghidra project "mahjongchina", made by tools/ghidra_import.sh): create the
# virtual functions of re/mahjongchina/vfuncs.txt (games/mahjongchina/tools/vtables.py), apply the port's symbols
# (games/mahjongchina/docs/symbols/*.jsonl), commit the decompiler's parameters (MjCommitParams.java), give the indirect calls their argument counts (MjCallOverrides.java)
# and export the decompilation again into re/mahjongchina/gh.
#   build/native/rundesk.exe bash games/mahjongchina/tools/ghidra_pass.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd -W 2>/dev/null || pwd)"
GHIDRA="${GHIDRA_HOME:-E:/retools/ghidra_12.1.4_PUBLIC}"
export JAVA_HOME="${JAVA_HOME:-E:/retools/jdk-21.0.12.1+1}"
export GHIDRA_HEADLESS_MAXMEM="${GHIDRA_HEADLESS_MAXMEM:-4G}"
PROJ="${GHIDRA_PROJECTS:-E:/retools/projects}"
OUT="$ROOT/re/mahjongchina/gh"
# The shared scripts and the game's in one folder: a ';'-separated -scriptPath does not survive analyzeHeadless.bat
SCRIPTS="$ROOT/re/mahjongchina/ghscripts"
mkdir -p "$SCRIPTS"
cp "$ROOT"/tools/ghidra_scripts/*.java "$ROOT"/games/mahjongchina/tools/ghidra/*.java "$SCRIPTS/"
scripts=()
[ -f "$ROOT/re/mahjongchina/vfuncs.txt" ] && scripts+=(-postScript MjCreateFunctions.java "$ROOT/re/mahjongchina/vfuncs.txt")
syms=()
for f in "$ROOT"/games/mahjongchina/docs/symbols/*.jsonl; do [ -f "$f" ] && syms+=("$f"); done
[ ${#syms[@]} -gt 0 ] && scripts+=(-postScript ApplySymbols.java "${syms[@]}")
scripts+=(-postScript MjCommitParams.java -postScript MjCallOverrides.java)
rm -rf "$OUT/decomp"
scripts+=(-postScript ExportAll.java "$OUT")
"$GHIDRA/support/analyzeHeadless.bat" "$PROJ" mahjongchina -process MahjongEscapeAC.exe -noanalysis \
	-scriptPath "$SCRIPTS" "${scripts[@]}" > "$OUT/pass.log" 2>&1 || true
grep -E "MjCommitParams:|MjCallOverrides:|MjCreateFunctions:|ApplySymbols|ExportAll.java>|sig failed|no function|ERROR|Exception" "$OUT/pass.log" | tail -12
