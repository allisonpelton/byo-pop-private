#!/usr/bin/env bash
# The port's tours run with two exes (OLD and NEW, default build/native/typershark_old.exe and typershark_new.exe), each tour's shots
# compared pixel for pixel (pngdiff.py): which shots does a change of the port change?
#   games/typershark/tools/phase2/stretchcheck.sh <tour>[:seed] ...      (a tour's script is build/ts-p2/<tour>/port.txt)
# Leaves build/ts-p2/<tour>/port_old and port_new; the summary goes to stdout.
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
R="$(cd "$ROOT" && pwd -W)"
cd "$ROOT"
OLD="${OLD:-$R/build/native/typershark_old.exe}"; NEW="${NEW:-$R/build/native/typershark_new.exe}"
for spec in "$@"; do
	IFS=: read -r N SEED <<< "$spec"; SEED="${SEED:-1}"
	D="build/ts-p2/$N"
	TSEXE="$OLD" bash games/typershark/tools/phase2/runport.sh "$N" "$SEED" -drawevery > /dev/null; rm -rf "$D/port_old"; mv "$D/port" "$D/port_old"
	TSEXE="$NEW" bash games/typershark/tools/phase2/runport.sh "$N" "$SEED" -drawevery > /dev/null; rm -rf "$D/port_new"; mv "$D/port" "$D/port_new"
	echo "=== $N"; python -I games/typershark/tools/phase2/pngdiff.py "$D/port_old" "$D/port_new"
done
