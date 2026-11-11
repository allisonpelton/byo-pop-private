#!/usr/bin/env bash
# The port's half of vis3d.sh again, on the steps an earlier desktop run of the original landed at (build/ts-p2/<tour>/vis3d.steps.tsv):
# the port's script from them (mkport3d.py), the port's run, shotdiff.   games/typershark/tools/phase2/port3d.sh <tour name> [2d]
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
N="$1"; MODE="${2:-3d}"
P=games/typershark/tools/phase2
if [ "$MODE" = 2d ]; then SUF=p2d; ORIG=orig2d; FIX=port-win; else SUF=p3d; ORIG=orig3d; FIX=port-3d; fi
python -I $P/mkport3d.py "$N" $([ "$MODE" = 2d ] && echo 2d) > /dev/null || exit 1
USERS=$FIX bash $P/runport.sh "${N}_$SUF" 1 -drawevery | tail -1
python tools/shotdiff/shotdiff.py --game typershark --orig "build/ts-p2/$N/$ORIG" --port "build/ts-p2/${N}_$SUF/port" --run "${N}_$MODE" | tail -1
