#!/usr/bin/env python3
"""Compile each scripts/common/*.lua that ships with a .luc of the same name using the port's compiler and compare the
chunk with the shipped one, instruction by instruction (cmpluc.py with line information and locals).

    python games/bookwormadv2/tests/poplua/pairs.py <bwa2_luarun.exe> [<game folder>]

Prints "pairs: N of M identical" and exits with the number of pairs that differ."""
import os
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))


def main():
    luarun = os.path.abspath(sys.argv[1])
    game = os.path.abspath(sys.argv[2] if len(sys.argv) > 2 else os.path.join(HERE, '..', '..', '..', '..', 'assets', 'bookwormadv2'))
    common = os.path.join(game, 'scripts', 'common')
    names = sorted(f[:-4] for f in os.listdir(common) if f.endswith('.lua') and os.path.isfile(os.path.join(common, f[:-4] + '.luc')))
    same = 0
    with tempfile.TemporaryDirectory() as tmp:
        for n in names:
            out = os.path.join(tmp, n + '.luc')
            c = subprocess.run([luarun, '-gamedir=' + game, '-compile', os.path.join(common, n + '.lua'), out],
                               capture_output=True, text=True)
            if c.returncode != 0:
                print('%s: compile failed: %s' % (n, c.stdout.strip()))
                continue
            d = subprocess.run([sys.executable, os.path.join(HERE, 'cmpluc.py'), os.path.join(common, n + '.luc'), out,
                                '--lines', '--locals', '--quiet'], capture_output=True, text=True)
            if d.returncode == 0:
                same += 1
            else:
                print('%s: %d functions differ' % (n, d.returncode))
    print('pairs: %d of %d identical' % (same, len(names)))
    return len(names) - same if names else 1


if __name__ == '__main__':
    sys.exit(main())
