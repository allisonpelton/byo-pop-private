#!/usr/bin/env python3
"""The string at each virtual address of TalismaniaGame.exe (the Ghidra export marks short ones as DAT_ names).

    python -I games/talismania/tools/rdstr.py <hex address> [<hex address>...]
"""
import os
import struct
import sys

EXE = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..", "re", "talismania", "bin",
                                    "TalismaniaGame.exe"))


def sections(data):
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    count = struct.unpack_from("<H", data, pe + 6)[0]
    optional = struct.unpack_from("<H", data, pe + 20)[0]
    base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
    first = pe + 24 + optional
    out = []
    for i in range(count):
        size, rva, raw_size, raw = struct.unpack_from("<IIII", data, first + 8 + i * 40)
        out.append((base + rva, size, raw))
    return out


def main():
    data = open(EXE, "rb").read()
    table = sections(data)
    for arg in sys.argv[1:]:
        address = int(arg, 16)
        for start, size, raw in table:
            if start <= address < start + size:
                text = data[raw + address - start:raw + address - start + 200].split(b"\0")[0]
                print("%x %r" % (address, text.decode("latin-1")))
                break


if __name__ == "__main__":
    main()
