# play.py [--full] [--games N] [--seed S] [--diff D] [--out DIR] [--timeout SEC]: plays whole games of Rocket Mania natively with real
# input only, as a driver (docs/TEST_COMMANDS.md): the port runs under rundesk (invisible, silent) with -commands (the live
# channel) and -realinput, so every click and key is an SDL event in SDL's own queue. The loop is thin: ask `state`, ask `move`
# (the port's bot: the play link, a dialog's button, the next mode button of the menu, the clicks that turn a path across the
# board), send each step of its input as a real event and go on, until N games have ended (the board back at the menu). With --full
# the window goes to full screen first (Alt+Enter, as a hand does it); a focus loss and gain is sent in the middle of every game.
# It prints one line per game and a RESULT line; screenshots of each game's end and of the menu go to --out.
import argparse, json, os, re, subprocess, sys, time

ap = argparse.ArgumentParser()
ap.add_argument('--play', type=int, default=90, help='seconds of play after which `lose` ends a game; 0: games end by themselves')
ap.add_argument('--fast', action='store_true')
ap.add_argument('--probe', action='store_true')
ap.add_argument('--trace', action='store_true')
ap.add_argument('--full', action='store_true')
ap.add_argument('--games', type=int, default=3)
ap.add_argument('--seed', type=int, default=1)
ap.add_argument('--diff', type=int, default=2)
ap.add_argument('--out', default='build/ph2/driver')
ap.add_argument('--timeout', type=int, default=900)
ap.add_argument('--exe', default='build/native/rocketmania.exe')
ap.add_argument('--reg', default='', help='settings the run starts with, "Name=value,..." as port.sh takes them (Is3D=0 starts in 2D)')
args = ap.parse_args()

out = os.path.abspath(args.out)
os.makedirs(out, exist_ok=True)
log_path = os.path.join(out, 'log.txt')
cmd_path = os.path.join(out, 'commands.txt')
empty = os.path.join(out, 'empty.txt')
user = os.path.join(out, 'user')
subprocess.run(['cmd', '/c', 'rmdir', '/s', '/q', user], capture_output=True)
if args.reg:
    os.makedirs(user)
    with open(os.path.join(user, 'registry.ini'), 'w') as f:
        f.write('; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n\n[SOFTWARE\\PopCap\\RocketMania]\n')
        for kv in args.reg.split(','):
            f.write('%s=dword:%s\n' % tuple(kv.split('=')))
for p in (cmd_path, empty, log_path):
    open(p, 'w').close()
os.remove(cmd_path)
exe = os.path.abspath(args.exe)
rundesk = os.path.abspath('build/native/rundesk.exe')
logf = open(log_path, 'wb')
proc = subprocess.Popen([rundesk, exe, '-commands=' + cmd_path, '-realinput=' + empty] + (['-fast'] if args.fast else []) + ['-seed=%d' % args.seed,
                         '-userdir=' + user], stdout=logf, stderr=subprocess.STDOUT)
pos = 0
t0 = time.time()


def log_text():
    with open(log_path, 'rb') as f:
        return f.read().decode('latin-1')


def send(line, pattern, wait=30.0):
    """appends a command line and returns the first log line after it that matches the pattern"""
    global pos
    pos = len(log_text())
    with open(cmd_path, 'a') as f:
        f.write(line + '\n')
    end = time.time() + wait
    while time.time() < end:
        if proc.poll() is not None:
            raise RuntimeError('the game ended')
        text = log_text()[pos:]
        for l in text.split('\n'):
            if re.search(pattern, l):
                return l.strip()
        time.sleep(0.02)
    raise RuntimeError('no answer to "%s"' % line)


def ok(line):
    name = line.split()[0]
    a = send(line, r'(OK|ERROR) ' + re.escape(name) + r'\b')
    if 'ERROR' in a:
        raise RuntimeError(a)
    return a


def state():
    l = send('state', r'^STATE \{')
    return json.loads(l[l.index('{'):])


def move():
    l = send('move', r'^MOVE ')
    return None if l.endswith('null') else json.loads(l[l.index('{'):])


