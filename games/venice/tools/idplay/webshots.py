#!/usr/bin/env python3
"""Phase two, the web build: a scenario's port run (the script the last `idplay.py shots <scenario>` wrote) played again by the
web build under Node (tools/web wasmrun.js, as docs/BUILDS.md describes), and its shots compared with the native port's.

    python games/venice/tools/idplay/webshots.py <scenario>

The web build gets the same switches as the native run (-fast, -drawevery, -seed, -renderscale=1), the same ticks file, profile and
script; the shots are written into the run's folder (the shot paths of the script are made relative to it) and shotdiff compares
them with build/ph2/<scenario>/port, the native port's. The two are one program on two back ends (SDL's renderer natively, its
software renderer under Node), so what differs is the back end's drawing, not the game's play.
"""
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))
NODE = 'E:/retools/emsdk/node/24.19.0_64bit/node.exe'
WASMRUN = os.path.join(ROOT, 'games', 'bejeweled', 'tools', 'verify', 'wasmrun.js')
WEB_JS = os.path.join(ROOT, 'build', 'web', 'venice.js')
GAMEDIR = os.path.join(ROOT, 'assets', 'venice')


def main():
    name = sys.argv[1]
    run = os.path.join(ROOT, 'build', 'logs', 'idplay', name)
    case = os.path.join(ROOT, 'build', 'logs', 'ph2', 'web', name)
    shutil.rmtree(case, ignore_errors=True)
    os.makedirs(os.path.join(case, 'shots'))
    shutil.copytree(os.path.join(run, 'port.user'), os.path.join(case, 'port.user'))
    # the player the native run started with, not the one it saved (a Trick Shot player is on the level it reached)
    start_profile = os.path.join(run, 'start_profile.dat')
    if os.path.isfile(start_profile):
        shutil.copyfile(start_profile, os.path.join(case, 'port.user', 'profiles.dat'))
    shutil.copyfile(os.path.join(run, 'ticks.txt'), os.path.join(case, 'ticks.txt'))
    script = open(os.path.join(run, 'port.txt'), encoding='utf-8').read()
    script = re.sub(r'(screenshot) \S*/([^/\s]+\.png)', r'\1 shots/\2', script)
    script = re.sub(r'(ticks) \S+', r'\1 ticks.txt', script)
    script = re.sub(r'(trace) \S+', r'\1 web.trace', script)
    with open(os.path.join(case, 'port.txt'), 'w', newline='\n') as out:
        out.write(script)
    command = [NODE, WASMRUN, 'game', '--timeout', '3000', WEB_JS, '-gamedir=' + GAMEDIR, '-userdir=port.user', '-fast', '-drawevery',
               '-seed=1', '-renderscale=1', '-script=port.txt']
    with open(os.path.join(case, 'web.out'), 'w') as out:
        status = subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=case)
    shots = [f for f in os.listdir(os.path.join(case, 'shots')) if f.endswith('.png')]
    print('%s: the web build exited with %d and took %d shots' % (name, status, len(shots)))
    native = os.path.join(ROOT, 'build', 'ph2', name, 'port')
    if shots and os.path.isdir(native):
        # shotdiff takes the native port's shots as the reference (--orig) and the web build's as the port's
        subprocess.call([sys.executable, os.path.join(ROOT, 'tools', 'shotdiff', 'shotdiff.py'), '--game', 'venice', '--orig', native,
                         '--port', os.path.join(case, 'shots'), '--run', name + '_web'], cwd=ROOT)
    return status


if __name__ == '__main__':
    sys.exit(main())
