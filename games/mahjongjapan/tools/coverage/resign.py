#!/usr/bin/env python3
"""Changes the progress of an Ancient Japan player (<id>.usr) that the original wrote, and signs it again, so that a coverage
run can start at a later stage from a real player (docs/COVERAGE.md).

A player's file ends with a CRC-32 of its data followed by the exe's folder (User::Serialize, 0x415fc0; the same code as
games/mahjongchina/tools/resign.py, which has 150 classic records where Ancient Japan has 175), so the original loads a
player only from the folder that saved it. The staged copy's folder is C:\\byo-og\\mahjongjapancov\\game\\ (run.sh).

    python resign.py <in.usr> <out.usr> --progress N [--folder FOLDER]
"""
import argparse
import struct
import sys
import zlib

FOLDER = "C:\\byo-og\\mahjongjapancov\\game\\"


def sign(body, folder):
    # jb::Stream runs the CRC register over the data (0x424ec0), inverts it, runs it on over the folder and inverts it
    # again (0x4252d0), and writes the inverse of that, high byte first like every int of the stream
    m = 0xFFFFFFFF
    return ~zlib.crc32(folder.encode("latin-1"), ~zlib.crc32(body) & m) & m


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("src")
    ap.add_argument("dst")
    ap.add_argument("--folder", default=FOLDER)
    ap.add_argument("--progress", type=int, required=True)
    args = ap.parse_args()
    data = open(args.src, "rb").read()
    body, sig = data[:-4], struct.unpack(">I", data[-4:])[0]
    if sign(body, args.folder) != sig:
        sys.exit("%s is not signed for %s" % (args.src, args.folder))
    # the magic, the version, the id, the name and its end, two flags, the layout, the quads, two styles and a flag,
    # then the 175 classic records of 6 bytes and the progress
    at = body.index(b"\0", 5) + 10 + 175 * 6
    body = body[:at] + struct.pack(">h", args.progress) + body[at + 2:]
    open(args.dst, "wb").write(body + struct.pack(">I", sign(body, args.folder)))
    print("progress %d, signed for %s" % (args.progress, args.folder))


main()
