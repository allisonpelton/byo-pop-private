#!/bin/bash
# pairs.sh <out dir> [rounds]: the performance pass's measurement (GAME_NOTES.md, "Performance"): the original and the port in its
# configurations turn about over the scenario (scenario.txt, three games of the port's bot), each run pinned to a performance core of
# its own (tools/pincore.py), one game process at a time, counting cycles at the marker updates (cycles.py). A round is one run of the
# original and one of each configuration of the port, in an order that turns by one place each round, so that a drift of the machine falls
# on each alike:
#   port3d   Is3D=1 (Direct3D 11), windowed at the game's own size (-windowscale=1): like for like with the original's window
#   port2d   Is3D=0 (the game's 2D path; the display line still names SDL's Direct3D 11 renderer), windowed at the game's own size
#   portdef  Is3D=1, the window the port opens by default (twice the game's size where it fits): what a player gets
#   orig     on rundesk's private desktop (its software screen, docs/ORIGRUN.md), windowed 640x480
# CONFIGS="port3d port2d portdef" picks the port's configurations. Waits first, up to QUIET_S seconds (default 900), for a quiet machine
# (2 other game processes or fewer); past that it measures anyway, and every run waits for room (fewer than 6 game processes) before it
# starts; each run's line says how many other game processes ran (others_start, others_end) and how busy the machine was. PERF_EXE and
# PERF_MARKS go to cycles.py (its header). Run it in the background from the worktree root: it ends by itself.
# `python games/rocketmania/tools/profile/cycles.py summary <out dir>` prints the medians.
cd "$(dirname "$0")/../../../.." || exit 2
OUT="$1"; N="${2:-5}"; QUIET_S="${QUIET_S:-900}"; CONFIGS="${CONFIGS:-port3d port2d portdef}"
PY="$(cygpath -w "$(command -v python)")"
CYC=games/rocketmania/tools/profile/cycles.py
SC=games/rocketmania/tools/profile/scenario.txt
mkdir -p "$OUT"
start=$(date +%s)
until python -I tools/gamecount.py --limit 3 > /dev/null; do
	[ $(( $(date +%s) - start )) -ge "$QUIET_S" ] && break
	sleep 15
done
echo "start $(date '+%F %T'), waited $(( $(date +%s) - start )) s, $(python -I tools/gamecount.py | head -1)" > "$OUT/runs.txt"
slot() { until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done; }
run() {   # <name> <config>: one pinned run, its result line in <out dir>/<name>.json
	slot
	case "$2" in
	orig) python -I tools/pincore.py -- "$PY" -I $CYC orig "$OUT/$1" "$SC" 2>> "$OUT/errors.txt" | grep '^{' > "$OUT/$1.json" ;;
	port3d) PERF_PORT_REG="ScreenMode=0,Is3D=1" PERF_PORT_SCALE=1 python -I tools/pincore.py -- build/native/rundesk.exe "$PY" -I $CYC port "$OUT/$1" "$SC" 2>> "$OUT/errors.txt" | grep '^{' > "$OUT/$1.json" ;;
	port2d) PERF_PORT_REG="ScreenMode=0,Is3D=0" PERF_PORT_SCALE=1 python -I tools/pincore.py -- build/native/rundesk.exe "$PY" -I $CYC port "$OUT/$1" "$SC" 2>> "$OUT/errors.txt" | grep '^{' > "$OUT/$1.json" ;;
	portdef) PERF_PORT_REG="ScreenMode=0,Is3D=1" PERF_PORT_SCALE=default python -I tools/pincore.py -- build/native/rundesk.exe "$PY" -I $CYC port "$OUT/$1" "$SC" 2>> "$OUT/errors.txt" | grep '^{' > "$OUT/$1.json" ;;
	esac
	echo "$1 $(date '+%T') $(python -c "import json,sys; r=json.loads(open(sys.argv[1]).readline()); print('markers', r['markers_seen'], 'exit', r['exit'], 'others', r['others_start'], r['others_end'], 'busy', r['machine_busy'], 'states', r.get('states'), r.get('display'))" "$OUT/$1.json" 2>&1 | tail -1)" >> "$OUT/runs.txt"
}
ALL=(orig $CONFIGS)
for i in $(seq 1 "$N"); do
	for j in $(seq 0 $(( ${#ALL[@]} - 1 ))); do
		c="${ALL[$(( (j + i - 1) % ${#ALL[@]} ))]}"
		run "p$i-$c" "$c"
	done
done
echo "end $(date '+%F %T')" >> "$OUT/runs.txt"
