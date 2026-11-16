#!/bin/bash
# altconfigs.sh <rounds> <out>: for each round and each game (Ancient Japan, then Ancient China) the original (origrun) and then the
# port in the three sizes a player meets, one run each, back to back: o = the original (windowed, 800x600, as origrun runs it),
# p1 = the port's window at 1x (FullScreen=0, -windowscale=1), p2 = the port's window at 2x (the default window), pf = the port
# full screen at the desktop's size (a fresh player's default; 3840x2160 on rundesk's desktop). threadprof.py reads each thread's
# cycles in the same 30 s (QueryThreadCycleTime). Run it under tools/pincore.py:
#   python -I tools/pincore.py -- bash games/mahjongjapan/tools/profile/altconfigs.sh 5 build/perf/cfg
# showconfigs.py <out> prints the medians. A CONFIGS="o p1" list picks the runs.
cd "$(git rev-parse --show-toplevel)" || exit 1
N="$1"; OUT="$2"; mkdir -p "$OUT"
export TOOL=games/mahjongjapan/tools/profile/threadprof.py
P=games/mahjongjapan/tools/profile/perfrun.sh
for i in $(seq 1 "$N"); do
  for G in mahjongjapan mahjongchina; do
    for C in ${CONFIGS:-o p1 p2 pf}; do
      case "$C" in
        o)  GAME=$G bash $P orig 1 $OUT/$G/o$i > $OUT.$G.o$i.log 2>&1 ;;
        p1) GAME=$G WINDOWED=1 build/native/rundesk.exe bash $P port 1 $OUT/$G/p1_$i > $OUT.$G.p1_$i.log 2>&1 ;;
        p2) GAME=$G WINDOWED=1 EXTRA="-windowscale=2" build/native/rundesk.exe bash $P port 1 $OUT/$G/p2_$i > $OUT.$G.p2_$i.log 2>&1 ;;
        pf) GAME=$G build/native/rundesk.exe bash $P port 1 $OUT/$G/pf_$i > $OUT.$G.pf_$i.log 2>&1 ;;
      esac
    done
  done
done
echo done > $OUT.done
