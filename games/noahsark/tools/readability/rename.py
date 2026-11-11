"""Rename local variables and parameters inside one function each, for the readability pass (docs/CODE_STANDARDS.md, rules 4 and 5).

    python -I games/noahsark/tools/readability/rename.py <spec file> [--dry]

Each line of the spec is "<file under src>:<line> <old> <new>": the line holds the declaration, and the name is replaced as
a whole word in the code from that line to the end of its scope: not in string or character literals, and not in comments (a
comment in the scope that has the old name as a word is printed, to be read and changed by hand). A function's header (a line that starts in the first
column) is the scope of its parameters: to the next closing brace in the first column. A local's scope is the block its
declaration is in: to the brace that closes it. A line starting with # is skipped, and so is a declaration an earlier line of
the spec has already renamed (a second loop with the same counter). The line numbers are those of the files before any rename of
the same run: renames never add or remove lines. A new name already present in the scope is refused, since the two would become
one. The proof that nothing changed is the object code (objcode.sh).
"""
import os
import re
import sys

SRC = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'src')
STRINGS = re.compile(r'''"(?:[^"\\]|\\.)*"|'(?:[^'\\]|\\.)*' ''', re.X)     # string and character literals: their braces do not count


def main():
    dry = '--dry' in sys.argv
    files = {}
    notes = []
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
        if lines[start][:1].isspace():
            depth = 0
            while end < len(lines) - 1:
                code = STRINGS.sub('', lines[end]).split('//')[0]
                depth += code.count('{') - code.count('}')
                if depth < 0 or (end > start and lines[end].startswith('}')):
                    break
                end += 1
        else:
            while end < len(lines) - 1 and not lines[end].startswith('}'):
                end += 1
        word_old = re.compile(r'(?<![\w>.])' + re.escape(old) + r'\b')
        word_new = re.compile(r'\b' + re.escape(new) + r'\b')
        if not word_old.search(lines[start]):
            if word_new.search(lines[start]):
                continue                            # renamed by an earlier line of the spec
            print('%s: no "%s" on the line: %s' % (where, old, lines[start].strip()[:80]))
            failed += 1
            continue
        if any(word_new.search(line.split('//')[0]) for line in lines[start:end + 1]):
            print('%s: "%s" is already used in the function' % (where, new))
            failed += 1
            continue
        count = 0
        for i in range(start, end + 1):
            # the code of the line only: not its string and character literals, and not its comment, which is listed instead
            parts = []
            last = 0
            comment = len(lines[i])
            for literal in STRINGS.finditer(lines[i]):
                slash = lines[i].find('//', last, literal.start())
                if slash >= 0:
                    comment = slash
                    break
                parts.append((last, literal.start()))
                last = literal.end()
            else:
                slash = lines[i].find('//', last)
                if slash >= 0:
                    comment = slash
            parts.append((last, comment))
            line = lines[i]
            for a, b in reversed(parts):
                changed, n = word_old.subn(new, line[a:b])
                if n:
                    line = line[:a] + changed + line[b:]
                    count += n
            if word_old.search(line[comment:] if comment < len(lines[i]) else ''):
                notes.append('%s:%d (%s -> %s): %s' % (name, i + 1, old, new, lines[i][comment:].strip()[:110]))
            lines[i] = line
        if dry:
            print('%s: %s -> %s, %d places, lines %d-%d' % (where, old, new, count, start + 1, end + 1))
    if failed:
        print('%d lines of the spec refused; nothing written' % failed)
        return 1
    for note in notes:
        print('comment ' + note)
    if not dry:
        for name, lines in files.items():
            open(os.path.join(SRC, name), 'w', encoding='latin-1', newline='').write('\n'.join(lines))
        print('%d files written' % len(files))
    return 0


if __name__ == '__main__':
    sys.exit(main())
