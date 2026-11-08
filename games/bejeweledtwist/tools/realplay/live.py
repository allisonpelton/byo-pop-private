"""Plays the Bejeweled Twist port through the trunk's live channel (docs/REAL_INPUT.md, docs/TEST_COMMANDS.md "Sending
them"): the game runs with -commands=<file> and an empty -realinput file, so every line this driver appends is a real
SDL event through HandleEvent, the window-to-game mapping and the widget manager, and the window's focus, minimize and
Alt+Enter are real too. Each twist comes from the port's own bot (the test commands `state` and `move`, which also
click the tutorial's and the tips' buttons and make their forced twists); code plays, a session reads the result and
the shots. Modelled on games/alchemy/tools/realplay/live.py.

    build/native/rundesk.exe python games/bejeweledtwist/tools/realplay/live.py <out dir> <step>... [options]

Options: --seed N, --fast, --2d (-nod3d9), --full (starts in full screen), --art 600|768|1200 (768), --keep (keeps
<out dir>/user from an earlier run; otherwise a new player is made).
Steps, run in order in one game process (a failed goal ends the run with code 1):
    launch                  a new player's name typed and Return, then the main menu
    goto <mode>             from the main menu, through the menu's own Play and mode buttons (the test command `goto`
                            presses them); a new player's first Classic is the tutorial
    play <n>                n twists by the bot (tips and forced twists too), or until the game is over
    finish                  play until the game is over, then on to the main menu (clicks given by the bot's `move`)
    unlock                  the test command `unlock all` (every mode)
    zenquit                 the tube's menu button, the options' Main Menu and Yes in "Save and Quit", by clicks
    focus                   "focus lost" for 100 updates (the game must say it is not active and not change), then
                            "focus gained"
    minimize                "focus lost", minimize, 100 updates, restore, "focus gained"
    altenter                Alt+Enter: the screen mode changes (windowed <-> full screen)
    shot <name>             the game's own screenshot into <out dir>/<name>.png
    close                   the window's close button; the game must exit by itself with code 0
Exit code 0 when every step reached its goal.
"""
import json
import os
import re
import shutil
import subprocess
import sys
import time

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
EXE = os.path.join(ROOT, 'build', 'native', 'bejeweledtwist.exe')
GAMEDIR = os.path.join(ROOT, 'assets', 'bejeweledtwist')


class Failure(Exception):
    pass


