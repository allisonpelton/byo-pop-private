"""Plays a list of steps in the port under -hold and prints what each asks for, to find where the buttons of each screen are.

    build/native/rundesk.exe python games/pixelus/tools/screens/explore.py <out dir> <steps file> [--preset <user folder>] [--arg A]...

Each step is a line: `wait N`, `click X Y`, `move X Y`, `key NAME`, `type TEXT`, `shot NAME`, `targets`, `state`, or any other
test command line (`unlock all`, `goto island`...). `targets` and `state` print the answer; `shot` writes <out dir>/<NAME>.png.
"""
import json, os, shutil, subprocess, sys, time

args = sys.argv[1:]
OUT = os.path.abspath(args[0]); STEPS = args[1]
preset = args[args.index('--preset') + 1] if '--preset' in args else None
extra = [args[i + 1] for i, a in enumerate(args) if a == '--arg']
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
EXE = os.path.join(ROOT, 'build', 'native', os.environ.get('EXE', 'pixelus.exe'))
os.makedirs(OUT, exist_ok=True)
USER = os.path.join(OUT, 'user')
shutil.rmtree(USER, ignore_errors=True)
if preset:
    shutil.copytree(preset, USER)
CF = os.path.join(OUT, 'commands.txt'); LOG = os.path.join(OUT, 'out.txt')
for f in (CF, LOG):
    if os.path.exists(f): os.remove(f)
open(CF, 'w').close()
RI = os.path.join(OUT, 'realinput.txt'); open(RI, 'w').close()
P = subprocess.Popen([EXE, '-userdir=' + USER, '-commands=' + CF, '-realinput=' + RI, '-fast', '-hold', '-windowscale=1', '-seed=1'] + extra,
                     stdout=open(LOG, 'w'), stderr=subprocess.STDOUT, cwd=ROOT)


def send(line, token, wait=60):
    mark = os.path.getsize(LOG) if os.path.exists(LOG) else 0
    with open(CF, 'a') as f: f.write(line + '\n')
    if token is None: return None
    end = time.time() + wait; nudged = False
    while time.time() < end:
        if P.poll() is not None: return 'GAME ENDED'
        with open(LOG, 'rb') as f:
            f.seek(mark)
            for raw in f.read().decode('utf-8', 'replace').splitlines():
                if token in raw: return raw
        if not nudged and time.time() > end - wait + 3:
            nudged = True
            with open(CF, 'a') as f: f.write('wait 1\n')
        time.sleep(0.003)
    return 'NO ANSWER to ' + line


for line in open(STEPS):
    line = line.split('#')[0].strip()
    if not line: continue
    c, _, rest = line.partition(' ')
    if c == 'wait': r = send(line, 'OK wait', 300)
    elif c in ('click', 'move'): r = send(c + ' game ' + rest, 'OK ' + c, 60)
    elif c == 'key': r = send('key ' + rest, 'OK key', 30)
    elif c == 'type': r = send('type ' + rest, 'OK type', 30)
    elif c == 'shot': r = send('screenshot ' + os.path.join(OUT, rest + '.png').replace(chr(92), '/'), 'screenshot', 60)
    elif c == 'targets': r = send('targets', 'TARGETS {')
    elif c == 'state': r = send('state', 'STATE {')
    else: r = send(line, 'OK ' + c, 60)
    if c in ('targets', 'state') or (r and ('NO ANSWER' in r or 'ENDED' in r or 'ERROR' in r)):
        print(line, '->', (r or '')[:1400])
send('quit', None)
try: P.wait(20)
except Exception: P.kill()
