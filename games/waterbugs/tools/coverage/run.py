"""One coverage run of the original Water Bugs (docs/COVERAGE.md): origrun with --coverage on the list that
`python -I tools/coverage.py list waterbugs` writes, the script that scripts.py wrote (orig-<name>.txt), a fresh copy of the game
under C:/byo-og/wbcov, windowed (the original's full screen is DirectDraw's own mode, which a private desktop opens only through
dgVoodoo2, so --dgvoodoo keeps the copy's wrapper and the config's full screen).

    python -I games/waterbugs/tools/coverage/run.py <name> [--clock virtual|real] [--timeout 240] [--cb <cb.dat>] [--cfg <text>]
                                                   [--dgvoodoo] [--sound real]

Waits for room (3 game processes on the machine), runs, and copies the hits to build/coverage/waterbugs/hits/<name>.txt with
the log beside it. A run on the virtual clock has no sound device; one run with --clock real reaches the sound code. Run it
from the worktree's root, in the background (it ends by itself).
"""
import argparse
import os
import shutil
import subprocess
import sys
import time

ROOT = os.getcwd()
CFG = '<CP_CONFIG\n    fullscreen="%d"\n    desktopResolution="0"\n    wave="1"\n    music="1"\n/>\n'
COPY = 'C:/byo-og/wbcov'


def room():
    return subprocess.run([sys.executable, '-I', 'tools/gamecount.py', '--limit', '6'], capture_output=True).returncode == 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('name')
    ap.add_argument('--clock', default='virtual')
    ap.add_argument('--timeout', default='240')
    ap.add_argument('--cb', default=None)
    ap.add_argument('--cfg', default=None, help='the whole cp.cfg text, instead of the windowed default')
    ap.add_argument('--dgvoodoo', action='store_true')
    ap.add_argument('--sound', default=None)
    a = ap.parse_args()
    origrun = os.path.join(ROOT, 'build', 'origrun-bin', 'origrun.exe')
    script = os.path.join(ROOT, 'build', 'coverage', 'waterbugs', 'scripts', 'orig-%s.txt' % a.name)
    hits = os.path.join(ROOT, 'build', 'coverage', 'waterbugs', 'hits')
    os.makedirs(hits, exist_ok=True)
    while not room():
        time.sleep(15)
    shutil.rmtree(COPY, ignore_errors=True)
    shutil.rmtree(os.path.join(ROOT, 'build', 'origrun', 'wbcov'), ignore_errors=True)
    subprocess.run([origrun, '--game', 'waterbugs', '--run', 'wbcov', '--stage-root', 'C:/byo-og', '--copy-only'], capture_output=True)
    game = COPY + '/game'
    with open(game + '/cp.cfg', 'w', newline='\n') as f:
        f.write(a.cfg if a.cfg else CFG % (1 if a.dgvoodoo else 0))
    if not a.dgvoodoo:
        for n in ('DDraw.dll', 'D3DImm.dll', 'dgVoodoo.conf'):
            if os.path.exists(game + '/' + n):
                os.remove(game + '/' + n)
    if a.cb:
        shutil.copy(a.cb, game + '/cb.dat')
    cmd = [origrun, '--game', 'waterbugs', '--run', 'wbcov', '--stage-root', 'C:/byo-og', '--script', script.replace('\\', '/'),
           '--counter', '0x460a1c', '--update-ms', '1', '--cursor', 'scripted', '--clock', a.clock, '--timeout', a.timeout,
           '--coverage', os.path.join(ROOT, 'build', 'coverage', 'waterbugs', 'functions.txt').replace('\\', '/')]
    if a.dgvoodoo:
        cmd.append('--dgvoodoo')
    if a.sound:
        cmd += ['--sound', a.sound]
    log = os.path.join(hits, a.name + '.log')
    with open(log, 'wb') as f:
        t0 = time.time()
        subprocess.run(cmd, stdout=f, stderr=subprocess.STDOUT)
        took = time.time() - t0
    src = os.path.join(ROOT, 'build', 'origrun', 'wbcov', 'coverage.txt')
    if not os.path.exists(src):
        print('%s: no hits file (see %s)' % (a.name, log))
        return 1
    shutil.copy(src, os.path.join(hits, a.name + '.txt'))
    lines = open(src).read().split('\n')
    n = sum(1 for l in lines if l and not l.startswith('#'))
    last = [l for l in lines if l.startswith('#')][-1:]
    print('%s: %d functions in %.0f s; %s' % (a.name, n, took, last[0] if last else ''))
    return 0


if __name__ == '__main__':
    sys.exit(main())
