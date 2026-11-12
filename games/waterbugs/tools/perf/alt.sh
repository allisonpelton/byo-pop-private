#!/bin/bash
# alt.sh N LABEL=KIND[:EXE]...: perf.py's runs alternated, N rounds (every run of a round, then the next round), so that what else
# the machine runs spreads over all of them; then perf.py summary. KIND is orig (the original, a pass every 16 ms), portmute (the port with the sound and the music off, as origrun's original, which mixes none, is counted) or port
# (the exe after the colon, else build/native/waterbugs.exe; portmute takes one too). Results in build/wbperf/<label><round>. Each run waits for a free
# game slot (tools/gamecount.py --limit 6). Run from the worktree's root, in the background:
#   python -I tools/pincore.py -- bash games/waterbugs/tools/perf/alt.sh 5 orig=orig before=port:build/wbperf/before/waterbugs.exe after=port:<a copy of the new exe>
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
		name="$label$round"
		until python tools/gamecount.py --limit 6 > /dev/null; do sleep 10; done
		echo "run $name ($kind $exe)"
		if [ "$kind" = orig ]; then
			python games/waterbugs/tools/perf/perf.py orig --paced 16 --label "$name" > "build/wbperf/$name.log" 2>&1
		else
			args="--label $name"
			[ -n "$exe" ] && args="$args --exe $exe"
			[ "$kind" = portmute ] && args="$args --nosound"
			build/native/rundesk.exe python games/waterbugs/tools/perf/perf.py port $args > "build/wbperf/$name.log" 2>&1
		fi
		dirs="$dirs build/wbperf/$name"
	done
done
python games/waterbugs/tools/perf/perf.py summary $dirs
echo "exit 0"
