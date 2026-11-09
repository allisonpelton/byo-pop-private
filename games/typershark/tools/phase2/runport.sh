#!/usr/bin/env bash
# Run the port on a phase-2 script: invisible, silent, uncapped (rundesk, -fast), from a fresh copy of the users folder fixture.
#   games/typershark/tools/phase2/runport.sh <name> [seed] [port args...]
# Reads build/ts-p2/<name>/port.txt (made by tour.py), writes the shots to build/ts-p2/<name>/port/ and the log to port.out.
# TSEXE overrides the exe (a frozen build), USERS the users folder fixture (default build/ts-p1/fixtures/port-clean).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
R="$(cd "$ROOT" && pwd -W)"
N="$1"; SEED="${2:-1}"; shift 2 2>/dev/null || shift $#
D="$ROOT/build/ts-p2/$N"
EXE="${TSEXE:-$R/build/native/typershark.exe}"
bash "$ROOT/games/typershark/tools/phase2/waitroom.sh" || exit 3
rm -rf "$D/port" "$D/ud"; mkdir -p "$D/port"
cp -r "$ROOT/build/ts-p1/fixtures/${USERS:-port-clean}" "$D/ud"
"$ROOT/build/native/rundesk.exe" "$EXE" -fast -seed="$SEED" -userdir="$R/build/ts-p2/$N/ud" -script="$R/build/ts-p2/$N/port.txt" \
	${STATELOG:+-statelog="$R/build/ts-p2/$N/port.sl"} "$@" > "$D/port.out" 2>&1
echo "port exit=$? shots=$(ls "$D/port" | wc -l)" | tee -a "$D/port.out" | tail -1
