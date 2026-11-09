#!/bin/bash
# drivers.sh <driver>...: the port's scripted minute (perf.py port play) once per SDL render driver, by SDL_RENDER_DRIVER
# ("direct3d11" is what SDL picks on this machine; also "direct3d", "opengl", "software"), one game process of ours at a time, each
# after waiting for room. The results are build/perf/drv-<driver>/perf.json and one line each on stdout. Run from the worktree root.
cd "$(git rev-parse --show-toplevel)" || exit 1
R="$(pwd)/build/native/rundesk.exe"
for d in "$@"; do
	bash build/waitslots.sh > /dev/null
	echo "$d: $(SDL_RENDER_DRIVER=$d "$R" python -I games/alchemy/tools/profile/perf.py port play build/perf/drv-$d build/perf/port-play.txt 2>&1 | tail -1 | cut -c1-330)"
done
echo "drivers done"
