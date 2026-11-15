#!/usr/bin/env bash
# The port's side of the performance runs (see runperf.sh), alone: absolute paths, as rundesk's private desktop needs.
#   bash games/sevenseas/tools/profile/runport.sh <rounds>
ROOT="$(pwd -W)"; P=games/sevenseas/tools/profile; PY=C:/Python312/python.exe
for i in $(seq 1 "${1:-3}"); do
  while ! $PY tools/gamecount.py --limit 6 >/dev/null 2>&1; do sleep 20; done
  $PY tools/gamecount.py --limit 99 2>&1 | head -1 > build/perf/procs_p$i.txt
  rm -rf build/perf/ud$i
  build/native/rundesk.exe $PY "$ROOT/$P/perfmin.py" "$ROOT/build/perf/port$i.json" 20,80 -- "$ROOT/build/native/sevenseas.exe" "-userdir=$ROOT/build/perf/ud$i" "-script=$ROOT/$P/perf_port.txt"
  $PY tools/gamecount.py --limit 99 2>&1 | head -1 >> build/perf/procs_p$i.txt
done
echo finished
