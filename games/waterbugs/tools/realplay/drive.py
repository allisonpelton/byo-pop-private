"""Plays Water Bugs natively with real input only (CLAUDE.md, "Done means identical"): code plays, the session judges.

    build/native/rundesk.exe python games/waterbugs/tools/realplay/drive.py --exe <abs exe> --gamedir <abs> \
        --userdir <abs> --out <dir> [--fast] [--steps N] [--until-level N] [--pre cmd;cmd] [--at N:cmd;cmd ...]
    build/native/rundesk.exe python games/waterbugs/tools/realplay/drive.py ... --lines "state;goto options;state"

The game runs as a child of this script with -commands=<file> and an empty -realinput file (docs/REAL_INPUT.md,
docs/TEST_COMMANDS.md): every line the driver appends to the commands file is read between two updates and answered on
the log. Moves are asked from the game's own bot (`move`: where to aim the ship, turn it, fire, or the click that gets a
screen out of the way) and sent as real SDL events (`move game X Y`, `down`/`up`, `key`), so they go through HandleEvent,
the window-to-game mapping and the widget manager as a hand's do. The game's `state` tells when the game is over. Run it
under rundesk with absolute paths (the window is then on rundesk's private desktop and nothing shows or sounds).

    --pre cmd;cmd      lines sent before the first move (set shootermode 0, set difficulty 1, unlock all, level 5 ...)
    --at N:cmd;cmd     after N moves send these lines to the game (any live command or realinput line), e.g.
                       "--at 40:focus lost;wait 200;focus gained" ("wait N" waits N updates; "shot name" takes a screenshot)
    --steps N          stop after N moves (0: play until the state says over)
    --until-level N    stop when a board of level N or later is up (a game from the start through the first N-1 levels)
    --fast             the game's -fast (updates run back to back); without it the game runs in real time
    --lines a;b;c      send these lines, print every answer and quit (for looking at the game)
    --exe-arg=ARG      an argument for the game (repeatable), e.g. --exe-arg=-modsdir=<dir> or --exe-arg=-nomods

It prints one JSON line when it ends and exits 0 when the game was played to its end (or to --steps) without an error.
"""
import argparse
import json
import os
import re
import subprocess
import sys
import time

SDL_KEYS = {'ArrowUp': 'Up', 'ArrowDown': 'Down', 'ArrowLeft': 'Left', 'ArrowRight': 'Right', ' ': 'Space',
            'Enter': 'Return', 'Escape': 'Escape'}
REPLY = re.compile(r'^(?:INFO: )?(STATE|MOVE|GRID|OK|ERROR)\b\s*(.*)$')


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
            time.sleep(0.003)

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


def sdl_key(k):
    return SDL_KEYS.get(k, k)


def send_input(g, step):
    """One step of a MOVE's input (docs/TEST_COMMANDS.md) as a real-input line."""
    def pt(v):
        return (v['x'], v['y']) if isinstance(v, dict) else (v[0], v[1])
    if 'move' in step:
        x, y = pt(step['move'])
        g.cmd('move game %d %d' % (x, y))
    elif 'click' in step:
        x, y = pt(step['click'])
        g.cmd('click game %d %d %s' % (x, y, step.get('button', 'left')))
    elif 'press' in step:
        x, y = pt(step['press'])
        g.cmd('down game %d %d %s' % (x, y, step.get('button', 'left')))
    elif 'release' in step:
        x, y = pt(step['release'])
        g.cmd('up game %d %d %s' % (x, y, step.get('button', 'left')))
    elif 'wait' in step:
        g.wait_updates(int(step['wait']))
    elif 'key' in step:
        k = sdl_key(step['key'])
        if len(k) == 1:
            g.cmd('key ' + k)
        else:
            g.cmd('keydown ' + k)
            g.cmd('state')
            g.cmd('keyup ' + k)
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
            print(line, '->', kind, rest[:600])
            if kind == 'ERROR':
                raise RuntimeError('%s -> %s %s' % (line, kind, rest))


