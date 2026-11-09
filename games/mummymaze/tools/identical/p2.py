#!/usr/bin/env python3
"""Phase two of the identical-play pass: the same scenario in the port and in the original, shots compared by tools/shotdiff.

  p2.py make <scenario> <name> <outdir>
      writes <outdir>/<name>.port.txt (the port's -script file: shots go to <outdir>/<name>/port/<shot>.png) and
      <outdir>/<name>.orig.txt (origrun's script: `shot <shot>`)

A scenario is one command per line, `<port update> <command> [args]` in the port's script syntax, plus
  shot <name>          a screenshot of both, at that update
  title                a click on the title's play link (the original's is always at update 1500: its load takes longer)
Updates are the port's; the original's are those plus OFFSET (2301: the original's first menu link click at 2400 is the
port's at 99). Commands: click x y, move x y, down x y, up x y (a drag), key N, keydown N, keyup N, char C, focus lose|regain, quit.
A `key N` is held for one update (down at U, up at U+1, as the port's bot does; the original gets `kdown` and `kup` then).
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
        elif c in ('down', 'up'):          # one edge of the left button, at the point (a slider's drag is down, moves, up)
            x, y = a.split()
            port.append((u, '%s %s %s' % (c, x, y)))
            orig.append((u + OFFSET - 1, 'move %s %s' % (x, y)))
            orig.append((u + OFFSET, c))
        elif c == 'key':
            port.append((u, 'keydown %s' % a))
            port.append((u + 1, 'keyup %s' % a))
            orig.append((u + OFFSET, 'kdown %s' % a))
            orig.append((u + OFFSET + 1, 'kup %s' % a))
        elif c in ('keydown', 'keyup'):
            port.append((u, '%s %s' % (c, a)))
            orig.append((u + OFFSET, 'kdown %s' % a if c == 'keydown' else 'kup %s' % a))
        elif c == 'char':
            port.append((u, 'char %s' % a))
            orig.append((u + OFFSET, 'text %s' % a))
        elif c == 'text':
            for ch in a:
                port.append((u, 'char %s' % ch))
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


def fromlog(log, out, step, last):
    """A scenario that replays a phase-one bot log (its keys and clicks, from the menu click on) and takes a shot every
    STEP updates up to LAST updates after the menu click"""
    import os
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    from identical import port_inputs
    ins = port_inputs(log)
    menu = [i for i, (u, c) in enumerate(ins) if c.startswith('menuclick')]
    # a log without the menu click: the bot clicked the classic link at update 99
    mi, mu, mc = (menu[0], ins[menu[0]][0], ins[menu[0]][1].split(' ', 1)[1]) if menu else (-1, 99, '185 376')
    at = 1000                       # the menu click's update in the scenario
    lines = ['99 title', '%d click %s' % (at, mc)]
    for u, c in ins[mi + 1:]:
        if u - mu > last:
            break
        lines.append('%d %s' % (u - mu + at, c))
    for u in range(step, last + 1, step):
        lines.append('%d shot t%05d' % (u + at, u))
    lines.append('%d quit' % (last + at + 20))
    lines.sort(key=lambda l: int(l.split()[0]))
    open(out, 'w', newline=chr(10)).write(chr(10).join(lines) + chr(10))

if __name__ == '__main__':
    if sys.argv[1] == 'make':
        make(*sys.argv[2:5])
    elif sys.argv[1] == 'fromlog':
        fromlog(sys.argv[2], sys.argv[3], int(sys.argv[4]), int(sys.argv[5]))
