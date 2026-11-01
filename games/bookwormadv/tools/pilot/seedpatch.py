"""seedpatch.py <staged exe> [seed]: fix the original's random seeds in a staged copy (never the install).

SexyAppBase::Init (0x54bff0) seeds the app's Mersenne Twister with the tick count (mRandSeed = GetTickCount();
SRand(mRandSeed), at 0x54c430) and then the C library's rand() with another tick count (srand(GetTickCount()), at
0x54c4bc). The patch replaces both clock reads with constants: SRand gets <seed> (default 12345), as the port's
-seed=<seed> does, and srand gets 1, where the port's math.random generator (poplua/lmathlib.cpp) always starts.
The expected bytes are checked first; the patch is logged to stdout.
"""
import struct
import sys

def file_offset(data, va):
    pe = struct.unpack_from("<I", data, 0x3c)[0]
    nsec = struct.unpack_from("<H", data, pe + 6)[0]
    opt = struct.unpack_from("<H", data, pe + 20)[0]
    base = struct.unpack_from("<I", data, pe + 24 + 28)[0]
    sec = pe + 24 + opt
    for i in range(nsec):
        vsize, vaddr, rsize, raw = struct.unpack_from("<IIII", data, sec + i * 40 + 8)
        if base + vaddr <= va < base + vaddr + max(vsize, rsize):
            return va - base - vaddr + raw
    sys.exit("0x%x is in no section" % va)


def main():
    path = sys.argv[1]
    seed = int(sys.argv[2]) if len(sys.argv) > 2 else 12345
    data = bytearray(open(path, "rb").read())
    patches = [
        # mov edi, [GetTickCount]; call edi  ->  mov eax, <seed>; nop x3 (then mRandSeed = eax; SRand)
        (0x54c430, bytes.fromhex("8b3dc8707900ffd7"), b"\xb8" + struct.pack("<I", seed) + b"\x90\x90\x90"),
        # call edi; push eax  ->  push 1; nop (srand's argument)
        (0x54c4bc, bytes.fromhex("ffd750"), bytes.fromhex("6a0190")),
    ]
    for va, old, new in patches:
        off = file_offset(data, va)
        cur = bytes(data[off:off + len(old)])
        if cur == new:
            print("seedpatch: 0x%x already patched" % va)
            continue
        if cur != old:
            sys.exit("seedpatch: 0x%x holds %s, expected %s: not patched" % (va, cur.hex(), old.hex()))
        data[off:off + len(old)] = new
        print("seedpatch: 0x%x (file 0x%x) %s -> %s" % (va, off, old.hex(), new.hex()))
    open(path, "wb").write(data)


main()
