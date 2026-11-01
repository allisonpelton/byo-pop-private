"""Compares what the original Bejeweled and the port play, by order and state, from the logs of one scripted run each
(orig.sh with ORIG_SOUND=on, port.sh). It reads the logs with Bookworm Adventures' soundcmp.py (the same framework,
DirectSound for the sounds and BASS for the music) and compares:

- every play of a sound, in order, with its volume, pan and pitch, the sounds named by load order (checked by length);
- the music: which song plays from which order, stops, and each change of its volume while it plays.

Stops of sounds are not compared: the original's DirectSound manager stops (frees) a duplicate it played once its
sound has ended, when it next looks for a free one, and the port logs a stop only where the game asks for one, so the
two lists put inaudible stops in different places. A stop that cuts a sound short would show as a missing or extra
play's effect in the music or as a later difference, and none does in these runs.

    python games/bejeweled/tools/origcmp/soundcmp.py <origrun run dir> <port run dir> [--times]
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "..", "bookwormadv", "tools", "pilot"))
import soundcmp as S  # noqa: E402


def audible(music):
    """The music events without volume changes of a song that is stopped (after its stop, before its next play): they
    cannot be heard. At quit the port's fade sets a stopped song's volume once more; the original exits first."""
    out, stopped = [], set()
    for e in music:
        kind, song = e[1], e[2]
        if kind == "music stop":
            stopped.add(song)
        elif kind == "music play":
            stopped.discard(song)
        elif kind == "music volume" and song in stopped:
            continue
        out.append(e)
    return out


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    times = "--times" in sys.argv
    orun, prun = args
    osnd, oloads = S.orig_sounds(os.path.join(orun, "logs", "sound.log"))
    omus, _ = S.orig_music(os.path.join(orun, "game", "basslog.txt"))
    psnd, ploads = S.port_sounds(os.path.join(prun, "portsound.log"))
    pmus, _ = S.port_music(os.path.join(prun, "portsound.log"))
    oloads = [x for x in oloads if x is not None]
    names = {}
    for k, (o, p) in enumerate(zip(oloads, ploads)):
        names[k] = os.path.basename(p[1])
    pnames = dict((p[0], os.path.basename(p[1])) for p in ploads)
    print("sounds loaded: original %d, port %d" % (len(oloads), len(ploads)))
    oplays = [e for e in osnd if e[1] == "play"]
    pplays = [e for e in psnd if e[1] == "play"]
    a = S.compare("sound plays", oplays, pplays, lambda s: names.get(s, "o%s" % s),
                  lambda s: pnames.get(s, "p%s" % s), times)
    b = S.compare("music", audible(omus), audible(pmus), lambda s: "song%s" % s, lambda s: "song%s" % s, times)
    print("soundcmp: %s" % ("same" if a and b else "DIFFERENT"))
    sys.exit(0 if a and b else 1)


if __name__ == "__main__":
    main()
