#!/usr/bin/env python3
"""Read TipTop Deluxe's data archive (TipTop.dat).

Every byte of the file, the table included, is stored with its bits permuted (the same fixed permutation
for every byte; see DECODE below). Decoded, the file is:

    u32 table_size                  bytes of the table that follows (the first file starts at 4 + table_size)
    table: entries until an empty name
        char name[]                 NUL-terminated, e.g. "art\\Splash.JPG" (case as packed; lookups ignore case)
        u32 offset                  from the start of the file
        u32 size
    file data, back to back in table order

Usage:
    tiptopdat.py list <TipTop.dat>
    tiptopdat.py extract <TipTop.dat> <out dir> [name ...]     # all files, or only those named
    tiptopdat.py cat <TipTop.dat> <name>                       # one file to stdout
"""
import os
import struct
import sys

# Bit i of a stored byte holds bit SRC[i] of the real byte.
SRC = (2, 6, 0, 7, 5, 4, 1, 3)
ENCODE = bytes(sum(((c >> SRC[i]) & 1) << i for i in range(8)) for c in range(256))
DECODE = bytes(ENCODE.index(c) for c in range(256))


def read_archive(path):
    data = open(path, 'rb').read().translate(DECODE)
    table_size = struct.unpack_from('<I', data, 0)[0]
    entries = []
    p = 4
    while p < 4 + table_size:
        end = data.index(b'\0', p)
        name = data[p:end].decode('latin-1')
        if not name:
            break
        offset, size = struct.unpack_from('<II', data, end + 1)
        entries.append((name, offset, size))
        p = end + 9
    return data, entries


def main(argv):
    if len(argv) < 3 or argv[1] not in ('list', 'extract', 'cat'):
        sys.exit(__doc__)
    data, entries = read_archive(argv[2])
    if argv[1] == 'list':
        for name, offset, size in entries:
            print(f'{offset:8x} {size:8d}  {name}')
        print(f'{len(entries)} files')
    elif argv[1] == 'cat':
        want = argv[3].replace('/', '\\').lower()
        for name, offset, size in entries:
            if name.lower() == want:
                sys.stdout.buffer.write(data[offset:offset + size])
                return
        sys.exit(f'{argv[3]}: not in the archive')
    else:
        out = argv[3]
        want = {n.replace('/', '\\').lower() for n in argv[4:]}
        for name, offset, size in entries:
            if want and name.lower() not in want:
                continue
            path = os.path.join(out, *name.split('\\'))
            os.makedirs(os.path.dirname(path) or out, exist_ok=True)
            with open(path, 'wb') as f:
                f.write(data[offset:offset + size])
        print(f'extracted {len(want) or len(entries)} files to {out}')


if __name__ == '__main__':
    main(sys.argv)
