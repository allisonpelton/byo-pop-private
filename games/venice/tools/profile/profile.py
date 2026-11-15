#!/usr/bin/env python3
"""Venice Deluxe's processor time like for like, the port against the original (docs/COORDINATION.md, "A performance pass";
games/venice/docs/GAME_NOTES.md, "Performance").

    python -I games/venice/tools/profile/profile.py prep <out dir>              the player's profile and the input of the stretch
    python -I games/venice/tools/profile/profile.py port <out dir> <run dir>    one measured run of the port (real time)
    python -I games/venice/tools/profile/profile.py orig <out dir> <run dir>    one measured run of the original (origrun, real clock)
    python -I games/venice/tools/profile/profile.py sum <out dir>               the medians of pairs.sh's runs

The stretch is `scenario.py`: Journey from a mid-game building, the menus clicked and then the port's bot's shots, given to both
games as input at the same frame of the game's own count (the frames the game's widget has updated since the loading ended: the
port's `after <frame>`, the original's update counter from the update its tick counter first leaves 0). The bot ran once, in `prep`,
to make the input; the measured runs carry only the script's lines (a `cyclesplit` or `count` line at the two ends of the stretch).
Counting is the shared one (`-cyclesplit=<file>` in the port, `--count` in the original; docs/TEST_COMMANDS.md, "The cycle split"),
read with tools/cyclesplit.py. A run is started under pincore by pairs.sh, the port under rundesk, the original by origrun.
"""
import json
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..', '..'))
sys.path.insert(0, os.path.join(ROOT, 'games', 'venice', 'tools', 'idplay'))
import idplay  # noqa: E402

SCENARIO = os.path.join(HERE, 'scenario.py')
NAME = 'perf'
FIRST = 2500    # the scenario frame the stretch starts at (label a): the level is in play
LAST = 6500     # and ends at (label b)
QUIT = LAST + 300
RENDER_COUNTER = '[0xa2a178]+0x484'   # the original's SexyAppBase update count (UpdateFrames adds one a frame)
FIRST_RUNNING = '0xa2a1ac'            # the game's tick count: 0 until the first update after the loading


def scenario(out):
    idplay.RUN_DIR = os.path.join(out, 'idplay')
    sc = idplay.load_scenario(SCENARIO)
    return sc


def prep(out):
    sc = scenario(out)
    os.makedirs(os.path.join(idplay.RUN_DIR, NAME), exist_ok=True)
    idplay.make_profile(NAME, sc)
    inputs = [tuple(entry) for entry in sc.INPUTS] + [tuple(entry) for entry in idplay.bot_inputs(NAME, sc)]
    with open(os.path.join(out, 'inputs.json'), 'w') as handle:
        json.dump(inputs, handle)
    shutil.copyfile(idplay.start_profile(NAME, sc), os.path.join(out, 'profiles.dat'))
    print('inputs', len(inputs), 'last frame', max(frame for frame, _, _ in inputs))


def load_inputs(out):
    return [tuple(entry) for entry in json.load(open(os.path.join(out, 'inputs.json')))]


def port(out, run):
    os.makedirs(run, exist_ok=True)
    sc = scenario(out)
    user = os.path.join(run, 'user')
    shutil.rmtree(user, ignore_errors=True)
    os.makedirs(user)
    idplay.set_port_mode(user, sc, windowed=True)
    if os.environ.get('CYC_3D') == '1':   # the port's 3D display (the registry's Is3D left at its default), for the record
        with open(os.path.join(user, 'registry.ini'), 'w', newline='') as registry:
            registry.write('[SOFTWARE\\PopCap\\Venice]\nScreenMode=dword:0\n')
    shutil.copyfile(os.path.join(out, 'profiles.dat'), os.path.join(user, 'profiles.dat'))
    lines = []
    for frame, _, command in load_inputs(out):
        if frame <= QUIT:
            lines.append('1 after %d %s' % (idplay.BOT_START + frame, command))
    lines.append('1 after %d cyclesplit a' % (idplay.BOT_START + FIRST))
    lines.append('1 after %d cyclesplit b' % (idplay.BOT_START + LAST))
    lines.append('1 after %d quit' % (idplay.BOT_START + QUIT))
    script = os.path.join(run, 'port.txt')
    open(script, 'w').write('\n'.join(lines) + '\n')
    scale = os.environ.get('CYC_WINDOWSCALE', '1')
    command = [idplay.PORT_EXE, '-gamedir=' + idplay.GAMEDIR, '-userdir=' + user, '-seed=' + idplay.SEED, '-script=' + script,
               '-cyclesplit=' + os.path.join(run, 'port.split.jsonl')]
    if scale != '0':
        command.append('-windowscale=' + scale)
    with open(os.path.join(run, 'port.out'), 'w') as handle:
        subprocess.call(command, stdout=handle, stderr=subprocess.STDOUT, cwd=ROOT)


