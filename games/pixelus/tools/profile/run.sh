#!/bin/bash
# The measurements of GAME_NOTES.md's "Performance", back to back: the port and the original alternate, three load runs
# of each and two scripted minutes of each, one game process of ours at a time, each after waiting for a free slot.
# Needs build/perf/port-play.txt (the port's script, to update 6450 and "quit") and build/perf/orig-play.txt (the same input
# for origrun, made by games/pixelus/tools/identical/toorig.py, ending in "quit"), build/native/pixelus.exe,
# build/origrun-bin/origrun.exe and the staged original under C:/byo-og/pix (games/pixelus/tools/identical/setup.sh).
# Run from the worktree root, in the background; the results are build/perf/<name>/perf.json and one line each in the log.
cd "$(git rev-parse --show-toplevel)" || exit 1
R="$(pwd)/build/native/rundesk.exe"
P=games/pixelus/tools/profile/perf.py

for i in 1 2 3; do
	bash build/waitslots.sh > /dev/null; echo "port load $i: $("$R" python -I $P port load build/perf/port-load$i 2>&1 | tail -1 | cut -c1-400)"
	bash build/waitslots.sh > /dev/null; echo "orig load $i: $(python -I $P orig load build/perf/orig-load$i 2>&1 | tail -1 | cut -c1-400)"
done
for i in 1 2; do
	bash build/waitslots.sh > /dev/null; echo "port play $i: $("$R" python -I $P port play build/perf/port-play$i build/perf/port-play.txt 2>&1 | tail -1 | cut -c1-400)"
	bash build/waitslots.sh > /dev/null; echo "orig play $i: $(python -I $P orig play build/perf/orig-play$i build/perf/orig-play.txt 2>&1 | tail -1 | cut -c1-400)"
done
echo "done"
