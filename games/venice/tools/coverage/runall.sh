#!/bin/bash
# runall.sh <name:scenario[:seed[:kind]]>...: the coverage runs one after another (run.sh), one line each; a third field is the game's
# seed for that run (VENICE_SEED, 1 when there is none), a fourth the kind of every power-up (POWERUP_KIND, run.sh)
cd "$(dirname "$0")/../../../.." || exit 2
for item in "$@"; do
	IFS=: read -r name scenario seed kind <<< "$item"
	VENICE_SEED="${seed:-1}" POWERUP_KIND="$kind" bash games/venice/tools/coverage/run.sh "$name" "$scenario"
done
echo all-finished
