#!/usr/bin/env python3
"""botrec.py <mode> <seed> <out script> [--shots <dir>] [--moves N] [--match45] [--level N]
[--museumwait N] [--winafter N]: plays the port by its own bot and writes
the run as a port -script, so that the same input can be played again in the port and in the original (tour.sh with
SCRIPT=, toorig.py). <mode> is tutorial, endless, stomped, fossil, panic or timetrial.

The port runs under rundesk with -fast -drawevery (the cases' settings) on the live channel (-commands) with -hold, so
its updates run only when this driver's lines let them. There is no -realinput, so every line goes through the same
dispatch as a -script line, at the same point of the update (SexyAppBase::UpdateFrames). Each input line is sent with
"state" in one batch, which runs both in one update and answers with its number, and the script gets "<update> <line>".

The input is the tours': the title's link at update 25010, the main menu's pages, then the bot's moves (the tutorial's
Continue button, the museum's buttons, its shots), each a move and a click 5 updates later, until the tutorial is over
and the main menu is back, or the game is over. A game then waits for the name dialog, types "Bot", presses Okay and
waits for the table. After --moves moves (default 400) the bot's shots give way to shots straight up, which end a game
the bot is still winning (Endless and Panic, IDENTICAL_PLAY.md, "Played with real input"). With --shots a screenshot
line goes into the script before each button move, 40 updates after each new step of the tutorial, every 50 shots, at
the game's end, in the name dialog and at the table. With --match45 the bot's first shot at the tutorial's step 45 is
the one a player would make (`matchmove`, which makes a match; IDENTICAL_PLAY.md, "Oddities"), and the run ends 3000
updates later.
With --level N (Fossil) the run starts fossil N at its first fragment by the `level` test command once the game is up.
With --museumwait N the museum's button is pressed N updates after the bot first offers it, and again N updates after
each press while it is still offered, so that the museum's own sounds play: the fragment's at its fade-in, the chime of
a fossil's last fragment, and after fossil 15 the Rex attack's footsteps, roar and chime, which take about 900 updates
from the press of its "Beware" button (the button stays disabled meanwhile, and a press of it does nothing).
With --winafter N (the Fossil Challenge) the `win` test command ends each fragment after the bot's N-th shot of it, so
a run reaches the last fossil and the Rex attack, which the bot's own play does not (fossil 14 beat it).
"""
import atexit
import json
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))))
os.chdir(ROOT)
mode, seed, out = sys.argv[1], int(sys.argv[2]), sys.argv[3]
match45 = '--match45' in sys.argv
shotdir = sys.argv[sys.argv.index('--shots') + 1] if '--shots' in sys.argv else None
maxmoves = int(sys.argv[sys.argv.index('--moves') + 1]) if '--moves' in sys.argv else 400
startlevel = int(sys.argv[sys.argv.index('--level') + 1]) if '--level' in sys.argv else None
museumwait = int(sys.argv[sys.argv.index('--museumwait') + 1]) if '--museumwait' in sys.argv else 0
winafter = int(sys.argv[sys.argv.index('--winafter') + 1]) if '--winafter' in sys.argv else 0
# the main menu's clicks for each mode (mkscript.py's): New Game, the mode, Next, the page's choice, Let's Play
MENU = {'tutorial': [(486, 355), (488, 432)],
        'endless': [(486, 213), (465, 160), (488, 432)],
        'stomped': [(486, 213), (465, 230), (488, 432), (465, 162), (488, 432)],
        'fossil': [(486, 213), (465, 295), (488, 432), (465, 200), (488, 432)],
        'panic': [(486, 213), (465, 295), (488, 432), (465, 280), (488, 432)],
        'timetrial': [(486, 213), (465, 360), (488, 432), (465, 265), (488, 432)]}
NAME_DIALOG = 6
run = os.path.join(os.path.dirname(os.path.abspath(out)), 'rec')
os.makedirs(os.path.join(run, 'user'), exist_ok=True)
for f in os.listdir(os.path.join(run, 'user')):
    os.remove(os.path.join(run, 'user', f))
