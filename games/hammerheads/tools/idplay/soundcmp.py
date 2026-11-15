"""Compares what the original and the port play, by order and state (phase two, "sound").

The original's side is the log of origrun's DirectSound stand-in (logs/sound.log: every buffer made, filled, duplicated, played and
stopped, with the time in milliseconds of the virtual clock); the game plays its sounds through DirectSound, the music through BASS
(not in this file's part: see the music comparison). The port's side is the framework's sound log (BYOPOP_SOUNDLOG: the update count
and a line for each sample that is played or stopped). Each side becomes a list of events:

    play <sound> vol=<hundredths of a dB> pan=<-10000..10000> pitch=<ratio to the sound's own rate> loop=<0|1>
    stop <sound>

A sound is the number of its file in the order of resources.xml: the original loads them all at the start, one DirectSound buffer
each from buffer 3 on (buffer 2 is the primary), so the buffer a play comes from (the buffer it duplicates) minus 3 is the number
the port logs in its `load` lines. A buffer is stopped by the game before it is played, to start it from the beginning: the stop
of a buffer that is not playing is no event. The original's stand-in answers `GetStatus` so that a one-shot buffer "plays" for its
length, and the port's instance ends the same way, so the stops of a sound that ended by itself are not compared (the original's
stop is the game's own, for a buffer it still thinks is playing and that has ended in the port): the plays are.

    python soundcmp.py <original sound.log> <port sound log> [--times] [--end <update>] [--port-only <first>-<last>:<text> ...]

The times: with --times the plays both sides have are paired, the clocks' zero is the median offset of the pairs, and each pair's
distance in updates (the original's milliseconds over ten) is given for those more than one update apart.
"""
import difflib
import re
import sys


def original_events(path):
    sources = {}        # buffer -> the loaded buffer it duplicates (itself for a loaded one)
    base_rate = {}      # loaded buffer -> its own rate in Hz
    playing = {}        # buffer -> sound, while the game has it playing
    events = []
    for line in open(path, errors='replace'):
        match = re.match(r'\s*([\d.]+)\s+buffer (\d+) (\w+)(.*)$', line)
        if not match:
            continue
        ms, buffer, what, rest = float(match.group(1)), int(match.group(2)), match.group(3), match.group(4)
        if what == 'create':
            fmt = re.search(r'fmt=(\d+)Hz', rest)
            sources[buffer] = buffer
            base_rate[buffer] = int(fmt.group(1)) if fmt else 0
        elif what == 'duplicate':
            source = int(re.search(r'of (\d+)', rest).group(1))
            sources[buffer] = sources.get(source, source)
        elif what == 'play':
            fields = re.search(r'(once|loop\w*)\s+vol=(-?\d+)\s+pan=(-?\d+)\s+freq=(\d+)Hz', rest)
            if not fields:
                continue
            source = sources.get(buffer, buffer)
            rate = base_rate.get(source) or 1
            sound = source - 3
            playing[buffer] = sound
            events.append((ms, ('play', sound, int(fields.group(2)), int(fields.group(3)),
                                round(int(fields.group(4)) / rate, 3), 0 if fields.group(1) == 'once' else 1)))
        elif what == 'stop':
            if buffer in playing:
                events.append((ms, ('stop', playing.pop(buffer))))
    return events


def port_events(path):
    events = []
    for line in open(path, errors='replace'):
        match = re.match(r'(\d+) play (\d+) #\d+ vol=(-?\d+) pan=(-?\d+) pitch=([\d.]+) loop=(\d+)', line)
        if match:
            update = int(match.group(1))
            events.append((update * 10.0, ('play', int(match.group(2)), int(match.group(3)), int(match.group(4)),
                                           round(float(match.group(5)), 3), int(match.group(6)))))
            continue
        match = re.match(r'(\d+) stop (\d+) #\d+', line)
        if match:
            events.append((int(match.group(1)) * 10.0, ('stop', int(match.group(2)))))
    return events


