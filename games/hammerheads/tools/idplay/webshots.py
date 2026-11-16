#!/usr/bin/env python3
"""Phase two, the web build: a scenario's port inputs played by the native port (under rundesk) and by the web build (under Node,
tools wasmrun.js), the same script, ticks and switches, and the two sets of shots compared with tools/shotdiff.

    python games/hammerheads/tools/idplay/webshots.py <scenario> <first> <last> <step>

A shot is taken every <step> updates from <first> to <last>, as looks.py does for the port (the inputs go in at their updates, with no
original and so no loading-screen mark). The two are one program on two back ends (SDL's renderer natively, its software renderer
under Node), so what differs is the back end's drawing, not the game's play. Shots land in build/logs/webshots/<scenario>/{native,web}.
"""
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import idplay  # noqa: E402

ROOT = idplay.ROOT
NODE = 'E:/retools/emsdk/node/24.19.0_64bit/node.exe'
WASMRUN = os.path.join(ROOT, 'games', 'bejeweled', 'tools', 'verify', 'wasmrun.js')
WEB_JS = os.path.join(ROOT, 'build', 'web', 'hammerheads.js')
GAMEDIR = os.path.join(ROOT, 'assets', 'hammerheads')


def script_text(scenario, updates, folder):
    lines = []
    for count, _, port_command in scenario.INPUTS:
        if port_command and count <= updates[-1]:
            lines.append((count + 1, port_command))
    for count in updates:
        lines.append((count, 'screenshot %s/u%d.png' % (folder, count)))
    lines.sort(key=lambda item: item[0])
    lines.append((updates[-1] + 20, 'quit'))
    return '\n'.join('%d %s' % item for item in lines) + '\n'


def main():
    name = sys.argv[1]
    first, last, step = (int(v) for v in sys.argv[2:5])
    scenario = idplay.load_scenario(name)
    updates = list(range(first, last + 1, step))
    base = os.path.join(ROOT, 'build', 'logs', 'webshots', name)
    shutil.rmtree(base, ignore_errors=True)
    seed = '-seed=%s' % getattr(scenario, 'SEED', 1)
    # native: absolute shot paths, a user folder of its own
    native = os.path.join(base, 'native')
    os.makedirs(os.path.join(native, 'shots'))
    open(os.path.join(native, 'port.txt'), 'w', newline='\n').write(script_text(scenario, updates, (native + '/shots').replace('\\', '/')))
    status_native = subprocess.call(
        [idplay.RUNDESK, idplay.PORT_EXE, '-fast', '-drawevery', '-renderscale=1', seed, '-userdir=' + os.path.join(native, 'user'),
         '-script=' + os.path.join(native, 'port.txt')],
        stdout=open(os.path.join(native, 'run.log'), 'w'), stderr=subprocess.STDOUT, cwd=ROOT)
    # web: shot paths relative to the run's folder
    web = os.path.join(base, 'web')
    os.makedirs(os.path.join(web, 'shots'))
    os.makedirs(os.path.join(web, 'user'))
    open(os.path.join(web, 'port.txt'), 'w', newline='\n').write(script_text(scenario, updates, 'shots'))
    status_web = subprocess.call(
        [NODE, WASMRUN, 'game', '--timeout', '3000', WEB_JS, '-gamedir=' + GAMEDIR, '-userdir=user', '-fast', '-drawevery', seed,
         '-renderscale=1', '-script=port.txt'],
        stdout=open(os.path.join(web, 'run.log'), 'w'), stderr=subprocess.STDOUT, cwd=web)
    print('%s: native exit %d, %d shots; web exit %d, %d shots' % (
        name, status_native, len(os.listdir(os.path.join(native, 'shots'))), status_web, len(os.listdir(os.path.join(web, 'shots')))))
    subprocess.call([sys.executable, os.path.join(ROOT, 'tools', 'shotdiff', 'shotdiff.py'), '--game', 'hammerheads',
                     '--orig', os.path.join(native, 'shots'), '--port', os.path.join(web, 'shots'), '--run', name + '_web'], cwd=ROOT)
    return 0 if status_native == 0 and status_web == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
