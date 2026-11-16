#!/bin/bash
# NOTE (2026-11-15): the software-screen override and HAMMERHEADS_SCREEN=gpu were taken out afterwards; g3off and s3off now run the same (GPU) build, so only g3on and g3onnv are repeatable.
# The screens against each other (the coordinator's order of 2026-11-15): per round one original (paced) and the port in four ways, each
# one game process at a time on a performance core of its own, updates 4000 to 8000: g3off (the GPU path, 3D off:
# HAMMERHEADS_SCREEN=gpu, Is3D=0), s3off (the software screen, 3D off), g3on (the default, vsync on), g3onnv (the default with
# SDL_RENDER_VSYNC=0 and -presentlog). Labels build/perf/hammerheads/<way><n>-{port,orig}; peak private bytes in peak.txt.
# From the worktree root: bash games/hammerheads/tools/perf/screens.sh <rounds>
N=${1:-5}; R="$(pwd)"
slot() { until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done; }
run() { slot; python -I tools/pincore.py -- python games/hammerheads/tools/perf/peak.py "$@" --total 2>&1 | tail -1; }
for i in $(seq 1 "$N"); do
	slot; python -I tools/pincore.py -- python games/hammerheads/tools/perf/perf.py orig o$i-orig --total --paced 4 2>&1 | tail -1
	PERF_REG=$'Is3D=dword:0\n' HAMMERHEADS_SCREEN=gpu run g3off$i-port
	PERF_REG=$'Is3D=dword:0\n' run s3off$i-port
	run g3on$i-port
	SDL_RENDER_VSYNC=0 run g3onnv$i-port --port-arg -presentlog=$R/build/perf/hammerheads/presents-nv$i.txt
done
echo finished
