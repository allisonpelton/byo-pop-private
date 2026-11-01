#!/usr/bin/env python3
"""Find a table of the port in Ancient Japan's exe and print Ancient China's table in its place.

The port's tables were copied from Japan's exe (MapLayer.cpp's levels and paths, the backgrounds, the tips' places).
Given a Japan address (or the table's first values, to find it) this reads the same number of 32-bit values from
China's exe at the address compare_exes.py paired with it (re/mahjongchina/compare/addrmap.tsv).

    python games/mahjongchina/tools/exetable.py --at 0x51ce98 --count 12 [--signed]
    python games/mahjongchina/tools/exetable.py --find 0xe8 0xe9 0xea 0x100 --count 21
    python games/mahjongchina/tools/exetable.py --china 0x51b9c0 --count 21
"""
import argparse
import os
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
sys.path.insert(0, HERE)
import pefile_lite  # noqa: E402

JAPAN = os.path.join(ROOT, "re", "mahjongjapan", "bin", "MahjongEscapeAJ.exe")
CHINA = os.path.join(ROOT, "re", "mahjongchina", "bin", "MahjongEscapeAC.exe")


def read(pe, va, n, signed):
    o = pe.off(va - pe.image_base)
    return list(struct.unpack_from("<%d%s" % (n, "i" if signed else "I"), pe.data, o))


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--at", type=lambda s: int(s, 0))
    ap.add_argument("--find", nargs="+", type=lambda s: int(s, 0))
    ap.add_argument("--china", type=lambda s: int(s, 0))
    ap.add_argument("--count", type=int, required=True)
    ap.add_argument("--signed", action="store_true")
    args = ap.parse_args()
    j, c = pefile_lite.PE(JAPAN), pefile_lite.PE(CHINA)
    amap = {}
    for line in open(os.path.join(ROOT, "re", "mahjongchina", "compare", "addrmap.tsv")):
        p = line.split("\t")
        amap[int(p[0], 16)] = int(p[1], 16)
    va = args.at
    if args.find:
        needle = struct.pack("<%di" % len(args.find), *args.find)
        hits = []
        k = j.data.find(needle)
        while k >= 0:
            hits.append(k)
            k = j.data.find(needle, k + 1)
        for name, sva, vsize, raw, rsize in j.sections:
            for h in hits:
                if raw <= h < raw + rsize:
                    print("japan %s 0x%x" % (name, j.image_base + sva + h - raw))
        if len(hits) != 1:
            return
        for name, sva, vsize, raw, rsize in j.sections:
            if raw <= hits[0] < raw + rsize:
                va = j.image_base + sva + hits[0] - raw
    cva = args.china
    if va is not None:
        print("japan 0x%x: %s" % (va, ", ".join(str(v) for v in read(j, va, args.count, args.signed))))
        if cva is None:
            cva = amap.get(va)
            if cva is None:
                print("not paired; the paired data nearest below and above:")
                below = max((a for a in amap if a < va), default=None)
                above = min((a for a in amap if a > va), default=None)
                for a in (below, above):
                    if a is not None:
                        print("  japan 0x%x -> china 0x%x" % (a, amap[a]))
                return
    print("china 0x%x: %s" % (cva, ", ".join(str(v) for v in read(c, cva, args.count, args.signed))))


main()
