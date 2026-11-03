#!/usr/bin/env bash
# Bonnie's Bookstore's own Ghidra pass over the imported program (tools/ghidra_import.sh ... bonnies_game re/bonnies/gh):
# creates the functions analysis misses (the static initializers and their atexit destructors, listed in
# re/bonnies/initfuncs.txt by games/bonnies/tools/initfuncs.py), applies symbol files, and re-exports.
#   build/native/rundesk.exe bash games/bonnies/tools/ghidra_pass.sh [symbols.jsonl...]
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd -W 2>/dev/null || pwd)"
GHIDRA="${GHIDRA_HOME:-E:/retools/ghidra_12.1.4_PUBLIC}"
export JAVA_HOME="${JAVA_HOME:-E:/retools/jdk-21.0.12.1+1}"
export GHIDRA_HEADLESS_MAXMEM="${GHIDRA_HEADLESS_MAXMEM:-12G}"
PROJ="${GHIDRA_PROJECTS:-E:/retools/projects}"
EXPORT="$ROOT/re/bonnies/gh"
scripts=(-postScript CreateFunctionsAt.java "$ROOT/re/bonnies/initfuncs.txt")
# PARAMID=1: run the Decompiler Parameter ID analysis once (after the functions above exist)
if [ "${PARAMID:-0}" = 1 ]; then scripts+=(-postScript ParamId.java); fi
if [ $# -gt 0 ]; then
  files=()
  for f in "$@"; do files+=("$(cd "$(dirname "$f")" && pwd -W)/$(basename "$f")"); done
  scripts+=(-postScript ApplySymbols.java "${files[@]}")
fi
# The .bat launcher splits a ';' scriptPath list, so the scripts are gathered in one (git-ignored) folder
SCRIPTS="$ROOT/re/bonnies/ghidra_scripts"
mkdir -p "$SCRIPTS"
cp "$ROOT"/tools/ghidra_scripts/*.java "$ROOT"/games/bonnies/tools/ghidra/*.java "$SCRIPTS/"
rm -rf "$EXPORT/decomp"
scripts+=(-postScript ExportAll.java "$EXPORT")
"$GHIDRA/support/analyzeHeadless.bat" "$PROJ" bonnies_game -process BonniesGame.exe -noanalysis \
  -scriptPath "$SCRIPTS" "${scripts[@]}" 2>&1 \
  | grep -E --line-buffered "CreateFunctionsAt|ParamId|ApplySymbols|ExportAll done|no function|failed |ERROR|SEVERE|Exception" || true
