# pairs.py <summary.json>: one line per shotdiff pair that is not a match, with its first boxes (x y w h)
import json, sys
s = json.load(open(sys.argv[1]))
for p in s['pairs']:
    if p['status'] != 'match':
        print(p['name'], p['status'], p.get('percent'), [(b['x'], b['y'], b['w'], b['h']) for b in p.get('boxes', [])][:6])
