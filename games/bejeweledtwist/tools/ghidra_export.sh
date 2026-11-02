#!/usr/bin/env bash
# Re-export Bejeweled Twist's Ghidra project (E:/retools/projects/bejeweledtwist_game) into re/bejeweledtwist/gh after
# fixing _ftol2's signature (tools/ghidra/FixFtol.java) and applying games/bejeweledtwist/docs/symbols/*.jsonl.
#   bash games/bejeweledtwist/tools/ghidra_export.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd -W 2>/dev/null || pwd)"
GHIDRA="${GHIDRA_HOME:-E:/retools/ghidra_12.1.4_PUBLIC}"
export JAVA_HOME="${JAVA_HOME:-E:/retools/jdk-21.0.12.1+1}"
export GHIDRA_HEADLESS_MAXMEM="${GHIDRA_HEADLESS_MAXMEM:-8G}"
EXPORT="$ROOT/re/bejeweledtwist/gh"
syms=()
for f in "$ROOT"/games/bejeweledtwist/docs/symbols/*.jsonl; do syms+=("$f"); done
# The shared scripts and the game's in one folder: a ';'-separated -scriptPath does not survive analyzeHeadless.bat
SCRIPTS="$ROOT/re/bejeweledtwist/ghscripts"
mkdir -p "$SCRIPTS"
cp "$ROOT"/tools/ghidra_scripts/*.java "$ROOT"/games/bejeweledtwist/tools/ghidra/*.java "$SCRIPTS/"
rm -rf "$EXPORT/decomp"
"$GHIDRA/support/analyzeHeadless.bat" E:/retools/projects bejeweledtwist_game -process BejeweledTwist.exe -noanalysis \
  -scriptPath "$SCRIPTS" \
  -postScript FixFtol.java -postScript ApplySymbols.java "${syms[@]}" -postScript ExportAll.java "$EXPORT" 2>&1 \
  | grep -E --line-buffered "FixFtol|ApplySymbols>|ExportAll>|ERROR|SEVERE|Exception" || true
