#!/bin/bash
# One comparison: the port plays a mode by its bot with real input (record.py), the same inputs go to the original in Windows
# Sandbox at the same updates (to_origrun.py, run_original.sh), and the four watches are compared (compare.py).
#   run_mode.sh <mode: classic|survival> <moves> [seed] [outdir]
# Writes <outdir>/ (default build/logs/ap/cmp/<mode>-<moves>-s<seed>): events.json, script.txt, game.log (the port's), orig.log
# (the original's) and result.txt (compare.py's output). Exits 0 when the two sides are identical.
mode=$1; moves=$2; seed=${3:-1}
cd "$(dirname "$0")/../../../.." || exit 2
d=${4:-build/logs/ap/cmp/$mode-$moves-s$seed}
T=games/astropop/tools/origcmp
python $T/record.py --out "$d" --mode "$mode" --moves "$moves" --seed "$seed" --max-updates "${MAX_UPDATES:-60000}" > "$d.record.txt" 2>&1 || { cat "$d.record.txt"; exit 2; }
python $T/to_origrun.py "$d/events.json" "$d/script.txt" > /dev/null || exit 2
$T/run_original.sh "$d/script.txt" "$d/orig.log" "$seed" 900 || exit 2
last=$(python -c "import json,sys; print(json.load(open(sys.argv[1]))['last'])" "$d/events.json")
python $T/compare.py "$d/orig.log" "$d/game.log" --last "$last" > "$d/result.txt" 2>&1
st=$?
echo "$mode $moves moves seed $seed: $(tail -1 $d.record.txt)"; cat "$d/result.txt"
exit $st
