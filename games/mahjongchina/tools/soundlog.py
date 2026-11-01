"""Compares what the original and the port play: the original's BASS calls (basslog.txt, from tools/bassproxy beside a
copy of the exe) and the port's sound log (MJ_SOUNDLOG). Both become the same events, in order:

    load <id>, play <id> <volume>, stop <id>, pause <id>, resume <id>,
    music load <id>, music play <id>, music slide <volume> <ms>, music stop, music pause, music resume,
    music position <order>, music loop

The ids are the port's resource ids; the original's samples and musics are matched to them by the CRC of their data.

    python soundlog.py <basslog.txt> <port sound log> [--times]
"""
import difflib
import sys


def port_events(path):
    ids, out, music_volume = {}, [], 100
    for line in open(path):
        p = line.split()
        t, ev = int(p[0]), p[1:]
        if ev[0] == 'load':
            ids[ev[2]] = int(ev[1])
            out.append((t, 'load %s' % ev[1]))
        elif ev[0] == 'music' and ev[1] == 'load':
            ids[ev[3]] = int(ev[2])
            out.append((t, 'music load %s' % ev[2]))
        elif ev[0] == 'music' and ev[1] == 'volume':
            music_volume = int(ev[2])
        elif ev[0] == 'music' and ev[1] == 'play':
            out.append((t, 'music play %s' % ev[2]))
            if ev[6] != '0':
                out.append((t, 'music position %s' % ev[6]))
            if ev[4] != '0':
                out.append((t, 'music slide %d %s' % (music_volume, ev[4])))
        elif ev[0] == 'music' and ev[1] == 'loop':
            out.append((t, 'music loop'))
        elif ev[0] == 'volume':
            continue
        else:
            out.append((t, ' '.join(ev)))
    return out, ids


def orig_events(path, ids):
    out, handle, chan, music, fresh, pending = [], {}, {}, set(), set(), {}
    calls = {}
    for line in open(path):
        if line.startswith('#'):
            continue
        p = line.split()
        t, thread = int(p[0]), p[1]
        if p[2] == '=':
            name, res = p[3], int(p[4], 16)
            args, crc = calls.pop(thread)
            a = [int(x, 16) for x in args]
            if name == 'BASS_SampleLoad' and res:
                handle[res] = ids.get(crc, 'crc' + str(crc))
                out.append((t, 'load %s' % handle[res]))
            elif name == 'BASS_MusicLoad' and res:
                handle[res] = ids.get(crc, 'crc' + str(crc))
                music.add(res)
                out.append((t, 'music load %s' % handle[res]))
            elif name == 'BASS_SampleGetChannel' and res:
                chan[res] = handle.get(a[0], hex(a[0]))
                fresh.add(res)
            elif name == 'BASS_ChannelSetAttributes':
                if a[0] not in music:
                    pending[a[0]] = a[2]
            elif name == 'BASS_ChannelPlay':
                if a[0] in music:
                    out.append((t, 'music play %s' % handle[a[0]] if a[1] else 'music resume'))
                elif a[0] in fresh:
                    fresh.discard(a[0])
                    out.append((t, 'play %s %d' % (chan.get(a[0]), pending.get(a[0], -1))))
                else:
                    out.append((t, 'resume %s' % chan.get(a[0])))
            elif name == 'BASS_ChannelPause':
                out.append((t, 'music pause' if a[0] in music else 'pause %s' % chan.get(a[0])))
            elif name == 'BASS_ChannelStop' and a[0] in music:
                out.append((t, 'music stop'))
            elif name == 'BASS_SampleStop':
                out.append((t, 'stop %s' % handle.get(a[0])))
            elif name == 'BASS_ChannelSlideAttributes' and a[0] in music:
                out.append((t, 'music slide %d %d' % (a[2], a[4])))
            elif name == 'BASS_ChannelSetPosition' and a[0] in music:
                out.append((t, 'music position %d' % (a[1] & 0xffff)))
            elif name == 'BASS_ChannelSetSync':
                out.append((t, 'music loop'))
            continue
        name, args = p[2], p[3:9]
        crc = p[p.index('crc') + 1] if 'crc' in p else None
        calls[thread] = (args, crc)
    return out


def main():
    port, ids = port_events(sys.argv[2])
    orig = orig_events(sys.argv[1], ids)
    show_times = '--times' in sys.argv
    a = [e for _, e in orig]
    b = [e for _, e in port]
    sm = difflib.SequenceMatcher(a=a, b=b, autojunk=False)
    same = 0
    for op, i1, i2, j1, j2 in sm.get_opcodes():
        if op == 'equal':
            same += i2 - i1
            if show_times:
                for k in range(i2 - i1):
                    print('  %7d %7d  %s' % (orig[i1 + k][0], port[j1 + k][0], a[i1 + k]))
            continue
        for k in range(i1, i2):
            print('- %7d          %s' % (orig[k][0], a[k]))
        for k in range(j1, j2):
            print('+         %7d  %s' % (port[k][0], b[k]))
    print('%d events the same; the original %d, the port %d' % (same, len(a), len(b)))


main()
