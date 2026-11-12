#!/bin/bash
# pairs.sh <out dir> [pairs]: the performance pass's measurement (GAME_NOTES.md, "Performance"): the original and the port turn about,
# one game process at a time, a pair being one run of every kind below. Wrap the whole loop in tools/pincore.py (one performance core of its
# own), take five pairs at least, and read runs.txt with summarize.py. Processor time is the cycle count (QueryProcessCycleTime), never
# GetProcessTimes (the 15.6 ms tick). The kinds, each with the original and the port:
#   *-idle  the idle board in real time (PERF_STRETCH=idle: the recording's inputs up to the first dialog's click, then none, to update 5700; the
#           last 20 s): orig-idle (2D), port-idle (2D on SDL's software renderer: DDInterface::mIs3D false, as every test run), port2dgpu-idle
#           (2D on the GPU renderer: a player's machine), orig3d-idle (under dgVoodoo2), port3d-idle (1024 by 768, PERF_MODE=3d)
#   *-work  the recorded game (scenario/classic909.json, 3D: classic909_3d.json) without waiting; the port as a -commands file under -hold
#   *-load  the process's start to the title taking input
# BEFORE_EXE=<name> adds a -work run of an exe kept in build/native from before a change; KINDS="orig-idle port-idle ..." runs only those. Waits
# first, up to WAIT_S seconds (default 3600), for at most one other game process; past that it measures anyway, and each run's line says how
# many other game processes ran (others_start, others_end). The lines go to <out dir>/runs.txt, each with its kind before it. Run it in the
# background: it ends by itself (about 7 minutes a pair).
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
KINDS="${KINDS:-orig-idle port-idle port2dgpu-idle orig3d-idle port3d-idle orig-work port-work port2dgpu-work port3d-work orig-load port-load port2dgpu-load orig3d-load port3d-load}"
want() { case " $KINDS " in *" $1 "*) return 0;; esac; return 1; }
for i in $(seq 1 "$WANT"); do
	# the idle stretch (PERF_STRETCH=idle: the board idle, the last 20 s of 57 s of play, in real time; the port plays it as a -script run)
	want orig-idle && PERF_CLOCK=real PERF_STRETCH=idle one "orig-idle" python -I $P orig "$OUT/orig-idle$i" 1
	want port-idle && PERF_STRETCH=idle one "port-idle" build/native/rundesk.exe "$PY" -I $P port-idle "$OUT/port-idle$i"
	want port2dgpu-idle && PERF_STRETCH=idle PERF_RENDER=gpu one "port2dgpu-idle" build/native/rundesk.exe "$PY" -I $P port-idle "$OUT/port2dgpu-idle$i"
	want orig3d-idle && PERF_CLOCK=real PERF_STRETCH=idle PERF_MODE=3d one "orig3d-idle" python -I $P orig "$OUT/orig3d-idle$i" 1
	want port3d-idle && PERF_STRETCH=idle PERF_MODE=3d one "port3d-idle" build/native/rundesk.exe "$PY" -I $P port-idle "$OUT/port3d-idle$i"
	# the recorded game, without waiting (the work of the game's code and drawing; the port as a -commands file under -hold, its poll counted)
	want orig-work && one "orig-work" python -I $P orig "$OUT/orig-work$i" 1
	want port-work && one "port-work" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/port-work$i"
	want port2dgpu-work && PERF_RENDER=gpu one "port2dgpu-work" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/port2dgpu-work$i"
	want port3d-work && PERF_MODE=3d one "port3d-work" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/port3d-work$i"
	want orig3d-work && PERF_MODE=3d one "orig3d-work" python -I $P orig "$OUT/orig3d-work$i" 1
	[ -n "${BEFORE_EXE:-}" ] && PERF_EXE="$BEFORE_EXE" one "before-work" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/before-work$i"
	want orig-load && one "orig-load" python -I $P orig "$OUT/orig-load$i" 1 load
	want port-load && one "port-load" build/native/rundesk.exe "$PY" -I $P port-load "$OUT/port-load$i" 1
	want port2dgpu-load && PERF_RENDER=gpu one "port2dgpu-load" build/native/rundesk.exe "$PY" -I $P port-load "$OUT/port2dgpu-load$i" 1
	want orig3d-load && PERF_MODE=3d one "orig3d-load" python -I $P orig "$OUT/orig3d-load$i" 1 load
	want port3d-load && PERF_MODE=3d one "port3d-load" build/native/rundesk.exe "$PY" -I $P port-load "$OUT/port3d-load$i" 1
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
