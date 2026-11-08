#!/bin/bash
# The performance pass of GAME_NOTES.md, section 10, one run after another, each started when the machine has room
# (tools/gamecount.py --limit 7). Run from anywhere: bash games/noahsark/tools/profile/perf_all.sh
# The work folder is build/nt/perf (git-ignored); results go to *.json there and the steps to perf_all.log.
#   1. the port's bot plays an action game (easy, seed 1, 7500 updates) once, in -fast, for its click log (bot.log)
#   2. origrun_replay.py turns that log and the menu clicks into the original's script (orig.txt)
#   3. port-load x5, orig-load x5 (the first screen that takes input), then the scripted minute twice for each: A and B
# A minute of the original that did not play (its load_s is over 5 s, a working set that stays near 63 MB) is run again.
cd "$(dirname "$0")/../../../.."
W=$(pwd -W)
P=build/nt/perf
PERF=games/noahsark/tools/profile/perf.py
mkdir -p $P
printf '301 click 320 456\n360 click 198 188\n381 click 320 195\n420 click 400 405\n' > $P/menu.txt   # title link, action, easy, tutorial
room() { until python tools/gamecount.py --limit 7 > /dev/null; do sleep 20; done; }
log() { echo "$(date +%H:%M:%S) $*" >> $P/perf_all.log; }
rm -f $P/perf_all.done $P/perf_all.log
room; log bot
(cat $P/menu.txt; echo "7700 quit") > $P/port0.txt
rm -rf $P/user0
build/native/rundesk.exe "$W/build/native/noahsark.exe" -gamedir="$W/assets/noahsark" -userdir="$W/$P/user0" -fast -seed=1 \
  -script="$W/$P/port0.txt" -autoplay=1 -autoplaymenu -autoplaymode=action -autoplaydiff=0 -autoplaycap=7400 \
  -autoplaylog="$W/$P/bot.log" -statelog="$W/$P/port0.state" > $P/port0.log 2>&1
python -I games/noahsark/tools/origrun_replay.py script $P/menu.txt $P/bot.log $P/orig.txt 7500 >> $P/perf_all.log 2>&1
L=$(python -c "import re; print(max(int(m.group(1)) for l in open('$P/orig.txt') for m in [re.match(r'at (\d+) ', l)] if m))")
log "orig script last update $L"
(cat $P/menu.txt; echo "$L quit") > $P/port.txt
room; log port-load; build/native/rundesk.exe python $PERF port-load $P/pl 5 > $P/port-load.json 2>>$P/perf_all.log
room; log orig-load; python $PERF orig $P/ol $P/orig.txt 5 load > $P/orig-load.json 2>>$P/perf_all.log
for r in A B; do
  room; log orig-minute-$r; python $PERF orig $P/om$r $P/orig.txt 1 > $P/orig-minute-$r.json 2>>$P/perf_all.log
  room; log port-minute-$r; build/native/rundesk.exe python $PERF port-minute $P/pm$r $P/port.txt 1 > $P/port-minute-$r.json 2>>$P/perf_all.log
done
log done; touch $P/perf_all.done
