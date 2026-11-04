#!/bin/bash
# cmp.sh <tour stem>: shotdiff of build/ph2/<tour>/orig against .../port, then one line per differing pair (boxes: x y w h).
source "$(dirname "$0")/../identical/common.sh"
N=$1; shift
python tools/shotdiff/shotdiff.py --game alchemy --orig build/ph2/$N/orig --port build/ph2/$N/port --run "$N" "$@" | tail -1
python - "$N" <<'PY'
import json, sys
s = json.load(open('build/shotreview/alchemy/%s/summary.json' % sys.argv[1]))
for p in s['pairs']:
    if p['status'] != 'match':
        print(p['name'], p['status'], [(b['x'], b['y'], b['w'], b['h']) for b in p.get('boxes', [])][:5])
for it in s.get('items', []):
    print('item', it['name'], it['count'])
PY
