"""tools/soundpreset.py: a soundboard preset (web/soundboards.js, "presets") from a stretch of a port's sound log.

    python tools/soundpreset.py <sound.log> <first update> <last update> [--files <prefix>] [--skip <a,b>] [--name <name>]
        [--when <text>] [--ms <ms an update>] [--gamedir <assets/game>]

A port writes the sound log when BYOPOP_SOUNDLOG names a file (framework/SDLSoundManager.h): each line is the update it
happened on and the event. Every sound the game plays from <first update> to <last update> becomes a step, at its time
from the first play (--ms an update: 10, the framework's frame time, unless the game's own is 5, as the Mahjong
Escapes' is), with its volume and pan (hundredths of a decibel, as
logged; the volume relative to the loudest play in the log, so the game's sound effects level drops out), its pitch,
whether it loops, when it is stopped (only when that is before it would end by itself) and every change of its volume
or pan while it plays. Prints the preset as one line of JavaScript for the table. <prefix> is put before each file name
(the folder as the import writes it, if the log names the files without it); the extension is the file's in the log, or
none, and the table matches either (web/sounds.js tries the extensions the game ships). --skip leaves out the sounds
named (the file's name without folder or extension): those the player's own moves played, a bot's in a driven run.
Venice's port logs its own lines (games/venice/src/VeniceSound.cpp: by file name, a frequency and a volume in percent),
read the same way, the file's rate and length taken from it in --gamedir (Venice runs 250 updates a second: --ms 4).
"""
import argparse
import json
import math
import os
import re
import struct
import sys


