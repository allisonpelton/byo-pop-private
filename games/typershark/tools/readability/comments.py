"""Gives the new names to the comments and docs that still carry the old ones, after rename.py has renamed the code.

    python -I games/typershark/tools/readability/comments.py <map file>...

Reads the same maps as rename.py. An old name that two classes declare with different new names is ambiguous: it is left alone
and listed, for the person to edit. Every other old name is replaced as a whole word inside the comments of the .h and .cpp
files of games/typershark/src (not in code or strings) and anywhere in games/typershark/docs/*.md and games/typershark/mods.
Prints the number of places replaced and the ambiguous names that still occur.
"""
import collections
import glob
import os
import re
import sys

ROOT = os.getcwd()
GAME = os.path.join(ROOT, 'games', 'typershark')


def read(path):
    with open(path, newline='', errors='replace') as f:
        return f.read()


def write(path, text):
    with open(path, 'w', newline='') as f:
        f.write(text)


def replace_in_comments(text, pattern, table):
    """Replaces the words only inside // and /* */ comments; returns the new text and the number of places."""
    out, i, n, count = [], 0, len(text), 0
    while i < n:
        c = text[i]
        if text.startswith('//', i):
            j = text.find('\n', i)
            j = n if j < 0 else j
            piece, k = pattern.subn(lambda m: table[m.group(0)], text[i:j])
            out.append(piece)
            count += k
            i = j
        elif text.startswith('/*', i):
            j = text.find('*/', i)
            j = n if j < 0 else j + 2
            piece, k = pattern.subn(lambda m: table[m.group(0)], text[i:j])
            out.append(piece)
            count += k
            i = j
        elif c in '"\'':
            j = i + 1
            while j < n and text[j] != c:
                j += 2 if text[j] == '\\' else 1
            out.append(text[i:j + 1])
            i = j + 1
        else:
            out.append(c)
            i += 1
    return ''.join(out), count


def main():
    news = collections.defaultdict(set)
    for path in sys.argv[1:]:
        for line in read(path).split('\n'):
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            line = line.split(' -- ', 1)[0]
            cls, where, old, new = line.split()
            news[old].add(new)
    table = {o: next(iter(v)) for o, v in news.items() if len(v) == 1}
    ambiguous = sorted(o for o, v in news.items() if len(v) > 1)
    pattern = re.compile(r'\b(?:%s)\b' % '|'.join(re.escape(o) for o in sorted(table, key=len, reverse=True)))
    total = 0
    for path in sorted(glob.glob(os.path.join(GAME, 'src', '**', '*.[ch]*'), recursive=True)):
        if not path.endswith(('.h', '.cpp', '.inc')):
            continue
        text = read(path)
        new_text, k = replace_in_comments(text, pattern, table)
        if k:
            write(path, new_text)
            total += k
    for path in sorted(glob.glob(os.path.join(GAME, 'docs', '*.md')) + glob.glob(os.path.join(GAME, 'mods', '**', '*.md'), recursive=True)):
        text = read(path)
        new_text, k = pattern.subn(lambda m: table[m.group(0)], text)
        if k:
            write(path, new_text)
            total += k
    print('places replaced in comments and docs:', total)
    if ambiguous:
        amb = re.compile(r'\b(?:%s)\b' % '|'.join(re.escape(o) for o in ambiguous))
        for path in sorted(glob.glob(os.path.join(GAME, 'src', '**', '*.[ch]*'), recursive=True) + glob.glob(os.path.join(GAME, 'docs', '*.md'))):
            for n, line in enumerate(read(path).split('\n'), 1):
                if amb.search(line):
                    print('ambiguous: %s:%d: %s' % (os.path.relpath(path, GAME), n, line.strip()[:140]))


main()
