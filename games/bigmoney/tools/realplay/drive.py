"""Plays Big Money natively with real input only (CLAUDE.md, "Done means identical"): code plays, the session judges.

    build/native/rundesk.exe python games/bigmoney/tools/realplay/drive.py --exe <abs exe> --gamedir <abs> \
        --userdir <abs> --out <dir> [--plan action:1,strategy:2,puzzle:1] [--fast] [--steps N] \
        [--at N:cmd;cmd ...] [--pre cmd;cmd] [--shots]

The game runs as a child of this script with -commands=<file> and an empty -realinput file (docs/REAL_INPUT.md,
docs/TEST_COMMANDS.md): every line the driver appends to the commands file is read between two updates and answered on the
log. Moves are asked from the game's own bot (`move`: the click that gets the title, the selector, the help screen, a dialog
or the level-done screen out of the way, the solver's click in Puzzle, the best group elsewhere, the name of a high score)
and sent as real SDL events (`click game X Y`, `key`, `type`), so they go through HandleEvent, the window-to-game mapping and
the widget manager as a hand's do. The game's `state` tells when a game is over. Run it under rundesk with absolute paths (the
window is then on rundesk's private desktop and nothing shows or sounds).

    --plan m:s,...   the games to play, in order: a mode (action, strategy, puzzle) and its skill (1 to 3). Before each game the
                     driver names the mode and skill the selector will be given (`set mode`, `set skill`); it plays each to its
                     end ("play again" starts the next). Default: action:1
    --at N:cmd;cmd   after N moves send these lines to the game (any live command or realinput line), e.g.
                     "--at 40:focus lost;wait 200;focus gained" ("wait N" waits N updates; "shot name" takes a screenshot)
    --pre cmd;cmd    lines sent before the first move
    --steps N        stop after N moves (0: play until the plan is done)
    --fast           the game's -fast (updates run back to back); without it the game runs in real time
    --shots          a screenshot the first time each (screen, board state, dialog) is seen, in the output folder
    --cap N          updates a game is played to win; after that `set bot lose` has the bot play to lose, so that the game ends
                     by itself (Action: the stack is left to rise; Puzzle: the largest group in place of the solver's plan;
                     default 12000, 0 never; a bot that never errs would play an Easy game for hours)
    --stuck N        stop when the state (but for the updates) has not changed for N updates (default 20000)

It prints one JSON line when it ends and exits 0 when the plan was played to its end (or to --steps) without an error.
"""
import argparse
import json
import os
import re
import subprocess
import sys
import time

SDL_KEYS = {'Enter': 'Return', 'ArrowUp': 'Up', 'ArrowDown': 'Down', 'ArrowLeft': 'Left', 'ArrowRight': 'Right', ' ': 'Space'}
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
            time.sleep(0.0005)
        raise RuntimeError('no answer to "%s" within %d s' % (line, timeout))

    def state(self):
        kind, rest = self.cmd('state')
        if kind != 'STATE':
            raise RuntimeError('state answered ' + kind + ' ' + rest)
        return json.loads(rest)

    def wait_updates(self, n, timeout=300):
        u0 = self.state()['updates']
        t = time.time()
        while True:
            s = self.state()
            if s['updates'] >= u0 + n:
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


def send_move(g, m):
    """The move's input steps (docs/TEST_COMMANDS.md): clicks, keys (runs of single characters are typed, Shift for
    capitals, named keys pressed), waits."""
    text = ''
    for st in m.get('input', []) + [None]:
        if st is not None and 'key' in st and len(st['key']) == 1 and st['key'] != ' ':
            text += st['key']
            continue
        if text:
            g.cmd('type ' + text)
            text = ''
        if st is None:
            break
        if 'click' in st:
            g.cmd('click game %d %d' % tuple(st['click']))
        elif 'key' in st:
            g.cmd('key ' + sdl_key(st['key']))
        elif 'wait' in st:
            g.wait_updates(int(st['wait']))
        else:
            raise RuntimeError('unknown input step ' + json.dumps(st))


WINDOW = {'last': None, 'moves': 0}


def note_window(result, s):
    """The window's, the focus's and the pause's changes (full screen and focus tests), by the move they came at."""
    win = (s.get('windowed'), s.get('focus'), s.get('paused'))
    if win != WINDOW['last']:
        result.setdefault('window', []).append({'move': WINDOW['moves'], 'updates': s['updates'], 'windowed': win[0],
                                                'focus': win[1], 'paused': win[2]})
        WINDOW['last'] = win


def run_lines(g, text, a, shots, result):
    for line in [x.strip() for x in text.split(';') if x.strip()]:
        if line.startswith('wait '):
            note_window(result, g.wait_updates(int(line.split()[1])))
        elif line.startswith('shot '):
            name = os.path.join(a.out, line.split()[1] + '.png')
            g.cmd('screenshot ' + name)
            shots.append(name)
        else:
            kind, rest = g.cmd(line)
            if kind == 'ERROR':
                raise RuntimeError('%s -> %s %s' % (line, kind, rest))


