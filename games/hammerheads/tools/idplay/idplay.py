#!/usr/bin/env python3
"""Phase one of the identical-play pass: the original and the port get the same inputs, and the game's state is compared
(docs/COORDINATION.md; games/hammerheads/docs/IDENTICAL_PLAY.md).

The original runs under origrun (tools/origrun, docs/ORIGRUN.md) on its virtual clock with its random seed pinned
(games/hammerheads/tools/origrun/patches.txt). At every update count origrun holds the game and reads the values below with a
`peek` line. The port runs the same inputs with the `trace` test command, which writes a TRACE line with the same names.

The framework's frame (SexyAppBase::Process) is three steps: the first runs one tick (Update); the second runs the ticks the
frame still owes (the 60 Hz frame owes 5 ticks in 3 frames) and then the frame's UpdateF; the third waits. origrun's hold lands
between steps, so a record is one of two kinds, told by the original's update state (+0x450) and its pending-tick sum (+0x428):
  T  the first step's tick has run and the frame's UpdateF has not (state 2);
  F  the frame is over: its UpdateF has run (state 3, the sum below 1.0).
A record in the middle of the second step (state 3, the sum 1.0 or more) is not compared. The port logs a T line after each
first-step tick and an F line after each frame's UpdateF (Application::TraceRecord), so a record is looked up by its kind and
the update count. Only the T records are compared: an F record of the original was seen taken inside the frame's UpdateF (the
gnome's pop had run and the level's time had not), so where the hold lands for a count reached in the second step is not known,
and a frame's UpdateF shows in the next T record anyway. An `at N` input lands at the first hold with the count at N or more; one at a multiple of 5 lands where a frame
is over, so an input's N is a multiple of 5, and the port runs it at the next update (N + 1), where the original reads the message.

    idplay.py run <scenario> [--end N]     the original's run, the port's, and their comparison
    idplay.py compare <scenario>           the comparison of runs that were made
    idplay.py original <scenario>          only the original's run
    idplay.py port <scenario>              only the port's run
    idplay.py generate <scenario>          the port plays the scenario's prefix and then its bot's clicks (`botclick`), which are
                                           written beside the scenario as <scenario>.clicks and replayed by the other commands

A scenario is a python file under scenarios/ that defines INPUTS, a list of (update count, original command, port command)
with each count a multiple of 5 (the commands are origrun's script commands and the framework's -script commands), and END, the
last update count compared. It may define SEED (default 1). Instead of INPUTS it may define PREFIX (the same kind of list: the
loading screen, the name, the menu, the level's introduction) and BOT_FROM, BOT_STEP and BOT_SEED: `generate` then has the port's
bot make a click at each BOT_STEP updates from BOT_FROM (now and then a bomb or a miss, drawn from BOT_SEED), and the clicks it
made are the rest of the inputs.
"""

import argparse
import importlib.util
import os
import random
import re
import shutil
import struct
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
RUN_DIR = os.path.join(ROOT, 'build', 'logs', 'idplay')

APP = '0x5cea54'                               # the application's global
COUNTER = '[%s]+0x44c' % APP
ORIGRUN = os.path.join(ROOT, 'build', 'origrun-bin', 'origrun.exe')
RUNDESK = os.path.join(ROOT, 'build', 'native', 'rundesk.exe')
PORT_EXE = os.path.join(ROOT, 'build', 'native', 'hammerheads.exe')
ORIGINAL_EXE = os.environ.get('HAMMERHEADS_ORIGINAL', r'C:\byo-og\hh2\game\HammerHeadsGame.exe')
STAGE_ROOT = os.environ.get('HAMMERHEADS_STAGE_ROOT', r'C:\byo-og')
PATCHES = os.path.join(ROOT, 'games', 'hammerheads', 'tools', 'origrun', 'patches.txt')
PEEK_LINE_LENGTH = 880                         # origrun logs a peek line up to about 1000 characters, the values in it too, and reads a
PEEKS_PER_LINE = 30                            # peek of up to 30 expressions: a longer line loses its tail
PEEK_VALUE_LENGTH = 13                         # what a value adds to its expression in the log: '=-1717986917 '

