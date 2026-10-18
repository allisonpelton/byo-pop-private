#!/usr/bin/env python3
"""List or extract a PopCap main.pak (SexyAppFramework PakInterface format).

    python tools/popcap_pak.py list    <main.pak> [--key 0xF7]
    python tools/popcap_pak.py extract <main.pak> <out dir> [--key 0xF7]

Format (PakLib/PakInterface.cpp): the whole file is XORed with one byte (0xF7 in the shipped games).
Header: uint32 magic 0xBAC04AC0, uint32 version 0. Then records until a flags byte with 0x80:
uint8 flags, uint8 name length, name, int32 size, FILETIME (8 bytes). File data follows the table in
record order.
"""
import argparse
import os
import struct
import sys

MAGIC = 0xBAC04AC0


def read_records(data, key):
    def b(i, n):
        return bytes(x ^ key for x in data[i:i + n])

    magic, version = struct.unpack("<II", b(0, 8))
    if magic != MAGIC:
        raise ValueError(f"not a PopCap pak with key {key:#x} (magic {magic:#x})")
    pos = 8
    records = []
    while True:
        flags = data[pos] ^ key
        pos += 1
        if flags & 0x80:
            break
        nlen = data[pos] ^ key
        name = b(pos + 1, nlen).decode("latin-1")
        size, filetime = struct.unpack("<iq", b(pos + 1 + nlen, 12))
        pos += 1 + nlen + 12
        records.append((name, size, filetime))
    offset = pos
    out = []
    for name, size, filetime in records:
        out.append((name.replace("\\", "/"), offset, size))
        offset += size
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("command", choices=["list", "extract"])
    ap.add_argument("pak")
    ap.add_argument("out", nargs="?")
    ap.add_argument("--key", type=lambda s: int(s, 0), default=0xF7)
    args = ap.parse_args()
    data = open(args.pak, "rb").read()
    records = read_records(data, args.key)
    if args.command == "list":
        for name, offset, size in records:
            print(f"{size:>10}  {name}")
        print(f"{len(records)} files", file=sys.stderr)
        return
    if not args.out:
        sys.exit("extract needs an output directory")
    for name, offset, size in records:
        dest = os.path.join(args.out, *name.split("/"))
        os.makedirs(os.path.dirname(dest), exist_ok=True)
        with open(dest, "wb") as f:
            f.write(bytes(x ^ args.key for x in data[offset:offset + size]))
    print(f"extracted {len(records)} files to {args.out}")


if __name__ == "__main__":
    main()
