"""Plays the Ning Po port with real input only (CLAUDE.md, "Done means identical"): every click and key goes through
-realinput into SDL's own queue and takes a hand's path from there (docs/REAL_INPUT.md). The port's own bot chooses
every move (the `move` test command, docs/TEST_COMMANDS.md); this script sends the input the move names over the live
channel (-commands) and checks what the game's `state` says next.

    build/native/rundesk.exe python games/ningpo/tools/realplay/drive.py <out dir> [--fullscreen] [--games N]
        [--exe E] [--realtime]

The tour: the start screen, the title's arrows (layout, tiles, background, each one way and back), the options and
their best times, the rules page by page, a focus loss and regain, then N games (default 2) on the bot's moves, with
every board button tried in the first (hint, show moves, pause and resume, undo, shuffle, and New Game answered no),
each game to its end (no pair left and End Game; the last of two or more won by the `win` test command after ten real
pairs, since the bot never clears a board), the name, the best times, back to the title. With
--fullscreen the window goes full screen by Alt+Enter before the games and back after them. The log is
<out dir>/drive.log, the game's own log <out dir>/game.log, its screenshots <out dir>/*.png; the last line of the
log, and of stdout, is "RESULT ok" or "RESULT FAILED: <why>". It runs with -fast unless --realtime.
"""
import json
import os
import subprocess
import sys
import threading
import time

# the board's buttons (NingPo::createControls: each sprite's top left, plus a point inside it), the title's arrows
BOARD = {'newgame': (47, 115), 'options': (50, 185), 'rules': (55, 220), 'hint': (50, 265), 'showmoves': (50, 300),
         'pause': (50, 335), 'undo': (60, 370), 'shuffle': (50, 405)}
# the options' buttons (OptionsDialog::setup, centres as its state lists them)
OPTIONS = {'besttimes': (282, 289), 'fullscreen': (312, 229), 'autohint': (571, 229), 'done': (387, 354)}
ARROWS = {'background': ((212, 97), (248, 97)), 'tileset': ((508, 96), (543, 96)), 'layout': ((270, 161), (484, 161))}
NATIVE_KEYS = {'Enter': 'Return'}  # a move's key names are the page's (DOM); SDL's differ only here


class Failed(Exception):
    pass


class Game:
    def __init__(self, out, exe, fast):
        self.out = os.path.abspath(out)
        os.makedirs(self.out, exist_ok=True)
        self.log = open(os.path.join(self.out, 'drive.log'), 'w')
        self.cf = os.path.join(self.out, 'commands.txt')
        empty = os.path.join(self.out, 'realinput.txt')
        open(self.cf, 'w').close()
        open(empty, 'w').close()
        cmd = [exe, '-gamedir=' + os.path.abspath('assets/ningpo'), '-userdir=' + os.path.join(self.out, 'user'),
               '-commands=' + self.cf, '-realinput=' + empty] + (['-fast'] if fast else [])
        self.proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, bufsize=1)
        self.lines, self.cond = [], threading.Condition()
        self.gamelog = open(os.path.join(self.out, 'game.log'), 'w')
        threading.Thread(target=self.reader, daemon=True).start()
        self.shots = 0

    def say(self, text):
        print('%8.2f %s' % (time.time() - T0, text), file=self.log, flush=True)

    def reader(self):
        for line in self.proc.stdout:
            self.gamelog.write(line)
            self.gamelog.flush()
            line = line.rstrip('\n')
            if line.startswith('INFO: '):
                line = line[6:]
            with self.cond:
                self.lines.append(line)
                self.cond.notify_all()
        with self.cond:
            self.lines.append(None)
            self.cond.notify_all()

    def ask(self, line, timeout=30.0):
        """Sends one line and returns its answer: the STATE or MOVE value for state and move, else the OK line"""
        with self.cond:
            start = len(self.lines)
        with open(self.cf, 'a', newline='\n') as f:
            f.write(line + '\n')
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
                        return json.loads(ln[6:])
                    if line == 'move' and ln.startswith('MOVE '):
                        return json.loads(ln[5:])
                    if ln == 'OK ' + line or ln.startswith('OK ' + line):
                        return ln
                    if ln.startswith('ERROR ' + word):
                        raise Failed(ln)
                if not self.cond.wait(max(0.0, end - time.time())) and time.time() > end:
                    raise Failed('no answer to "%s"' % line)

    def state(self):
        return self.ask('state')

    def until(self, what, test, timeout=60.0):
        end = time.time() + timeout
        st = None
        while time.time() < end:
            st = self.state()
            if test(st):
                return st
            time.sleep(0.02)
        raise Failed('%s: not reached; last state %s' % (what, json.dumps(st)))

    def click(self, x, y):
        self.ask('click game %d %d' % (x, y))

    def key(self, name):
        self.ask('key ' + NATIVE_KEYS.get(name, name))

    def send(self, mv):
        for step in mv.get('input', []):
            if 'move' in step:
                self.ask('move game %d %d' % tuple(step['move']))
            elif 'click' in step:
                self.click(*step['click'])
            elif 'key' in step:
                self.key(step['key'])
            elif 'wait' in step:
                self.wait(step['wait'])
        self.wait(mv.get('updates', 1))

    def wait(self, n):
        u = self.state()['updates']
        self.until('%d updates' % n, lambda s: s['updates'] >= u + n)

    def shot(self, name):
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