def run_input(steps):
    for st in steps:
        if 'click' in st:
            x, y = st['click']
            ok('click game %d %d%s' % (x, y, ' right' if st.get('button') == 'right' else ''))
        elif 'key' in st:
            ok('key ' + st['key'])
        elif 'move' in st:
            ok('move game %d %d' % tuple(st['move']))
        elif 'wait' in st:
            time.sleep(st['wait'] * (0.004 if args.fast else 0.01))
        time.sleep(0.01)


OUTF = out.replace(chr(92), '/')
results = []
def probe():
    def cell(): return state()['board']['grid'][8].split()[0]
    ok('screenshot ' + OUTF + '/p0.png'); print('start', cell(), json.dumps(state())[:200])
    ok('click game 271 435'); time.sleep(0.5); print('click ->', cell())
    ok('move game 271 435'); time.sleep(0.1); ok('down game 271 435'); time.sleep(0.1); ok('up game 271 435'); time.sleep(0.3); print('down/up ->', cell())
    ok('rclick game 271 435') if False else None
    ok('click game 271 435 right'); time.sleep(0.3); print('right ->', cell())
    def row(): return state()['board']['grid'][8]
    print('A', row())
    for i in range(3):
        ok('click game 271 435'); time.sleep(0.3); print('click', i, row())
    time.sleep(2.0); print('after 2 s', row())
    for i in range(3):
        ok('click game 271 435'); time.sleep(0.3); print('click', i, row())
    time.sleep(1); print('B', row(), state().get('boardState'))
    for gap in ():
        a = cell(); ok('click game 313 435'); time.sleep(gap); ok('click game 313 435'); time.sleep(0.5)
        print('two clicks gap', gap, state()['board']['grid'][8].split()[1], 'was', a)
    ok('screenshot ' + OUTF + '/p1.png'); print(json.dumps(state())[:400])
try:
    send('state', r'^STATE \{', 120)         # the game answers once it is running
    ok('set difficulty %d' % args.diff)
    if args.full:
        ok('keydown Left Alt'); ok('key Return'); ok('keyup Left Alt')
        time.sleep(0.5)
    if args.probe:
        while state().get('screen') != 'board':
            run_input((move() or {'input': []})['input'])
        time.sleep(0.5); probe(); raise SystemExit
    started, finished, was_board, board_updates, focus_done = 0, 0, False, 0, False
    shots = 0
    while finished < args.games and time.time() - t0 < args.timeout:
        s = state()
        on_board = s.get('screen') == 'board' and 'dialog' not in s
        if s.get('screen') == 'board' and not was_board:
            started += 1
            focus_done = False
            board_updates = 0
            board_t0 = time.time()
            lost = False
        if s.get('screen') == 'menu' and was_board:
            finished += 1
            results.append('game %d ended at score %s' % (finished, s.get('score')))
        was_board = s.get('screen') == 'board'
        if was_board:
            board_updates += 1
            if not focus_done and board_updates > 60:
                focus_done = True
                ok('focus lost'); time.sleep(0.3); a = state().get('paused'); ok('focus gained'); time.sleep(0.3); b = state().get('paused')
                results.append('focus: paused %s after the loss, %s after the regain' % (a, b))
        if was_board and not lost and args.play > 0 and time.time() - board_t0 > args.play and 'dialog' not in s:
            lost = True
            ok('lose')
        m = move()
        if m is None:
            time.sleep(0.05)
            continue
        run_input(m['input'])
        if args.trace:
            s2 = state(); print('TRACE', m['what'][:30], len(m['input']), 'moves', s.get('moves'), '->', s2.get('moves'), 'score', s2.get('score'), 'state', s2.get('boardState'), 'row8', (s2.get('board') or {}).get('grid', [''])[-1], flush=True)
        if s.get('screen') == 'menu' and not was_board:
            shots += 1
    s = state()
    ok('screenshot ' + os.path.join(out, 'end.png').replace('\\', '/'))
    status = 'ok' if finished >= args.games else 'TIMEOUT'
    print('\n'.join(results))
    print('RESULT %s: %d games ended, %s, %.0f s, last state %s' % (status, finished, 'full screen' if args.full else 'windowed', time.time() - t0, json.dumps(s)[:120]))
except Exception as e:
    print('RESULT FAILED:', e)
finally:
    try:
        with open(cmd_path, 'a') as f:
            f.write('quit\n')
        proc.wait(timeout=20)
    except Exception:
        proc.kill()
