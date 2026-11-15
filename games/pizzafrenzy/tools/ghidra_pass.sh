#!/usr/bin/env bash
# One pass over the analysed PizzaFrenzyGame.exe (Ghidra project "pizzafrenzy_game", made by tools/ghidra_import.sh with
# CLASS_RECOVERY=0): create the virtual functions of re/pizzafrenzy/vfuncs.txt (games/pizzafrenzy/tools/vtables.py),
# apply the symbol files (re/pizzafrenzy/symbols/*.jsonl: names carried over from Word Harmony's port and the Mac
# builds, docs/README.md, "The export"), give the indirect calls their argument counts (PFCallOverrides.java) and
# export the decompilation again into re/pizzafrenzy/gh. re/ is the main checkout's (git-ignored).
#   build/native/rundesk.exe bash games/pizzafrenzy/tools/ghidra_pass.sh
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd -W 2>/dev/null || pwd)"
REPO="$(cd "$HERE/../../.." && pwd -W 2>/dev/null || pwd)"
MAIN="$(cd "$(git -C "$HERE" rev-parse --git-common-dir)/.." && pwd -W 2>/dev/null || pwd)"
GHIDRA="${GHIDRA_HOME:-E:/retools/ghidra_12.1.4_PUBLIC}"
export JAVA_HOME="${JAVA_HOME:-E:/retools/jdk-21.0.12.1+1}"
export GHIDRA_HEADLESS_MAXMEM="${GHIDRA_HEADLESS_MAXMEM:-4G}"
PROJ="${GHIDRA_PROJECTS:-E:/retools/projects}"
RE="$MAIN/re/pizzafrenzy"
OUT="$RE/gh"
# The shared scripts and the game's in one folder: a ';'-separated -scriptPath does not survive analyzeHeadless.bat
SCRIPTS="$RE/ghscripts"
mkdir -p "$SCRIPTS"
cp "$REPO"/tools/ghidra_scripts/*.java "$HERE"/ghidra/*.java "$SCRIPTS/"
scripts=()
[ -f "$RE/vfuncs.txt" ] && scripts+=(-postScript PFCreateFunctions.java "$RE/vfuncs.txt")
# the code labels the export refers to as data (a signal's handler, a callback), re/pizzafrenzy/labels.txt
[ -f "$RE/labels.txt" ] && scripts+=(-postScript PFCreateFunctions.java "$RE/labels.txt")
syms=()
for f in "$RE"/symbols/*.jsonl; do [ -f "$f" ] && syms+=("$f"); done
[ ${#syms[@]} -gt 0 ] && scripts+=(-postScript ApplySymbols.java "${syms[@]}")
[ "${NO_OVERRIDES:-0}" = 1 ] || scripts+=(-postScript PFCallOverrides.java)
rm -rf "$OUT/decomp"
scripts+=(-postScript ExportAll.java "$OUT")
"$GHIDRA/support/analyzeHeadless.bat" "$PROJ" pizzafrenzy_game -process PizzaFrenzyGame.exe -noanalysis \
	-scriptPath "$SCRIPTS" "${scripts[@]}" > "$OUT/pass.log" 2>&1 || true
grep -E "PFCreateFunctions:|PFCallOverrides:|ApplySymbols|ExportAll.java>|sig failed|no function|ERROR|Exception" "$OUT/pass.log" | tail -12
