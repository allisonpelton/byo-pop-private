#!/usr/bin/env bash
# Run a list of tours one stream at a time: ports (all the port sides) or origs (all the original sides, in the sandbox), no diffs.
#   games/typershark/tools/phase2/queue.sh ports|origs <name>[:seed[:timeout]] ...
# The port side runs from the windowed, software fixture (USERS, default port-win) so that its Options dialog matches a sandbox run's.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
MODE="$1"; shift
for spec in "$@"; do
	IFS=: read -r N SEED LIM <<< "$spec"
	SEED="${SEED:-1}"; LIM="${LIM:-300}"
	echo "=== $MODE $N seed $SEED $(date +%H:%M:%S)"
	if [ "$MODE" = ports ]; then
		USERS="${USERS:-port-win}" bash games/typershark/tools/phase2/runport.sh "$N" "$SEED" -drawevery | tail -1
	else
		bash games/typershark/tools/phase2/runorig.sh "$N" "$SEED" "$LIM" | tail -4
	fi
done
echo "=== $MODE done $(date +%H:%M:%S)"
