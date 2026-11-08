# soundcmp.py <tour>: a tour's sound in the original against the port (games/dynomite/tools/shots/sound.sh makes the logs).
#
# Samples: the original's plays are origrun's --sound wav log (a play of a duplicate of a loaded buffer, its time in virtual
# ms), the port's are BYOPOP_SOUNDLOG's "<update> play <id> #n" lines. A buffer is named by the port's load line whose rate
# and channels are the same and whose length in frames is nearest (the original decodes some files a little shorter). The
# two time lines are put together at the first play (the title's click in both), the original's ms / 10 being its update.
#
# Music: the original's BASS calls (Big Money's logging BASS, orig_bass.log; a music's handle is named by its load's order,
# the port's song ids 0, 1, 2 and 4) against the port's "music ..." lines: a play (MusicPlayEx, or the port's play and
# fadein) with its order, a stop, a pause and a resume, and every volume change. A fade is a run of volume changes in both,
# so the volumes are compared as the list of values each song passes through (repeats dropped), not by update.
import re
import sys

D = 'build/ph2/%s/' % sys.argv[1]

# --- samples
loads, port_plays = {}, []
music_port = []
for l in open(D + 'port_sound.log'):
    f = l.split()
    m = re.match(r'(\d+) load (\d+) (\S+) frames=(\d+) rate=(\d+) ch=(\d+)', l)
    if m:
        loads[int(m.group(2))] = (m.group(3), int(m.group(4)), int(m.group(5)), int(m.group(6)))
        continue
    m = re.match(r'(\d+) play (\d+) #\d+', l)
    if m:
        port_plays.append((int(m.group(1)), loads[int(m.group(2))][0]))
        continue
    if len(f) >= 4 and f[1] == 'music' and f[2] in ('play', 'fadein', 'stop', 'pause', 'resume', 'volume'):
        u, kind, song = int(f[0]), f[2], int(f[3])
        if kind == 'fadein':
            kind = 'play'
        if kind == 'play':
            music_port.append((u, 'play', song, int(f[4].split('=')[1])))
        elif kind == 'volume':
            music_port.append((u, 'volume', song, int(f[4])))
        else:
            music_port.append((u, kind, song, None))


def name_of(frames, rate, ch):
    best = None
    for n, fr, r, c in loads.values():
        if r == rate and c == ch and (best is None or abs(fr - frames) < abs(best[1] - frames)):
            best = (n, fr)
    if best is None or abs(best[1] - frames) > max(200, frames // 50):
        return '?%d/%d/%d' % (frames, rate, ch)
    return best[0]


bufs, dup, orig_plays = {}, {}, []
for l in open(D + 'orig_sound.log'):
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) create\s+bytes=(\d+)\s+fmt=(\d+)Hz/(\d+)bit/(\d+)ch', l)
    if m and int(m.group(5)):
        b, rate, bits, ch = int(m.group(3)), int(m.group(4)), int(m.group(5)), int(m.group(6))
        bufs[int(m.group(2))] = name_of(b // (bits // 8 * ch), rate, ch)
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) duplicate of (\d+)', l)
    if m:
        dup[int(m.group(2))] = int(m.group(3))
    m = re.match(r'\s*([\d.]+)\s+buffer (\d+) play\s+(\w+)', l)
    if m and int(m.group(2)) in dup:
        orig_plays.append((float(m.group(1)) / 10, bufs.get(dup[int(m.group(2))], '?')))

shift = (port_plays[0][0] - orig_plays[0][0]) if port_plays and orig_plays else 0
op = [(int(round(t + shift)), n) for t, n in orig_plays]
print('samples: port %d plays, original %d (the original\'s times moved by %+d updates)' % (len(port_plays), len(op), shift))
bad = 0
for i in range(max(len(port_plays), len(op))):
    a = port_plays[i] if i < len(port_plays) else None
    b = op[i] if i < len(op) else None
    if not a or not b or a[1] != b[1] or abs(a[0] - b[0]) > 2:
        bad += 1
        if bad <= 15:
            print('  #%d port %s original %s' % (i, a, b))
print('  sample differences: %d' % bad)

# --- music
try:
    lines = open(D + 'orig_bass.log').read().splitlines()
except OSError:
    lines = []
handles, music_orig = {}, []
SONGS = [0, 1, 2, 4]
pending = None
for l in lines:
    f = l.split()
    if len(f) < 3 or l.startswith('#'):
        continue
    if f[2] == '=' and pending and f[3] == 'BASS_MusicLoad':
        handles[f[4]] = SONGS[len(handles)] if len(handles) < len(SONGS) else -1
        pending = None
        continue
    if f[2] == 'BASS_MusicLoad':
        pending = f
        continue
    if f[2] == 'BASS_MusicPlayEx' and f[3] in handles:
        music_orig.append((int(f[0]), 'play', handles[f[3]], int(f[4], 16)))
    elif f[2] == 'BASS_ChannelSetAttributes' and f[3] in handles and f[5] != 'ffffffff':
        music_orig.append((int(f[0]), 'volume', handles[f[3]], int(f[5], 16)))
    elif f[2] in ('BASS_ChannelStop', 'BASS_ChannelPause', 'BASS_ChannelResume') and f[3] in handles:
        music_orig.append((int(f[0]), f[2][12:].lower(), handles[f[3]], None))


def summary(ev):
    # the order of plays, stops, pauses and resumes; each song's volumes as the values it passes through
    order, vols = [], {}
    for _, kind, song, arg in ev:
        if kind == 'volume':
            v = vols.setdefault(song, [])
            if not v or v[-1] != arg:
                v.append(arg)
        else:
            order.append((kind, song, arg))
    return order, vols


po, pv = summary(music_port)
oo, ov = summary(music_orig)
print('music: port %d events, original %d' % (len(po), len(oo)))
mbad = 0
for i in range(max(len(po), len(oo))):
    a = po[i] if i < len(po) else None
    b = oo[i] if i < len(oo) else None
    if a != b:
        mbad += 1
        if mbad <= 15:
            print('  #%d port %s original %s' % (i, a, b))
for s in sorted(set(pv) | set(ov)):
    if pv.get(s) != ov.get(s):
        mbad += 1
        print('  song %d volumes: port %s original %s' % (s, (pv.get(s) or [])[:20], (ov.get(s) or [])[:20]))
print('  music differences: %d' % mbad)
