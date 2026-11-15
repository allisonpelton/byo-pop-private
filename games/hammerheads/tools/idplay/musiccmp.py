"""Compares the music of the original and the port by state: what BASS is asked for, in order, with the update count of each call.

The original's side is basslog.txt of games/hammerheads/tools/bassproxy (a stand-in bass.dll beside the original's copy; its streams
are the proxy's own, since BASS's no-sound device cannot make one). The port's side is the music lines of the framework's sound log
(BYOPOP_SOUNDLOG, the update count first). Both are reduced to the same events:

    load <file>                    a music file made into a stream (the original's BASS_StreamCreateFile, the port's LoadMusic)
    volume <file> <0-100>          the song's volume set (BASS_ChannelSetAttributes with the volume, the port's ApplyVolume)
    play <file> <flush>            a song started (BASS_StreamPlay, the port's PlayMusic or FadeIn; flush is "offset != -1")
    stop <file>                    a song stopped (BASS_ChannelStop)
    pause <file> / resume <file>   (BASS_ChannelPause / BASS_ChannelResume)
    master <0-100>                 the master music volume (BASS_SetConfig 6, or BASS_SetGlobalVolumes)

The comparison is by order, value by value, and then by the update each event came at: the events must agree in what they are; the
update each came at is reported (the difference as a count of events per offset), and an offset other than a constant is an error.

    python musiccmp.py <original basslog.txt> <port sound log> [--first N] [--last N]   events between those updates only
"""
import os
import re
import sys


def signed(value):
    return value - (1 << 32) if value >= (1 << 31) else value


def original_events(path):
    events = []
    files = {}         # the stream handle -> the file's base name
    pending_file = None
    for line in open(path, errors='replace'):
        if line.startswith('#'):
            continue
        parts = line.split()
        if len(parts) < 4:
            continue
        update = int(parts[0])
        if parts[2] == '=':
            name = parts[3]
            if name == 'BASS_StreamCreateFile' and pending_file is not None:
                files[int(parts[4], 16)] = pending_file
                events.append((update, 'load', pending_file, 0))
                pending_file = None
            continue
        name = parts[2]
        args = [int(word, 16) for word in parts[3:9]]
        match = re.search(r' file (.+)$', line.rstrip('\r\n'))
        if name == 'BASS_StreamCreateFile':
            pending_file = os.path.basename((match.group(1) if match else '?').replace('\\', '/'))
        elif name == 'BASS_ChannelSetAttributes':
            if signed(args[2]) >= 0 and args[0] in files:
                events.append((update, 'volume', files[args[0]], signed(args[2])))
        elif name == 'BASS_StreamPlay' and args[0] in files:
            events.append((update, 'play', files[args[0]], args[1] & 1))
        elif name == 'BASS_ChannelStop' and args[0] in files:
            events.append((update, 'stop', files[args[0]], 0))
        elif name == 'BASS_ChannelPause' and args[0] in files:
            events.append((update, 'pause', files[args[0]], 0))
        elif name == 'BASS_ChannelResume' and args[0] in files:
            events.append((update, 'resume', files[args[0]], 0))
        elif name == 'BASS_SetConfig' and args[0] == 6:
            events.append((update, 'master', '', args[1]))
        elif name == 'BASS_SetGlobalVolumes':
            events.append((update, 'master', '', signed(args[0])))
    return events


def port_events(path):
    events = []
    files = {}         # the port's song id -> the file's base name
    for line in open(path, errors='replace'):
        match = re.match(r'(\d+) music (\w+) ?(.*)$', line.rstrip('\r\n'))
        if not match:
            continue
        update = int(match.group(1))
        what = match.group(2)
        rest = match.group(3)
        if what == 'load':
            fields = re.match(r'(\d+) (.*) crc=', rest)
            files[int(fields.group(1))] = os.path.basename(fields.group(2).replace('\\', '/'))
            events.append((update, 'load', files[int(fields.group(1))], 0))
        elif what == 'volume':
            song, volume = (int(word) for word in rest.split())
            events.append((update, 'volume', files.get(song, '?'), volume))
        elif what in ('play', 'fadein'):
            fields = re.match(r'(\d+) offset=(-?\d+)', rest)
            events.append((update, 'play', files.get(int(fields.group(1)), '?'), 0 if int(fields.group(2)) == -1 else 1))
        elif what in ('stop', 'pause', 'resume'):
            events.append((update, what, files.get(int(rest.split()[0]), '?'), 0))
        elif what == 'master':
            events.append((update, 'master', '', int(round(float(rest) * 100))))
    return events


