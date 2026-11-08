"""Plays the Alchemy port through the trunk's live channel (docs/REAL_INPUT.md, docs/TEST_COMMANDS.md "Sending them"): the
game runs with -commands=<file> and an empty -realinput file, so every line this driver appends is a real SDL event through
HandleEvent, the window-to-game mapping and the widget manager, and the window's focus, minimize and Alt+Enter are real
too. drive.py (posted window messages) cannot lose the focus; this can. Each move comes from the port's own bot (the
test commands `state` and `move`); code plays, a session reads the result and the shots.

    build/native/rundesk.exe python games/alchemy/tools/realplay/live.py <out dir> <step>... [--seed N] [--fast]

Steps, run in order in one game process (a failed goal ends the run with code 1):
    launch                  the title: wait for the link, click it; the selector comes up
    start <mode> <skill>    from the selector: click the skill, the mode and Start (mode: strategic | timetrial)
    tutorial                from the selector: click the tutorial picture
    play <n>                n pieces placed by the bot's moves (or until the selector is back)
    finish                  play until the selector is back (the game, the name entry and the table by the bot)
    focus                   "focus lost" for 100 updates (the game must hold still and say it is not active), then
                            "focus gained"
    minimize                "focus lost", minimize, 100 updates, restore, "focus gained" (a hand's minimize)
    altenter                Alt+Enter: full screen (windowed false), then play on with `play`, then altenter again
    shot <name>             the game's own screenshot into <out dir>/<name>.png
    close                   the window's close button; the game must exit by itself
Exit code 0 when every step reached its goal.
"""
import json
import os
import re
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
EXE = os.path.join(ROOT, 'build', 'native', 'alchemy.exe')
KEYS = {'Enter': 'Return', 'ArrowLeft': 'Left', 'ArrowRight': 'Right', 'ArrowUp': 'Up', 'ArrowDown': 'Down', ' ': 'Space'}


class Failure(Exception):
    pass


