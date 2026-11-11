"""Rename local variables and parameters inside one function each, for the readability pass (docs/CODE_STANDARDS.md, rules 4 and 5).

    python -I games/noahsark/tools/readability/rename.py <spec file> [--dry]

Each line of the spec is "<file under src>:<line> <old> <new>": the line holds the declaration (a local's, or the function's
header for a parameter), and the name is replaced as a whole word from that line to the end of the function it is in (the next
line that is a closing brace in the first column), comments included. A line starting with # is skipped. The line numbers are
those of the files before any rename of the same run: renames never add or remove lines. A new name already present in the range
is refused, since the two would become one. The proof that nothing changed is the object code (objcode.sh).
"""
import os
import re
import sys

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src')


def main():
    dry = '--dry' in sys.argv
    files = {}
    failed = 0
    for raw in open(sys.argv[1], encoding='utf-8'):
        raw = raw.strip()
        if not raw or raw.startswith('#'):
            continue
        where, old, new = raw.split()
        name, number = where.rsplit(':', 1)
        number = int(number)
        if name not in files:
            files[name] = open(os.path.join(SRC, name), encoding='latin-1', newline='').read().split('\n')
        lines = files[name]
        start = number - 1
        end = start
        while end < len(lines) and not lines[end].startswith('}'):
            end += 1
        word_old = re.compile(r'(?<![\w>.])' + re.escape(old) + r'\b')
        word_new = re.compile(r'\b' + re.escape(new) + r'\b')
        if not word_old.search(lines[start]):
            print('%s: no "%s" on the line: %s' % (where, old, lines[start].strip()[:80]))
            failed += 1
            continue
        if any(word_new.search(line.split('//')[0]) for line in lines[start:end + 1]):
            print('%s: "%s" is already used in the function' % (where, new))
            failed += 1
            continue
        count = 0
        for i in range(start, end + 1):
            changed, n = word_old.subn(new, lines[i])
            if n:
                lines[i] = changed
                count += n
        if dry:
            print('%s: %s -> %s, %d places, lines %d-%d' % (where, old, new, count, start + 1, end + 1))
    if failed:
        print('%d lines of the spec refused; nothing written' % failed)
        return 1
    if not dry:
        for name, lines in files.items():
            open(os.path.join(SRC, name), 'w', encoding='latin-1', newline='').write('\n'.join(lines))
        print('%d files written' % len(files))
    return 0


if __name__ == '__main__':
    sys.exit(main())
