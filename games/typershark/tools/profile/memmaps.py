"""Where the memory of Typer Shark Deluxe goes: the framework's memreport of the port and tools/memmap.py's map of the port and of the
original, taken in play on the scenario of perf.py (build/ts-p2/rec/rt2.in.txt, cut at PERF_END, 12000 unless set).

From the worktree root:

    python -I games/typershark/tools/profile/memmaps.py memreport <out dir>   the port with -fast under rundesk; a memreport (the detail of
                                                                               every picture and the heaps in <out dir>/mem<update>.txt, the
                                                                               JSON line in game.out) at updates 300 (the title), 900, 1250,
                                                                               1600, 6000 and 12000
    python -I games/typershark/tools/profile/memmaps.py port <out dir>        the port in real time under rundesk; memmap at 4 s, 40 s and
                                                                               100 s (the title, then in play)
    python -I games/typershark/tools/profile/memmaps.py orig <out dir>        the original on origrun's virtual clock with phase 2's loading
                                                                               patch, held in a waitfor once the script has played to its
                                                                               end, memmap twice 4 s apart

memmap's summary line of each map goes to <out dir>/<name>.txt and its JSON beside it. The runs need room for a game process
(tools/gamecount.py) and are not made under another session's heavy load; each stops its game before it ends.
"""
import os
import shutil
import subprocess
import sys
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import perf  # noqa: E402

root = os.getcwd()
mode, out = sys.argv[1], os.path.abspath(sys.argv[2])
shutil.rmtree(out, ignore_errors=True)
os.makedirs(out)
os.environ.setdefault('PERF_END', '12000')
SC = os.path.join('build', 'ts-p2', 'rec', 'rt2.in.txt')


def memmap(pid, name):
    r = subprocess.run([sys.executable, '-I', os.path.join('tools', 'memmap.py'), str(pid), '--json', os.path.join(out, name + '.json'),
                        '--top', '12'], capture_output=True, text=True)
    open(os.path.join(out, name + '.txt'), 'w').write(r.stdout + r.stderr)
    print(name, r.stdout.strip()[:1800], flush=True)


if mode == 'memreport':
    perf.fresh_users(os.path.join(out, 'ud'))
    lines, end = perf.scenario(SC)
    steps = [(n, c, a) for n, c, a in lines] + [(u, 'memreport', [os.path.join(out, 'mem%d.txt' % u).replace('\\', '/')])
                                                for u in (300, 900, 1250, 1600, 6000, end - 1)]
    with open(os.path.join(out, 'script.txt'), 'w', newline='\n') as f:
        for n, c, a in sorted(steps, key=lambda s: s[0]):
            f.write('%d %s %s\n' % (n, c, ' '.join(a)))
        f.write('%d quit\n' % end)
    exe, gamedir = perf.port_exe()
    with open(os.path.join(out, 'game.out'), 'wb') as log:
        subprocess.run([os.path.join(root, 'build', 'native', 'rundesk.exe'), exe, '-fast', '-seed=1', '-gamedir=' + gamedir,
                        '-userdir=' + os.path.join(out, 'ud'), '-script=' + os.path.join(out, 'script.txt')], stdout=log,
                       stderr=subprocess.STDOUT)
    print(subprocess.run(['grep', '-c', 'MEMREPORT', os.path.join(out, 'game.out')], capture_output=True, text=True).stdout.strip(),
          'memreports in', out)
elif mode == 'port':
    perf.fresh_users(os.path.join(out, 'ud'))
    sc, _ = perf.port_script(SC, os.path.join(out, 'script.txt'))
    exe, gamedir = perf.port_exe()
    p = subprocess.Popen([os.path.join(root, 'build', 'native', 'rundesk.exe'), exe, '-gamedir=' + gamedir,
                          '-userdir=' + os.path.join(out, 'ud'), '-seed=1', '-script=' + sc],
                         stdout=open(os.path.join(out, 'game.log'), 'wb'), stderr=subprocess.STDOUT)
    pid = None
    while not pid:
        pid = perf.find_process(os.path.basename(exe), root)
        time.sleep(0.1)
    t0 = time.time()
    for at, name in ((4, 'title'), (40, 'play1'), (100, 'play2')):
        time.sleep(max(0, at - (time.time() - t0)))
        memmap(pid, name)
    subprocess.run(['taskkill', '/F', '/PID', str(pid)], capture_output=True)
else:
    sc, end = perf.orig_script(SC, os.path.join(out, 'script.txt'))
    text = open(sc).read().replace('quit\n', 'waitfor [0x565380]+0x4b8 < 0 40000\nquit\n')
    open(sc, 'w', newline='\n').write(text)
    users = os.path.join(root, perf.GAME, 'users')
    shutil.rmtree(users, ignore_errors=True)
    shutil.copytree(os.path.join(root, 'build', 'ts-p1', 'fixtures', 'users-clean'), users)
    subprocess.run(['reg', 'delete', r'HKCU\Software\PopCap\TyperShark', '/f'], capture_output=True)
    log = os.path.join(out, 'origrun.log')
    p = subprocess.Popen([os.path.join(root, 'build', 'origrun-bin', 'origrun.exe'), '--exe', os.path.join(root, perf.GAME, 'TyperSharkGame.exe'),
                          '--run', 'typershark', '--script', sc, '--clock', 'virtual', '--cursor', 'scripted', '--reg-map', 'popcap',
                          '--seed', '1', '--counter', perf.COUNTER_EXPR, '--patches', os.path.join(root, 'build', 'ts-p2', 'patches.txt'),
                          '--timeout', '300'], stdout=open(log, 'wb'), stderr=subprocess.STDOUT)
    pid = None
    while not pid:
        pid = perf.find_process('TyperSharkGame.exe', '\\' + perf.GAME.replace('/', '\\') + '\\')
        time.sleep(0.1)
    while open(log, errors='replace').read().count('peek u=') < 2:
        time.sleep(0.2)
    for name in ('play1', 'play2'):
        memmap(pid, name)
        time.sleep(4)
    subprocess.run(['taskkill', '/F', '/PID', str(pid)], capture_output=True)
    p.wait(60)
