#!/usr/bin/env python3
"""Venice Deluxe's phase-two shots with the original in its 3D mode: the original runs on the visible desktop under tools/visrun.py
(docs/VISRUN.md; neither Windows Sandbox nor rundesk gives it Direct3D, docs/ORIGRUN.md "Hardware acceleration"), the port runs in
its own 3D mode at the updates the original's steps landed on, and tools/shotdiff compares the two folders.

    python -I games/venice/tools/idplay/visshots.py prep <scenario>                     the player's profile, the step file and
                                                                                         the run's config, then visrun's --dry-run
    python -I games/venice/tools/idplay/visshots.py original <scenario> [visrun options]  the original's run (--user-window-until HH:MM
                                                                                         outside 22:00 to 08:00; the options are visrun's)
    python -I games/venice/tools/idplay/visshots.py port <scenario>                     the port's run and its shots
    python -I games/venice/tools/idplay/visshots.py diff <scenario>                     shotdiff on the two folders

The scenario is an idplay scenario of `scenarios/` (shots_menus3d, ...): its INPUTS and SHOTS become the step file, with the frames
counted from the first update after the loading (`waitfor` the game's tick count off 0, then `mark`, then `at +frame`), which is
frame 0 of the port's `1 after <frame>`. The port is given the updates the steps *landed* on (visrun's steps.tsv), whatever the
clock, so a step that came a update late on the real clock is replayed at that update. The port runs one tick a frame (no `ticks`
file): the original's virtual clock gives one update a frame. The original's shots are `--shape sandbox`, as shotdiff's Venice
settings expect them.
"""
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..', '..', '..', '..'))
sys.path.insert(0, HERE)
import idplay  # noqa: E402

BASE_CONFIG = os.path.join(ROOT, 'games', 'venice', 'tools', 'visrun', 'venice.toml')
VISRUN = os.path.join(ROOT, 'tools', 'visrun.py')
TICK_COUNT = '0xa2a1ac'    # the game's tick count: 0 until the first update after the loading (the port's frame 0)
LOADING_WAIT_MS = 180000
PORT_END_MARGIN = 300      # frames the port's run goes on after its last step


def names(scenario_argument):
    scenario = idplay.load_scenario(scenario_argument)
    name = idplay.scenario_name(scenario_argument)
    prep_dir = os.path.join(idplay.RUN_DIR, name)
    os.makedirs(prep_dir, exist_ok=True)
    return scenario, name, prep_dir


def run_dir(name):
    return os.path.join(ROOT, 'build', 'visrun', 'venice', name)


def plan_of(scenario):
    """the steps of the scenario in the order of the step file: (frame, kind, original command or shot name, port command)"""
    inputs = {}
    for frame, original_command, port_command in scenario.INPUTS:
        if original_command:
            inputs.setdefault(frame, []).append((original_command, port_command))
    shots = {}
    for frame, shot_name in scenario.SHOTS:
        shots.setdefault(frame, []).append(shot_name)
    plan = []
    for frame in sorted(set(inputs) | set(shots)):
        for original_command, port_command in inputs.get(frame, []):
            plan.append((frame, 'input', original_command, port_command))
        for shot_name in shots.get(frame, []):
            plan.append((frame, 'shot', shot_name, None))
    return plan


def write_steps(name, scenario, prep_dir):
    if getattr(scenario, 'FOCUS', None):
        sys.exit('a scenario with focus losses is not run on the desktop')
    lines = ['# made by visshots.py from scenarios/%s.py: frames count from the first update after the loading' % name]
    mouse = getattr(scenario, 'MOUSE_AT', None)
    if mouse:
        lines.append('move %d %d' % mouse)
    lines += ['waitfor %s != 0 %d' % (TICK_COUNT, LOADING_WAIT_MS), 'mark']
    for frame, kind, text, _ in plan_of(scenario):
        lines.append('at +%d %s%s' % (frame, 'shot ' if kind == 'shot' else '', text))
    lines.append('quit')
    path = os.path.join(prep_dir, 'visrun-steps.txt')
    with open(path, 'w') as out:
        out.write('\n'.join(lines) + '\n')
    return path


def write_config(name, scenario, prep_dir):
    """the run's copy of the base config: the profile the scenario starts with, seeded into the original's profile folder"""
    profile = idplay.start_profile(name, scenario)
    base = open(BASE_CONFIG, encoding='utf-8').read()
    if profile:
        seed = "save_seed = [['%s', '%s']]" % (profile, idplay.ORIGINAL_PROFILES)
    else:
        seed = '# no profile: a new player'
    path = os.path.join(prep_dir, 'venice-run.toml')
    with open(path, 'w', encoding='utf-8') as out:
        out.write(base.replace('@SAVE_SEED@', seed))
    return path


def visrun_command(name, config, steps, extra=()):
    return [sys.executable, '-I', VISRUN, config, steps, '--out', run_dir(name), '--seed', idplay.SEED, '--3d', 'on', '--shape', 'sandbox'] + list(extra)


def prep(scenario_argument):
    scenario, name, prep_dir = names(scenario_argument)
    if hasattr(scenario, 'PROFILE_COMMANDS'):
        idplay.make_profile(name, scenario)
    steps = write_steps(name, scenario, prep_dir)
    config = write_config(name, scenario, prep_dir)
    print('steps', steps, '\nconfig', config)
    return subprocess.call(visrun_command(name, config, steps, ['--dry-run']), cwd=ROOT)


