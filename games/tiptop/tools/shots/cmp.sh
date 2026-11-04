#!/bin/bash
# cmp.sh <tour> [shotdiff arguments]: shotdiff of build/ph2/<tour>/orig against .../port, then one line per pair that
# is not a match (boxes: x y w h).
source "$(dirname "$0")/../identical/common.sh"
N=$1; shift
python tools/shotdiff/shotdiff.py --game tiptop --orig build/ph2/$N/orig --port build/ph2/$N/port --run "$N" "$@" | tail -1
python - "$N" <<'PY'
import json, sys
s = json.load(open('build/shotreview/tiptop/%s/summary.json' % sys.argv[1]))
for p in s['pairs']:
    if p['status'] != 'match':
        print(p['name'], p['status'], p.get('percent'), [(b['x'], b['y'], b['w'], b['h']) for b in p.get('boxes', [])][:6])
PY
