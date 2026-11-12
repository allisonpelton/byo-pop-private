#!/usr/bin/env python3
"""A sampling processor profile of the port on the performance pass's scripted minute (README.md, "Performance"): tour 2's
TSUNAMI game in real time with -splashhold, as perf.py port-minute plays it, under cpuprof_stacks.py (Bejeweled's cpuprof.py with a stack scan),
which suspends each busy thread about a thousand times a second and names the function its instruction pointer is in.

From the worktree root:

    build/native/rundesk.exe <python.exe> games/wordharmony/tools/profile/cpu.py <out dir> [port.txt] [seconds]

port.txt is tour 2 made into the port's script (python games/wordharmony/tools/shots/make.py
games/wordharmony/tools/shots/tour2.txt <dir> port > port.txt); without it, build/ph2/tour2/port.txt. The profile is
<out dir>/cpu.txt: the flat profile by module and function, weighted by the cycles each thread used. WH_PERF_EXE=<name>
profiles a copy of the port in build/native instead of wordharmony.exe, as perf.py does.
"""
import importlib.util
import os
import shutil
import subprocess
import sys

ROOT = os.getcwd()
spec = importlib.util.spec_from_file_location('perf', os.path.join(ROOT, 'games', 'wordharmony', 'tools', 'profile', 'perf.py'))
perf = importlib.util.module_from_spec(spec)
spec.loader.exec_module(perf)


def main():
    out = os.path.abspath(sys.argv[1])
    port = os.path.abspath(sys.argv[2]) if len(sys.argv) > 2 else os.path.join(ROOT, 'build', 'ph2', 'tour2', 'port.txt')
    seconds = sys.argv[3] if len(sys.argv) > 3 else '150'
    exe, gamedir = perf.port_exe()
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(os.path.join(out, 'user'))
    frames = perf.strip_script(port, os.path.join(out, 'frames.txt'), ['screenshot'])
    script = os.path.join(out, 'script.txt')
    with open(frames) as f, open(script, 'w', newline='\n') as g:
        for line in f:
            n, rest = line.split(' ', 1)
            g.write('%d %s' % (perf.to_update(int(n)), rest))
    cpuprof = os.path.join(ROOT, 'games', 'wordharmony', 'tools', 'profile', 'cpuprof_stacks.py')
    subprocess.run([sys.executable, cpuprof, os.path.join(out, 'cpu.txt'), seconds, '--', exe, '-gamedir=' + gamedir,
                    '-userdir=' + os.path.join(out, 'user'), '-nomods', '-splashhold', '-seed=1', '-script=' + script], check=False)


if __name__ == '__main__':
    main()
