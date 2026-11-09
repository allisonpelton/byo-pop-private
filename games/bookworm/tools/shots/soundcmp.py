# soundcmp.py <tour>: the original's sample plays (origrun's --sound wav log: a play is a duplicate of a loaded buffer, its
# time in virtual ms) against the port's BYOPOP_SOUNDLOG ("<update> play <id> #n vol= pan= pitch="), in order. A sample
# is named by its size in bytes (the port's load line: frames x channels x 2; the original's buffer create) and its
# name from the port's load line. Times are compared in updates (10 ms) from each side's first play. Prints the
# first differences and the counts.
import re, sys
D = 'build/ph2/%s/' % sys.argv[1]
SLACK = int(sys.argv[2]) if len(sys.argv) > 2 else 1   # updates a play may be off

names, port_plays = {}, []
port_size = {}
for l in open(D + 'port_sound.log'):
    m = re.match(r'0 load (\d+) (\S+) frames=(\d+) rate=(\d+) ch=(\d+)', l)
    if m:
        port_size[int(m.group(1))] = int(m.group(3)) * int(m.group(5)) * 2
        names.setdefault(int(m.group(3)) * int(m.group(5)) * 2, m.group(2))
    m = re.match(r'(\d+) play (\d+) #\d+ vol=(-?\d+) pan=(-?\d+)', l)
    if m:
        port_plays.append((int(m.group(1)), port_size[int(m.group(2))], int(m.group(3)), int(m.group(4))))

size, dup, orig_plays = {}, {}, []
for l in open(D + 'orig_sound.log'):
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) create\s+bytes=(\d+)', l)
    if m:
        size[int(m.group(2))] = int(m.group(3))
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) duplicate of (\d+)', l)
    if m:
        dup[int(m.group(2))] = int(m.group(3))
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) play\s+\w+\s+vol=(-?\d+) pan=(-?\d+)', l)
    if m and int(m.group(2)) in dup:
        orig_plays.append((int(float(m.group(1)) / 10), size[dup[int(m.group(2))]], int(m.group(3)), int(m.group(4))))

print('port plays %d, original plays %d' % (len(port_plays), len(orig_plays)))
p0 = port_plays[0][0] if port_plays else 0
o0 = orig_plays[0][0] if orig_plays else 0
bad = late = 0
for i in range(max(len(port_plays), len(orig_plays))):
    a = port_plays[i] if i < len(port_plays) else None
    b = orig_plays[i] if i < len(orig_plays) else None
    same = a and b and a[1:] == b[1:]
    off = (a[0] - p0) - (b[0] - o0) if (a and b) else None
    if not same or abs(off) > SLACK:
        bad += 1
        if bad <= 12:
            fa = a and '%d %s vol=%d pan=%d' % (a[0] - p0, names.get(a[1], a[1]), a[2], a[3])
            fb = b and '%d %s vol=%d pan=%d' % (b[0] - o0, names.get(b[1], b[1]), b[2], b[3])
            print('#%d port %s | original %s' % (i, fa, fb))
print('differences:', bad)
