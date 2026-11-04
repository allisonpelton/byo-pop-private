#!/usr/bin/env python3
"""Turn a PopCap 2010 launcher's encrypted game image (`<Game>.dat`) into the clear game exe.

The 2010 DRM launcher (`BananaBugs.exe` and its kin) keeps the real game as `<Game>.dat` next to it
and unwraps it to `popcapgame1.exe` in a temp folder before running it.  The file is:

    0x00  u32    must be 0
    0x04  u8[32] SHA-256 of the clear game exe
    0x24  u8[32] AES-256 key
    0x44  ...    AES-256-CBC ciphertext: a 16-byte IV, then the blocks, the last one partial
                 (ciphertext stealing, the launcher's own variant: see decrypt_cbc_cts)

The plaintext is a container:

    0x00  'PCWE'
    0x04  u32    size of the clear exe
    0x08  u8[32] its SHA-256 again
    0x28  u32    chunk count
    0x2c  chunks: u8 flag (0), u16 unused, u32 offset, u32 length, then `length` bytes

The chunks come shuffled; sorted by offset they tile the exe without gaps.  Both hashes are checked.
Everything needed is in the file, so this works offline on any installed copy.

Usage: extract_popcap2010.py <Game.dat> <out.exe>
"""
import hashlib
import struct
import sys

# ---- AES-256, decrypt only (FIPS-197, equivalent inverse cipher with T-tables) ----

def _xt(a):
    return ((a << 1) ^ 0x11b) & 0xff if a & 0x80 else a << 1


def _mul(a, b):
    r = 0
    while b:
        if b & 1:
            r ^= a
        a = _xt(a)
        b >>= 1
    return r


SBOX = [0] * 256
INV = [0] * 256
_p = _q = 1
while True:
    _p ^= _xt(_p)
    _p &= 0xff
    _q ^= _q << 1
    _q ^= _q << 2
    _q ^= _q << 4
    _q &= 0xff
    if _q & 0x80:
        _q ^= 0x09
    _x = _q ^ (_q << 1) ^ (_q << 2) ^ (_q << 3) ^ (_q << 4)
    _x = (_x ^ (_x >> 8) ^ 0x63) & 0xff
    SBOX[_p] = _x
    INV[_x] = _p
    if _p == 1:
        break
SBOX[0] = 0x63
INV[0x63] = 0


def _ror8(w):
    return ((w >> 8) | (w << 24)) & 0xffffffff


TD0 = [(_mul(INV[x], 14) << 24) | (_mul(INV[x], 9) << 16) | (_mul(INV[x], 13) << 8) | _mul(INV[x], 11)
       for x in range(256)]
TD1 = [_ror8(w) for w in TD0]
TD2 = [_ror8(w) for w in TD1]
TD3 = [_ror8(w) for w in TD2]
RCON = [1, 2, 4, 8, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36]


def _inv_mix(w):
    return (TD0[SBOX[w >> 24]] ^ TD1[SBOX[(w >> 16) & 0xff]] ^ TD2[SBOX[(w >> 8) & 0xff]]
            ^ TD3[SBOX[w & 0xff]])