class Live:
    def __init__(self, out, seed=1, fast=False, extra=()):
        self.out = os.path.abspath(out)
        os.makedirs(self.out, exist_ok=True)
        self.cf = os.path.join(self.out, 'commands.txt')
        self.ri = os.path.join(self.out, 'realinput.txt')
        self.logp = os.path.join(self.out, 'stdout.txt')
        open(self.cf, 'w').close()
        open(self.ri, 'w').close()
        self.say_f = open(os.path.join(self.out, 'live.log'), 'w', buffering=1)
        args = [EXE, '-commands=' + self.cf, '-realinput=' + self.ri, '-userdir=' + os.path.join(self.out, 'user'),
                '-seed=%d' % seed] + (['-fast'] if fast else []) + list(extra)
        # SDL's log (the framework's "OK ..." answers, "INFO: " lines) goes to stderr, the game's STATE and MOVE to stdout:
        # one file holds both, in the order they were written
        self.p = subprocess.Popen(args, cwd=ROOT, stdout=open(self.logp, 'w', buffering=1), stderr=subprocess.STDOUT)
        self.reader = open(self.logp)
        self.buf = ''
        self.state = None
        self.stats = dict(places=0, forges=0, dialogs=0)

    def say(self, *a):
        line = time.strftime('%H:%M:%S ') + ' '.join(str(x) for x in a)
        print(line, flush=True)
        self.say_f.write(line + '\n')

    # ---- the channel ----
    def lines(self):
        chunk = self.reader.read()
        if chunk:
            self.buf += chunk
        *done, self.buf = self.buf.split('\n')
        return [re.sub(r'^\w+: ', '', ln.rstrip('\r')) for ln in done]

    def ask(self, line, want, timeout=20):
        """Appends one command and returns the log line that answers it (the first that starts with `want`)."""
        self.lines()                                    # what came before is not its answer
        with open(self.cf, 'a', newline='') as f:
            f.write(line + '\n')
        t0 = time.time()
        while time.time() - t0 < timeout:
            for ln in self.lines():
                if ln.startswith(want):
                    return ln
                if ln.startswith('ERROR') and line.split()[0] in ln:
                    raise Failure('%s -> %s' % (line, ln))
            if self.p.poll() is not None:
                raise Failure('the game exited (%s) during "%s"' % (self.p.returncode, line))
            time.sleep(0.005)
        raise Failure('no answer to "%s" within %d s' % (line, timeout))

    def do(self, line):
        self.ask(line, 'OK ' + line.split()[0])

    def get_state(self):
        ln = self.ask('state', 'STATE ')
        self.state = json.loads(ln[6:])
        return self.state

    def get_move(self):
        ln = self.ask('move', 'MOVE ')
        body = ln[5:]
        return None if body == 'null' else json.loads(body)

    def updates(self):
        return self.get_state()['updates']

    def sleep_updates(self, n):
        time.sleep(n * 0.0102)

    def wait_state(self, pred, timeout, what):
        t0 = time.time()
        while time.time() - t0 < timeout:
            st = self.get_state()
            if pred(st):
                return st
            time.sleep(0.02)
        raise Failure('timeout (%d s) waiting for %s; last %s' % (timeout, what, json.dumps(self.state)[:400]))

    # ---- input ----
    def move(self, x, y):
        self.do('move game %d %d' % (x, y))

    def click(self, x, y, right=False):
        self.do('click game %d %d%s' % (x, y, ' right' if right else ''))

    def key(self, name):
        name = KEYS.get(name, name)
        self.do('key %s' % (name.upper() if len(name) == 1 else name))

    def press(self, pt, what):
        self.move(pt[0] - 6, pt[1] - 6)
        self.sleep_updates(1)
        self.move(*pt)
        self.sleep_updates(2)
        self.click(*pt)
        self.sleep_updates(5)
        self.say('pressed', what, pt)

    def act(self, mv):
        k = mv['kind']
        self.stats['places' if k == 'place' else 'forges' if k == 'forge' else 'dialogs'] += 1
        for st in mv['input']:
            if 'move' in st:
                self.move(*st['move'])
            elif 'wait' in st:
                self.sleep_updates(st['wait'])
            elif 'click' in st:
                self.click(*st['click'], right=(st.get('button') == 'right'))
            elif 'key' in st:
                self.key(st['key'])
        self.sleep_updates(1)

    def play(self, places, limit_s=900):
        """Plays the bot's moves until `places` pieces were placed (None: until the selector is back)."""
        t0 = time.time()
        start = self.stats['places']
        while time.time() - t0 < limit_s:
            st = self.get_state()
            if st['screen'] == 'menu':
                return
            if places is not None and self.stats['places'] - start >= places:
                return
            mv = self.get_move()
            if mv is None:
                self.sleep_updates(5)
                continue
            before = json.dumps([(st.get('board') or {}).get('moves'), (st.get('board') or {}).get('state')])
            self.act(mv)
            if mv['kind'] == 'place':       # the next move waits until the game shows this one
                t1 = time.time()
                while time.time() - t1 < 2.5:
                    b = self.get_state().get('board') or {}
                    if json.dumps([b.get('moves'), b.get('state')]) != before:
                        break
                    time.sleep(0.02)
        raise Failure('play ran past %d s; last %s' % (limit_s, json.dumps(self.state)[:300]))

    # ---- steps ----
    def s_launch(self):
        self.wait_state(lambda s: s['screen'] == 'title' and s.get('ready'), 180, 'the title link')
        self.shot('title')
        self.act(self.get_move())
        self.wait_state(lambda s: s['screen'] == 'menu', 60, 'the selector')
        self.sleep_updates(30)
        self.shot('menu')

    def s_start(self, mode, skill):
        sel = self.get_state()['selector']
        self.press(sel['skill' + skill], 'skill ' + skill)
        sel = self.get_state()['selector']
        self.press(sel[mode], mode)
        sel = self.get_state()['selector']
        self.press(sel['start'], 'start')
        self.wait_state(lambda s: s['screen'] == 'game', 60, 'the board')
        self.shot(mode + '-start')

    def s_tutorial(self):
        self.press(self.get_state()['selector']['tutorial'], 'tutorial')
        self.wait_state(lambda s: s['screen'] == 'game', 60, 'the board')

    def snap(self):
        s = self.get_state()
        b = s.get('board') or {}
        return dict(active=s.get('active'), muted=s.get('muted'), windowed=s.get('windowed'), score=s.get('score'),
                    moves=b.get('moves'), paused=b.get('paused'), forge=s.get('forge'), updates=s['updates'])

    def s_focus(self):
        before = self.snap()
        self.do('focus lost')
        self.sleep_updates(100)
        during = self.snap()
        self.shot('focus-lost')
        self.say('focus lost: before', before)
        self.say('focus lost: during', during)
        if during['active']:
            raise Failure('the game stayed active after focus lost')
        if (during['score'], during['moves']) != (before['score'], before['moves']):
            raise Failure('score or moves changed while the window had no focus')
        self.do('focus gained')
        self.sleep_updates(30)
        after = self.snap()
        self.say('focus gained: after', after)
        if not after['active']:
            raise Failure('the game stayed inactive after focus gained')

    def s_minimize(self):
        before = self.snap()
        self.do('focus lost')
        self.do('minimize')
        self.sleep_updates(100)
        during = self.snap()
        self.say('minimized: before', before)
        self.say('minimized: during', during)
        if during['active']:
            raise Failure('the game stayed active while minimized')
        if (during['score'], during['moves']) != (before['score'], before['moves']):
            raise Failure('score or moves changed while minimized')
        self.do('restore')
        self.do('focus gained')
        self.sleep_updates(40)
        after = self.snap()
        self.say('restored: after', after)
        if not after['active']:
            raise Failure('the game stayed inactive after restore')
        self.shot('restored')

    def s_altenter(self):
        was = self.snap()['windowed']
        self.do('keydown Left Alt')
        self.do('key Return')
        self.do('keyup Left Alt')
        t0 = time.time()
        while time.time() - t0 < 10:
            if self.snap()['windowed'] != was:
                break
            time.sleep(0.1)
        now = self.snap()
        self.say('alt+enter: windowed', was, '->', now['windowed'])
        if now['windowed'] == was:
            raise Failure('Alt+Enter did not change the screen mode')
        self.sleep_updates(40)
        self.shot('altenter-' + ('window' if now['windowed'] else 'full'))

    def shot(self, name):
        path = os.path.join(self.out, name + '.png')
        try:
            os.remove(path)
        except OSError:
            pass
        self.do('screenshot ' + path)

    def s_close(self):
        self.do('close')
        for _ in range(300):
            if self.p.poll() is not None:
                break
            time.sleep(0.1)
        if self.p.poll() is None:
            raise Failure('the window\'s close button did not end the game')
        self.say('closed: exit code', self.p.returncode)
        if self.p.returncode != 0:
            raise Failure('exit code %s' % self.p.returncode)

    def stop(self):
        if self.p.poll() is None:
            self.p.terminate()
            try:
                self.p.wait(10)
            except Exception:
                self.p.kill()


