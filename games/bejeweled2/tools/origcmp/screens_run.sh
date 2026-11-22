#!/bin/bash
# The screens run (SCREENS.md, docs/SHOTDIFF.md "Every screen, listed"): every tour the list names, the port's recording and the
# original's shots of the same inputs, 2D (640 by 480, windowed, the original in Windows Sandbox), merged into one pair of folders
# (each shot named <tour>__<shot>.png) and compared by one shotdiff run.
#   screens_run.sh [tour ...]      (default: all; a tour that already has its original's shots is not run again)
# The player folders (make_gapprofile.py): build/logs/bej2/profile (make_profile.py's player), prof_hints (every one-time box on),
# prof_p0 (the first puzzle of the galaxy), prof_p75b (the last planet, Cognito not yet unlocked).
cd "$(dirname "$0")/../../../.." || exit 2
OUT=build/screens/2d
mkdir -p "$OUT/port" "$OUT/orig"
B=build/logs/bej2
# name | record.py mode | moves | player folder | environment
SPECS=(
"tour|tour|6||"
"dialogs|tour:dialogs|1|$B/prof_p0|"
"classic|tour:classic|60|$B/prof_hints|TOUR_HINTBOXES=1"
"action|tour:action|60|$B/prof_hints|TOUR_HINTBOXES=1"
"hyper|tour:hyper|80|$B/prof_hints|TOUR_HINTBOXES=1"
"twilight|tour:twilight|60|$B/prof_hints|TOUR_HINTBOXES=1"
"endless|tour:endless|140|$B/prof_hints|TOUR_HINTBOXES=1"
"finity|tour:finity|12|$B/profile|"
"cognito|tour:cognito|12|$B/profile|TOUR_DOME=1"
"puzzle|tour:puzzle|1|$B/prof_p0|TOUR_DOME=1 TOUR_HINTS=1 TOUR_GALAXY=1 TOUR_PLANETS=445,157 TOUR_WORLDS=1 TOUR_WARP_STEP=100 TOUR_WARP_SHOTS=8"
"finale|tour:puzzle|1|$B/prof_p75b|TOUR_DOME=1 TOUR_WORLDS=1 TOUR_FINALE=1 TOUR_WARP_STEP=100 TOUR_WARP_SHOTS=8"
)
want=" $* "
for spec in "${SPECS[@]}"; do
  IFS='|' read -r name mode moves prof envs <<< "$spec"
  [ "$want" != "  " ] && [[ "$want" != *" $name "* ]] && continue
  rec=$OUT/rec-$name
  if [ ! -f "$rec/events.json" ]; then
    rm -rf "$rec"
    env LOWRES=1 DRAWEVERY=1 PLAY_AT=500 ${prof:+PROFILE=$prof} $envs bash games/bejeweled2/tools/origcmp/record_port.sh "$mode" "$moves" "$rec" 909 > "$OUT/rec-$name.log" 2>&1
    [ -f "$rec/events.json" ] || { echo "$name: the port's recording failed (see $OUT/rec-$name.log)"; continue; }
  fi
  og=$OUT/og-$name
  if [ ! -d "$og" ] || [ -z "$(ls $og/*.png 2>/dev/null)" ]; then
    for try in $(seq 1 24); do
      PROFILE=$prof bash games/bejeweled2/tools/origcmp/run_tour_original.sh "$rec" "$og" 909 1200 > "$OUT/og-$name.log" 2>&1
      [ -n "$(ls $og/*.png 2>/dev/null)" ] && break
      sleep 30
    done
  fi
  echo "$name: $(ls $rec/*.png | wc -l) port shots, $(ls $og/*.png 2>/dev/null | wc -l) original shots"
done
# one folder each, a shot named <tour>__<shot>.png
rm -rf "$OUT/port" "$OUT/orig"; mkdir -p "$OUT/port" "$OUT/orig"
for rec in $OUT/rec-*/; do
  name=$(basename "$rec"); name=${name#rec-}
  for f in "$rec"/*.png; do [ -f "$OUT/og-$name/$(basename $f)" ] && { cp "$f" "$OUT/port/${name}__$(basename $f)"; cp "$OUT/og-$name/$(basename $f)" "$OUT/orig/${name}__$(basename $f)"; }; done
done
python tools/shotdiff/shotdiff.py --game bejeweled2 --orig "$OUT/orig" --port "$OUT/port" --run "${RUN:-screens2d}" | tail -1
