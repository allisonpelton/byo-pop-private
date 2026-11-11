"""The scans of "Code the port does not need" (GAME_NOTES.md): which functions nothing refers to, what the compiler warns of
with every unused-code warning on, which members nothing reads, and which constants nothing names.

    python -I games/noahsark/tools/unneeded/scan.py functions|warnings|members|constants     (from the worktree root, after a native build)

functions  every function the game's object files define against every relocation in them (llvm-nm --defined-only, llvm-objdump -r):
           the ones nothing refers to. Constructors and destructors appear in their aliased forms, and a function the compiler has
           inlined at every call has no relocation either, so each name is then searched in the source.
warnings   every source of the game compiled with -fsyntax-only and the warnings below.
members    every member m... declared in the game's headers, with how often code reads and writes it; prints those read at most
           once. A use counts as a write when it is the left side of a plain assignment or a constructor's initializer.
constants  every enumerator, #define and file-level const of the game's sources that no other line names.
"""
import glob
import json
import os
import re
import subprocess
import sys

LLVM = 'E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/'
SRC = 'games/noahsark/src'
OBJ = 'build/native/games/noahsark/CMakeFiles/noahsark.dir/src'
WARNINGS = ('-Wall -Wextra -Wunused -Wunused-member-function -Wunused-private-field -Wunused-template -Wunused-macros '
            '-Wunreachable-code').split()


def sources():
    files = {}
    for path in glob.glob(SRC + '/**/*', recursive=True):
        if os.path.isfile(path):
            files[path.replace(os.sep, '/')] = open(path, encoding='latin-1').read()
    return files


def functions():
    objects = glob.glob(OBJ + '/**/*.obj', recursive=True)
    defined = {}
    referred = set()
    for obj in objects:
        out = subprocess.run([LLVM + 'llvm-nm', '--defined-only', obj], capture_output=True, text=True).stdout
        for line in out.splitlines():
            parts = line.split()
            if len(parts) == 3 and parts[1] in 'TtWw':
                defined.setdefault(parts[2], os.path.basename(obj).replace('.cpp.obj', ''))
        out = subprocess.run([LLVM + 'llvm-objdump', '-r', obj], capture_output=True, text=True).stdout
        for line in out.splitlines():
            parts = line.split()
            if len(parts) >= 3 and len(parts[0]) == 8 and all(c in '0123456789abcdef' for c in parts[0]):
                referred.add(parts[2])
    alone = sorted(name for name in defined if name not in referred)
    plain = subprocess.run([LLVM + 'llvm-cxxfilt', '-n'] + [name[1:] if name.startswith('__Z') else name for name in alone],
                           capture_output=True, text=True).stdout.splitlines()
    print('%d objects, %d functions defined, %d with no relocation to them' % (len(objects), len(defined), len(alone)))
    for name, text in zip(alone, plain):
        print('  %-14s %s' % (defined[name], text))


def warnings():
    root = os.getcwd().replace(os.sep, '/')
    count = 0
    for entry in json.load(open('build/native/compile_commands.json')):
        path = entry['file'].replace(os.sep, '/')
        if not path.startswith(root + '/' + SRC + '/'):
            continue
        command = re.sub(r' -o \S+', '', entry['command']).replace(' -Wno-unused-private-field', '')
        command = command.replace(' -c ', ' -fsyntax-only ' + ' '.join(WARNINGS) + ' ')
        result = subprocess.run(command, capture_output=True, text=True, cwd=entry['directory'])
        for line in result.stderr.splitlines():
            if 'warning:' in line:
                count += 1
                print(line.replace(os.sep, '/').replace(root + '/', '')[:260])
    print('%d warnings' % count)


def members():
    files = sources()
    found = set()
    for path, text in files.items():
        if path.endswith('.h'):
            for match in re.finditer(r'^\s+[A-Za-z_][\w:<>\*&, ]*?[\s\*&]+(m[A-Z]\w*)\s*(?:\[[^\]]*\])*\s*;', text, re.M):
                found.add((match.group(1), path))
    print('%d members' % len(found))
    for name, header in sorted(found):
        reads = writes = 0
        places = []
        for path, text in files.items():
            for number, line in enumerate(text.split('\n'), 1):
                code = line.split('//')[0]
                for match in re.finditer(r'\b' + name + r'\b', code):
                    after = code[match.end():]
                    before = code[:match.start()]
                    if path == header and re.match(r'\s*(\[[^\]]*\])*\s*;', after) and not re.search(r'[=\(]', before):
                        continue                    # the declaration
                    if re.match(r'\s*(\[[^\]]*\])*\s*=[^=]', after) or (re.match(r'\s*\(', after) and re.search(r'^[\s,:]*$', before)):
                        writes += 1
                    else:
                        reads += 1
                    places.append('%s:%d' % (os.path.basename(path), number))
        if reads <= 1:
            print('%-24s %-12s reads %d writes %d  %s' % (name, os.path.basename(header), reads, writes, ' '.join(places[:7])))


def constants():
    files = sources()
    names = set()
    for path, text in files.items():
        for match in re.finditer(r'^\s*#define\s+([A-Za-z_]\w*)', text, re.M):
            names.add((match.group(1), path))
        for match in re.finditer(r'^\s*(?:static\s+)?const\s+[\w:\*\s]+?\b([A-Za-z_]\w*)\s*(?:\[[^\]]*\])*\s*=', text, re.M):
            names.add((match.group(1), path))
        for body in re.finditer(r'\benum\b[^{;]*\{([^}]*)\}', text):
            for match in re.finditer(r'^\s*([A-Z][A-Z0-9_]+)\b', re.sub(r'//[^\n]*', '', body.group(1)), re.M):
                names.add((match.group(1), path))
    print('%d constants' % len(names))
    for name, where in sorted(names):
        uses = sum(len(re.findall(r'\b' + name + r'\b', re.sub(r'//[^\n]*', '', text))) for text in files.values())
        if uses <= 1:
            print('%-28s %s' % (name, os.path.basename(where)))


if __name__ == '__main__':
    {'functions': functions, 'warnings': warnings, 'members': members, 'constants': constants}[sys.argv[1]]()
