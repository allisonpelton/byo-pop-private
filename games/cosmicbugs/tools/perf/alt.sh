#!/bin/bash
# A KIND may end in @N (port@1, portmute@1): the port's window at N times the game's 800x600 (perf.py --windowscale; the
# original's window is 800x600, so @1 is like for like; without it the port's window is the framework's default 2x).
# alt.sh N LABEL=KIND[:EXE]...: perf.py's runs alternated, N rounds (every run of a round, then the next round), so that what else
# the machine runs spreads over all of them; then perf.py summary. KIND is orig (the original, a pass every 16 ms), portmute (the port with the sound and the music off, as origrun's original, which mixes none, is counted) or port
# (the exe after the colon, else build/native/cosmicbugs.exe). Results in build/cbperf/<label><round>. Each run waits for a free
# game slot (tools/gamecount.py --limit 6). Run from the worktree's root, in the background:
#   games/cosmicbugs/tools/perf/alt.sh 3 orig=orig before=port:build/cbperf/before/cosmicbugs.exe after=port
cd "$(dirname "$0")/../../../.." || exit 1
N=$1
shift
dirs=""
for round in $(seq 1 "$N"); do
	for spec in "$@"; do
		label=${spec%%=*}
		rest=${spec#*=}
		kind=${rest%%:*}
		exe=""
		if [ "$rest" != "$kind" ]; then
			exe=${rest#*:}
		fi
		scale=""
		case "$kind" in *@*) scale=${kind#*@}; kind=${kind%%@*} ;; esac
		name="$label$round"
		until python tools/gamecount.py --limit 6 > /dev/null; do sleep 10; done
		echo "run $name ($kind $exe)"
		if [ "$kind" = orig ]; then
			python games/cosmicbugs/tools/perf/perf.py orig --paced 16 --label "$name" > "build/cbperf/$name.log" 2>&1
		elif [ -n "$exe" ]; then
			build/native/rundesk.exe python games/cosmicbugs/tools/perf/perf.py port --exe "$exe" ${scale:+--windowscale $scale} --label "$name" > "build/cbperf/$name.log" 2>&1
		elif [ "$kind" = portmute ]; then
			build/native/rundesk.exe python games/cosmicbugs/tools/perf/perf.py port --nosound ${scale:+--windowscale $scale} --label "$name" > "build/cbperf/$name.log" 2>&1
		else
			build/native/rundesk.exe python games/cosmicbugs/tools/perf/perf.py port ${scale:+--windowscale $scale} --label "$name" > "build/cbperf/$name.log" 2>&1
		fi
		dirs="$dirs build/cbperf/$name"
	done
done
python games/cosmicbugs/tools/perf/perf.py summary $dirs
echo "exit 0"
