"""Where a Mahjong Escape's memory is (docs/MEMORY.md, `memreport`): the game in its 800x600 window (surface mode), once at the main menu and
once on escape stage <level> after the bot has played <played> percent of its tiles, with the standard `memreport` command's line and the
process's private bytes at each place (the Mahjong engine's memreport counts no pictures or sounds of its own, so its line gives the process's numbers
only; --memmap adds tools/memmap.py's summary line of the same moment). Run it under rundesk:

    build/native/rundesk.exe python -I games/mahjongjapan/tools/realplay/memprobe.py <out dir> --game japan|china [--level N] [--played PERCENT] [--memmap]
"""
import json
import os
import shutil
import sys
import time

sys.path.insert(0, os.path.dirname(__file__))
from mjgame import Game  # noqa: E402

GAMES = {'japan': ('build/native/mahjongjapan.exe', 'assets/mahjongjapan', 'Mahjong Escape Ancient Japan'),
         'china': ('build/native/mahjongchina.exe', 'assets/mahjongchina', 'Mahjong Escape')}


def report(g, out, name):
    """Sends memreport and prints the MEMREPORT line the game logs for it (the command's own OK line is not waited for)"""
    log = os.path.join(out, 'game.log')

    def lines():
        return [x for x in open(log, errors='replace').read().splitlines() if 'MEMREPORT' in x]

    before = len(lines())
    with open(g.cf, 'a', newline='\n') as f:
        f.write('memreport ' + os.path.join(out, 'memreport-' + name + '.txt').replace(chr(92), '/') + '\n')
    end = time.time() + 30
    while time.time() < end and len(lines()) == before:
        time.sleep(0.2)
    found = lines()
    print(name, found[-1][found[-1].index('MEMREPORT'):] if len(found) > before else 'no MEMREPORT line')
    if '--memmap' in sys.argv:  # tools/memmap.py's reading of the process's address space (what the libraries, the heaps and the files hold)
        import subprocess
        root = os.path.join(os.path.dirname(__file__), '..', '..', '..', '..')
        done = subprocess.run([sys.executable, '-I', os.path.join(root, 'tools', 'memmap.py'), str(g.proc.pid)], capture_output=True, text=True)
        print(name, 'MEMMAP', (done.stdout + done.stderr).strip().splitlines()[0])


def main():
    out = os.path.abspath(sys.argv[1])
    game = sys.argv[sys.argv.index('--game') + 1] if '--game' in sys.argv else 'japan'
    level = int(sys.argv[sys.argv.index('--level') + 1]) if '--level' in sys.argv else 9
    played = int(sys.argv[sys.argv.index('--played') + 1]) if '--played' in sys.argv else 25
    exe, gamedir, section = GAMES[game]
    shutil.rmtree(os.path.join(out, 'user'), ignore_errors=True)
    os.makedirs(os.path.join(out, 'user'))
    open(os.path.join(out, 'user', 'registry.ini'), 'w').write(
        '; Settings (emulated Windows registry, HKEY_CURRENT_USER)\n\n[SOFTWARE' + chr(92) + 'JollyBear' + chr(92) + '%s]\nFullScreen=dword:0\n' % section)
    g = Game(out, os.path.abspath(exe), gamedir, fast=True, extra=['-windowscale=1'])
    try:
        g.until('the menu', lambda s: s['screen'] == 'menu' and not s['busy'])
        report(g, out, 'menu')
        g.ask('set player Probe')
        g.ask('level %d' % level)
        st = g.until('the board', lambda s: s['screen'] == 'escape' and not s['busy'] and s['board'] and s['board']['started'] and s['tiles'] > 0)
        goal = st['tiles'] * (100 - played) // 100
        for _ in range(4000):
            if st['tiles'] <= goal:
                break
            if st['busy']:
                g.wait(3)
            else:
                mv = g.ask('move')
                if mv is None:
                    g.wait(3)
                else:
                    g.send(mv)
                    g.wait(5)
            st = g.state()
        g.settle('the board at rest')
        report(g, out, 'board')
    finally:
        g.close()


if __name__ == '__main__':
    main()
