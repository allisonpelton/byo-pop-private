# soundcmp.py <tour> [start update]: the sounds of the original (origrun's stand-in log, orig_sound.log: "buffer N create
# bytes=", "buffer N play", "buffer N stop", times in virtual ms, an update = 10 ms) against the port's -soundlog
# (port_sound.log: "<update> load <handle> bytes=", "<update> play <handle>", "play2" for a start unless it plays, and
# "stop" for every stop LOWOS_PlaySound asks for). Both create the sounds in the same order, so a sound is named by its
# place in that order (the original's first buffer is DirectSound's primary buffer), and the sizes must agree.
#
# The port's test run has no mixer that plays, so whether a sound still plays is decided here from its length, as the
# stand-in decides it for the original (44100 Hz, 16-bit stereo: 1764 bytes an update): a stop counts only while the
# sound plays, a "play2" only while it does not. Starts and stops are then compared in order, and their updates told
# as shifts. Prints the first differences and the counts.
import re, sys
D = 'build/ph2/%s/' % sys.argv[1]


def port_log(path):
    """a -soundlog's sizes and its starts and stops, the playing state decided from the sounds' lengths"""
    size, events, end, music = {}, [], {}, []
    for l in open(path):
        m = re.match(r'(\d+) (load|play2?|stop) (\d+)(?: bytes=(\d+))?', l)
        if not m:
            if ' music ' in l:
                music.append(l.strip())
            continue
        u, what, h = int(m.group(1)), m.group(2), int(m.group(3))
        if what == 'load':
            size[h] = int(m.group(4))
        elif what == 'stop':
            if end.get(h, 0) > u:
                events.append((u, 'stop', h))
            end[h] = 0
        elif what == 'play' or end.get(h, 0) <= u:
            events.append((u, 'play', h))
            end[h] = u + size[h] / 1764.0
    return size, events, music


# soundcmp.py <tour> web: the web build's log (web_sound.log, games/tiptop/tools/shots/websound.sh) against the native
# port's, which must be the same
if len(sys.argv) > 2 and sys.argv[2] == 'web':
    a, b = port_log(D + 'port_sound.log'), port_log(D + 'web_sound.log')
    print('native: %d sounds, %d starts and stops, %d music states; web: %d, %d, %d; %s' % (
        len(a[0]), len(a[1]), len(a[2]), len(b[0]), len(b[1]), len(b[2]), 'same' if a == b else 'DIFFERENT'))
    sys.exit(0)
start = int(sys.argv[2]) if len(sys.argv) > 2 else 0
port_size, port, _ = port_log(D + 'port_sound.log')
orig_size, orig_id, orig = {}, {}, []
for l in open(D + 'orig_sound.log', errors='replace'):
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) (create|play|stop)\b(?:\s+bytes=(\d+))?', l)
    if not m:
        continue
    b, what = int(m.group(2)), m.group(3)
    if what == 'create':
        orig_id[b] = len(orig_size) - 1
        orig_size[len(orig_size)] = int(m.group(4))
    else:
        orig.append((int(float(m.group(1))) // 10, what, orig_id.get(b, -1)))
for i, b in port_size.items():
    if orig_size.get(i + 1) != b:
        print('sound %d: port %d bytes, original %s' % (i, b, orig_size.get(i + 1)))
port = [p for p in port if p[0] >= start]
orig = [o for o in orig if o[0] >= start]
print('port: %d starts, %d stops; original: %d starts, %d stops' % (
    sum(p[1] == 'play' for p in port), sum(p[1] == 'stop' for p in port),
    sum(o[1] == 'play' for o in orig), sum(o[1] == 'stop' for o in orig)))
# in order: the same sound started or stopped; then the updates, as the original's update less the port's. The stand-in
# times the original by origrun's virtual clock, which jumps ahead (by 515 and 751 s in two runs) while the window is inactive,
# so after a loss of the focus (tour 3) the original's times are shifted by a constant
bad, shifts = 0, {}
for i in range(max(len(port), len(orig))):
    a = port[i] if i < len(port) else None
    b = orig[i] if i < len(orig) else None
    if not a or not b or a[1:] != b[1:]:
        bad += 1
        if bad <= 12:
            print('#%d port %s original %s' % (i, a, b))
    else:
        d = b[0] - a[0]
        shifts.setdefault(d, [0, a[0]])[0] += 1
print('differences:', bad)
print('update shifts (original - port: count, first port update):', ', '.join(
    '%+d: %d from %d' % (d, n, u) for d, (n, u) in sorted(shifts.items(), key=lambda x: x[1][1])))

# The music (sound.sh, unless MUSIC=0): its state, change by change. origrun's watches 1-3 (orig_sound_run.log) read the
# playing flag, the pause count and the music volume; the port logs "<update> music <playing> <pauses> <volume>". As in
# phase one, the original's watch at update N is the port's state after update N-1. The first two are bytes, so only
# their low byte is the value.
try:
    run = open(D + 'orig_sound_run.log', errors='replace').read()
except OSError:
    run = ''
if 'watch 1:' in run:
    masks = [0xff, 0xff, 0xffffffff]
    cur = [None, None, None]
    ochg = []
    for m in re.finditer(r'watch (\d): \S+ -> (\S+) at update (\d+)', run):
        k, v, u = int(m.group(1)) - 1, m.group(2), int(m.group(3))
        if k > 2 or v == '-':
            continue
        v = int(v) & masks[k]
        if v != cur[k]:
            cur[k] = v
            ochg.append((u - 1, k, v))
    pcur = [None, None, None]
    pchg = []
    for l in open(D + 'port_sound.log'):
        m = re.match(r'(\d+) music (\d+) (\d+) (\d+)', l)
        if m:
            vals = [int(m.group(i)) for i in (2, 3, 4)]
            for k in range(3):
                if vals[k] != pcur[k]:
                    pcur[k] = vals[k]
                    pchg.append((int(m.group(1)), k, vals[k]))
    names = ['playing', 'pauses', 'volume']
    for k in range(3):
        # each side's first value is its state at the start (the port's at update 1, the original's at its first read)
        a = [(u, v) for u, kk, v in pchg if kk == k]
        b = [(u, v) for u, kk, v in ochg if kk == k]
        # the original's changes before the port's first update (the preferences' volume) make its starting value
        b = [x for x in b if x[0] <= 1][-1:] + [x for x in b if x[0] > 1]
        a, b = a[:1] + [x for x in a[1:] if x[0] >= start], b[:1] + [x for x in b[1:] if x[0] >= start]
        if a and b:
            a[0], b[0] = (0, a[0][1]), (0, b[0][1])
        same = len(a) == len(b) and all(x[1] == y[1] and abs(x[0] - y[0]) <= 1 for x, y in zip(a, b))
        print('music %-8s port %s original %s %s' % (names[k], a[:8], b[:8], 'same' if same else 'DIFFERS'))
