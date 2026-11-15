#!/usr/bin/env python3
"""Extract Word Harmony's archive for reading: the files of WHArchive.saf into re/wordharmony/saf, under their
lower-case names (as the engine looks them up). The format is in docs/ENGINE.md, "Files".

    python games/wordharmony/tools/extract_saf.py [archive] [-o re/wordharmony/saf]
"""
import argparse
import os
import struct

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("archive", nargs="?", default=os.path.join(ROOT, "assets", "wordharmony", "WHArchive.saf"))
    ap.add_argument("-o", "--out", default=os.path.join(ROOT, "re", "wordharmony", "saf"))
    args = ap.parse_args()
    d = open(args.archive, "rb").read()
    if d[:4] != b"FFAS" or struct.unpack_from("<I", d, 4)[0] != 1:
        raise SystemExit("not a Sprout archive: " + args.archive)
    pos = struct.unpack_from("<I", d, 8)[0]
    if struct.unpack_from("<I", d, pos)[0] != 1:
        raise SystemExit("bad table version")
    count = struct.unpack_from("<I", d, pos + 20)[0]
    pos += 24
    for _ in range(count):
        off, size = struct.unpack_from("<II", d, pos)
        name_len = struct.unpack_from("<H", d, pos + 24)[0]
        pos += 26
        name = d[pos:pos + name_len].split(b"\0")[0].decode("latin-1").lower()
        pos += name_len
        path = os.path.join(args.out, *name.replace("\\", "/").split("/"))
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "wb") as f:
            f.write(d[off:off + size])
    print("%d files -> %s" % (count, args.out))


if __name__ == "__main__":
    main()
