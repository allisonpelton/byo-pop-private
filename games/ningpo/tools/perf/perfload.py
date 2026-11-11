"""perfload.py <perfmin json>...: the numbers of tools/perf/perf.sh's runs, the original's (orig-*.json) and the port's
(port-*.json), as medians with their ranges. The loading time is read from the samples the same way for both: Ning Po
loads everything before its loading screen takes a click, and then waits for events (PGApp::mainLoop), so the load is
over at the first sample after which the process uses less than a tenth of a core for a second (the processor
time Windows keeps moves in steps of 15.6 ms, too coarse for a shorter span)."""
import json
import statistics
import sys


def loaded(samples):
    for i, s in enumerate(samples):
        later = [t for t in samples[i:] if t[0] <= s[0] + 1.0]
        if s[0] > 0.1 and later[-1][0] - s[0] >= 0.9 and later[-1][3] - s[3] < 0.1:
            return s[0]
    return None


def fmt(v, unit=''):
    v = [x for x in v if x is not None]
    if not v:
        return 'none'
    return '%.2f%s median (%.2f to %.2f)' % (statistics.median(v), unit, min(v), max(v))


def main():
    runs = {'orig': [], 'port': []}
    for p in sys.argv[1:]:
        r = json.load(open(p))
        kind = 'orig' if 'orig-' in p.replace('\\', '/').split('/')[-1] else 'port'
        r['loaded'] = loaded(r['samples'])
        r['threads'] = sorted({s[4] for s in r['samples']})
        runs[kind].append(r)
    for kind, rs in runs.items():
        if not rs:
            continue
        print('%s, %d runs:' % (kind, len(rs)))
        print('  loaded after   ', fmt([r['loaded'] for r in rs], ' s'))
        print('  peak working set', fmt([r.get('peak working set MB') for r in rs], ' MB'))
        print('  peak private   ', fmt([r.get('peak private MB') for r in rs], ' MB'))
        print('  window cpu     ', fmt([r.get('window cpu percent of one core') for r in rs], ' %'),
              'of', fmt([r.get('window seconds') for r in rs], ' s'))
        print('  cpu, whole run ', fmt([r.get('cpu seconds, whole run') for r in rs], ' s'),
              'of', fmt([r.get('run seconds') for r in rs], ' s'))
        print('  threads         %d to %d' % (min(min(r['threads']) for r in rs), max(max(r['threads']) for r in rs)))


if __name__ == '__main__':
    main()