def original(scenario_argument, extra):
    scenario, name, prep_dir = names(scenario_argument)
    steps = os.path.join(prep_dir, 'visrun-steps.txt')
    config = os.path.join(prep_dir, 'venice-run.toml')
    if not (os.path.exists(steps) and os.path.exists(config)):
        sys.exit('run prep first')
    os.makedirs(run_dir(name), exist_ok=True)
    return subprocess.call(visrun_command(name, config, steps, extra), cwd=ROOT)


def read_steps(path):
    """the rows of visrun's steps.tsv: (wanted update, landed update, command, arguments)"""
    rows = []
    for line in open(path, encoding='utf-8', errors='replace'):
        fields = line.rstrip('\n').split('\t')
        if len(fields) >= 3 and re.fullmatch(r'-?\d+', fields[0]) and re.fullmatch(r'-?\d+', fields[1]):
            rows.append((int(fields[0]), int(fields[1]), fields[2], fields[3] if len(fields) > 3 else ''))
    return rows


def landed_frames(scenario, name):
    """for each step of the plan, the port's frame: the update it landed on, less the update of the mark (the first update after the
    loading); None when visrun has no such row"""
    rows = read_steps(os.path.join(run_dir(name), 'steps.tsv'))
    mark = next((row for row in rows if row[2] == 'mark'), None)
    if mark is None:
        sys.exit('steps.tsv has no mark row: the run did not get past the loading')
    first = mark[1]
    after = [row for row in rows[rows.index(mark) + 1:] if row[2] not in ('quit', 'waitfor', 'mark')]
    plan = plan_of(scenario)
    if len(after) != len(plan):
        sys.exit('steps.tsv has %d rows after the mark, the plan %d steps' % (len(after), len(plan)))
    frames = []
    for step, row in zip(plan, after):
        if step[1] == 'shot' and not (row[2] == 'shot' and row[3].split()[0:1] == [step[2]]):
            sys.exit('steps.tsv row %r does not fit the plan step %r' % (row, step))
        frames.append(row[1] - first)
    return frames


def port(scenario_argument):
    scenario, name, prep_dir = names(scenario_argument)
    frames = landed_frames(scenario, name)
    shots_dir = os.path.join(idplay.ROOT, 'build', 'ph2', name, 'port')
    shutil.rmtree(shots_dir, ignore_errors=True)
    os.makedirs(shots_dir)
    lines = []
    mouse = getattr(scenario, 'MOUSE_AT', None)
    if mouse:
        lines.append('1 move %d %d' % mouse)
    last = 0
    for (frame, kind, text, port_command), landed in zip(plan_of(scenario), frames):
        at = max(landed, 1)   # "1 after 0" is not a frame count: the frame 0 steps are taken at the first
        last = max(last, at)
        if kind == 'shot':
            lines.append('1 after %d screenshot %s/%s.png' % (at, shots_dir.replace('\\', '/'), text))
        elif port_command:
            lines.append('1 after %d %s' % (at, port_command))
    lines.append('1 after %d quit' % (last + PORT_END_MARGIN))
    script = os.path.join(prep_dir, 'port-3d.txt')
    with open(script, 'w') as out:
        out.write('\n'.join(lines) + '\n')
    user_dir = os.path.join(prep_dir, 'port3d.user')
    shutil.rmtree(user_dir, ignore_errors=True)
    os.makedirs(user_dir)
    with open(os.path.join(user_dir, 'registry.ini'), 'w', newline='') as registry:
        registry.write('[SOFTWARE\\PopCap\\Venice]\nScreenMode=dword:0\n')   # the port's 3D display (Is3D left at its default), in a window
    if idplay.start_profile(name, scenario):
        shutil.copyfile(idplay.start_profile(name, scenario), os.path.join(user_dir, 'profiles.dat'))
    idplay.wait_for_slot()
    command = [idplay.RUNDESK, idplay.PORT_EXE, '-gamedir=' + idplay.GAMEDIR, '-userdir=' + user_dir, '-fast', '-drawevery',
               '-seed=' + idplay.SEED, '-script=' + script, '-renderscale=1']
    with open(os.path.join(prep_dir, 'port-3d.out'), 'w') as out:
        subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=ROOT)
    print('%d shots of the port' % len(os.listdir(shots_dir)))


def diff(scenario_argument):
    scenario, name, prep_dir = names(scenario_argument)
    original_shots = os.path.join(idplay.ROOT, 'build', 'ph2', name, 'orig')
    shutil.rmtree(original_shots, ignore_errors=True)
    os.makedirs(original_shots)
    for file_name in os.listdir(run_dir(name)):
        if file_name.endswith('.png'):
            shutil.copy(os.path.join(run_dir(name), file_name), original_shots)
    port_shots = os.path.join(idplay.ROOT, 'build', 'ph2', name, 'port')
    print('%d shots of the original, %d of the port' % (len(os.listdir(original_shots)), len(os.listdir(port_shots))))
    return subprocess.call([sys.executable, os.path.join(ROOT, 'tools', 'shotdiff', 'shotdiff.py'), '--game', 'venice', '--orig', original_shots,
                            '--port', port_shots, '--run', name], cwd=ROOT)


if __name__ == '__main__':
    if len(sys.argv) < 3 or sys.argv[1] not in ('prep', 'original', 'port', 'diff'):
        sys.exit(__doc__)
    verb, scenario_argument = sys.argv[1], sys.argv[2]
    if verb == 'original':
        sys.exit(original(scenario_argument, sys.argv[3:]))
    sys.exit({'prep': prep, 'port': port, 'diff': diff}[verb](scenario_argument))
