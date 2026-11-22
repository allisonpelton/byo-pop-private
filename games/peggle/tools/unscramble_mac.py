#!/usr/bin/env python3
"""Write a readable copy of Peggle Deluxe's Mac binary, for tools/macmatch.py and for reading its code.

    python -I games/peggle/tools/unscramble_mac.py [re/peggle/deluxe/mac/Peggle] [--out re/peggle/deluxe/mac/Peggle.dec]

The Mac executable's code section is not machine code as it stands: its bytes are scrambled so that a disassembler
reads garbage (the symbol table, the strings, the vtables and the unwind data are plain, so a name list and a vtable
match still work on it). This reads the Mach-O header, finds the `__TEXT,__text` section, undoes the scrambling in
place on a copy, and checks that the result starts like the compiler's own start-up code (a frame set up with push ebp
and mov ebp,esp). The copy holds the user's game code: it stays under the git-ignored `re/` folder, is never committed
or published, and the original file is only read.

<!-- private -->
How: the section's bytes are XORed with a four-byte key that repeats from the section's first byte
(`9c f7 20 75`, found by the index of coincidence of each byte column against the PC exe's own code, and confirmed by the
first function decoding as a standard prologue). Nothing else in the file is touched.
<!-- /private -->
"""
import argparse
import os
import struct
import sys

KEY = bytes((0x9C, 0xF7, 0x20, 0x75))
MAGIC_32_LE = 0xFEEDFACE
LC_SEGMENT = 0x1
START_UP_CODE = bytes((0x55, 0x89, 0xE5))


def find_text_section(data):
    """(file offset, size) of __TEXT,__text in a thin 32-bit little-endian Mach-O file."""
    magic, _cpu, _sub, _type, command_count = struct.unpack_from("<5I", data, 0)
    if magic != MAGIC_32_LE:
        raise ValueError("not a thin 32-bit little-endian Mach-O file (magic %#x)" % magic)
    offset = 28
    for _ in range(command_count):
        command, size = struct.unpack_from("<II", data, offset)
        if command == LC_SEGMENT:
            segment_name = data[offset + 8:offset + 24].rstrip(b"\0")
            section_count = struct.unpack_from("<I", data, offset + 48)[0]
            section_offset = offset + 56
            for _ in range(section_count):
                section_name = data[section_offset:section_offset + 16].rstrip(b"\0")
                _address, section_size, file_offset = struct.unpack_from("<3I", data, section_offset + 32)
                if segment_name == b"__TEXT" and section_name == b"__text":
                    return file_offset, section_size
                section_offset += 68
        offset += size
    raise ValueError("no __TEXT,__text section")


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("binary", nargs="?", default="re/peggle/deluxe/mac/Peggle")
    parser.add_argument("--out", default=None)
    args = parser.parse_args()
    out = args.out or args.binary + ".dec"
    with open(args.binary, "rb") as handle:
        data = bytearray(handle.read())
    start, size = find_text_section(data)
    for index in range(size):
        data[start + index] ^= KEY[index & 3]
    # The first function of the section is the compiler's start-up check: frame setup.
    if bytes(data[start:start + 3]) != START_UP_CODE:
        sys.exit("the section does not start with a frame setup after unscrambling; wrong binary or key")
    with open(out, "wb") as handle:
        handle.write(data)
    print("wrote %s (%d bytes of code at file offset %#x)" % (out, size, start))


if __name__ == "__main__":
    main()
