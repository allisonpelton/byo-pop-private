#!/usr/bin/env python3
"""musiccmp.py <port sound log> <fmodlog.txt> [limit update]: the music calls of the original (games/atomica/tools/fmodproxy's log: FMUSIC_PlaySong,
SetOrder, StopSong, SetMasterVolume, with the game's update counter) against the port's BYOPOP_SOUNDLOG music events, by update. The port's script runs
at the start of an update, so its updates are one after the original's: the port's are moved back by one."""
import re, sys
LIM = int(sys.argv[3]) if len(sys.argv) > 3 else 10 ** 9
port = []
for l in open(sys.argv[1]):
    m = re.match(r'(\d+) music (play|stop) (\d+)(?: offset=(\d+))?', l)
    if m and int(m.group(1)) <= LIM:
        port.append((max(int(m.group(1)) - 1, 0), m.group(2), int(m.group(3)), int(m.group(4)) if m.group(4) else None))
songs, orig = {}, []
for l in open(sys.argv[2]):
    m = re.match(r'(\d+) \d+ _FMUSIC_(SetOrder|StopSong|LoadSong)@\d+ ([0-9a-f]+) ([0-9a-f]+)', l)
    if m and int(m.group(1)) <= LIM:
        k = m.group(2)
        if k == 'LoadSong':
            continue
        orig.append((int(m.group(1)), 'play' if k == 'SetOrder' else 'stop', m.group(3), int(m.group(4), 16)))
for o in orig:
    songs.setdefault(o[2], len(songs))
orig = [(u, k, songs[h], a if k == 'play' else None) for u, k, h, a in orig]
# the original's slot numbers follow its first use of each song handle; the port's are its own slots
print('port %d events, original %d' % (len(port), len(orig)))
bad = 0
for i in range(max(len(port), len(orig))):
    a = port[i] if i < len(port) else None; b = orig[i] if i < len(orig) else None
    if not a or not b or a[1] != b[1] or a[3] != b[3] or abs(a[0] - b[0]) > 1:
        bad += 1; print('#%d port %s original %s' % (i, a, b))
print('differences:', bad)
