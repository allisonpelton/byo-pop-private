#!/bin/bash
# Phase one for one mode: the port plays it with real input (record.py), the original gets the same inputs on origrun's virtual clock
# (to_origrun.py, run_original.sh), and compare.py lines up their watches. One line comes out: IDENTICAL or DIFFERENT, with the logs
# in build/logs/bej2/<tag>/.
#   run_mode.sh <mode> <moves> [seed] [tag]
mode=$1; moves=$2; seed=${3:-1}; tag=${4:-$mode}
cd "$(dirname "$0")/../../../.." || exit 2
T=games/bejeweled2/tools/origcmp
R=build/logs/bej2/$tag
mkdir -p build/logs/bej2/og_$tag
PLAY_AT=${PLAY_AT:-500} bash $T/record_port.sh "$mode" "$moves" "$R" "$seed" > "$R.record.txt" 2>&1 || { echo "$tag: the recording failed (see $R.record.txt)"; exit 1; }
python $T/to_origrun.py "$R/events.json" "build/origrun/bej2_$tag.txt" > /dev/null
bash $T/run_original.sh "build/origrun/bej2_$tag.txt" "build/logs/bej2/og_$tag/origrun.log" "$seed" ${TIMEOUT:-900}
last=$(python -c "import json;print(json.load(open('$R/events.json'))['last'])")
python $T/compare.py "build/logs/bej2/og_$tag/origrun.log" "$R/game.log" --last "$last" > "$R.cmp.txt"
rc=$?
echo "$tag: $(tail -1 $R.cmp.txt) ($(head -1 $R.record.txt | cut -c1-110))"
exit $rc
