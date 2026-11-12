#!/bin/bash
# variants.sh <out dir> [rounds]: the port's real-time play of <out dir>/scenario.txt (perf.py port-minute, PERF_CLOCK=real) with
# SDL's renderer settings changed by environment, alternating with the unchanged port so that a busy machine hits all of them:
# default, SDL_RENDER_VSYNC=0, SDL_RENDER_DRIVER=direct3d (Direct3D 9), opengl and software. Results: <out dir>/v<round>-<name>.json,
# summarised by `python games/bookworm/tools/profile/summary.py <out dir> v`. From the worktree root; one process at a time.
OUT=$1; N=${2:-2}
PY="C:\\Python312\\python.exe"
PERF=games/bookworm/tools/profile/perf.py
slot() { until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done; }
run() {   # run <name> [VAR=value]
	local name=$1; shift
	slot; env PERF_CLOCK=real "$@" build/native/rundesk.exe "$PY" $PERF port-minute "$OUT/v$round-$name" "$OUT/scenario.txt" 2> /dev/null | grep '^{' > "$OUT/v$round-$name.json"
}
for round in $(seq 1 "$N"); do
	run default
	run novsync SDL_RENDER_VSYNC=0
	run d3d9 SDL_RENDER_DRIVER=direct3d
	run gl SDL_RENDER_DRIVER=opengl
	run software SDL_RENDER_DRIVER=software
done
echo finished > "$OUT/vdone.txt"
