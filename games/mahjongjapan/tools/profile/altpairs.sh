#!/bin/bash
# altpairs.sh <pairs> <out>: for each pair, Ancient Japan's port (under rundesk, real time) then its original (origrun), then
# Ancient China's port and original; threadprof.py reads each thread's cycles (QueryThreadCycleTime). Run under tools/pincore.py (python -I tools/pincore.py -- bash games/mahjongjapan/tools/profile/altpairs.sh 5 <out>), with WINDOWED=1 for the window the original has (and EXTRA="-windowscale=2" for the 2x start a player gets); showmedians.py <out> prints the medians.
cd "$(git rev-parse --show-toplevel)" || exit 1
N="$1"; OUT="$2"; mkdir -p "$OUT"
export TOOL=games/mahjongjapan/tools/profile/threadprof.py
for i in $(seq 1 "$N"); do
  for G in mahjongjapan mahjongchina; do
    GAME=$G build/native/rundesk.exe bash games/mahjongjapan/tools/profile/perfrun.sh port 1 $OUT/$G/p$i > $OUT.$G.p$i.log 2>&1
    GAME=$G bash games/mahjongjapan/tools/profile/perfrun.sh orig 1 $OUT/$G/o$i > $OUT.$G.o$i.log 2>&1
  done
done
echo done > $OUT.done
