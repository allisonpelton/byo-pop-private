#!/bin/bash
# pairs2.sh <out dir> [pairs]: the performance pass's like-for-like measurement (docs/COORDINATION.md, "The target"): the port and the
# original turn about over the scenario of GAME_NOTES.md section 11 in real time, each run on a performance core of its own
# (tools/pincore.py) and counted as cycles, thread by thread (cyc.py, whose header says what is counted and how). One game process
# at a time; before each run it waits, up to WAIT_S seconds (default 7200), for a free slot among the 6 that the machine allows
# (tools/gamecount.py), then goes on anyway and the run's line says how many other game processes ran.
# PORT_ARGS adds arguments to the port's command line (for instance -renderscale=2), and KINDS names the runs ("port orig" by default;
# "port" alone for the port's own steps; port3d and orig3d are the runs on the visible desktop, in 3D, overnight only). The lines of every run go to <out dir>/runs.txt, each with its kind before it.
# Run it in the background: it ends by itself.
cd "$(dirname "$0")/../../../.." || exit 2
OUT="$1"; WANT="${2:-5}"; WAIT_S="${WAIT_S:-7200}"; KINDS="${KINDS:-port orig}"
PY="$(cygpath -w "$(command -v python)")"
P=games/typershark/tools/profile/cyc.py; P3=games/typershark/tools/profile/cyc3d.py; SC=build/ts-p2/rec/rt2.in.txt
mkdir -p "$OUT"; : > "$OUT/runs.txt"
echo "start $(date '+%F %T')" >> "$OUT/runs.txt"
slot() {
	local start; start=$(date +%s)
	until python -I tools/gamecount.py --limit 6 > /dev/null 2>&1; do
		[ $(( $(date +%s) - start )) -ge "$WAIT_S" ] && break
		sleep 10
	done
}
one() {  # <kind> <command...>: a run's last JSON line with its kind
	local kind="$1"; shift
	slot
	echo "$kind $("$@" 2>> "$OUT/errors.txt" | grep '^{' | tail -1)" >> "$OUT/runs.txt"
}
for i in $(seq 1 "$WANT"); do
	for kind in $KINDS; do
		case "$kind" in
		port) one port python -I tools/pincore.py -- build/native/rundesk.exe "$PY" -I $P port "$OUT/port$i" $SC $PORT_ARGS ;;
		orig) PERF_CLOCK=real PERF_PATCHES=build/ts-p2/patches.txt one orig python -I tools/pincore.py -- "$PY" -I $P orig "$OUT/orig$i" $SC ;;
		# the visible desktop, both in 3D, only between 22:00 and 08:00 (cyc3d.py refuses outside them)
		port3d) one port3d python -I tools/pincore.py -- "$PY" -I $P3 port3d "$OUT/port3d$i" $SC $PORT_ARGS ;;
		orig3d) one orig3d python -I tools/pincore.py -- "$PY" -I $P3 orig3d "$OUT/orig3d$i" $SC ;;
		esac
	done
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
