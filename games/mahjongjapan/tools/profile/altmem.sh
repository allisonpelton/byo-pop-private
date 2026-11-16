#!/bin/bash
# altmem.sh <rounds> <out>: the memory of the same runs as altconfigs.sh (the scripted play of tools/check.sh, 60 s), one run each of the
# original (o), the port's 1x window (p1), its 2x window (p2) and its full screen (pf) in turn, sampled by perfmin.py every 50 ms (the
# whole run's peak private bytes and peak working set). showmem.py <out> prints the medians. CONFIGS="o p1" picks the runs.
cd "$(git rev-parse --show-toplevel)" || exit 1
N="$1"; OUT="$2"; mkdir -p "$OUT"
unset TOOL
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
