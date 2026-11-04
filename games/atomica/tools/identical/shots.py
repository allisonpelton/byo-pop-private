#!/usr/bin/env python3
"""Identical-play pass, phase two: screens. Plays a scenario (see compare.py; the extra command "<update> shot <name>" takes
a picture) in the original, inside Windows Sandbox on origrun's virtual clock (the only place origrun takes pictures,
docs/ORIGRUN.md), and in the port (-fast -drawevery, the shot leaves the game's random numbers as they were), then runs
tools/shotdiff on the two folders.

    python games/atomica/tools/identical/shots.py <scenario.txt> --run <id> [--seed N] [--port-only | --orig-only] [--savefile F]
"""
import argparse
import os
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, '..', '..', '..', '..'))
BASH = os.environ.get('BASH_EXE') or 'C:/Program Files/Git/usr/bin/bash.exe'
REGKEY = r'HKLM\SOFTWARE\WOW6432Node\PopCap\Atomica'


def fwd(p):
    return p.replace(chr(92), '/')


def read(path):
    lines, end = [], None
    for l in open(path):
        w = l.split('#')[0].split()
        if not w:
            continue
        if w[0] == 'end':
            end = int(w[1])
        else:
            lines.append((int(w[0]), w[1], w[2:]))
    lines.sort(key=lambda l: l[0])	# the scripts run in file order
    return lines, end


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('scenario')
    ap.add_argument('--run', required=True)
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--port-only', action='store_true')
    ap.add_argument('--orig-only', action='store_true')
    ap.add_argument('--savefile')
    a = ap.parse_args()
    lines, end = read(a.scenario)
    out = os.path.join(ROOT, 'build/shots-atomica', a.run)
    pdir = os.path.join(out, 'port')
    odir = os.path.join(ROOT, 'build/origrun', 'shot-' + a.run, 'shots')
    if not a.orig_only:
        shutil.rmtree(pdir, ignore_errors=True)
        os.makedirs(pdir)
        d = tempfile.mkdtemp(prefix='atomshot')
        script = os.path.join(d, 's.txt')
        with open(script, 'w', newline='\n') as f:
            for n, c, w in lines:
                if c == 'shot':
                    f.write('%d shot %s\n' % (n, fwd(os.path.join(pdir, w[0] + '.png'))))
                elif c == 'text':
                    for ch in ' '.join(w):
                        f.write('%d char %s\n' % (n + 1, 'SPACE' if ch == ' ' else ch))
                else:
                    f.write('%d %s %s\n' % (n + 1, c, ' '.join(w)))
            f.write('%d quit\n' % (end + 1))
        ud = os.path.join(d, 'user')
        if a.savefile:
            os.makedirs(ud)
            shutil.copy(a.savefile, os.path.join(ud, 'SAVEGAME.DAT'))
        subprocess.run([BASH, fwd(os.path.join(ROOT, 'games/atomica/tools/run_script.sh')), fwd(script), fwd(ud), '600',
                        '-seed=%d' % a.seed, '-drawevery'], cwd=ROOT, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        shutil.rmtree(d, ignore_errors=True)
        print('port: %d pictures' % len(os.listdir(pdir)))
    if not a.port_only:
        d = tempfile.mkdtemp(prefix='atomshot')
        script = os.path.join(d, 's.txt')
        with open(script, 'w', newline='\n') as f:
            for n, c, w in lines:
                f.write('at %d %s %s\n' % (n, c, ' '.join(w)))
            f.write('at %d move 1 1\nquit\n' % end)
        run = 'shot-' + a.run
        shutil.rmtree(os.path.join(ROOT, 'build/origrun', run), ignore_errors=True)
        if a.savefile:
            # origrun stages the copy itself: stage it, put the saved game in, run
            subprocess.run([os.path.join(ROOT, 'build/origrun-bin/origrun.exe'), '--game', 'atomica', '--run', run,
                            '--copy-only'], cwd=ROOT, capture_output=True)
            shutil.copy(a.savefile, os.path.join(ROOT, 'build/origrun', run, 'game', 'SAVEGAME.DAT'))
        p = subprocess.run([os.path.join(ROOT, 'build/origrun-bin/origrun.exe'), '--game', 'atomica', '--run', run,
                            '--script', script, '--clock', 'virtual', '--cursor', 'scripted', '--seed', str(a.seed),
                            '--sandbox', '--timeout', '200'], cwd=ROOT, capture_output=True, text=True)
        for l in p.stdout.splitlines():
            if 'RESULT' in l or 'SOUND LEAK' in l:
                print(l[:200])
        shutil.rmtree(d, ignore_errors=True)
        print('original: %d pictures' % len(os.listdir(odir)) if os.path.isdir(odir) else 'original: no pictures')
    if not a.port_only and not a.orig_only:
        r = subprocess.run([sys.executable, os.path.join(ROOT, 'tools/shotdiff/shotdiff.py'), '--game', 'atomica', '--orig', odir,
                            '--port', pdir, '--run', a.run], cwd=ROOT, capture_output=True, text=True)
        print(r.stdout.strip()[-600:], r.stderr.strip()[-300:])


main()
