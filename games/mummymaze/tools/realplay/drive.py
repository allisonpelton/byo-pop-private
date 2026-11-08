"""Plays Mummy Maze natively with real input only (CLAUDE.md, "Done means identical"): code plays, the session judges.

    build/native/rundesk.exe python games/mummymaze/tools/realplay/drive.py --exe <abs exe> --gamedir <abs> \
        --userdir <abs> --out <dir> [--mode classic|adventure|tutorial] [--fast] [--stepinput keys|clicks] \
        [--steps N] [--at N:cmd;cmd ...] [--pre cmd;cmd]

The game runs as a child of this script with -commands=<file> and an empty -realinput file (docs/REAL_INPUT.md,
docs/TEST_COMMANDS.md): every line the driver appends to the commands file is read between two updates and answered on the
log. Moves are asked from the game's own bot (`move`: the solver's next step, or the click that gets a screen out of the
way) and sent as real SDL events (`click game X Y`, `keydown`/`keyup`), so they go through HandleEvent, the window-to-game
mapping and the widget manager as a hand's do. The game's `state` tells when a move was taken and when the game is over.
Run it under rundesk with absolute paths (the window is then on rundesk's private desktop and nothing shows or sounds).

    --at N:cmd;cmd   after N moves send these lines to the game (any live command or realinput line), e.g.
                     "--at 40:focus lost;wait 200;focus gained" ("wait N" waits N updates; "shot name" takes a screenshot)
    --pre cmd;cmd    lines sent before the first move (set botmode adventure, goto menu, ...)
    --steps N        stop after N moves (0: play until the state says over)
    --fast           the game's -fast (updates run back to back); without it the game runs in real time
    --stepinput      keys (the default; each key is held for one update, which needs real time or a short -fast latency)
                     or clicks (a click on the cell the explorer goes to)

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


def sdl_key(k):
    return SDL_KEYS.get(k, k)


def send_input(g, step, a):
    if 'click' in step:
        c = step['click']
        g.cmd('click game %d %d' % (c['x'], c['y']))
    elif 'key' in step:
        k = sdl_key(step['key'])
        if len(k) == 1:
            g.cmd('key ' + k)
        else:
            # held for at least one update: a `state` between the press and the release is answered at the next update
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
        if a.pre:
            run_lines(g, a.pre, a, shots)
        for line in ('set botmode ' + a.mode, 'set stepinput ' + a.stepinput):
            kind, rest = g.cmd(line)
            if kind != 'OK':
                raise RuntimeError(line + ' -> ' + kind + ' ' + rest)
        nulls = 0
        last_sig = None
        last_change = 0
        moves = 0
        kinds = {}
        while True:
            s = g.state()
            if s['over']:
                result['stop'] = 'over'
                break
            if s.get('moves', 0) > a.maxmoves:
                result['stop'] = 'loop in a chamber: %d moves taken in %s' % (s['moves'], json.dumps({k: s[k] for k in ('mode', 'level', 'pyramid', 'chamber')}))
                break
            if a.steps and moves >= a.steps:
                result['stop'] = 'steps'
                break
            sig = json.dumps({k: v for k, v in s.items() if k not in ('update', 'timer')}, sort_keys=True)
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
            u0 = s['update']
            for step in m.get('input', []):
                send_input(g, step, a)
            moves += 1
            # a step of the explorer: wait until the board is ready again; any other move: the updates it asks for
            need = max(1, m.get('updates', 1))
            if m['kind'] == 'step':
                t = time.time()
                while True:
                    s2 = g.state()
                    if s2['screen'] != 'board' or (s2.get('board', {}).get('ready') and s2['update'] >= u0 + 3):
                        # the explorer must stand where the move said (a key held too long takes a second step, a click
                        # that missed takes none)
                        b2 = s2.get('board', {})
                        if s2['screen'] == 'board' and 'to' in m and b2.get('explorer') != m['to'] and s2['mode'] in ('classic', 'adventure'):
                            result.setdefault('offtrack', []).append({'move': moves, 'want': m['to'], 'have': b2.get('explorer')})
                        break
                    if time.time() - t > a.timeout:
                        raise RuntimeError('the explorer did not finish a step: ' + json.dumps(s2)[:300])
            else:
                g.wait_updates(need)
        result['state'] = g.state()
        if a.finish and result['stop'] == 'over':
            kind, rest = g.cmd('move')
            if kind == 'MOVE' and rest.strip() != 'null':
                for step in json.loads(rest).get('input', []):
                    send_input(g, step, a)
                g.wait_updates(60)
                result['after'] = g.state()['screen']
        result['moves'] = moves
        result['kinds'] = kinds
    except Exception as e:
        result['stop'] = 'error: %s' % e
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
    p.add_argument('--mode', default='classic')
    p.add_argument('--fast', action='store_true')
    p.add_argument('--stepinput', default='keys')
    p.add_argument('--steps', type=int, default=0)
    p.add_argument('--stuck', type=int, default=20000)
    p.add_argument('--timeout', type=int, default=120)
    p.add_argument('--maxmoves', type=int, default=500)
    p.add_argument('--at', action='append', default=[])
    p.add_argument('--pre', default='')
    p.add_argument('--finish', action='store_true')
    p.add_argument('exe_args', nargs='*')
    sys.exit(play(p.parse_args()))
