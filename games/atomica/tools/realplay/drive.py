"""Plays Atomica natively with real input only (CLAUDE.md, "Done means identical"): code plays, the session judges.

    build/native/rundesk.exe python games/atomica/tools/realplay/drive.py --exe <abs exe> --gamedir <abs> \
        --userdir <abs> --out <dir> [--games strategy,strategy-expert,action,action-expert] [--fast] \
        [--steps N] [--at N:cmd;cmd ...] [--pre cmd;cmd] [game arguments such as -seed=1]

The game runs as a child of this script with -commands=<file> and an empty -realinput file (docs/REAL_INPUT.md,
docs/TEST_COMMANDS.md): every line the driver appends to the commands file is read between two updates and answered on the
log. Moves are asked from the port's own bot (`move`: the next click, the tutorial box's buttons, the entry box's name, or a
wait while the game is busy) and sent as real SDL events (`click game X Y`, `key B`), so they go through HandleEvent, the
window-to-game mapping and the widget manager as a hand's do. The game's `state` tells when a game is over. The title
screen's link, the menu's four start buttons, the tutorial boxes, the level boxes, the high score entry and the
"main menu" button after a game are all moves of the bot, so one run is the whole of launch to game over.

    --games a,b,c    the modes to play one after the other, in one run of the game: strategy, strategy-expert, action,
                     action-expert (the menu's four buttons). After each game the driver clicks "main menu" (the bot's
                     move) and sets the next mode for the bot's menu click
    --at N:cmd;cmd   after N moves send these lines to the game (any live command or realinput line), e.g.
                     "--at 40:focus lost;wait 300;focus gained" ("wait N" waits N updates; "shot name" takes a screenshot);
                     "--at 60:keydown Left Alt;key Return;keyup Left Alt" is Alt+Enter (full screen on and off)
    --pre cmd;cmd    lines sent before the first move
    --steps N        stop each game after N moves (0: play until the state says over)
    --idle-after N   after N moves in a game send no more on the board: the game runs on to its own end (the field fills) and
                     the bot takes over again for the entry box and the page after it
    --fast           the game's -fast (updates run back to back); without it the game runs in real time

It prints one JSON line when it ends and exits 0 when every game was played to its end (or to --steps) without an error.
"""
import argparse
import json
import os
import re
import subprocess
import sys
import time

REPLY = re.compile(r'^(?:INFO: )?(STATE|MOVE|OK|ERROR)\b\s*(.*)$')
MODES = {'strategy': ('strategy', 0), 'strategy-expert': ('strategy', 1), 'action': ('action', 0), 'action-expert': ('action', 1)}


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
    if 'click' in step:
        c = step['click']      # [x, y] (docs/TEST_COMMANDS.md); {"x", "y"} was the form before it
        x, y = (c['x'], c['y']) if isinstance(c, dict) else c
        g.cmd('click game %d %d' % (x, y))
    elif 'wait' in step:
        g.wait_updates(int(step['wait']))
    elif 'key' in step:
        g.cmd('key ' + step['key'])
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


def ask_move(g):
    kind, rest = g.cmd('move')
    if kind != 'MOVE':
        raise RuntimeError('move answered %s %s' % (kind, rest))
    return None if rest.strip() == 'null' else json.loads(rest)


def play_one(g, a, game, at, shots, moves_so_far):
    """One game from wherever the menu or the title screen stands, to the game over page."""
    mode, expert = MODES[game]
    for line in ('set mode ' + mode, 'set expert %d' % expert):
        kind, rest = g.cmd(line)
        if kind != 'OK':
            raise RuntimeError(line + ' -> ' + kind + ' ' + rest)
    res = {'game': game, 'moves': 0, 'kinds': {}, 'screens': {}, 'levels': 0}
    nulls = 0
    last_sig, last_change = None, 0
    t0 = time.time()
    while True:
        s = g.state()
        res['screens'][s['screen']] = res['screens'].get(s['screen'], 0) + 1
        res['levels'] = max(res['levels'], s.get('level', 0))
        if s['over']:
            res['stop'] = 'over'
            break
        if a.steps and res['moves'] >= a.steps:
            res['stop'] = 'steps'
            break
        sig = json.dumps({k: v for k, v in s.items() if k != 'update'}, sort_keys=True)
        if sig != last_sig:
            last_sig, last_change = sig, s['update']
        elif s['update'] - last_change > a.stuck:
            res['stop'] = 'stuck: no change for %d updates in %s' % (a.stuck, sig[:300])
            break
        n = moves_so_far + res['moves']
        if n in at:
            for c in at.pop(n):
                run_lines(g, c, a, shots)
        if a.idle_after and res['moves'] >= a.idle_after and s['screen'] == 'board':
            g.wait_updates(100)     # no more moves: the game runs to its own end (the field fills), the bot takes over after it
            continue
        m = ask_move(g)
        if m is None:
            nulls += 1
            if nulls >= 3:
                res['stop'] = 'no move: ' + json.dumps(s)[:300]
                break
            g.wait_updates(30)
            continue
        nulls = 0
        res['kinds'][m['kind']] = res['kinds'].get(m['kind'], 0) + 1
        for step in m.get('input', []):
            send_input(g, step)
        if m['kind'] != 'wait':
            res['moves'] += 1
        g.wait_updates(max(1, m.get('updates', 1)))
        if time.time() - t0 > a.timeout:
            res['stop'] = 'timeout after %d s' % a.timeout
            break
    res['state'] = g.state()
    res['seconds'] = round(time.time() - t0, 1)
    return res


def play(a):
    g = Game(a)
    result = {'games': []}
    shots = []
    at = {}
    for item in a.at:
        n, _, c = item.partition(':')
        at.setdefault(int(n), []).append(c)
    ok = True
    moves = 0
    try:
        if a.pre:
            run_lines(g, a.pre, a, shots)
        games = a.games.split(',')
        for i, game in enumerate(games):
            r = play_one(g, a, game, at, shots, moves)
            moves += r['moves']
            result['games'].append(r)
            ok = ok and r['stop'] in ('over', 'steps')
            if r['stop'] != 'over' or i == len(games) - 1:
                break
            # "main menu" is the bot's move on the page after a game; the page then shows the main page again
            m = ask_move(g)
            if m is None:
                raise RuntimeError('no move on the page after a game: ' + json.dumps(r['state'])[:300])
            for step in m.get('input', []):
                send_input(g, step)
            g.wait_updates(200)
    except Exception as e:
        ok = False
        result['error'] = 'error: %s' % e
    result['errors'] = g.errors[:10]
    result['seconds'] = round(time.time() - g.t0, 1)
    result['shots'] = shots
    g.close()
    print(json.dumps(result))
    return 0 if (ok and not g.errors) else 1


if __name__ == '__main__':
    p = argparse.ArgumentParser()
    p.add_argument('--exe', required=True)
    p.add_argument('--gamedir', required=True)
    p.add_argument('--userdir', required=True)
    p.add_argument('--out', required=True)
    p.add_argument('--games', default='strategy')
    p.add_argument('--fast', action='store_true')
    p.add_argument('--steps', type=int, default=0)
    p.add_argument('--idle-after', type=int, default=0, help='after this many moves in a game the driver only waits on the board')
    p.add_argument('--stuck', type=int, default=20000)
    p.add_argument('--timeout', type=int, default=3600)
    p.add_argument('--at', action='append', default=[])
    p.add_argument('--pre', default='')
    a, rest = p.parse_known_args()     # anything else is for the game (-seed=1)
    a.exe_args = rest
    sys.exit(play(a))
