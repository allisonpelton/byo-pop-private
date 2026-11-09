#!/bin/bash
# The before and after of the performance pass (GAME_NOTES.md, section 10): the port as it was before the pass (build/perf2/before/, a copy of
# the build at the merge of the drag runs) and the port now (build/native), alternately so that the machine's drift falls on both. From the
# worktree root: bash games/noahsark/tools/profile/perfpass.sh. Each run waits for room (three game processes on the machine).
#   load x5 for each, twice (A B A B), the scripted minute x2 for each (A B B A), and a whole -fast game x5 for each.
# Results: build/perf2/res/*.json and fast_*.txt, the steps in build/perf2/res/steps.log.
cd "$(dirname "$0")/../../../.."
W=$(pwd -W)
R=build/perf2/res; mkdir -p $R
PERF=games/noahsark/tools/profile/perf.py
room() { until python -I tools/gamecount.py --limit 3 > /dev/null; do sleep 15; done; }
log() { echo "$(date +%H:%M:%S) $*" >> $R/steps.log; }
exe() { if [ "$1" = before ]; then echo "$W/build/perf2/before/noahsark.exe"; else echo "$W/build/native/noahsark.exe"; fi; }
rm -f $R/steps.log $R/perfpass.done
for round in 1 2; do
  for v in before after; do
    room; log "load $v round $round"
    PORT_EXE=$(exe $v) build/native/rundesk.exe python $PERF port-load $R/pl_${v}_$round 5 > $R/load_${v}_$round.json 2>> $R/steps.log
  done
done
(cat build/nt/perf/menu.txt; echo "7500 quit") > $R/minute.txt
for v in before after after before; do
  room; log "minute $v"
  n=$(ls $R/minute_${v}_*.json 2>/dev/null | wc -l)
  PORT_EXE=$(exe $v) build/native/rundesk.exe python $PERF port-minute $R/pm_${v}_$n $R/minute.txt 1 > $R/minute_${v}_$n.json 2>> $R/steps.log
done
for i in 1 2 3 4 5; do
  for v in before after; do
    room
    rm -rf $R/user_fast
    s=$(date +%s%N)
    build/native/rundesk.exe "$(exe $v)" -gamedir="$W/assets/noahsark" -userdir="$W/$R/user_fast" -fast -seed=1 -script="$W/build/perf2/port0.txt" -autoplay=1 -autoplaymenu -autoplaymode=action -autoplaydiff=0 -autoplaycap=7400 > $R/fast_${v}_$i.log 2>&1
    e=$(date +%s%N)
    echo "$v $i $(( (e - s) / 1000000 )) ms; $(grep 'Seconds =' $R/fast_${v}_$i.log | sed 's/INFO: //')" >> $R/fast_all.txt
  done
done
log done; touch $R/perfpass.done