def normalize(events):
    """What is heard, not how it is asked for: the stops of a song that does not play are dropped (the original stops every
    other song whenever one is faded in; the port's fade-outs of silent songs end in a stop too), a stop that the same update's
    play of the same song follows (the song's volume may be set between them) goes on where the play puts it anyway, and of several master volumes set at one update only
    the last stays (the port's mixer starts at full volume before the game sets its own)."""
    playing = set()
    result = []
    for at, event in enumerate(events):
        update, kind, song, value = event
        if kind == 'master':
            if at + 1 < len(events) and events[at + 1][1] == 'master' and events[at + 1][0] == update:
                continue
        elif kind == 'stop':
            if song not in playing:
                continue
            # the original sets the song's volume between the stop and the play
            later = at + 1
            while later < len(events) and events[later][0] == update and events[later][1] == 'volume' and events[later][2] == song:
                later += 1
            following = events[later] if later < len(events) else None
            if following is not None and following[0] == update and following[1] == 'play' and following[2] == song:
                continue
            playing.discard(song)
        elif kind == 'play':
            playing.add(song)
        result.append(event)
    return result


def window(events, first, last):
    return [event for event in events if first <= event[0] <= last]


def main():
    args = [word for word in sys.argv[1:] if not word.startswith('--')]
    options = {}
    for at, word in enumerate(sys.argv):
        if word in ('--first', '--last') and at + 1 < len(sys.argv):
            options[word] = int(sys.argv[at + 1])
    args = [word for word in args if not word.isdigit()]
    original = normalize(original_events(args[0]))
    port = normalize(port_events(args[1]))
    last = options.get('--last', max([event[0] for event in original] + [0]))
    first = options.get('--first', 0)
    original = window(original, first, last)
    port = window(port, first, last)
    # when the songs are made into streams is a matter of the loader (the original's loads wait for the loading thread), not of
    # the music: the loads are compared as lists of files, the rest by order and by update
    loads = ([event[2] for event in original if event[1] == 'load'], [event[2] for event in port if event[1] == 'load'])
    original = [event for event in original if event[1] != 'load']
    port = [event for event in port if event[1] != 'load']
    print('original: %d events, port: %d events (updates %d to %d)' % (len(original), len(port), first, last))
    kinds = {}
    for _, kind, _, _ in original:
        kinds[kind] = kinds.get(kind, 0) + 1
    print('original kinds:', ', '.join('%s %d' % kv for kv in sorted(kinds.items())))
    kinds = {}
    for _, kind, _, _ in port:
        kinds[kind] = kinds.get(kind, 0) + 1
    print('port kinds:    ', ', '.join('%s %d' % kv for kv in sorted(kinds.items())))
    offsets = {}
    late_inputs = 0
    mismatches = []
    for at in range(min(len(original), len(port))):
        o = original[at]
        p = port[at]
        if o[1:] != p[1:]:
            mismatches.append((at, o, p))
        else:
            # the master volume is set by the focus changes, which the harness posts to the port one update after the original
            # (as every input); the game's own events must come at the same update
            if p[1] == 'master' and p[0] - o[0] == 1:
                late_inputs += 1
            else:
                offsets[p[0] - o[0]] = offsets.get(p[0] - o[0], 0) + 1
    print('update offset (port minus original) of the %d matching events:' % sum(offsets.values()),
          ', '.join('%+d: %d' % kv for kv in sorted(offsets.items())))
    if late_inputs:
        print('and %d master volumes set by a focus change, one update after the original (the harness posts the port its input then)' % late_inputs)
    if len(original) != len(port):
        print('DIFFERENT: the counts of events differ (%d and %d)' % (len(original), len(port)))
    for at, o, p in mismatches[:8]:
        print('DIFFERENT at event %d: original %s, port %s' % (at, o, p))
    if len(mismatches) > 8:
        print('... and %d more' % (len(mismatches) - 8))
    if loads[0] != loads[1]:
        print('DIFFERENT: the files made into streams: original %s, port %s' % loads)
    ok = not mismatches and len(original) == len(port) and len(offsets) <= 1 and loads[0] == loads[1]
    print('MUSIC IDENTICAL' if ok else 'MUSIC DIFFERS')
    return 0 if ok else 1


if __name__ == '__main__':
    sys.exit(main())
