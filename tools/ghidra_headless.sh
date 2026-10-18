#!/usr/bin/env bash
# Re-process the analyzed Bejeweled program: apply symbol files, then re-export decompilation.
#   tools/ghidra_headless.sh apply re/bejeweled/symbols/*.jsonl   # apply + export to re/bejeweled/game
#   tools/ghidra_headless.sh export                               # export only
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd -W 2>/dev/null || pwd)"
GHIDRA="${GHIDRA_HOME:-E:/retools/ghidra_12.1.4_PUBLIC}"
export JAVA_HOME="${JAVA_HOME:-E:/retools/jdk-21.0.12.1+1}"
export GHIDRA_HEADLESS_MAXMEM="${GHIDRA_HEADLESS_MAXMEM:-12G}"
PROJ="${GHIDRA_PROJECTS:-E:/retools/projects}"
mode="${1:-export}"; shift || true
scripts=()
if [ "$mode" = apply ]; then
  files=()
  for f in "$@"; do files+=("$(cd "$(dirname "$f")" && pwd -W)/$(basename "$f")"); done
  scripts+=(-postScript ApplySymbols.java "${files[@]}")
fi
if [ "${NO_EXPORT:-0}" != 1 ]; then
  rm -rf "$ROOT/re/bejeweled/game/decomp"
  scripts+=(-postScript ExportAll.java "$ROOT/re/bejeweled/game")
fi
"$GHIDRA/support/analyzeHeadless.bat" "$PROJ" bejeweled_game -process BejeweledGame.exe -noanalysis \
  -scriptPath "$ROOT/tools/ghidra_scripts" "${scripts[@]}" 2>&1 \
  | grep -E --line-buffered "ApplySymbols|ExportAll|sig failed|field failed|field rename failed|no struct|no function|failed |ERROR|SEVERE|Exception" || true
