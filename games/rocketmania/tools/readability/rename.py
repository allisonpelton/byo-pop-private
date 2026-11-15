"""Renames members of Rocket Mania's classes, found and checked by the compiler (docs/CODE_STANDARDS.md, "Tools": rename the
declarations, compile with -fsyntax-only, replace the old name exactly where the compiler reports it missing, so that classes
that share a name are never confused).

    python -I games/rocketmania/tools/readability/rename.py <map file> [--jobs N]

The map holds one rename a line, "<Class> <file>:<line> <old> <new> [-- <comment>]": the class that declares the member, the
source file (relative to games/rocketmania/src) and the line of the declaration, the old name and the new one, and, if given,
the comment that replaces everything after the // on the declaration's line. A line starting with # is a comment. The declarations are renamed first (the old name, whole word, on that line); then every unit of the game's native
build (build/native/compile_commands.json, run from the worktree's root) is compiled with -fsyntax-only, and each place at
which the compiler reports an error with an old name at its position (no member of that name, an undeclared identifier,
a member initializer that names nothing) is given the new name of the entry whose class is the class of the report (or the
class that holds the function, for an undeclared identifier, or a base of either). This repeats until the game compiles.
Strings and comments are never touched: a comment that mentions the old name is for the person to edit.

It prints one line a round and, at the end, how many places it renamed.
"""
import collections
import json
import os
import re
import subprocess
import sys
from concurrent.futures import ThreadPoolExecutor

ROOT = os.getcwd()
SRC = os.path.join(ROOT, 'games', 'rocketmania', 'src')
CLASS_RE = re.compile(r'^\s*(?:class|struct)\s+(\w+)\s*(?::([^{;]*))?[{\n]?')


def read(path):
    with open(path, newline='', errors='replace') as f:
        return f.read()


def write(path, text):
    with open(path, 'w', newline='') as f:
        f.write(text)


def bases_of_classes():
    """{class: [direct base classes]} from the headers' class lines."""
    bases = {}
    for dirpath, _, files in os.walk(SRC):
        for name in files:
            if not name.endswith('.h'):
                continue
            for line in read(os.path.join(dirpath, name)).split('\n'):
                m = re.match(r'^\s*(?:class|struct)\s+(\w+)\s*:\s*(.*?)\s*(?:\{.*)?$', line.rstrip('\r'))
                if m:
                    found = re.findall(r'(?:public|private|protected|virtual)?\s*(?:virtual\s+)?(\w+)\s*(?:,|$)', m.group(2))
                    bases[m.group(1)] = [b for b in found if b not in ('public', 'private', 'protected', 'virtual')]
    return bases


def lineage(cls, bases):
    """The class and its bases, nearest first."""
    out, todo = [], [cls]
    while todo:
        c = todo.pop(0)
        if c in out:
            continue
        out.append(c)
        todo.extend(bases.get(c, []))
    return out


def enclosing_class(path, line_no):
    """The class that holds the function around a line: Class::Method at the start of a line above it (a .cpp), or the
    class declaration above it (a header)."""
    lines = read(path).split('\n')
    i = line_no - 1
    if path.endswith('.h'):
        while i >= 0:
            m = re.match(r'^\s*(?:class|struct)\s+(\w+)\b', lines[i])
            if m and not lines[i].rstrip().endswith(';'):
                return m.group(1)
            i -= 1
        return None
    while i >= 0:
        m = re.match(r'^(?:[A-Za-z_][\w:<>,\s\*&~]*?\s)?(\w+)::~?\w+\s*\(', lines[i])
        if m:
            return m.group(1)
        i -= 1
    return None


def compile_one(entry):
    command = re.sub(r'\s-o\s+\S+', '', entry['command'])
    command = command.replace(' -c ', ' -fsyntax-only -ferror-limit=0 -fno-color-diagnostics -fno-caret-diagnostics -c ', 1)
    r = subprocess.run(command, capture_output=True, text=True, cwd=entry['directory'])
    return r.stderr


