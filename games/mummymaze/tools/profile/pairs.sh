#!/bin/bash
# pairs.sh <out dir> [pairs]: processor time like for like (GAME_NOTES.md, "Processor time like for like"): the original and
# the port in turn over the same scripted stretch (phase two's scenario c6, 6000 updates of a classic game) in real time,
# [pairs] pairs (default 5), each run started when the machine has room for one more game process
# (tools/gamecount.py --limit 6). Run the whole of it on a performance core of its own, in the background:
#   python -I tools/pincore.py -- bash games/mummymaze/tools/profile/pairs.sh build/cyc 5
# Each run's JSON line (cycles.py) goes to <out dir>/runs.txt; `python -I games/mummymaze/tools/profile/cycles.py summary
# <out dir>/runs.txt` prints the medians. The original starts first in every pair, a pair after a pair, so a slow
# stretch of the machine falls on both sides.
cd "$(dirname "$0")/../../../.." || exit 2
OUT="$1"; N="${2:-5}"
C=games/mummymaze/tools/profile/cycles.py
PY="$(cygpath -w "$(command -v python)")"
mkdir -p "$OUT"
python -I games/mummymaze/tools/identical/p2.py make games/mummymaze/tools/identical/scenarios/c6.txt c6 "$OUT" || exit 2
room() { until python -I tools/gamecount.py --limit 6 > /dev/null 2>&1; do sleep 20; done; }
echo "start $(date '+%F %T')" >> "$OUT/runs.txt"
for i in $(seq 1 "$N"); do
	room
	echo "orig $(python -I $C orig "$OUT/o$i" "$OUT/c6.orig.txt" 21 2>> "$OUT/errors.txt" | tail -1)" >> "$OUT/runs.txt"
	room
	echo "port $(build/native/rundesk.exe "$PY" -I $C port "$OUT/p$i" "$OUT/c6.port.txt" 21 2>> "$OUT/errors.txt" | tail -1)" >> "$OUT/runs.txt"
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
