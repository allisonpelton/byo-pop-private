#!/usr/bin/env bash
# The medal screens in 3D (GAME_NOTES section 18, "Not compared"): the original on the user's visible desktop through visrun3d.py (only between
# 22:00 and 08:00 local time, CLAUDE.md: the overnight rule; a run started in that window ends by 08:00), the port with Is3D on at the updates the
# original's steps landed at, shotdiff. The saved adventure of mksaved.sh and the profiles of mkprofile.py must exist (medal.sh makes them).
#   games/typershark/tools/phase2/medal3d.sh dialog <0|2|4> | dialogkeep <2> | stats <1|3|5|12>
set -uo pipefail
ROOT="$(cd "$(dirname "$0")/../../../.." && pwd)"
cd "$ROOT"
KIND="$1"; N="$2"; TOUR="m3d_${KIND}_$N"
H="$(date +%H)"
if [ "${3:-}" != "--any-time" ] && [ "$H" -ge 8 ] && [ "$H" -lt 22 ]; then
	echo "medal3d: it is $(date +%H:%M); the desktop route is allowed from 22:00 to 08:00 (--any-time is for the user's own say-so)"; exit 4
fi
[ -s build/ts-p2/mk1/ud/users/user0.dat ] || bash games/typershark/tools/phase2/mksaved.sh || exit 1
case "$KIND" in
	dialog|dialogkeep) TAG="sv$N" ;;
	stats) TAG="ngc$N" ;;
	*) echo "kind: dialog, dialogkeep or stats"; exit 1 ;;
esac
[ -s "build/ts-p1/fixtures/medal-ngc1/registry.ini" ] || python games/typershark/tools/phase2/mkprofile.py 1 > /dev/null
python games/typershark/tools/phase2/mkprofile.py "$N" $([ "$TAG" = "sv$N" ] && echo saved) > /dev/null
python games/typershark/tools/phase2/mkprofile.py "$N" $([ "$TAG" = "sv$N" ] && echo saved) 3d > /dev/null
python games/typershark/tools/phase2/tour_medal.py "$KIND" "$TOUR"
FIXTURE="medal-$TAG-3d" VIS_USERS="build/ts-p2/profiles/$TAG/orig.users" VIS_REG="build/ts-p2/profiles/$TAG/orig.reg" \
	VIS_PATCH="va:0x454e9b 83f807 83f801" bash games/typershark/tools/phase2/vis3d.sh "$TOUR"
python games/typershark/tools/phase2/sumshot.py "${TOUR}_3d" | head -30