GAMEPLAY = '0x597f5c'                          # where the Gameplay object's pointer is (the global ObjectRef at 0x597f40 and its pointer at +0x1c)
GAME_SCREEN = '[%s]+0x728' % APP
TIMER = '0x59807c'                             # the pointer of the level's Timer (the global ObjectRef at 0x598060 and its pointer at +0x1c)
STATS_FIRST = '[0x597f2c]+0'                   # the pointer to the first double (the Stats object is that pointer and more)


TRACE_CUBBIES = 10                              # the cubbies a record lists
AVATAR = '[%s]+0x1e0' % GAME_SCREEN            # the hammer's pointer (the game screen's ObjectRef at +0x1c4 and its pointer at +0x1c)


def cubby(index):
    """the expression of the pointer to the Cubby at an index of the Gameplay's list (an array of 0x20-byte ObjectRefs at +0x84)"""
    return '[[%s]+0x84]+0x%x' % (GAMEPLAY, 0x20 * index + 0x1c)


def creature(index):
    """the expression of the pointer to the creature in that cubby (Cubby +0xb8)"""
    return '[%s]+0xb8' % cubby(index)


def doubles(base, offset):
    """the two ints of the double at `offset` past the pointer `base` evaluates to"""
    return ['[%s]+0x%x' % (base, offset), '[%s]+0x%x' % (base, offset + 4)]


# (name, [expressions]) in the order the port prints them: a double is its low int then its high int
FIELDS = [
    ('mti', ['0x5ce810']),
    ('mt0', ['0x5cde50']),
    ('mt623', ['0x5ce80c']),
    ('hasstats', ['0x597f2c']),                # the pointers, which are 0 until the object is made
] + [('s%d' % i, doubles(STATS_FIRST, 8 * i)) for i in range(50)] + [
    ('gs', [GAME_SCREEN]),
    ('hasgp', [GAMEPLAY]),
    ('level', ['[%s]+0x200' % GAME_SCREEN]),
    ('mode', ['[%s]+0x23c' % GAME_SCREEN]),
    ('cycle', ['[%s]+0x238' % GAME_SCREEN]),
    ('kindindex', ['[%s]+0x1fc' % GAME_SCREEN]),
    ('leveltime', doubles(GAME_SCREEN, 0x240)),
    ('phase', ['[%s]+0x94' % GAMEPLAY]),
    ('phasetimer', doubles(GAMEPLAY, 0x98)),
    ('nextpop', doubles(GAMEPLAY, 0xa0)),
    ('clover', doubles(GAMEPLAY, 0xa8)),
    ('strawberry', doubles(GAMEPLAY, 0xb0)),
    ('doublehit', doubles(GAMEPLAY, 0xb8)),
    ('fivetimes', doubles(GAMEPLAY, 0xc0)),
    ('timerphase', ['[%s]+0x98' % TIMER]),
    ('timerleft', doubles(TIMER, 0x88)),
    ('timerrate', doubles(TIMER, 0x90)),
    ('ncubbies', ['[%s]+0x8c' % GAMEPLAY]),
] + [item for i in range(TRACE_CUBBIES) for item in (
    ('cb%d_state' % i, ['[%s]+0xa0' % cubby(i)]),
    ('cb%d_rise' % i, doubles(cubby(i), 0xa8)),
    ('cb%d_kind' % i, ['[%s]+0xc' % creature(i)]),
    ('cb%d_cstate' % i, ['[%s]+0x8' % creature(i)]),
    ('cb%d_age' % i, ['[%s]+0x70' % creature(i)]),
    ('cb%d_height' % i, doubles(creature(i), 0x40)),
    ('cb%d_hvel' % i, doubles(creature(i), 0x48)),
    ('cb%d_x' % i, doubles(creature(i), 0x88)),
    ('cb%d_y' % i, doubles(creature(i), 0x90)))] + [
    ('swing', ['[%s]+0xc0' % AVATAR]),
    ('swingframe', doubles(AVATAR, 0xf8)),
    ('clickcount', ['[%s]+0x138' % AVATAR]),
]
GAME_FIELDS = {'hasgp', 'level', 'mode', 'cycle', 'kindindex', 'leveltime', 'phase', 'phasetimer', 'nextpop', 'clover', 'strawberry',
               'doublehit', 'fivetimes', 'timerphase', 'timerleft', 'timerrate', 'ncubbies', 'swing', 'swingframe',
               'clickcount'} | {'cb%d_%s' % (i, name) for i in range(TRACE_CUBBIES) for name in
                                ('state', 'rise', 'kind', 'cstate', 'age', 'height', 'hvel', 'x', 'y')}
