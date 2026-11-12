#!/bin/bash
# pairs.sh <out dir> [pairs]: BookWorm's real-time measurement, the port and the original turn about, one process at a time
# (as games/astropop/tools/profile/pairs.sh): each pair is the port's play of the scenario, then the original's, both in real
# time (PERF_CLOCK=real), then a load of each. A run waits for a free game slot first. From the worktree root. The scenario
# is <out dir>/scenario.txt (a copy of games/bookworm/tools/profile/scenario.txt, the port script of a bot game); the results are <out dir>/p<n>-port.json,
# p<n>-orig.json, p<n>-portload.json and p<n>-origload.json, summarised by `python games/bookworm/tools/profile/summary.py <out dir>`.
# The original needs its stage (OG_STAGE, default C:/byo-og/bwperf: `OG_STAGE=... games/bookworm/tools/identical/setup.sh`).
OUT=$1; N=${2:-3}
ROOT=$(pwd)
PY="C:\\Python312\\python.exe"
PERF=games/bookworm/tools/profile/perf.py
mkdir -p "$OUT"
slot() { until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done; }
for i in $(seq 1 "$N"); do
	slot; PERF_CLOCK=real build/native/rundesk.exe "$PY" $PERF port-minute "$OUT/p$i-port" "$OUT/scenario.txt" 2> /dev/null | grep '^{' > "$OUT/p$i-port.json"
	slot; PERF_CLOCK=real "$PY" $PERF orig "$OUT/p$i-orig" "$OUT/scenario.txt" 1 2> /dev/null | grep '^{' > "$OUT/p$i-orig.json"
	slot; build/native/rundesk.exe "$PY" $PERF port-load "$OUT/p$i-portload" 1 2> /dev/null | grep '^{' > "$OUT/p$i-portload.json"
	slot; "$PY" $PERF orig "$OUT/p$i-origload" "$OUT/scenario.txt" 1 load 2> /dev/null | grep '^{' > "$OUT/p$i-origload.json"
done
echo finished > "$OUT/done.txt"
