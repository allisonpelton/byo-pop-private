"""Plays Pixelus with real input only (CLAUDE.md, "Done means identical"): the port runs with -commands and -realinput,
so every click and key is an SDL event pushed into SDL's own queue and goes the way a hand's input goes. The code plays,
not the session: it reads the state and the click targets of the screen (`state`, `targets`), asks the port's own solver
for the next move (`move`), and sends that move's input steps as real events. A state needed in a hurry is never
reached with a test command here: launch, the loading screen, the new-user dialog, the story, the island, the temples
and the boards are all played through clicks and keys.

    build/native/rundesk.exe python games/pixelus/tools/realplay/drive.py <out dir> [--puzzles N] [--full]
        [--focus] [--exe PATH] [--timeout SECONDS]

--puzzles N   stop once the profile has N solved puzzles (default 3: the tutorial temple's first three)
--full        Alt+Enter into full screen after the first board has opened (and back at the end)
--expert      tick the new player's expert box (no undo on glue)
--focus       lose the focus and regain it in the middle of a board
Results: <out dir>/drive.log (every command and answer), <out dir>/result.txt (one line: PASS/FAIL and counts), the
shots in <out dir>/shots. The port's own log is <out dir>/out.txt.
"""
import json
import os
import subprocess
import sys
import time

ARGS = sys.argv[1:]
OUT = os.path.abspath(ARGS[0])
OPTS = ARGS[1:]


def opt(name, default=None):
    if name in OPTS:
        i = OPTS.index(name)
        return OPTS[i + 1] if i + 1 < len(OPTS) and not OPTS[i + 1].startswith('--') else True
    return default


PUZZLES = int(opt('--puzzles', 3))
FULL = bool(opt('--full'))
FOCUS = bool(opt('--focus'))
EXPERT = bool(opt('--expert'))
TIMEOUT = float(opt('--timeout', 600))
ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
EXE = opt('--exe', os.path.join(ROOT, 'build', 'native', 'pixelus.exe'))

os.makedirs(OUT, exist_ok=True)
CF = os.path.join(OUT, 'commands.txt')
LOG = os.path.join(OUT, 'out.txt')
DLOG = open(os.path.join(OUT, 'drive.log'), 'w')
START = time.time()
for f in (CF, LOG):
    if os.path.exists(f):
        os.remove(f)
open(CF, 'w').close()
EMPTY = os.path.join(OUT, 'realinput.txt')
open(EMPTY, 'w').close()
USER = os.path.join(OUT, 'user')
if os.path.exists(USER):
    import shutil
    shutil.rmtree(USER)

PROC = subprocess.Popen([EXE, '-userdir=' + USER, '-commands=' + CF, '-realinput=' + EMPTY, '-fast'],
                        stdout=open(LOG, 'w'), stderr=subprocess.STDOUT, cwd=ROOT)


def note(text):
    DLOG.write('%7.1f %s\n' % (time.time() - START, text))
    DLOG.flush()


def finish(ok, text):
    note(('PASS ' if ok else 'FAIL ') + text)
    with open(os.path.join(OUT, 'result.txt'), 'w') as f:
        f.write(('PASS ' if ok else 'FAIL ') + text + '\n')
    try:
        send('quit', None, 5)
    except Exception:
        pass
    try:
        PROC.wait(15)
    except Exception:
        PROC.kill()
    print(('PASS ' if ok else 'FAIL ') + text)
    sys.exit(0 if ok else 1)


def log_size():
    return os.path.getsize(LOG) if os.path.exists(LOG) else 0


def send(line, token, wait=30):
    """Appends a command to the live channel and returns the log line that answers it (the first after the command with
    the token in it), or None when no token is wanted."""
    mark = log_size()
    with open(CF, 'a') as f:
        f.write(line + '\n')
    note('> ' + line)
    if token is None:
        return None
    end = time.time() + wait
    while time.time() < end:
        if PROC.poll() is not None:
            finish(False, 'the game ended on "%s"' % line)
        with open(LOG, 'rb') as f:
            f.seek(mark)
            for raw in f.read().decode('utf-8', 'replace').splitlines():
                if token in raw:
                    note('< ' + raw[:300])
                    return raw
        time.sleep(0.02)
    finish(False, 'no answer to "%s" within %ds' % (line, wait))


def jsonof(raw, key):
    return json.loads(raw[raw.index(key) + len(key):])


def state():
    return jsonof(send('state', 'STATE {'), 'STATE ')


def targets():
    return jsonof(send('targets', 'TARGETS {'), 'TARGETS ')


def click(x, y):
    send('click game %d %d' % (x, y), 'OK click', 10)


def wait_updates(n=12):
    """Lets some updates run: a state query is answered between two updates, so a few of them are enough."""
    for _ in range(n):
        send('state', 'STATE {')


def shot(name):
    os.makedirs(os.path.join(OUT, 'shots'), exist_ok=True)
    send('screenshot ' + os.path.join(OUT, 'shots', name + '.png'), 'screenshot', 10)


def wait_screen(names, seconds=30):
    end = time.time() + seconds
    while time.time() < end:
        s = state()
        if s['screen'] in names:
            return s
        time.sleep(0.05)
    finish(False, 'screen %s never came (last: %s)' % (names, state()['screen']))


