#!/bin/bash
# toggleruns.sh <out dir> [runs]: what does counting the cycle split cost? Each run idles on the title screen (no input, so every
# block does the same work) and alternates counting on and paused in blocks of 200 updates, in the order on, paused, paused, on (odd
# runs; the reverse in even ones): the port's `cyclesplit pause|resume` or the original's `count pause|resume` (origrun --count)
# at each block's first update, the process's cycles read at the same updates (cycles.py, PERF_TOGGLE). A block of one run sits
# next to its opposite in time, so the machine's own noise (run to run it is a quarter of the cost of an update here) falls on both.
# SIDE=port (the default) or SIDE=orig. One game process at a time, each waiting for a free slot, pinned to a core (tools/pincore.py).
# From the worktree root. `python games/bookworm/tools/profile/cycles.py toggle <out dir>` prints the comparison.
OUT=$1; N=${2:-4}; SIDE=${SIDE:-port}
PY="C:\\Python312\\python.exe"
CYC=games/bookworm/tools/profile/cycles.py
MARKS=${PERF_MARKS:-800,1000,1200,1400,1600,1800,2000,2200,2400,2600,2800,3000,3200}
mkdir -p "$OUT"
echo "4000 move 0 0" > "$OUT/idle.txt"
slot() { until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done; }
export PERF_MARKS="$MARKS"
for i in $(seq 1 "$N"); do
	mode=$(( (i + 1) % 2 + 1 ))
	slot
	export PERF_TOGGLE=$mode
	if [ "$SIDE" = port ]; then
		python -I tools/pincore.py -- build/native/rundesk.exe "$PY" $CYC port "$OUT/t$i-m$mode" "$OUT/idle.txt" 2> /dev/null | grep '^{' > "$OUT/t$i-m$mode.json"
	else
		python -I tools/pincore.py -- "$PY" $CYC orig "$OUT/t$i-m$mode" "$OUT/idle.txt" 2> /dev/null | grep '^{' > "$OUT/t$i-m$mode.json"
	fi
	echo "run $i done"
done
echo finished
