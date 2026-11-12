#!/bin/bash
# paced.sh <out dir> [pairs]: the performance target's measurement (docs/COORDINATION.md, "The target"; GAME_NOTES.md, "Performance"):
# the original paced as on a player's 60 Hz screen and the port, turn about, over the same stretch of play in real time.
# Each pair is two runs: the original with PERF_PACED=16 (a pass of its loop waits 16 ms where it waits for the screen's vertical
# blank, which the hidden desktop never gives) counting its updates by the app's +0x3ec, and the port; UNPACED=1 adds a third, the
# original as it is. The processor time of a run is the cycle counts of its threads (QueryThreadCycleTime, summed: perf.py), read
# every 0.1 s. Run the whole loop on one core of its own, so both sides get the same core:
#
#   python -I <main checkout>/tools/pincore.py -- bash games/atomica/tools/profile/paced.sh build/perfruns/paced 5
#
# Waits first, up to WAIT_S seconds (default 3600), until the machine has room for a game process (tools/gamecount.py --limit 3).
# Each run's JSON line goes to <out dir>/runs.txt, after its kind; run it in the background: it ends by itself.
cd "$(dirname "$0")/../../../.." || exit 2
OUT="$1"; WANT="${2:-5}"; WAIT_S="${WAIT_S:-3600}"
P=games/atomica/tools/profile/perf.py; SC=games/atomica/tools/identical/s2.txt
PY="$(cygpath -w "$(command -v python)")"
mkdir -p "$OUT"; : > "$OUT/runs.txt"
start=$(date +%s)
until python -I tools/gamecount.py --limit 3 > /dev/null 2>&1; do
	[ $(( $(date +%s) - start )) -ge "$WAIT_S" ] && break
	sleep 15
done
echo "start $(date '+%F %T'), waited $(( $(date +%s) - start )) s" >> "$OUT/runs.txt"
one() {  # <kind> <command...>: a run's last JSON line with its kind
	local kind="$1"; shift
	echo "$kind $("$@" 2>> "$OUT/errors.txt" | grep others_start | tail -1)" >> "$OUT/runs.txt"
}
for i in $(seq 1 "$WANT"); do
	PERF_CLOCK=real PERF_PACED=16 PERF_COUNTER=0x4d004c PERF_COUNTER_SHIFT=1 one "orig-paced" python -I $P orig "$OUT/orig-paced$i" $SC 1
	PERF_CLOCK=real one "port-real" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/port-real$i" $SC
	[ -n "${UNPACED:-}" ] && PERF_CLOCK=real one "orig-unpaced" python -I $P orig "$OUT/orig-unpaced$i" $SC 1
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
