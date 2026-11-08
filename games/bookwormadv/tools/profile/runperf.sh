#!/usr/bin/env bash
# The performance runs for README "Performance": the original (origrun on the host, real time, silent) and the port
# (natively under rundesk, real time, silent), back to back, <rounds> rounds. Each run is 90 s from launch: the loading
# screen, then the title with a saved player, idle (no input), sampled every 50 ms by Bejeweled's perfmin.py (working set,
# private bytes, processor time, threads), the processor figures over 20 to 80 s. The original ends at origrun's timeout,
# the port at a quit on its live channel. Run from the worktree root:
#   bash games/bookwormadv/tools/profile/runperf.sh <rounds> <a port user folder with a player> [port]   (port: the port only)
ROOT="$(pwd)"; PERF=games/bejeweled/tools/profile/perfmin.py; PY=python
MAIN=/c/Users/Administrator/Documents/claude/byo-pop
ORIGRUN=$MAIN/build/origrun-bin/origrun.exe
PROFILE="$2"
mkdir -p build/perf
printf 'wait 95000\n' > build/perf/orig_idle.txt
for i in $(seq 1 "${1:-3}"); do
  if [ "$3" != port ]; then
  while ! $PY tools/gamecount.py --limit 2 >/dev/null 2>&1; do sleep 20; done
  $PY tools/gamecount.py --limit 99 2>&1 | tail -1 > build/perf/procs_o$i.txt
  $ORIGRUN --game bookwormadv --run pfo$i --stage-root 'C:\byo-og' --timeout 90 --script build/perf/orig_idle.txt > build/perf/orig$i.log 2>&1 &
  $PY $PERF build/perf/orig$i.json 20,80 --attach "byo-og/pfo$i/game/bookwormadventures.exe"
  wait
  $PY tools/gamecount.py --limit 99 2>&1 | tail -1 >> build/perf/procs_o$i.txt
  fi
  while ! $PY tools/gamecount.py --limit 2 >/dev/null 2>&1; do sleep 20; done
  $PY tools/gamecount.py --limit 99 2>&1 | tail -1 > build/perf/procs_p$i.txt
  rm -rf build/perf/ud$i; cp -r "$PROFILE" build/perf/ud$i
  : > build/perf/cmd$i.txt
  ( sleep 90; echo quit >> build/perf/cmd$i.txt ) &
  build/native/rundesk.exe $PY $PERF build/perf/port$i.json 20,80 -- "$(cygpath -w "$ROOT/build/native/bookwormadv.exe")" -userdir=build/perf/ud$i -commands=build/perf/cmd$i.txt
  wait
  $PY tools/gamecount.py --limit 99 2>&1 | tail -1 >> build/perf/procs_p$i.txt
done
echo finished
