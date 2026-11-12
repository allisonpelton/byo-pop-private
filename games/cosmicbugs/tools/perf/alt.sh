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
START=${START:-0}   # START=5 numbers the rounds from 6 on, to add rounds to an earlier set (its run directories stay)
for round in $(seq $((START + 1)) $((START + N))); do
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
		# every run on a performance core of its own (docs/COORDINATION.md, "Alternating runs"): its children inherit the core
		PIN="python -I tools/pincore.py --"
		if [ "$kind" = orig ]; then
			$PIN python games/cosmicbugs/tools/perf/perf.py orig --paced 16 --label "$name" > "build/cbperf/$name.log" 2>&1
		elif [ -n "$exe" ]; then
			$PIN build/native/rundesk.exe python games/cosmicbugs/tools/perf/perf.py port --exe "$exe" ${scale:+--windowscale $scale} --label "$name" > "build/cbperf/$name.log" 2>&1
		elif [ "$kind" = portmute ]; then
			$PIN build/native/rundesk.exe python games/cosmicbugs/tools/perf/perf.py port --nosound ${scale:+--windowscale $scale} --label "$name" > "build/cbperf/$name.log" 2>&1
		else
			$PIN build/native/rundesk.exe python games/cosmicbugs/tools/perf/perf.py port ${scale:+--windowscale $scale} --label "$name" > "build/cbperf/$name.log" 2>&1
		fi
		# the window and the renderer the run got (the framework's display line, from the port's log)
		[ -f "build/cbperf/$name/game.log" ] && echo "$name: $(grep -h -m1 'display: client' "build/cbperf/$name/game.log" | cut -c1-200)" >> build/cbperf/display-lines.txt
		dirs="$dirs build/cbperf/$name"
	done
done
python games/cosmicbugs/tools/perf/perf.py summary $dirs
echo "exit 0"
