"""Plays Noah's Ark natively with real input only (CLAUDE.md, "Done means identical"): code plays, the session judges.

    build/native/rundesk.exe python games/noahsark/tools/realplay/drive.py --exe <abs exe> --gamedir <abs> \
        --userdir <abs> --out <dir> [--mode action|strategy|puzzle] [--diff easy|medium|hard] [--fast] [--steps N] \
        [--at N:cmd;cmd ...] [--pre cmd;cmd] [-- <game args>]

The game runs as a child of this script with -commands=<file> and an empty -realinput file (docs/REAL_INPUT.md,
docs/TEST_COMMANDS.md): every line the driver appends to the commands file is read between two updates and answered on the
log. The moves come from the game's own bot (`move`: the pair, the swap or the group to click, and the click that gets a
screen out of the way: the title's link, the menu's mode button, the difficulty, the tutorial, the level-up panel, the name
entry) and are sent as real SDL events (`move game X Y`, `click game X Y`, `type B`), so they go through HandleEvent, the
window-to-game mapping and the widget manager as a hand's do. The game to start is set first (`set mode action`, `set
difficulty easy`), which the bot starts from the menu; nothing is started by `goto`. The game's `state` tells where
the game is (`screen`) and when it is over (the high-score list). Run it under rundesk with absolute paths (the window is
then on rundesk's private desktop and nothing shows or sounds).

    --at N:cmd;cmd   after N moves send these lines to the game (any live command or realinput line), e.g.
                     "--at 40:focus lost;wait 200;focus gained" ("wait N" waits N updates; "shot name" takes a screenshot);
                     full screen is "keydown Left Alt;key Return;keyup Left Alt"
    --pre cmd;cmd    lines sent before the first move
    --steps N        stop after N moves (0: play until the state says over)
    --fast           the game's -fast (updates run back to back); without it the game runs in real time
    --trace          one line a move on stderr
    --finish         after "over", one more move (the click on the list's link back to the menu)

It prints one JSON line when it ends (the screens in the order the game went through them, with the update of each) and
exits 0 when the game was played to its end (or to --steps) without an error.
"""
import argparse
import json
import os
import re
import subprocess
import sys
import time

REPLY = re.compile(r'^(?:INFO: )?(STATE|MOVE|OK|ERROR)\b\s*(.*)$')


class Game:
    def __init__(self, a):
        self.a = a
        os.makedirs(a.out, exist_ok=True)
        self.cf = os.path.join(a.out, 'commands.txt')
        self.rf = os.path.join(a.out, 'realinput.txt')
        self.logpath = os.path.join(a.out, 'game.log')
        if os.path.exists(self.cf):
            os.remove(self.cf)
        open(self.rf, 'w').close()
        self.log = open(self.logpath, 'w+b')
        args = [a.exe, '-gamedir=' + a.gamedir, '-userdir=' + a.userdir, '-commands=' + self.cf, '-realinput=' + self.rf]
        if a.fast:
            args.append('-fast')
        args += a.exe_args
        self.proc = subprocess.Popen(args, stdout=self.log, stderr=subprocess.STDOUT)
        self.pos = 0
        self.errors = []
        self.t0 = time.time()
        try:
            self.wait_text('commands: waiting for', 120)
        except Exception:
            self.proc.kill()
            raise
        open(self.cf, 'w').close()

    def lines(self):
        """New complete lines of the log since the last call."""
        with open(self.logpath, 'rb') as f:
            f.seek(self.pos)
            data = f.read()
        end = data.rfind(b'\n')
        if end < 0:
            return []
        self.pos += end + 1
        return data[:end].decode('utf-8', 'replace').splitlines()

    def wait_text(self, text, timeout):
        t = time.time()
        while time.time() - t < timeout:
            for l in self.lines():
                if text in l:
                    return l
            if self.proc.poll() is not None:
                raise RuntimeError('the game ended before "%s" (exit %s)' % (text, self.proc.returncode))
            time.sleep(0.005)
        raise RuntimeError('no "%s" within %d s' % (text, timeout))

    def cmd(self, line, timeout=60):
        """Sends one line and returns (kind, rest) of its answer."""
        with open(self.cf, 'a', newline='\n') as f:
            f.write(line + '\n')
        t = time.time()
        while time.time() - t < timeout:
            for l in self.lines():
                m = REPLY.match(l)
                if m:
                    if m.group(1) == 'ERROR':
                        self.errors.append(l)
                    return m.group(1), m.group(2)
                if 'script:' in l and 'unknown command' in l:
                    self.errors.append(l)
            if self.proc.poll() is not None:
                raise RuntimeError('the game ended while waiting for the answer to "%s"' % line)
            time.sleep(0.002)
        raise RuntimeError('no answer to "%s" within %d s' % (line, timeout))

    def state(self):
        kind, rest = self.cmd('state')
        if kind != 'STATE':
            raise RuntimeError('state answered ' + kind + ' ' + rest)
        return json.loads(rest)

    def wait_updates(self, n, timeout=120):
        u0 = self.state()['update']
        t = time.time()
        while True:
            s = self.state()
            if s['update'] >= u0 + n:
                return s
            if time.time() - t > timeout:
                raise RuntimeError('the game does not update')

    def close(self):
        try:
            self.cmd('quit', 10)
        except Exception:
            pass
        try:
            self.proc.wait(30)
        except Exception:
            self.proc.kill()
        self.log.close()


