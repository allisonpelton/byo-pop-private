# soundcmp.py <origrun sound.log> <port BYOPOP_SOUNDLOG file>: the sample plays of the original (origrun's --sound wav log: a play of a
# duplicate of a loaded buffer, time in virtual ms, update = ms / 10) against the port's, by sample name and order. A sample is
# named by the port's load line (frames, rate, channels -> the buffer's byte size). Prints the differences in order and the range
# of the original's update minus the port's for the plays that agree (the original's clock is milliseconds, the port's updates).
import re, sys, difflib
bufs, dup, op, pl, pp = {}, {}, [], {}, []
for l in open(sys.argv[1]):
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) create\s+bytes=(\d+)\s+fmt=(\d+)Hz', l)
    if m: bufs[int(m.group(2))] = (int(m.group(3)), int(m.group(4)))
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) duplicate of (\d+)', l)
    if m: dup[int(m.group(2))] = int(m.group(3))
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) play\s+(\w+)', l)
    if m: op.append((float(m.group(1)), int(m.group(2))))
for l in open(sys.argv[2]):
    m = re.match(r'\d+ load (\d+) (\S+) frames=(\d+) rate=(\d+) ch=(\d+)', l)
    if m: pl[int(m.group(1))] = (m.group(2), int(m.group(3)) * int(m.group(5)) * 2, int(m.group(4)))
    m = re.match(r'(\d+) play (\d+) #', l)
    if m: pp.append((int(m.group(1)), pl[int(m.group(2))][0]))
size = {(b, r): n for n, b, r in pl.values()}
oo = [(int(t / 10), size.get(bufs.get(dup[b]), '?%s' % (bufs.get(dup[b]),))) for t, b in op if b in dup]
sm = difflib.SequenceMatcher(None, [n for _, n in pp], [n for _, n in oo], autojunk=False)
d = []
for tag, i1, i2, j1, j2 in sm.get_opcodes():
    if tag != 'equal': print(tag, 'port', pp[i1:i2], 'orig', oo[j1:j2])
    else: d += [oo[j1 + k][0] - pp[i1 + k][0] for k in range(i2 - i1)]
print('%d port plays, %d original plays, %d agree, original minus port update %d..%d' % (len(pp), len(oo), len(d), min(d), max(d)))
