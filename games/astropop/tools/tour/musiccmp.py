"""The music's state of one tour, the original's against the port's.

    python games/astropop/tools/tour/musiccmp.py <orig.log> <port_sound.log> [--delta N] [--tolerance N]

The original's side is origrun's log of a run with four watches on the application (musictour.sh): the song position the game
chose (AstroPopApp::mMusicOrder, +0x6dc), the slot that plays (mMusicSlot, +0x6d8) and the two ends of the paused-slots list
(+0x634, +0x638), as "watch N: a -> b at update U" lines. The port's side is BYOPOP_SOUNDLOG: each song the game starts is a
"<update> music fadein <slot> offset=<position> ..." line (PlaySong's FadeIn). What the game decides is compared: the positions it
starts, in order, with the slot and the update (the original's update = the port's + delta, 2 by default as for the sounds,
and a difference of up to --tolerance updates, 1 by default, is the same). What BASS does with the songs is not seen by origrun: the
sandbox has no sound device, so the original's BASS never plays and its list of paused slots stays empty; the port's pauses and
resumes at a focus change are listed beside, for reading.
Prints the two sequences' lengths, each difference and a verdict line; exits 1 on any difference.
"""
import re
import sys

args = [a for a in sys.argv[1:] if not a.startswith('--')]


def opt(name, default):
    if name in sys.argv:
        return int(sys.argv[sys.argv.index(name) + 1])
    return default


DELTA = opt('--delta', 2)
TOL = opt('--tolerance', 1)
orig_log, port_log = args[0], args[1]

order = []   # (update, value) of watch 1
slot = {}    # update -> value of watch 2
paused = []  # (update, value) of watches 3 and 4
for l in open(orig_log, errors='replace'):
    m = re.search(r'watch (\d): (\S+) -> (\S+) at update (\d+)', l)
    if not m:
        continue
    w, old, new, u = int(m.group(1)), m.group(2), m.group(3), int(m.group(4))
    if w == 1:
        order.append((u, int(new)))
    elif w == 2:
        slot[u] = int(new)
    else:
        paused.append((u, w, new))
orig = [(u, v, slot.get(u)) for u, v in order]

port = []
port_focus = []
for l in open(port_log, errors='replace'):
    m = re.match(r'(\d+) music fadein (\d+) offset=(\d+)', l)
    if m:
        port.append((int(m.group(1)), int(m.group(3)), int(m.group(2))))
        continue
    m = re.match(r'(\d+) music (pause|resume) (\d+)', l)
    if m:
        port_focus.append((int(m.group(1)), m.group(2), int(m.group(3))))

# the original's first reading is the state at the start (order -1 or 0): the port's first FadeIn at update 0 is PlaySong(0)
bad = 0
n = max(len(orig), len(port))
print('original: %d song starts, port: %d' % (len(orig), len(port)))
for i in range(n):
    o = orig[i] if i < len(orig) else None
    p = port[i] if i < len(port) else None
    if o is None or p is None:
        print('  #%d only in the %s: %s' % (i, 'port' if o is None else 'original', p or o))
        bad += 1
        continue
    start_up = o[0] == 0 and p[0] == 0   # PlaySong(0) of the start-up: before the first update counted
    ok = (o[1] == p[1]) and (o[2] is None or o[2] == p[2]) and (start_up or abs(o[0] - (p[0] + DELTA)) <= TOL)
    if not ok:
        print('  #%d differs: original %s, port %s (update + %d)' % (i, o, p, DELTA))
        bad += 1
if port_focus:
    print('port: %d pauses and resumes at a focus change (%s)' % (len(port_focus), ', '.join('%d %s' % (u, k) for u, k, _ in port_focus[:8])))
print('original: the paused-slots list changed %d times' % len([1 for u, w, v in paused if u > 0]))
print('musiccmp: %s (%d starts compared)' % ('DIFFERENT' if bad else 'same', min(len(orig), len(port))))
sys.exit(1 if bad else 0)