def dialog(name):
    return lambda s: s['dialog'] == name


def bot_move(g, kinds, timeout=20.0):
    """Waits until the bot's move is one of these kinds and makes it"""
    end = time.time() + timeout
    while True:
        mv = g.ask('move')
        if mv is not None and mv['kind'] in kinds:
            g.send(mv)
            return mv
        if time.time() > end:
            raise Failed('no move %s; last move %s, state %s' % (kinds, json.dumps(mv), json.dumps(g.state())))
        g.wait(5)


def button(g, name, expect, test):
    g.click(*BOARD[name])
    st = g.until(expect, test)
    g.say('%s: %s' % (name, expect))
    return st


def close_dialog(g, name):
    g.until('the %s open' % name, dialog(name))
    bot_move(g, ['default', 'ok'])
    g.until('the %s closed' % name, lambda s: s['dialog'] != name)


def tour_title(g):
    st = g.until('the title', lambda s: s['screen'] == 'title' and s['dialog'] is None)
    g.shot('title')
    for what, (back, on) in ARROWS.items():
        v = st[what]
        g.click(*on)
        st = g.until(what + ' on', lambda s: s[what] != v)
        g.click(*back)
        st = g.until(what + ' back', lambda s: s[what] == v)
        g.say('title arrows: %s %d, on and back' % (what, v))
    st = button(g, 'options', 'the options', dialog('options'))
    g.shot('options')
    g.say('options buttons %s' % st['buttons'])
    if list(OPTIONS['besttimes']) not in st['buttons']:
        raise Failed('the options have no Best Times button at %s' % (OPTIONS['besttimes'],))
    g.click(*OPTIONS['besttimes'])
    g.until('the best times', dialog('besttimes'))
    g.shot('options-besttimes')
    close_dialog(g, 'besttimes')
    close_dialog(g, 'options')
    st = button(g, 'rules', 'the rules', dialog('rules'))
    g.say('rules buttons %s' % st['buttons'])
    # two pages (NingPo::showRules), one arrow at the bottom right that goes on, then back
    g.shot('rules0')
    for name in ('rules1', 'rules0-back'):
        g.click(*st['buttons'][-1])
        g.wait(5)
        g.shot(name)
    close_dialog(g, 'rules')
    g.say('rules: two pages, there and back, closed')
    g.until('the title again', lambda s: s['screen'] == 'title' and s['dialog'] is None)


def focus(g, where):
    st = g.state()
    g.ask('focus lost')
    g.wait(10)
    lost = g.state()
    g.shot('%s-focus-lost' % where)
    g.ask('focus gained')
    g.wait(10)
    back = g.state()
    g.say('focus %s: paused %s, lost %s, regained %s' % (where, st['paused'], lost['paused'], back['paused']))
    if back['paused'] and not st['paused']:  # a focus loss pauses (NingPo::setActive); a click on the board resumes
        bot_move(g, ['resume'])
        g.until('resumed', lambda s: not s['paused'])
        g.say('focus %s: resumed by a click on the board' % where)


def toggle_fullscreen(g, on):
    g.ask('keydown Left Alt')
    g.ask('key Return')
    g.ask('keyup Left Alt')
    g.until('full screen %s' % ('on' if on else 'off'), lambda s: s['fullscreen'] == on)
    g.say('full screen %s' % ('on' if on else 'off'))


