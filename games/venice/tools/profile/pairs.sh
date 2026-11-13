#!/bin/bash
# pairs.sh <out dir> [rounds]: Venice's processor time like for like (GAME_NOTES.md, "Performance"): the original and the port in turn over
# the same scripted stretch (scenario.py: Journey from building 10, 4000 updates in play) in real time, [rounds] rounds (default 5) of
# the original, the port in a window of the game's own size (-windowscale=1, the original's: the "p<i>1" runs) and the port in its default
# window (2x, "p<i>0"; the two in the other order every other round), each run started when the machine has room for one more game process
# (tools/gamecount.py --limit 6). `profile.py prep <out dir>` makes the input first. Run the whole of it on a performance core of its own,
# in the background:
#   python -I tools/pincore.py -- bash games/venice/tools/profile/pairs.sh build/perf 5
# then `python -I games/venice/tools/profile/profile.py sum build/perf` prints the medians.
cd "$(dirname "$0")/../../../.." || exit 2
OUT="$1"; N="${2:-5}"
P=games/venice/tools/profile/profile.py
PY="$(cygpath -w "$(command -v python)")"
room() { until python -I tools/gamecount.py --limit 6 > /dev/null 2>&1; do sleep 20; done; }
echo "start $(date '+%F %T')" >> "$OUT/runs.txt"
for i in $(seq 1 "$N"); do
	room
	python -I $P orig "$OUT" "$OUT/o$i" 2>> "$OUT/errors.txt"
	echo "orig $i done $(date '+%T')" >> "$OUT/runs.txt"
	for v in $([ $((i % 2)) = 1 ] && echo "1 0" || echo "0 1"); do
		room
		CYC_WINDOWSCALE=$v build/native/rundesk.exe "$PY" -I $P port "$OUT" "$OUT/p$i$v" 2>> "$OUT/errors.txt"
		echo "port scale $v $i done $(date '+%T')" >> "$OUT/runs.txt"
	done
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