def press_button(label=None, index=0):
    t = targets()['buttons']
    if label is None:
        # a labelled button first (continue, OK): the empty ones are the tablet's picture buttons
        t = [b for b in t if b['label']] or t
    if label is not None:
        t = [b for b in t if b['label'].lower() == label.lower()] or t
    if not t:
        return False
    click(t[index]['x'], t[index]['y'])
    return True


def play_board():
    """Plays the board in view to its end with the solver's moves, as real clicks."""
    moved = 0
    stall = 0
    focused = False
    while True:
        s = state()
        if s['screen'] != 'board':
            return moved
        b = s.get('board', {})
        if b.get('done'):
            wait_updates(20)
            return moved
        if b.get('busy'):
            wait_updates(3)
            continue
        m = send('move', 'MOVE ', 10)
        if m.strip().endswith('MOVE null'):
            stall += 1
            if stall > 400:
                finish(False, 'the board stood still')
            wait_updates(5)
            continue
        stall = 0
        mv = jsonof(m, 'MOVE ')
        before = b.get('moves')
        for step in mv['input']:
            if 'move' in step:
                send('move game %d %d' % tuple(step['move']), 'OK move', 10)
                wait_updates(2)
            elif 'click' in step:
                click(*step['click'])
        moved += 1
        # the move has been taken once the move count changes or the tile flies
        for _ in range(100):
            n = state().get('board', {})
            if n.get('moves') != before or n.get('busy') or n.get('done'):
                break
            wait_updates(2)
        if FOCUS and not focused and moved == 3:
            focused = True
            send('focus lost', 'OK focus', 10)
            wait_updates(30)
            send('focus gained', 'OK focus', 10)
            wait_updates(10)
            note('focus lost and regained')


DONE = set()
LAST = [0]


def main():
    send('seed 1', 'OK seed', 180)
    # the loading screen: wait for the play link, then click it
    for _ in range(2000):
        s = state()
        if s['screen'] != 'loading':
            break
        t = targets()['buttons']
        if t:
            shot('loading')
            click(t[0]['x'], t[0]['y'])
            wait_updates(5)
        else:
            time.sleep(0.05)
    solved_total = 0
    full_done = False
    last = None
    same = 0
    while True:
        if time.time() - START > TIMEOUT:
            finish(False, 'timeout')
        s = state()
        scr = s['screen']
        if scr == last:
            same += 1
        else:
            same = 0
            last = scr
            shot('%02d_%s' % (len(os.listdir(os.path.join(OUT, 'shots'))) if os.path.isdir(os.path.join(OUT, 'shots')) else 0, scr))
        if same > 200:
            finish(False, 'stuck on %s' % scr)
        if scr != 'board' and s.get('solved', 0) >= PUZZLES:
            break
        if scr == 'menu':
            press_button('Play') or press_button(None, 0)
            wait_updates(10)
        elif scr == 'dialog':
            tg = targets()
            t = tg['buttons']
            if EXPERT:
                for box in tg.get('checks', []):
                    if box['id'] == 1 and not box['checked']:
                        click(box['x'], box['y'])
                        wait_updates(3)
            send('type Driver', 'OK type', 10)
            wait_updates(3)
            ok = [b for b in t if b['label'].lower() in ('ok', 'continue', 'yes')]
            click((ok or t)[0]['x'], (ok or t)[0]['y'])
            wait_updates(10)
        elif scr in ('story', 'help', 'completed', 'completion'):
            wait_updates(40)
            if not press_button(None, 0):
                wait_updates(10)
            wait_updates(10)
        elif scr == 'island':
            t = targets()['buttons']
            hot = [b for b in t if b['label'] == '' and b['id'] >= 2]
            hot.sort(key=lambda b: b['id'])
            hot = [b for b in hot if b['id'] not in DONE] or hot
            entered = False
            for b in hot:
                LAST[0] = b['id']
                click(b['x'], b['y'])
                wait_updates(10)
                if state()['screen'] == 'temple':
                    entered = True
                    break
                if state()['screen'] == 'dialog':
                    press_button(None, 0)
                    wait_updates(5)
            if not entered and same > 5:
                finish(False, 'no temple opens')
        elif scr == 'temple':
            p = [q for q in targets()['puzzles'] if not q['solved'] and not q['mystery']]
            if not p:
                p = [q for q in targets()['puzzles'] if not q['solved']]
            if not p:
                # every puzzle of this temple is solved: leave it and try the next one
                DONE.add(LAST[0])
                press_button(None, 0)
                wait_updates(20)
                continue
            click(p[0]['x'], p[0]['y'])
            wait_updates(20)
        elif scr == 'board':
            if FULL and not full_done:
                full_done = True
                send('keydown Left Alt', 'OK keydown', 10)
                send('key Return', 'OK key', 10)
                send('keyup Left Alt', 'OK keyup', 10)
                wait_updates(20)
                shot('fullscreen')
            n = play_board()
            solved_total += 1
            note('board played: %d moves, %d boards' % (n, solved_total))
        else:
            wait_updates(20)
    if FULL:
        send('keydown Left Alt', 'OK keydown', 10)
        send('key Return', 'OK key', 10)
        send('keyup Left Alt', 'OK keyup', 10)
        wait_updates(20)
    shot('end')
    finish(True, '%d boards played, %d solved in the profile, %.0f s' % (solved_total, state().get('solved', 0), time.time() - START))


main()
