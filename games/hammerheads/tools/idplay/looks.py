"""Shots of a scenario for the looks comparison (phase two): the original in Windows Sandbox (the only place origrun takes shots)
and the port, at the same updates, then tools/shotdiff.

    python looks.py <scenario> <first> <last> <step> [--run <name>] [--only original|port|diff]

The scenario's inputs play as in idplay (so the screens are the ones its state comparison passed through), and a shot is taken
every <step> updates from <first> to <last>: `shot u<update>` in the original (an `at` line), `screenshot u<update>` in the port
(one update later, as its inputs are). Shots land in build/logs/looks/<run>/original and .../port; the diff run is
`python tools/shotdiff/shotdiff.py --game hammerheads --orig <original> --port <port> --run <run>`.
"""
import argparse
import os
import shutil
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import idplay                                   # noqa: E402

SHOT_DELAY = int(os.environ.get('LOOKS_SHOT_DELAY', '0'))       # the update after the original's at which the port's shot is taken
INPUT_DELAY = int(os.environ.get('LOOKS_INPUT_DELAY', '1'))     # the update after the original's at which the port's inputs go in
ACCEL = os.environ.get('LOOKS_ACCEL', 'off')       # Hardware Acceleration on both sides: off (the 2D mode, the default of a plain run) or on (3D, in the sandbox with the host's GPU)
PREFIX = ''                                     # put before the update in a shot's name (--prefix), to keep scenarios' shots apart in one run
RUN = 'hhlooks'                                 # origrun's run for the sandbox copy
LOOKS_DIR = os.path.join(idplay.ROOT, 'build', 'logs', 'looks')


def shot_name(scenario, count):
    """u<update>, or the scenario's own name for the shot (NAMES: {update: name})."""
    return getattr(scenario, 'NAMES', {}).get(count, PREFIX + 'u%d' % count)


def seed_profile(scenario, folder):
    """A scenario's PROFILE (a name in mkprofile.py) is the player's file both sides start from: <folder>/users."""
    name = getattr(scenario, 'PROFILE', None)
    if name:
        import mkprofile
        mkprofile.make(name, folder)


def original_shots(scenario, updates, run_dir):
    inputs = {}
    for count, original_command, _ in scenario.INPUTS:
        if original_command and count <= updates[-1]:
            inputs.setdefault(count, []).append(original_command)
    # the sandbox loads more slowly than the host (the loading thread runs on the wall clock, the updates on the virtual one), so the
    # loading screen's button may not work yet at update 1000: the inputs count from update 6000, and the port's, which has loaded at
    # once, count from its start; what the update counter itself shows is not on any screen
    lines = ['waitfor %s > 0 120000' % idplay.COUNTER, 'waitfor %s > 6000 240000' % idplay.COUNTER, 'mark']
    for count in sorted(set(inputs) | set(updates)):
        for command in inputs.get(count, []):
            lines.append('at +%d %s' % (count, command))
        if count in updates:
            lines.append('at +%d shot %s' % (count, shot_name(scenario, count)))
    lines.append('quit')
    script = os.path.join(run_dir, 'original.txt')
    open(script, 'w').write('\n'.join(lines) + '\n')
    return script


