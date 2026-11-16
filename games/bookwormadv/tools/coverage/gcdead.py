"""gcdead.py <build dir> <link log> <out.tsv> <object folder>...: the functions the linker discarded that one of the
game's own objects defines (README, "Unneeded code"; docs/COVERAGE.md). The build is an -O0 build of the game alone with
-ffunction-sections -fdata-sections, linked with -Wl,--gc-sections -Wl,--verbose, its output in <link log>; each
<object folder> is a CMake object folder under <build dir> (games/bookwormadv/CMakeFiles/bookwormadv.dir, ...).
Writes `object<TAB>function` lines, demangled, and prints how many. With TYPES=DdBbRr, the variables instead.

    python -I games/bookwormadv/tools/coverage/gcdead.py build/gc build/logs/gc.log build/logs/gcdead.tsv \\
        games/bookwormadv/CMakeFiles/{bookwormadv,bwa_jefflib,bwa_luaapp,bwa_lualib,bwa_popanim,poplua}.dir
"""
import glob
import os
import subprocess
import sys

BIN = 'E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/'
TYPES = os.environ.get('TYPES', 'Tt')


def main():
    build, log, out = sys.argv[1:4]
    where = {}
    for folder in sys.argv[4:]:
        for obj in glob.glob(os.path.join(build, folder, '**', '*.obj'), recursive=True):
            name = os.path.basename(folder).replace('.dir', '') + '/' + os.path.basename(obj).split('.')[0]
            nm = subprocess.run([BIN + 'llvm-nm', '--defined-only', obj], capture_output=True, text=True).stdout
            for line in nm.splitlines():
                parts = line.split()
                if len(parts) == 3 and parts[1] in TYPES:
                    where.setdefault(parts[2], name)
    dead = set()
    for line in open(log, errors='replace'):
        if line.startswith('ld.lld: Discarded '):
            symbol = line[len('ld.lld: Discarded '):].strip()
            if symbol in where:
                dead.add(symbol)
    dead = sorted(dead)
    names = subprocess.run([BIN + 'llvm-cxxfilt', '-_'], input='\n'.join(dead), capture_output=True,
                           text=True).stdout.splitlines()
    rows = sorted((where[s], n) for s, n in zip(dead, names))
    with open(out, 'w', newline='\n') as f:
        for obj, name in rows:
            f.write('%s\t%s\n' % (obj, name))
    print(len(rows), 'discarded, defined in the given objects')


if __name__ == '__main__':
    main()
