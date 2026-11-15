"""Renames a local variable or a parameter inside its scope (docs/CODE_STANDARDS.md, "Tools"), skipping comments, strings and
character literals.

    python -I games/rocketmania/tools/readability/scoped.py <file> <line> <old> <new> local|param

`local`: the scope runs from the declaration's line to the brace that closes the block it is in. `param`: from the declaration's
line to the end of the function's body (the first block that follows). Prints the number of places renamed. The renames are
proved by the object code afterwards (tools/objsnap.py), as every rename of the pass is.
"""
import re
import sys


def main():
    path, line_no, old, new, kind = sys.argv[1], int(sys.argv[2]), sys.argv[3], sys.argv[4], sys.argv[5]
    with open(path, newline='', errors='replace') as f:
        text = f.read()
    start = 0
    for _ in range(line_no - 1):
        start = text.index('\n', start) + 1
    out = [text[:start]]
    i, n = start, len(text)
    depth, opened, count = 0, False, 0
    while i < n:
        c = text[i]
        if text.startswith('//', i):
            j = text.index('\n', i) if '\n' in text[i:] else n
            out.append(text[i:j])
            i = j
            continue
        if text.startswith('/*', i):
            j = text.index('*/', i) + 2
            out.append(text[i:j])
            i = j
            continue
        if c in '"\'':
            j = i + 1
            while text[j] != c:
                j += 2 if text[j] == '\\' else 1
            out.append(text[i:j + 1])
            i = j + 1
            continue
        if c == '{':
            depth += 1
            opened = True
        elif c == '}':
            depth -= 1
            if kind == 'local' and depth < 0:
                break
            if kind == 'param' and opened and depth == 0:
                out.append(c)
                i += 1
                break
        if c.isalpha() or c == '_':
            m = re.compile(r'\w+').match(text, i)
            word = m.group(0)
            member = text[i - 1:i] == '.' or text[i - 2:i] in ('->', '::')
            if word == old and not member:
                word = new
                count += 1
            out.append(word)
            i = m.end()
            continue
        out.append(c)
        i += 1
    out.append(text[i:])
    with open(path, 'w', newline='') as f:
        f.write(''.join(out))
    print('%s: %d places of %s renamed to %s' % (path, count, old, new))


main()
