#!/usr/bin/env python3
"""Recover C++ classes and vtables from MSVC RTTI, including VS.NET 2002 (MSVC 7.0) binaries.

Ghidra 12's RTTI analyzer recovers no classes from MSVC 7.0 images: their BaseClassDescriptors are 24
bytes, without the pClassDescriptor field, and Ghidra rejects them (docs/POPCAP_GAMES_SURVEY.md 3.4, "Findings
that apply to many games"). This scanner only follows CompleteObjectLocator -> TypeDescriptor / ClassHierarchyDescriptor, which is
the same in every MSVC version, and writes:

  <out>.jsonl        ApplySymbols.java lines: Class::vfNN names for virtual functions, vftable labels, and
                     (with --functions) Class::ctor_or_dtor_<addr> for functions that store a vftable
  <out>.classes.json {class: {"bases": [...], "vtables": [{"va", "offset", "slots": [...]}]}}

    python tools/rtti_msvc70.py <game.exe> <out> [--functions functions.jsonl]

The --functions pass needs Ghidra's function list (ExportAll's functions.jsonl), so the pipeline runs
it after the first export (tools/ghidra_import.sh does both passes when RTTI_SCAN=1).
"""
import argparse
import bisect
import collections
import json
import re
import struct


class PE:
    def __init__(self, path):
        self.d = d = open(path, "rb").read()
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        nsec = struct.unpack_from("<H", d, pe + 6)[0]
        optsz = struct.unpack_from("<H", d, pe + 20)[0]
        self.base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
        self.secs = []
        off = pe + 24 + optsz
        for _ in range(nsec):
            name = d[off:off + 8].rstrip(b"\0").decode("latin-1")
            vsz, va, rsz, rp = struct.unpack_from("<IIII", d, off + 8)
            self.secs.append((name, self.base + va, vsz, rp, rsz))
            off += 40

    def sec(self, name):
        return next(s for s in self.secs if s[0] == name)

    def v2o(self, va):
        for _, s, _, rp, rs in self.secs:
            if s <= va < s + rs:
                return rp + va - s
        return None

    def o2v(self, o):
        for _, s, _, rp, rs in self.secs:
            if rp <= o < rp + rs:
                return s + o - rp
        return None

    def u32(self, va):
        o = self.v2o(va)
        return None if o is None or o + 4 > len(self.d) else struct.unpack_from("<I", self.d, o)[0]


def demangle_type(raw):
    """'Board@Sexy' -> 'Sexy::Board'. Templates ('?$...') are kept mangled (returned as None)."""
    if raw.startswith("?$") or "?$" in raw:
        return None
    return "::".join(reversed([p for p in raw.split("@") if p]))


def scan(pe):
    d = pe.d
    text = pe.sec(".text")
    tlo, thi = text[1], text[1] + text[4]
    # TypeDescriptors: pVFTable, spare, ".?AV<name>@@\0"
    tds = {}
    for m in re.finditer(rb"\.\?A[VU]([A-Za-z0-9_@?$<>]+?)@@\0", d):
        va = pe.o2v(m.start() - 8)
        if va is not None:
            tds[va] = m.group(1).decode("latin-1")
    # CompleteObjectLocators: signature 0, offset, cdOffset, pTypeDescriptor, pClassHierarchyDescriptor
    cols = {}
    for name, s, _, rp, rs in pe.secs:
        if name not in (".rdata", ".data"):
            continue
        for o in range(rp, rp + rs - 20, 4):
            sig, offs, cdo, td, chd = struct.unpack_from("<IIIII", d, o)
            if sig == 0 and td in tds and offs < 0x10000 and cdo < 0x10000 and pe.v2o(chd) is not None:
                cols[pe.o2v(o)] = (tds[td], offs, chd)
    # Base class lists from the ClassHierarchyDescriptor (first entry is the class itself)
    bases = {}
    for col, (n, offs, chd) in cols.items():
        _, attr, nb, pba = struct.unpack_from("<IIII", d, pe.v2o(chd))
        bl = []
        for i in range(min(nb, 64)):
            bcd = pe.u32(pba + 4 * i)
            bl.append(tds.get(pe.u32(bcd) if bcd else None, "?"))
        bases[n] = bl
    # vftables: a pointer to a COL followed by code pointers
    vts = []
    rd = pe.sec(".rdata")
    for o in range(rd[3], rd[3] + rd[4] - 4, 4):
        x = struct.unpack_from("<I", d, o)[0]
        if x not in cols:
            continue
        fns = []
        k = o + 4
        while k + 4 <= rd[3] + rd[4]:
            f = struct.unpack_from("<I", d, k)[0]
            if tlo <= f < thi and (k == o + 4 or pe.o2v(k) not in cols and f not in cols):
                fns.append(f)
                k += 4
            else:
                break
        n, offs, _ = cols[x]
        vts.append((n, offs, pe.o2v(o + 4), fns))
    return tds, cols, bases, vts


