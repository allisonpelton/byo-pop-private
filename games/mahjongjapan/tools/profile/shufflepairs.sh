#!/bin/bash
# shufflepairs.sh <game: japan|china> <level> <played percent> <shuffles> <pairs> <out dir>: the lite mod's measurement in the game
# (games/<game>/docs/LITE_MOD.md): in turn one run of the original and one with -lite=quickRedeal, each playing the stage down with the bot
# and clicking Shuffle <shuffles> times in a row (tools/realplay/shuffletest.py), each shuffle's processor cycles counted from the click
# to the board at rest. Prints one line a run: the total, the longest shuffle and how many shuffles cost over 1.2 G (a search that ran
# out its second costs about 2 G here, one that found a deal at once about 0.4 G). Run it under tools/pincore.py and rundesk:
#   python -I tools/pincore.py -- bash games/mahjongjapan/tools/profile/shufflepairs.sh japan 23 25 30 3 build/perf/shuf
cd "$(git rev-parse --show-toplevel)" || exit 1
G=$1; L=$2; P=$3; N=$4; K=$5; OUT=$6
mkdir -p "$OUT"
for i in $(seq 1 "$K"); do
  for m in orig lite; do
    x=""; [ $m = lite ] && x="--lite"
    D=$OUT/$G-$L-$m$i
    build/native/rundesk.exe python -I games/mahjongjapan/tools/realplay/shuffletest.py "$D" --game "$G" --level "$L" --played "$P" --shuffles "$N" $x > "$D.out" 2>&1
    python -I - "$D.out" "$m$i" <<'PY'
import json, sys
run = None
for line in open(sys.argv[1], errors='replace'):
    if line.startswith('SHUFFLE '):
        run = json.loads(line[8:])
if run is None:
    print(sys.argv[2], 'no result')
else:
    cycles = [s['cycles_G'] for s in run['shuffles']]
    print(sys.argv[2], 'option read' if run['log_option'] else 'option off', 'shuffles', len(cycles), 'total %.2f G' % sum(cycles),
          'longest %.2f G' % max(cycles), 'over 1.2 G:', sum(1 for c in cycles if c > 1.2))
PY
  done
done
