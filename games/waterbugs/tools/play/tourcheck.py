# tourcheck.py: the frozen baseline of the screen tours (games/waterbugs/test/tours; docs/SCREENS.md lists what they show).
# The port's pictures are not the same from one run to the next: the menu's bubbles, a loss's fade and the loading thread's
# share of the updates differ in a few pixels or a few updates between two runs of the same script (the same of the original,
# which differs from itself the same way). So the baseline holds only the shots that were whole in every one of several
# reference runs, each by the first 12 hex digits of the sha256 of its pixels, and the check runs the tours once and wants
# all of them unchanged, and every shot of the tours present.
#   tourcheck.py freeze <frozen file> <run dir>...    each run dir holds tour-fresh, tour-end and tour-worlds (shot PNGs)
#   tourcheck.py check <frozen file> <run dir>        prints one line; exit 1 when a frozen shot differs or a shot is missing
import hashlib
import os
import sys

from PIL import Image

TOURS = ('fresh', 'end', 'worlds')


def hashes(run):
    out = {}
    for t in TOURS:
        d = os.path.join(run, 'tour-' + t)
        for fn in sorted(os.listdir(d)):
            if fn.endswith('.png'):
                out['%s/%s' % (t, fn[:-4])] = hashlib.sha256(Image.open(os.path.join(d, fn)).convert('RGB').tobytes()).hexdigest()[:12]
    return out


def main():
    mode, frozen = sys.argv[1], sys.argv[2]
    if mode == 'freeze':
        runs = [hashes(r) for r in sys.argv[3:]]
        names = sorted(set(runs[0]).intersection(*[set(r) for r in runs[1:]]))
        stable = [n for n in names if all(r[n] == runs[0][n] for r in runs)]
        with open(frozen, 'w', newline='\n') as f:
            f.write('# the shots of the screen tours that were the same in %d runs (tools/play/tourcheck.py freeze): name, pixel hash\n' % len(runs))
            f.write('# shots in a run: %d\n' % len(names))
            for n in stable:
                f.write('%s %s\n' % (n, runs[0][n]))
        print('%d runs, %d shots, %d stable' % (len(runs), len(names), len(stable)))
    else:
        want, total = {}, 0
        for line in open(frozen):
            if line.startswith('# shots in a run: '):
                total = int(line.split(':')[1])
            elif not line.startswith('#') and line.strip():
                n, h = line.split()
                want[n] = h
        got = hashes(sys.argv[3])
        bad = [n for n in want if got.get(n) != want[n]]
        if len(got) != total or bad:
            print('TOURS CHANGED (%d shots, wanted %d; differ: %s)' % (len(got), total, ' '.join(bad[:8])))
            sys.exit(1)
        print('%d shots, %d of them frozen, unchanged' % (len(got), len(want)))


main()
