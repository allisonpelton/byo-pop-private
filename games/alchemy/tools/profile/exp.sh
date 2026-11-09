#!/bin/bash
# exp.sh <name> [VAR=VALUE ...]: the port's scripted minute (build/perf/port-play.txt, seed 11) once with the given environment, by
# perf.py port play, after waiting for room (at most 3 game processes machine-wide, tools/gamecount.py). The result is
# build/perf/exp-<name>/perf.json and one line on stdout. Run from the worktree root. Examples of what it is for:
#   exp.sh software SDL_RENDER_DRIVER=software        the SDL software renderer instead of the GPU one
#   exp.sh novsync SDL_RENDER_VSYNC=0                 no wait for the display's refresh
#   exp.sh nobatch SDL_RENDER_BATCHING=0              SDL's draw queue flushed at every call (the cost of not batching)
cd "$(git rev-parse --show-toplevel)" || exit 1
name=$1; shift
for i in $(seq 1 360); do
	python -I tools/gamecount.py --limit 3 > /dev/null && break
	sleep 10
done
rm -rf "build/perf/exp-$name"
echo "$name: $(env "$@" build/native/rundesk.exe python -I games/alchemy/tools/profile/perf.py port play "build/perf/exp-$name" build/perf/port-play.txt 2>&1 | tail -1 | cut -c1-420)"
