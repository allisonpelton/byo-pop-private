#!/bin/bash
# web.sh <scenario.txt> <run>: the scenario's pictures from the web build (build/web/atomica.js under Node, games/bejeweled/tools/verify/wasmrun.js)
# into build/shots-atomica/<run>/web, compared by shotdiff with the native port's (shots.py <scenario> --run <run> --port-only first).
# Needs build/web (TARGET=atomica tools/build.sh web). A sprite drawn rotated or scaled may come out about a pixel off in the web build (a shared item).
cd "$(dirname "$0")/../../../.."
SC=$1; N=$2; R="$(pwd -W)"; D="$R/build/shots-atomica/$N"; W="$R/build/webrun-atomica/$N"
NODE=$(ls /e/retools/emsdk/node/*/node.exe | head -1)
rm -rf "$W" "$D/web"; mkdir -p "$W/shots" "$W/user"
python - "$SC" "$W/script.txt" <<'PY'
import sys
out = []
end = 0
for l in open(sys.argv[1]):
    w = l.split()
    if not w or l.startswith('#'): continue
    if w[0] == 'end': end = int(w[1]); continue
    if w[1] == 'shot': out.append('%d shot shots/%s.png' % (int(w[0]) + 1, w[2])); continue
    if w[1] == 'text':
        for ch in ' '.join(w[2:]): out.append('%d char %s' % (int(w[0]) + 1, 'SPACE' if ch == ' ' else ch))
        continue
    out.append('%d %s' % (int(w[0]) + 1, ' '.join(w[1:])))
out.append('%d quit' % (end + 1))
open(sys.argv[2], 'w', newline='\n').write('\n'.join(out) + '\n')
PY
( cd "$W" && "$R/build/native/rundesk.exe" "$NODE" "$R/games/bejeweled/tools/verify/wasmrun.js" game --timeout 900 "$R/build/web/atomica.js" \
	-gamedir="$R/assets/atomica" -userdir=user -fast -seed=1 -drawevery -script=script.txt > out.txt 2>&1 )
mv "$W/shots" "$D/web"; ls "$D/web" | wc -l
python tools/shotdiff/shotdiff.py --game atomica --orig "build/shots-atomica/$N/port" --port "build/shots-atomica/$N/web" --run "web-$N" --allow 0 --config build/shotreview/empty.json | tail -1
