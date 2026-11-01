"""Compares what the original and the port play, by order and state.

The original's side is two logs from a pilot run with PILOT_SOUND=on: origrun's DirectSound stand-in (logs/sound.log:
the sounds, which the game loads into buffers and plays through duplicates) and tools/bassproxy (basslog.txt: the
music, through BASS 2.3). The port's side is the framework's sound log (BYOPOP_SOUNDLOG). Each becomes two lists of
events, the sounds and the music, named by the port's ids:

    play <id> vol=<hundredths of a dB> pan=<pan> pitch=<rate> loop=<0|1>, volume <id> <v>, pan <id> <p>,
    pitch <id> <rate>, stop <id>
    music play <id> order=<n>, music volume <id> <0..100>, music stop <id>

The original's sounds are matched to the port's by the order they are loaded (a hash of the decoded data cannot
match: the port decodes Ogg with stb_vorbis, the original with libvorbis), and each pair's length and format are
checked. Its musics are matched by load order too, and their CRCs checked. A volume, pan or pitch set before a sound
plays is part of its play line, not an event of its own; a music volume is listed only when it changes.

    python soundcmp.py <sound.log> <basslog.txt> <port sound log> [--times]
"""
import difflib
import sys


def orig_sounds(path):
    """The original's sound events, with the sounds as load numbers 0, 1, ..."""
    bufs = {}       # buffer id -> dict(bytes, fmt, hash, load, src, playing, vol, pan, freq)
    loads = []      # (bytes, rate, bits, ch) in load order
    out = []
    for line in open(path, errors='replace'):
        p = line.split()
        if len(p) < 4 or p[1] != 'buffer':
            continue
        t, n, ev = float(p[0]), int(p[2]), p[3]
        b = bufs.setdefault(n, {'playing': False, 'vol': 0, 'pan': 0, 'freq': 0})
        if ev == 'create':
            b['bytes'] = int(p[4].split('=')[1])
            rate, bits, ch = p[5].split('=')[1].split('/')
            b['fmt'] = (int(rate[:-2]), int(bits[:-3]), int(ch[:-2]))
            b['freq'] = b['fmt'][0]
        elif ev == 'data':
            off, size = int(p[4].split('=')[1]), int(p[5].split('=')[1])
            # a sound is loaded by its first whole write (the game locks each buffer a second time; the music goes
            # through BASS, so there are no streams here)
            if off == 0 and b.get('bytes') and size >= b['bytes'] and 'load' not in b:
                b['load'] = len(loads)
                loads.append((b['bytes'],) + b['fmt'])
        elif ev == 'duplicate':
            s = bufs.get(int(p[5]), {})
            b['src'] = s.get('load')
            b['rate'] = s.get('fmt', (0,))[0]
            b['freq'] = s.get('freq', 0)
            b['vol'], b['pan'] = s.get('vol', 0), s.get('pan', 0)   # a duplicate starts with its source's
        elif 'src' not in b or b['src'] is None:
            continue
        elif ev in ('volume', 'pan', 'freq'):
            v = int(p[4].rstrip('Hz'))
            key = {'volume': 'vol', 'pan': 'pan', 'freq': 'freq'}[ev]
            b[key] = v
            if b['playing']:
                out.append((t, ev, b['src'], v if ev != 'freq' else round(v / b['rate'], 4) if b['rate'] else v))
        elif ev == 'play':
            b['playing'] = True
            pitch = round(b['freq'] / b['rate'], 4) if b['rate'] and b['freq'] else 1.0
            out.append((t, 'play', b['src'], 'vol=%d pan=%d pitch=%.4f loop=%d' % (b['vol'], b['pan'], pitch,
                                                                               p[4] == 'looping')))
        elif ev == 'stop' and b['playing']:
            b['playing'] = False
            out.append((t, 'stop', b['src'], None))
    return out, loads


def port_sounds(path):
    loads, out, inst = [], [], {}
    for line in open(path, errors='replace'):
        p = line.split()
        if len(p) < 2:
            continue
        t, ev = int(p[0]), p[1]
        if ev == 'load':
            kv = dict(x.split('=') for x in p[4:])
            loads.append((int(p[2]), p[3], int(kv['frames']), int(kv['rate']), int(kv['ch'])))
        elif ev == 'instance':
            inst[p[3]] = {'playing': False}
        elif ev in ('play', 'volume', 'pan', 'pitch', 'stop') and len(p) > 3 and p[3] in inst:
            i, sid = inst[p[3]], int(p[2])
            if ev == 'play':
                i['playing'] = True
                out.append((t, 'play', sid, ' '.join(p[4:])))
            elif ev == 'stop':
                i['playing'] = False
                out.append((t, 'stop', sid, None))
            elif i['playing']:
                out.append((t, ev, sid, int(p[4]) if ev != 'pitch' else float(p[4])))
    return out, loads


