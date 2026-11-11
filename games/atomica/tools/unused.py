#!/usr/bin/env python3
"""The survey of code the port does not need (GAME_NOTES.md, "Code the port does not need"): what in games/atomica/src
nothing refers to.

    python -I games/atomica/tools/unused.py functions    functions defined in the native build's object files that no
                                                         relocation of any of them names
    python -I games/atomica/tools/unused.py members      members (m...) of the headers that the sources name at most twice (the declaration and one other)
    python -I games/atomica/tools/unused.py constants    enumerators, #defines and consts that the sources name only once

`functions` reads build/native/games/atomica/CMakeFiles/atomica.dir (build the port first) with llvm-nm and llvm-objdump
(LLVM_BIN, else llvm-mingw's under E:/retools). A function the compiler inlined everywhere shows up too: look for its
callers in the source before believing a line. Constructors and destructors come in pairs (the complete-object and the
base-object versions); a pair with one half referenced is in use.
"""
import glob
import os
import re
import subprocess
import sys

ROOT = os.getcwd()
SRC = os.path.join(ROOT, 'games', 'atomica', 'src')
OBJ = os.path.join(ROOT, 'build', 'native', 'games', 'atomica', 'CMakeFiles', 'atomica.dir', 'src')
BIN = os.environ.get('LLVM_BIN', 'E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin')


def run(tool, *args):
    return subprocess.run([os.path.join(BIN, tool)] + list(args), capture_output=True, text=True, errors='replace').stdout


def functions():
    objs = glob.glob(os.path.join(OBJ, '**', '*.obj'), recursive=True)
    defined, referenced = {}, set()
    for o in objs:
        for l in run('llvm-nm.exe', '--defined-only', o).splitlines():
            w = l.split()
            if len(w) == 3 and w[1] in 'Tt':
                defined.setdefault(w[2], os.path.basename(o))
        for l in run('llvm-objdump.exe', '-r', o).splitlines():
            w = l.split()
            if len(w) >= 3 and re.match(r'^[0-9a-f]{8}$', w[0]):
                referenced.add(w[2].split('+')[0])
    names = sorted(n for n in defined if n not in referenced)
    demangled = run('llvm-cxxfilt.exe', *names).splitlines() if names else []
    for n, d in zip(names, demangled):
        if d.startswith('_main') or d == 'main' or 'SDL_main' in d:
            continue
        print('%-22s %s' % (defined[n].replace('.cpp.obj', ''), d))
    print('%d of %d defined functions have no reference' % (len(names), len(defined)))


def words():
    text = {}
    for f in glob.glob(os.path.join(SRC, '**', '*'), recursive=True):
        if f.endswith(('.cpp', '.h', '.inc')):
            text[os.path.relpath(f, SRC)] = open(f, errors='replace').read()
    count = {}
    for t in text.values():
        t = re.sub(r'//[^\n]*', '', t)          # a name in a comment is not a use
        for w in re.findall(r'[A-Za-z_][A-Za-z0-9_]*', t):
            count[w] = count.get(w, 0) + 1
    return text, count


def members():
    text, count = words()
    for name, t in sorted(text.items()):
        if not name.endswith('.h'):
            continue
        for m in re.finditer(r'^[ \t]+[A-Za-z_][\w:<>\*&, \t]*?[ \t\*&](m[A-Z0-9]\w*)(\[[^\]]*\])*[ \t]*;', t, re.M):
            if count.get(m.group(1), 0) <= 2:      # its declaration and at most one other place
                print('%-18s %s' % (name, m.group(1)))


def constants():
    text, count = words()
    for name, t in sorted(text.items()):
        found = set(re.findall(r'^\s*#define\s+([A-Z_][A-Z0-9_]*)', t, re.M))
        for body in re.findall(r'enum[^{;]*\{([^}]*)\}', t):
            found |= set(re.findall(r'\b([A-Z][A-Z0-9_]{2,})\b\s*(?:=|,|$)', re.sub(r'//[^\n]*', '', body), re.M))
        found |= set(re.findall(r'\bconst\s+[\w\s\*]+?\b([ks][A-Z]\w*|[A-Z][A-Z0-9_]{2,})\s*(?:\[[^\]]*\])*\s*=', t))
        for c in sorted(found):
            if count.get(c, 0) <= 1 and not c.startswith('__'):
                print('%-18s %s' % (name, c))


WARNINGS = ('-Wall -Wextra -Wunused -Wunused-member-function -Wunused-private-field -Wunused-template -Wunused-macros '
            '-Wunreachable-code -Wno-unused-parameter')


def warnings():
    """Every source of the game compiled again (syntax only) with the compiler's unused-code warnings; the commands come
    from build/native/compile_commands.json."""
    import json
    seen = set()
    for entry in json.load(open(os.path.join(ROOT, 'build', 'native', 'compile_commands.json'))):
        name = entry['file'].replace(os.sep, '/')
        if '/games/atomica/src/' not in name or name in seen:
            continue
        seen.add(name)
        cmd = re.sub(r' -o \S+', '', entry['command']).replace(' -c ', ' -fsyntax-only ') + ' ' + WARNINGS
        err = subprocess.run(cmd, shell=True, capture_output=True, text=True, errors='replace', cwd=entry['directory']).stderr
        for l in err.splitlines():
            l = l.replace(os.sep, '/')
            if 'warning:' in l and '/games/atomica/src/' in l:
                print(l.split('/games/atomica/src/')[1])
    print('%d files compiled' % len(seen))


if __name__ == '__main__':
    {'functions': functions, 'members': members, 'constants': constants, 'warnings': warnings}[sys.argv[1]]()
