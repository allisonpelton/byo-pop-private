"""Compares two Word Harmony executables (the Collection check, README.md "The PopCap Game Collection version"): the link
time and checksum, the header bytes that differ, each section's size and the number of bytes that differ (the .rdata ones listed),
every resource (type, name, language) present in one and not in the other or different, and what follows the last section.

    python cmpexe.py <a.exe> <b.exe>

a is the game carved from the retail copy's WordHarmony.exe (re/wordharmony/bin/WordHarmonyGame.exe), b the Collection's
WordHarmony.exe.
"""
import struct
import sys
import time


def load(path):
    data = open(path, 'rb').read()
    return data, struct.unpack_from('<I', data, 0x3c)[0]


def sections(data, pe):
    count = struct.unpack_from('<H', data, pe + 6)[0]
    opt = struct.unpack_from('<H', data, pe + 20)[0]
    table = pe + 24 + opt
    out = []
    for i in range(count):
        name, vsize, va, rsize, raw = struct.unpack_from('<8sIIII', data, table + 40 * i)
        out.append((name.rstrip(b'\0').decode(), va, vsize, raw, rsize))
    return out


def rva_to_offset(secs, rva):
    for _, va, vsize, raw, rsize in secs:
        if va <= rva < va + max(vsize, rsize):
            return raw + rva - va
    return None


def resources(data, secs, root_rva):
    """{(type, name, language): bytes} from the resource directory."""
    base = rva_to_offset(secs, root_rva)
    out = {}

    def entries(offset):
        named, ids = struct.unpack_from('<HH', data, offset + 12)
        return [struct.unpack_from('<II', data, offset + 16 + 8 * i) for i in range(named + ids)]

    for kind, kind_off in entries(base):
        for name, name_off in entries(base + (kind_off & 0x7fffffff)):
            for lang, leaf in entries(base + (name_off & 0x7fffffff)):
                rva, size, _, _ = struct.unpack_from('<IIII', data, base + leaf)
                start = rva_to_offset(secs, rva)
                out[(kind, name, lang)] = data[start:start + size]
    return out


def main():
    a, pa = load(sys.argv[1])
    b, pb = load(sys.argv[2])
    print('sizes', len(a), len(b))
    for label, data, pe in (('a', a, pa), ('b', b, pb)):
        stamp = struct.unpack_from('<I', data, pe + 8)[0]
        print(label, 'link time', time.strftime('%Y-%m-%d %H:%M:%S', time.gmtime(stamp)), 'checksum 0x%x' % struct.unpack_from('<I', data, pe + 24 + 64)[0])
    print('header bytes that differ, at file offsets:', [hex(i) for i in range(0x400) if a[i] != b[i]])
    sa, sb = sections(a, pa), sections(b, pb)
    for (name, va, vsize, raw, rsize), other in zip(sa, sb):
        da, db = a[raw:raw + rsize], b[other[3]:other[3] + other[4]]
        diff = [i for i in range(min(len(da), len(db))) if da[i] != db[i]]
        print('%-8s raw %d / %d, %d bytes differ' % (name, rsize, other[4], len(diff) + abs(len(da) - len(db))))
        if name == '.rdata':
            for i in diff:
                print('  va 0x%x: 0x%02x -> 0x%02x' % (0x400000 + va + i, da[i], db[i]))
    ra = struct.unpack_from('<I', a, pa + 24 + 96 + 16)[0]
    rb = struct.unpack_from('<I', b, pb + 24 + 96 + 16)[0]
    resa, resb = resources(a, sa, ra), resources(b, sb, rb)
    print('resources:', len(resa), 'in a,', len(resb), 'in b,', sum(1 for k in resa if k in resb and resa[k] == resb[k]), 'the same')
    for k in sorted(set(resa) | set(resb), key=str):
        if k not in resb:
            print('  only in a:', k, len(resa[k]))
        elif k not in resa:
            print('  only in b:', k, len(resb[k]))
        elif resa[k] != resb[k]:
            print('  differ:', k, len(resa[k]), len(resb[k]))
    enda, endb = max(s[3] + s[4] for s in sa), max(s[3] + s[4] for s in sb)
    print('after the last section: %d and %d bytes' % (len(a) - enda, len(b) - endb))


main()