# the settings port.sh gives the port (windowed, as tour.sh runs it) and mkhs.py's low tables
with open(os.path.join(run, 'user', 'registry.ini'), 'w') as f:
    f.write('[SOFTWARE\\PopCap\\Dynomite]\nCustomCursors=dword:0\nInProgress=dword:0\nMusicVolume=dword:50\nMuted=dword:0\n'
            'PreferredX=dword:-1\nPreferredY=dword:-1\nScreenMode=dword:0\nSfxVolume=dword:85\nTimesExecuted=dword:5\n'
            'TimesPlayed=dword:0\nWaitForVSync=dword:0\n')
subprocess.check_call([sys.executable, 'games/dynomite/tools/identical/mkhs.py', os.path.join(run, 'user')])
commands = os.path.join(run, 'commands.txt')
open(commands, 'wb').close()
logname = os.path.join(run, 'game.log')
log = open(logname, 'wb')
proc = subprocess.Popen([os.path.abspath('build/native/rundesk.exe'), os.path.abspath('build/native/dynomite.exe'),
                         '-gamedir=assets/dynomite',
                         '-userdir=' + os.path.join(run, 'user'), '-seed=%d' % seed, '-fast', '-drawevery',
                         '-commands=' + commands, '-hold'], stdout=log, stderr=subprocess.STDOUT)


def end_port():
    """On any exit (an error included), the held port is told to quit, and stopped if it does not."""
    if proc.poll() is None:
        with open(commands, 'ab') as f:
            f.write(b'quit\n')
        try:
            proc.wait(15)
        except subprocess.TimeoutExpired:
            proc.kill()


atexit.register(end_port)
reader = open(logname, 'rb')
partial = b''
script = []


def lines():
    global partial
    data = partial + reader.read()
    cut = data.rfind(b'\n')
    if cut < 0:
        partial = data
        return []
    partial = data[cut + 1:]
    return [l.rstrip(b'\r').decode('utf-8', 'replace') for l in data[:cut].split(b'\n')]


def send(*ls):
    """the lines at once (one update runs them); the answers of state and move and the OK of the rest, in order"""
    want = list(ls)
    with open(commands, 'ab') as f:
        f.write(b''.join(l.encode() + b'\n' for l in ls))
    answers = []
    start = time.time()
    while want:
        for raw in lines():
            t = raw[6:] if raw.startswith('INFO: ') else raw
            w = want[0].split()[0]
            if (w == 'state' and t.startswith('STATE ')) or (w in ('move', 'matchmove') and t.startswith('MOVE ')) or \
                    t.startswith('OK ' + w) or t.startswith('ERROR ' + w):
                if t.startswith('ERROR'):
                    raise SystemExit('botrec: ' + t)
                answers.append(t)
                want.pop(0)
                if not want:
                    break
        if want:
            if proc.poll() is not None:
                raise SystemExit('botrec: the game exited waiting for ' + want[0])
            if time.time() - start > 120:
                raise SystemExit('botrec: no answer to ' + want[0])
            time.sleep(0.002)
    return answers


def state():
    return json.loads(send('state')[0][6:])


def wait(n):
    if n > 0:
        send('wait %d' % n)


def at(u, line):
    """the line at update u (at the next update when u has passed), recorded with the update it ran at"""
    s = state()
    wait(u - s['update'] - 1)
    a = send(line, 'state')
    upd = json.loads(a[1][6:])['update']
    script.append('%d %s' % (upd, line))
    return upd


def click(x, y, u=None):
    """a move, then the click 5 updates later; returns the click's update"""
    u = state()['update'] + 1 if u is None else u
    at(u, 'move %d %d' % (x, y))
    return at(u + 5, 'click %d %d' % (x, y))


def shot(name, delay=1):
    if shotdir:
        script.append('%d screenshot %s/%s.png' % (state()['update'] + delay, shotdir, name))


s = state()
print('botrec: loaded at update', s['update'], s['screen'])
u = click(320, 450, 25000)
for x, y in MENU[mode]:
    u = click(x, y, u + 60)
