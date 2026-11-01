#!/usr/bin/env python3
"""Re-sign a Mahjong Escape: Ancient China player (<id>.usr) for another install folder.

A player's file ends with a CRC-32 of its data followed by the exe's folder (User::Serialize, 0x415fc0), so the
original loads a player only from the folder that saved it. To play the original from a player the port saved (in
Windows Sandbox the copy runs from C:\\run\\game\\), the file is signed again for that folder.

    python games/mahjongchina/tools/resign.py <in.usr> <out.usr> [--from FOLDER] [--to FOLDER] [--progress N]

--from defaults to the port's own signature (the collection's install folder) and is checked first; --to defaults
to the sandbox's C:\\run\\game\\. --progress also sets the Dynasty Adventure's progress (the levels done,
the short after the 150 classic records), to start a run at a later stage from a real player.
"""
import argparse
import struct
import sys
import zlib

INSTALL = "C:\\Program Files (x86)\\Popcap Game Collection\\Mahjong Escape Ancient China\\"
SANDBOX = "C:\\run\\game\\"


def sign(body, folder):
    # jb::Stream runs the CRC register over the data (0x424ec0), inverts it, runs it on over the folder and inverts it
    # again (0x4252d0), and writes the inverse of that, high byte first like every int of the stream
    m = 0xFFFFFFFF
    return ~zlib.crc32(folder.encode("latin-1"), ~zlib.crc32(body) & m) & m


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("src")
    ap.add_argument("dst")
    ap.add_argument("--from", dest="old", default=INSTALL)
    ap.add_argument("--to", dest="new", default=SANDBOX)
    ap.add_argument("--progress", type=int)
    args = ap.parse_args()
    data = open(args.src, "rb").read()
    body, sig = data[:-4], struct.unpack(">I", data[-4:])[0]
    if sign(body, args.old) != sig:
        sys.exit("%s is not signed for %s" % (args.src, args.old))
    if args.progress is not None:
        # the magic, the version, the id, the name and its end, two flags, the layout, the quads, two styles and a flag
        at = body.index(b"\0", 5) + 10 + 150 * 6
        body = body[:at] + struct.pack(">h", args.progress) + body[at + 2:]
    open(args.dst, "wb").write(body + struct.pack(">I", sign(body, args.new)))


main()
