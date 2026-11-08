# musiccmp.py <tour>: the music, by what can be heard: each song's volume over time. The original's BASS calls (the
# logging bass.dll, orig_bass.log: "<ms> <thread> BASS_ChannelSetAttributes <handle> <freq> <vol> <pan> ...", a song's
# handle 0xa0000101 + its load order) against the port's BYOPOP_SOUNDLOG ("<update> music volume <song> <vol>").
# Repeated values are dropped on both sides (the original sets the volume every update of a fade, changed or not).
# Times are in updates (10 ms) from each side's second change (the first is the title's song at start-up, set while
# the original still loads). The first change is not compared. Prints the first differences.
import re, sys
D = 'build/ph2/%s/' % sys.argv[1]
SLACK = int(sys.argv[2]) if len(sys.argv) > 2 else 1

def changes(events):
    last, out = {}, []
    for t, song, vol in events:
        if last.get(song) != vol:
            last[song] = vol
            out.append((t, song, vol))
    return out

port = []
for l in open(D + 'port_sound.log'):
    m = re.match(r'(\d+) music volume (\d+) (\d+)', l)
    if m:
        port.append((int(m.group(1)), int(m.group(2)), int(m.group(3))))
orig = []
for l in open(D + 'orig_bass.log'):
    m = re.match(r'(\d+) \d+ BASS_ChannelSetAttributes ([0-9a-f]+) [0-9a-f]+ ([0-9a-f]+) ', l)
    if m and m.group(3) != 'ffffffff':
        orig.append((int(m.group(1)) // 10, int(m.group(2), 16) - 0xa0000101, int(m.group(3), 16)))
port, orig = changes(port), changes(orig)

p0 = port[1][0] if len(port) > 1 else 0
o0 = orig[1][0] if len(orig) > 1 else 0
print('volume changes: port %d, original %d' % (len(port), len(orig)))
bad = 0
for i in range(1, max(len(port), len(orig))):
    a = port[i] if i < len(port) else None
    b = orig[i] if i < len(orig) else None
    if (a is None) or (b is None) or a[1:] != b[1:] or abs((a[0] - p0) - (b[0] - o0)) > SLACK:
        bad += 1
        if bad <= 12:
            print('#%d port %s | original %s' % (i, a and (a[0] - p0,) + a[1:], b and (b[0] - o0,) + b[1:]))
print('differences:', bad)
