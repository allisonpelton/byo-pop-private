# sd.sh <case dir> <run id>: shotdiff and a one-line-per-shot summary
cd /c/Users/Administrator/Documents/claude/byo-pop/.claude/worktrees/astropop-c6c8fd
python tools/shotdiff/shotdiff.py --game pixelus --orig build/origrun/$2/shots --port $1/shots-port --run $2 2>&1 | tail -1
python - "$2" <<'PY'
import json,sys
d=json.load(open('build/shotreview/pixelus/%s/summary.json'%sys.argv[1]))
items=d if isinstance(d,list) else d.get('pairs',d.get('items'))
for i in sorted(items,key=lambda i:int(i['name'][1:-4])): print(i['name'],i['diff_pct'],[(b['x'],b['y'],b['w'],b['h']) for b in i['boxes']])
PY
