#!/bin/bash
# pair.sh <out dir> [rounds]: the original and the port play tour 2's minute at the same time (perf.py orig and port-minute, two game
# processes), so that both meet the same load from the machine's other sessions; run_all.sh runs them back to back, and processor
# time moves with the machine's load (the same port took 6.5 and 11.1 s on two runs with the machine 69 and 83% busy). Each round waits
# for room for both (tools/gamecount.py --limit 5: at most 4 game processes of others) and prints one JSON line a side.
# WH_PERF_EXE=<name> measures a copy of the port in build/native. Needs build/ph2/tour2/{port,orig}.txt (make.py of tools/shots).
cd "$(git rev-parse --show-toplevel)" || exit 1
O=${1:-build/perf/pair}; N=${2:-2}; mkdir -p "$O"
PY=$(cygpath -w "$(command -v python)")
for k in $(seq 1 "$N"); do
	for w in $(seq 1 480); do python tools/gamecount.py --limit 5 > /dev/null && break; sleep 15; done
	python games/wordharmony/tools/profile/perf.py orig "$O/orig$k" build/ph2/tour2/orig.txt 1 > "$O/orig$k.json" 2> "$O/orig$k.err" &
	build/native/rundesk.exe "$PY" games/wordharmony/tools/profile/perf.py port-minute "$O/port$k" build/ph2/tour2/port.txt > "$O/port$k.json" 2> "$O/port$k.err" &
	wait
	echo "orig $k: $(tail -1 "$O/orig$k.json")"
	echo "port $k: $(tail -1 "$O/port$k.json")"
done