def port_names(path):
    """the port's `load <index> <file>` lines: the name of each sound"""
    names = {}
    for line in open(path, errors='replace'):
        match = re.match(r'\d+ load (\d+) (\S.*?) frames=', line)
        if match:
            names[int(match.group(1))] = match.group(2)
    return names


def option_values(name):
    """the values of an option that may be given more than once: `--name value`"""
    values = []
    for at, word in enumerate(sys.argv):
        if word == name and at + 1 < len(sys.argv):
            values.append(sys.argv[at + 1])
    return values


def main():
    options = {'--end', '--port-only'}
    arguments = [a for at, a in enumerate(sys.argv[1:], 1) if not a.startswith('--') and sys.argv[at - 1] not in options]
    times = '--times' in sys.argv
    original = original_events(arguments[0])
    port = port_events(arguments[1])
    # the port's side ends with the run (--end N, in updates), and leaves out the plays an input of its own makes that the original,
    # given the same thing another way, does not (--port-only first-last:text: a sound whose file's name has the text, in those updates)
    if option_values('--end'):
        end = int(option_values('--end')[0])
        port = [(t, e) for t, e in port if t / 10.0 <= end]
        # and the original's ends where its script quits: its shutdown runs the virtual clock ahead (marathon1's by 20 seconds), and
        # what it plays then (a voice, here, in the port's shutdown too) is not the run's. The run's own events are never later than
        # the update's 10 ms steps from 3000 ms, with the held input's tenth of a millisecond and a little slack.
        original = [(t, e) for t, e in original if t <= 3000.0 + end * 10.0 + 100.0]
    names = port_names(arguments[1])
    for rule in option_values('--port-only'):
        span, text = rule.split(':', 1)
        first, last = (int(v) for v in span.split('-'))
        port = [(t, e) for t, e in port
                if not (e[0] == 'play' and first <= t / 10.0 <= last and text in names.get(e[1], ''))]
    original_plays = [(t, e) for t, e in original if e[0] == 'play']
    port_plays = [(t, e) for t, e in port if e[0] == 'play']
    print('original: %d plays, %d stops; port: %d plays, %d stops' % (
        len(original_plays), len(original) - len(original_plays), len(port_plays), len(port) - len(port_plays)))
    matcher = difflib.SequenceMatcher(None, [e for _, e in original_plays], [e for _, e in port_plays], autojunk=False)
    different = 0
    for tag, i1, i2, j1, j2 in matcher.get_opcodes():
        if tag == 'equal':
            continue
        different += 1
        print('%s: original plays %d-%d, port plays %d-%d' % (tag, i1, i2, j1, j2))
        for t, e in original_plays[i1:i2][:6]:
            print('   original %8.1f ms  %s' % (t, e))
        for t, e in port_plays[j1:j2][:6]:
            print('   port     update %5d  %s' % (round(t / 10), e))
    if times:
        # the plays both sides have, paired by the matcher; the clocks' zero differs (the original's virtual clock begins at about
        # 3000 ms), so the distances are taken from the median offset of the pairs
        pairs = [(original_plays[i + k][0], port_plays[j + k][0], original_plays[i + k][1])
                 for i, j, size in matcher.get_matching_blocks() for k in range(size)]
        if pairs:
            offsets = sorted(to - tp for to, tp, _ in pairs)
            median = offsets[len(offsets) // 2]
            worst = 0
            for to, tp, event in pairs:
                delta = round((to - tp - median) / 10.0)
                if abs(delta) > 1:
                    print('   time: %s at update %.0f in the original, %.0f in the port' % (event, (to - median) / 10.0, tp / 10.0))
                worst = max(worst, abs(delta))
            print('the largest distance between matched plays: %d updates (the original clock is %.0f ms ahead)' % (worst, median))
    print('SOUND IDENTICAL' if different == 0 else 'SOUND DIFFERS in %d places' % different)
    return 1 if different else 0


if __name__ == '__main__':
    sys.exit(main())
