"""Compares what the original and the port play, by order and state (phase two, "sound").

The original's side is the log of tools/bassproxy (basslog.txt, every BASS call the game makes: Venice plays everything through
BASS 2.2 samples, its music too, as looping samples that start at volume 0 and are faded by volume calls). The port's side is
the framework's sound log (BYOPOP_SOUNDLOG), written by the sound layer (VeniceSound.cpp). Each becomes a list of events, with the
sounds named by their file's checksum and length (the original loads every file when the game starts and the port at the call
of `Load`, so the order of the loads is not the same; the loads are compared as lists of sounds, not in the sequence):

    play <sound> freq=<Hz or -1> vol=<0..100>, volume <sound> <0..100> (a sound that is playing), stop <sound>, pause, start

A sample's play is the three calls the original makes in a row (SampleGetChannel, ChannelSetAttributes, ChannelPlay); a volume
is a ChannelSetAttributes with the "keep the pan" value -101 and no play after it. The original's stand-in samples (the sandbox
has no sound device, so the proxy answers for the samples) never end by themselves, so a volume call on a sample that has ended
in the port exists only in the original: those of looping sounds (the music, a loop that plays until it is stopped) are
compared, the others only counted.

    python soundcmp.py <basslog.txt> <port sound log> [--times]
"""
import difflib
import re
import sys

KEEP_PAN = 0xffffff9b


def signed(value):
    return value - (1 << 32) if value >= 1 << 31 else value


def original_events(path):
    """[(ms, event)] of the original's log, and the BASS functions that were called that this does not compare"""
    handles = {}   # sample handle -> sound (crc:length)
    channels = {}  # channel handle -> sound
    pending = {}   # channel -> (freq, vol, pan) of the last ChannelSetAttributes
    loads = []
    events = []
    others = {}
    stack = {}     # thread -> the call waiting for its result
    started = False  # the calls before BASS_Init are the window's first focus changes, before any sound exists
    for line in open(path, errors='replace'):
        if line.startswith('#'):
            continue
        parts = line.split()
        if len(parts) < 4:
            continue
        ms, thread = int(parts[0]), parts[1]
        if parts[2] == '=' and parts[3] == 'BASS_Init':
            started = True
            continue
        if not started:
            continue
        if parts[2] == '=':
            name, result = parts[3], int(parts[4], 16)
            call = stack.pop(thread, None)
            if call is None or call['name'] != name:
                continue
            args = call['args']
            if name == 'BASS_SampleLoad':
                handles[result] = call['sound'] or 'unnamed%d' % len(loads)
                loads.append({'loop': bool(args[5] & 4), 'sound': handles[result]})
            elif name == 'BASS_SampleGetChannel':
                if args[0] in handles and result:
                    channels[result] = handles[args[0]]
            continue
        name = parts[2]
        args = [int(word, 16) for word in parts[3:9] if re.fullmatch('[0-9a-f]+', word)]
        sound = re.search(r' crc ([0-9a-f]+) len (\d+)', line)
        stack[thread] = {'name': name, 'args': args, 'sound': sound.group(1) + ':' + sound.group(2) if sound else None}
        if name == 'BASS_ChannelSetAttributes':
            pending[args[0]] = (signed(args[1]), signed(args[2]), signed(args[3]), ms)
        elif name == 'BASS_ChannelPlay':
            channel = args[0]
            if channel in channels:
                setup = pending.pop(channel, None)
                if setup is None:
                    events.append((ms, ('play', channels[channel], None, None)))
                elif signed(KEEP_PAN) == setup[2]:
                    events.append((setup[3], ('volume', channels[channel], setup[1])))
                    events.append((ms, ('play', channels[channel], None, None)))
                else:
                    events.append((ms, ('play', channels[channel], setup[0], setup[1])))
        elif name == 'BASS_ChannelStop':
            if args[0] in channels:
                events.append((ms, ('stop', channels[args[0]])))
        elif name == 'BASS_Pause':
            events.append((ms, ('pause',)))
        elif name == 'BASS_Start':
            if events or loads:
                events.append((ms, ('start',)))
        elif name not in ('BASS_ChannelIsActive', 'BASS_ErrorGetCode', 'BASS_Init', 'BASS_Free', 'BASS_GetInfo', 'BASS_GetVersion',
                          'BASS_SetConfig', 'BASS_GetConfig', 'BASS_PluginLoad'):
            others[name] = others.get(name, 0) + 1
        # a volume call: a ChannelSetAttributes with the keep-pan value that no ChannelPlay follows
        if name != 'BASS_ChannelSetAttributes' and name not in ('BASS_ChannelPlay', 'BASS_ErrorGetCode'):
            for channel, setup in list(pending.items()):
                if signed(KEEP_PAN) == setup[2] and channel in channels:
                    events.append((setup[3], ('volume', channels[channel], setup[1])))
                    del pending[channel]
    for channel, setup in pending.items():
        if signed(KEEP_PAN) == setup[2] and channel in channels:
            events.append((setup[3], ('volume', channels[channel], setup[1])))
    # a volume call is known to be one only when the next call shows no ChannelPlay after it, so it was appended after that call:
    # the events go back in the order of their times (a stable sort)
    events.sort(key=lambda event: event[0])
    return events, loads, others


