# soundcmp.py <tour stem>: the sample plays of the original (origrun's --sound wav log: a play of a duplicate of a loaded buffer, time in virtual
# ms, update = ms / 10) against the port's BYOPOP_SOUNDLOG ("<update> play <sample id> #n"). A sample is named by the port's load line (frames,
# rate, channels -> the buffer's byte size) and the original's buffers by their size. Prints the first differences and the counts.
import re, sys
D = 'build/ph2/%s/' % sys.argv[1]
port_loads, port_plays = {}, []
for l in open(D + 'port_sound.log'):
    m = re.match(r'0 load (\d+) (\S+) frames=(\d+) rate=(\d+) ch=(\d+)', l)
    if m:
        port_loads[int(m.group(1))] = (m.group(2), int(m.group(3)) * int(m.group(5)) * 2, int(m.group(4)))
    m = re.match(r'(\d+) play (\d+) #\d+', l)
    if m:
        port_plays.append((int(m.group(1)), int(m.group(2))))
size_name = {(16384, 22050): 'sounds'+chr(92)+'bad'}  # the original makes its "bad" buffer 16384 bytes long (a streamed one), the port's load line says 46080
for i, (n, b, r) in port_loads.items():
    size_name.setdefault((b, r), n)
bufs, dup, orig_plays = {}, {}, []
for l in open(D + 'orig_sound.log'):
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) create\s+bytes=(\d+)\s+fmt=(\d+)Hz', l)
    if m:
        bufs[int(m.group(2))] = (int(m.group(3)), int(m.group(4)))
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) duplicate of (\d+)', l)
    if m:
        dup[int(m.group(2))] = int(m.group(3))
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) play\s+(\w+)', l)
    if m and int(m.group(2)) in dup:
        src = bufs.get(dup[int(m.group(2))])
        orig_plays.append((int(float(m.group(1)) / 10), size_name.get(src, '?%s' % (src,)), m.group(3)))
pp = [(u, port_loads[i][0]) for u, i in port_plays]
op = [(u, n) for u, n, _ in orig_plays if u >= 2900]  # the "Updates" answer's click (update 310) is the original's alone
print('port plays %d, original plays %d' % (len(pp), len(op)))
bad = 0
for i in range(max(len(pp), len(op))):
    a = pp[i] if i < len(pp) else None
    b = op[i] if i < len(op) else None
    if not a or not b or a[1] != b[1] or abs(a[0] - b[0]) > 2:
        bad += 1
        if bad <= 12:
            print('#%d port %s original %s' % (i, a, b))
print('differences:', bad)
