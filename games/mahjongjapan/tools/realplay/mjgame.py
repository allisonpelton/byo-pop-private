"""The game under test for the Mahjong Escape drivers: the exe started with -realinput and -commands, a reader of its
log, and the calls a driver makes on it (docs/TEST_COMMANDS.md, docs/REAL_INPUT.md). One class for both games:
mahjongjapan.exe and mahjongchina.exe answer the same commands. Every click and key it sends is a real SDL event."""
import json
import os
import subprocess
import threading
import time

T0 = time.time()


class Failed(Exception):
    pass


def wait_for_slot(limit=None, timeout=3 * 3600):
    """tools/gamecount.py counts every session's game processes: start one only when there is room (CLAUDE.md). The limit is
    6 (GAME_LIMIT sets another)"""
    limit = limit or int(os.environ.get('GAME_LIMIT', '6'))
    root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
    end = time.time() + timeout
    while subprocess.call(['python', os.path.join(root, 'tools', 'gamecount.py'), '--limit', str(limit)],
                          stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) != 0:
        if time.time() > end:
            raise Failed('no free game slot in %d s' % timeout)
        time.sleep(15)


class Game:
    def __init__(self, out, exe, gamedir, fast=True, extra=()):
        wait_for_slot()
        self.out = os.path.abspath(out)
        os.makedirs(self.out, exist_ok=True)
        self.log = open(os.path.join(self.out, 'drive.log'), 'w')
        self.cf = os.path.join(self.out, 'commands.txt')
        empty = os.path.join(self.out, 'realinput.txt')
        open(self.cf, 'w').close()
        open(empty, 'w').close()
        cmd = [exe, '-gamedir=' + os.path.abspath(gamedir), '-userdir=' + os.path.join(self.out, 'user'),
               '-commands=' + self.cf, '-realinput=' + empty] + (['-fast'] if fast else []) + list(extra)
        self.proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
        self.lines, self.cond = [], threading.Condition()
        self.gamelog = open(os.path.join(self.out, 'game.log'), 'w')
        threading.Thread(target=self.reader, daemon=True).start()
        self.shots = 0
        self.asked = 0
        self.updates = 0  # the update the last state came from, for the log's lines

    def say(self, text):
        print('%8.2f u%-7d %s' % (time.time() - T0, self.updates, text), file=self.log, flush=True)

    def reader(self):
        for line in self.proc.stdout:
            self.gamelog.write(line)
            self.gamelog.flush()
            line = line.rstrip('\n').rstrip('\r')
            if line.startswith('INFO: '):
                line = line[6:]
            with self.cond:
                self.lines.append(line)
                self.cond.notify_all()
        with self.cond:
            self.lines.append(None)
            self.cond.notify_all()

    def ask(self, line, timeout=60.0):
        """Sends one line; returns the STATE or MOVE value for state and move, else the OK line"""
        with self.cond:
            start = len(self.lines)
        with open(self.cf, 'a', newline='\n') as f:
            f.write(line + '\n')
        self.asked += 1
        word = line.split()[0]
        end = time.time() + timeout
        i = start
        with self.cond:
            while True:
                while i < len(self.lines):
                    ln = self.lines[i]
                    i += 1
                    if ln is None:
                        raise Failed('the game exited at "%s"' % line)
                    if line == 'state' and ln.startswith('STATE '):
                        st = json.loads(ln[6:])
                        self.updates = st['updates']
                        return st
                    if line == 'move' and ln.startswith('MOVE '):
                        v = ln[5:]
                        return None if v == 'null' else json.loads(v)
                    if ln == 'OK ' + line or ln.startswith('OK ' + line):
                        return ln
                    if ln.startswith('ERROR ' + line) or (ln.startswith('ERROR ' + word) and word not in ('state', 'move')):
                        raise Failed(ln)
                if not self.cond.wait(max(0.0, end - time.time())) and time.time() > end:
                    raise Failed('no answer to "%s"' % line)

    def state(self):
        return self.ask('state')

    def until(self, what, test, timeout=60.0, poll=0.02):
        end = time.time() + timeout
        st = None
        while time.time() < end:
            st = self.state()
            if test(st):
                return st
            time.sleep(poll)
        raise Failed('%s: not reached; last state %s' % (what, json.dumps(st)))

    def settle(self, what='the screen to settle', timeout=60.0):
        return self.until(what, lambda s: not s['busy'], timeout)

    def click(self, x, y):
        self.ask('click game %d %d' % (x, y))

    def key(self, name):
        self.ask('key ' + name)

    def type(self, text):
        self.ask('type ' + text)

    def hover(self, x, y):
        self.ask('move game %d %d' % (x, y))

    def send(self, mv):
        """Plays a move's input steps (docs/TEST_COMMANDS.md) as real events"""
        for step in mv.get('input', []):
            if 'move' in step:
                self.hover(*step['move'])
            elif 'click' in step:
                self.click(*step['click'])
            elif 'key' in step:
                self.key(step['key'])
            elif 'wait' in step:
                self.wait(step['wait'])

    def wait(self, n):
        """n updates of the framework (the game's frame is 25 ms, one update with -fast)"""
        u = self.state()['updates']
        self.until('%d updates' % n, lambda s: s['updates'] >= u + n, poll=0.005)

    def shot(self, name):
        self.shots += 1
        self.ask('screenshot ' + os.path.join(self.out, name + '.png').replace('\\', '/'))

    def close(self):
        if self.proc.poll() is None:
            try:
                self.ask('quit', 10)
            except Failed:
                pass
            try:
                self.proc.wait(15)
            except subprocess.TimeoutExpired:
                self.proc.kill()