def parse_plan(text):
    plan = []
    for item in text.split(','):
        mode, _, skill = item.partition(':')
        plan.append((mode.strip(), int(skill or 1)))
    return plan


def play(a):
    g = Game(a)
    result = {'moves': 0, 'stop': None, 'games': []}
    shots = []
    seen = set()
    at = {}
    for item in a.at:
        n, _, c = item.partition(':')
        at.setdefault(int(n), []).append(c)
    plan = parse_plan(a.plan)
    try:
        if a.pre:
            run_lines(g, a.pre, a, shots, result)
        game_no = 0
        counted = False
        game_start = None
        losing = False
        levels = {}
        moves = 0
        kinds = {}
        nulls = 0
        last_sig, last_change = None, 0
        max_level = 0
        for line in ('set mode ' + plan[0][0], 'set skill %d' % plan[0][1]):
            kind, rest = g.cmd(line)
            if kind != 'OK':
                raise RuntimeError(line + ' -> ' + kind + ' ' + rest)
        while True:
            s = g.state()
            if a.shots:
                key = (s['screen'], s.get('boardState'), s.get('dialog'))
                if key not in seen:
                    seen.add(key)
                    name = os.path.join(a.out, 'seen-%02d-%s-%s-d%s.png' % (len(seen), s['screen'].replace(' ', '_'),
                                        str(s.get('boardState')).replace(' ', '_'), s.get('dialog')))
                    g.cmd('screenshot ' + name)
                    shots.append(name)
            WINDOW['moves'] = moves
            note_window(result, s)
            if s['screen'] == 'board':
                if game_start is None:
                    game_start = s['updates']
                if a.cap and not losing and s['updates'] - game_start > a.cap:
                    kind, rest = g.cmd('set bot lose')
                    if kind != 'OK':
                        raise RuntimeError('set bot lose -> ' + kind + ' ' + rest)
                    losing = True
                    result.setdefault('lostafter', []).append(s['updates'] - game_start)
                max_level = max(max_level, s.get('level', 0))
                levels[game_no] = max(levels.get(game_no, 0), s.get('level', 0))
            if s['over']:
                if not counted:
                    counted = True
                    result['games'].append({'mode': s['mode'], 'skill': s['skill'], 'level': s.get('level'),
                                            'score': s.get('score'), 'updates': s['updates']})
                    game_no += 1
                    game_start = None
                    if losing:
                        g.cmd('set bot win')
                        losing = False
                    if game_no >= len(plan):
                        result['stop'] = 'over'
                        break
                    mode, skill = plan[game_no]
                    for line in ('set mode ' + mode, 'set skill %d' % skill):
                        kind, rest = g.cmd(line)
                        if kind != 'OK':
                            raise RuntimeError(line + ' -> ' + kind + ' ' + rest)
                    if (mode, skill) != (s['mode'], s['skill']):
                        # "play again" starts the same game again: another mode or skill goes back to the selector,
                        # whose clicks are real input as the rest
                        kind, rest = g.cmd('goto selector')
                        if kind != 'OK':
                            raise RuntimeError('goto selector -> ' + kind + ' ' + rest)
                        continue
            else:
                counted = False
            if a.steps and moves >= a.steps:
                result['stop'] = 'steps'
                break
            sig = json.dumps({k: v for k, v in s.items() if k not in ('updates', 'go')}, sort_keys=True)
            if sig != last_sig:
                last_sig, last_change = sig, s['updates']
            elif s['updates'] - last_change > a.stuck:
                result['stop'] = 'stuck: no change for %d updates in %s' % (a.stuck, sig[:300])
                break
            if moves in at:
                for c in at.pop(moves):
                    run_lines(g, c, a, shots, result)
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
            kname = m['kind'].split(':')[0].split(' ')[0]
            kinds[kname] = kinds.get(kname, 0) + 1
            send_move(g, m)
            moves += 1
            g.wait_updates(max(1, m.get('updates', 1)))
        result['state'] = g.state()
        result['moves'] = moves
        result['kinds'] = kinds
        result['maxlevel'] = max_level
    except Exception as e:
        result['stop'] = 'error: %s' % e
    result['errors'] = g.errors[:10]
    result['seconds'] = round(time.time() - g.t0, 1)
    result['shots'] = shots
    g.close()
    print(json.dumps(result))
    return 0 if (result['stop'] in ('over', 'steps') and not g.errors) else 1


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--exe', required=True)
    p.add_argument('--gamedir', required=True)
    p.add_argument('--userdir', required=True)
    p.add_argument('--out', required=True)
    p.add_argument('--plan', default='action:1')
    p.add_argument('--fast', action='store_true')
    p.add_argument('--steps', type=int, default=0)
    p.add_argument('--at', action='append', default=[])
    p.add_argument('--pre', default='')
    p.add_argument('--shots', action='store_true')
    p.add_argument('--stuck', type=int, default=20000)
    p.add_argument('--cap', type=int, default=12000)
    p.add_argument('exe_args', nargs='*')
    a = p.parse_args()
    sys.exit(play(a))


if __name__ == '__main__':
    main()
