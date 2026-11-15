# Prints the thread tables of threadprof.py's perf.json files: python showthreads.py <perf.json>...
import json, sys
for p in sys.argv[1:]:
    r = json.load(open(p))
    print('%s: window %.1f s, process %.2f s (%.2f%% of one core), %.3f G cycles' % (
        p, r['wall'], r['process cpu'], r['process percent of one core'], r['threads cycles G']))
    for x in r['threads']:
        if x['cycles'] < 20e6:
            continue
        print('   %6.3f Gcyc %5.2f s  %-26s %s' % (x['cycles'] / 1e9, x['kernel'] + x['user'], x['start'], x['name']))
