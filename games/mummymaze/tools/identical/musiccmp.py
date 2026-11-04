#!/usr/bin/env python3
"""musiccmp.py <port sound log> <fmodlog.txt>: the music calls of the original (games/mummymaze/tools/fmodproxy's log: FMUSIC_SetOrder, PlaySong,
StopSong, SetMasterVolume, with the game's update counter) against the port's BYOPOP_SOUNDLOG music events ("music play <slot> offset=<order>",
"music stop <slot>", "music volume <slot> <0..100>"), by update. The port's updates are moved to the original's as soundcmp.py does."""
import re, sys
def shift(u): return u + 1401 if u <= 99 else u + 2301
port = []
for l in open(sys.argv[1]):
    m = re.match(r'(\d+) music (play|fadein|stop|volume) (\d+)(?: offset=(\d+)| (\d+))?', l)
    if m:
        u, k, slot, off, vol = int(m.group(1)), m.group(2), int(m.group(3)), m.group(4), m.group(5)
        port.append((shift(u), k, slot, int(off) if off is not None else (int(vol) if vol is not None else None)))
orig = []
for l in open(sys.argv[2]):
    m = re.match(r'(\d+) \d+ _FMUSIC_(SetOrder|StopSong|SetMasterVolume|PlaySong)@\d+ ([0-9a-f]+) ([0-9a-f]+)', l)
    if m:
        orig.append((int(m.group(1)), m.group(2), int(m.group(3), 16), int(m.group(4), 16)))
songs = {}
for u, k, h, a in orig:
    songs.setdefault(h, len(songs))
for k in ('play', 'stop'):
    pass
po = [(u, s, o) for u, k, s, o in port if k in ('play', 'fadein')]
oo = [(u, k, songs[h], a) for u, k, h, a in orig if k == 'SetOrder']
ps = [(u, s) for u, k, s, o in port if k == 'stop']
os_ = [(u, songs[h]) for u, k, h, a in orig if k == 'StopSong']
print('orders: port %d original %d; stops: port %d original %d' % (len(po), len(oo), len(ps), len(os_)))
bad = 0
for i in range(max(len(po), len(oo))):
    a = po[i] if i < len(po) else None; b = oo[i] if i < len(oo) else None
    if not a or not b or abs(a[0] - b[0]) > 2 or a[2] != b[3]:
        bad += 1
        if bad < 10: print('order #%d port %s original %s' % (i, a, b))
print('order differences:', bad)
