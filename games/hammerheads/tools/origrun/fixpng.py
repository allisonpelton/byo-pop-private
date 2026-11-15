#!/usr/bin/env python3
"""fixpng.py <png> [...]: rewrites the PNG files that the original's PNG loader cannot read on this Windows, with the same pixels.

Two faults of the files, both met when the game's loading thread runs under origrun:
  - A 4-bit palette file (images/gnomefemale_.png is the only one): the loader sizes its destination buffer from the file's
    row size before libpng expands the rows to 32 bits, so the buffer is half what the rows need and the loading thread
    crashes in libpng at the middle row. The file is written as an 8-bit palette PNG with the same pixels and palette.
  - A file whose first chunk is a tEXt (images/gui/tinyhammer2.png and tough_cookie_mini.png): libpng stops with "Missing IHDR
    before tEXt" and the game says "Failed to load image". The header chunk is moved to the front.
A copy of the game that is run under origrun gets its files fixed (never an install); the decoded pixels, and so the game,
are the same. Files that need neither are left alone."""
import struct
import sys
import zlib


def chunks(data):
    pos = 8
    while pos < len(data):
        length, kind = struct.unpack('>I4s', data[pos:pos + 8])
        yield kind, data[pos + 8:pos + 8 + length]
        pos += 12 + length


def chunk(kind, body):
    return struct.pack('>I', len(body)) + kind + body + struct.pack('>I', zlib.crc32(kind + body) & 0xffffffff)


def unfilter(raw, width_bytes, height, bpp=1):
    rows, prev = [], bytearray(width_bytes)
    pos = 0
    for _ in range(height):
        ft = raw[pos]
        line = bytearray(raw[pos + 1:pos + 1 + width_bytes])
        pos += 1 + width_bytes
        for i in range(width_bytes):
            a = line[i - bpp] if i >= bpp else 0
            b = prev[i]
            c = prev[i - bpp] if i >= bpp else 0
            if ft == 1:
                line[i] = (line[i] + a) & 255
            elif ft == 2:
                line[i] = (line[i] + b) & 255
            elif ft == 3:
                line[i] = (line[i] + ((a + b) >> 1)) & 255
            elif ft == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pr = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[i] = (line[i] + pr) & 255
        rows.append(line)
        prev = line
    return rows


def convert(path):
    data = open(path, 'rb').read()
    pieces = list(chunks(data))
    changed = False
    if pieces[0][0] != b'IHDR':
        header = [piece for piece in pieces if piece[0] == b'IHDR'][0]
        pieces = [header] + [piece for piece in pieces if piece is not header]
        changed = True
        print('%s: the header chunk moved to the front' % path)
    width, height, depth, ctype, comp, filt, interlace = struct.unpack('>IIBBBBB', pieces[0][1])
    if depth == 4 and ctype == 3 and interlace == 0:
        idat = b''.join(body for kind, body in pieces if kind == b'IDAT')
        rows = unfilter(zlib.decompress(idat), (width + 1) // 2, height)
        raw = bytearray()
        for line in rows:
            pixels = bytearray()
            for byte in line:
                pixels.append(byte >> 4)
                pixels.append(byte & 15)
            raw += bytes(1) + pixels[:width]
        rebuilt = []
        done = False
        for kind, body in pieces:
            if kind == b'IHDR':
                rebuilt.append((kind, struct.pack('>IIBBBBB', width, height, 8, 3, comp, filt, 0)))
            elif kind == b'IDAT':
                if not done:
                    rebuilt.append((kind, zlib.compress(bytes(raw), 9)))
                    done = True
            else:
                rebuilt.append((kind, body))
        pieces = rebuilt
        changed = True
        print('%s: %dx%d, 4-bit palette -> 8-bit palette' % (path, width, height))
    if changed:
        open(path, 'wb').write(data[:8] + b''.join(chunk(kind, body) for kind, body in pieces))


for name in sys.argv[1:]:
    convert(name)
