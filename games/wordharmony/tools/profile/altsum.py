"""The summary of alt.sh's runs: for each run of the original and of the port (and of portold, the port with SURFACE_DISPLAY=off), the
cycles of the scripted minute (billions: the threads' QueryThreadCycleTime, perf.py's cycles_in_minute_g) by thread, then the medians
and each port side's cycles against the original's. The port's SDL audio thread (its software mixer, which the original's stand-in
DirectSound has no counterpart of) is shown apart, and the comparison is given with and without it.

    python -I games/wordharmony/tools/profile/altsum.py <alt.sh's out dir>
"""
import glob
import json
import os
import statistics
import sys


def load(path):
    with open(path) as f:
        lines = [l for l in f.read().splitlines() if l.startswith('{')]
    return json.loads(lines[-1]) if lines else None


def split(r):
    """(total, the audio thread, the graphics driver's threads, the game's main thread) in billions of cycles"""
    total = r['cycles_in_minute_g']
    audio = sum(t['g'] for t in r['threads_in_minute'] if 'SDL' in t['start'] or 'Audio' in t.get('name', ''))
    main = r['threads_in_minute'][0]['g'] if r['threads_in_minute'] else 0
    driver = total - audio - main   # the graphics driver's threads (nvwgf2um.dll here), the port's only other ones
    return total, audio, driver, main


def main():
    d = sys.argv[1]
    sides = ['orig', 'port', 'portold', 'portrs1']
    rows = {}
    for side in sides:
        rows[side] = []
        for path in sorted(glob.glob(os.path.join(d, side + '[0-9]*.json'))):
            r = load(path)
            if r is None or 'cycles_in_minute_g' not in r:
                continue
            total, audio, driver, mn = split(r)
            rows[side].append((total, audio, driver, mn, r['machine_busy'], os.path.basename(path)))
            print('%-12s total %6.3f G   main %6.3f   driver %6.3f   audio %5.3f   busy %.2f' % (
                os.path.basename(path), total, mn, driver, audio, r['machine_busy']))
    for side in sides:
        if rows[side]:
            print('%-7s median total %6.3f G (%d runs)   main %6.3f   driver %6.3f   audio %5.3f   without the audio thread %6.3f G' % (
                side, statistics.median(x[0] for x in rows[side]), len(rows[side]), statistics.median(x[3] for x in rows[side]),
                statistics.median(x[2] for x in rows[side]), statistics.median(x[1] for x in rows[side]),
                statistics.median(x[0] - x[1] for x in rows[side])))
    if rows['orig']:
        o = statistics.median(x[0] for x in rows['orig'])
        for side in sides[1:]:
            if rows[side]:
                p = statistics.median(x[0] for x in rows[side])
                pa = statistics.median(x[0] - x[1] for x in rows[side])
                print('%s / original: %.2f   (without the audio thread: %.2f)' % (side, p / o, pa / o))


if __name__ == '__main__':
    main()