def thin(points, eps):
    """Ramer-Douglas-Peucker: the points of a polyline whose straight-line stand-ins stay within eps of every point."""
    if len(points) < 3:
        return points
    (t0, v0), (t1, v1) = points[0], points[-1]
    worst, at = -1, 0
    for i in range(1, len(points) - 1):
        t, v = points[i]
        line = v0 + (v1 - v0) * (t - t0) / (t1 - t0) if t1 != t0 else v0
        if abs(v - line) > worst:
            worst, at = abs(v - line), i
    if worst <= eps:
        return [points[0], points[-1]]
    return thin(points[:at + 1], eps)[:-1] + thin(points[at:], eps)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("log")
    ap.add_argument("first", type=int)
    ap.add_argument("last", type=int)
    ap.add_argument("--files", default="")
    ap.add_argument("--skip", default="")
    ap.add_argument("--name", default="Preset")
    ap.add_argument("--when", default="")
    ap.add_argument("--ms", type=int, default=10)
    ap.add_argument("--gamedir", default="", help="the staged game folder, for a log that names files (Venice's)")
    a = ap.parse_args()

    names, length = {}, {}
    events = []
    # Venice's own sound layer (games/venice/src/VeniceSound.cpp) logs by file name: "play <file> freq=<Hz, -1 the file's
    # own> vol=<gain in percent>", "volume <file> <percent>" and "stop <file>", each for the file's latest voice; its loop
    # comes from the "load" line and its rate and length from the file itself (--gamedir)
    loops, latest, count = {}, {}, 0

    def ogg_rate_length(name):
        path = os.path.join(a.gamedir or ".", name.replace("\\", "/"))
        try:
            data = open(path, "rb").read()
        except OSError:
            return 0, 0
        at = data.find(b"\x01vorbis")
        rate = struct.unpack_from("<I", data, at + 12)[0] if at >= 0 else 0
        last = data.rfind(b"OggS")
        granule = struct.unpack_from("<q", data, last + 6)[0] if last >= 0 else 0
        return rate, (granule / rate if rate else 0)

    def db(percent):
        return int(round(2000 * math.log10(percent / 100.0))) if percent > 0 else -10000

    for line in open(a.log, encoding="utf-8", errors="replace"):
        m = re.match(r"(\d+) load (\S+) loop=(\d)", line)
        if m:
            loops[m.group(2)] = m.group(3)
            continue
        m = re.match(r"(\d+) play (\S+) freq=(-?\d+) vol=(\d+)", line)
        if m:
            name = m.group(2)
            if name not in length:
                rate, length[name] = ogg_rate_length(name)
                names[name] = rate
            rate, freq = names[name], int(m.group(3))
            pitch = freq / rate if freq > 0 and rate else 1.0
            count += 1
            latest[name] = count
            rest = "vol=%d pan=0 pitch=%f loop=%s" % (db(int(m.group(4))), pitch, loops.get(name, "0"))
            events.append((int(m.group(1)), "play", name, count, rest, name.replace("\\", "/"), length[name]))
            continue
        m = re.match(r"(\d+) (stop|volume) (\S+)\s*(-?\d*)", line)
        if m and m.group(3) in latest:
            name = m.group(3)
            rest = str(db(int(m.group(4)))) if m.group(2) == "volume" else ""
            events.append((int(m.group(1)), m.group(2), name, latest[name], rest, name.replace("\\", "/"), length[name]))
            continue
        m = re.match(r"(\d+) load (\d+) (.+?) frames=(\d+) rate=(\d+)", line)
        if m:
            i = int(m.group(2))
            names[i] = m.group(3).replace("\\", "/")
            length[i] = int(m.group(4)) / max(1, int(m.group(5)))
            continue
        m = re.match(r"(\d+) (play|stop|volume|pan|pitch) (\d+) #(\d+)\s*(.*)", line)
        if m:
            # the name and length the id has now: a game may load another file into an id it used before (Bookworm
            # Adventures' foes' sounds)
            sid = int(m.group(3))
            events.append((int(m.group(1)), m.group(2), sid, int(m.group(4)), m.group(5), names.get(sid, str(sid)), length.get(sid, 0)))

    skip = set(x for x in a.skip.split(",") if x)

    def short(name):
        return re.sub(r"\.[^./]*$", "", name.rsplit("/", 1)[-1])

    # a volume above full is logged as more than 0 dB, and plays at full (framework/SDLSoundManager.cpp, LogVolume)
    vols = [min(0, int(re.search(r"vol=(-?\d+)", e[4]).group(1))) for e in events if e[1] == "play"]
    base = max(vols) if vols else 0
    steps, live = [], {}
    t0 = None
    for (u, kind, sid, inst, rest, name, secs) in events:
        if kind == "play" and a.first <= u <= a.last and short(name) not in skip:
            m = re.search(r"vol=(-?\d+) pan=(-?\d+) pitch=([-\d.]+) loop=(\d)", rest)
            if t0 is None:
                t0 = u
            step = {"file": a.files + name, "at": (u - t0) * a.ms, "sid": sid, "u": u, "len": secs}
            v, p, r, loop = min(0, int(m.group(1))) - base, int(m.group(2)), float(m.group(3)), m.group(4) == "1"
            if v:
                step["v"] = v
            if p:
                step["p"] = p
            if r and abs(r - 1) > 1e-4:
                step["r"] = round(r, 4)
            if loop:
                step["loop"] = 1
            step["_v"], step["_p"] = v, p
            steps.append(step)
            live[inst] = step
        elif inst in live:
            s = live[inst]
            if u > a.last and kind != "stop":
                continue
            at = (u - t0) * a.ms
            if kind == "stop":
                natural = s["at"] + s["len"] * 1000 / s.get("r", 1)
                if s.get("loop") or at < natural - 10:
                    s["end"] = at
                del live[inst]
            elif kind == "volume":
                v = min(0, int(rest.split()[0])) - base
                if v != s["_v"]:
                    s.setdefault("ramp", []).append([at, v])
                    s["_v"] = v
            elif kind == "pan":
                p = int(rest.split()[0])
                if p != s["_p"]:
                    s.setdefault("pans", []).append([at, p])
                    s["_p"] = p
    # a loop still playing at the end of the stretch stops there
    for s in live.values():
        if s.get("loop") and "end" not in s:
            s["end"] = (a.last - t0) * a.ms
    # (a loop that starts as the stretch ends would not be heard)
    steps = [s for s in steps if not (s.get("loop") and s["end"] <= s["at"])]
    if not steps:
        sys.exit("no plays from update %d to %d" % (a.first, a.last))
    # a fade is logged every update: kept to the points a straight line between them misses by more than 0.3 dB
    for s in steps:
        for key in ("ramp", "pans"):
            if key in s and len(s[key]) > 2:
                s[key] = thin(s[key], 30)
    out = []
    for s in steps:
        opts = {k: s[k] for k in ("v", "p", "r", "loop", "end", "ramp", "pans") if k in s}
        f = re.sub(r"\.(ogg|wav|mp3|au)$", "", s["file"], flags=re.I)
        out.append([f, s["at"], opts] if opts else [f, s["at"]])
    preset = {"name": a.name, "when": a.when, "steps": out}
    if not a.when:
        del preset["when"]
    print(json.dumps(preset, separators=(", ", ": ")).replace('{"name"', '{ name').replace('"when": ', "when: ")
          .replace('"steps": ', "steps: ").replace('"', '"'))


if __name__ == "__main__":
    main()
