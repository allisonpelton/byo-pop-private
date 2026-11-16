#!/usr/bin/env python3
"""One coverage run of the original per idplay scenario (docs/COVERAGE.md): the scenario's inputs for the original, played on the host
by origrun on the virtual clock with --coverage on the list `python -I tools/coverage.py list hammerheads` writes.

    python games/hammerheads/tools/coverage/run.py <scenario> [<end update>]...

The hits go to build/coverage/hammerheads/hits/<scenario>.txt, the run's log beside them. Then:
    python -I tools/coverage.py report hammerheads build/coverage/hammerheads/hits/*.txt -o build/coverage/hammerheads/report.md
"""
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(HERE, '..', 'idplay'))
import idplay  # noqa: E402

ROOT = idplay.ROOT
HITS = os.path.join(ROOT, 'build', 'coverage', 'hammerheads', 'hits')


def script_text(scenario, end):
    inputs = {}
    for count, original_command, _ in scenario.INPUTS:
        if original_command:
            inputs.setdefault(count, []).append(original_command)
    lines = ['waitfor %s > 0 120000' % idplay.COUNTER]
    for count in sorted(inputs):
        if count <= end:
            for command in inputs[count]:
                lines.append('at %d %s' % (count, command))
    lines.append('at %d quit' % end)
    lines.append('quit')
    return '\n'.join(lines) + '\n'


def main():
    name = sys.argv[1]
    scenario = idplay.load_scenario(name)
    end = int(sys.argv[2]) if len(sys.argv) > 2 else getattr(scenario, 'END', max(c for c, _, _ in scenario.INPUTS) + 600)
    os.makedirs(HITS, exist_ok=True)
    run = os.path.join(ROOT, 'build', 'logs', 'coverage', name)
    shutil.rmtree(run, ignore_errors=True)
    os.makedirs(run)
    script = os.path.join(run, 'original.txt')
    open(script, 'w').write(script_text(scenario, end))
    shutil.rmtree(os.path.join(os.path.dirname(idplay.ORIGINAL_EXE), 'users'), ignore_errors=True)
    hits = os.path.join(ROOT, 'build', 'origrun', idplay.ORIGINAL_RUN, 'coverage.txt')
    if os.path.exists(hits):
        os.remove(hits)
    while subprocess.call([sys.executable, os.path.join(ROOT, 'tools', 'gamecount.py'), '--limit', '6'], stdout=subprocess.DEVNULL):
        import time
        time.sleep(20)
    command = [idplay.ORIGRUN, '--exe', idplay.ORIGINAL_EXE, '--run', idplay.ORIGINAL_RUN, '--stage-root', idplay.STAGE_ROOT, '--script', script,
               '--clock', 'virtual', '--reg-map', 'popcap', '--cursor', 'scripted', '--counter', idplay.COUNTER,
               '--patches', idplay.patches_file(run, scenario), '--seed', str(getattr(scenario, 'SEED', 1)),
               '--coverage', os.path.join(ROOT, 'build', 'coverage', 'hammerheads', 'functions.txt'), '--timeout', str(600 + end // 20)]
    with open(os.path.join(HITS, name + '.log'), 'w') as out:
        status = subprocess.call(command, stdout=out, stderr=subprocess.STDOUT, cwd=ROOT)
    if not os.path.exists(hits):
        print('%s: origrun exited with %d and wrote no hits file' % (name, status))
        return 1
    shutil.copyfile(hits, os.path.join(HITS, name + '.txt'))
    lines = open(hits).read().splitlines()
    print('%s: exit %d, %d functions; %s' % (name, status, sum(1 for l in lines if l and not l.startswith('#')), [l for l in lines if l.startswith('#')][-1:]))
    return 0


if __name__ == '__main__':
    sys.exit(main())
