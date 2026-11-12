#!/bin/bash
# splitpairs.sh <out dir> [pairs]: does counting the cycle split change a side's processor time? Alternating pinned runs of the
# like-for-like scenario (cycles.py), counting off (p<n>-off.json) and on (p<n>-on.json, PERF_SPLIT=1: the port's -cyclesplit and a
# cyclesplit line at each marker, or the original's origrun --count and a count line at each), one game process at a time, each
# waiting for a free slot. SIDE=port (the default) or SIDE=orig. From the worktree root. The original needs its stage (see pairs2.sh).
# `python games/bookworm/tools/profile/cycles.py compare <out dir>` prints the medians of each side; the "on" runs' snapshots are
# <out dir>/p<n>-on/split.jsonl, for tools/cyclesplit.py.
OUT=$1; N=${2:-6}; SIDE=${SIDE:-port}
PY="C:\\Python312\\python.exe"
CYC=games/bookworm/tools/profile/cycles.py
mkdir -p "$OUT"
[ -f "$OUT/scenario.txt" ] || cp games/bookworm/tools/profile/scenario.txt "$OUT/scenario.txt"
slot() { until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done; }
run() {   # <name>: one pinned run of the side, its result line in <out dir>/<name>.json
	if [ "$SIDE" = port ]; then
		python -I tools/pincore.py -- build/native/rundesk.exe "$PY" $CYC port "$OUT/$1" "$OUT/scenario.txt" 2> /dev/null | grep '^{' > "$OUT/$1.json"
	else
		python -I tools/pincore.py -- "$PY" $CYC orig "$OUT/$1" "$OUT/scenario.txt" 2> /dev/null | grep '^{' > "$OUT/$1.json"
	fi
}
for i in $(seq 1 "$N"); do
	if [ $((i % 2)) = 1 ]; then first=off; second=on; else first=on; second=off; fi
	for k in $first $second; do
		slot
		if [ $k = on ]; then export PERF_SPLIT=1; else unset PERF_SPLIT; fi
		run "p$i-$k"
	done
	echo "pair $i done"
done
echo finished
