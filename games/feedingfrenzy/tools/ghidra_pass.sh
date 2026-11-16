#!/usr/bin/env bash
# One pass over the analysed FeedingFrenzy.exe (Ghidra project "feedingfrenzy_game", made by tools/ghidra_import.sh with
# CLASS_RECOVERY=0): create the virtual functions of re/feedingfrenzy/vfuncs.txt (games/feedingfrenzy/tools/vtables.py),
# apply the symbol files (re/feedingfrenzy/symbols/*.jsonl: names carried over from Word Harmony's port and the Mac
# builds, docs/README.md, "The export"), give the indirect calls their argument counts (FFCallOverrides.java) and
# export the decompilation again into re/feedingfrenzy/gh. re/ is the main checkout's (git-ignored).
#   build/native/rundesk.exe bash games/feedingfrenzy/tools/ghidra_pass.sh
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd -W 2>/dev/null || pwd)"
REPO="$(cd "$HERE/../../.." && pwd -W 2>/dev/null || pwd)"
MAIN="$REPO"
GHIDRA="${GHIDRA_HOME:-E:/retools/ghidra_12.1.4_PUBLIC}"
export JAVA_HOME="${JAVA_HOME:-E:/retools/jdk-21.0.12.1+1}"
export GHIDRA_HEADLESS_MAXMEM="${GHIDRA_HEADLESS_MAXMEM:-4G}"
PROJ="${GHIDRA_PROJECTS:-E:/retools/projects}"
RE="$MAIN/re/feedingfrenzy"
OUT="$RE/gh"
# The shared scripts and the game's in one folder: a ';'-separated -scriptPath does not survive analyzeHeadless.bat
SCRIPTS="$RE/ghscripts"
mkdir -p "$SCRIPTS"
cp "$REPO"/tools/ghidra_scripts/*.java "$HERE"/ghidra/*.java "$SCRIPTS/"
scripts=()
[ -f "$RE/vfuncs.txt" ] && scripts+=(-postScript FFCreateFunctions.java "$RE/vfuncs.txt")
# the code labels the export refers to as data (a signal's handler, a callback), re/feedingfrenzy/labels.txt
[ -f "$RE/labels.txt" ] && scripts+=(-postScript FFCreateFunctions.java "$RE/labels.txt")
syms=()
for f in "$RE"/symbols/*.jsonl; do [ -f "$f" ] && syms+=("$f"); done
[ ${#syms[@]} -gt 0 ] && scripts+=(-postScript ApplySymbols.java "${syms[@]}")
[ "${NO_OVERRIDES:-0}" = 1 ] || scripts+=(-postScript FFCallOverrides.java)
rm -rf "$OUT/decomp"
scripts+=(-postScript ExportAll.java "$OUT")
"$GHIDRA/support/analyzeHeadless.bat" "$PROJ" feedingfrenzy_game -process FeedingFrenzy.exe -noanalysis \
	-scriptPath "$SCRIPTS" "${scripts[@]}" > "$OUT/pass.log" 2>&1 || true
grep -E "FFCreateFunctions:|FFCallOverrides:|ApplySymbols|ExportAll.java>|sig failed|no function|ERROR|Exception" "$OUT/pass.log" | tail -12