wait(60 if mode == 'tutorial' else 500)
if startlevel is not None:
    at(state()['update'] + 1, 'level %d' % startlevel)
    wait(100)
shot('start')
step = None
moves = shots = buttons = 0
nulls = 0
end = None
offered = None	# the update the bot first offered the museum's button (--museumwait)
fragment = None	# the fragment (level, round) and the bot's shots in it (--winafter)
fragshots = 0
won = set()
while True:
    s = state()
    if mode == 'tutorial':
        if s['screen'] != 'game' and 'tutorial' not in s:
            break
        if end is not None and s['update'] >= end:
            break
        if s.get('tutorial') != step:
            step = s.get('tutorial')
            shot('step%03d' % (step if step is not None else 999), 40)
    elif s.get('over') or s.get('dialog') == NAME_DIALOG:
        break
    if s.get('busy'):
        wait(10)
        continue
    if winafter and 'round' in s:
        if (s['level'], s['round']) != fragment:
            fragment, fragshots = (s['level'], s['round']), 0
        if fragshots >= winafter and fragment not in won:
            at(s['update'] + 1, 'win')
            won.add(fragment)
            wait(100)
            continue
    any45 = match45 and step == 0x2d and end is None
    if mode != 'tutorial' and moves >= maxmoves:
        m = 'MOVE {"kind":"up","input":[{"click":[400,60]}]}'	# straight up from the slingshot
    else:
        m = send('matchmove' if any45 else 'move')[0]
    if m == 'MOVE null':
        nulls += 1
        if nulls > 200:
            raise SystemExit('botrec: no move for 6000 updates at step %s' % step)
        wait(30)
        continue
    nulls = 0
    mv = json.loads(m[5:])
    x, y = mv['input'][-1]['click']
    if museumwait and mv['kind'] == 'button' and mode != 'tutorial':
        if offered is None:
            offered = s['update']
        if s['update'] < offered + museumwait:
            wait(min(30, offered + museumwait - s['update']))
            continue
        offered = state()['update'] + 6	# a press that leaves the button offered waits again
    elif mv['kind'] != 'button':
        offered = None
    if mv['kind'] in ('button', 'continue') and mode != 'tutorial':
        buttons += 1
        shot('button%02d' % buttons)
    click(x, y)
    moves += 1
    if mv['kind'] in ('shot', 'up'):
        shots += 1
        fragshots += 1
        if shots % 50 == 0:
            shot('shot%04d' % shots, 30)
    if any45:
        end = state()['update'] + 3000
        print('botrec: the matching shot at step 45, update', end - 3000, mv)
    wait(20 if mv['kind'] in ('shot', 'up') else 30)
s = state()
print('botrec: %d moves (%d shots), step %s, update %d, %s' % (moves, shots, step, s['update'], json.dumps(s)[:300]))
if mode != 'tutorial':
    # the end panel, then the name dialog (the low tables make every score a high score), "Bot", Okay, the table
    shot('over', 30)
    for i in range(100):
        if state().get('dialog') == NAME_DIALOG:
            break
        wait(30)
    else:
        raise SystemExit('botrec: no name dialog')
    wait(30)
    for c in 'Bot':
        at(state()['update'] + 5, 'char %s' % c)
    wait(25)
    shot('name')
    click(352, 369)
    wait(200)
    shot('table')
last = state()['update'] + 10
shot('end', 10)
# the run's last state, which a replay of the script must print the same (toorig.py leaves test commands out)
final = send('state')[0]
print('botrec: final', final)
script.append('%d state' % (json.loads(final[6:])['update']))
script.append('%d quit' % (last + 5))
with open(commands, 'ab') as f:
    f.write(b'quit\n')
try:
    proc.wait(30)
except subprocess.TimeoutExpired:
    proc.kill()
script.sort(key=lambda l: int(l.split()[0]))
with open(out, 'w', newline='\n') as f:
    f.write('\n'.join(script) + '\n')
print('botrec: wrote', out, len(script), 'lines; last update', last + 5)
