#!/usr/bin/env bash
# Dumps the instructions of both games' code (MjInsnDump.java, read-only) to re/<game>/insns.txt, for
# games/mahjongchina/tools/compare_exes.py. Needs both Ghidra projects (tools/ghidra_import.sh, then each game's
# ghidra_pass.sh).
#   build/native/rundesk.exe bash games/mahjongchina/tools/insndump.sh
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../../.." && pwd -W 2>/dev/null || pwd)"
GHIDRA="${GHIDRA_HOME:-E:/retools/ghidra_12.1.4_PUBLIC}"
export JAVA_HOME="${JAVA_HOME:-E:/retools/jdk-21.0.12.1+1}"
export GHIDRA_HEADLESS_MAXMEM="${GHIDRA_HEADLESS_MAXMEM:-4G}"
PROJ="${GHIDRA_PROJECTS:-E:/retools/projects}"
SCRIPTS="$ROOT/re/mahjongchina/ghscripts"
mkdir -p "$SCRIPTS"
cp "$ROOT"/games/mahjongchina/tools/ghidra/MjInsnDump.java "$SCRIPTS/"
for g in mahjongjapan:MahjongEscapeAJ.exe mahjongchina:MahjongEscapeAC.exe; do
	name="${g%%:*}"; exe="${g#*:}"
	"$GHIDRA/support/analyzeHeadless.bat" "$PROJ" "$name" -process "$exe" -noanalysis -readOnly \
		-scriptPath "$SCRIPTS" -postScript MjInsnDump.java "$ROOT/re/$name/insns.txt" > "$ROOT/re/$name/insndump.log" 2>&1 || true
	grep -E "MjInsnDump:|ERROR|Exception" "$ROOT/re/$name/insndump.log" | tail -3
done
