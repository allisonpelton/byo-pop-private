#!/usr/bin/env python3
"""The auto splitter's block table against the game's (Big Money).

    check_asl_offsets.py <BigMoneyDeluxe-byopop.asl> <games/bigmoney/src/LiveSplitBlock.h> [<bigmoney.exe>]

Checks that every field in the script's vars.Fields has the offset and type the C struct gives it, that the
script's magic, game id and block size are the header's, and (with the exe) that the magic is in the exe once,
in a writable section, followed by the game id.
"""
import re
import struct
import sys

SIZES = {"uint8_t": 1, "uint16_t": 2, "uint32_t": 4, "int32_t": 4, "uint64_t": 8}
TYPES = {"uint16_t": "u16", "uint32_t": "u32", "int32_t": "i32", "uint64_t": "u64"}


def struct_fields(header):
    body = re.search(r"struct LiveSplitBlockV1\s*\{(.*?)\};", header, re.S).group(1)
    fields, offset = {}, 0
    for m in re.finditer(r"(u?int\d+_t)\s+(\w+)(?:\[(\w+)\])?;", body):
        ctype, name, count = m.group(1), m.group(2), m.group(3)
        n = 1
        if count:
            n = 20 if count == "LIVESPLIT_MAGIC_SIZE" else int(count)
        size = SIZES[ctype]
        offset = (offset + size - 1) // size * size
        fields[name] = (offset, TYPES.get(ctype, "bytes") if n == 1 else "bytes")
        offset += size * n
    return fields, offset


def main(argv):
    if len(argv) < 3:
        print(__doc__)
        return 2
    asl = open(argv[1], encoding="utf-8").read()
    header = open(argv[2], encoding="utf-8").read()
    fields, size = struct_fields(header)
    errors = 0
    table = re.search(r"vars\.Fields = new string\[\] \{(.*?)\};", asl, re.S).group(1)
    for name, off, typ in re.findall(r'"(\w+) (0x[0-9A-Fa-f]+) (\w+)"', table):
        if name not in fields:
            print("script field %s: not in the struct" % name)
            errors += 1
        elif fields[name] != (int(off, 16), typ):
            print("script field %s: %s %s, struct %s %s" % (name, off, typ, hex(fields[name][0]), fields[name][1]))
            errors += 1
    magic_h = [int(x, 16) for x in re.search(r"LIVESPLIT_MAGIC_BYTES \{(.*?)\}", header, re.S).group(1).replace("\\", "").split(",")]
    magic_s = [int(x, 16) for x in re.search(r"vars\.Magic = new byte\[\] \{(.*?)\};", asl).group(1).split(",")]
    if magic_h != magic_s:
        print("magic differs")
        errors += 1
    gid_h = int(re.search(r"LIVESPLIT_GAME_ID = (0x[0-9A-Fa-f]+)u", header).group(1), 16)
    gid_s = int(re.search(r"vars\.GameId = (0x[0-9A-Fa-f]+)L", asl).group(1), 16)
    size_s = int(re.search(r"vars\.BlockSize = (0x[0-9A-Fa-f]+);", asl).group(1), 16)
    if gid_h != gid_s or size_s != size:
        print("game id or size differs: header %#x/%#x, script %#x/%#x" % (gid_h, size, gid_s, size_s))
        errors += 1
    print("%d script fields checked against the struct (%d bytes)" % (len(re.findall(r'"\w+ 0x', table)), size))
    if len(argv) > 3:
        data = open(argv[3], "rb").read()
        magic = bytes(magic_h)
        hits = [i for i in range(len(data)) if data.startswith(magic, i)]
        pe = struct.unpack_from("<I", data, 0x3C)[0]
        nsec, opt = struct.unpack_from("<H", data, pe + 6)[0], struct.unpack_from("<H", data, pe + 20)[0]
        where = []
        for h in hits:
            for i in range(nsec):
                off = pe + 24 + opt + 40 * i
                name = data[off:off + 8].rstrip(b"\0").decode()
                rsize, rptr = struct.unpack_from("<II", data, off + 16)
                chars = struct.unpack_from("<I", data, off + 36)[0]
                if rptr <= h < rptr + rsize:
                    where.append((name, bool(chars & 0x80000000), struct.unpack_from("<I", data, h + 20)[0] == gid_h))
        print("magic in the exe: %s" % where)
        if len(hits) != 1 or not where or not where[0][1] or not where[0][2]:
            print("the magic must be in the exe once, in a writable section, followed by the game id")
            errors += 1
    print("ok" if not errors else "%d errors" % errors)
    return 1 if errors else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
