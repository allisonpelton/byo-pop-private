"""tools/exeprobe.py: whether a game's exe gives up what its port reads from it, as the import checks it (web/gameimport.js,
exeReadable; the table web/exeprobes.js).

    python tools/exeprobe.py <exe> <va> <length> <fnv1a>      prints "ok" or "no" and exits 0 or 1

The game's image is the file itself, or, in PopCap's own launcher, what follows its "!popcapdrmprotect!" or
"!popcapcdprotect!" marker (the clear image appended to it). The probe is one text the port reads: its virtual address in the
image, its length, and the FNV-1a hash of its bytes, which must end in a NUL. Never the text itself. A copy whose game image is
packed or encrypted fails, since the bytes at that address are not the text.
"""
import struct
import sys

MARKERS = (b"!popcapdrmprotect!", b"!popcapcdprotect!")


def image_base(data):
    for m in MARKERS:
        i = data.find(m)
        if i >= 0:
            j = data.find(b"MZ", i + len(m))
            return j if j >= 0 else len(data)
    return 0


def offset(data, base, va):
    if data[base:base + 2] != b"MZ" or len(data) < base + 0x40:
        return None
    pe = base + struct.unpack_from("<I", data, base + 0x3C)[0]
    if data[pe:pe + 4] != b"PE\0\0":
        return None
    nsec, = struct.unpack_from("<H", data, pe + 6)
    opt, = struct.unpack_from("<H", data, pe + 20)
    image, = struct.unpack_from("<I", data, pe + 24 + 28)
    sec = pe + 24 + opt
    for i in range(nsec):
        rva, raw_size, raw_ptr = struct.unpack_from("<III", data, sec + i * 40 + 12)
        if image + rva <= va < image + rva + raw_size:
            return base + raw_ptr + (va - image - rva)
    return None


def fnv1a(b):
    h = 2166136261
    for c in b:
        h = ((h ^ c) * 16777619) & 0xFFFFFFFF
    return h


def check(data, va, length, want):
    at = offset(data, image_base(data), va)
    return at is not None and at + length < len(data) and data[at + length] == 0 and fnv1a(data[at:at + length]) == want


if __name__ == "__main__":
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    data = open(sys.argv[1], "rb").read()
    ok = check(data, int(sys.argv[2], 0), int(sys.argv[3], 0), int(sys.argv[4], 0))
    print("ok" if ok else "no")
    sys.exit(0 if ok else 1)
