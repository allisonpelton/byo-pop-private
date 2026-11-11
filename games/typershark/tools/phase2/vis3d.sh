#!/usr/bin/env bash
# One tour of the 3D comparison with the original (GAME_NOTES.md, "The 3D drawing"): the original in 3D on the user's visible desktop
# (visrun3d.py), then the port on the same inputs at the updates they landed at, then shotdiff.
#   games/typershark/tools/phase2/vis3d.sh <tour name> [2d]
# The tour's script must exist (python tour_<x>.py <name>: build/ts-p2/<name>/orig.txt). With 2d both runs are in 2D (the check that the
# desktop method gives what the sandbox's runs did).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
N="$1"; MODE="${2:-3d}"
P=games/typershark/tools/phase2
if [ "$MODE" = 2d ]; then SUF=p2d; ON=off; ORIG=orig2d; FIX=port-win; else SUF=p3d; ON=on; ORIG=orig3d; FIX=port-3d; fi
bash $P/waitroom.sh || exit 3
python -I $P/visrun3d.py "$N" --3d $ON > "build/ts-p2/$N/vis.out" 2>&1
echo "visrun exit=$? $(cat "build/ts-p2/$N/vis${MODE}.log" | tr '\n' ';' | cut -c1-300)"
python -I $P/mkport3d.py "$N" $([ "$MODE" = 2d ] && echo 2d) > /dev/null
USERS=$FIX bash $P/runport.sh "${N}_$SUF" 1 -drawevery | tail -1
python tools/shotdiff/shotdiff.py --game typershark --orig "build/ts-p2/$N/$ORIG" --port "build/ts-p2/${N}_$SUF/port" --run "${N}_$MODE" | tail -1
echo done
