"""Write a hiscores.dat for Noah's Ark (the original's layout, 0x411ea0 reads it and 0x412060 writes it): per record
[u16 length][name][i32 score][i32 check], the check being score ^ 0x42beef xor each name byte (a signed char) times 0x11.

    python games/noahsark/tools/make_hiscores.py <out file> [--cut <bytes>] <name>:<score>[:bad] ...

The name may contain \0 (a NUL: the game takes the name up to it, the check still covers every byte); ":bad" gives the
record a check that does not match. --cut writes only the first <bytes> bytes of the file (a truncated file).
"""
import struct
import sys


def record(name, score, bad):
    raw = name.encode('latin-1').replace(b'\0', b'\0')
    check = (score & 0xffffffff) ^ 0x42beef
    for c in raw:
        check ^= ((c - 256 if c > 127 else c) * 0x11) & 0xffffffff
    if bad:
        check ^= 1
    return struct.pack('<H', len(raw)) + raw + struct.pack('<i', score) + struct.pack('<I', check & 0xffffffff)


def main(argv):
    cut = None
    args = []
    i = 1
    while i < len(argv):
        if argv[i] == '--cut':
            cut = int(argv[i + 1])
            i += 2
        else:
            args.append(argv[i])
            i += 1
    out = args[0]
    data = b''
    for spec in args[1:]:
        parts = spec.split(':')
        data += record(parts[0], int(parts[1]), len(parts) > 2 and parts[2] == 'bad')
    if cut is not None:
        data = data[:cut]
    open(out, 'wb').write(data)


main(sys.argv)
