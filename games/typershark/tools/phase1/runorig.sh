#!/usr/bin/env bash
# Run the original from a fixed starting state: restore the copy's users folder from a fixture folder (default
# build/ts-p1/fixtures/users-clean), delete the copy's registry key, then run games/typershark/tools/run_original.sh.
#   games/typershark/tools/phase1/runorig.sh <script.txt> <log> [seed] [timeout s] [fixture folder name]
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
SCRIPT="$1"; LOG="$2"; SEED="${3:-1}"; LIMIT="${4:-120}"; FIX="${5:-users-clean}"
GAME="$ROOT/build/origrun/typershark/game"
rm -rf "$GAME/users"; cp -r "$ROOT/build/ts-p1/fixtures/$FIX" "$GAME/users"
reg delete "HKCU\Software\PopCap\TyperShark" //f > /dev/null 2>&1
"$ROOT/games/typershark/tools/run_original.sh" "$SCRIPT" "$LOG" "$SEED" "$LIMIT"
# keep the state the run ended with (P1b: saving, scores and the profile as state): the copy's users folder and the registry key
rm -rf "$LOG.users"; cp -r "$GAME/users" "$LOG.users" 2>/dev/null
reg export "HKCU\Software\PopCap\TyperShark" "$LOG.reg" //y > /dev/null 2>&1
