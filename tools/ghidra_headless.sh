#!/usr/bin/env bash
# Re-process the analyzed game program: apply symbol files, then re-export decompilation.
#   tools/ghidra_headless.sh apply re/symbols/*.jsonl      # apply + export to re/game
#   tools/ghidra_headless.sh export                        # export only
# Another program: GHIDRA_PROJECT=<project> GHIDRA_PROGRAM=<file name> EXPORT_DIR=<dir> (defaults: Bejeweled's).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd -W 2>/dev/null || pwd)"
GHIDRA="${GHIDRA_HOME:-E:/retools/ghidra_12.1.4_PUBLIC}"
export JAVA_HOME="${JAVA_HOME:-E:/retools/jdk-21.0.12.1+1}"
export GHIDRA_HEADLESS_MAXMEM="${GHIDRA_HEADLESS_MAXMEM:-12G}"
PROJ="${GHIDRA_PROJECTS:-E:/retools/projects}"
GPROJECT="${GHIDRA_PROJECT:-bejeweled_game}"
GPROGRAM="${GHIDRA_PROGRAM:-BejeweledGame.exe}"
EXPORT="${EXPORT_DIR:-$ROOT/re/game}"
mode="${1:-export}"; shift || true
scripts=()
if [ "$mode" = apply ]; then
  files=()
  for f in "$@"; do files+=("$(cd "$(dirname "$f")" && pwd -W)/$(basename "$f")"); done
  scripts+=(-postScript ApplySymbols.java "${files[@]}")
fi
if [ "${NO_EXPORT:-0}" != 1 ]; then
  rm -rf "$EXPORT/decomp"
  scripts+=(-postScript ExportAll.java "$EXPORT")
fi
"$GHIDRA/support/analyzeHeadless.bat" "$PROJ" "$GPROJECT" -process "$GPROGRAM" -noanalysis \
  -scriptPath "$ROOT/tools/ghidra_scripts" "${scripts[@]}" 2>&1 \
  | grep -E --line-buffered "ApplySymbols|ExportAll|sig failed|field failed|field rename failed|no struct|no function|failed |ERROR|SEVERE|Exception" || true
