#!/usr/bin/env python3
"""Prints the constants at addresses of the Heavy Weapon image: each as a double, float, int and its 4 bytes.

    python games/heavyweapon/tools/consts.py 52a0e0 52a0d8 ...      # each address
    python games/heavyweapon/tools/consts.py -t 4015d8 6             # a jump table of 6 entries
"""
import os
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import pe


def main():
    image = pe.Image()
    args = sys.argv[1:]
    if args and args[0] == "-t":
        base, count = int(args[1], 16), int(args[2])
        for index in range(count):
            print("%d: %08x" % (index, image.u32(base + 4 * index)))
        return
    for arg in args:
        va = int(arg, 16)
        raw = image.bytes(va, 8)
        print("%08x  double %-24r float %-14r int %d  hex %s" % (va, struct.unpack("<d", raw)[0],
              struct.unpack("<f", raw[:4])[0], struct.unpack("<i", raw[:4])[0], raw.hex()))


if __name__ == "__main__":
    main()
