#!/usr/bin/env python3
"""Phase two of the identical-play pass: the same scenario in the port and in the original, shots compared by tools/shotdiff.

  p2.py make <scenario> <name> <outdir>
      writes <outdir>/<name>.port.txt (the port's -script file: shots go to <outdir>/<name>/port/<shot>.png) and
      <outdir>/<name>.orig.txt (origrun's script: `shot <shot>`)

A scenario is one command per line, `<port update> <command> [args]` in the port's script syntax, plus
  shot <name>          a screenshot of both, at that update
  title                a click on the title's play link (the original's is always at update 1500: its load takes longer)
Updates are the port's; the original's are those plus OFFSET (2301: the original's first menu link click at 2400 is the
port's at 99). Commands: click x y, move x y, key N, keydown N, keyup N, char C, focus lose|regain, quit.
A `key N` is a key held for the one update (`kdown` then `kup` one update later in the original, which polls its keys).
"""
import sys

OFFSET = 2301


def parse(path):
    out = []
    for line in open(path):
        line = line.split('#')[0].strip()
        if not line:
            continue
        u, cmd, *rest = line.split(None, 2)
        out.append((int(u), cmd, rest[0] if rest else ''))
    return out


def make(scn, name, outdir):
    cmds = parse(scn)
    port, orig = [], []
    for u, c, a in cmds:
        if c == 'shot':
            port.append((u, 'screenshot %s/%s/port/%s.png' % (outdir, name, a)))
            orig.append((u + OFFSET, 'shot %s' % a))
        elif c == 'title':
            port.append((u, 'click 323 470'))
            orig.append((1500, 'click 323 470'))
        elif c in ('click', 'move'):
            x, y = a.split()
            port.append((u, '%s %s %s' % (c, x, y)))
            if c == 'click':
                orig.append((u + OFFSET - 1, 'move %s %s' % (x, y)))
            orig.append((u + OFFSET, '%s %s %s' % (c, x, y)))
        elif c == 'key':
            port.append((u, 'key %s' % a))
            orig.append((u + OFFSET, 'kdown %s' % a))
            orig.append((u + OFFSET + 1, 'kup %s' % a))
        elif c in ('keydown', 'keyup'):
            port.append((u, '%s %s' % (c, a)))
            orig.append((u + OFFSET, 'kdown %s' % a if c == 'keydown' else 'kup %s' % a))
        elif c == 'char':
            port.append((u, 'char %s' % a))
            orig.append((u + OFFSET, 'text %s' % a))
        elif c == 'focus':
            port.append((u, 'focus %s' % a))
            orig.append((u + OFFSET, 'focus %s' % a))
        elif c == 'quit':
            port.append((u, 'quit'))
            orig.append((u + OFFSET + 5, 'quit'))
        else:
            raise SystemExit('unknown command ' + c)
    port.sort(key=lambda t: t[0])
    orig.sort(key=lambda t: t[0])
    open('%s/%s.port.txt' % (outdir, name), 'w', newline='\n').write(''.join('%d %s\n' % t for t in port))
    open('%s/%s.orig.txt' % (outdir, name), 'w', newline='\n').write(''.join('at %d %s\n' % t for t in orig))


if __name__ == '__main__':
    if sys.argv[1] == 'make':
        make(*sys.argv[2:5])