def play_game(g, n, buttons, win=False):
    bot_move(g, ['play'])
    st = g.until('a deal', lambda s: s['screen'] == 'playing' and s['dialog'] is None and s['tiles'] > 0)
    g.say('game %d: layout %d, %d tiles, %d free pairs' % (n, st['layout'], st['tiles'], st['moves']))
    g.shot('game%d-deal' % n)
    pairs, tried = 0, set()
    end = time.time() + 900
    while time.time() < end:
        st = g.state()
        if st['dialog'] is not None or st['screen'] != 'playing':
            if st['dialog'] in ('nomoves', 'name') or st['screen'] in ('won', 'gameover'):
                break
            g.wait(5)
            continue
        if win and pairs == 10:
            # the bot's first-found pair never clears a board, so the last game is won by the test command (every
            # pair but the last removed, the last matched as a match does), after ten real pairs; its end screens are
            # answered by real input as any other's
            g.ask('win')
            st = g.until('the game won', lambda s: s['screen'] == 'won' or s['dialog'] is not None)
            g.say('game %d: won by the test command after %d pairs' % (n, pairs))
            break
        if buttons and pairs == 3 and 'hint' not in tried:
            tried.add('hint')
            h = st['hints']
            button(g, 'hint', 'a hint', lambda s: s['hints'] == h + 1)
            g.shot('game%d-hint' % n)
            button(g, 'showmoves', 'show moves', lambda s: True)
            g.wait(20)
            g.shot('game%d-showmoves' % n)
            button(g, 'pause', 'paused', lambda s: s['paused'])
            g.shot('game%d-paused' % n)
            button(g, 'pause', 'resumed', lambda s: not s['paused'])
            focus(g, 'game%d' % n)
            continue
        if buttons and pairs == 6 and 'undo' not in tried:
            tried.add('undo')
            u, t = st['undos'], st['tiles']
            button(g, 'undo', 'an undo', lambda s: s['undos'] == u + 1 and s['tiles'] == t + 2)
            sh = st['shuffles']
            button(g, 'shuffle', 'a shuffle', lambda s: s['shuffles'] == sh + 1)
            g.shot('game%d-shuffled' % n)
            button(g, 'newgame', 'the new-game question', dialog('question'))
            g.shot('game%d-newgame-question' % n)
            t = g.state()['tiles']
            close_dialog(g, 'question')
            st2 = g.state()
            if st2['screen'] != 'playing' or st2['tiles'] != t:
                raise Failed('New Game answered no changed the game: %s' % json.dumps(st2))
            g.say('New Game: no')
            continue
        mv = g.ask('move')
        if mv is None or mv['kind'] != 'pair':
            g.wait(5)
            continue
        t = st['tiles']
        g.send(mv)
        st = g.until('a pair matched', lambda s: s['tiles'] < t or s['dialog'] is not None or s['screen'] != 'playing')
        pairs += 1
        if pairs % 20 == 0:
            g.say('game %d: %d pairs, %d tiles left, timer %d' % (n, pairs, st['tiles'], st['timer']))
    else:
        raise Failed('game %d did not end' % n)
    g.say('game %d: %d pairs by the bot, %s, dialog %s, %d tiles left, timer %d, penalty %d' %
          (n, pairs, st['screen'], st['dialog'], st['tiles'], st['timer'], st['penalty']))
    g.shot('game%d-end' % n)
    if st['dialog'] == 'nomoves':
        bot_move(g, ['endgame'])
    bot_move(g, ['continue'])
    g.until('the name entry', dialog('name'))
    g.shot('game%d-name' % n)
    bot_move(g, ['name'])
    g.until('the best times', dialog('besttimes'))
    g.shot('game%d-besttimes' % n)
    bot_move(g, ['ok'])
    return g.until('the title', lambda s: s['screen'] == 'title' and s['dialog'] is None)


def main():
    args = sys.argv[1:]
    out, fullscreen, games, exe, fast = None, False, 2, 'build/native/ningpo.exe', True
    while args:
        a = args.pop(0)
        if a == '--fullscreen':
            fullscreen = True
        elif a == '--games':
            games = int(args.pop(0))
        elif a == '--exe':
            exe = args.pop(0)
        elif a == '--realtime':
            fast = False
        else:
            out = a
    g = Game(out, os.path.abspath(exe), fast)
    result = 'ok'
    try:
        g.until('the start screen', dialog('start'))
        g.shot('start')
        bot_move(g, ['start'])
        tour_title(g)
        focus(g, 'title')
        if fullscreen:
            toggle_fullscreen(g, True)
            g.shot('title-fullscreen')
        for n in range(1, games + 1):
            play_game(g, n, n == 1, n == games and games > 1)
        if fullscreen:
            toggle_fullscreen(g, False)
            g.shot('title-windowed')
    except Failed as e:
        result = 'FAILED: %s' % e
        try:
            g.shot('failed')
        except Failed:
            pass
    finally:
        g.close()
    g.say('RESULT ' + result)
    print('RESULT ' + result)
    return 0 if result == 'ok' else 1


T0 = time.time()
if __name__ == '__main__':
    sys.exit(main())
