#!/usr/bin/env python3
"""swfinfo.py: what a cutscene's SWF file is made of: its header, a count of each tag (and of the actions in
its DoAction tags) and of the features its shapes and placements use, to see which parts of the format a
player for the game's cutscenes needs.

    python games/bookwormadv/tools/swfinfo.py assets/bookwormadv/images/cutscenes/*.swf
"""
import re
import struct
import sys
import zlib
from collections import Counter

TAGS = {
    0: "End", 1: "ShowFrame", 2: "DefineShape", 4: "PlaceObject", 5: "RemoveObject", 6: "DefineBits",
    7: "DefineButton", 8: "JPEGTables", 9: "SetBackgroundColor", 10: "DefineFont", 11: "DefineText",
    12: "DoAction", 13: "DefineFontInfo", 14: "DefineSound", 15: "StartSound", 17: "DefineButtonSound",
    18: "SoundStreamHead", 19: "SoundStreamBlock", 20: "DefineBitsLossless", 21: "DefineBitsJPEG2",
    22: "DefineShape2", 24: "Protect", 26: "PlaceObject2", 28: "RemoveObject2", 32: "DefineShape3",
    33: "DefineText2", 34: "DefineButton2", 35: "DefineBitsJPEG3", 36: "DefineBitsLossless2",
    37: "DefineEditText", 39: "DefineSprite", 43: "FrameLabel", 45: "SoundStreamHead2",
    46: "DefineMorphShape", 48: "DefineFont2", 56: "ExportAssets", 57: "ImportAssets",
    58: "EnableDebugger", 59: "DoInitAction", 60: "DefineVideoStream", 61: "VideoFrame",
    62: "DefineFontInfo2", 64: "EnableDebugger2", 65: "ScriptLimits", 66: "SetTabIndex",
    69: "FileAttributes", 70: "PlaceObject3", 73: "DefineFontAlignZones", 74: "CSMTextSettings",
    75: "DefineFont3", 77: "Metadata", 83: "DefineShape4", 84: "DefineMorphShape2",
}
SOUND_FORMATS = {0: "raw", 1: "ADPCM", 2: "MP3", 3: "raw LE", 6: "Nellymoser"}


def tags(data, pos, end):
    while pos < end:
        code, = struct.unpack_from("<H", data, pos)
        pos += 2
        tag, length = code >> 6, code & 0x3f
        if length == 0x3f:
            length, = struct.unpack_from("<I", data, pos)
            pos += 4
        yield tag, data[pos:pos + length]
        pos += length
        if tag == 0:
            return


def actions(body):
    pos = 0
    while pos < len(body):
        code = body[pos]
        pos += 1
        if code == 0:
            return
        if code >= 0x80:
            length, = struct.unpack_from("<H", body, pos)
            yield code, body[pos + 2:pos + 2 + length]
            pos += 2 + length
        else:
            yield code, b""


def rect_size(body, pos):
    nbits = body[pos] >> 3
    return (5 + 4 * nbits + 7) // 8


def fill_styles(body, pos, shape_version, morph=False):
    """The fill style types of a shape's first fill style array, up to the first that is not solid (0x00 solid,
    0x10 linear, 0x12 radial, 0x13 focal, 0x4x bitmap; the rest would need the matrices parsed)."""
    count = body[pos]
    pos += 1
    if count == 0xff and shape_version >= 2:
        count, = struct.unpack_from("<H", body, pos)
        pos += 2
    types = []
    for _ in range(count):
        kind = body[pos]
        types.append(kind)
        if kind != 0x00:
            break
        pos += 1 + (8 if morph else (4 if shape_version >= 3 else 3))
    return types


def census(data, counts, extra):
    for tag, body in data:
        counts[TAGS.get(tag, "tag%d" % tag)] += 1
        if tag == 39:  # DefineSprite: id, frame count, then its own tags
            census(tags(body, 4, len(body)), counts, extra)
        elif tag in (12, 59):
            for code, args in actions(body if tag == 12 else body[2:]):
                extra["action 0x%02x" % code] += 1
                if code == 0x83:
                    url, target = args.split(b"\0")[:2]
                    extra["geturl %r %r" % (url.decode("latin1"), target.decode("latin1"))] += 1
        elif tag in (18, 45):
            extra["stream sound " + SOUND_FORMATS.get(body[1] >> 4, str(body[1] >> 4))] += 1
        elif tag == 14:
            extra["event sound " + SOUND_FORMATS.get(body[2] >> 4, str(body[2] >> 4))] += 1
        elif tag in (2, 22, 32):
            for kind in fill_styles(body, 2 + rect_size(body, 2), {2: 1, 22: 2, 32: 3}[tag]):
                extra["fill 0x%02x" % kind] += 1
        elif tag == 46:
            pos = 2 + rect_size(body, 2)
            pos += rect_size(body, pos) + 4
            for kind in fill_styles(body, pos, 2, True):
                extra["morph fill 0x%02x" % kind] += 1
        elif tag == 26:
            for bit, name in ((0x40, "clipdepth"), (0x20, "name"), (0x10, "ratio"), (0x08, "cxform"),
                              (0x04, "matrix"), (0x02, "character"), (0x01, "move")):
                if body[0] & bit:
                    extra["place " + name] += 1


def main():
    for path in sys.argv[1:]:
        raw = open(path, "rb").read()
        sig, version, length = raw[:3], raw[3], struct.unpack_from("<I", raw, 4)[0]
        data = raw[:8] + (zlib.decompress(raw[8:]) if sig == b"CWS" else raw[8:])
        nbits = data[8] >> 3
        rect_bytes = (5 + 4 * nbits + 7) // 8
        bits = int.from_bytes(data[8:8 + rect_bytes], "big")
        total = rect_bytes * 8
        vals = [(bits >> (total - 5 - nbits * (i + 1))) & ((1 << nbits) - 1) for i in range(4)]
        rate, frames = struct.unpack_from("<HH", data, 8 + rect_bytes)
        print("%s: %s v%d, %d bytes, %dx%d, %.2f fps, %d frames" % (
            path, sig.decode(), version, length, (vals[1] - vals[0]) // 20, (vals[3] - vals[2]) // 20,
            rate / 256, frames))
        counts, extra = Counter(), Counter()
        census(tags(data, 8 + rect_bytes + 4, len(data)), counts, extra)
        print("  tags:", ", ".join("%s %d" % kv for kv in sorted(counts.items())))
        print("  details:", ", ".join("%s %d" % kv for kv in sorted(extra.items())))


if __name__ == "__main__":
    main()
