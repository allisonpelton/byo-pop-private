"""Compares what the original Cosmic Bugs and the port play, by state and update, from one run each of the same script:
the original's BASS calls (basslog.txt of games/cosmicbugs/tools/bassproxy, with the game's update counter) and the
runner's sound calls (CB_SOUNDLOG=1). Both become a list of events at their updates:

    load <file>, play <file>, stopall, musicload <file>, musicfree, musicplay, musicresume, musicpause,
    musicvolume <0..100>

with the sounds by file name. On the original's side BASS_SampleLoad is a load, BASS_SampleGetChannel then
BASS_ChannelPlay of that channel a play, BASS_SampleStop of every sample a stopall, BASS_ChannelPlay of the music with
restart a musicplay (without, a musicresume), BASS_ChannelPause of it a musicpause, and BASS_SetConfig's music volume
(BASS_CONFIG_GVOL_MUSIC, 6) a musicvolume. The engine's calls that reach no BASS call are left out of the port's list
in the same way.

    python games/cosmicbugs/tools/play/soundcmp.py <basslog.txt> <runner output> [--all]
"""
import re
import sys


def orig_events(path):
    events = []
    samples = {}      # handle -> file
    channels = {}     # channel -> file
    music = None
    stops = []
    pending = {}      # thread -> (name, args, update)
    for line in open(path, errors="replace"):
        if line.startswith("#"):
            continue
        f = line.split()
        if len(f) < 3:
            continue
        upd = int(f[0])
        if f[2] == "=":
            name, result = f[3], int(f[4], 16)
            call = pending.pop(f[1], None)
            if call is None:
                continue
            _, a, cupd, fname = call
            if name == "BASS_SampleLoad":
                samples[result] = fname
                events.append((cupd, "load " + fname))
            elif name == "BASS_MusicLoad":
                music = result
                events.append((cupd, "musicload " + fname))
            elif name == "BASS_SampleGetChannel":
                channels[result] = samples.get(a[0], "?%x" % a[0])
            continue
        name = f[2]
        a = [int(x, 16) for x in f[3:9]]
        m = re.search(r" file (.*)$", line.rstrip("\n"))
        fname = m.group(1).replace("\\", "/").lower() if m else ""
        pending[f[1]] = (name, a, upd, fname)
        if name == "BASS_ChannelPlay":
            if a[0] == music:
                events.append((upd, "musicplay" if a[1] else "musicresume"))
            else:
                events.append((upd, "play " + channels.get(a[0], "?%x" % a[0])))
        elif name == "BASS_ChannelPause" and a[0] == music:
            events.append((upd, "musicpause"))
        elif name == "BASS_MusicFree":
            events.append((upd, "musicfree"))
            music = None
        elif name == "BASS_SampleStop":
            stops.append(upd)
            if len(stops) == len(samples):   # one stop of every sample: the engine's StopAll
                events.append((upd, "stopall"))
                stops = []
        elif name == "BASS_SetConfig" and a[0] == 6:
            events.append((upd, "musicvolume %d" % a[1]))
    return events


def port_events(path):
    events = []
    sounds = []
    for line in open(path, errors="replace"):
        m = re.match(r"sound (\d+): (\w+) ?(.*)$", line.rstrip("\n"))
        if not m:
            continue
        upd, what, arg = int(m.group(1)), m.group(2), m.group(3).strip()
        if what == "load":
            sounds.append(arg.replace("\\", "/").lower())
            events.append((upd, "load " + sounds[-1]))
        elif what == "play":
            events.append((upd, "play " + sounds[int(arg)]))
        elif what == "musicload":
            events.append((upd, "musicload " + arg.replace("\\", "/").lower()))
        elif what == "musicvolume":
            events.append((upd, "musicvolume " + arg))
        else:
            events.append((upd, what))
    return events


def main():
    o = orig_events(sys.argv[1])
    p = port_events(sys.argv[2])
    show_all = "--all" in sys.argv
    n = max(len(o), len(p))
    first = None
    for i in range(n):
        a = o[i] if i < len(o) else None
        b = p[i] if i < len(p) else None
        same = a == b
        if not same and first is None:
            first = i
        if show_all or (first is not None and i < first + 12):
            print("%s %-40s %s" % ("  " if same else "!=", a, b))
    print("original %d events, port %d; %s" % (len(o), len(p),
          "the same, update for update" if first is None else "first difference at event %d" % first))


main()
