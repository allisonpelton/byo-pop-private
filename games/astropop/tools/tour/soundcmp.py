# soundcmp.py <port_sound.log> <orig_sound.log> [--from N]: the sound effects of one tour, the original's against the port's.
# The original side is origrun's logs/sound.log (a play of a duplicate of a loaded buffer: time in virtual ms, update = ms / 10, volume in
# hundredths of a dB and pan as the last calls set them); the port side is BYOPOP_SOUNDLOG ("<update> load <id> <file> frames= rate= ch=",
# "<update> play <id> #n vol= pan= pitch= loop="). A sample is named by its byte size and rate (the original's buffer length, the port's
# frames * channels * 2): two samples of one size would be told apart by nothing, so they get one name and the comparison says so.
# Prints the counts of each sample, the plays that differ in time (more than 2 updates), name, volume, pan or loop, and a verdict line.
import re, sys

args = [a for a in sys.argv[1:] if not a.startswith('--')]
FROM = 0
if '--from' in sys.argv:
    FROM = int(sys.argv[sys.argv.index('--from') + 1])
    args = [a for a in args if a != str(FROM)]
port_log, orig_log = args[0], args[1]

port_loads, port_plays = {}, []
for l in open(port_log, errors='replace'):
    m = re.match(r'\d+ load (\d+) (\S+) frames=(\d+) rate=(\d+) ch=(\d+)', l)
    if m:
        port_loads[int(m.group(1))] = (m.group(2), int(m.group(3)) * int(m.group(5)) * 2, int(m.group(4)))
        continue
    m = re.match(r'(\d+) play (\d+) #\d+ vol=(-?\d+) pan=(-?\d+) pitch=([\d.]+) loop=(\d+)', l)
    if m:
        port_plays.append((int(m.group(1)), int(m.group(2)), int(m.group(3)), int(m.group(4)), int(m.group(6))))

by_size = {}
for i, (n, b, r) in port_loads.items():
    by_size.setdefault((b, r), []).append(n)
ambiguous = {k: v for k, v in by_size.items() if len(v) > 1}


def name_of(key):
    v = by_size.get(key)
    if not v:
        return '?%s' % (key,)
    return v[0] if len(v) == 1 else '|'.join(sorted(v))


bufs, dup, vol, pan, orig_plays = {}, {}, {}, {}, []
for l in open(orig_log, errors='replace'):
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) create\s+bytes=(\d+)\s+fmt=(\d+)Hz', l)
    if m:
        bufs[int(m.group(2))] = (int(m.group(3)), int(m.group(4)))
        continue
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) duplicate of (\d+)', l)
    if m:
        dup[int(m.group(2))] = int(m.group(3))
        vol[int(m.group(2))] = 0
        pan[int(m.group(2))] = 0
        continue
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) volume\s+(-?\d+)', l)
    if m:
        vol[int(m.group(2))] = int(m.group(3))
        continue
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) pan\s+(-?\d+)', l)
    if m:
        pan[int(m.group(2))] = int(m.group(3))
        continue
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) play\s+(\w+)\s+vol=(-?\d+) pan=(-?\d+) freq=(\d+)', l)
    if m and int(m.group(2)) in dup:
        src = bufs.get(dup[int(m.group(2))])
        orig_plays.append((int(float(m.group(1)) / 10), name_of(src), int(m.group(4)), int(m.group(5)), 1 if m.group(3).startswith('loop') else 0))

def port_name(i):
    n, b, r = port_loads[i]
    return name_of((b, r))


pp = sorted([(u, port_name(i), v, p, lp) for u, i, v, p, lp in port_plays if u >= FROM], key=lambda x: (x[0], x[1]))
op = sorted([x for x in orig_plays if x[0] >= FROM], key=lambda x: (x[0], x[1]))
print('port plays %d, original plays %d (updates >= %d)' % (len(pp), len(op), FROM))
cp, co = {}, {}
for x in pp:
    cp[x[1]] = cp.get(x[1], 0) + 1
for x in op:
    co[x[1]] = co.get(x[1], 0) + 1
bad_counts = [(n, cp.get(n, 0), co.get(n, 0)) for n in sorted(set(cp) | set(co)) if cp.get(n, 0) != co.get(n, 0)]
for n, a, b in bad_counts:
    print('count %s: port %d original %d' % (n, a, b))
if ambiguous:
    print('note: samples of one size and rate (told apart by nothing): %s' % list(ambiguous.values()))
# the original's update counter and the port's differ by a constant (the virtual clock starts a little before the first update): the
# commonest difference between the k-th plays of a sample is taken as it, and the plays are compared after it
diffs = {}
for n in set(cp) & set(co):
    a = [x for x in pp if x[1] == n]
    b = [x for x in op if x[1] == n]
    for k in range(min(len(a), len(b))):
        d = b[k][0] - a[k][0]
        diffs[d] = diffs.get(d, 0) + 1
OFFSET = max(diffs, key=diffs.get) if diffs else 0
print('offset (original - port, in updates): %d (%d of %d plays)' % (OFFSET, diffs.get(OFFSET, 0), sum(diffs.values())))
# per name, in order: the k-th play of each sample in the port against the k-th in the original
bad = 0
for n in sorted(set(cp) | set(co)):
    a = [x for x in pp if x[1] == n]
    b = [x for x in op if x[1] == n]
    for k in range(max(len(a), len(b))):
        x = a[k] if k < len(a) else None
        y = b[k] if k < len(b) else None
        if not x or not y or abs(y[0] - x[0] - OFFSET) > 2 or x[2] != y[2] or x[3] != y[3] or x[4] != y[4]:
            bad += 1
            if bad <= 14:
                print('%s #%d port %s original %s' % (n, k, x and (x[0],) + x[2:], y and (y[0],) + y[2:]))
print('differences: %d' % bad)