def main():
    args = sys.argv[1:]
    if len(args) < 2:
        print(__doc__)
        return 2
    out = args.pop(0)
    seed = 1
    fast = False
    extra = []
    steps = []
    while args:
        a = args.pop(0)
        if a == '--seed':
            seed = int(args.pop(0))
        elif a == '--fast':
            fast = True
        elif a == '--modsdir':
            extra.append('-modsdir=' + os.path.abspath(args.pop(0)))
        else:
            steps.append(a)
    g = Live(out, seed, fast, extra)
    code = 1
    try:
        i = 0
        while i < len(steps):
            s = steps[i]
            i += 1
            g.say('step', s)
            if s == 'launch':
                g.s_launch()
            elif s == 'start':
                g.s_start(steps[i], steps[i + 1])
                i += 2
            elif s == 'tutorial':
                g.s_tutorial()
            elif s == 'play':
                g.play(int(steps[i]))
                i += 1
            elif s == 'finish':
                g.play(None)
                g.shot('finish')
            elif s == 'focus':
                g.s_focus()
            elif s == 'minimize':
                g.s_minimize()
            elif s == 'altenter':
                g.s_altenter()
            elif s == 'shot':
                g.shot(steps[i])
                i += 1
            elif s == 'close':
                g.s_close()
            else:
                raise Failure('unknown step ' + s)
        g.say('RESULT ok', json.dumps(g.stats), 'last', json.dumps(g.state)[:300])
        code = 0
    except Failure as e:
        g.say('RESULT FAILED:', e)
    finally:
        g.stop()
    return code


if __name__ == '__main__':
    sys.exit(main())
