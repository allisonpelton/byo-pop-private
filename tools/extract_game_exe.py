#!/usr/bin/env python3
"""Carve the real game executable out of PopCap's DRM wrapper (WinBej.exe).

The Steam build of Bejeweled Deluxe appends the unencrypted game PE after a `!popcapdrmprotect!`
marker. This writes it to re/bin/BejeweledGame.exe for analysis in Ghidra.

    python tools/extract_game_exe.py ["A:/.../Bejeweled Deluxe/WinBej.exe"] [-o re/bin/BejeweledGame.exe]
"""
import argparse
import os
import struct
import sys

MARKER = b"!popcapdrmprotect!"
DEFAULT = r"A:\SteamLibrary\steamapps\common\Bejeweled Deluxe\WinBej.exe"


def pe_end(data, base):
    """Return the end offset (relative to base) of a PE image's last section."""
    pe = base + struct.unpack_from("<I", data, base + 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        raise ValueError("not a PE image")
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    opt_size = struct.unpack_from("<H", data, pe + 20)[0]
    sections = pe + 24 + opt_size
    end = 0
    for i in range(nsec):
        raw_size, raw_ptr = struct.unpack_from("<II", data, sections + i * 40 + 16)
        end = max(end, raw_ptr + raw_size)
    return end


def main():
    here = os.path.dirname(os.path.abspath(__file__))
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("wrapper", nargs="?", default=DEFAULT)
    ap.add_argument("-o", "--out", default=os.path.join(here, "..", "re", "bin", "BejeweledGame.exe"))
    args = ap.parse_args()

    data = open(args.wrapper, "rb").read()
    overlay = pe_end(data, 0)
    if data[overlay:overlay + len(MARKER)] != MARKER:
        sys.exit(f"no {MARKER!r} marker at overlay offset {overlay:#x}; not a PopCap DRM wrapper?")
    start = data.find(b"MZ", overlay)
    if start < 0:
        sys.exit("no embedded PE found after the marker")
    end = start + pe_end(data, start)
    os.makedirs(os.path.dirname(os.path.abspath(args.out)), exist_ok=True)
    with open(args.out, "wb") as f:
        f.write(data[start:end])
    print(f"embedded game PE at {start:#x}-{end:#x} ({end - start} bytes) -> {os.path.abspath(args.out)}")


if __name__ == "__main__":
    main()