# fields that are a pointer in the original and a flag in the port: whether the object exists is what is compared
BOOLEAN_FIELDS = {'gs', 'hasstats', 'hasgp'}
# the update state machine, read for the kind of record (not compared)
KIND_EXPRESSIONS = ['[%s]+0x450' % APP, '[%s]+0x428' % APP, '[%s]+0x42c' % APP]
ALL_EXPRESSIONS = KIND_EXPRESSIONS + [e for _, exprs in FIELDS for e in exprs]


def clicks_path(path):
    return os.path.splitext(path)[0] + '.clicks'


def read_clicks(path):
    """the clicks a scenario's generating run made: (count, x, y) lines of its .clicks file"""
    clicks = []
    if os.path.exists(path):
        for line in open(path):
            words = line.split()
            if len(words) == 3:
                clicks.append(tuple(int(word) for word in words))
    return clicks


def load_scenario(name):
    path = name if name.endswith('.py') else os.path.join(os.path.dirname(os.path.abspath(__file__)), 'scenarios', name + '.py')
    spec = importlib.util.spec_from_file_location('scenario', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.PATH = path
    if not hasattr(module, 'INPUTS'):
        # PREFIX: the fixed inputs (the loading screen, the name, the menu, the level's introduction); the rest are the bot's clicks,
        # written by `generate` (the port's `botclick` command at each BOT_STEP updates from BOT_FROM to END)
        module.INPUTS = list(module.PREFIX)
        for count, x, y in read_clicks(clicks_path(path)):
            module.INPUTS.append((count, 'move %d %d' % (x, y), 'move %d %d' % (x, y)))
            module.INPUTS.append((count, 'click %d %d' % (x, y), 'click %d %d' % (x, y)))
        module.INPUTS.sort(key=lambda item: item[0])
    for count, _, _ in module.INPUTS:
        if count % 5:
            raise SystemExit('input at %d: the count must be a multiple of 5' % count)
    return module


def scenario_name(name):
    return os.path.splitext(os.path.basename(name))[0]


def original_script(scenario, end):
    inputs = {}
    for count, original_command, _ in scenario.INPUTS:
        if original_command:
            inputs.setdefault(count, []).append(original_command)
    lines = ['waitfor %s > 0 120000' % COUNTER]
    chunks = [[]]
    for expression in ALL_EXPRESSIONS:
        if chunks[-1] and (len(chunks[-1]) >= PEEKS_PER_LINE or sum(len(e) + PEEK_VALUE_LENGTH for e in chunks[-1] + [expression]) > PEEK_LINE_LENGTH):
            chunks.append([])
        chunks[-1].append(expression)
    for count in range(1, end + 1):
        for chunk in chunks:
            lines.append('at %d peek %s' % (count, ' '.join(chunk)))
        for command in inputs.get(count, []):
            lines.append('at %d %s' % (count, command))
    lines.append('quit')
    return '\n'.join(lines) + '\n'


def run_original(name, scenario, end):
    run = os.path.join(RUN_DIR, name)
    os.makedirs(run, exist_ok=True)
    script = os.path.join(run, 'original.txt')
    open(script, 'w').write(original_script(scenario, end))
    log = os.path.join(run, 'original.log')
    # the original keeps its players in users\ beside the exe; a player left by the last run changes the flow (no name box), so
    # every run starts with none, as the port's run does with its fresh user folder
    shutil.rmtree(os.path.join(os.path.dirname(ORIGINAL_EXE), 'users'), ignore_errors=True)
    command = [ORIGRUN, '--exe', ORIGINAL_EXE, '--run', 'hh2', '--stage-root', STAGE_ROOT, '--script', script, '--clock', 'virtual',
               '--reg-map', 'popcap', '--counter', COUNTER, '--patches', PATCHES, '--seed', str(getattr(scenario, 'SEED', 1)),
               '--timeout', str(max(120, end // 10))]
    with open(log, 'w') as out:
        status = subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=ROOT)
    print('original: origrun exited with', status, '->', log)
    return log


def port_script(scenario, end, trace_file):
    lines = ['1 trace %s' % trace_file.replace('\\', '/')]
    for count, _, port_command in scenario.INPUTS:
        if port_command:
            lines.append('%d %s' % (count + 1, port_command))
    lines.append('%d quit' % (end + 10))
    return '\n'.join(lines) + '\n'


def run_port_script(name, script_text, scenario):
    """the port with a script; its log is returned (and the trace is port.trace in the run's folder)"""
    run = os.path.join(RUN_DIR, name)
    os.makedirs(run, exist_ok=True)
    script = os.path.join(run, 'port.txt')
    open(script, 'w').write(script_text)
    user_dir = os.path.join(run, 'portuser')
    shutil.rmtree(user_dir, ignore_errors=True)
    os.makedirs(user_dir)
    log = os.path.join(run, 'port.log')
    command = [RUNDESK, PORT_EXE, '-fast', '-seed=%s' % getattr(scenario, 'SEED', 1), '-userdir=' + user_dir, '-script=' + script]
    with open(log, 'w') as out:
        status = subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=ROOT)
    print('port: exited with', status, '->', log)
    return log


def run_port(name, scenario, end):
    trace = os.path.join(RUN_DIR, name, 'port.trace')
    run_port_script(name, port_script(scenario, end, trace), scenario)
    return trace


def generate(name, scenario, end):
    """the port plays the scenario's prefix and then the bot's clicks, and the clicks it made are written to the .clicks file"""
    rng = random.Random(getattr(scenario, 'BOT_SEED', 1))
    lines = []
    for count, _, port_command in scenario.PREFIX:
        if port_command:
            lines.append((count + 1, port_command))
    for count in range(scenario.BOT_FROM, end + 1, scenario.BOT_STEP):
        chance = rng.random()
        mode = ' bomb' if chance < 0.03 else ' miss' if chance < 0.045 else ''
        lines.append((count + 1, 'botclick' + mode))
    lines.sort(key=lambda item: item[0])
    script = ''.join('%d %s\n' % item for item in lines) + '%d quit\n' % (end + 10)
    log = run_port_script(name, script, scenario)
    clicks = []
    for line in open(log, encoding='utf-8', errors='replace'):
        match = re.search(r'BOTCLICK (\d+) (-?\d+) (-?\d+)', line)
        if match:
            clicks.append((int(match.group(1)) - 1, int(match.group(2)), int(match.group(3))))
    with open(clicks_path(scenario.PATH), 'w') as out:
        for click in clicks:
            out.write('%d %d %d\n' % click)
    print('generate: %d clicks -> %s' % (len(clicks), clicks_path(scenario.PATH)))


def to_signed(value):
    value &= 0xffffffff
    return value - (1 << 32) if value & 0x80000000 else value


def parse_original(path):
    """(count, kind) -> {name: tuple of ints}: the peek lines of one hold (the same count) are one record"""
    line_re = re.compile(r'peek u=(\d+) (.*)$')
    records = {}
    pending = {}
    pending_u = None

    def flush():
        if pending_u is None or not pending:
            return
        state = pending.get(KIND_EXPRESSIONS[0])
        acc_high = pending.get(KIND_EXPRESSIONS[2])
        kind = None
        if state == 2:
            kind = 'T'
        elif state == 3 and acc_high is not None and acc_high < 0x3ff00000:
            kind = 'F'
        if kind is None:
            return
        record = {}
        for field, expressions in FIELDS:
            record[field] = tuple(pending.get(e, 0) for e in expressions)
        records[(pending_u, kind)] = record

    for line in open(path, encoding='utf-8', errors='replace'):
        match = line_re.search(line)
        if not match:
            continue
        count = int(match.group(1))
        if count != pending_u:
            flush()
            pending.clear()
            pending_u = count
        for item in match.group(2).split(' '):
            if '=' not in item:
                continue
            expression, value = item.rsplit('=', 1)
            pending[expression] = 0 if value == '-' else to_signed(int(value))
    flush()
    return records


def parse_port(path):
    records = {}
    for line in open(path, encoding='utf-8', errors='replace'):
        if not line.startswith('TRACE '):
            continue
        words = line.split()
        kind = words[1]
        record = {}
        count = None
        for item in words[2:]:
            name, value = item.split('=')
            numbers = tuple(int(part) for part in value.split(','))
            if name == 'c':
                count = numbers[0]
            else:
                record[name] = numbers
        records[(count, kind)] = record
    return records


def describe(name, values):
    if len(values) == 2:
        double = struct.unpack('<d', struct.pack('<ii', values[0], values[1]))[0]
        return '%s=%r' % (name, double)
    return '%s=%d' % (name, values[0])


def compare(name, scenario, end):
    run = os.path.join(RUN_DIR, name)
    original = parse_original(os.path.join(run, 'original.log'))
    port = parse_port(os.path.join(run, 'port.trace'))
    if not original:
        print('the original left no records (did the run reach its updates?)')
        return 2
    compared = 0
    missing = []
    for key in sorted(original):
        count, kind = key
        if count > end or kind != 'T':
            continue
        if key not in port:
            missing.append(key)
            continue
        ours, theirs = original[key], port[key]
        has_game = ours['gs'][0] != 0 and theirs['gs'][0] != 0
        bad = []
        board_cubbies = min(ours['ncubbies'][0], theirs.get('ncubbies', (0,))[0]) if has_game else 0
        for field, _ in FIELDS:
            if field in GAME_FIELDS and not has_game:
                continue
            cubby_field = re.match(r'cb(\d+)_', field)
            if cubby_field and int(cubby_field.group(1)) >= board_cubbies:
                continue
            if field in BOOLEAN_FIELDS:
                if (ours[field][0] != 0) != (theirs[field][0] != 0):
                    bad.append(field)
                continue
            if ours[field] != theirs.get(field):
                bad.append(field)
        compared += 1
        if bad:
            print('FIRST DIFFERENCE at update %d (kind %s), after %d records that matched' % (count, kind, compared - 1))
            for field in bad[:12]:
                print('    %-12s original %s, port %s' % (field, describe(field, ours[field]), describe(field, theirs.get(field, (0,)))))
            if len(bad) > 12:
                print('    ... and %d more fields' % (len(bad) - 12))
            earlier = [k for k in sorted(original) if k < key][-2:]
            for k in earlier:
                print('    before %s: original mti=%d port mti=%d' % (k, original[k]['mti'][0], port.get(k, {}).get('mti', (0,))[0]))
            return 1
    total = len([k for k in original if k[0] <= end and k[1] == 'T'])
    print('IDENTICAL: %d T records of the original up to update %d matched the port\'s' % (compared, end))
    if missing:
        print('    %d of the original\'s %d records have no counterpart in the port, for example %s' % (len(missing), total, missing[:5]))
    return 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('command', choices=['run', 'compare', 'original', 'port', 'generate'])
    parser.add_argument('scenario')
    parser.add_argument('--end', type=int)
    args = parser.parse_args()
    scenario = load_scenario(args.scenario)
    name = scenario_name(args.scenario)
    end = args.end or scenario.END
    os.makedirs(os.path.join(RUN_DIR, name), exist_ok=True)
    if args.command == 'generate':
        generate(name, scenario, end)
        return
    if args.command in ('run', 'original'):
        run_original(name, scenario, end)
    if args.command in ('run', 'port'):
        run_port(name, scenario, end)
    if args.command in ('run', 'compare'):
        sys.exit(compare(name, scenario, end))


if __name__ == '__main__':
    main()
