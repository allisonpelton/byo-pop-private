# soundcmp.py <tour stem>: the sample plays of the original (origrun's --sound wav log: a play of a duplicate of a loaded buffer, time in
# virtual ms) against the port's BYOPOP_SOUNDLOG ("<update> play <sample id> #n vol= pan= pitch="). The original's buffers for samples are
# the ones created with flags 0x40ca, in the order the game loads them, which is the port's sample ids; the virtual clock's ms are the
# update counter x 10 plus a constant (the original's loading time), taken from the first play. Prints the first differences, the counts
# and the largest gap in updates.
import re, sys
D = 'build/ph2/%s/' % sys.argv[1]
port_plays = []
for l in open(D + 'port_sound.log'):
    m = re.match(r'(\d+) play (\d+) #\d+ vol=(-?\d+) pan=(-?\d+)', l)
    if m:
        port_plays.append((int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4))))
samples = []   # buffer numbers created as samples, in order
dup, orig_plays, vol = {}, [], {}
for l in open(D + 'orig_sound.log'):
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) create\s+bytes=(\d+)\s+fmt=\S+\s+flags=0x40ca', l)
    if m:
        samples.append(int(m.group(2)))
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) duplicate of (\d+)', l)
    if m:
        dup[int(m.group(2))] = int(m.group(3))
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) play\s+(\w+)\s+vol=(-?\d+) pan=(-?\d+)', l)
    if m and int(m.group(2)) in dup and dup[int(m.group(2))] in samples:
        orig_plays.append((float(m.group(1)) / 10, samples.index(dup[int(m.group(2))]), int(m.group(4)), int(m.group(5))))
print('port plays %d, original plays %d' % (len(port_plays), len(orig_plays)))
if not port_plays or not orig_plays:
    sys.exit(1)
off = orig_plays[0][0] - port_plays[0][0]
print('clock offset (updates): %.1f' % off)
bad, worst = 0, 0
for i in range(max(len(port_plays), len(orig_plays))):
    a = port_plays[i] if i < len(port_plays) else None
    b = orig_plays[i] if i < len(orig_plays) else None
    if a and b:
        worst = max(worst, abs(b[0] - off - a[0]))
    if not a or not b or a[1] != b[1] or abs(b[0] - off - a[0]) > 2 or a[2] != b[2] or a[3] != b[3]:
        bad += 1
        if bad <= 12:
            print('#%d port %s original %s' % (i, a, (b[0] - off, b[1], b[2], b[3]) if b else None))
print('differences:', bad, 'largest gap (updates):', worst)
