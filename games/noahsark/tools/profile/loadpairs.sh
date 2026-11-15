#!/bin/bash
# Load time of the port before and after the performance pass, one run of each in turn (before, after, before, ...) so that the machine's
# drift falls on both: bash games/noahsark/tools/profile/loadpairs.sh [pairs]. Waits for room before each run. Lines go to
# build/perf2/res/loadpairs.txt: "<before|after> <load_s> <private MB at the title> <processor s at the title> <machine busy>".
cd "$(dirname "$0")/../../../.."
W=$(pwd -W)
R=build/perf2/res; mkdir -p $R
PERF=games/noahsark/tools/profile/perf.py
N=${1:-12}
rm -f $R/loadpairs.txt $R/loadpairs.done
for i in $(seq 1 $N); do
  for v in before after; do
    until python -I tools/gamecount.py --limit 6 > /dev/null; do sleep 10; done
    if [ $v = before ]; then E="$W/build/perf2/before/noahsark.exe"; else E="$W/build/native/noahsark.exe"; fi
    PORT_EXE=$E build/native/rundesk.exe python $PERF port-load $R/lp_tmp 1 2>/dev/null | python -I -c "
import sys, json
for l in sys.stdin:
    if l.startswith('{') and 'load_s' in l:
        j = json.loads(l); print('$v', j['load_s'], j['private_mb_at_title'], j['processor_s_at_title'], j['machine_busy'])
" >> $R/loadpairs.txt
  done
done
touch $R/loadpairs.done
