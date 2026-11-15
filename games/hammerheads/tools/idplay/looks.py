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

RUN = 'hhlooks'                                 # origrun's run for the sandbox copy
LOOKS_DIR = os.path.join(idplay.ROOT, 'build', 'logs', 'looks')


def original_shots(scenario, updates, run_dir):
    inputs = {}
    for count, original_command, _ in scenario.INPUTS:
        if original_command and count <= updates[-1]:
            inputs.setdefault(count, []).append(original_command)
    lines = ['waitfor %s > 0 120000' % idplay.COUNTER]
    for count in sorted(set(inputs) | set(updates)):
        for command in inputs.get(count, []):
            lines.append('at %d %s' % (count, command))
        if count in updates:
            lines.append('at %d shot u%d' % (count, count))
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
    shots = os.path.join(run_root, 'shots')
    exe = os.path.join(run_root, 'game', os.path.basename(idplay.ORIGINAL_EXE))
    command = [idplay.ORIGRUN, '--exe', exe, '--run', RUN, '--script', script, '--clock', 'virtual',
               '--sandbox', '--reg-map', 'popcap', '--cursor', 'scripted', '--counter', idplay.COUNTER,
               '--patches', idplay.patches_file(run_dir, scenario), '--seed', str(getattr(scenario, 'SEED', 1)),
               '--timeout', str(900 + updates[-1] // 20)]
    log = os.path.join(run_dir, 'original.log')
    with open(log, 'w') as out:
        status = subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=idplay.ROOT)
    print('original: origrun exited with', status, '->', log)
    target = os.path.join(run_dir, 'original')
    shutil.rmtree(target, ignore_errors=True)
    if os.path.isdir(shots):
        shutil.copytree(shots, target)
    print('original shots:', len(os.listdir(target)) if os.path.isdir(target) else 0)


def run_port(name, scenario, updates, run_dir):
    target = os.path.join(run_dir, 'port')
    shutil.rmtree(target, ignore_errors=True)
    os.makedirs(target)
    lines = []
    for count, _, port_command in scenario.INPUTS:
        if port_command and count <= updates[-1]:
            lines.append((count + 1, port_command))
    for count in updates:
        lines.append((count + 1, 'screenshot %s' % os.path.join(target, 'u%d.png' % count).replace('\\', '/')))
    lines.sort(key=lambda item: item[0])
    lines.append((updates[-1] + 20, 'quit'))
    script = os.path.join(run_dir, 'port.txt')
    open(script, 'w').write('\n'.join('%d %s' % item for item in lines) + '\n')
    user_dir = os.path.join(run_dir, 'portuser')
    shutil.rmtree(user_dir, ignore_errors=True)
    os.makedirs(user_dir)
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
    parser.add_argument('--only', choices=['original', 'port', 'diff'])
    arguments = parser.parse_args()
    scenario = idplay.load_scenario(arguments.scenario)
    name = arguments.run or idplay.scenario_name(arguments.scenario)
    run_dir = os.path.join(LOOKS_DIR, name)
    os.makedirs(run_dir, exist_ok=True)
    updates = list(range(arguments.first, arguments.last + 1, arguments.step))
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
