#!/usr/bin/env python3
"""What the port holds in memory in the scripted minute's game (README.md, "Performance"): tour 2's TSUNAMI game in real time
with -splashhold, as perf.py port-minute plays it, until its frame 2700, where the script gives the standard `memreport`
test command (the pictures, sounds and word lists the game keeps, the process's own figures; the detail goes to
<out dir>/memreport.txt); then tools/memmap.py reads the running process's address space (<out dir>/memmap.txt) before the
run is ended.

From the worktree root:

    build/native/rundesk.exe <python.exe> games/wordharmony/tools/profile/mem.py <out dir> [port.txt] [--series]

--series plays the whole tour at -fast and asks memreport at frames 600, 1500, 2000, 2700, 3800, 5000, 6900 and 7400 (the menu,
the start of the scripted minute, its middle and end, then the rest of the game and its end), so that how the memory grows
shows in <out dir>/series.txt; it does not run memmap.

port.txt is tour 2 made into the port's script (as cpu.py). WH_PERF_EXE=<name> measures a copy of the port in build/native.
Prints one line, the MEMREPORT line's figures and memmap's summary.
"""
import importlib.util
import os
import shutil
import subprocess
import sys
import time

ROOT = os.getcwd()
spec = importlib.util.spec_from_file_location('perf', os.path.join(ROOT, 'games', 'wordharmony', 'tools', 'profile', 'perf.py'))
perf = importlib.util.module_from_spec(spec)
spec.loader.exec_module(perf)
REPORT_FRAME = int(os.environ.get('WH_MEM_FRAME', '2700'))


def main():
    series = '--series' in sys.argv
    args = [a for a in sys.argv[1:] if a != '--series']
    out = os.path.abspath(args[0])
    port = os.path.abspath(args[1]) if len(args) > 1 else os.path.join(ROOT, 'build', 'ph2', 'tour2', 'port.txt')
    exe, gamedir = perf.port_exe()
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(os.path.join(out, 'user'))
    frames = perf.strip_script(port, os.path.join(out, 'frames.txt'), ['screenshot', 'quit'])
    script = os.path.join(out, 'script.txt')
    last = perf.to_update(REPORT_FRAME)
    with open(frames) as f, open(script, 'w', newline='\n') as g:
        for line in f:
            n, rest = line.split(' ', 1)
            if series:
                g.write('%d %s' % (int(n), rest))
            elif perf.to_update(int(n)) <= last:
                g.write('%d %s' % (perf.to_update(int(n)), rest))
        if series:
            for k in (600, 1500, 2000, 2700, 3800, 5000, 6900, 7450):
                g.write('%d memreport\n' % k)
            g.write('7460 quit\n')
        else:
            g.write('%d memreport %s\n' % (last + 1, os.path.join(out, 'memreport.txt').replace('\\', '/')))
            g.write('%d quit\n' % (last + 6000))   # a minute of waiting at most for memmap
    flags = ['-fast', '-splashhold', '-seed=1'] if series else ['-splashhold', '-seed=1']
    log = open(os.path.join(out, 'out.txt'), 'w')
    p = subprocess.Popen([exe, '-gamedir=' + gamedir, '-userdir=' + os.path.join(out, 'user'), '-nomods'] + flags +
                         ['-script=' + script], cwd=out, stdout=log, stderr=subprocess.STDOUT)
    line = ''
    if series:
        p.wait()
        log.close()
        with open(os.path.join(out, 'out.txt'), errors='replace') as f, open(os.path.join(out, 'series.txt'), 'w') as g:
            for l in f:
                if 'MEMREPORT' in l:
                    g.write(l[l.index('MEMREPORT'):])
        print(open(os.path.join(out, 'series.txt')).read())
        return
    for _ in range(1200):
        time.sleep(0.5)
        if p.poll() is not None:
            break
        # the exe's own stdout reaches the log only when it ends, so the report's file (it ends with the same line) says
        # when it has answered
        report = os.path.join(out, 'memreport.txt')
        if os.path.exists(report):
            with open(report, errors='replace') as f:
                summary = [l for l in f if l.startswith('summary:')]
            if not summary:
                continue
            line = summary[-1].strip()
            time.sleep(1.0)
            r = subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'tools', 'memmap.py'), str(p.pid), '--json',
                                os.path.join(out, 'memmap.json')], capture_output=True, text=True)
            with open(os.path.join(out, 'memmap.txt'), 'w') as g:
                g.write(r.stdout + r.stderr)
            break
    if p.poll() is None:
        p.terminate()
        try:
            p.wait(timeout=20)
        except subprocess.TimeoutExpired:
            p.kill()
    print(line)


if __name__ == '__main__':
    main()
