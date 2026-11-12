#!/bin/bash
# pairs.sh <out dir> [pairs]: the performance pass's measurement (GAME_NOTES.md, "Performance"): the original and the port turn
# about over the scenario scenario/classic909.json (perf.py), one game process at a time. Each pair is four runs: the two in real
# time (PERF_CLOCK=real: what a player's machine does) and the two without waiting (the work a test run does); then one load of
# each, timed to the title. BEFORE_EXE=<name> adds a run without waiting of that exe in build/native (a build kept from before a
# change). Waits first, up to WAIT_S seconds (default 3600), for at most one other game process; past that it measures anyway, and
# each run's line says how many other game processes ran (others_start, others_end). The lines of every run go to
# <out dir>/runs.txt, each with its kind before it. The port is also run in 3D (PERF_MODE=3d: the 1024 by 768 game on the GPU renderer, what
# a player's machine runs; scenario/classic909_3d.json): port3d-real, port3d-work, port3d-load; the original is measured in 2D only. Run it in the background: it ends by itself.
cd "$(dirname "$0")/../../../.." || exit 2
OUT="$1"; WANT="${2:-3}"; WAIT_S="${WAIT_S:-3600}"
P=games/bejeweled2/tools/profile/perf.py
PY="$(cygpath -w "$(command -v python)")"
mkdir -p "$OUT"; : > "$OUT/runs.txt"
start=$(date +%s)
until python -I tools/gamecount.py --limit 2 > /dev/null 2>&1; do
	[ $(( $(date +%s) - start )) -ge "$WAIT_S" ] && break
	sleep 15
done
echo "start $(date '+%F %T'), waited $(( $(date +%s) - start )) s" >> "$OUT/runs.txt"
one() {  # <kind> <command...>: a run's last JSON line with its kind
	local kind="$1"; shift
	echo "$kind $("$@" 2>> "$OUT/errors.txt" | grep others_start | tail -1)" >> "$OUT/runs.txt"
}
for i in $(seq 1 "$WANT"); do
	PERF_CLOCK=real one "orig-real" python -I $P orig "$OUT/orig-real$i" 1
	PERF_CLOCK=real one "port-real" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/port-real$i"
	one "orig-work" python -I $P orig "$OUT/orig-work$i" 1
	one "port-work" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/port-work$i"
	[ -n "${BEFORE_EXE:-}" ] && PERF_EXE="$BEFORE_EXE" one "before-work" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/before-work$i"
	one "orig-load" python -I $P orig "$OUT/orig-load$i" 1 load
	one "port-load" build/native/rundesk.exe "$PY" -I $P port-load "$OUT/port-load$i" 1
	PERF_CLOCK=real PERF_MODE=3d one "port3d-real" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/port3d-real$i"
	PERF_MODE=3d one "port3d-work" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/port3d-work$i"
	PERF_MODE=3d one "port3d-load" build/native/rundesk.exe "$PY" -I $P port-load "$OUT/port3d-load$i" 1
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
