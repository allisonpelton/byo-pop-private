#!/bin/bash
# The port's 3D tours (1024 by 768, Direct3D 11 through SDL, under rundesk): one recording of each game mode's screens, as the 2D tours
# (record.py --mode tour:<mode>) have them, for run_tour_visible.sh to give the original on the visible desktop and for the web build's
# page to replay (tools/web/tour3d.js).
#   [MODES="classic action ..."] [MOVES=40] [OUT=build/visrun/rec3d] record_3d_tours.sh
# Writes $OUT/<mode>/{events.json,game.log,shots/} and $OUT/<mode>.record.txt. The secret modes start from the player of make_profile.py
# (build/logs/bej2/profile, made if missing) with every mode unlocked. The title's play click is at update 940, where the original's 3D title has loaded.
MODES=${MODES:-classic action puzzle endless twilight hyper cognito finity}
MOVES=${MOVES:-40}
cd "$(dirname "$0")/../../../.." || exit 2
OUT=${OUT:-build/visrun/rec3d}
PROFILE=build/logs/bej2/profile
[ -d $PROFILE/users ] || python games/bejeweled2/tools/origcmp/make_profile.py --exe build/native/bejeweled2.exe --out $PROFILE > /dev/null
mkdir -p $OUT
for m in $MODES; do
  rm -rf $OUT/$m $OUT/$m.user
  PLAY_AT=940 DRAWEVERY=1 PROFILE=$PROFILE bash games/bejeweled2/tools/origcmp/record_port.sh tour:$m $MOVES $OUT/$m 909 > $OUT/$m.record.txt 2>&1 \
    && echo "$m: $(tail -n 1 $OUT/$m.record.txt | cut -c1-200)" || echo "$m: recording failed (see $OUT/$m.record.txt)"
done