def play(a):
    g = Game(a)
    result = {'moves': 0, 'stop': None}
    shots = []
    at = {}
    for item in a.at:
        n, _, c = item.partition(':')
        at.setdefault(int(n), []).append(c)
    try:
        if a.lines:
            run_lines(g, a.lines, a, shots)
            result['stop'] = 'steps'
            result['shots'] = shots
            g.close()
            print(json.dumps(result))
            return 0
        if a.pre:
            run_lines(g, a.pre, a, shots)
        nulls = 0
        last_move, repeats = None, 0
        last_sig = None
        last_change = 0
        moves = 0
        kinds = {}
        levels = {}
        while True:
            s = g.state()
            if s['screen'] == 'board':
                levels[s['level']] = {'percent': s.get('percent'), 'lives': s['lives'], 'score': s['score']}
            if s['over']:
                result['stop'] = 'over'
                break
            if a.until_level and s['screen'] == 'board' and s['level'] >= a.until_level:
                result['stop'] = 'level'
                break
            if a.steps and moves >= a.steps:
                result['stop'] = 'steps'
                break
            sig = json.dumps({k: s.get(k) for k in ('screen', 'level', 'score', 'lives', 'percent', 'dialog')}, sort_keys=True)
            if sig != last_sig:
                last_sig, last_change = sig, s['update']
            elif s['update'] - last_change > a.stuck:
                result['stop'] = 'stuck: no change for %d updates in %s' % (a.stuck, sig[:300])
                if s['screen'] == 'board':
                    result['grid'] = g.cmd('grid')[1].split('|')
                break
            if moves in at:
                for c in at.pop(moves):
                    run_lines(g, c, a, shots)
            kind, rest = g.cmd('move')
            if kind != 'MOVE':
                raise RuntimeError('move answered %s %s' % (kind, rest))
            if rest.strip() == 'null':
                if s.get('busy'):
                    g.wait_updates(5)
                    continue
                nulls += 1
                if nulls >= 3:
                    result['stop'] = 'no move: ' + json.dumps(s)[:300]
                    break
                g.wait_updates(30)
                continue
            nulls = 0
            if rest == last_move:
                repeats += 1
                if repeats >= a.repeat:
                    result['stop'] = 'stuck: the same move %d times: %s' % (repeats, rest[:200])
                    if s['screen'] == 'board':
                        result['at_stuck'] = {k: s['board'].get(k) for k in ('ship', 'mouse', 'pointer')}
                    if s['screen'] == 'board':
                        result['grid'] = g.cmd('grid')[1].split('|')
                        probe = []
                        for line in ('move game 100 100', 'wait 5', 'state', 'move game 216 120', 'wait 5', 'state'):
                            if line.startswith('wait'):
                                g.wait_updates(5)
                            else:
                                k, r = g.cmd(line)
                                if k == 'STATE':
                                    b = json.loads(r)['board']
                                    probe.append([b['mouse'], b['ship']['x'], b['ship']['y'], b['ship']['state']])
                        result['probe'] = probe
                    break
            else:
                last_move, repeats = rest, 0
            m = json.loads(rest)
            kinds[m['kind']] = kinds.get(m['kind'], 0) + 1
            for step in m.get('input', []):
                send_input(g, step)
            moves += 1
            g.wait_updates(max(1, m.get('updates', 1)))
        result['state'] = g.state()
        result['moves'] = moves
        result['kinds'] = kinds
        result['levels'] = levels
    except Exception as e:
        result['stop'] = 'error: %s' % e
    result['errors'] = g.errors[:10]
    result['seconds'] = round(time.time() - g.t0, 1)
    result['shots'] = shots
    g.close()
    print(json.dumps(result))
    return 0 if (result['stop'] in ('over', 'steps', 'level') and not g.errors) else 1


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--exe', required=True)
    p.add_argument('--gamedir', required=True)
    p.add_argument('--userdir', required=True)
    p.add_argument('--out', required=True)
    p.add_argument('--fast', action='store_true')
    p.add_argument('--steps', type=int, default=0)
    p.add_argument('--until-level', type=int, default=0)
    p.add_argument('--stuck', type=int, default=30000)
    p.add_argument('--repeat', type=int, default=40, help='stop after this many identical moves in a row')
    p.add_argument('--at', action='append', default=[])
    p.add_argument('--pre', default='')
    p.add_argument('--lines', default='')
    p.add_argument('--exe-arg', dest='exe_args', action='append', default=[], help='an argument for the game (repeatable), e.g. --exe-arg=-modsdir=<dir>')
    sys.exit(play(p.parse_args()))
