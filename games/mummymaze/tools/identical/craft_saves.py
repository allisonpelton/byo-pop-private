#!/usr/bin/env python3
"""craft_saves.py <base adv save> <out folder> <spec>...: saved adventures for the screen pass, made from a save the port wrote
(the file's layout is in docs/GAME_NOTES.md, "Saved game"). Each <spec> is
    <slot>:<name>:<progress>[:<marker>[:<seconds>[:<catches>]]]
with the progress as 15 comma-separated values (0 to 14 chambers done, 15 solved, 255 shut; a short list is filled with 255) and
the marker the pyramid the explorer stands on (default: the last open one). A name of "!" and a slot's file with the format
number 3 (a saved game from another version, which the game answers with its bad-save message) is made by `bad:<slot>`.
Only the name, the progress, the marker, the seconds and the catches change; the random generator is the base's."""
import struct
import sys

MAP = ".-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"


def web_string(values):
    bits = []
    for v in values:
        bits += [(v >> i) & 1 for i in range(8)]
    text = "%08X" % len(bits)
    for i in range(0, len(bits), 6):
        chunk = bits[i:i + 6]
        text += MAP[sum(b << k for k, b in enumerate(chunk))]
    return text


def parts(data):
    pos = 0
    n = struct.unpack_from("<i", data, pos)[0]
    name = data[pos + 4:pos + 4 + n]
    pos += 4 + n
    fmt = data[pos:pos + 4]
    pos += 4
    n = struct.unpack_from("<i", data, pos)[0]
    rng = data[pos:pos + 4 + n]
    pos += 4 + n
    n = struct.unpack_from("<i", data, pos)[0]
    pos += 4 + n                              # the progress, replaced
    return name, fmt, rng


def build(base, name, progress, marker, seconds, catches):
    _, fmt, rng = parts(base)
    values = (list(progress) + [255] * 15)[:15]
    text = web_string(values).encode()
    out = struct.pack("<i", len(name)) + name.encode() + fmt + rng + struct.pack("<i", len(text)) + text
    out += bytes([marker]) + struct.pack("<d", seconds) + struct.pack("<i", catches)
    return out


def main(argv):
    base = open(argv[1], "rb").read()
    folder = argv[2]
    for spec in argv[3:]:
        if spec.startswith("bad:"):
            slot = int(spec[4:])
            data = bytearray(open("%s/adv%d.sav" % (folder, slot), "rb").read())
            n = struct.unpack_from("<i", data, 0)[0]
            struct.pack_into("<i", data, 4 + n, 3)
            open("%s/adv%d.sav" % (folder, slot), "wb").write(bytes(data))
            continue
        f = spec.split(":")
        slot, name = int(f[0]), f[1]
        progress = [int(x) for x in f[2].split(",")]
        opened = [i for i, v in enumerate(progress) if v != 255]
        marker = int(f[3]) if len(f) > 3 and f[3] != "" else (opened[-1] if opened else 0)
        seconds = float(f[4]) if len(f) > 4 else 0.0
        catches = int(f[5]) if len(f) > 5 else 0
        open("%s/adv%d.sav" % (folder, slot), "wb").write(build(base, name, progress, marker, seconds, catches))


if __name__ == "__main__":
    main(sys.argv)