def vtable_stores(pe, vt_addrs):
    """Code addresses of `mov dword ptr [reg(+disp)], imm32` instructions whose immediate is a vftable."""
    d = pe.d
    text = pe.sec(".text")
    out = []
    pat = re.compile(rb"\xC7[\x00-\x07\x40-\x47\x80-\x87]")
    lo, hi = text[3], text[3] + text[4]
    for m in pat.finditer(d, lo, hi):
        o = m.start()
        modrm = d[o + 1]
        mod, rm = modrm >> 6, modrm & 7
        if rm == 4:  # SIB
            n = 3
        else:
            n = 2
        if mod == 0 and rm == 5:
            continue
        n += {0: 0, 1: 1, 2: 4}[mod]
        if o + n + 4 > len(d):
            continue
        imm = struct.unpack_from("<I", d, o + n)[0]
        if imm in vt_addrs:
            out.append((pe.o2v(o), imm))
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("exe")
    ap.add_argument("out")
    ap.add_argument("--functions", help="Ghidra functions.jsonl (enables ctor/dtor naming)")
    args = ap.parse_args()

    pe = PE(args.exe)
    tds, cols, bases, vts = scan(pe)

    lines = []
    owner = {}
    refs = collections.defaultdict(set)
    for n, offs, va, fns in vts:
        nb = len(bases.get(n, []))
        for i, f in enumerate(fns):
            refs[f].add(n)
            # The most-base class that has the function in its vtable owns it
            if f not in owner or nb < owner[f][0]:
                owner[f] = (nb, n, offs, i)
    # Identical-COMDAT-folded stubs (empty listener methods, _purecall) are shared by unrelated classes
    for f in list(owner):
        o = owner[f][1]
        if any(o not in bases.get(c, [c]) for c in refs[f]):
            del owner[f]
    used = collections.Counter()
    for f, (nb, n, offs, i) in sorted(owner.items()):
        dn = demangle_type(n)
        if not dn:
            continue
        nm = "vf%02d" % i if offs == 0 else "vf%02d_sub%d" % (i, offs)
        key = dn + "::" + nm
        used[key] += 1
        if used[key] > 1:
            nm += "_%x" % f
        lines.append({"addr": "%08x" % f, "name": dn + "::" + nm})
    for n, offs, va, fns in vts:
        dn = demangle_type(n)
        if dn:
            lines.append({"addr": "%08x" % va, "label": "vftable_" + dn.replace("::", "_") + ("" if offs == 0 else "_sub%d" % offs)})

    nctor = 0
    if args.functions:
        vtname = {va: demangle_type(n) for n, offs, va, fns in vts if offs == 0 and demangle_type(n)}
        starts = []
        for l in open(args.functions):
            f = json.loads(l)
            if not f.get("external"):
                starts.append(int(f["addr"], 16))
        starts.sort()
        cd = collections.defaultdict(set)
        for at, vt in vtable_stores(pe, set(vtname)):
            j = bisect.bisect_right(starts, at) - 1
            if j >= 0:
                cd[starts[j]].add(vtname[vt])
        named = {int(x["addr"], 16) for x in lines if "name" in x}
        for fa, cls in sorted(cd.items()):
            if fa in named or len(cls) != 1:
                continue
            lines.append({"addr": "%08x" % fa, "name": next(iter(cls)) + "::ctor_or_dtor_%x" % fa})
            nctor += 1

    with open(args.out + ".jsonl", "w") as w:
        for x in lines:
            w.write(json.dumps(x) + "\n")
    classes = {}
    for n, b in bases.items():
        classes[demangle_type(n) or n] = {"bases": [demangle_type(x) or x for x in b[1:]], "vtables": []}
    for n, offs, va, fns in vts:
        classes[demangle_type(n) or n]["vtables"].append({"va": "%08x" % va, "offset": offs, "slots": ["%08x" % f for f in fns]})
    with open(args.out + ".classes.json", "w") as w:
        json.dump(classes, w, indent=1)
    print("classes %d, vtables %d, symbols %d, ctor/dtor %d" % (len(bases), len(vts), len(lines), nctor))


if __name__ == "__main__":
    main()