def main():
    args = sys.argv[1:]
    jobs = 4
    if '--jobs' in args:
        i = args.index('--jobs')
        jobs = int(args[i + 1])
        del args[i:i + 2]
    entries = []
    for line in read(args[0]).split('\n'):
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        comment = None
        if ' -- ' in line:
            line, comment = line.split(' -- ', 1)
        cls, where, old, new = line.split()
        f, n = where.rsplit(':', 1)
        entries.append({'cls': cls, 'file': os.path.join(SRC, f), 'line': int(n), 'old': old, 'new': new, 'comment': comment})
    bases = bases_of_classes()
    by_old = collections.defaultdict(list)
    for e in entries:
        by_old[e['old']].append(e)

    # the declarations
    changed = 0
    for e in entries:
        lines = read(e['file']).split('\n')
        text = lines[e['line'] - 1]
        pattern = re.compile(r'\b%s\b' % re.escape(e['old']))
        if not pattern.search(text) and re.search(r'\b%s\b' % re.escape(e['new']), text):
            continue    # done by an earlier run
        if not pattern.search(text):
            sys.exit('no %s on %s:%d: %s' % (e['old'], e['file'], e['line'], text.strip()))
        text = pattern.sub(e['new'], text)
        if e['comment'] is not None:
            tail = '\r' if text.endswith('\r') else ''
            text = re.sub(r'//.*$', lambda m: '// ' + e['comment'], text.rstrip('\r')) + tail
        lines[e['line'] - 1] = text
        write(e['file'], '\n'.join(lines))
        changed += 1
    print('declarations renamed:', changed, flush=True)

    cc = json.load(open(os.path.join(ROOT, 'build', 'native', 'compile_commands.json')))
    units = [e for e in cc if 'games/rocketmania/' in e['file'].replace('\\', '/')]
    olds = sorted(by_old, key=len, reverse=True)
    old_re = re.compile(r'\b(%s)\b' % '|'.join(re.escape(o) for o in olds))
    for rnd in range(1, 12):
        with ThreadPoolExecutor(jobs) as pool:
            outputs = list(pool.map(compile_one, units))
        sites = {}
        other = []
        for out in outputs:
            for m in re.finditer(r'^(\S[^\n]*?):(\d+):(\d+): error: ([^\n]*)', out, re.M):
                path, ln, col, msg = m.group(1), int(m.group(2)), int(m.group(3)), m.group(4)
                path = os.path.normpath(path)
                if not os.path.isfile(path):
                    other.append(m.group(0))
                    continue
                text = read(path).split('\n')[ln - 1]
                hit = old_re.match(text, col - 1)
                if not hit:
                    other.append(m.group(0))
                    continue
                old = hit.group(1)
                cands = by_old[old]
                chosen = None
                if len(cands) == 1:
                    chosen = cands[0]
                else:
                    c = re.search(r"in '(?:[\w:]*::)?(\w+)'", msg)
                    cls = c.group(1) if c else enclosing_class(path, ln)
                    for k in lineage(cls, bases) if cls else []:
                        chosen = next((x for x in cands if x['cls'] == k), None)
                        if chosen:
                            break
                if not chosen:
                    other.append(m.group(0) + '  [class not found]')
                    continue
                sites[(path, ln, col)] = (old, chosen['new'])
        print('round %d: %d places to rename, %d other errors' % (rnd, len(sites), len(other)), flush=True)
        if not sites:
            if other:
                for o in sorted(set(other))[:40]:
                    print('  ' + o)
                sys.exit(1)
            break
        per_file = collections.defaultdict(list)
        for (path, ln, col), (old, new) in sites.items():
            per_file[path].append((ln, col, old, new))
        for path, items in per_file.items():
            lines = read(path).split('\n')
            for ln, col, old, new in sorted(items, reverse=True):
                text = lines[ln - 1]
                assert text[col - 1:col - 1 + len(old)] == old, (path, ln, col)
                lines[ln - 1] = text[:col - 1] + new + text[col - 1 + len(old):]
                changed += 1
            write(path, '\n'.join(lines))
    print('places renamed:', changed)


main()
