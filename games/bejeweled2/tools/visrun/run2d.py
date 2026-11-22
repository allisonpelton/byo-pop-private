"""The original's 2D tour on the visible desktop (docs/VISRUN.md, docs/RULES.md "The visible desktop as a second lane"):

    python -I games/bejeweled2/tools/visrun/run2d.py <recording dir> <out dir> [--profile <folder of make_profile.py / make_gapprofile.py>]
        [--slot 0-3] [--dry-run]

The recording's events.json becomes origrun's script (to_origrun.py), which visrun.py takes as its step file; the profile's
files are seeded as the original's players; the window goes to the right half of the screen (x 1920 and more, one of four
places of 660 by 520, --slot) with the virtual clock and Hardware Acceleration off; the shots are shaped as the sandbox's
(--shape sandbox), so one shotdiff config reads both. Writes <out dir>/*.png and visrun's logs."""
import argparse
import os
import subprocess
import sys

bs = chr(92)
ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))
PLACES = [(1920, 20), (2600, 20), (1920, 560), (2600, 560)]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('rec')
    ap.add_argument('out')
    ap.add_argument('--profile')
    ap.add_argument('--slot', type=int, default=0)
    ap.add_argument('--seed', type=int, default=909)
    ap.add_argument('--dry-run', action='store_true')
    a = ap.parse_args()
    out = os.path.abspath(a.out)
    os.makedirs(out, exist_ok=True)
    steps = os.path.join(out, 'steps.txt')
    subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'games/bejeweled2/tools/origcmp/to_origrun.py'),
                    os.path.join(a.rec, 'events.json'), steps], check=True, stdout=subprocess.DEVNULL)
    # visrun's `mark` takes no time of its own: `at N peek` first, then the bare mark
    lines = []
    first = next(l.split()[1] for l in open(steps).read().splitlines() if l.startswith('at '))
    lines.append('gate loaded %s 120' % first)  # the title's loading runs on the wall clock
    for line in open(steps).read().splitlines():
        t = line.split()
        if len(t) == 3 and t[0] == 'at' and t[2] == 'mark':
            lines += ['at %s peek' % t[1], 'mark']
        else:
            lines.append(line)
    open(steps, 'w').write(chr(10).join(lines) + chr(10))
    seed = ''
    if a.profile:
        files = []
        base = os.path.abspath(os.path.join(a.profile, 'users'))
        for root, _, names in os.walk(base):
            for n in names:
                src = os.path.join(root, n)
                dst = 'C:' + bs + 'ProgramData' + bs + 'Steam' + bs + 'Bejeweled2' + bs + 'users' + bs + os.path.relpath(src, base).replace('/', bs)
                files.append('["%s", "%s"]' % (src.replace(bs, '/'), dst.replace(bs, bs + bs)))
        seed = 'save_seed = [%s]' % ', '.join(files)
    cfg = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'bejeweled2-2d.toml')).read()
    x, y = PLACES[a.slot]
    cfg = cfg.replace('pos = [1920, 20]', 'pos = [%d, %d]' % (x, y)).replace('@SAVE_SEED@', seed)
    cfgpath = os.path.join(out, 'run.toml')
    open(cfgpath, 'w').write(cfg)
    cmd = [sys.executable, '-I', os.path.join(ROOT, 'tools/visrun.py'), cfgpath, steps, '--out', out, '--3d', 'off', '--seed', str(a.seed),
           '--clock', 'virtual', '--shape', 'sandbox', '--timeout', '600'] + (['--keep-copy'] if os.environ.get('KEEP_COPY') else []) + (['--dry-run'] if a.dry_run else [])
    return subprocess.run(cmd, cwd=ROOT).returncode


if __name__ == '__main__':
    sys.exit(main())
