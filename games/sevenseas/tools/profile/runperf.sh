#!/usr/bin/env bash
# The performance runs for GAME_NOTES "Performance": the original (origrun, on the host, real time, silent) and the port,
# back to back, 3 rounds, the same scripted input (perf_orig.txt / perf_port.txt: title, menu, the Easy demo, then a
# broadside every 1.5 s). Run from the worktree root:  bash games/sevenseas/tools/profile/runperf.sh <rounds>
ROOT="$(pwd)"; P=games/sevenseas/tools/profile; PY=C:/Python312/python.exe
mkdir -p build/perf
for i in $(seq 1 "${1:-3}"); do
  while ! $PY tools/gamecount.py --limit 7 >/dev/null 2>&1; do sleep 20; done
  $PY tools/gamecount.py --limit 99 2>&1 | head -1 > build/perf/procs_o$i.txt
  rm -rf build/origrun/pfo$i
  build/origrun-bin/origrun.exe --game sevenseas --run pfo$i --seed 7 --script $P/perf_orig.txt > build/perf/orig$i.log 2>&1 &
  $PY $P/perfmin.py build/perf/orig$i.json 20,80 --attach "origrun/pfo$i/game/winss.exe"
  wait
  $PY tools/gamecount.py --limit 99 2>&1 | head -1 >> build/perf/procs_o$i.txt
  while ! $PY tools/gamecount.py --limit 7 >/dev/null 2>&1; do sleep 20; done
  $PY tools/gamecount.py --limit 99 2>&1 | head -1 > build/perf/procs_p$i.txt
  rm -rf build/perf/ud$i
  build/native/rundesk.exe $PY $P/perfmin.py build/perf/port$i.json 20,80 -- build/native/sevenseas.exe -userdir=build/perf/ud$i -script=$P/perf_port.txt
  $PY tools/gamecount.py --limit 99 2>&1 | head -1 >> build/perf/procs_p$i.txt
done
echo finished
