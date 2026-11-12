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
    idplay.py shots <scenario>              phase two: the original (inside Windows Sandbox) and the port take the scenario's SHOTS
                                            at the same update, then tools/shotdiff compares them
    idplay.py sound <scenario>              phase two: the original on the host with the logging BASS (tools/bassproxy, built to
                                            build/bassproxy-venice/bass.dll) and the port with BYOPOP_SOUNDLOG play the same
                                            inputs, then soundcmp.py compares the sounds they play

A scenario is a python file under scenarios/ that defines `INPUTS`: a list of (frame, original command, port command), the
frame counted from the first update after the loading (frame 0 is where the original's update counter first moved off 0).
The original's own commands are origrun's script commands (`click`, `key`, `text`...), the port's the framework's `-script`
commands and the standard test commands. A scenario also gives `END`, the last frame compared, and may give `MOUSE_AT`, a pointer place both games are moved to
before the loading ends. Inside Windows Sandbox the original's pointer starts on a hot spot of the name screen (its
GetCursorPos reads 0,0 there), whose hot sound draws a random number at every flap, so a phase-two run puts it elsewhere.

Phase two (docs/SHOTDIFF.md) adds `SHOTS`: a list of (frame, name). The original takes `shot <name>` at that frame inside a
Windows Sandbox (origrun takes no shot on the host, docs/ORIGRUN.md), the port a `screenshot` at the frame the original's shot
landed on; both runs play the same ticks and inputs, so the two pictures show the same state.
"""

import argparse
import glob
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
MOUSE_FRAME = 1500                     # the original's frame at which a scenario's MOUSE_AT is moved to (while it is loading)
SHOT_MARGIN = 1000                      # frames after a scenario's last shot that its schedule goes on for (END is at least that)
MAX_CATCH_UP = 20                       # ticks the game runs in one frame at most (VeniceApp.cpp)

PORT_EXE = os.path.join(ROOT, 'build', 'native', 'venice.exe')
RUNDESK = os.path.join(ROOT, 'build', 'native', 'rundesk.exe')
DGVOODOO = os.environ.get('ORIGRUN_DGVOODOO', 'E:/retools/dgVoodoo2_87_5')  # the folder of dgVoodoo2 (origrun's --dgvoodoo)
ORIGRUN = os.path.join(ROOT, 'build', 'origrun-bin', 'origrun.exe')
ORIGINAL_RUN_DIR = os.path.join(ROOT, 'build', 'origrun', 'venice')
ORIGINAL_EXE = os.path.join(ORIGINAL_RUN_DIR, 'game', 'VeniceGame.exe')
PATCHES = os.path.join(ROOT, 'tools', 'origrun', 'patches', 'venice.txt')
GAMEDIR = os.path.join(ROOT, 'assets', 'venice')
# where the original keeps its profiles (a copy of the game puts them under ProgramData, whatever folder it runs from)
ORIGINAL_PROFILES = os.path.join(os.environ.get('ProgramData', r'C:\ProgramData'), 'PopCap Games', 'Venice', 'userdata', 'profiles.dat')


def load_scenario(name):
    path = name if name.endswith('.py') else os.path.join(os.path.dirname(os.path.abspath(__file__)), 'scenarios', name + '.py')
    spec = importlib.util.spec_from_file_location('scenario', path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    # the frames the port runs on the original's schedule (and the compare covers) are counted from the original's first running
    # frame, about 840 before `start`, so a shot must be that much inside END or the port runs the frames around it on its own clock
    last_shot = max([frame for frame, _ in getattr(module, 'SHOTS', [])] + [0])
    module.END = max(module.END, last_shot + SHOT_MARGIN)
    return module


def scenario_name(name):
    return os.path.splitext(os.path.basename(name))[0]


def original_script(scenario, start, shots=False):
    """The original's script: a peek at every frame from the first on, and the inputs `start` frames after the first (a
    frame by which the loading is done in every run; the first update after it is found in the log)."""
    peeks = ' '.join(expression for _, expression in EXPRESSIONS)
    inputs = {}
    for frame, original_command, _ in scenario.INPUTS:
        if original_command:
            inputs.setdefault(start + frame, []).append(original_command)
    shot_names = {}
    if shots:
        for frame, shot_name in scenario.SHOTS:
            shot_names.setdefault(start + frame, []).append(shot_name)
    lines = []
    blocks = focus_blocks(scenario, start)
    mouse = getattr(scenario, 'MOUSE_AT', None)
    for frame in range(1, start + scenario.END + 1):
        if mouse and frame == MOUSE_FRAME:
            lines.append('at %d move %d %d' % (frame, mouse[0], mouse[1]))
        for command in inputs.get(frame, []):
            lines.append('at %d %s' % (frame, command))
        lines.append('at %d peek %s' % (frame, peeks))
        for shot_name in shot_names.get(frame, []):
            lines.append('at %d shot %s' % (frame, shot_name))
        for anchor, steps, wait, _ in blocks:
            if anchor == frame:
                lines.append('at %d focus lose' % frame)
                for milliseconds, shot_name in steps:
                    lines.append('wait %d' % milliseconds)
                    lines.append('waitshot %s' % shot_name)
                lines.append('wait %d' % wait)
                lines.append('focus regain')
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


def focus_blocks(scenario, start):
    """The focus losses of a scenario (its FOCUS list): [(anchor, steps, wait, gap)]. The original does no update while it has no
    focus (its update counter stops), so a block of its script is a loss at counter `anchor`, a wait in game time and a shot for each
    step, a last wait and the regain; the port goes on counting frames meanwhile (its scenes do not update), so what follows
    the loss is `gap` frames later in the port's count: one frame for each 4 ms the original waited, and a few more."""
    blocks = []
    for entry in getattr(scenario, 'FOCUS', []):
        total = sum(milliseconds for milliseconds, _ in entry['steps']) + entry['wait']
        blocks.append((start + entry['frame'], entry['steps'], entry['wait'], total // 4 + 4))
    return blocks


def focus_shift(frame, blocks):
    """the frames the port has counted by the original's counter value `frame`, over the original's: the gaps of the losses before it"""
    return sum(gap for anchor, _, _, gap in blocks if frame > anchor)


def with_focus_gaps(ticks, blocks, first):
    """the tick schedule with the port's extra frames of each loss (no ticks run in them)"""
    result = list(ticks)
    for anchor, _, _, gap in sorted(blocks, reverse=True):
        at = anchor - first + 1
        result[at:at] = [0] * gap
    return result


def first_running_frame(records):
    """the frame of the original's log where its tick counter first moves off 0 with the game running"""
    for frame in sorted(records):
        record = records[frame]
        if record.get('loadstate', 0) >= 101 and record.get('counter', 0) != 0:
            return frame
    return None


def tick_schedule(records, first, last, blocks=()):
    """the ticks each frame from `first` ran: the tick counter's step (a late frame runs more than one tick), the batch the game
    logs when the counter was reset, none when it did not move; the steps over frames the log lacks are shared among them. After a
    loss of the focus the counter jumps by the ticks of the wait (which ran without updating the scenes) or is reset to 0 by the
    regain (0x4407c0), and the frames until it counts one by one again run no tick (whether one or two come before it is a race
    of the regain's message and the frame in the original)."""
    after_loss = {anchor + 1 for anchor, _, _, _ in blocks}
    settling = False
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
            counting = before != 0 and after != 0 and 0 <= after - before <= MAX_CATCH_UP
            if frame in after_loss:
                total = 0
                settling = True
            elif settling and not counting:
                total = 0
            elif counting:
                total = after - before
                settling = False
            else:
                total = records[frame]['batch'] if after != before else 0
            gap = frame - previous
            for share in range(gap - 1):
                ticks[len(ticks) - gap + 1 + share] = total // gap
            ticks.append(total - (total // gap) * (gap - 1))
        previous = frame
    return [0 if count is None else count for count in ticks]


def port_script(scenario, start, first, holds, ticks_file, trace_file, shots_dir=None, landed=None):
    """the port's script: the inputs' frames are the original's, counted from its first running frame"""
    lines = ['1 ticks %s' % ticks_file, '1 trace %s' % trace_file]
    blocks = focus_blocks(scenario, start)
    if getattr(scenario, 'MOUSE_AT', None):
        lines.append('1 move %d %d' % scenario.MOUSE_AT)
    for frame, original_command, port_command in scenario.INPUTS:
        if port_command:
            # the frame the original's hold of the input was at, else the frame asked for
            hold = holds.get((start + frame, original_command), start + frame)
            lines.append('1 after %d %s' % (hold - first + focus_shift(hold, blocks), port_command))
    for anchor, steps, wait, gap in blocks:
        # the port's frame of the loss: the original's counter value, over the gaps of the losses before it
        at = anchor - first + focus_shift(anchor, blocks)
        lines.append('1 after %d focus lose' % at)
        passed = 0
        for milliseconds, shot_name in steps:
            passed += milliseconds
            if shots_dir:
                lines.append('1 after %d screenshot %s/%s.png' % (at + passed // 4, shots_dir, shot_name))
        lines.append('1 after %d focus regain' % (at + gap))
    if shots_dir:
        for frame, shot_name in scenario.SHOTS:
            # the update the original's shot landed on (origrun's landed.tsv), else the frame asked for
            at = (landed or {}).get(shot_name, start + frame)
            lines.append('1 after %d screenshot %s/%s.png' % (at - first + focus_shift(at, blocks), shots_dir, shot_name))
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


def compare(name, scenario, start=3000):
    run = os.path.join(RUN_DIR, name)
    records, _ = parse_original(os.path.join(run, 'original.log'))
    first = first_running_frame(records)
    if first is None:
        print('the original never ran: no frame with the game running in its log')
        return 2
    port = parse_port(os.path.join(run, 'port.trace'))
    last = first + scenario.END
    ticks = tick_schedule(records, first, last, focus_blocks(scenario, start))
    blocks = focus_blocks(scenario, start)
    different = 0
    compared = 0
    unobserved = 0
    for offset in range(0, scenario.END + 1):
        frame = first + offset
        shift = focus_shift(frame, blocks)
        if offset + shift not in port:
            print('frame %d (original %d): missing in the port' % (offset, frame))
            different += 1
            break
        if frame not in records:
            unobserved += 1  # the original's hold did not come at this frame
            continue
        compared += 1
        original, ours = masked(records[frame]), port[offset + shift]
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
                        print('                       port     %s' % ({f: port[earlier + focus_shift(first + earlier, blocks)][f] for f in COMPARED}))
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


def run_original(name, scenario, start, log, sandbox=False, exe=None, run_dir=None):
    """The original's run. A phase-two run (`sandbox`) is inside Windows Sandbox, the only place origrun takes a shot
    (docs/ORIGRUN.md, "The host-capture ban"): the player's profiles file is staged under the run's sandbox-files, and nothing
    of the host's is touched. `run_dir` is the folder the run's copy of the game is in (the sound runs' own, by default the picture runs')."""
    run_dir = run_dir or ORIGINAL_RUN_DIR
    if sandbox:
        profiles = os.path.join(run_dir, 'sandbox-files', 'C', 'ProgramData', 'PopCap Games', 'Venice', 'userdata', 'profiles.dat')
        shutil.rmtree(os.path.join(run_dir, 'sandbox-files'), ignore_errors=True)
        if start_profile(name, scenario):
            os.makedirs(os.path.dirname(profiles), exist_ok=True)
            shutil.copyfile(start_profile(name, scenario), profiles)
    else:
        if os.path.exists(ORIGINAL_PROFILES):
            os.remove(ORIGINAL_PROFILES)
        if start_profile(name, scenario):
            shutil.copyfile(start_profile(name, scenario), ORIGINAL_PROFILES)
    script = os.path.join(RUN_DIR, name, 'original.txt')
    with open(script, 'w') as out:
        out.write(original_script(scenario, start, shots=sandbox))
    wait_for_slot()
    accelerated = getattr(scenario, 'ACCELERATED', False)
    timeout = min(3600, (600 + scenario.END // 10) if accelerated else (240 + scenario.END // 40))
    command = [ORIGRUN, '--exe', exe or ORIGINAL_EXE, '--script', script, '--clock', 'virtual', '--cursor', 'scripted', '--keys', 'scripted',
               '--counter', FRAME_COUNTER, '--update-ms', '4', '--patches', PATCHES, '--seed', '1', '--timeout', str(timeout)]
    if sandbox:
        command += ['--sandbox', '--run', os.path.basename(run_dir)]
        if accelerated:
            command.append('--sandbox-vgpu')  # the sandbox has no 3D without the host's GPU; only between 22:00 and 08:00 (CLAUDE.md)
    if accelerated:
        # the original's 3D mode (Direct3D 7 over dgVoodoo2, docs/ORIGRUN.md, "Hardware acceleration"); it runs slower than software
        command += ['--3d', 'on', '--3d-key', r'Software\PopCap\Venice', '--3d-flag', '[[0xa2a178]+0x36c]+0x38',
                    '--dgvoodoo', DGVOODOO]
    with open(log, 'w') as out:
        subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=ROOT)
    if sandbox:
        # the peeks and shots are logged inside the sandbox, in a file of the run's folder
        with open(log, 'a', encoding='utf-8') as out, open(os.path.join(run_dir, 'logs', 'origrun-sandbox.log'),
                                                         encoding='utf-8', errors='replace') as inside:
            shutil.copyfileobj(inside, out)


def set_port_mode(user_dir, scenario, windowed=False):
    """the port's 2D or 3D mode for the run: the original's run on rundesk's desktop is in software (docs/ORIGRUN.md, "Hardware
    acceleration"), so a scenario that is not marked ACCELERATED runs the port with the registry's Is3D at 0; the layer merge
    and the scroll follow the mode"""
    lines = [r'[SOFTWARE\PopCap\Venice]']
    if not getattr(scenario, 'ACCELERATED', False):
        lines.append('Is3D=dword:0')
    if windowed:
        lines.append('ScreenMode=dword:0')  # the original starts in a window (a shot shows its full-screen-only objects otherwise)
    if len(lines) > 1:
        with open(os.path.join(user_dir, 'registry.ini'), 'w', newline='') as out:
            out.write('\n'.join(lines + ['']))


def run_port(name, scenario, start, first, holds, ticks, log, shots_dir=None, landed=None, environment=None):
    run = os.path.join(RUN_DIR, name)
    ticks_file = os.path.join(run, 'ticks.txt')
    ticks = with_focus_gaps(ticks, focus_blocks(scenario, start), first)
    with open(ticks_file, 'w') as out:
        out.write(' '.join(str(count) for count in ticks) + '\n')
    script = os.path.join(run, 'port.txt')
    with open(script, 'w') as out:
        out.write(port_script(scenario, start, first, holds, ticks_file.replace('\\', '/'), os.path.join(run, 'port.trace').replace('\\', '/'),
                              shots_dir, landed))
    user_dir = os.path.join(run, 'port.user')
    shutil.rmtree(user_dir, ignore_errors=True)
    os.makedirs(user_dir, exist_ok=True)
    set_port_mode(user_dir, scenario, windowed=bool(shots_dir))
    if start_profile(name, scenario):
        shutil.copyfile(start_profile(name, scenario), os.path.join(user_dir, 'profiles.dat'))
    wait_for_slot()
    command = [RUNDESK, PORT_EXE, '-gamedir=' + GAMEDIR, '-userdir=' + user_dir, '-fast', '-drawevery', '-seed=1', '-script=' + script]
    if shots_dir:
        command.append('-renderscale=1')  # docs/SHOTDIFF.md: the shots are the game's own pixels
    with open(log, 'w') as out:
        subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=ROOT, env=dict(os.environ, **(environment or {})))


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


SHOT_DIR = os.path.join(ROOT, 'build', 'ph2')


def read_landed(path):
    """shot name -> the update origrun's shot landed on, from the run's shots/landed.tsv (the first column is the name, the
    second the update); a name that is not there is taken at the frame asked for"""
    landed = {}
    if os.path.exists(path):
        for line in open(path, encoding='utf-8', errors='replace'):
            fields = line.rstrip('\n').split('\t')
            if len(fields) >= 2 and fields[1].isdigit():
                landed[fields[0]] = int(fields[1])
    return landed


def take_shots(name, scenario, start, run, reuse=False, lag=0):
    """Phase two: the original's run inside Windows Sandbox with its shots, the port's run with its own at the updates the
    original's landed on, the comparison of their states (a shot draws the screen once more in both, so the seeds must still
    agree), and tools/shotdiff on the two folders"""
    out = os.path.join(SHOT_DIR, name)
    original_shots, port_shots = os.path.join(out, 'orig'), os.path.join(out, 'port')
    shutil.rmtree(port_shots, ignore_errors=True)
    os.makedirs(port_shots)
    if reuse:
        # the original's run of before: its log, its shots and the updates they landed on stay
        landed = read_landed(os.path.join(out, 'landed.tsv'))
    else:
        shutil.rmtree(original_shots, ignore_errors=True)
        os.makedirs(original_shots)
        shutil.rmtree(os.path.join(ORIGINAL_RUN_DIR, 'shots'), ignore_errors=True)
        run_original(name, scenario, start, os.path.join(run, 'original.log'), sandbox=True)
        for shot in glob.glob(os.path.join(ORIGINAL_RUN_DIR, 'shots', '*.png')):
            shutil.copy(shot, original_shots)
        shutil.copy(os.path.join(ORIGINAL_RUN_DIR, 'shots', 'landed.tsv'), out)
        landed = read_landed(os.path.join(out, 'landed.tsv'))
    landed = {shot_name: landed.get(shot_name, start + frame) - lag for frame, shot_name in scenario.SHOTS}
    records, holds = parse_original(os.path.join(run, 'original.log'))
    first = first_running_frame(records)
    if first is None:
        print('the original never ran')
        return 2
    run_port(name, scenario, start, first, holds, tick_schedule(records, first, first + scenario.END, focus_blocks(scenario, start)), os.path.join(run, 'port.out'),
             port_shots.replace('\\', '/'), landed)
    print('%s: %d shots of the original, %d of the port' % (name, len(os.listdir(original_shots)), len(os.listdir(port_shots))))
    state = compare(name, scenario, start)
    subprocess.call([sys.executable, os.path.join(ROOT, 'tools', 'shotdiff', 'shotdiff.py'), '--game', 'venice', '--orig', original_shots,
                     '--port', port_shots, '--run', name], cwd=ROOT)
    return state


SOUND_RUN_DIR = os.path.join(ROOT, 'build', 'origrun', 'venice-snd')
BASS_PROXY = os.path.join(ROOT, 'build', 'bassproxy-venice', 'bass.dll')  # games/venice/tools/bassproxy, built as its README says


def sound_copy():
    """the original's exe in a copy of its folder with the logging BASS beside it (the real one renamed bass_real.dll), so that
    the picture runs' copy stays as it is; the proxy writes every BASS call to basslog.txt in the folder"""
    game = os.path.join(SOUND_RUN_DIR, 'game')
    if not os.path.exists(os.path.join(game, 'bass_real.dll')):
        shutil.rmtree(SOUND_RUN_DIR, ignore_errors=True)
        shutil.copytree(os.path.join(ORIGINAL_RUN_DIR, 'game'), game)
        os.rename(os.path.join(game, 'bass.dll'), os.path.join(game, 'bass_real.dll'))
        shutil.copyfile(BASS_PROXY, os.path.join(game, 'bass.dll'))
    return os.path.join(game, 'VeniceGame.exe')


def compare_sound(name, scenario, start, run, reuse=False):
    """phase two, sound: the original runs on origrun's virtual clock with the BASS proxy (every call it makes to the sound
    library logged), the port runs the same inputs with BYOPOP_SOUNDLOG, and soundcmp.py compares the two lists of sounds"""
    original_log = os.path.join(run, 'original.log')
    bass_log = os.path.join(run, 'basslog.txt')
    if not reuse:
        exe = sound_copy()
        proxy_log = os.path.join(os.path.dirname(exe), 'basslog.txt')
        if os.path.exists(proxy_log):
            os.remove(proxy_log)
        # a scenario with focus changes takes the sandbox (origrun sends no window input to the host)
        in_sandbox = bool(getattr(scenario, 'FOCUS', None))
        run_original(name, scenario, start, original_log, sandbox=in_sandbox, exe=exe, run_dir=SOUND_RUN_DIR)
        shutil.copyfile(proxy_log, bass_log)
    records, holds = parse_original(original_log)
    first = first_running_frame(records)
    if first is None:
        print('the original never ran')
        return 2
    port_log = os.path.join(run, 'portsound.log')
    run_port(name, scenario, start, first, holds, tick_schedule(records, first, first + scenario.END, focus_blocks(scenario, start)), os.path.join(run, 'port.out'),
             environment={'BYOPOP_SOUNDLOG': port_log})
    return subprocess.call([sys.executable, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'soundcmp.py'), bass_log, port_log,
                            '--times'])


FRAME_COUNTER = '[0xa2a178]+0x47c'  # the original's SexyAppBase update counter, in the app that 0xa2a178 points to (one a 4 ms frame)


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('action', choices=['run', 'compare', 'original', 'port', 'shots', 'sound'])
    parser.add_argument('scenario')
    parser.add_argument('--start', type=int, default=3000, help='the frame of the original after which the inputs begin')
    parser.add_argument('--reuse', action='store_true', help="shots: keep the original's last run, take the port's shots again")
    parser.add_argument('--lag', type=int, default=0, help='shots: the port shoots this many updates before the update the original shot landed on')
    args = parser.parse_args()
    scenario = load_scenario(args.scenario)
    name = scenario_name(args.scenario)
    if args.action == 'sound':
        name += '_sound'  # its own run folder: a sound run's original.log and ticks are not those of the shots run's
    run = os.path.join(RUN_DIR, name)
    os.makedirs(run, exist_ok=True)
    if args.action in ('run', 'original', 'shots', 'sound') and hasattr(scenario, 'PROFILE_COMMANDS'):
        make_profile(name, scenario)
    with_bot(name, scenario, args.action in ('run', 'original', 'shots', 'sound'))
    if args.action == 'shots':
        return take_shots(name, scenario, args.start, run, args.reuse, args.lag)
    if args.action == 'sound':
        return compare_sound(name, scenario, args.start, run, args.reuse)
    if args.action in ('run', 'original'):
        run_original(name, scenario, args.start, os.path.join(run, 'original.log'))
    if args.action in ('run', 'port'):
        records, holds = parse_original(os.path.join(run, 'original.log'))
        first = first_running_frame(records)
        if first is None:
            print('the original never ran')
            return 2
        run_port(name, scenario, args.start, first, holds, tick_schedule(records, first, first + scenario.END, focus_blocks(scenario, args.start)), os.path.join(run, 'port.out'))
    return compare(name, scenario, args.start)


if __name__ == '__main__':
    sys.exit(main())