class Live:
    def __init__(self, out, seed=1, fast=False, extra=(), art=768, full=False, keep=False):
        self.out = os.path.abspath(out)
        os.makedirs(self.out, exist_ok=True)
        user = os.path.join(self.out, 'user')
        if not keep:
            shutil.rmtree(user, ignore_errors=True)
            os.makedirs(user)
            with open(os.path.join(user, 'registry.ini'), 'w') as f:
                f.write('[SOFTWARE\\Steam\\BejeweledTwist]\nArtRes=dword:%d\nScreenMode=dword:%d\n' % (art, 1 if full else 0))
        self.cf = os.path.join(self.out, 'commands.txt')
        self.ri = os.path.join(self.out, 'realinput.txt')
        self.logp = os.path.join(self.out, 'stdout.txt')
        open(self.cf, 'w').close()
        open(self.ri, 'w').close()
        self.say_f = open(os.path.join(self.out, 'live.log'), 'w', buffering=1)
        args = [EXE, '-gamedir=' + GAMEDIR, '-commands=' + self.cf, '-realinput=' + self.ri, '-userdir=' + user,
                '-seed=%d' % seed] + (['-fast'] if fast else []) + list(extra)
        # SDL's log (the framework's "OK ..." answers) goes to stderr, the game's STATE and MOVE to stdout: one file
        # holds both, in the order they were written
        self.p = subprocess.Popen(args, cwd=ROOT, stdout=open(self.logp, 'w', buffering=1), stderr=subprocess.STDOUT)
        self.reader = open(self.logp)
        self.buf = ''
        self.state = None
        self.stats = dict(twists=0, forced=0, dialogs=0, stops=0, ends=0)

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

    def ask(self, line, want, timeout=30):
        """Appends one command and returns the log line that answers it (the first that starts with `want`)."""
        self.lines()
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
        self.state = json.loads(self.ask('state', 'STATE ')[6:])
        return self.state

    def get_move(self):
        body = self.ask('move', 'MOVE ')[5:]
        return None if body == 'null' else json.loads(body)

    def sleep_updates(self, n):
        time.sleep(n * 0.0102)

    def wait_state(self, pred, timeout, what):
        t0 = time.time()
        while time.time() - t0 < timeout:
            st = self.get_state()
            if pred(st):
                return st
            time.sleep(0.05)
        raise Failure('timeout (%d s) waiting for %s; last %s' % (timeout, what, json.dumps(self.state)[:400]))

    # ---- input ----
    def move(self, x, y):
        self.do('move game %d %d' % (x, y))

    def click(self, x, y):
        self.do('click game %d %d' % (x, y))

    def press(self, pt, what):
        # a hover first, as a player's mouse arrives: the board takes the square under the cursor in its update
        self.move(pt[0] - 8, pt[1] - 8)
        self.sleep_updates(2)
        self.move(*pt)
        self.sleep_updates(3)
        self.click(*pt)
        self.sleep_updates(5)
        self.say('pressed', what, pt)

    def act(self, mv):
        k = mv['kind']
        self.stats[{'twist': 'twists', 'forced': 'forced', 'dialog': 'dialogs', 'stop': 'stops'}.get(k, 'ends')] += 1
        if k != 'twist':
            n = self.stats['forced'] + self.stats['dialogs'] + self.stats['stops'] + self.stats['ends']
            self.shot('%s%d' % (k, n))        # what the tip, the tutorial or the wheel shows, before its click
        pt = None
        for st in mv['input']:
            if 'click' in st:
                pt = st['click']
        self.press(pt, k + ('' if 'row' not in mv else ' %d,%d' % (mv['row'], mv['col'])))

    def board_key(self, st):
        return json.dumps([st.get('moves'), st.get('board'), st.get('score'), st.get('tip'), st.get('wheel')])

    def play(self, twists, limit_s=1800):
        """The bot's moves until `twists` twists were made (None: until the game is over or the board gone)."""
        t0 = time.time()
        start = self.stats['twists'] + self.stats['forced']
        last_shot = start
        idle = 0
        while time.time() - t0 < limit_s:
            st = self.get_state()
            if st['screen'] != 'game' or st.get('over'):
                return st
            made = self.stats['twists'] + self.stats['forced']
            if twists is not None and made - start >= twists:
                return st
            if made - last_shot >= 50:
                last_shot = made
                self.shot('%s-%d' % (st.get('mode'), made))
            if st.get('busy') and not st.get('wheel') and not st.get('rank') and not (st.get('tip') or {}).get('dialog'):
                time.sleep(0.03)
                continue
            mv = self.get_move()
            if mv is None:
                idle += 1
                if idle % 400 == 0:
                    self.say('no move for a while:', json.dumps(st)[:300])
                time.sleep(0.03)
                continue
            idle = 0
            before = self.board_key(st)
            self.act(mv)
            t1 = time.time()          # the next move waits until the game shows this one
            while time.time() - t1 < 3:
                if self.board_key(self.get_state()) != before:
                    break
                time.sleep(0.03)
        raise Failure('play ran past %d s; last %s' % (limit_s, json.dumps(self.state)[:300]))

    # ---- steps ----
    def s_launch(self):
        st = self.wait_state(lambda s: s['screen'] not in ('loading',), 300, 'the end of loading')
        self.say('after loading:', json.dumps(st)[:200])
        if st['screen'] == 'menu':                  # the menu's slide in, then the new player's panel
            self.sleep_updates(60)
            st = self.get_state()
        if st['screen'] == 'newplayer':
            self.sleep_updates(60)
            self.shot('profile')
            for ch in 'TWIST':
                self.do('key ' + ch)
            self.do('key Return')
        self.wait_state(lambda s: s['screen'] == 'menu', 120, 'the main menu')
        self.sleep_updates(100)
        self.shot('menu')

    def s_goto(self, mode):
        self.do('goto ' + mode)
        st = self.wait_state(lambda s: s['screen'] == 'game' and s.get('mode') == mode, 120, 'the ' + mode + ' board')
        self.say('board:', json.dumps(st)[:300])
        self.sleep_updates(50)
        self.shot(mode + '-start')

    def s_finish(self):
        st = self.play(None)
        self.say('game over:', json.dumps(st)[:400])
        self.shot('over')
        # the end of the game: whatever the bot clicks (none of it a link), until the main menu is back
        t0 = time.time()
        n = 0
        while time.time() - t0 < 300:
            st = self.get_state()
            if st['screen'] == 'menu':
                self.sleep_updates(60)
                self.shot('after-game')
                return
            mv = self.get_move()
            if mv is not None and mv['kind'] in ('dialog', 'stop', 'continue', 'back'):
                self.act(mv)
            elif mv is None and st['screen'] not in ('game', 'loading'):
                n += 1
                if n % 100 == 1:
                    self.say('waiting on', st['screen'])
            time.sleep(0.1)
        self.shot('stuck')
        raise Failure('the main menu did not come back after the game; last %s' % json.dumps(self.state)[:300])

    def s_zenquit(self, points):
        for i, pt in enumerate(points):
            self.press(pt, 'zen quit %d' % i)
            self.sleep_updates(150)
            self.shot('zenquit%d' % i)
            self.say('after click', i, json.dumps(self.get_state())[:120])
        self.wait_state(lambda s: s['screen'] == 'menu', 60, 'the main menu after Save and Quit')

    def snap(self):
        s = self.get_state()
        return dict(active=s.get('active'), windowed=s.get('windowed'), score=s.get('score'), moves=s.get('moves'),
                    board=s.get('board'), timer=s.get('timer'), updates=s['updates'])

    def same_play(self, a, b):
        return (a['score'], a['moves'], a['board'], a['timer']) == (b['score'], b['moves'], b['board'], b['timer'])

    def s_focus(self):
        before = self.snap()
        self.do('focus lost')
        self.sleep_updates(100)
        during = self.snap()
        self.shot('focus-lost')
        self.say('focus lost: before', {k: v for k, v in before.items() if k != 'board'})
        self.say('focus lost: during', {k: v for k, v in during.items() if k != 'board'})
        if during['active']:
            raise Failure('the game stayed active after focus lost')
        self.sleep_updates(100)
        later = self.snap()
        if not self.same_play(during, later):
            raise Failure('the game went on while the window had no focus')
        self.do('focus gained')
        self.sleep_updates(30)
        after = self.snap()
        self.say('focus gained: after', {k: v for k, v in after.items() if k != 'board'})
        if not after['active']:
            raise Failure('the game stayed inactive after focus gained')

    def s_minimize(self):
        before = self.snap()
        self.do('focus lost')
        self.do('minimize')
        self.sleep_updates(100)
        during = self.snap()
        self.sleep_updates(100)
        later = self.snap()
        self.say('minimized: before', {k: v for k, v in before.items() if k != 'board'})
        self.say('minimized: during', {k: v for k, v in during.items() if k != 'board'})
        if during['active']:
            raise Failure('the game stayed active while minimized')
        if not self.same_play(during, later):
            raise Failure('the game went on while minimized')
        self.do('restore')
        self.do('focus gained')
        self.sleep_updates(40)
        after = self.snap()
        self.say('restored: after', {k: v for k, v in after.items() if k != 'board'})
        if not after['active']:
            raise Failure('the game stayed inactive after restore')
        self.shot('restored')

    def s_altenter(self):
        was = self.snap()['windowed']
        self.do('keydown Left Alt')
        self.do('key Return')
        self.do('keyup Left Alt')
        t0 = time.time()
        while time.time() - t0 < 15:
            if self.snap()['windowed'] != was:
                break
            time.sleep(0.1)
        now = self.snap()
        self.say('alt+enter: windowed', was, '->', now['windowed'])
        if now['windowed'] == was:
            raise Failure('Alt+Enter did not change the screen mode')
        self.sleep_updates(60)
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
    seed, fast, art, full, keep = 1, False, 768, False, False
    extra = []
    steps = []
    while args:
        a = args.pop(0)
        if a == '--seed':
            seed = int(args.pop(0))
        elif a == '--fast':
            fast = True
        elif a == '--2d':
            extra.append('-nod3d9')
        elif a == '--full':
            full = True
        elif a == '--art':
            art = int(args.pop(0))
        elif a == '--keep':
            keep = True
        else:
            steps.append(a)
    g = Live(out, seed, fast, extra, art, full, keep)
    code = 1
    try:
        i = 0
        while i < len(steps):
            s = steps[i]
            i += 1
            g.say('step', s)
            if s == 'launch':
                g.s_launch()
            elif s == 'goto':
                g.s_goto(steps[i])
                i += 1
            elif s == 'unlock':
                g.do('unlock all')
            elif s == 'play':
                g.play(int(steps[i]))
                i += 1
            elif s == 'finish':
                g.s_finish()
            elif s == 'zenquit':
                pts = [tuple(int(v) for v in p.split(',')) for p in steps[i].split(';')]
                i += 1
                g.s_zenquit(pts)
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
        try:
            g.shot('failed')
        except Exception:
            pass
    finally:
        g.stop()
    return code


if __name__ == '__main__':
    sys.exit(main())
