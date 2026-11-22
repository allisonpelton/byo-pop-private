#!/bin/bash
# The 2D tours of screens_run.sh with the original on the host, its shots read by origrun's in-game frame reader (run_tour_reader.sh):
# the port's recordings (build/screens/2d/rec-<name>) are the ones screens_run.sh made. One folder per tour, rd-<name>, then the pairs are
# merged as <tour>__<shot>.png into build/screens/2d/rdorig and port and shotdiff runs as ${RUN:-screens2d_reader}.
#   screens_reader.sh [tour...]      READER_TRIAL=1 until origrun has no trial (docs/ORIGRUN.md)
cd "$(dirname "$0")/../../../.." || exit 2
OUT=build/screens/2d
B=build/logs/bej2
declare -A PROF=([dialogs]=prof_p0 [classic]=prof_hints [action]=prof_hints [hyper]=prof_hints [twilight]=prof_hints [endless]=prof_hints [finity]=profile [cognito]=profile [puzzle]=prof_p0 [finale]=prof_p75b)
want=" $* "
for name in tour dialogs classic action hyper twilight endless finity cognito puzzle finale; do
  [ "$want" != "  " ] && [[ "$want" != *" $name "* ]] && continue
  prof=${PROF[$name]}
  PROFILE=${prof:+$B/$prof} bash games/bejeweled2/tools/origcmp/run_tour_reader.sh "$OUT/rec-$name" "$OUT/rd-$name" 909 1200 > "$OUT/rd-$name.log" 2>&1
  echo "$name: $(ls $OUT/rec-$name/*.png | wc -l) port shots, $(ls $OUT/rd-$name/*.png 2>/dev/null | wc -l) original shots, $(grep -c 'refused \[reader' $B/og_rec-$name/origrun.log) refused"
done
rm -rf "$OUT/rdport" "$OUT/rdorig"; mkdir -p "$OUT/rdport" "$OUT/rdorig"
for rec in $OUT/rec-*/; do
  name=$(basename "$rec"); name=${name#rec-}
  for f in "$rec"/*.png; do [ -f "$OUT/rd-$name/$(basename $f)" ] && { cp "$f" "$OUT/rdport/${name}__$(basename $f)"; cp "$OUT/rd-$name/$(basename $f)" "$OUT/rdorig/${name}__$(basename $f)"; }; done
done
python tools/shotdiff/shotdiff.py --game bejeweled2 --orig "$OUT/rdorig" --port "$OUT/rdport" --run "${RUN:-screens2d_reader}" | tail -1