def run_original(name, scenario, updates, run_dir):
    script = original_shots(scenario, updates, run_dir)
    # the sandbox takes the game from inside the run's folder: a fresh copy of the staged one, with no players
    run_root = os.path.join(idplay.ROOT, 'build', 'origrun', RUN)
    shutil.rmtree(run_root, ignore_errors=True)
    shutil.copytree(os.path.dirname(idplay.ORIGINAL_EXE), os.path.join(run_root, 'game'))
    shutil.rmtree(os.path.join(run_root, 'game', 'users'), ignore_errors=True)
    seed_profile(scenario, os.path.join(run_root, 'game'))
    shots = os.path.join(run_root, 'shots')
    # the sandbox's registry starts empty, and the game's default is full screen, which it cannot make there: windowed, as the
    # host runs have it (WORKING_NOTES.md, "Phase one's runs")
    reg = os.path.join(run_root, 'settings.reg')
    open(reg, 'w', newline='').write('REGEDIT4\r\n\r\n[HKEY_CURRENT_USER\\SOFTWARE\\PopCap\\HammerHeads]\r\n"ScreenMode"=dword:00000000\r\n')
    exe = os.path.join(run_root, 'game', os.path.basename(idplay.ORIGINAL_EXE))
    command = [idplay.ORIGRUN, '--exe', exe, '--run', RUN, '--script', script, '--clock', 'virtual',
               '--sandbox', '--reg', reg, '--reg-map', 'popcap', '--cursor', 'scripted', '--counter', idplay.COUNTER,
               '--patches', idplay.patches_file(run_dir, scenario), '--seed', str(getattr(scenario, 'SEED', 1)),
               '--timeout', str(900 + updates[-1] // 20)]
    if ACCEL == 'on':           # the sandbox's own software display is the 2D mode, so `off` needs nothing
        command += ['--3d', 'on', '--3d-key', r'SOFTWARE\PopCap\HammerHeads', '--sandbox-vgpu']
    log = os.path.join(run_dir, 'original.log')
    with open(log, 'w') as out:
        status = subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=idplay.ROOT)
    print('original: origrun exited with', status, '->', log)
    target = os.path.join(run_dir, 'original')
    shutil.rmtree(target, ignore_errors=True)
    if os.path.isdir(shots):
        shutil.copytree(shots, target)
    for line in open(os.path.join(run_root, 'logs', 'origrun-sandbox.log'), errors='replace'):
        if 'mark: the counter reads' in line:
            open(os.path.join(run_dir, 'mark.txt'), 'w').write(line.rsplit('reads', 1)[1].strip().split()[0])
    print('original shots:', len(os.listdir(target)) if os.path.isdir(target) else 0)


def run_port(name, scenario, updates, run_dir):
    target = os.path.join(run_dir, 'port')
    shutil.rmtree(target, ignore_errors=True)
    os.makedirs(target)
    # the original waited at its loading screen until the counter was `mark` (the loading screen's own particles draw from the
    # generator the debris uses later, so the port waits as long and its draws come in the same order)
    mark = int(open(os.path.join(run_dir, 'mark.txt')).read()) if os.path.exists(os.path.join(run_dir, 'mark.txt')) else 0
    lines = [(mark + 1, '3d %s' % ACCEL)]
    for count, _, port_command in scenario.INPUTS:
        if port_command and count <= updates[-1]:
            lines.append((count + INPUT_DELAY + mark, port_command))
    for count in updates:
        lines.append((count + SHOT_DELAY + mark, 'screenshot %s' % os.path.join(target, shot_name(scenario, count) + '.png').replace('\\', '/')))
    lines.sort(key=lambda item: item[0])
    lines.append((updates[-1] + 20 + mark, 'quit'))
    script = os.path.join(run_dir, 'port.txt')
    open(script, 'w').write('\n'.join('%d %s' % item for item in lines) + '\n')
    user_dir = os.path.join(run_dir, 'portuser')
    shutil.rmtree(user_dir, ignore_errors=True)
    os.makedirs(user_dir)
    seed_profile(scenario, user_dir)
    command = [idplay.RUNDESK, idplay.PORT_EXE, '-fast', '-drawevery', '-renderscale=1', '-seed=%s' % getattr(scenario, 'SEED', 1),
               '-userdir=' + user_dir, '-script=' + script]
    with open(os.path.join(run_dir, 'port.log'), 'w') as out:
        status = subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=idplay.ROOT)
    print('port: exited with', status, '; shots:', len(os.listdir(target)))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('scenario')
    parser.add_argument('first', type=int)
    parser.add_argument('last', type=int)
    parser.add_argument('step', type=int)
    parser.add_argument('--run')
    parser.add_argument('--shots', action='store_true', help="take the scenario's own SHOTS list (first, last and step are then ignored)")
    parser.add_argument('--prefix', default='', help="a shot's name is this before u<update>")
    parser.add_argument('--only', choices=['original', 'port', 'diff'])
    arguments = parser.parse_args()
    global PREFIX
    PREFIX = arguments.prefix
    scenario = idplay.load_scenario(arguments.scenario)
    name = arguments.run or idplay.scenario_name(arguments.scenario)
    run_dir = os.path.join(LOOKS_DIR, name)
    os.makedirs(run_dir, exist_ok=True)
    updates = list(scenario.SHOTS) if arguments.shots else list(range(arguments.first, arguments.last + 1, arguments.step))
    if arguments.only in (None, 'original'):
        run_original(name, scenario, updates, run_dir)
    if arguments.only in (None, 'port'):
        run_port(name, scenario, updates, run_dir)
    if arguments.only in (None, 'diff'):
        command = [sys.executable, '-I', os.path.join(idplay.ROOT, 'tools', 'shotdiff', 'shotdiff.py'), '--game', 'hammerheads',
                   '--orig', os.path.join(run_dir, 'original'), '--port', os.path.join(run_dir, 'port'), '--run', name]
        subprocess.call(command, cwd=idplay.ROOT)


if __name__ == '__main__':
    main()
