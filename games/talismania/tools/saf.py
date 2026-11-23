#!/usr/bin/env python3
"""List or extract the entries of Talismania.saf (a Sprout Archive File), for reading the game's data while porting.

    python -I games/talismania/tools/saf.py list [<archive>] [<substring>]
    python -I games/talismania/tools/saf.py cat <name> [<archive>]            the entry's bytes on stdout
    python -I games/talismania/tools/saf.py extract <out dir> [<archive>] [<substring>]

The format (docs/POPCAP_GAMES_SURVEY.md, "Talismania Deluxe"): the header "FFAS", u16 1, u16 0, u32 offset of the
table; the table u16 1, u16 0, a 16-byte keyed MD5, u32 count, then for each entry u32 offset, u32 size, a 16-byte keyed
MD5 and a u16 name length followed by the name, which that length counts with its NUL. The digests are not checked here. The default archive is
assets/talismania/Talismania.saf. Extract only into a folder outside the repository: the entries are the game's files.
"""
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT = os.path.join(HERE, "..", "..", "..", "assets", "talismania", "Talismania.saf")


def read_toc(data):
    magic, major, minor, toc = struct.unpack_from("<4sHHI", data, 0)
    if magic != b"FFAS":
        raise SystemExit("not a Sprout archive")
    position = toc + 4 + 16
    (count,) = struct.unpack_from("<I", data, position)
    position += 4
    entries = []
    for _ in range(count):
        offset, size = struct.unpack_from("<II", data, position)
        position += 8 + 16
        (length,) = struct.unpack_from("<H", data, position)
        position += 2
        name = data[position:position + length].split(bytes(1))[0].decode("latin-1")
        position += length
        entries.append((name, offset, size))
    return entries


def main():
    args = sys.argv[1:]
    if not args:
        raise SystemExit(__doc__)
    command = args[0]
    if command == "cat":
        name = args[1]
        path = args[2] if len(args) > 2 else DEFAULT
    elif command == "extract":
        out = args[1]
        path = args[2] if len(args) > 2 else DEFAULT
        needle = args[3] if len(args) > 3 else ""
    else:
        path = args[1] if len(args) > 1 else DEFAULT
        needle = args[2] if len(args) > 2 else ""
    with open(path, "rb") as f:
        data = f.read()
    entries = read_toc(data)
    if command == "list":
        for name, offset, size in entries:
            if needle in name:
                print("%8d %8d %s" % (offset, size, name))
    elif command == "cat":
        for entry, offset, size in entries:
            if entry == name:
                sys.stdout.buffer.write(data[offset:offset + size])
                return
        raise SystemExit("no entry " + name)
    elif command == "extract":
        for entry, offset, size in entries:
            if needle in entry:
                target = os.path.join(out, entry)
                os.makedirs(os.path.dirname(target), exist_ok=True)
                with open(target, "wb") as f:
                    f.write(data[offset:offset + size])
    else:
        raise SystemExit(__doc__)


if __name__ == "__main__":
    main()
