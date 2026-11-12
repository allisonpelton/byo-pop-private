#!/bin/bash
# pairs2.sh <out dir> [pairs]: BookWorm's processor time like for like (cycles.py), the port and the original turn about, one game
# process at a time, each pair on a performance core of its own (tools/pincore.py), each run waiting for a free game slot first.
# From the worktree root. The scenario is <out dir>/scenario.txt (a copy of games/bookworm/tools/profile/scenario.txt); the results
# are <out dir>/p<n>-port.json and p<n>-orig.json, summarised by `python games/bookworm/tools/profile/cycles.py summary <out dir>`.
# The original needs its stage (OG_STAGE, default C:/byo-og/bwperf: `OG_STAGE=... games/bookworm/tools/identical/setup.sh`) and
# build/origrun-bin/origrun.exe built after the trunk's last merge.
OUT=$1; N=${2:-5}
PY="C:\Python312\python.exe"
CYC=games/bookworm/tools/profile/cycles.py
mkdir -p "$OUT"
[ -f "$OUT/scenario.txt" ] || cp games/bookworm/tools/profile/scenario.txt "$OUT/scenario.txt"
slot() { until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done; }
for i in $(seq 1 "$N"); do
	slot; python -I tools/pincore.py -- build/native/rundesk.exe "$PY" $CYC port "$OUT/p$i-port" "$OUT/scenario.txt" 2> /dev/null | grep '^{' > "$OUT/p$i-port.json"
	slot; python -I tools/pincore.py -- "$PY" $CYC orig "$OUT/p$i-orig" "$OUT/scenario.txt" 2> /dev/null | grep '^{' > "$OUT/p$i-orig.json"
	echo "pair $i done"
done
echo finished
