#!/bin/bash
# pairs.sh <out dir> [pairs]: the performance pass's measurement (GAME_NOTES.md, "Performance"): the original and the port turn
# about over the scenario build/ts-p2/rec/rt2.in.txt (perf.py), one game process at a time. Each pair is the runs in real time
# (PERF_CLOCK=real, PERF_END=7000, PERF_STRETCH=2000: what a player's machine does) and the runs without waiting (the work a test
# run does; WORK_END and WORK_STRETCH, 12000 and 5000 unless set); then one load of each, timed to the loading thread's flag
# (the original) and to the first update (the port). BEFORE_EXE=<name> adds a run without waiting of that exe in build/native (a
# build kept from before a change). Waits first, up to WAIT_S seconds (default 3600), for no game process anywhere; past that it
# measures anyway, and each run's line says how many other game processes ran (others_start, others_end).
# orig-work runs with phase 2's loading patch (build/ts-p2/patches.txt: the copy loads on the main thread, so that its first typed
# letters meet a loaded game whatever the counter at the flag); a run whose game did not get into play says NOT PLAYED (summary.py).
# The -sw runs set SDL_RENDER_DRIVER=software: a test of the renderer's cost, nothing more (no default changes without the user).
# The lines of every run go to <out dir>/runs.txt, each with its kind before it. Run it in the background: it ends by itself.
cd "$(dirname "$0")/../../../.." || exit 2
OUT="$1"; WANT="${2:-3}"; WAIT_S="${WAIT_S:-3600}"
WORK_END="${WORK_END:-12000}"; WORK_STRETCH="${WORK_STRETCH:-5000}"
P=games/typershark/tools/profile/perf.py; SC=build/ts-p2/rec/rt2.in.txt
PY="$(cygpath -w "$(command -v python)")"
mkdir -p "$OUT"; : > "$OUT/runs.txt"
start=$(date +%s)
until python -I tools/gamecount.py --limit 1 > /dev/null 2>&1; do
	[ $(( $(date +%s) - start )) -ge "$WAIT_S" ] && break
	sleep 15
done
echo "start $(date '+%F %T'), waited $(( $(date +%s) - start )) s" >> "$OUT/runs.txt"
one() {  # <kind> <command...>: a run's last JSON line with its kind
	local kind="$1"; shift
	echo "$kind $("$@" 2>> "$OUT/errors.txt" | grep others_start | tail -1)" >> "$OUT/runs.txt"
}
for i in $(seq 1 "$WANT"); do
	export PERF_CLOCK=real PERF_END=7000 PERF_STRETCH=2000
	one "orig-real" python -I $P orig "$OUT/orig-real$i" $SC 1
	one "port-real" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/port-real$i" $SC
	SDL_RENDER_DRIVER=software one "port-real-sw" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/port-real-sw$i" $SC   # SDL's software renderer: no Direct3D device
	unset PERF_CLOCK; export PERF_END="$WORK_END" PERF_STRETCH="$WORK_STRETCH"
	PERF_PATCHES=build/ts-p2/patches.txt one "orig-work" python -I $P orig "$OUT/orig-work$i" $SC 1
	one "port-work" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/port-work$i" $SC
	SDL_RENDER_DRIVER=software PERF_DRAW=every one "port-work-every-sw" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/port-work-every-sw$i" $SC
	PERF_DRAW=every one "port-work-every" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/port-work-every$i" $SC   # a draw every update, as the original does
	[ -n "${BEFORE_EXE:-}" ] && PERF_EXE="$BEFORE_EXE" one "before-work" build/native/rundesk.exe "$PY" -I $P port-minute "$OUT/before-work$i" $SC
	PERF_CLOCK=real one "orig-load" python -I $P orig "$OUT/orig-load$i" $SC 1 load
	one "port-load" build/native/rundesk.exe "$PY" -I $P port-load "$OUT/port-load$i" 1
	SDL_RENDER_DRIVER=software one "port-load-sw" build/native/rundesk.exe "$PY" -I $P port-load "$OUT/port-load-sw$i" 1
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
