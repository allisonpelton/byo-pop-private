#!/usr/bin/env python3
"""Identical-play pass: a game saved by the original and by the port. Runs a scenario (that ends by saving and quitting) on
both and compares the two SAVEGAME.DAT files word by word; the original's file is left in C:/byo-og/<run>/game and the
port's in <tmp>/user, for continue.py's runs.

    python games/atomica/tools/identical/savecmp.py <scenario.txt> [--keep <dir>]
"""
import argparse
import os
import shutil
import struct
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.argv, argv = [sys.argv[0]], sys.argv[1:]
src = open(os.path.join(HERE, 'compare.py')).read().replace('\nmain()\n', '\n')
__file__ = os.path.join(HERE, 'compare.py')
exec(src)


def run_original_keep(lines, end, seed, run):
    d = tempfile.mkdtemp(prefix='atomorig')
    script = os.path.join(d, 's.txt')
    with open(script, 'w', newline='\n') as f:
        for n, c, a in lines:
            f.write('at %d %s %s\n' % (n, c, ' '.join(a)))
        f.write('at %d move 1 1\n' % end)
    for value in ('GameHints', 'ShowSymbols', 'ShowTips'):
        subprocess.run(['reg', 'delete', REGKEY, '/v', value, '/f'], capture_output=True)
    shutil.rmtree(os.path.join('C:/byo-og', run), ignore_errors=True)
    shutil.rmtree(os.path.join(ROOT, 'build/origrun', run), ignore_errors=True)
    subprocess.run([os.path.join(ROOT, 'build/origrun-bin/origrun.exe'), '--game', 'atomica', '--run', run, '--stage-root',
                    'C:/byo-og', '--script', script, '--clock', 'virtual', '--cursor', 'scripted', '--seed', str(seed),
                    '--timeout', '200'], cwd=ROOT, capture_output=True, text=True)
    shutil.rmtree(d, ignore_errors=True)
    return os.path.join('C:/byo-og', run, 'game', 'SAVEGAME.DAT')


def run_port_keep(lines, end, seed, ud):
    d = tempfile.mkdtemp(prefix='atomport')
    script = os.path.join(d, 's.txt')
    with open(script, 'w', newline='\n') as f:
        for n, c, a in lines:
            f.write('%d %s %s\n' % (n + 1, c, ' '.join(a)))
        f.write('%d quit\n' % (end + 1))
    shutil.rmtree(ud, ignore_errors=True)
    subprocess.run([BASH, fwd(os.path.join(ROOT, 'games/atomica/tools/run_script.sh')), fwd(script), fwd(ud), '300',
                    '-seed=%d' % seed, '-drawevery'], cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    shutil.rmtree(d, ignore_errors=True)
    return os.path.join(ud, 'SAVEGAME.DAT')


def main2():
    ap = argparse.ArgumentParser()
    ap.add_argument('scenario')
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--keep', default=os.path.join(ROOT, 'build/identical-atomica'))
    a = ap.parse_args(argv)
    lines, end = read_scenario(a.scenario)
    os.makedirs(a.keep, exist_ok=True)
    o = run_original_keep(lines, end, a.seed, 'savecmp')
    p = run_port_keep(lines, end, a.seed, os.path.join(a.keep, 'user'))
    if not (os.path.exists(o) and os.path.exists(p)):
        print('no save file: original %s, port %s' % (os.path.exists(o), os.path.exists(p)))
        sys.exit(1)
    shutil.copy(o, os.path.join(a.keep, 'SAVEGAME.original.DAT'))
    shutil.copy(p, os.path.join(a.keep, 'SAVEGAME.port.DAT'))
    x, y = open(o, 'rb').read(), open(p, 'rb').read()
    diffs = [(4 * i, struct.unpack_from('<i', x, 4 * i)[0], struct.unpack_from('<i', y, 4 * i)[0])
             for i in range(min(len(x), len(y)) // 4) if x[4 * i:4 * i + 4] != y[4 * i:4 * i + 4]]
    print('original %d bytes, port %d bytes, %d word(s) differ%s' % (len(x), len(y), len(diffs),
          ''.join('; +0x%x orig=%d port=%d' % d for d in diffs[:8])))
    sys.exit(0 if (len(x) == len(y) and not diffs) else 1)


main2()
