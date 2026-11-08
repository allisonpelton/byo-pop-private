#!/usr/bin/env python3
"""soundcmp.py <port sound log> <origrun sound.log> [limit update]: the sample plays of the port (BYOPOP_SOUNDLOG) against the original's
(origrun --sound wav: plays of the DirectSound buffers that hold the 28 sounds; a sample is named by its byte size). The original's times are
the virtual clock's ms, so its first play is lined up with the port's by an offset of updates (printed); the original's script runs DELTA updates
later than the port's (case.sh), which is part of that offset. Music is BASS's mix and is not in this log."""
import re, sys
port_loads, port_plays = {}, []
for l in open(sys.argv[1]):
    m = re.match(r'0 load (\d+) (\S.*?) frames=(\d+) rate=(\d+) ch=(\d+)', l)
    if m:
        port_loads[int(m.group(1))] = (m.group(2), int(m.group(3)) * int(m.group(5)) * 2)
    m = re.match(r'(\d+) play (\d+) #\d+ vol=(-?\d+) pan=(-?\d+) pitch=([\d.]+)', l)
    if m:
        port_plays.append((int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)), float(m.group(5))))
bufs, dup, plays = {}, {}, []
for l in open(sys.argv[2], errors='ignore'):
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) create\s+bytes=(\d+)\s+fmt=(\d+)Hz.*flags=(0x\w+)', l)
    if m:
        bufs[int(m.group(2))] = (int(m.group(3)), m.group(5))
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) duplicate of (\d+)', l)
    if m:
        dup[int(m.group(2))] = int(m.group(3))
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) play\s+(\w+)\s+vol=(-?\d+) pan=(-?\d+) freq=(\d+)', l)
    if m and m.group(3) == 'once':
        root = dup.get(int(m.group(2)), int(m.group(2)))
        if bufs.get(root, (0, ''))[1] == '0x80ea':
            plays.append([float(m.group(1)) / 10, bufs[root][0], int(m.group(4)), int(m.group(5)), int(m.group(6))])
LIM = int(sys.argv[3]) if len(sys.argv) > 3 else 10 ** 9
pp = [(u, port_loads[i][1], port_loads[i][0], v, p, pitch) for u, i, v, p, pitch in port_plays if i in port_loads]
off = int(sys.argv[4]) if len(sys.argv) > 4 else (round(plays[0][0] - pp[0][0]) if plays and pp else 0)
plays = [(int(round(p[0] - off)),) + tuple(p[1:]) for p in plays]
pp = [x for x in pp if x[0] <= LIM]
plays = [x for x in plays if x[0] <= LIM]
print('offset %d updates; up to update %d: port plays %d, original plays %d' % (off, LIM, len(pp), len(plays)))
bad = 0
# the two sequences, matched as multisets per update (plays of one update come in any order)
from collections import Counter
P = Counter((u, size, v, p) for u, size, name, v, p, pitch in pp)
O = Counter((u, size, v, p) for u, size, v, p, f in plays)
names = {size: name for u, size, name, v, p, pitch in pp}
for k in sorted(set(P) | set(O)):
    if P[k] != O[k]:
        bad += 1
        print('update %d %s (%d bytes): port %d, original %d' % (k[0], names.get(k[1], '?'), k[1], P[k], O[k]))
print('%d differing plays' % bad)
