#!/usr/bin/env python3
"""Phase one of the identical-play pass: the original and the port play the same inputs, and the game's state is compared
frame by frame (docs/COORDINATION.md; games/venice/docs/IDENTICAL_PLAY.md).

One run of the original under origrun (tools/origrun, docs/ORIGRUN.md) logs, at the end of every update of its window
(the SexyAppBase update counter is origrun's --counter), the values below with a `peek` line. The port then replays the same
run: the ticks each frame ran (the original's 4 ms ticks per 10 ms frame follow its clock, so they are read from its log
and given to the port with the `ticks` test command), and the inputs at the same frames (`after <frame> <command>`). The
port's `trace` command writes a FRAME line with the same values at the end of each frame. The first value that differs is
the first difference of the logic.

    idplay.py run <scenario> [--end N]      the original's run, the port's, and their comparison
    idplay.py compare <scenario>            the comparison of runs that were made

A scenario is a python file under scenarios/ that defines `INPUTS`: a list of (frame, original command, port command), the
frame counted from the first update after the loading (frame 0 is where the original's update counter first moved off 0).
The original's own commands are origrun's script commands (`click`, `key`, `text`...), the port's the framework's `-script`
commands and the standard test commands. A scenario also gives `END`, the last frame compared.
"""

import argparse
import importlib.util
import json
import os
import re
import shutil
import subprocess
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..'))
RUN_DIR = os.path.join(ROOT, 'build', 'logs', 'idplay')

# what is read from the original's memory at the end of every frame, and what the port's trace calls it (TestCommands.cpp,
# VeniceApp::TraceFrame). A null pointer's read shows as '-', which is 0.
EXPRESSIONS = [
    ('counter', '0xa2a1ac'),            # the tick count the game has run up to (0 until the first update after a reset)
    ('batch', '0x5b51c8'),              # how many ticks the last batch ran
    ('loadstate', '0x5b51b8'),          # 101 once the game runs
    ('seed', '0x5b5174'),               # the random generator
    ('tail', '0x5ba2c0'),               # the last scene (a pointer: only the equality with the active level is used)
    ('active', '0x5ba2a4'),             # the level that was made last
    ('scene', '[0x5ba2c0]+0x100'),      # the last scene's tick count, as float bits
    ('shooter', '[0x5ba2a8]+0xa8'),     # the shooter's x, as float bits
    ('level', '[0x5ba2a4]+0x100'),      # the active level's tick count, as float bits
    ('hits', '[0x5ba2a4]+0x3b0'),
    ('shots', '[0x5ba2a4]+0x3ac'),
    ('score', '0xa1c99c'),              # the first profile's score
    ('middle', '[0x5ba2b0]+0xac'),      # the y of the level's middle layer, as float bits
    ('plates', '[0x5ba2b4]+0xac'),      # the y of the layer that holds the plates
] + [('k%d' % kind, '0x%x' % (0x5ba3f4 + 4 * kind)) for kind in range(12)]  # the unfilled slots of each colour, and the colours
COMPARED = ['seed', 'scene', 'shooter', 'level', 'hits', 'shots', 'score', 'middle', 'plates'] + ['k%d' % kind for kind in range(12)]
MAX_CATCH_UP = 20                       # ticks the game runs in one frame at most (VeniceApp.cpp)

PORT_EXE = os.path.join(ROOT, 'build', 'native', 'venice.exe')
RUNDESK = os.path.join(ROOT, 'build', 'native', 'rundesk.exe')
DGVOODOO = os.environ.get('ORIGRUN_DGVOODOO', 'E:/retools/dgVoodoo2_87_5')  # the folder of dgVoodoo2 (origrun's --dgvoodoo)
ORIGRUN = os.path.join(ROOT, 'build', 'origrun-bin', 'origrun.exe')
ORIGINAL_EXE = os.path.join(ROOT, 'build', 'origrun', 'venice', 'game', 'VeniceGame.exe')
PATCHES = os.path.join(ROOT, 'tools', 'origrun', 'patches', 'venice.txt')
GAMEDIR = os.path.join(ROOT, 'assets', 'venice')
# where the original keeps its profiles (a copy of the game puts them under ProgramData, whatever folder it runs from)
ORIGINAL_PROFILES = os.path.join(os.environ.get('ProgramData', r'C:\ProgramData'), 'PopCap Games', 'Venice', 'userdata', 'profiles.dat')


