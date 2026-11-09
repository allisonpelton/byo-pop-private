"""Makes the player "Ali" with every secret mode unlocked, as a folder of the original's own save files (users/hiscores.dat,
users/Ali/profile.dat, ...), for the phase-one comparison of the secret modes (Twilight, Hyper, Cognito, Finity): the original has no
`unlock all`, so both sides start from this profile (record_port.sh copies it into the port's user folder with PROFILE=<folder>,
run_original.sh into the original's with PROFILE=<folder> too).

    python games/bejeweled2/tools/origcmp/make_profile.py --exe build/native/bejeweled2.exe --out build/logs/bej2/profile

Writes <out>/users/... . Runs under rundesk (the Driver does that), in 2D at 640x480 like the other recordings.
"""
import argparse
import os
import shutil
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), 'realplay'))
import full  # noqa: E402
from driver_base import Driver  # noqa: E402


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('--exe', required=True)
    ap.add_argument('--out', required=True)
    a = ap.parse_args()
    user = a.out + '.user'
    shutil.rmtree(user, ignore_errors=True)
    os.makedirs(a.out, exist_ok=True)
    d = Driver(a.exe, '', user, a.out, True, [], windowed=True, is3d=False)
    try:
        full.until(d, lambda s: s['screen'] in ('title', 'dialog', 'menu'), 'the title', 180)
        full.to_menu(d)
        d.ok('unlock all')
        d.ok('wait 5') if d.hold else d.idle(0.5)
        s = d.state()
        print('menu', s.get('secret'), s.get('widgets'))
    finally:
        d.close()
    src = os.path.join(user, 'users')
    dst = os.path.join(a.out, 'users')
    shutil.rmtree(dst, ignore_errors=True)
    shutil.copytree(src, dst)
    for root, _, files in os.walk(dst):
        for f in files:
            p = os.path.join(root, f)
            print(os.path.relpath(p, dst), os.path.getsize(p))


if __name__ == '__main__':
    main()