def send_input(g, step):
    if 'move' in step:
        x, y = step['move']
        g.cmd('move game %d %d' % (x, y))
    elif 'click' in step:
        x, y = step['click']
        g.cmd('click game %d %d' % (x, y))
    elif 'wait' in step:
        g.wait_updates(int(step['wait']))
    elif 'key' in step:
        k = step['key']
        g.cmd(('type ' + k) if len(k) == 1 else ('key ' + k))
    else:
        raise RuntimeError('unknown input step ' + json.dumps(step))


def run_lines(g, text, a, shots):
    for line in [x.strip() for x in text.split(';') if x.strip()]:
        if line.startswith('wait '):
            g.wait_updates(int(line.split()[1]))
        elif line.startswith('shot '):
            name = os.path.join(a.out, line.split()[1] + '.png')
            g.cmd('screenshot ' + name)
            shots.append(name)
        else:
            kind, rest = g.cmd(line)
            if kind == 'ERROR':
                raise RuntimeError('%s -> %s %s' % (line, kind, rest))


def play(a):
    g = Game(a)
    result = {'moves': 0, 'stop': None}
    shots = []
    screens = []
    at = {}
    for item in a.at:
        n, _, c = item.partition(':')
        at.setdefault(int(n), []).append(c)
    try:
        if a.pre:
            run_lines(g, a.pre, a, shots)
        nulls = 0
        last_sig = None
        last_change = 0
        moves = 0
        kinds = {}
        for line in ('set mode ' + a.mode, 'set difficulty ' + a.diff):
            kind, rest = g.cmd(line)
            if kind != 'OK':
                raise RuntimeError(line + ' -> ' + kind + ' ' + rest)
        while True:
            s = g.state()
            if not screens or screens[-1][0] != s['screen']:
                screens.append((s['screen'], s['update'], s.get('mode'), s.get('level')))
            if s['over']:
                result['stop'] = 'over'
                break
            if a.steps and moves >= a.steps:
                result['stop'] = 'steps'
                break
            sig = json.dumps({k: v for k, v in s.items() if k not in ('update', 'water')}, sort_keys=True)
            if sig != last_sig:
                last_sig, last_change = sig, s['update']
            elif s['update'] - last_change > a.stuck:
                result['stop'] = 'stuck: no change for %d updates in %s' % (a.stuck, sig[:300])
                break
            if moves in at:
                for c in at.pop(moves):
                    run_lines(g, c, a, shots)
            kind, rest = g.cmd('move')
            if kind != 'MOVE':
                raise RuntimeError('move answered %s %s' % (kind, rest))
            if rest.strip() == 'null':
                nulls += 1
                if nulls >= 3:
                    result['stop'] = 'no move: ' + json.dumps(s)[:300]
                    break
                g.wait_updates(30)
                continue
            nulls = 0
            m = json.loads(rest)
            kinds[m['kind']] = kinds.get(m['kind'], 0) + 1
            if a.trace:
                print('move %d at update %d (%s): %s animals %s phase %s' % (moves, s['update'], s['screen'], m['kind'], s.get('animals'), s.get('phase')), file=sys.stderr, flush=True)
            for step in m.get('input', []):
                send_input(g, step)
            moves += 1
            g.wait_updates(max(1, m.get('updates', 1)))
        result['state'] = g.state()
        if a.finish and result['stop'] == 'over':
            kind, rest = g.cmd('move')
            if kind == 'MOVE' and rest.strip() != 'null':
                for step in json.loads(rest).get('input', []):
                    send_input(g, step)
                g.wait_updates(60)
                result['after'] = g.state()['screen']
        result['moves'] = moves
        result['kinds'] = kinds
    except Exception as e:
        result['stop'] = 'error: %s' % e
    result['screens'] = screens
    result['errors'] = g.errors[:10]
    result['seconds'] = round(time.time() - g.t0, 1)
    result['shots'] = shots
    g.close()
    print(json.dumps(result))
    return 0 if (result['stop'] in ('over', 'steps') and not g.errors) else 1


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--exe', required=True)
    p.add_argument('--gamedir', required=True)
    p.add_argument('--userdir', required=True)
    p.add_argument('--out', required=True)
    p.add_argument('--mode', default='action')
    p.add_argument('--diff', default='easy')
    p.add_argument('--fast', action='store_true')
    p.add_argument('--steps', type=int, default=0)
    p.add_argument('--stuck', type=int, default=20000)
    p.add_argument('--at', action='append', default=[])
    p.add_argument('--pre', default='')
    p.add_argument('--finish', action='store_true')
    p.add_argument('--trace', action='store_true')
    p.add_argument('exe_args', nargs='*')
    sys.exit(play(p.parse_args()))
