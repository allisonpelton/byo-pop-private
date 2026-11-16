#!/bin/bash
# PACED=ms (default 4: a longer sleep starves the 60 draws a second (8 ms gives 41, 13 gives 10), 4 gives 59) is the sleep a pass of the original's main loop makes (perf.py: paced_patches).
# pairs.sh <pairs> [split]: Hammer Heads' processor time like for like (docs/COORDINATION.md, "The target"): the port and the original
# turn about, one game process at a time, each on a performance core of its own (tools/pincore.py), updates 4000 to 8000 of the
# classic1 inputs (level 1 in play), the window of both 800x600. Without "split" the counting is paused in the window (the player's
# cycles); with it the parts are counted. Results: build/perf/hammerheads/<mode><n>-{port,orig}/{port,orig}.json; summed by
# `python games/hammerheads/tools/perf/pairsum.py <mode>`. From the worktree root.
N=${1:-5}; MODE=${2:-total}; EXTRA="--total"; [ "$MODE" = split ] && EXTRA=""
slot() { until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 15; done; }
for i in $(seq 1 "$N"); do
	slot; python -I tools/pincore.py -- python games/hammerheads/tools/perf/perf.py port $MODE$i-port $EXTRA 2>&1 | tail -1
	slot; python -I tools/pincore.py -- python games/hammerheads/tools/perf/perf.py orig $MODE$i-orig $EXTRA --paced ${PACED:-4} 2>&1 | tail -1
done
echo finished
