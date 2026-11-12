import json, glob, os, statistics, sys
d = sys.argv[1]
names = {0: 'SDL surface 1:1 (800x600)', 8: 'GDI BitBlt 1:1 (800x600)', 1: 'software 2x nearest into SDL surface (1600x1200)', 2: 'StretchBlt COLORONCOLOR 2x (1600x1200)',
         3: 'StretchBlt HALFTONE 2x (1600x1200)', 4: 'StretchBlt COLORONCOLOR 1.6x (1280x960)', 5: 'StretchBlt HALFTONE 1.6x (1280x960)',
         6: 'StretchBlt COLORONCOLOR 3.6x, 3840x2160 letterboxed', 7: 'StretchBlt HALFTONE 3.6x, 3840x2160 letterboxed', 9: 'software 4x nearest into SDL surface (3200x2400)'}
for m in (0, 8, 1, 2, 3, 4, 5, 6, 7, 9):
    v = []
    for f in sorted(glob.glob(os.path.join(d, 'm%d.r*.json' % m))):
        r = json.load(open(f)); v.append(r['threads cycles G'])
    if v:
        print('%-52s %.3f G per 15 s (%s, n=%d)' % (names[m], statistics.median(v), ' '.join('%.2f' % x for x in v), len(v)))
