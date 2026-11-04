"""Compares what the original Big Money and the port play, by order and state, from the logs of one scripted run each
(orig.sh with ORIG_SOUND=on, port.sh with BYOPOP_SOUNDLOG=<run dir>/portsound.log). The sounds are read with Bookworm
Adventures' soundcmp.py (origrun's DirectSound stand-in on the original's side) and compared as Bejeweled's are: every
play, in order, with its volume, pan and pitch, the sounds named by load order. Stops of sounds are not compared (see
games/bejeweled/tools/origcmp/soundcmp.py: the original's duplicates are freed when next needed).

The music goes through Big Money's older BASS (games/bigmoney/tools/bassproxy logs it): a song plays with
BASS_MusicPlayEx(handle, order, flags, reset), its volume is BASS_ChannelSetAttributes' second argument (a fade sets it
each update), and it pauses and resumes with BASS_ChannelPause/Resume. Each side becomes a list of

    music play <song> order=<n|cur> vol=<0..100>, music volume <song> <v>, music stop <song>,
    music pause <song>, music resume <song>

with the songs by load order (the port's ids are its load order too). Only what can be heard is listed: a volume or a
stop of a song that is not playing is left out (the port logs a fade-out and a stop of every song when it switches
screens and quits, the original asks BASS only about the songs that play), and a volume is listed when it changes.

    python games/bigmoney/tools/origcmp/soundcmp.py <origrun run dir> <port run dir> [--times]
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "..", "bookwormadv", "tools", "pilot"))
import soundcmp as S  # noqa: E402


# Sounds whose original load differs, known and left as they are: (frames, rate, channels) the original loads, why
KNOWN = {"bad": ((4096, 22050, 2), "the install's FMOD cache holds only the first 4096 frames ("
                 "cached/sounds/bad.wav); played only for an invalid registration code, an oddity not chased")}


class Songs:
    """The state of each song, turning raw calls into the audible events."""

    def __init__(self):
        self.vol, self.playing, self.paused, self.out = {}, set(), set(), []

    def volume(self, t, s, v):
        if self.vol.get(s) == v:
            return
        self.vol[s] = v
        if s in self.playing:
            self.out.append((t, "music volume", s, v))

    def play(self, t, s, order):
        self.playing.add(s)
        self.paused.discard(s)
        self.out.append((t, "music play", s, "order=%s vol=%s" % ("cur" if order < 0 else order, self.vol.get(s, "?"))))

    def stop(self, t, s):
        if s in self.playing:
            self.playing.discard(s)
            self.paused.discard(s)
            self.out.append((t, "music stop", s, None))

    def pause(self, t, s, on):
        if s in self.playing and (s in self.paused) != on:
            (self.paused.add if on else self.paused.discard)(s)
            self.out.append((t, "music pause" if on else "music resume", s, None))


def orig_music(path):
    handles, loads, pending = {}, [], None
    m = Songs()
    lines = [ln.split() for ln in open(path, errors="replace") if ln[:1].isdigit()]
    for k, p in enumerate(lines):
        t, fn = int(p[0]), p[2]
        if fn == "=":
            if p[3] == "BASS_MusicLoad" and pending is not None:
                handles[p[4]] = pending
                pending = None
            continue
        if fn == "BASS_MusicLoad":
            pending = len(loads)
            loads.append(p[p.index("file") + 1] if "file" in p else "?")
            continue
        h = p[3] if len(p) > 3 else None
        if h not in handles:
            continue
        s = handles[h]
        if fn in ("BASS_MusicPlayEx", "BASS_MusicPlay"):
            pos = int(p[4], 16) if fn == "BASS_MusicPlayEx" else 0xffffffff
            m.play(t, s, -1 if pos == 0xffffffff else pos & 0xffff)
        elif fn == "BASS_ChannelStop":
            # a play starts with a stop: a stop is an event only when no play of the song follows in the same update
            nxt = [q[2] for q in lines[k + 1:k + 12] if len(q) > 3 and q[3] == h and q[2] != "=" and int(q[0]) == t]
            if not any(f.startswith("BASS_MusicPlay") for f in nxt):
                m.stop(t, s)
        elif fn == "BASS_ChannelSetAttributes":
            v = int(p[5], 16)
            if v != 0xffffffff:
                m.volume(t, s, v)
        elif fn in ("BASS_ChannelPause", "BASS_ChannelResume"):
            m.pause(t, s, fn == "BASS_ChannelPause")
    return m.out, loads


def port_music(path):
    loads = []
    m = Songs()
    for line in open(path, errors="replace"):
        p = line.split()
        if len(p) < 4 or p[1] != "music":
            continue
        t, ev, s = int(p[0]), p[2], p[3]
        if ev == "load":
            loads.append(p[4])
            continue
        if ev == "master":
            continue
        s = int(s)
        if ev in ("play", "fadein"):
            m.play(t, s, int(p[4].split("=")[1]))
        elif ev == "stop":
            m.stop(t, s)
        elif ev == "volume":
            m.volume(t, s, int(p[4]))
        elif ev in ("pause", "resume"):
            m.pause(t, s, ev == "pause")
    return m.out, loads


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    times = "--times" in sys.argv
    orun, prun = args
    osnd, oloads = S.orig_sounds(os.path.join(orun, "logs", "sound.log"))
    omus, omloads = orig_music(os.path.join(orun, "game", "basslog.txt"))
    plog = os.path.join(prun, "portsound.log")
    psnd, ploads = S.port_sounds(plog)
    pmus, pmloads = port_music(plog)
    oloads = [x for x in oloads if x is not None]
    bad = 0
    for k, (o, p) in enumerate(zip(oloads, ploads)):
        size, rate, bits, ch = o
        frames = size // (bits // 8 * ch) if bits and ch else -1
        if (frames, rate, ch) != (p[2], p[3], p[4]):
            known = KNOWN.get(os.path.basename(p[1].replace("\\", "/")))
            if known and (frames, rate, ch) == known[0]:
                print("sound %d (%s): original %d frames, port %d: %s" % (k, p[1], frames, p[2], known[1]))
                continue
            bad += 1
            print("sound %d: original %d frames %d Hz %d ch, port %s %d frames %d Hz %d ch" %
                  (k, frames, rate, ch, p[1], p[2], p[3], p[4]))
    print("sounds loaded: original %d, port %d, %d pairs differ" % (len(oloads), len(ploads), bad))
    mbad = [k for k, (o, p) in enumerate(zip(omloads, pmloads)) if o.lower() != p.lower()]
    print("songs loaded: original %d, port %d, %d names differ" % (len(omloads), len(pmloads), len(mbad)))
    names = dict((k, os.path.basename(p[1].replace("\\", "/"))) for k, p in enumerate(ploads))
    pnames = dict((p[0], os.path.basename(p[1].replace("\\", "/"))) for p in ploads)
    a = S.compare("sound plays", [e for e in osnd if e[1] == "play"], [e for e in psnd if e[1] == "play"],
                  lambda s: names.get(s, "o%s" % s), lambda s: pnames.get(s, "p%s" % s), times)
    b = S.compare("music", omus, pmus, lambda s: "song%s" % s, lambda s: "song%s" % s, times)
    ok = a and b and not bad and not mbad and len(oloads) == len(ploads) and len(omloads) == len(pmloads)
    print("soundcmp: %s" % ("same" if ok else "DIFFERENT"))
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()
