#!/bin/bash
# The 3D tours of every mode against the original, for the visible desktop's night (22:00 to 08:00 local, docs/RULES.md): each recording
# in <dir> (record_3d_tours.sh's, <dir>/<mode>/events.json) is played by the original on the visible desktop (run_tour_visible.sh,
# visrun.py: virtual clock, silent, posted input) and by the port natively, and shotdiff compares them. One mode at a time.
#
#   [MODES="puzzle classic ..."] [DIR=build/visrun/rec3d_night] [TAGPREFIX=n3d] [STOP_HOUR=7] tour3d_night.sh
#
# Waits for 22:00 when started earlier. Starts no run from STOP_HOUR (07:00) on, so that every run ends by 08:00. One result line a mode
# in $DIR/night.txt: the tag, the number of shots and shotdiff's last line.
cd "$(dirname "$0")/../../../.." || exit 2
MODES=${MODES:-puzzle classic action endless hyper twilight cognito finity}
DIR=${DIR:-build/visrun/rec3d_night}
TAGPREFIX=${TAGPREFIX:-n3d}
STOP_HOUR=${STOP_HOUR:-7}
RES=$DIR/night.txt

allowed() {  # the hour is 22 to 23, or 00 to STOP_HOUR - 1: a run started then has an hour left
  local h
  h=$(date +%H); h=$((10#$h))
  [ $h -ge 22 ] && return 0
  [ $h -lt $STOP_HOUR ] && return 0
  return 1
}

while ! allowed; do sleep 60; done
for m in $MODES; do
  allowed || { echo "$m: not started, the night is over" >> $RES; continue; }
  [ -f $DIR/$m/events.json ] || { echo "$m: no recording" >> $RES; continue; }
  bash games/bejeweled2/tools/origcmp/run_tour_visible.sh $DIR/$m/events.json ${TAGPREFIX}_$m > $DIR/$m.night.log 2>&1
  echo "$m: $(grep -a "shots$" $DIR/$m.night.log | tail -n 1); $(tail -n 1 $DIR/$m.night.log | cut -c1-300)" >> $RES
done
echo "done $(date '+%F %T')" >> $RES
