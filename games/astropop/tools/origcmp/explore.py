"""Plays lines into the port with real input, a batch at a time, and prints the state after each (a way to find the clicks of a flow).

    python games/astropop/tools/origcmp/explore.py <dir> "<batch>" "<batch>" ...     a batch: lines separated by ;
"""
import argparse
import os
import subprocess
import sys
import time

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))))
sys.path.insert(0, os.path.join(ROOT, 'tools', 'playbymove'))
import playbymove  # noqa: E402


def wait_room(root):
    # at most 3 game processes on the machine at a time (CLAUDE.md)
    while subprocess.call([sys.executable, os.path.join(root, 'tools', 'gamecount.py'), '--limit', '6'], stdout=subprocess.DEVNULL) != 0:
        time.sleep(15)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('dir')
    ap.add_argument('batches', nargs='+')
    ap.add_argument('--exe', default=os.path.join(ROOT, 'build', 'native', 'astropop.exe'))
    ap.add_argument('--userdir')
    a = ap.parse_args()
    ns = argparse.Namespace(dir=a.dir, timeout=60, exe=a.exe, realtime=False,
                            game_args=['-seed=1', '-trace', '-userdir=' + (a.userdir or os.path.join(a.dir, 'user'))])
    wait_room(ROOT)
    g = playbymove.Game(ns)
    try:
        print(g.send('state')[0][:200])
        for b in a.batches:
            lines = [l.strip() for l in b.split(';') if l.strip()]
            lines = [('screenshot ' + os.path.abspath(l.split(None, 1)[1])) if l.startswith('screenshot ') else l for l in lines]
            ans = g.send(*(lines + ['state']))
            print(b, '->', ans[-1][:230])
    finally:
        g.finish(None)


if __name__ == '__main__':
    main()
