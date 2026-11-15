#!/bin/bash
# The 3D tours of every mode against the original on the visible desktop (any hour, CLAUDE.md): each recording
# in <dir> (record_3d_tours.sh's, <dir>/<mode>/events.json) is played by the original on the visible desktop (run_tour_visible.sh,
# visrun.py: virtual clock, silent, posted input) and by the port natively, and shotdiff compares them. One mode at a time.
#
#   [MODES="puzzle classic ..."] [DIR=build/visrun/rec3d_night] [TAGPREFIX=n3d] tour3d_night.sh
#
# One result line a mode
# in $DIR/night.txt: the tag, the number of shots and shotdiff's last line.
cd "$(dirname "$0")/../../../.." || exit 2
MODES=${MODES:-puzzle classic action endless hyper twilight cognito finity}
DIR=${DIR:-build/visrun/rec3d_night}
TAGPREFIX=${TAGPREFIX:-n3d}
RES=$DIR/night.txt

for m in $MODES; do
  [ -f $DIR/$m/events.json ] || { echo "$m: no recording" >> $RES; continue; }
  bash games/bejeweled2/tools/origcmp/run_tour_visible.sh $DIR/$m/events.json ${TAGPREFIX}_$m > $DIR/$m.night.log 2>&1
  echo "$m: $(grep -a "shots$" $DIR/$m.night.log | tail -n 1); $(tail -n 1 $DIR/$m.night.log | cut -c1-300)" >> $RES
done
echo "done $(date '+%F %T')" >> $RES