def orig(out, run):
    os.makedirs(run, exist_ok=True)
    sc = scenario(out)
    if os.path.exists(idplay.ORIGINAL_PROFILES):
        os.remove(idplay.ORIGINAL_PROFILES)
    shutil.copyfile(os.path.join(out, 'profiles.dat'), idplay.ORIGINAL_PROFILES)
    timed = [(frame, 'at +%d %s' % (idplay.BOT_START + frame, original)) for frame, original, _ in load_inputs(out) if frame <= QUIT]
    timed += [(FIRST, 'at +%d count a' % (idplay.BOT_START + FIRST)), (LAST, 'at +%d count b' % (idplay.BOT_START + LAST))]
    timed.sort(key=lambda entry: entry[0])   # stable: the inputs of one frame keep their order
    lines = ['waitfor %s != 0 180000' % FIRST_RUNNING, 'mark'] + [line for _, line in timed]
    lines += ['at +%d count end' % (idplay.BOT_START + QUIT), 'quit']
    script = os.path.join(run, 'orig.txt')
    open(script, 'w').write('\n'.join(line for line in lines if line) + '\n')
    counts = os.path.join(ROOT, 'build', 'origrun', 'run', 'counts.jsonl')   # origrun's default run folder
    if os.path.exists(counts):
        os.remove(counts)
    command = [idplay.ORIGRUN, '--exe', idplay.ORIGINAL_EXE, '--script', script, '--clock', 'real', '--cursor', 'scripted',
               '--keys', 'scripted', '--counter', RENDER_COUNTER, '--patches', idplay.PATCHES, '--seed', idplay.SEED,
               '--count', os.path.join(HERE, 'count.txt'), '--timeout', '600']
    with open(os.path.join(run, 'orig.out'), 'w') as handle:
        subprocess.call(command, stdout=handle, stderr=subprocess.STDOUT, cwd=ROOT)
    shutil.copyfile(counts, os.path.join(run, 'orig.split.jsonl'))


def cycle_rate(path):
    """the cycle counter's rate, as tools/cyclesplit.py measures it"""
    import re
    text = subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'tools', 'cyclesplit.py'), path, '--from', 'a', '--to', 'b'],
                          capture_output=True, text=True).stdout
    return float(re.search(r'taken as (\d+) MHz', text).group(1)) * 1e6


def summary(out):
    """the medians of the runs pairs.sh made: for each side the process's cycles an update over the stretch, its parts and its share of
    one core"""
    import glob
    import statistics
    sides = {'original': glob.glob(os.path.join(out, 'o[0-9]*', 'orig.split.jsonl')),
             'port, window 1x': glob.glob(os.path.join(out, 'p[0-9]*1', 'port.split.jsonl')),
             'port, default window (2x)': glob.glob(os.path.join(out, 'p[0-9]*0', 'port.split.jsonl')),
             'port, 3D display, window 1x': glob.glob(os.path.join(out, 'q[0-9]*', 'port.split.jsonl'))}
    first = next(path for files in sides.values() for path in files)
    hz = cycle_rate(first)
    print('cycle counter %.0f MHz; the stretch is labels a to b; M cycles an update, the share of one core' % (hz / 1e6))
    print('%-28s %4s %8s %8s %8s %8s %8s %8s' % ('', 'runs', 'update', 'draw', 'present', 'driver', 'total', 'share'))
    for name, files in sides.items():
        rows = []
        for path in sorted(files):
            text = subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'tools', 'cyclesplit.py'), path, '--from', 'a', '--to', 'b',
                                   '--json', '--hz', str(hz)], capture_output=True, text=True).stdout
            d = json.loads(text)[0]
            rows.append([d['update'] / d['updates'] / 1e6, d['draw'] / d['updates'] / 1e6, d['present'] / d['updates'] / 1e6,
                         d['driver'] / d['updates'] / 1e6, d['process'] / d['updates'] / 1e6, d['process'] / (d['wall_s'] * hz) * 100,
                         d['updates'], d['draw_calls']])
        if not rows:
            continue
        med = [statistics.median(column) for column in zip(*rows)]
        print('%-28s %4d %8.3f %8.3f %8.3f %8.3f %8.3f %7.1f%%   (%d updates, %d draws)' % (name, len(rows), *med[:6], med[6], med[7]))
        print('%-28s runs: %s' % ('', ' '.join('%.3f' % row[4] for row in rows)))


if __name__ == '__main__':
    verb = sys.argv[1]
    if verb == 'prep':
        prep(sys.argv[2])
    elif verb == 'port':
        port(sys.argv[2], sys.argv[3])
    elif verb == 'orig':
        orig(sys.argv[2], sys.argv[3])
    elif verb == 'sum':
        summary(sys.argv[2])
    else:
        sys.exit(__doc__)