def port_events(path):
    """[(update, event)] of the port's log; its sounds numbered by load order (a name is made once)"""
    numbers = {}
    events = []
    loads = []
    for line in open(path, errors='replace'):
        parts = line.split()
        if len(parts) < 2:
            continue
        update, kind = int(parts[0]), parts[1]
        if kind == 'load':
            loop = int(parts[3].split('=')[1])
            numbers[parts[2]] = parts[5] + ':' + parts[7] if len(parts) >= 8 else parts[2]
            loads.append({'loop': bool(loop), 'sound': numbers[parts[2]], 'name': parts[2]})
        elif kind == 'play':
            events.append((update, ('play', numbers[parts[2]], int(parts[3].split('=')[1]), int(parts[4].split('=')[1]))))
        elif kind == 'volume':
            events.append((update, ('volume', numbers[parts[2]], int(parts[3]))))
        elif kind == 'stop':
            events.append((update, ('stop', numbers[parts[2]])))
        elif kind in ('pause', 'start'):
            events.append((update, (kind,)))
    return events, loads


def main():
    paths = [argument for argument in sys.argv[1:] if not argument.startswith('--')]
    if len(paths) != 2:
        print(__doc__)
        return 2
    original, original_loads, others = original_events(paths[0])
    port, port_loads = port_events(paths[1])

    # a sound loops as the original's load of its file says (the BASS sample's flag is fixed when the file is loaded; the port's
    # own flag follows the game's latest ask for the sound, which can come after the end of the original's run)
    original_looping = {load['sound']: load['loop'] for load in original_loads}

    def looping_of(loads):
        looping = {load['sound']: load['loop'] for load in loads}
        looping.update(original_looping)
        return looping

    def compared(events, loads):
        # the volume calls of a sound that ends by itself exist only in the original: the looping ones are compared
        looping = looping_of(loads)
        return [(when, event) for when, event in events if event[0] != 'volume' or looping.get(event[1], False)]

    def skipped(events, loads):
        looping = looping_of(loads)
        return sum(1 for _, event in events if event[0] == 'volume' and not looping.get(event[1], False))

    original_kept = compared(original, original_loads)
    port_kept = compared(port, port_loads)
    # the port's run can go on after the original's (a frame the original never ran): what it did after the time of the original's
    # last event is not compared. A time is one update of 4 ms from the first play of each; the port's updates stand still while
    # the window has no focus, so the cut is never too early.
    first_plays = [next((when for when, event in events if event[0] == 'play'), None) for events in (original_kept, port_kept)]
    if None not in first_plays and original_kept:
        cut = first_plays[1] + (original_kept[-1][0] - first_plays[0]) // 4 + 2
        port_kept = [(when, event) for when, event in port_kept if when <= cut]
    print('original: %d events (%d plays, %d loads), port: %d events (%d plays, %d loads); volume calls of sounds that end: '
          'original %d, port %d' % (len(original_kept), sum(1 for _, e in original_kept if e[0] == 'play'), len(original_loads),
                                    len(port_kept), sum(1 for _, e in port_kept if e[0] == 'play'), len(port_loads),
                                    skipped(original, original_loads), skipped(port, port_loads)))
    only_original = sorted(set(load['sound'] for load in original_loads) - set(load['sound'] for load in port_loads))
    missing = sorted(set(load['name'] for load in port_loads if load['sound'] == '00000000:0'))  # not in the game's files: the original's
    # load fails before it reaches BASS
    only_port = sorted(set(load['name'] for load in port_loads if load['sound'] not in set(l['sound'] for l in original_loads) and
                           load['sound'] != '00000000:0'))
    if missing:
        print('sounds the game asks for that its files do not hold (the original makes no BASS load for them): %s' % missing)
    print('sounds loaded only by the original (checksum:length): %s; only by the port (names): %s' % (only_original, only_port))
    if others:
        print('BASS calls this does not compare: ' + ', '.join('%s x%d' % item for item in sorted(others.items())))
    matcher = difflib.SequenceMatcher(None, [event for _, event in original_kept], [event for _, event in port_kept], autojunk=False)
    differences = 0
    tail = 0
    for tag, a0, a1, b0, b1 in matcher.get_opcodes():
        if tag == 'equal':
            continue
        if tag == 'insert' and a0 == len(original_kept):
            tail = b1 - b0  # the port's run goes on after the original's last event: not a difference
            continue
        differences += 1
        if differences <= 25:
            print('%s: original[%d:%d] %s | port[%d:%d] %s' % (
                tag, a0, a1, ['%s@%d' % (e, t) for t, e in original_kept[a0:min(a1, a0 + 3)]], b0, b1,
                ['%s@%d' % (e, t) for t, e in port_kept[b0:min(b1, b0 + 3)]]))
    if tail:
        print("the port logged %d more events after the original's last (its run went on longer)" % tail)
    print('%d differing blocks' % differences)
    if '--times' in sys.argv:
        pairs = [(original_kept[a + k][0], port_kept[b + k][0]) for tag, a0, a1, b0, b1 in matcher.get_opcodes() if tag == 'equal'
                 for a, b in [(a0, b0)] for k in range(a1 - a0) if original_kept[a + k][1][0] == 'play']
        if pairs:
            base = pairs[0]
            print('matched plays (original ms, port update) relative to the first: first 12 and last 6')
            for when_original, when_port in pairs[:12] + pairs[-6:]:
                print('  %6d ms  %6d updates' % (when_original - base[0], when_port - base[1]))
    return 0 if differences == 0 else 1


if __name__ == '__main__':
    sys.exit(main())
