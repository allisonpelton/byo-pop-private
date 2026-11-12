#!/bin/bash
# best.sh <tour name> [window]: the port on a tour with a shot at every update of each shot's window (the original's frame can be
# older than its label: lagscan.py), the best frame of each picked against the original's, then shotdiff of those: build/ph2/<tour>all/,
# run id <tour>best. The original's shots (orig.sh) must be in build/ph2/<tour>/orig. Waits for room first: at most 6 game processes
# run on the machine (tools/gamecount.py). GS=<game seed>.
source "$(dirname "$0")/../identical/common.sh"
N=$1; W=${2:-1}; D=build/ph2/${N}all
mkdir -p "$D"
python games/rocketmania/tools/shots/lagscan.py games/rocketmania/tools/shots/$N.txt build/ph2/$N/orig "$D" --window $W --ahead 0 --prepare > /dev/null
until python tools/gamecount.py --limit 6 > /dev/null 2>&1; do sleep 20; done
rm -rf "$D/port" "$D/best"; mkdir -p "$D/port"
timeout 1800 "$TOOLS/port.sh" "$D" -seed=${GS:-1} -script="$(win "$D/lag.txt")" | tail -1
python games/rocketmania/tools/shots/lagscan.py games/rocketmania/tools/shots/$N.txt build/ph2/$N/orig "$D" --window $W --ahead 0 --compare --pick "$D/best" > "$D/lag.log"
python tools/shotdiff/shotdiff.py --game rocketmania --orig build/ph2/$N/orig --port "$D/best" --run ${N}best | tail -1
python - "$N" <<'PY'
import json, sys
s = json.load(open('build/shotreview/rocketmania/%sbest/summary.json' % sys.argv[1]))
for p in s['pairs']:
    if p['status'] != 'match':
        print(p['name'], p['status'], [(b['x'], b['y'], b['w'], b['h'], b.get('pixels')) for b in p.get('boxes', [])][:6])
PY
