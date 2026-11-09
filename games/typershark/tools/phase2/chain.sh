#!/usr/bin/env bash
# Run tours one after another: the port, the original in the sandbox, then shotdiff and a summary. Each argument is
#   <name>[:seed[:timeout[:port args with + for spaces]]]    (the tour's scripts are build/ts-p2/<name>/{orig,port}.txt, made before)
# e.g. games/typershark/tools/phase2/chain.sh m1 g_rt1:1:900
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
for spec in "$@"; do
	IFS=: read -r N SEED LIM PARGS <<< "$spec"
	SEED="${SEED:-1}"; LIM="${LIM:-300}"
	echo "=== $N seed $SEED timeout $LIM"
	[ -s "build/ts-p2/$N/port.txt" ] || { echo "no port script for $N"; continue; }
	if [ -z "${SKIP_PORT:-}" ]; then bash games/typershark/tools/phase2/runport.sh "$N" "$SEED" -drawevery ${PARGS//+/ } | tail -1; fi
	if [ -z "${SKIP_ORIG:-}" ]; then bash games/typershark/tools/phase2/runorig.sh "$N" "$SEED" "$LIM"; fi
	python tools/shotdiff/shotdiff.py --game typershark --orig "build/ts-p2/$N/orig" --port "build/ts-p2/$N/port" --run "p2-$N" | tail -1
	python games/typershark/tools/phase2/sumshot.py "p2-$N" | head -60
done
