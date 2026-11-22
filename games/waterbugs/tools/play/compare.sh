# compare.sh <run id> <tour>...: the shots of the tours' original and port runs (build/origrun/<tour>/shots, build/wbs/<tour>) put
# in one pair of folders (shot names are unique across the tours: gentours.py prefixes them) and compared by tools/shotdiff as
# one run, the way tools/check.sh screens wants it (the newest run holds every shot of SCREENS.md). Prints shotdiff's line and
# the pairs that differ.
cd "$(dirname "$0")/../../../.."
ID=$1; shift
D=build/wbs/cmp-$ID
rm -rf $D; mkdir -p $D/orig $D/port
for n in "$@"; do
	cp build/origrun/$n/shots/*.png $D/orig/ 2>/dev/null
	cp build/wbs/$n/*.png $D/port/ 2>/dev/null
done
python tools/shotdiff/shotdiff.py --game waterbugs --orig $D/orig --port $D/port --run $ID | tail -1
python - "$ID" <<'PYEOF'
import json, sys
d = json.load(open('build/shotreview/waterbugs/%s/summary.json' % sys.argv[1]))
rows = [p for p in d['pairs'] if p['status'] != 'match']
for p in sorted(rows, key=lambda p: p['name']):
    vis = [b for b in p['boxes']]
    print('%-34s %-6s %6.2f%% %d boxes %s' % (p['name'], p['status'], p['diff_pct'], len(vis),
          [(b['x'], b['y'], b['w'], b['h']) for b in vis[:3]]))
print('%d pairs, %d differ; only in the original: %s; only in the port: %s' % (len(d['pairs']), len(rows), d['orig_only'][:5], d['port_only'][:5]))
PYEOF
