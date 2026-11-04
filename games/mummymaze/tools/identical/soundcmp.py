#!/usr/bin/env python3
"""soundcmp.py <port sound log> <origrun sound.log>: the sample plays of the port (BYOPOP_SOUNDLOG: "<update> play <sample> #n vol= pan= pitch=")
against the original's (origrun --sound wav: a play of a buffer that duplicates a loaded one; time in virtual ms, update = ms / 10).
A sample is named by its byte size and rate. The port's updates are moved to the original's: the title click (port 99) is the original's
1500, every later update is the port's plus 2301. Prints each differing play and the counts."""
import re, sys
import difflib
ALIGN = '-align' in sys.argv
port_loads, port_plays = {}, []
for l in open(sys.argv[1]):
    m = re.match(r'0 load (\d+) (\S+) frames=(\d+) rate=(\d+) ch=(\d+)', l)
    if m:
        port_loads[int(m.group(1))] = (m.group(2), int(m.group(3)) * int(m.group(5)) * 2, int(m.group(4)))
    m = re.match(r'(\d+) play (\d+) #\d+ vol=(-?\d+) pan=(-?\d+) pitch=([\d.]+)', l)
    if m:
        port_plays.append((int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)), float(m.group(5))))
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
    if m and int(m.group(2)) in dup:
        plays.append((int(float(m.group(1)) / 10), name_of.get(bufs.get(dup[int(m.group(2))]), '?'), int(m.group(4)), int(m.group(5)), int(m.group(6))))
pp = [((u + 1401 if u <= 99 else u + 2301), port_loads[i][0], v, p, port_loads[i][2], pitch) for u, i, v, p, pitch in port_plays]
print('port plays %d, original plays %d' % (len(pp), len(plays)))
bad = 0
for i in range(max(len(pp), len(plays))):
    a = pp[i] if i < len(pp) else None
    b = plays[i] if i < len(plays) else None
    if not a or not b or a[1] != b[1] or abs(a[0] - b[0]) > 2 or a[2] != b[2] or a[3] != b[3]:
        bad += 1
        if bad <= 15 and not ALIGN:
            print('#%d port %s original %s' % (i, a, b))
print('differences:', bad)
if ALIGN:	# the plays matched in order: what only one side plays
    ka = [(a[1], a[2], a[3]) for a in pp]; kb = [(b[1], b[2], b[3]) for b in plays]
    for tag, i1, i2, j1, j2 in difflib.SequenceMatcher(None, ka, kb, autojunk=False).get_opcodes():
        if tag != 'equal':
            print(tag, 'port', [(a[0],) + tuple(a[1:4]) for a in pp[i1:i2]], 'original', [b[:4] for b in plays[j1:j2]])
