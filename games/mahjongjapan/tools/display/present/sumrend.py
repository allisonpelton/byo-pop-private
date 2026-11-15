import json, glob, os, statistics, sys, re
d = sys.argv[1]
names = {1: "framework's chain, window 800x600", 10: "framework's chain, window 1600x1200 (screen texture 1600x1200)", 11: '15% rect uploaded, window 1600x1200'}
for drv in ('direct3d11', 'direct3d'):
    for m in (1, 10, 11):
        v = []; got = set()
        for f in sorted(glob.glob(os.path.join(d, '%s.m%d.r*.json' % (drv, m)))):
            r = json.load(open(f)); v.append(r['threads cycles G'])
            mods = ' '.join(r.get('modules', [])).lower()
            got.add('d3d11 umd' if 'nvwgf2um' in mods or 'd3d11' in mods else ('d3d9 umd' if 'nvd3dum' in mods or 'd3d9' in mods else '?'))
        if v:
            print('%-11s %-62s %.3f G per 15 s (%s, n=%d) got %s' % (drv, names[m], statistics.median(v), ' '.join('%.2f' % x for x in v), len(v), ','.join(sorted(got))))
