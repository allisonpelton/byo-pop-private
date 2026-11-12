#!/usr/bin/env python3
"""Renames a local or a parameter inside its own scope (docs/CODE_STANDARDS.md, rule 5), comments and strings left alone.

    python scoperename.py <kind> <file>:<line> <old> <new> [<kind> <file>:<line> <old> <new> ...]

<kind> is L (a local, declared on <line>: its scope runs to the end of the block that holds the declaration) or P (a
parameter, named on <line>: its scope is the function's body, or just that line for a prototype). Every identifier
`old` in the scope, outside strings, character literals and comments, becomes `new`. Run the compiler afterwards (and
`tools/objsnap.py`, which shows a rename that changed what a name meant).
"""
import re, sys


def code_spans(line, in_block):
    """[(start, end)] of the line's code (not strings, characters or comments); the block-comment state after it."""
    spans = []
    i, n, start = 0, len(line), 0
    while i < n:
        if in_block:
            j = line.find("*/", i)
            if j < 0:
                return spans, True
            i = start = j + 2
            in_block = False
            continue
        c = line[i]
        if line.startswith("//", i):
            if i > start:
                spans.append((start, i))
            return spans, False
        if line.startswith("/*", i):
            if i > start:
                spans.append((start, i))
            in_block = True
            continue
        if c in "\"'":
            if i > start:
                spans.append((start, i))
            j = i + 1
            while j < n and line[j] != c:
                j += 2 if line[j] == "\\" else 1
            i = start = j + 1
            continue
        i += 1
    if start < n:
        spans.append((start, n))
    return spans, in_block


def braces(line, in_block):
    spans, in_block = code_spans(line, in_block)
    seq = []
    for a, b in spans:
        for k in range(a, b):
            if line[k] in "{};":
                seq.append(line[k])
    return seq, in_block


def scope_end(lines, first, kind):
    """The last line index of the scope of a name declared on lines[first]."""
    depth, in_block, opened = 0, False, False
    for idx in range(first, len(lines)):
        seq, in_block = braces(lines[idx], in_block)
        for c in seq:
            if c == "{":
                depth += 1
                opened = True
            elif c == "}":
                depth -= 1
                if kind == "L" and depth < 0:
                    return idx
                if kind == "P" and opened and depth == 0:
                    return idx
            elif c == ";" and kind == "P" and not opened and depth == 0:
                return idx
    return len(lines) - 1


def rename(path, line, old, new, kind):
    text = open(path, "rb").read().decode("latin-1")
    lines = text.split("\n")
    first = line - 1
    last = scope_end(lines, first, kind)
    pat = re.compile(r"(?<![A-Za-z0-9_])" + re.escape(old) + r"(?![A-Za-z0-9_])")
    count, in_block = 0, False
    for idx in range(first, last + 1):
        spans, in_block = code_spans(lines[idx], in_block)
        s = lines[idx]
        out, pos = [], 0
        for a, b in spans:
            out.append(s[pos:a])
            seg, c = pat.subn(new, s[a:b])
            count += c
            out.append(seg)
            pos = b
        out.append(s[pos:])
        lines[idx] = "".join(out)
    open(path, "wb").write("\n".join(lines).encode("latin-1"))
    return count, first + 1, last + 1


if __name__ == "__main__":
    args = sys.argv[1:]
    if len(args) % 4:
        print(__doc__)
        sys.exit(2)
    # apply from the bottom of each file up, so the line numbers given stay true (a rename never changes a line count)
    for i in range(0, len(args), 4):
        kind, loc, old, new = args[i:i + 4]
        path, line = loc.rsplit(":", 1)
        count, a, b = rename(path, int(line), old, new, kind)
        print("%s:%s %s -> %s: %d uses in lines %d..%d" % (path, line, old, new, count, a, b))
        if count == 0:
            print("  NOTHING RENAMED")
