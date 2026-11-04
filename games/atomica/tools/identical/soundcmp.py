#!/usr/bin/env python3
"""soundcmp.py <port sound log> <origrun sound.log>: the sample plays of the port (BYOPOP_SOUNDLOG: "<update> play <sample> #n vol= pan= pitch=")
against the original's (origrun --sound wav: a play of a DirectSound buffer that is a loaded one or duplicates one; its time is the virtual clock's ms, which
includes the load before the first update, so the original's times are moved by the offset that puts its first play on the port's first; a sample is
named by its byte size and rate). Prints each differing play and the counts. The port's updates run one before the original's (its script runs at the
start of an update), so the port's are moved by one."""
import re, sys
port_loads, port_plays = {}, []
for l in open(sys.argv[1]):
    m = re.match(r'0 load (\d+) (\S+) frames=(\d+) rate=(\d+) ch=(\d+)', l)
    if m:
        port_loads[int(m.group(1))] = (m.group(2), int(m.group(3)) * int(m.group(5)) * 2, int(m.group(4)))
    m = re.match(r'(\d+) play (\d+) #\d+ vol=(-?\d+) pan=(-?\d+) pitch=([\d.]+)', l)
    if m:
        port_plays.append((int(m.group(1)) + 1, int(m.group(2)), int(m.group(3)), int(m.group(4)), float(m.group(5))))
name_of = {(b, r): n for n, b, r in port_loads.values()}
bufs, dup, plays = {}, {}, []
for l in open(sys.argv[2]):
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) create\s+bytes=(\d+)\s+fmt=(\d+)Hz', l)
    if m:
        bufs[int(m.group(2))] = (int(m.group(3)), int(m.group(4)))
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) duplicate of (\d+)', l)
    if m:
        dup[int(m.group(2))] = int(m.group(3))
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) play\s+(\w+)\s+vol=(-?\d+) pan=(-?\d+) freq=(\d+)', l)
    if m and m.group(3) == 'once' and (int(m.group(2)) in dup or int(m.group(2)) in bufs):	# a play of the loaded buffer itself or of a duplicate of it
        plays.append([float(m.group(1)) / 10, name_of.get(bufs.get(dup.get(int(m.group(2)), int(m.group(2)))), '?'), int(m.group(4)), int(m.group(5)), int(m.group(6))])
off = (plays[0][0] - port_plays[0][0]) if plays and port_plays else 0
for p in plays:
    p[0] = int(round(p[0] - off))
canon = lambda n: n.replace('electric2', 'electric3')	# the same size, told apart by nothing in the log
pp = [(u, canon(port_loads[i][0]), v, p, port_loads[i][2], pitch) for u, i, v, p, pitch in port_plays]
print('port plays %d, original plays %d, offset %.0f updates' % (len(pp), len(plays), off))
bad = 0
LIM = int(sys.argv[3]) if len(sys.argv) > 3 else 10 ** 9
pp = [x for x in pp if x[0] <= LIM]
plays = [x for x in plays if x[0] <= LIM]
print('up to update %d: port plays %d, original plays %d' % (LIM, len(pp), len(plays)))
for i in range(max(len(pp), len(plays))):
    a = pp[i] if i < len(pp) else None
    b = plays[i] if i < len(plays) else None
    if not a or not b or a[1] != b[1] or abs(a[0] - b[0]) > 2 or a[2] != b[2] or a[3] != b[3]:
        bad += 1
        if bad <= 15:
            print('#%d port %s original %s' % (i, a, b))
print('differences:', bad)
