"""fastcycles.py <rounds> <label=exe>... : the processor cycles the port's tour takes under -fast, build against build.

A real-time minute of Bonnie's differs by 30 % between two runs of the same exe on a busy machine (perf.py), which hides a
change of a few percent. This runs the port's whole tour (tools/tour_test.txt without its screenshots, -fast -seed=1, the
virtual clock: no waiting) so that the work is the same every run, and reads the cycles the process ran
(QueryProcessCycleTime at its end) and, from the last snapshot before the end, each thread's. By default the builds take
turns, a round each, and the minimum and median of the rounds are what to compare: a busy machine only adds cycles. With
TOGETHER=1 every build of a round runs at the same time instead, so that they meet the same load, and the per-round ratio
of two builds is printed (the same build against itself gave 0.996, from 0.989 to 1.003, so a change of 2 % shows).

    TOGETHER=1 build/native/rundesk.exe python games/bonnies/tools/fastcycles.py 8 before=build/perf/before/bonnies.exe after=build/perf/after1/bonnies.exe

Each run waits for a free game slot (tools/gamecount.py), runs from the repository's root (assets/bonnies) and deletes its
folder (build/perf/fc/<label><round>) once its line is printed. Run it under rundesk.
"""
import json
import os
import shutil
import statistics
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import perf  # noqa: E402  (the Windows calls and the thread snapshots are perf.py's)

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..'))
TOUR = os.path.join(ROOT, 'games', 'bonnies', 'tools', 'tour_test.txt')


def wait_for_slots(count):
    """Waits until `count` more game processes fit under the machine's 6 (tools/gamecount.py exits 0 below its limit)."""
    while subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'tools', 'gamecount.py'), '--limit', str(7 - count)],
                         stdout=subprocess.DEVNULL, cwd=ROOT).returncode != 0:
        time.sleep(20)


def run(group, number):
    """Runs the builds of `group` [(label, exe)] at the same time, so that they meet the same load, and returns their rows."""
    wait_for_slots(len(group))
    t0 = time.time()
    runs = []
    for label, exe in group:
        d = os.path.join(ROOT, 'build', 'perf', 'fc', '%s%d' % (label, number))
        shutil.rmtree(d, ignore_errors=True)
        os.makedirs(os.path.join(d, 'user'))
        script = perf.strip(TOUR, os.path.join(d, 'script.txt'), {'screenshot', 'shot'}, [('@DIR@', d)])
        p = subprocess.Popen([exe, '-gamedir=assets/bonnies', '-userdir=' + os.path.join(d, 'user'), '-fast', '-seed=1',
                              '-script=' + script], cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        runs.append({'label': label, 'dir': d, 'p': p, 'h': int(p._handle), 'mods': [], 'last': {}, 'done': None})
    while any(r['done'] is None for r in runs):
        time.sleep(0.5)
        for r in runs:
            if r['done'] is not None:
                continue
            if r['p'].poll() is not None:
                cycles = perf.ctypes.c_uint64()
                perf.k32.QueryProcessCycleTime(r['h'], perf.ctypes.byref(cycles))
                r['done'] = (cycles.value / 1e9, time.time() - t0)
                continue
            if not r['mods'] and time.time() - t0 > 6:
                r['mods'] = perf.modules(r['h'])
            snap = perf.threads(r['p'].pid)
            if snap:
                r['last'] = snap
    rows = []
    for r in runs:
        agg = {}
        for tid, (c, start) in r['last'].items():
            name = next((n for b, s, n in r['mods'] if b <= start < b + s), '?')
            agg[name] = agg.get(name, 0) + c / 1e9
        shutil.rmtree(r['dir'], ignore_errors=True)
        rows.append({'label': r['label'], 'round': number, 'exit': r['p'].returncode, 'gcycles': round(r['done'][0], 3),
                     'wall_s': round(r['done'][1], 1),
                     'threads': {k: round(v, 3) for k, v in sorted(agg.items(), key=lambda x: -x[1]) if v >= 0.02}})
    return rows


def main():
    """Alone: one build at a time, rounds alternating the builds. Together (TOGETHER=1): all the builds at once in a round,
    which cancels the busy machine's changes from one minute to the next; read each round's ratios."""
    rounds = int(sys.argv[1])
    builds = [(a.split('=', 1)[0], os.path.abspath(a.split('=', 1)[1])) for a in sys.argv[2:]]
    together = os.environ.get('TOGETHER') == '1'
    rows = []
    for number in range(1, rounds + 1):
        for group in ([builds] if together else [[b] for b in builds]):
            for r in run(group, number):
                rows.append(r)
                print(json.dumps(r), flush=True)
    print()
    for label, _ in builds:
        g = [r['gcycles'] for r in rows if r['label'] == label and r['exit'] == 0]
        m = [r['threads'].get('bonnies.exe', 0) for r in rows if r['label'] == label and r['exit'] == 0]
        if g:
            print('%-10s total G cycles: min %.2f median %.2f max %.2f (%s); main thread: min %.2f median %.2f' % (
                label, min(g), statistics.median(g), max(g), ' '.join('%.2f' % x for x in g), min(m), statistics.median(m)))
    if together and len(builds) == 2:
        a, b = builds[0][0], builds[1][0]
        for what, get in (('total', lambda r: r['gcycles']), ('main thread', lambda r: r['threads'].get('bonnies.exe', 0))):
            ratios = []
            for number in range(1, rounds + 1):
                ra = next((r for r in rows if r['label'] == a and r['round'] == number and r['exit'] == 0), None)
                rb = next((r for r in rows if r['label'] == b and r['round'] == number and r['exit'] == 0), None)
                if ra and rb and get(ra):
                    ratios.append(get(rb) / get(ra))
            if ratios:
                print('%s / %s, %s, by round: %s; median %.3f' % (b, a, what, ' '.join('%.3f' % x for x in ratios),
                                                                    statistics.median(ratios)))


main()
