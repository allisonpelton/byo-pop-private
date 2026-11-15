#!/bin/bash
# softshots.sh: the pictures of the software path against the default path's, and the frames for tools/scaletest
# (games/alchemy/docs/GAME_NOTES.md, "The software path"). Run from the worktree root, after softpath.sh has made
# build/perf/sp/short.txt.
#  1. One frame of the bot's game (update 1200) by the default path: drawn at the game's own size (frame640.png, the
#     input of scaletest), at twice the size in a window (default2x.png) and at four times in full screen (default4x.png).
#  2. The screen tours 1, 2, 5 and 7 (tools/shots), windowed, by the default path and by the software renderer, each
#     at the window's scale and at -renderscale=1; the screenshots of a pair are then compared byte for byte.
# One game process at a time, after waiting for room. Everything goes under build/perf/ss.
cd "$(git rev-parse --show-toplevel)" || exit 1
W=$(pwd -W)
S=build/perf/ss
mkdir -p $S
one() {     # one <name> <fullscreen> <script> <path> <game arguments...>
	local name=$1 fs=$2 script=$3 path=$4; shift 4
	for i in $(seq 1 360); do python -I tools/gamecount.py --limit 6 > /dev/null && break; sleep 10; done
	local envarg=""
	[ "$path" = software ] && envarg="--env SDL_RENDER_DRIVER=software"
	build/native/rundesk.exe python -I games/alchemy/tools/profile/softpath.py "$W/$S/$name.json" --exe "$W/build/native/alchemy.exe" \
		--script "$script" --fullscreen $fs $envarg --timeout 900 -- -fast "$@" 2>&1 | tail -1 | cut -c1-110
}
frame() {   # frame <name> <fullscreen> <path> <command> <game arguments...>: the bot's game with one screenshot at update 1200
	local name=$1 fs=$2 path=$3 cmd=$4; shift 4
	awk -v f="$W/$S/$name.png" -v c="$cmd" '$1 > 1200 && !done { print "1200 " c " " f; done = 1 } { print }' build/perf/sp/short.txt > $S/$name.txt
	one $name $fs "$W/$S/$name.txt" $path "$@"
}
if [ -z "$ONLY" ] || [ "$ONLY" = frames ]; then
	frame frame640 0 default screenshot -renderscale=1
	frame default2x 0 default screenshotfull
	frame default4x 1 default screenshotfull
	frame soft640 0 software screenshot -renderscale=1
	frame soft2x 0 software screenshotfull
fi
if [ -z "$ONLY" ] || [ "$ONLY" = tours ]; then
	for t in tour1 tour2 tour5 tour7; do
		for v in d s d1 s1; do
			case $v in d|d1) path=default;; *) path=software;; esac
			case $v in d1|s1) scale=-renderscale=1;; *) scale=;; esac
			D=$S/$t-$v
			rm -rf $D; mkdir -p $D/port
			python -I games/alchemy/tools/shots/make.py games/alchemy/tools/shots/$t.txt "$W/$D" port > $D/port.txt
			one $t-$v 0 "$W/$D/port.txt" $path $scale > /dev/null
		done
		for pair in "d s" "d1 s1"; do
			set -- $pair
			n=0; bad=0
			for f in $S/$t-$1/port/*.png; do
				n=$((n+1)); cmp -s "$f" "$S/$t-$2/port/$(basename "$f")" || bad=$((bad+1))
			done
			echo "$t $1 against $2: $n shots, $bad differ"
		done
	done
fi
