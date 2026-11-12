#!/bin/bash
# alt.sh <out dir> [rounds]: the original and the port play tour 2's minute (perf.py orig and port-minute) one after the other, alternating
# (the sides rotate by one place each round, so each takes each place the same number of times), all on one performance core that no other
# pincore run holds (run as `python -I tools/pincore.py -- bash games/wordharmony/tools/profile/alt.sh <out> 5`). The measure is the cycles
# of the process's threads over the minute (perf.py: cycles_in_minute_g, QueryThreadCycleTime; GetProcessTimes charges by the 15.6 ms tick
# and is not used), so the machine's other load moves it little; every run waits for room first (tools/gamecount.py --limit 6). Prints one
# JSON line a run.
#   WH_ALT_SIDES="orig port"  the sides, any of: orig (the original under origrun), port (the exe as it is: the surface display),
#                             portold (the same exe with SURFACE_DISPLAY=off: the framework's own display, the picture uploaded to the renderer)
#                             portrs1 (the port run with -renderscale=1, which replaces WH_PERF_PORTARGS: the screen texture at the logical size)
#   WH_ALT_PORT_SCRIPT, WH_ALT_ORIG_SCRIPT  the two sides' scripts (tour 2's by default; with WH_PERF_FRAMES=300:900, a script that
#                             only waits in the main menu: "10 state" for the port, "at 10 move 0 0" for the original)
#   WH_PERF_PORTARGS="-windowscale=1"  more arguments for the port's runs (the window at the original's size)
#   WH_PERF_PATCHES=<file> the original's patches (perf-lean: no screen copy), WH_PERF_EXE=<name> a copy of the port.
# Needs build/ph2/tour2/{port,orig}.txt (make.py of tools/shots).
cd "$(git rev-parse --show-toplevel)" || exit 1
O=${1:-build/perf/alt}; N=${2:-5}; mkdir -p "$O"
SIDES=(${WH_ALT_SIDES:-orig port})
PS=${WH_ALT_PORT_SCRIPT:-build/ph2/tour2/port.txt}; OS=${WH_ALT_ORIG_SCRIPT:-build/ph2/tour2/orig.txt}
PY=$(cygpath -w "$(command -v python)")
waitslot() { for w in $(seq 1 480); do python tools/gamecount.py --limit 6 > /dev/null && return; sleep 15; done; }
run_orig() { waitslot; python games/wordharmony/tools/profile/perf.py orig "$O/orig$1" "$OS" 1 > "$O/orig$1.json" 2> "$O/orig$1.err"; echo "orig $1: $(tail -1 "$O/orig$1.json" | cut -c1-1500)"; }
run_port() {  # <side name> <round> <environment assignment, or nothing>
	waitslot; env $3 build/native/rundesk.exe "$PY" games/wordharmony/tools/profile/perf.py port-minute "$O/$1$2" "$PS" > "$O/$1$2.json" 2> "$O/$1$2.err"
	echo "$1 $2: $(tail -1 "$O/$1$2.json" | cut -c1-1500)"
}
for k in $(seq 1 "$N"); do
	for i in $(seq 0 $((${#SIDES[@]} - 1))); do
		case "${SIDES[$(((i + k - 1) % ${#SIDES[@]}))]}" in
			orig) run_orig $k ;;
			port) run_port port $k ;;
			portold) run_port portold $k SURFACE_DISPLAY=off ;;
			portrs1) run_port portrs1 $k WH_PERF_PORTARGS=-renderscale=1 ;;
		esac
	done
done
echo DONE
