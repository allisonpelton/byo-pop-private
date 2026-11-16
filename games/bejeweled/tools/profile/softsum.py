# Sums softpairs.sh's runs: for each configuration the peak private bytes (the run's, which is the figure the original's 36 MB
# is) and the processor time (the process's cycles in the window 20 s to 80 s), as medians with ranges, and the cycles of each
# run against the original's of the same round. A run's "others" is the most game processes seen on the machine during it, less
# its own (<run>.counts).
#
#   python -I softsum.py <out dir> [--all]        --all lists every run
import ctypes, glob, json, os, re, statistics, sys, time

def counter_rate():
    """The rate of the cycle counter QueryProcessCycleTime reads, in cycles a second, measured by a spin of a quarter second."""
    k32 = ctypes.WinDLL('kernel32')
    k32.GetCurrentProcess.restype = ctypes.c_void_p
    k32.QueryProcessCycleTime.argtypes = [ctypes.c_void_p, ctypes.POINTER(ctypes.c_ulonglong)]
    me, a, b = k32.GetCurrentProcess(), ctypes.c_ulonglong(), ctypes.c_ulonglong()
    k32.QueryProcessCycleTime(me, ctypes.byref(a))
    t0 = time.perf_counter()
    while time.perf_counter() - t0 < 0.25:
        pass
    k32.QueryProcessCycleTime(me, ctypes.byref(b))
    return (b.value - a.value) / (time.perf_counter() - t0)


out = sys.argv[1]
HZ = counter_rate()
print('cycle counter rate: %.2f GHz' % (HZ / 1e9))
rows = {}
for path in sorted(glob.glob(os.path.join(out, '*.json'))):
    name = os.path.basename(path)[:-5]
    m = re.match(r'([a-z]+\d?)-?(\d+)$', name)
    if not m:
        continue
    r = json.load(open(path))
    counts = os.path.join(out, name + '.counts')
    others = 0
    if os.path.exists(counts):
        vals = [int(x) for x in open(counts).read().split() if x.isdigit()]
        others = (max(vals) - 1) if vals else 0
    rows.setdefault(m.group(1), {})[int(m.group(2))] = {
        'peak': r.get('peak private MB'), 'wpeak': r.get('window peak private MB'), 'rate': r.get('window M cycles a second'),
        'whole': r.get('G cycles, whole run'), 'secs': r.get('run seconds'), 'others': others}


def span(vals):
    vals = [v for v in vals if v is not None]
    if not vals:
        return '-'
    return '%.1f (%.1f to %.1f)' % (statistics.median(vals), min(vals), max(vals))


def percent(vals):
    vals = [v for v in vals if v is not None]
    return '%.1f %%' % (100 * statistics.median(vals) * 1e6 / HZ) if vals else '-'


print('%-9s %2s  %-24s %-24s %-8s %-22s %s' % ('config', 'n', 'peak private MB', 'window M cycles/s', 'of a core', 'against orig, paired', 'others (max over runs)'))
orig = rows.get('orig', {})
for cfg in sorted(rows, key=lambda c: (c != 'orig', c)):
    runs = rows[cfg]
    ratios = [runs[k]['rate'] / orig[k]['rate'] for k in runs if k in orig and runs[k]['rate'] and orig[k]['rate']]
    pr = ('%.2f (%.2f to %.2f)' % (statistics.median(ratios), min(ratios), max(ratios))) if ratios and cfg != 'orig' else '-'
    print('%-9s %2d  %-24s %-24s %-8s %-22s %d' % (cfg, len(runs), span([v['peak'] for v in runs.values()]),
          span([v['rate'] for v in runs.values()]), percent([v['rate'] for v in runs.values()]), pr,
          max(v['others'] for v in runs.values())))
if '--all' in sys.argv:
    print()
    for cfg in sorted(rows, key=lambda c: (c != 'orig', c)):
        for k in sorted(rows[cfg]):
            v = rows[cfg][k]
            print('%s-%d: peak %s MB, window peak %s MB, %s M cycles/s, whole %s G cycles in %s s, others %d' % (
                cfg, k, v['peak'], v['wpeak'], v['rate'], v['whole'], v['secs'], v['others']))
