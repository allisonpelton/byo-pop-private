#!/usr/bin/env bash
# One pass over the analysed MahjongEscapeAJ.exe (Ghidra project "mahjongjapan", made by tools/ghidra_import.sh): create the
# virtual functions of re/mahjongjapan/vfuncs.txt (games/mahjongjapan/tools/vtables.py), apply the port's symbols
# (games/mahjongjapan/docs/symbols/*.jsonl), commit the decompiler's parameters (MjCommitParams.java), give the indirect calls their argument counts (MjCallOverrides.java)
# and export the decompilation again into re/mahjongjapan/gh.
#   build/native/rundesk.exe bash games/mahjongjapan/tools/ghidra_pass.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd -W 2>/dev/null || pwd)"
GHIDRA="${GHIDRA_HOME:-E:/retools/ghidra_12.1.4_PUBLIC}"
export JAVA_HOME="${JAVA_HOME:-E:/retools/jdk-21.0.12.1+1}"
export GHIDRA_HEADLESS_MAXMEM="${GHIDRA_HEADLESS_MAXMEM:-4G}"
PROJ="${GHIDRA_PROJECTS:-E:/retools/projects}"
OUT="$ROOT/re/mahjongjapan/gh"
# The shared scripts and the game's in one folder: a ';'-separated -scriptPath does not survive analyzeHeadless.bat
SCRIPTS="$ROOT/re/mahjongjapan/ghscripts"
mkdir -p "$SCRIPTS"
cp "$ROOT"/tools/ghidra_scripts/*.java "$ROOT"/games/mahjongjapan/tools/ghidra/*.java "$SCRIPTS/"
scripts=()
[ -f "$ROOT/re/mahjongjapan/vfuncs.txt" ] && scripts+=(-postScript MjCreateFunctions.java "$ROOT/re/mahjongjapan/vfuncs.txt")
syms=()
for f in "$ROOT"/games/mahjongjapan/docs/symbols/*.jsonl; do [ -f "$f" ] && syms+=("$f"); done
[ ${#syms[@]} -gt 0 ] && scripts+=(-postScript ApplySymbols.java "${syms[@]}")
scripts+=(-postScript MjCommitParams.java -postScript MjCallOverrides.java)
rm -rf "$OUT/decomp"
scripts+=(-postScript ExportAll.java "$OUT")
"$GHIDRA/support/analyzeHeadless.bat" "$PROJ" mahjongjapan -process MahjongEscapeAJ.exe -noanalysis \
	-scriptPath "$SCRIPTS" "${scripts[@]}" > "$OUT/pass.log" 2>&1 || true
grep -E "MjCommitParams:|MjCallOverrides:|MjCreateFunctions:|ApplySymbols|ExportAll.java>|sig failed|no function|ERROR|Exception" "$OUT/pass.log" | tail -12
