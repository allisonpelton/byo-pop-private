#!/bin/bash
# pairs.sh <out dir> <strategy|menu> [pairs] [-- extra port args]: TipTop's processor time like for like (docs/COORDINATION.md, "The target"):
# the port and the paced original turn about, one game process at a time, each run on a performance core of its own
# (tools/pincore.py) and waiting for a free game slot first, the window of both 1x (the original's size: -windowscale=1, a test
# option) with the music left out of both, the original paced at PACED ms a pass (default 13: Sleep's granularity gives it 62 to 63
# passes a second as measured with --split, as a 60 Hz screen's blank would; 16 gives 52, 14 gives 56, 12 gives 73). From the worktree root; build/tt/perf/tiptop-perf.txt must exist (patches.sh).
# Results: <out dir>/p<n>-port.json and p<n>-orig.json (perf.py's RESULT line); `python games/tiptop/tools/perf/pairsum.py <out dir>`.
PACED=${PACED:-13}
OUT=$1; KIND=$2; N=${3:-5}; shift $(( $# < 3 ? $# : 3 )); [ "$1" = "--" ] && shift
PY="C:\Python312\python.exe"
PERF=games/tiptop/tools/perf/perf.py
mkdir -p "$OUT"
OUT=$(cd "$OUT" && pwd -W)
if [ "$KIND" = menu ]; then ARGS="--menu --seconds 20 --window-from 8"; else ARGS="--mode strategy --every 2.4 --count 25 --seconds 30 --window-from 8"; fi
slot() { until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done; }
for i in $(seq 1 "$N"); do
	slot; python -I tools/pincore.py -- "$PY" $PERF portscript --label p$i-port --out "$OUT/p$i-port" $ARGS --port-arg=-soundfont=/nonexistent.sf2 --port-arg=-windowscale=1 "$@" 2> /dev/null | grep '^RESULT ' | sed 's/^RESULT //' > "$OUT/p$i-port.json"
	slot; python -I tools/pincore.py -- "$PY" $PERF orig --label p$i-orig --out "$OUT/p$i-orig" $ARGS --paced $PACED 2> /dev/null | grep '^RESULT ' | sed 's/^RESULT //' > "$OUT/p$i-orig.json"
	echo "pair $i done"
done
echo finished
