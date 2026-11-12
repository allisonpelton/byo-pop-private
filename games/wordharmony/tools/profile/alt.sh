#!/bin/bash
# alt.sh <out dir> [rounds]: the original and the port play tour 2's minute (perf.py orig and port-minute) one after the other, alternating (the
# original first in odd rounds, the port first in even ones), all on one performance core that no other pincore run holds
# (run as `python -I tools/pincore.py -- bash games/wordharmony/tools/profile/alt.sh <out> 5`). The measure is the cycles of the process's
# threads over the minute (perf.py: cycles_in_minute_g, QueryThreadCycleTime; GetProcessTimes charges by the 15.6 ms tick and is not
# used), so the machine's other load moves it little; every run waits for room first (tools/gamecount.py --limit 6). Prints one
# JSON line a run. WH_PERF_PATCHES=<file> gives the original's patches (perf-lean: no screen copy), WH_PERF_EXE=<name> a copy of the port.
# Needs build/ph2/tour2/{port,orig}.txt (make.py of tools/shots).
cd "$(git rev-parse --show-toplevel)" || exit 1
O=${1:-build/perf/alt}; N=${2:-5}; mkdir -p "$O"
PY=$(cygpath -w "$(command -v python)")
waitslot() { for w in $(seq 1 480); do python tools/gamecount.py --limit 6 > /dev/null && return; sleep 15; done; }
run_orig() { waitslot; python games/wordharmony/tools/profile/perf.py orig "$O/orig$1" build/ph2/tour2/orig.txt 1 > "$O/orig$1.json" 2> "$O/orig$1.err"; echo "orig $1: $(tail -1 "$O/orig$1.json" | cut -c1-1500)"; }
run_port() { waitslot; build/native/rundesk.exe "$PY" games/wordharmony/tools/profile/perf.py port-minute "$O/port$1" build/ph2/tour2/port.txt > "$O/port$1.json" 2> "$O/port$1.err"; echo "port $1: $(tail -1 "$O/port$1.json" | cut -c1-1500)"; }
for k in $(seq 1 "$N"); do
	if [ $((k % 2)) = 1 ]; then run_orig $k; run_port $k; else run_port $k; run_orig $k; fi
done
echo DONE
