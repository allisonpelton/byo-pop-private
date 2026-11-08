"""The thin loop around the port's test commands (docs/TEST_COMMANDS.md): launches the game with -commands (and -realinput
for hand-made input), sends a command line, waits for its answer. The class is the one of games/bejeweled/tools/realplay."""
import json
import os
import re
import subprocess
import time

ANSWER = re.compile(r'^(STATE |MOVE |OK |ERROR )')


ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))))


class Driver:
    """Starts the game under build/native/rundesk.exe (a private desktop, silent, below-normal priority: CLAUDE.md, "Running
    games in tests") unless the caller already runs under it (rundesk sets BYOPOP_TEST_RUN=1 for its children), so that no
    caller can show the game on the user's screen. A new user folder starts windowed (ScreenMode 0) unless windowed=False."""

    def __init__(self, exe, gamedir, userdir, out, fast, extra, windowed=True, is3d=None, hold=False):
        exe, gamedir, userdir, out = (os.path.abspath(p) if p else p for p in (exe, gamedir, userdir, out))  # rundesk starts the exe by an absolute path
        self.out = out
        self.hold = hold  # -hold: an update runs only as the driver's lines let it (a wait N runs N), so the game's clock is the driver's
        registry = os.path.join(userdir, 'registry.ini')
        if not os.path.exists(registry):
            os.makedirs(userdir, exist_ok=True)
            with open(registry, 'w') as f:
                f.write('[SOFTWARE\\Steam\\Bejeweled2]\n')
                if windowed:
                    f.write('ScreenMode=dword:0\n')
                if is3d is not None:
                    f.write('Is3D=dword:%d\n' % (1 if is3d else 0))
        self.cmdfile = os.path.join(out, 'commands.txt')
        self.empty = os.path.join(out, 'realinput.txt')
        open(self.cmdfile, 'w').close()
        open(self.empty, 'w').close()
        self.logpath = os.path.join(out, 'game.log')
        args = [exe, '-gamedir=' + gamedir, '-userdir=' + userdir, '-commands=' + self.cmdfile,
                '-realinput=' + self.empty] + (['-fast'] if fast else []) + (['-hold'] if hold else []) + extra
        self.log = open(self.logpath, 'wb')
        if os.environ.get('BYOPOP_TEST_RUN') != '1':
            args = [os.path.join(ROOT, 'build', 'native', 'rundesk.exe')] + args
        self.proc = subprocess.Popen(args, stdout=self.log, stderr=subprocess.STDOUT)
        self.pos = 0
        self.sent = 0

    def alive(self):
        return self.proc.poll() is None

    def _new_lines(self):
        with open(self.logpath, 'rb') as f:
            f.seek(self.pos)
            data = f.read()
        end = data.rfind(b'\n')
        if end < 0:
            return []
        self.pos += end + 1
        return [l.decode('utf-8', 'replace').rstrip('\r') for l in data[:end].split(b'\n')]

    def send(self, line, timeout=60):
        """Appends a line to the commands file and waits for its answer (STATE/MOVE/OK/ERROR); returns the answer line."""
        self._new_lines()  # what came before is not this line's answer
        with open(self.cmdfile, 'ab') as f:
            f.write((line + '\n').encode())
        self.sent += 1
        deadline = time.time() + timeout
        while True:
            for l in self._new_lines():
                l = re.sub(r'^INFO: ', '', l)
                if ANSWER.match(l):
                    return l
            if not self.alive():
                raise RuntimeError('the game ended before the answer to "%s"' % line)
            if time.time() > deadline:
                raise RuntimeError('no answer to "%s" in %d s' % (line, timeout))
            time.sleep(0.005)

    def ok(self, line):
        a = self.send(line)
        if not a.startswith('OK'):
            raise RuntimeError('"%s": %s' % (line, a))

    def state(self):
        a = self.send('state')
        if not a.startswith('STATE '):
            raise RuntimeError('state: ' + a)
        return json.loads(a[6:])

    def bot_move(self):
        a = self.send('move')
        if a.startswith('MOVE null'):
            return None
        if not a.startswith('MOVE '):
            raise RuntimeError('move: ' + a)
        return json.loads(a[5:])

    # a hand's input, in game pixels
    def move(self, p):
        self.ok('move game %d %d' % tuple(p))

    def click(self, p):
        self.ok('move game %d %d' % tuple(p))
        self.ok('click game %d %d' % tuple(p))

    def key(self, name):
        self.ok('key ' + name)

    def type(self, text):
        self.ok('type ' + text)

    def shot(self, name):
        path = os.path.join(self.out, name + '.png')
        self.ok('screenshot ' + path.replace('\\', '/'))
        return path

    def wait_updates(self, n, timeout=30):
        """Waits until the board has made n more updates (the board's tick), or the screen is no longer the game."""
        if self.hold:
            self.ok('wait %d' % n)
            return self.state()
        s = self.state()
        if 'tick' not in s:
            time.sleep(0.05)
            return s
        want = s['tick'] + n
        deadline = time.time() + timeout
        while s.get('tick', want) < want and s['screen'] == 'game' and time.time() < deadline:
            time.sleep(0.01)
            s = self.state()
        return s

    def idle(self, seconds=0.02):
        """A short pause: real time, or with -hold a few updates of the game's own clock."""
        if self.hold:
            self.ok('wait 2')
        else:
            time.sleep(seconds)

    def wait_screen(self, screens, timeout=60):
        deadline = time.time() + timeout
        s = self.state()
        while s['screen'] not in screens:
            if time.time() > deadline:
                raise RuntimeError('screen %s, wanted %s' % (s['screen'], '/'.join(screens)))
            time.sleep(0.02)
            s = self.state()
        return s

    def play_input(self, steps):
        """A MOVE's input: an ordered list of steps in game pixels (docs/TEST_COMMANDS.md)."""
        for st in steps:
            if 'move' in st:
                self.move(st['move'])
            elif 'click' in st:
                p = st['click']
                self.ok('click game %d %d%s' % (p[0], p[1], ' right' if st.get('button') == 'right' else ''))
            elif 'press' in st:
                self.ok('down game %d %d' % tuple(st['press']))
            elif 'release' in st:
                self.ok('up game %d %d' % tuple(st['release']))
            elif 'key' in st:
                self.key(st['key'])
            elif 'wait' in st:
                self.wait_updates(st['wait'])

    def close(self):
        try:
            if self.alive():
                with open(self.cmdfile, 'ab') as f:
                    f.write(b'quit\n')
                try:
                    self.proc.wait(10)
                except subprocess.TimeoutExpired:
                    self.proc.kill()
        finally:
            self.log.close()


