#!/bin/bash
# run_all.sh <out dir>: the performance runs of README.md "Performance", back to back, each waiting for a free game slot
# (tools/gamecount.py --limit 6): the original and the port on tour 2's minute twice (original A, port A, original B,
# port B), then the port's load five times and the original's three times. One JSON line a run on stdout.
# WH_PERF_EXE=<name> runs a copy of the port in build/native instead of wordharmony.exe, so that a rebuild meanwhile
# leaves the measured exe alone.
cd "$(git rev-parse --show-toplevel)" || exit 1
O=${1:-build/perf}; mkdir -p "$O"
PY=$(cygpath -w "$(command -v python)")
slot() { for w in $(seq 1 480); do python tools/gamecount.py --limit 6 > /dev/null && return; sleep 15; done; }
for k in A B; do
	slot; echo "orig $k: $(python games/wordharmony/tools/profile/perf.py orig "$O/orig$k" build/ph2/tour2/orig.txt 1 | tail -1)"
	slot; echo "port $k: $(build/native/rundesk.exe "$PY" games/wordharmony/tools/profile/perf.py port-minute "$O/port$k" build/ph2/tour2/port.txt | tail -1)"
done
slot; build/native/rundesk.exe "$PY" games/wordharmony/tools/profile/perf.py port-load "$O/load" 5
for k in 1 2 3; do slot; echo "orig load $k: $(python games/wordharmony/tools/profile/perf.py orig "$O/oload$k" build/ph2/tour2/orig.txt 1 load | tail -1)"; done
