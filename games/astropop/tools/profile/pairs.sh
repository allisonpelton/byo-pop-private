#!/bin/bash
# pairs.sh <out dir> [pairs]: the performance pass's measurement (GAME_NOTES.md, "Performance"): the original and the port turn
# about over the scenario scenario/classic11.json (perf.py), one game process at a time, in 2D, in real time (PERF_CLOCK=real: what a
# player's machine does), the order flipping every pair; then one load of each, timed to the title. Processor time is the threads' cycle
# counts (QueryThreadCycleTime), as the coordinator asks (2026-11-11), and the whole loop runs on one performance core of its own
# (tools/pincore.py: this script starts itself under it). Five pairs and medians are the rule.
#   WORK=1        also a run of each without waiting (the work a test run does)
#   BEFORE_EXE=<name>  adds a run without waiting of that exe in build/native (a build kept from before a change)
#   PERF_3D=1 PERF_DESKTOP=1  the port in 3D on the visible desktop (22:00 to 08:00; the original is `night3d.sh`'s side, perf.py orig3d)
# Waits first, up to WAIT_S seconds (default 3600), for room (at most 6 game processes on the machine); each run's line says how many other
# game processes ran (others_start, others_end). The lines of every run go to <out dir>/runs.txt, each with its kind before it. Run it in
# the background: it ends by itself.
cd "$(dirname "$0")/../../../.." || exit 2
if [ -z "${PINCORED:-}" ]; then
	PINCORE=tools/pincore.py
	[ -f "$PINCORE" ] || PINCORE=C:/Users/Administrator/Documents/claude/byo-pop/tools/pincore.py
	export PINCORED=1
	exec python -I "$PINCORE" -- bash "$0" "$@"
fi
OUT="$1"; WANT="${2:-5}"; WAIT_S="${WAIT_S:-3600}"
P=games/astropop/tools/profile/perf.py
PY="$(cygpath -w "$(command -v python)")"
mkdir -p "$OUT"; : > "$OUT/runs.txt"
start=$(date +%s)
until python -I tools/gamecount.py --limit 6 > /dev/null 2>&1; do
	[ $(( $(date +%s) - start )) -ge "$WAIT_S" ] && break
	sleep 15
done
echo "start $(date '+%F %T'), waited $(( $(date +%s) - start )) s" >> "$OUT/runs.txt"
one() {  # <kind> <command...>: a run's last JSON line with its kind
	local kind="$1"; shift
	echo "$kind $("$@" 2>> "$OUT/errors.txt" | grep others_start | tail -1)" >> "$OUT/runs.txt"
}
orig() { one "$1" python -I $P orig "$2" 1 $3; }
port() { one "$1" build/native/rundesk.exe "$PY" -I $P port-minute "$2"; }
for i in $(seq 1 "$WANT"); do
	if [ $((i % 2)) -eq 1 ]; then
		PERF_CLOCK=real orig orig-real "$OUT/orig-real$i"
		PERF_CLOCK=real port port-real "$OUT/port-real$i"
	else
		PERF_CLOCK=real port port-real "$OUT/port-real$i"
		PERF_CLOCK=real orig orig-real "$OUT/orig-real$i"
	fi
	if [ -n "${WORK:-}" ]; then
		orig orig-work "$OUT/orig-work$i"
		port port-work "$OUT/port-work$i"
		[ -n "${BEFORE_EXE:-}" ] && PERF_EXE="$BEFORE_EXE" port before-work "$OUT/before-work$i"
	fi
	orig orig-load "$OUT/orig-load$i" load
	one "port-load" build/native/rundesk.exe "$PY" -I $P port-load "$OUT/port-load$i" 1
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
