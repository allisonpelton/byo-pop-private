#!/bin/bash
# alt.sh N LABEL=KIND[:EXE]...: perf.py's runs alternated, N rounds (every run of a round, then the next round), so that what else
# the machine runs spreads over all of them; then perf.py summary. KIND is orig (the original, a pass every 16 ms) or port
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
		name="$label$round"
		until python tools/gamecount.py --limit 6 > /dev/null; do sleep 10; done
		echo "run $name ($kind $exe)"
		if [ "$kind" = orig ]; then
			python games/cosmicbugs/tools/perf/perf.py orig --paced 16 --label "$name" > "build/cbperf/$name.log" 2>&1
		elif [ -n "$exe" ]; then
			build/native/rundesk.exe python games/cosmicbugs/tools/perf/perf.py port --exe "$exe" --label "$name" > "build/cbperf/$name.log" 2>&1
		else
			build/native/rundesk.exe python games/cosmicbugs/tools/perf/perf.py port --label "$name" > "build/cbperf/$name.log" 2>&1
		fi
		dirs="$dirs build/cbperf/$name"
	done
done
python games/cosmicbugs/tools/perf/perf.py summary $dirs
echo "exit 0"