def load_scenario(name):
    path = name if name.endswith('.py') else os.path.join(os.path.dirname(os.path.abspath(__file__)), 'scenarios', name + '.py')
    spec = importlib.util.spec_from_file_location('scenario', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def scenario_name(name):
    return os.path.splitext(os.path.basename(name))[0]


def original_script(scenario, start):
    """The original's script: a peek at every frame from the first on, and the inputs `start` frames after the first (a
    frame by which the loading is done in every run; the first update after it is found in the log)."""
    peeks = ' '.join(expression for _, expression in EXPRESSIONS)
    inputs = {}
    for frame, original_command, _ in scenario.INPUTS:
        if original_command:
            inputs.setdefault(start + frame, []).append(original_command)
    lines = []
    for frame in range(1, start + scenario.END + 1):
        for command in inputs.get(frame, []):
            lines.append('at %d %s' % (frame, command))
        lines.append('at %d peek %s' % (frame, peeks))
    lines.append('quit')
    return '\n'.join(lines) + '\n'


def parse_original(path):
    """frame -> {name: int} from the origrun log's peek lines. A hold that comes late in a frame that stalled (the original
    loads a level inside an update, which takes seconds) logs the frame counter one behind: the second line with a number is
    the next frame's. Also returns {(N, command): the frame the hold of `at N command` was at}."""
    records = {}
    holds = {}
    peek_line = re.compile(r'peek u=(\d+) (.*)$')
    hold_line = re.compile(r'at (\d+): counter reached (\d+) -> (?!peek)(.*)$')
    names = {expression: name for name, expression in EXPRESSIONS}
    for line in open(path, encoding='utf-8', errors='replace'):
        match = hold_line.search(line)
        if match:
            holds.setdefault((int(match.group(1)), match.group(3).strip()), int(match.group(2)))
            continue
        match = peek_line.search(line)
        if not match:
            continue
        record = {}
        for item in match.group(2).split(' '):
            if '=' not in item:
                continue
            expression, value = item.rsplit('=', 1)
            if expression in names:
                record[names[expression]] = 0 if value == '-' else int(value)
        frame = int(match.group(1))
        if frame in records and records[frame] != record and frame + 1 not in records:
            frame += 1
        records[frame] = record
    return records, holds


def masked(record):
    """the original's values as the port prints them: the active level's own values only while it is the last scene (a screen
    is made as a level too, and the pointer stays after the level is freed)"""
    masked_record = dict(record)
    if record.get('active', 0) != record.get('tail', 0) or record.get('tail', 0) == 0:
        for field in ('shooter', 'level', 'hits', 'shots', 'middle', 'plates'):
            masked_record[field] = 0
    return masked_record


def first_running_frame(records):
    """the frame of the original's log where its tick counter first moves off 0 with the game running"""
    for frame in sorted(records):
        record = records[frame]
        if record.get('loadstate', 0) >= 101 and record.get('counter', 0) != 0:
            return frame
    return None


def tick_schedule(records, first, last):
    """the ticks each frame from `first` ran: the tick counter's step (a late frame runs more than one tick), the batch the game
    logs when the counter was reset, none when it did not move; the steps over frames the log lacks are shared among them"""
    ticks = []
    previous = first
    for frame in range(first, last + 1):
        if frame not in records:
            ticks.append(None)
            continue
        if frame == first:
            ticks.append(0)
        else:
            before = records[previous]['counter']
            after = records[frame]['counter']
            if before != 0 and 0 <= after - before <= MAX_CATCH_UP:
                total = after - before
            else:
                total = records[frame]['batch'] if after != before else 0
            gap = frame - previous
            for share in range(gap - 1):
                ticks[len(ticks) - gap + 1 + share] = total // gap
            ticks.append(total - (total // gap) * (gap - 1))
        previous = frame
    return [0 if count is None else count for count in ticks]


def port_script(scenario, start, first, holds, ticks_file, trace_file):
    """the port's script: the inputs' frames are the original's, counted from its first running frame"""
    lines = ['1 ticks %s' % ticks_file, '1 trace %s' % trace_file]
    for frame, original_command, port_command in scenario.INPUTS:
        if port_command:
            # the frame the original's hold of the input was at, else the frame asked for
            hold = holds.get((start + frame, original_command), start + frame)
            lines.append('1 after %d %s' % (hold - first, port_command))
    lines.append('%d quit' % (scenario.END * 2 + 40000))
    return '\n'.join(lines) + '\n'


def parse_port(path):
    records = {}
    for line in open(path, encoding='utf-8', errors='replace'):
        if not line.startswith('FRAME '):
            continue
        record = dict(item.split('=') for item in line.split()[1:])
        numbers = {name: int(value) for name, value in record.items() if name != 'kinds'}
        for kind, count in enumerate(record['kinds'].split(',')):
            numbers['k%d' % kind] = int(count)
        records[int(record['f'])] = numbers
    return records


def compare(name, scenario):
    run = os.path.join(RUN_DIR, name)
    records, _ = parse_original(os.path.join(run, 'original.log'))
    first = first_running_frame(records)
    if first is None:
        print('the original never ran: no frame with the game running in its log')
        return 2
    port = parse_port(os.path.join(run, 'port.trace'))
    last = first + scenario.END
    ticks = tick_schedule(records, first, last)
    different = 0
    compared = 0
    unobserved = 0
    for offset in range(0, scenario.END + 1):
        frame = first + offset
        if offset not in port:
            print('frame %d (original %d): missing in the port' % (offset, frame))
            different += 1
            break
        if frame not in records:
            unobserved += 1  # the original's hold did not come at this frame
            continue
        compared += 1
        original, ours = masked(records[frame]), port[offset]
        bad = [field for field in COMPARED if original.get(field, 0) != ours[field]]
        if ours['ticks'] != ticks[offset]:
            bad.append('ticks')
        if bad:
            different += 1
            if different <= 3:
                print('frame %d (original %d): differs in %s' % (offset, frame, ', '.join(bad)))
                for field in bad:
                    if field == 'ticks':
                        print('    ticks: original %d, port %d' % (ticks[offset], ours['ticks']))
                    else:
                        print('    %s: original %d, port %d' % (field, original.get(field, 0), ours[field]))
                for earlier in range(max(0, offset - 2), offset):
                    if first + earlier in records:
                        print('    before (frame %d): original %s' % (earlier, {f: masked(records[first + earlier]).get(f, 0) for f in COMPARED}))
                        print('                       port     %s' % ({f: port[earlier][f] for f in COMPARED}))
    print('%s: the original ran from frame %d (its frame %d is frame 0); compared %d frames (%d not seen in its log): %d differ' %
          (name, first, first, compared, unobserved, different))
    return 1 if different else 0


def wait_for_slot():
    while subprocess.call([sys.executable, os.path.join(ROOT, 'tools', 'gamecount.py'), '--limit', '6'],
                          stdout=subprocess.DEVNULL) != 0:
        import time
        time.sleep(15)


def start_profile(name, scenario):
    """the profiles file a run starts with: the one made for the scenario's PROFILE_COMMANDS (make_profile), else its PROFILE (a
    file under scenarios/), else none (a new player)"""
    if hasattr(scenario, 'PROFILE_COMMANDS'):
        return os.path.join(RUN_DIR, name, 'start_profile.dat')
    profile = getattr(scenario, 'PROFILE', None)
    return os.path.join(os.path.dirname(os.path.abspath(__file__)), 'scenarios', profile) if profile else None


def make_profile(name, scenario):
    """the port makes the player a scenario starts with: the name Test typed, the scenario's commands (for example `unlock all`
    and `goto menu`) and a quit, which saves the profiles file; both games are then started with it"""
    run = os.path.join(RUN_DIR, name)
    user_dir = os.path.join(run, 'profile.user')
    shutil.rmtree(user_dir, ignore_errors=True)
    os.makedirs(user_dir, exist_ok=True)
    set_port_mode(user_dir, scenario)
    lines = ['3000 key T', '3010 key E', '3020 key S', '3030 key T', '3200 keydown RETURN', '3204 keyup RETURN']
    lines += ['%d %s' % (3900 + 10 * index, command) for index, command in enumerate(scenario.PROFILE_COMMANDS)]
    lines.append('%d save' % (3900 + 10 * len(scenario.PROFILE_COMMANDS)))
    lines.append('5000 quit')
    script = os.path.join(run, 'profile.txt')
    with open(script, 'w') as out:
        out.write('\n'.join(lines) + '\n')
    wait_for_slot()
    command = [RUNDESK, PORT_EXE, '-gamedir=' + GAMEDIR, '-userdir=' + user_dir, '-fast', '-seed=1', '-script=' + script]
    with open(os.path.join(run, 'profile.out'), 'w') as out:
        subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=ROOT)
    shutil.copyfile(os.path.join(user_dir, 'profiles.dat'), start_profile(name, scenario))


def run_original(name, scenario, start, log):
    if os.path.exists(ORIGINAL_PROFILES):
        os.remove(ORIGINAL_PROFILES)
    if start_profile(name, scenario):
        shutil.copyfile(start_profile(name, scenario), ORIGINAL_PROFILES)
    script = os.path.join(RUN_DIR, name, 'original.txt')
    with open(script, 'w') as out:
        out.write(original_script(scenario, start))
    wait_for_slot()
    accelerated = getattr(scenario, 'ACCELERATED', False)
    timeout = min(3600, (600 + scenario.END // 10) if accelerated else (240 + scenario.END // 40))
    command = [ORIGRUN, '--exe', ORIGINAL_EXE, '--script', script, '--clock', 'virtual', '--cursor', 'scripted', '--keys', 'scripted',
               '--counter', FRAME_COUNTER, '--update-ms', '4', '--patches', PATCHES, '--seed', '1', '--timeout', str(timeout)]
    if accelerated:
        # the original's 3D mode (Direct3D 7 over dgVoodoo2, docs/ORIGRUN.md, "Hardware acceleration"); it runs slower than software
        command += ['--3d', 'on', '--3d-key', r'Software\PopCap\Venice', '--3d-flag', '[[0xa2a178]+0x36c]+0x38',
                    '--dgvoodoo', DGVOODOO]
    with open(log, 'w') as out:
        subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=ROOT)


def set_port_mode(user_dir, scenario):
    """the port's 2D or 3D mode for the run: the original's run on rundesk's desktop is in software (docs/ORIGRUN.md, "Hardware
    acceleration"), so a scenario that is not marked ACCELERATED runs the port with the registry's Is3D at 0; the layer merge
    and the scroll follow the mode"""
    if getattr(scenario, 'ACCELERATED', False):
        return
    with open(os.path.join(user_dir, 'registry.ini'), 'w', newline='') as out:
        out.write('\n'.join([r'[SOFTWARE\PopCap\Venice]', 'Is3D=dword:0', '']))


def run_port(name, scenario, start, first, holds, ticks, log):
    run = os.path.join(RUN_DIR, name)
    ticks_file = os.path.join(run, 'ticks.txt')
    with open(ticks_file, 'w') as out:
        out.write(' '.join(str(count) for count in ticks) + '\n')
    script = os.path.join(run, 'port.txt')
    with open(script, 'w') as out:
        out.write(port_script(scenario, start, first, holds, ticks_file.replace('\\', '/'), os.path.join(run, 'port.trace').replace('\\', '/')))
    user_dir = os.path.join(run, 'port.user')
    shutil.rmtree(user_dir, ignore_errors=True)
    os.makedirs(user_dir, exist_ok=True)
    set_port_mode(user_dir, scenario)
    if start_profile(name, scenario):
        shutil.copyfile(start_profile(name, scenario), os.path.join(user_dir, 'profiles.dat'))
    wait_for_slot()
    command = [RUNDESK, PORT_EXE, '-gamedir=' + GAMEDIR, '-userdir=' + user_dir, '-fast', '-drawevery', '-seed=1', '-script=' + script]
    with open(log, 'w') as out:
        subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=ROOT)


BOT_START = 900  # the port's frame, from its first running frame, that a bot run's scenario frame 0 is at (the name screen is up)


def bot_inputs(name, scenario):
    """The input of the port's bot: the port runs the scenario's own inputs and then the bot (`bot <frames>`), which gives its
    moves, presses and releases a frame at a time and writes each as an INPUT line of the trace. They are the scenario's
    later inputs, so that the original is given them too."""
    run = os.path.join(RUN_DIR, name)
    trace = os.path.join(run, 'bot.trace')
    script = os.path.join(run, 'bot.txt')
    lines = ['1 trace %s' % trace.replace('\\', '/')]
    for frame, _, port_command in scenario.INPUTS:
        if port_command:
            lines.append('1 after %d %s' % (BOT_START + frame, port_command))
    lines.append('1 after %d bot %d %s' % (BOT_START + scenario.BOT_AT, scenario.BOT_FRAMES, getattr(scenario, 'BOT_MODE', '')))
    lines.append('%d quit' % (BOT_START + scenario.END + 30000))
    with open(script, 'w') as out:
        out.write('\n'.join(lines) + '\n')
    user_dir = os.path.join(run, 'bot.user')
    shutil.rmtree(user_dir, ignore_errors=True)
    os.makedirs(user_dir, exist_ok=True)
    set_port_mode(user_dir, scenario)
    if start_profile(name, scenario):
        shutil.copyfile(start_profile(name, scenario), os.path.join(user_dir, 'profiles.dat'))
    wait_for_slot()
    command = [RUNDESK, PORT_EXE, '-gamedir=' + GAMEDIR, '-userdir=' + user_dir, '-fast', '-drawevery', '-seed=1', '-script=' + script]
    with open(os.path.join(run, 'bot.out'), 'w') as out:
        subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=ROOT)
    inputs = []
    for line in open(trace, encoding='utf-8', errors='replace'):
        if line.startswith('INPUT '):
            _, frame, command, x, y = line.split()
            inputs.append((int(frame.split('=')[1]) - BOT_START, '%s %s %s' % (command, x, y), '%s %s %s' % (command, x, y)))
    with open(os.path.join(run, 'bot_inputs.json'), 'w') as out:
        json.dump(inputs, out)
    return inputs


def with_bot(name, scenario, regenerate):
    """the scenario with the bot's input after its own, made now or read from the last bot run"""
    if not hasattr(scenario, 'BOT_AT'):
        return
    path = os.path.join(RUN_DIR, name, 'bot_inputs.json')
    scenario.INPUTS = list(scenario.INPUTS) + [tuple(entry) for entry in (bot_inputs(name, scenario) if regenerate else json.load(open(path)))]


FRAME_COUNTER = '[0xa2a178]+0x47c'  # the original's SexyAppBase update counter, in the app that 0xa2a178 points to (one a 4 ms frame)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('action', choices=['run', 'compare', 'original', 'port'])
    parser.add_argument('scenario')
    parser.add_argument('--start', type=int, default=3000, help='the frame of the original after which the inputs begin')
    args = parser.parse_args()
    scenario = load_scenario(args.scenario)
    name = scenario_name(args.scenario)
    run = os.path.join(RUN_DIR, name)
    os.makedirs(run, exist_ok=True)
    if args.action in ('run', 'original') and hasattr(scenario, 'PROFILE_COMMANDS'):
        make_profile(name, scenario)
    with_bot(name, scenario, args.action in ('run', 'original'))
    if args.action in ('run', 'original'):
        run_original(name, scenario, args.start, os.path.join(run, 'original.log'))
    if args.action in ('run', 'port'):
        records, holds = parse_original(os.path.join(run, 'original.log'))
        first = first_running_frame(records)
        if first is None:
            print('the original never ran')
            return 2
        run_port(name, scenario, args.start, first, holds, tick_schedule(records, first, first + scenario.END), os.path.join(run, 'port.out'))
    return compare(name, scenario)


if __name__ == '__main__':
    sys.exit(main())
