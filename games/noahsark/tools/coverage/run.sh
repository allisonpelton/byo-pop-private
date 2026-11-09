#!/bin/bash
# run.sh <name> <origrun script> <seed> [origrun options]: one coverage run of the original (docs/COVERAGE.md) on the host: the
# original through origrun (tools/run_original.sh: the virtual clock, no sound device, the seed pinned by the patch list) with
# --coverage on the list `python -I tools/coverage.py list noahsark` writes. Options after the seed go to origrun, e.g.
# "--clock real --timeout 300" for the run that gives the sound code its clock. The hits go to
# build/coverage/noahsark/hits/<name>.txt, the log beside them. Waits for room (three game processes on the machine).
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
NAME="$1"; SCRIPT="$2"; SEED="$3"; shift 3
H=build/coverage/noahsark/hits; mkdir -p "$H"
COV="$(cygpath -w "$PWD/build/coverage/noahsark/functions.txt")"
rm -rf "build/origrun/cov_$NAME"
until python -I tools/gamecount.py --limit 3 > /dev/null; do sleep 20; done
bash games/noahsark/tools/run_original.sh "cov_$NAME" "$SCRIPT" "$SEED" --coverage "$COV" "$@"
cp "build/origrun/cov_$NAME/run.log" "$H/$NAME.log" 2>/dev/null
cp "build/origrun/cov_$NAME/coverage.txt" "$H/$NAME.txt" || { echo "$NAME: no hits file"; exit 1; }
echo "$NAME: $(grep -vc '^#' "$H/$NAME.txt") functions; $(grep '^#' "$H/$NAME.txt" | tail -1)"