def orig_music(path):
    handles, loads, out, vols = {}, [], [], {}
    pending = None
    lines = [l.split() for l in open(path, errors='replace') if l[:1].isdigit()]
    for k, p in enumerate(lines):
        t, fn = int(p[0]), p[2]
        if fn == '=':
            if p[3] == 'BASS_MusicLoad' and pending is not None:
                handles[p[4]] = pending
                pending = None
            continue
        if fn == 'BASS_MusicLoad':
            pending = len(loads)
            loads.append(p[p.index('crc') + 1] if 'crc' in p else '?')
            continue
        h = p[3] if len(p) > 3 else None
        if h not in handles:
            continue
        song = handles[h]
        if fn == 'BASS_ChannelPlay':
            pos = next((int(q[4], 16) for q in reversed(lines[max(0, k - 4):k])
                        if q[2] == 'BASS_ChannelSetPosition' and q[3] == h), 0)
            out.append((t, 'music play', song, 'order=%d' % (pos & 0xffff)))
        elif fn == 'BASS_ChannelStop':
            # a play starts with stops; a stop on its own (a fade's end, a StopMusic) is an event. The setup calls
            # before a play can take 20 ms or more in a slow sandbox run, so a stop joins a play up to 50 ms after it
            nxt = [q[2] for q in lines[k + 1:k + 40] if len(q) > 3 and q[3] == h and q[2] != '=' and int(q[0]) - t <= 50]
            setup = ('BASS_ChannelStop', 'BASS_ChannelSetPosition', 'BASS_ChannelSetFlags',
                     'BASS_ChannelSetAttributes', 'BASS_ChannelIsActive')
            first = next((f for f in nxt if f not in setup), None)
            if first != 'BASS_ChannelPlay':
                out.append((t, 'music stop', song, None))
        elif fn == 'BASS_ChannelSetAttributes':
            v = int(p[5], 16)
            if v != 0xffffffff and vols.get(song) != v:
                vols[song] = v
                out.append((t, 'music volume', song, v))
    return out, loads


def port_music(path):
    loads, out, vols = [], [], {}
    for line in open(path, errors='replace'):
        p = line.split()
        if len(p) < 3 or p[1] != 'music':
            continue
        t, ev = int(p[0]), p[2]
        if ev == 'load':
            # the proxy's CRC always covers 32 KB, so a shorter song's cannot agree: compared as 'short'
            short = len(p) > 6 and p[6].startswith('len=') and int(p[6][4:]) < 0x8000
            loads.append((int(p[3]), 'short' if short else p[5].split('=')[1]))
        elif ev in ('play', 'fadein'):
            off = int(p[4].split('=')[1])
            out.append((t, 'music play', int(p[3]), 'order=%d' % (max(off, 0) & 0xffff)))
        elif ev == 'stop':
            out.append((t, 'music stop', int(p[3]), None))
        elif ev == 'volume':
            sid, v = int(p[3]), int(p[4])
            if vols.get(sid) != v:
                vols[sid] = v
                out.append((t, 'music volume', sid, v))
    return out, loads


def text(ev, name):
    t, kind, s, v = ev
    return ('%s %s' % (kind, name(s)) + ('' if v is None else ' %s' % v))


def compare(title, a, b, aname, bname, times):
    at = [text(e, aname) for e in a]
    bt = [text(e, bname) for e in b]
    print('%s: original %d events, port %d' % (title, len(at), len(bt)))
    same = True
    for op, i1, i2, j1, j2 in difflib.SequenceMatcher(None, at, bt, autojunk=False).get_opcodes():
        if op == 'equal':
            continue
        same = False
        for i in range(i1, i2):
            print('  - %s%s' % (at[i], ' @%.0fms' % a[i][0] if times else ''))
        for j in range(j1, j2):
            print('  + %s%s' % (bt[j], ' @u%d' % b[j][0] if times else ''))
        print('  --')
    if same:
        print('  identical')
    return same


def main():
    args = [a for a in sys.argv[1:] if not a.startswith('--')]
    times = '--times' in sys.argv
    if len(args) != 3:
        sys.exit(__doc__)
    osnd, oloads = orig_sounds(args[0])
    omus, omloads = orig_music(args[1])
    psnd, ploads = port_sounds(args[2])
    pmus, pmloads = port_music(args[2])

    # pair the sounds by load order, and check each pair
    oloads = [l for l in oloads if l is not None]
    ids, bad = {}, 0
    for k, (o, p) in enumerate(zip(oloads, ploads)):
        ids[k] = p[0]
        size, rate, bits, ch = o
        frames = size // (bits // 8 * ch) if bits and ch else -1
        if (frames, rate, ch) != (p[2], p[3], p[4]):
            bad += 1
            print('load %d: original %d frames %d Hz %d ch, port %s %d frames %d Hz %d ch' %
                  (k, frames, rate, ch, p[1], p[2], p[3], p[4]))
    print('sounds loaded: original %d, port %d, %d pairs differ' % (len(oloads), len(ploads), bad))
    musics = {}
    for k, (o, p) in enumerate(zip(omloads, pmloads)):
        musics[k] = p[0]
        if o != p[1] and p[1] != 'short':
            print('music %d: original crc %s, port id %d crc %s' % (k, o, p[0], p[1]))
    names = {p[0]: p[1].rsplit('/', 1)[-1].rsplit('\\', 1)[-1] for p in ploads}

    same = compare('sounds', osnd, psnd, lambda s: names.get(ids.get(s, -1), 'L%s' % s),
                   lambda s: names.get(s, s), times)
    same &= compare('music', omus, pmus, lambda s: musics.get(s, 'M%s' % s), lambda s: s, times)
    sys.exit(0 if same else 1)


if __name__ == '__main__':
    main()
