#!/usr/bin/env python3
"""combine.py <out dir> [part ...]: the pilot's captures in one pair of folders for one shotdiff run, as
docs/SCREENS.md wants them: <out>/orig and <out>/port, the 2D captures (build/scratch/pilot/{orig,port}) by their own
names and the 3D ones ({orig3d,port3d}) with "_3d" added (a still x.png becomes x_3d.png, a clip's folder x becomes
x_3d; --2d leaves the 3D ones out). Only the shots and clips the parts' step lists name are taken (every part in tools/pilot when none is given),
and a name missing on either side is listed instead of copied. Then:

    python tools/shotdiff/shotdiff.py --game bookwormadv --orig <out>/orig --port <out>/port --run <id>
"""
import os
import re
import shutil
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..', '..', '..'))
P = os.path.join(ROOT, 'build', 'scratch', 'pilot')
NOT_PARTS = {'gen', 'inireg', 'lag', 'loads', 'reset', 'run', 'seedpatch', 'soundcmp', 'visrun', 'vis3d', 'addshots',
             'combine'}


def names(part):
    out = []
    for line in open(os.path.join(HERE, part + '.txt'), encoding='utf-8'):
        m = re.match(r'(shot|clip|title)\s+(\S+)', line)
        if m:
            # "title <ms> <step>" is the startup's clip, named title
            out.append('title' if m.group(1) == 'title' else m.group(2))
    return out


def find(folder, name):
    for cand in (name + '.png', name):
        path = os.path.join(folder, cand)
        if os.path.exists(path):
            return cand
    return None


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    only2d = '--2d' in sys.argv
    args = [a for a in sys.argv[1:] if a != '--2d']
    out = args[0]
    parts = args[1:] or sorted(f[:-4] for f in os.listdir(HERE) if f.endswith('.txt'))
    for side in ('orig', 'port'):
        shutil.rmtree(os.path.join(out, side), ignore_errors=True)
        os.makedirs(os.path.join(out, side))
    taken, missing = 0, []
    for part in parts:
        for name in names(part):
            for suf in ('',) if only2d else ('', '3d'):
                got = [find(os.path.join(P, side + suf), name) for side in ('orig', 'port')]
                if not all(got):
                    if suf == '' or any(got):
                        missing.append('%s%s (%s: %s)' % (name, '_3d' if suf else '', part,
                                                         'no original' if not got[0] else 'no port'))
                    continue
                for side, cand in zip(('orig', 'port'), got):
                    src = os.path.join(P, side + suf, cand)
                    dst_name = cand[:-4] + '_3d.png' if suf and cand.endswith('.png') else cand + ('_3d' if suf else '')
                    dst = os.path.join(out, side, dst_name)
                    if os.path.isdir(src):
                        shutil.copytree(src, dst)
                    else:
                        shutil.copyfile(src, dst)
                taken += 1
    print('%d pairs in %s' % (taken, out))
    for m in missing:
        print('missing: ' + m)
    return 0


if __name__ == '__main__':
    sys.exit(main())
