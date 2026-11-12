"""Compares the strings of two Bejeweled 2 executables (the Collection check, GAME_NOTES.md "The PopCap Game Collection version"):
the Steam game's image (the carved re/bejeweled2/bin/Bejeweled2Game.exe) and the Collection's Bejeweled2.exe.

    python textcmp.py <steam game image> <collection exe> [--list N]

It lists the strings that only one of the two executables has (runs of printable characters ending in a zero byte, in .rdata
and .data), so a text the Collection's build changed, added or dropped shows. (The port reads no text from the exe, so there is
no table of the game's texts to look up, as Typer Shark's and AstroPop's checks have.) Nothing is written; the output goes to the
log of the run.
"""
import argparse
import re
import struct
import sys



class PE:
    def __init__(self, path):
        self.data = open(path, "rb").read()
        d = self.data
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        n = struct.unpack_from("<H", d, pe + 6)[0]
        opt = struct.unpack_from("<H", d, pe + 20)[0]
        self.base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
        self.sections = []
        s = pe + 24 + opt
        for i in range(n):
            name = d[s:s + 8].rstrip(b"\0").decode("latin1")
            vsize, va, rawsize, rawptr = struct.unpack_from("<IIII", d, s + 8)
            self.sections.append((name, self.base + va, vsize, rawsize, rawptr))
            s += 40

    def offset(self, va):
        for name, sva, vsize, rawsize, rawptr in self.sections:
            if sva <= va < sva + rawsize:
                return rawptr + (va - sva)
        return None

    def cstring(self, va):
        o = self.offset(va)
        if o is None:
            return None
        e = self.data.index(b"\0", o)
        return self.data[o:e]

    def strings(self, minlen=6):
        """Every NUL-terminated run of printable characters in the data sections, as a set of bytes (at least 6 long, with a run
        of 4 letters in it: shorter runs are tables of numbers that happen to be printable)"""
        out = set()
        pat = re.compile(rb"[\x20-\x7e\t\r\n]{%d,}(?=\0)" % minlen)
        for name, sva, vsize, rawsize, rawptr in self.sections:
            if name in (".rdata", ".data"):
                for m in pat.finditer(self.data[rawptr:rawptr + rawsize]):
                    if re.search(rb"[A-Za-z]{4}", m.group()):
                        out.add(m.group())
        return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("steam")
    ap.add_argument("collection")
    ap.add_argument("--list", type=int, default=25)
    args = ap.parse_args()
    a, b = PE(args.steam), PE(args.collection)
    sa, sb = a.strings(), b.strings()
    print("strings: steam", len(sa), "collection", len(sb), "common", len(sa & sb))

    only_a = sorted(sa - sb, key=lambda s: (len(s), s))
    only_b = sorted(sb - sa, key=lambda s: (len(s), s))
    print("only in the steam image:", len(only_a), "only in the collection's exe:", len(only_b))
    for title, lst in (("steam only", only_a), ("collection only", only_b)):
        print("--", title)
        for s in lst[:args.list]:
            print("  ", repr(s[:90]))


if __name__ == "__main__":
    main()
