#!/bin/bash
# softpath.sh: the default drawing path against SDL's software renderer, on Alchemy's scripted bot game
# (games/alchemy/docs/GAME_NOTES.md, "The software path"). It takes build/perf/port-play.txt (the performance runs'
# script: the bot's mouse game of 60 seconds from update 450 to 6450) and makes two scripts of it: the whole game with
# the framework's memreport command at the title (update 300), in play (3000) and at the end (6440), and its first 1500
# updates for the frame times. Each path (the default; SDL_RENDER_DRIVER=software) runs them windowed and in full
# screen: the whole game with -fast and in real time, and the short one with -fast and with -fast -drawevery, whose
# difference in wall time over their difference in draws is the time a frame takes. Both paths also run with
# -renderscale=1, which draws at the game's own 640 x 480 and scales only the finished frame. One game process at a time, after waiting for room (at most 3 on the
# machine). The results are build/perf/sp/<run>.json and one line each; softpath_table.py makes the table of them.
# Run from the worktree root. RUNS="<names>" runs only those (see the list below).
cd "$(git rev-parse --show-toplevel)" || exit 1
W=$(pwd -W)
mkdir -p build/perf/sp
python -I - <<'PYEOF'
lines = [l.rstrip('\n') for l in open('build/perf/port-play.txt')]
quit_line = [l for l in lines if l.endswith(' quit')][0]
body = [(int(l.split()[0]), l) for l in lines if l != quit_line]
full = sorted(body + [(k, '%d memreport' % k) for k in (300, 3000, 6440)], key=lambda t: t[0])
open('build/perf/sp/full.txt', 'w').write('\n'.join(l for _, l in full) + '\n' + quit_line + '\n')
open('build/perf/sp/short.txt', 'w').write('\n'.join(l for k, l in body if k <= 1500) + '\n1500 quit\n')
PYEOF
run() {     # run <name> <fullscreen 0|1> <script> <path: default|software> <game arguments...>
	local name=$1 fs=$2 script=$3 path=$4; shift 4
	if [ -n "$RUNS" ] && ! echo " $RUNS " | grep -q " $name "; then return; fi
	for i in $(seq 1 360); do python -I tools/gamecount.py --limit 3 > /dev/null && break; sleep 10; done
	local envarg=""
	[ "$path" = software ] && envarg="--env SDL_RENDER_DRIVER=software"
	build/native/rundesk.exe python -I games/alchemy/tools/profile/softpath.py "$W/build/perf/sp/$name.json" --exe "$W/build/native/alchemy.exe" \
		--script "$W/build/perf/sp/$script.txt" --fullscreen $fs $envarg --timeout ${TIMEOUT:-900} -- "$@" 2>&1 | tail -1
}
# d: the default path as it is (it draws at the window's scale); d1: the default path drawing at the game's own size;
# s1: the software renderer drawing at the game's own size, scaled only as the frame goes to the window; s: the software
# renderer drawing at the window's scale
for v in d d1 s1 s; do
	case $v in d|d1) path=default;; *) path=software;; esac
	case $v in d1|s1) scale=-renderscale=1;; *) scale=;; esac
	for fs in 0 1; do
		run $v-fs$fs-short-fast $fs short $path -fast $scale
		run $v-fs$fs-short-every $fs short $path -fast -drawevery $scale
		run $v-fs$fs-game-fast $fs full $path -fast $scale
		run $v-fs$fs-game-real $fs full $path $scale
		[ $v = s ] || run $v-fs$fs-game-every $fs full $path -fast -drawevery $scale      # the long pair for the frame time; too slow for s
	done
done
