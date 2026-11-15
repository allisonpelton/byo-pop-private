# Sums a probe file (engines/jollybear/PerfProbe.h's, a temporary measuring aid) over a window of the process's age:
#   python probesum.py <probe file> <from s>,<to s> [names]
import sys
a, b = (float(x) for x in sys.argv[2].split(','))
names = sys.argv[3].split(',') if len(sys.argv) > 3 else None
rows = []
for line in open(sys.argv[1]):
    f = line.split()
    d = {'age': float(f[0][4:]), 'total': int(f[1][6:])}
    for x in f[2:]:
        if '=' in x:
            k, v = x.split('=')
            d[k] = int(v)
            continue
        k, v = x.split(':')
        s, c, px = v.split('/')
        d[int(k)] = (int(s), int(c), int(px))
    rows.append(d)
lo = min(rows, key=lambda r: abs(r['age'] - a))
hi = min(rows, key=lambda r: abs(r['age'] - b))
print('window %.1f..%.1f s, main thread total %.3f G cycles' % (lo['age'], hi['age'], (hi['total'] - lo['total']) / 1e9))
for k in sorted(k for k in lo if isinstance(k, int)):
    s = hi[k][0] - lo[k][0]
    c = hi[k][1] - lo[k][1]
    px = hi[k][2] - lo[k][2]
    nm = names[k] if names and k < len(names) else str(k)
    print('  %-14s %7.3f G cycles  %6d calls  %9.0f cycles/call  %8.1f Mpx %6.2f cycles/px' % (nm, s / 1e9, c, s / max(c, 1), px / 1e6, s / max(px, 1) if px else 0))

if 'frames' in lo:
    fr = hi['frames'] - lo['frames']
    print('  frames %d, identical to the last %d, changed pixels %.1f%% of a screen per frame, bounding box %.1f%%' % (
        fr, hi['same'] - lo['same'], 100 * (hi['changedpx'] - lo['changedpx']) / max(fr, 1) / 480000, 100 * (hi['boxpx'] - lo['boxpx']) / max(fr, 1) / 480000))
