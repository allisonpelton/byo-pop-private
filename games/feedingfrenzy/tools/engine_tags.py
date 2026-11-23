#!/usr/bin/env python3
"""Give the addresses in the comments of Sprout's shared engine (engines/sprout) their games' tags (docs/ENGINES.md,
"The address tag").

    python -I games/feedingfrenzy/tools/engine_tags.py tag wordharmony [files...]
        appends " (wordharmony)" to every exe address (0x400000-0x7fffff) in a // comment that has no tag yet
    python -I games/feedingfrenzy/tools/engine_tags.py add pizzafrenzy <map.tsv> [files...]
        before each "0x<a> (wordharmony)" whose address the map pairs (two columns: Word Harmony's address, Pizza
        Frenzy's), writes "0x<b> (pizzafrenzy), "; a range is paired when both of its ends are

With no files, every .h and .cpp file of engines/sprout.
"""
import glob
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
ADDR = re.compile(r"\b0x([0-9a-fA-F]{5,8})\b(\.\.0x([0-9a-fA-F]{5,8})\b)?(?! \([a-z0-9]+\))")
TAGGED = re.compile(r"\b0x([0-9a-fA-F]{5,8})\b(?:\.\.0x([0-9a-fA-F]{5,8})\b)? \(wordharmony\)")
LOW, HIGH = 0x400000, 0x7FFFFF


def comment_start(line):
    """The index of the line's // comment outside a string, or -1."""
    in_str = None
    i = 0
    while i < len(line):
        c = line[i]
        if in_str:
            if c == "\\":
                i += 2
                continue
            if c == in_str:
                in_str = None
        elif c in "\"'":
            in_str = c
        elif line.startswith("//", i):
            return i
        i += 1
    return -1


def in_exe(*addrs):
    return any(a is not None and LOW <= int(a, 16) <= HIGH for a in addrs)


def tag_line(line, game):
    at = comment_start(line)
    if at < 0:
        return line
    head, body = line[:at], line[at:]

    def repl(m):
        if not in_exe(m.group(1), m.group(3)):
            return m.group(0)
        return m.group(0) + " (%s)" % game
    return head + ADDR.sub(repl, body)


def add_line(line, game, pairs):
    at = comment_start(line)
    if at < 0:
        return line
    head, body = line[:at], line[at:]

    def repl(m):
        lo = pairs.get(int(m.group(1), 16))
        hi = pairs.get(int(m.group(2), 16)) if m.group(2) else None
        if lo is None or (m.group(2) and hi is None):
            return m.group(0)
        mine = "0x%x" % lo + ("..0x%x" % hi if m.group(2) else "")
        return "%s (%s), %s" % (mine, game, m.group(0))
    return head + TAGGED.sub(repl, body)


def main():
    if len(sys.argv) < 3 or sys.argv[1] not in ("tag", "add"):
        sys.exit(__doc__)
    mode, game = sys.argv[1], sys.argv[2]
    rest = sys.argv[3:]
    pairs = {}
    if mode == "add":
        for row in open(rest.pop(0)):
            cols = row.split()
            if len(cols) >= 2 and not row.startswith("#"):
                pairs[int(cols[0], 16)] = int(cols[1], 16)
    files = rest or sorted(glob.glob(os.path.join(ROOT, "engines", "sprout", "*.h")) +
                           glob.glob(os.path.join(ROOT, "engines", "sprout", "*.cpp")))
    changed = 0
    for path in files:
        text = open(path, encoding="utf-8", newline="").read()
        lines = text.split("\n")
        out = [tag_line(l, game) if mode == "tag" else add_line(l, game, pairs) for l in lines]
        changed += sum(1 for a, b in zip(lines, out) if a != b)
        if out != lines:
            open(path, "w", encoding="utf-8", newline="").write("\n".join(out))
    print("%d lines changed in %d files" % (changed, len(files)))


if __name__ == "__main__":
    main()
