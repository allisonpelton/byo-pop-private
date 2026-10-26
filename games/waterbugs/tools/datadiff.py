#!/usr/bin/env python3
"""For the function pairs that tools/funcmatch.py matched between Water Bugs and Cosmic Bugs, list the pairs whose
code is identical (apart from addresses) but that read different data: funcmatch masks every address in a function,
so a constant, a table entry or a string that moved and changed with the same code still matches.

    python games/waterbugs/tools/datadiff.py re/waterbugs/match.jsonl re/waterbugs/bin/WaterBugs.exe \
        assets/cosmicbugs/CosmicBugs.exe re/waterbugs/gh/functions.jsonl re/cosmicbugs/gh/functions.jsonl

Each address operand that points into .rdata/.data in both builds is compared: 8 bytes there (a float, a double or
an int), or the C string when both start like text. Output is one line per differing pair, with the operand offsets.
Reads only: it prints addresses and the kind of difference, never the strings' contents.
"""
import json
import struct
import sys


class Image:
    def __init__(self, path, functions):
        d = self.d = open(path, "rb").read()
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        optsz = struct.unpack_from("<H", d, pe + 20)[0]
        self.base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
        self.size = struct.unpack_from("<I", d, pe + 24 + 56)[0]
        self.secs = []
        off = pe + 24 + optsz
        for _ in range(nsec):
            name = d[off:off + 8].rstrip(b"\0").decode()
            vsz, va, rsz, rp = struct.unpack_from("<IIII", d, off + 8)
            self.secs.append((name, self.base + va, vsz, rp, rsz))
            off += 40
        self.sizes = {}
        for line in open(functions, encoding="utf-8"):
            f = json.loads(line)
            self.sizes[int(f["addr"], 16)] = f.get("size") or 0
        self.starts = sorted(self.sizes)

    def section(self, va):
        for name, sva, vsz, rp, rsz in self.secs:
            if sva <= va < sva + vsz:
                return name
        return None

    def raw(self, va, n):
        for name, sva, vsz, rp, rsz in self.secs:
            if sva <= va < sva + rsz:
                return self.d[rp + va - sva:rp + va - sva + n]
        return b""

    def body(self, a):
        n = self.sizes[a]
        i = self.starts.index(a) + 1
        if i < len(self.starts):
            n = min(n, self.starts[i] - a) if n else self.starts[i] - a
        b = bytearray(self.raw(a, n))
        while b and b[-1] in (0xCC, 0x90):
            b.pop()
        return bytes(b)

    def masked(self, a):
        """The body with every address and call target zeroed (as tools/funcmatch.py does), and the address
        operands that point into the image (offset, value)"""
        b = bytearray(self.body(a))
        ops = []
        i = 0
        while i + 4 <= len(b):
            v = struct.unpack_from("<I", b, i)[0]
            if i > 0 and b[i - 1] in (0xE8, 0xE9):
                b[i:i + 4] = b"\x00\x00\x00\x00"
                i += 4
            elif self.base <= v < self.base + self.size:
                ops.append((i, v))
                b[i:i + 4] = b"\x00\x00\x00\x00"
                i += 4
            else:
                i += 1
        return bytes(b), ops

    def norm(self, r):
        """8 bytes with a dword that is an address of the image (a pointer in a table) replaced"""
        out = []
        for i in range(0, len(r) - 3, 4):
            v = struct.unpack_from("<I", r, i)[0]
            out.append("A" if self.base <= v < self.base + self.size else v)
        return out

    def cstr(self, va):
        b = self.raw(va, 64)
        n = b.find(b"\0")
        return b[:n] if n > 0 else None


def texty(s):
    return s is not None and len(s) >= 2 and all(32 <= c < 127 or c in (9, 10, 13) for c in s)


def main():
    match, wexe, cexe, wfun, cfun = sys.argv[1:6]
    W = Image(wexe, wfun)
    C = Image(cexe, cfun)
    diffs = 0
    pairs = 0
    for line in open(match, encoding="utf-8"):
        m = json.loads(line)
        a, b = int(m["a"], 16), int(m["b"], 16)
        if a not in W.sizes or b not in C.sizes or W.sizes[a] == 0:
            continue
        pairs += 1
        wb, wops = W.masked(a)
        cb, cops = C.masked(b)
        if wb != cb:
            print(f"{a:08x} {b:08x} {W.sizes[a]:5d} CODE differs ({m['how']})")
            diffs += 1
            continue
        out = []
        for (i, wv), (_, cv) in zip(wops, cops):
            ws, cs = W.section(wv), C.section(cv)
            if ws != cs:
                out.append(f"+{i:x} section {ws}/{cs}")
            elif ws in (".rdata", ".data"):
                s1, s2 = W.cstr(wv), C.cstr(cv)
                if texty(s1) and texty(s2):
                    if s1 != s2:
                        out.append(f"+{i:x} str {wv:08x}/{cv:08x}")
                else:
                    r1, r2 = W.raw(wv, 8), C.raw(cv, 8)
                    if wv < 0x458280 or cv < 0x458280:
                        continue  # the import table: its entries are RVAs of the DLLs' names
                    if W.norm(r1) != C.norm(r2):
                        out.append(f"+{i:x} data {wv:08x} {r1.hex()} / {cv:08x} {r2.hex()}")
        if out:
            diffs += 1
            print(f"{a:08x} {b:08x} {W.sizes[a]:5d} " + "; ".join(out[:6]) + (f" (+{len(out) - 6})" if len(out) > 6 else ""))
    print(f"{pairs} pairs, {diffs} differ")


if __name__ == "__main__":
    main()
