#!/usr/bin/env python3
"""craft_adventure.py <saved adventure> <out> [solved]: a copy of a saved adventure (adv0.sav, made by a bot or a player) with the
first [solved] pyramids (default 14: every one but the tomb) marked solved, the tomb open at no chambers done, and the marker on
the last solved one, so that a test reaches the last treasure by playing one pyramid. The file's layout is in
docs/GAME_NOTES.md ("The adventure"): int length and name, int 2, int length and the random generator, int length and the
progress as a Buffer's web string (the 15 values as bytes: 0xff shut, 0 to 14 chambers done, 15 solved), a byte for the
pyramid the marker stands on, a double and an int."""
import struct
import sys

MAP = ".-0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz"


def web_string(values):
    bits = []
    for v in values:
        bits += [(v >> i) & 1 for i in range(8)]    # a byte's bits, the lowest first, as Buffer::ReadNumBits reads them
    text = "%08X" % len(bits)
    for i in range(0, len(bits), 6):
        chunk = bits[i:i + 6]
        text += MAP[sum(b << k for k, b in enumerate(chunk))]
    return text


data = open(sys.argv[1], "rb").read()
solved = int(sys.argv[3]) if len(sys.argv) > 3 else 14
pos = 0
n = struct.unpack_from("<i", data, pos)[0]
pos += 4 + n                       # the name
pos += 4                           # the format, 2
n = struct.unpack_from("<i", data, pos)[0]
pos += 4 + n                       # the random generator
n = struct.unpack_from("<i", data, pos)[0]
old = data[pos + 4:pos + 4 + n].decode()
values = [15] * solved + [0] + [0xFF] * (14 - solved)
text = web_string(values)
assert len(text) == len(old), (text, old)
out = bytearray(data)
out[pos + 4:pos + 4 + n] = text.encode()
out[pos + 4 + n] = min(solved, 14) - 1 if solved > 0 else 0     # the marker's pyramid
open(sys.argv[2], "wb").write(out)
print(text, "(was %s)" % old)
