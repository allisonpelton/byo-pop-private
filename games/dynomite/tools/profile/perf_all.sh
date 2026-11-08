#!/bin/bash
# The measurements of GAME_NOTES.md, "Performance", one run after another, each started when the machine has room
# (tools/gamecount.py --limit 7). Run from anywhere: bash games/dynomite/tools/profile/perf_all.sh
# The work folder is build/perf (git-ignored); results go to *.json there and the steps to perf_all.log.
#   1. one script for both: the title's link at update 1500 (both have loaded by then), Time Trial on Normal from the main
#      menu, then 150 shots at random points of the board, one every 40 updates: the scripted minute is the 6000 updates
#      from the first shot, the same input in both. toorig.py turns it into the original's script.
#   2. port-load x5, orig-load x5 (the first screen that takes input), then the scripted minute twice for each: A and B.
# Needs identical/setup.sh's staged original (C:/byo-og/dyn) and its patch list.
cd "$(dirname "$0")/../../../.."
P=build/perf
PERF=games/dynomite/tools/profile/perf.py
T=games/dynomite/tools/identical
mkdir -p $P
python $T/mkscript.py $P/port.txt 1500 3 title menu:timetrial:normal wait:300 shots:150:40 quit > /dev/null
python $T/toorig.py $P/port.txt -1 > $P/orig.txt
# the first shot: the first move after the title's click and the menu's five (Let's Play the last)
START=$(awk '$2 == "click" {n++} n == 6 && $2 == "move" {print $1; exit}' $P/port.txt)
SEED=10
room() { until python tools/gamecount.py --limit 7 > /dev/null; do sleep 20; done; }
log() { echo "$(date +%H:%M:%S) $*" >> $P/perf_all.log; }
rm -f $P/perf_all.done $P/perf_all.log
log "minute from update $START"
room; log port-load; build/native/rundesk.exe python $PERF port-load $P/pl 5 > $P/port-load.json 2>>$P/perf_all.log
room; log orig-load; python $PERF orig $P/ol $P/orig.txt $SEED $START 5 load > $P/orig-load.json 2>>$P/perf_all.log
for r in A B; do
  room; log orig-minute-$r; python $PERF orig $P/om$r $P/orig.txt $SEED $START 1 > $P/orig-minute-$r.json 2>>$P/perf_all.log
  room; log port-minute-$r; build/native/rundesk.exe python $PERF port-minute $P/pm$r $P/port.txt $SEED $START \
    > $P/port-minute-$r.json 2>>$P/perf_all.log
done
log done; touch $P/perf_all.done
