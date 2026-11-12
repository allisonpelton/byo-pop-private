#!/bin/bash
# pairs.sh <out dir> [pairs]: processor time like for like (GAME_NOTES.md, "Processor time like for like"): the original and
# the port in turn over the same scripted stretch (the board's updates 600 to 6100, cycles.py) in real time, [pairs]
# rounds (default 5: the original, then the port at 1x and at 2x), each run started when the machine has room for one more game process (tools/gamecount.py --limit 6).
# Run the whole of it on a performance core of its own, in the background:
#   python -I tools/pincore.py -- bash games/sevenseas/tools/profile/pairs.sh build/cyc 5
# Each run's JSON line (cycles.py) goes to <out dir>/runs.txt; `python -I games/sevenseas/tools/profile/cycles.py summary
# <out dir>/runs.txt` prints the medians. The original starts first in every pair.
cd "$(dirname "$0")/../../../.." || exit 2
OUT="$1"; N="${2:-5}"
D=games/sevenseas/tools/profile
C=$D/cycles.py
PY="$(cygpath -w "$(command -v python)")"
mkdir -p "$OUT"
room() { until python -I tools/gamecount.py --limit 6 > /dev/null 2>&1; do sleep 20; done; }
echo "start $(date '+%F %T')" >> "$OUT/runs.txt"
for i in $(seq 1 "$N"); do
	room
	echo "orig $(python -I $C orig "$OUT/o$i" $D/perf_orig.txt 7 2>> "$OUT/errors.txt" | tail -1)" >> "$OUT/runs.txt"
	# the port's two windows (cycles.py CYC_WINDOW): the original's size (1x) and the default window (2x), in alternating order
	if [ $((i % 2)) = 1 ]; then W="1x 2x"; else W="2x 1x"; fi
	for w in $W; do
		room
		echo "port $(CYC_WINDOW=$w build/native/rundesk.exe "$PY" -I $C port "$OUT/p$w$i" $D/perf_port.txt 7 2>> "$OUT/errors.txt" | tail -1)" >> "$OUT/runs.txt"
	done
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
