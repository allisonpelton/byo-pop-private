#!/usr/bin/env python3
"""The music of the original and of the port as two lists of what can be heard, set side by side.

    python games/typershark/tools/phase2/musicsum.py <tour folder with orun/game/basslog.txt and portsound.log>

Big Money's soundcmp.py turns each side's calls into events (play <song> at an order, volume, stop, pause, resume); it compares
them line by line, and two songs that fade at the same time interleave their volume steps differently on the two sides. Here
each run of volume steps of one song becomes one ramp "<song> <from>..<to>", so that a cross-fade reads the same whatever the
interleaving, and the two lists are printed with their differences marked."""
import difflib
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "bigmoney", "tools", "origcmp"))
import soundcmp as BM  # noqa: E402


def ramps(events):
    """events: Big Money's lines ("music play song0 order=17 vol=0", "music volume song0 5", ...) -> plays, stops and ramps"""
    out, open_ramp = [], {}

    def close(song):
        r = open_ramp.pop(song, None)
        if r:
            out.append("%s ramp %d..%d" % (song, r[0], r[1]))

    for _, what, song, value in events:        # Big Money's tuples: (update, "music volume", song, value)
        kind, song = what.split()[1], "song%d" % song
        if kind == "volume":
            v = int(value)
            if song in open_ramp:
                open_ramp[song][1] = v
            else:
                open_ramp[song] = [v, v]
            continue
        close(song)
        out.append("%s %s%s" % (song, kind, " " + value if value else ""))
    for song in sorted(open_ramp):
        close(song)
    return out


def main():
    d = sys.argv[1]
    o, _ = BM.orig_music(os.path.join(d, "orun", "game", "basslog.txt"))
    p, _ = BM.port_music(os.path.join(d, "portsound.log"))
    a, b = ramps(o), ramps(p)
    print("original: %d events, %d plays, stops and ramps; port: %d events, %d" % (len(o), len(a), len(p), len(b)))
    bad = 0
    for line in difflib.ndiff(a, b):
        if line[0] in "+-":
            bad += 1
        if line[0] != "?":
            print(("  " if line[0] == " " else line[0] + " ") + line[2:].replace("+ ", "", 0))
    print("differences: %d (- the original's, + the port's)" % bad)


if __name__ == "__main__":
    main()