def expand_decrypt_key(key):
    """Round keys for the equivalent inverse cipher, as 4 big-endian words each."""
    nk = len(key) // 4
    nr = nk + 6
    w = list(struct.unpack('>%dI' % nk, key))
    for i in range(nk, 4 * (nr + 1)):
        t = w[i - 1]
        if i % nk == 0:
            t = ((t << 8) | (t >> 24)) & 0xffffffff
            t = ((SBOX[t >> 24] << 24) | (SBOX[(t >> 16) & 0xff] << 16) | (SBOX[(t >> 8) & 0xff] << 8)
                 | SBOX[t & 0xff]) ^ (RCON[i // nk - 1] << 24)
        elif nk > 6 and i % nk == 4:
            t = ((SBOX[t >> 24] << 24) | (SBOX[(t >> 16) & 0xff] << 16) | (SBOX[(t >> 8) & 0xff] << 8)
                 | SBOX[t & 0xff])
        w.append(w[i - nk] ^ t)
    rks = [w[4 * r:4 * r + 4] for r in range(nr + 1)]
    for r in range(1, nr):
        rks[r] = [_inv_mix(x) for x in rks[r]]
    return rks, nr


def decrypt_block(rks, nr, block):
    s0, s1, s2, s3 = struct.unpack('>4I', block)
    k = rks[nr]
    s0 ^= k[0]
    s1 ^= k[1]
    s2 ^= k[2]
    s3 ^= k[3]
    for r in range(nr - 1, 0, -1):
        k = rks[r]
        t0 = TD0[s0 >> 24] ^ TD1[(s3 >> 16) & 0xff] ^ TD2[(s2 >> 8) & 0xff] ^ TD3[s1 & 0xff] ^ k[0]
        t1 = TD0[s1 >> 24] ^ TD1[(s0 >> 16) & 0xff] ^ TD2[(s3 >> 8) & 0xff] ^ TD3[s2 & 0xff] ^ k[1]
        t2 = TD0[s2 >> 24] ^ TD1[(s1 >> 16) & 0xff] ^ TD2[(s0 >> 8) & 0xff] ^ TD3[s3 & 0xff] ^ k[2]
        t3 = TD0[s3 >> 24] ^ TD1[(s2 >> 16) & 0xff] ^ TD2[(s1 >> 8) & 0xff] ^ TD3[s0 & 0xff] ^ k[3]
        s0, s1, s2, s3 = t0, t1, t2, t3
    k = rks[0]
    out = []
    for a, b, c, d, kk in ((s0, s3, s2, s1, k[0]), (s1, s0, s3, s2, k[1]), (s2, s1, s0, s3, k[2]),
                           (s3, s2, s1, s0, k[3])):
        out.append(((INV[a >> 24] << 24) | (INV[(b >> 16) & 0xff] << 16) | (INV[(c >> 8) & 0xff] << 8)
                    | INV[d & 0xff]) ^ kk)
    return struct.pack('>4I', *out)


def _selftest():
    rks, nr = expand_decrypt_key(bytes(range(32)))
    pt = decrypt_block(rks, nr, bytes.fromhex('8ea2b7ca516745bfeafc49904b496089'))
    assert pt == bytes.fromhex('00112233445566778899aabbccddeeff'), 'AES self-test failed'


def _xor(a, b):
    return bytes(x ^ y for x, y in zip(a, b))


def decrypt_cbc_cts(key, data):
    """CBC with the launcher's ciphertext stealing.  data = IV + blocks [+ partial block].

    For a final partial block P of n bytes after the full block C: the plaintext of C's slot is
    D(P padded with the tail of D(C)) ^ C_prev, and the last n plaintext bytes are D(C)[:n] ^ P.
    The result is 16 bytes shorter than the input (the IV)."""
    rks, nr = expand_decrypt_key(key)
    out = bytearray()
    prev = data[:16]
    cur = data[16:32]
    pos = 32
    while True:
        nxt = data[pos:pos + 16]
        n = len(nxt)
        pos += n
        tmp = decrypt_block(rks, nr, cur)
        if n in (0, 16):
            out += _xor(tmp, prev)
            prev, cur = cur, nxt
            if n == 0:
                break
        else:
            tail = _xor(tmp[:n], nxt) + tmp[n:]
            out += _xor(decrypt_block(rks, nr, nxt + tmp[n:]), prev)
            out += tail[:n]
            break
    return bytes(out)


def extract(src, dst):
    data = open(src, 'rb').read()
    if len(data) < 0x44 + 32:
        raise SystemExit('too small to be a launcher image')
    flag, = struct.unpack_from('<I', data, 0)
    if flag != 0:
        raise SystemExit('header dword 0 is %#x, expected 0' % flag)
    file_hash = data[4:0x24]
    key = data[0x24:0x44]
    clear = decrypt_cbc_cts(key, data[0x44:])
    if clear[:4] != b'PCWE':
        raise SystemExit('decryption gave no PCWE container (magic %r); wrong key or scheme' % clear[:4])
    size, = struct.unpack_from('<I', clear, 4)
    inner_hash = clear[8:0x28]
    count, = struct.unpack_from('<I', clear, 0x28)
    if inner_hash != file_hash:
        raise SystemExit('the container hash differs from the file header hash')
    chunks = []
    pos = 0x2c
    for i in range(count):
        if pos + 11 > len(clear):
            raise SystemExit('chunk %d header runs past the end' % i)
        cflag = clear[pos]
        off, length = struct.unpack_from('<II', clear, pos + 3)
        pos += 11
        if cflag != 0:
            raise SystemExit('chunk %d has flag %d, expected 0' % (i, cflag))
        if off + length > size or pos + length > len(clear):
            raise SystemExit('chunk %d: offset %#x length %#x out of range' % (i, off, length))
        chunks.append((off, length, pos))
        pos += length
    # The chunks are shuffled; the launcher sorts them by offset (a std::set) and expects them to
    # tile the exe without gaps.
    chunks.sort()
    exe = bytearray()
    for off, length, start in chunks:
        if off != len(exe):
            raise SystemExit('chunk at offset %#x does not follow the previous one (%#x)' % (off, len(exe)))
        exe += clear[start:start + length]
    if len(exe) != size:
        raise SystemExit('chunks sum to %d bytes, container says %d' % (len(exe), size))
    if hashlib.sha256(exe).digest() != file_hash:
        raise SystemExit('SHA-256 of the extracted exe does not match the header')
    open(dst, 'wb').write(exe)
    print('%s: %d bytes in %d chunk(s), SHA-256 verified -> %s' % (src, size, count, dst))


if __name__ == '__main__':
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    _selftest()
    extract(sys.argv[1], sys.argv[2])
