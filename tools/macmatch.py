#!/usr/bin/env python3
"""Pair the functions of a PopCap game's Mac build (GCC, symbols left in) with its PC exe's Ghidra export.

Several PopCap games shipped a Mac build whose symbols were never stripped. This carries the Mac's
demangled names onto the PC exe's (mostly unnamed) functions, so that a new decompilation starts from a
named export (docs/MACMATCH.md):

    python tools/macmatch.py --mac re/<game>/mac/<binary> [--mac re/<game>/mac/SexyApp]
        --exe re/<game>/bin/<Game>.exe --functions re/<game>/gh/functions.jsonl
        --out <match.jsonl> [--apply-names <names.jsonl>] [--cache <dir>]
        [--check-src games/<game>/src] [--check-alias <alias.json>] [--check-names] [--check-out <file>]

A Mac binary can be thin or universal; a PowerPC binary is decoded here (llvm-objdump has no PowerPC
target), an i386 one through llvm-objdump --macho. A second --mac names a framework dylib the game
loads (Bejeweled 2's SexyApp): the game's calls into it are resolved by name. The PC side reads the exe
itself (calls in order, float constants, vtable stores, the stack cleanup of each function) beside
Ghidra's functions.jsonl (strings, function bounds), and tools/rtti_msvc70.py's classes (run into
--cache when --rtti is not given).

Passes, strongest first (each match records its pass and a confidence score in 0..1):
  str    a string or float constant that one function uses in each build, kept when the two
         functions' whole sets of strings and constants agree (Jaccard); then sets of constants that
         are unique as sets
  vtbl   vtables paired by RTTI class name, or through a matched constructor that stores them, then
         a map from Mac slots to PC slots for each class (the Mac's two destructor slots are the PC's
         one deleting destructor, MSVC's overload order is rebuilt, a class starts from its base's
         map); the slots a class adds pair where the derived classes override both or neither and
         the functions already matched agree, then in order between those pairs; inherited slots
         are left to the base, folded PC functions pair with none, thunks pair with thunks
  imp    C library functions the Mac imports, by name, against the PC's FID-named copies (anchors only)
  call   callees of matched pairs aligned by call order around the calls already matched, and the
         single unmatched caller of a matched pair; checked against strings, the stack cleanup
         against the Mac's parameter list, the size ratio and the matched neighbours
  order  functions between two matched pairs that lie next to each other in both builds' address
         order, when the gaps hold the same number of functions
Every pair stays one-to-one; a candidate that two sources give different partners is dropped.

Output, one line per PC function matched: {"a", "b", "how", "name", "comment", "score"} (a = PC
address, b = Mac address, name = the Mac name made usable in Ghidra, comment = the demangled Mac
signature; "mac" names the binary when it is not the first). --apply-names writes ApplySymbols.java
lines for the PC functions whose export name is still a placeholder (FUN_, vfNN, ctor_or_dtor_; all of
them with --rename-all). The --check-* options measure the result against port sources' address
comments (`// 0x4118c9` above a definition), an alias file of renames ({"classes": {port: mac},
"methods": {"PortClass::Method": "MacClass::Method"}}) and the export's own meaningful names.
"""
import argparse
import bisect
import collections
import difflib
import json
import math
import os
import re
import struct
import subprocess
import sys

LLVM_BIN = "E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin"
HERE = os.path.dirname(os.path.abspath(__file__))

# ---------------------------------------------------------------------------------------------------
# Mac side: Mach-O


def s32(v):
    return v - 0x100000000 if v & 0x80000000 else v


class MachO:
    """A 32-bit Mach-O image (thin, or the i386 slice of a universal binary, else its PowerPC one)."""

    def __init__(self, path, index, llvm):
        self.path = path
        self.tag = os.path.basename(path)
        self.index = index
        data = open(path, "rb").read()
        if data[:4] == b"\xca\xfe\xba\xbe":
            n = struct.unpack(">I", data[4:8])[0]
            arches = [struct.unpack(">5I", data[8 + i * 20:28 + i * 20]) for i in range(n)]
            pick = next((a for a in arches if a[0] == 7), None) or next((a for a in arches if a[0] == 18), arches[0])
            data = data[pick[2]:pick[2] + pick[3]]
        self.d = data
        if data[:4] == b"\xfe\xed\xfa\xce":
            e = self.e = ">"
        elif data[:4] == b"\xce\xfa\xed\xfe":
            e = self.e = "<"
        else:
            sys.exit("%s: not a 32-bit Mach-O" % path)
        self.cpu, _, self.filetype, ncmds, _, self.flags = struct.unpack(e + "6I", data[4:28])
        self.ppc = self.cpu == 18
        self.segs = []
        self.sects = []
        self.indirect = []
        self.extrel_raw = []
        symoff = nsyms = stroff = 0
        off = 28
        for _ in range(ncmds):
            cmd, size = struct.unpack(e + "2I", data[off:off + 8])
            if cmd == 1:
                vmaddr, vmsize, fileoff, filesize, _, _, nsects, _ = struct.unpack(e + "8I", data[off + 24:off + 56])
                self.segs.append((data[off + 8:off + 24].rstrip(b"\0").decode(), vmaddr, vmsize, fileoff, filesize))
                so = off + 56
                for _ in range(nsects):
                    sname = data[so:so + 16].rstrip(b"\0").decode()
                    seg = data[so + 16:so + 32].rstrip(b"\0").decode()
                    addr, sz, offs, _, _, _, sfl, r1, r2 = struct.unpack(e + "9I", data[so + 32:so + 68])
                    self.sects.append({"seg": seg, "name": sname, "addr": addr, "size": sz, "off": offs,
                                       "flags": sfl, "r1": r1, "r2": r2, "type": sfl & 0xFF})
                    so += 68
            elif cmd == 2:
                symoff, nsyms, stroff, _ = struct.unpack(e + "4I", data[off + 8:off + 24])
            elif cmd == 0xB:
                v = struct.unpack(e + "18I", data[off + 8:off + 80])
                ind_off, n_ind, ext_off, n_ext = v[12], v[13], v[14], v[15]
                self.indirect = list(struct.unpack(e + "%dI" % n_ind, data[ind_off:ind_off + 4 * n_ind]))
                for i in range(n_ext):
                    self.extrel_raw.append(struct.unpack(e + "iI", data[ext_off + 8 * i:ext_off + 8 * i + 8]))
            off += size
        # symbols
        self.syms = []  # (value, n_sect (1-based), raw name, external)
        raw = []
        for i in range(nsyms):
            strx, ntype, nsect, _, value = struct.unpack(e + "IBBHI", data[symoff + i * 12:symoff + i * 12 + 12])
            end = data.index(b"\0", stroff + strx)
            nm = data[stroff + strx:end].decode("latin-1")
            raw.append((nm, ntype, nsect, value))
        self.symnames = [r[0] for r in raw]
        dem = demangle(sorted(set(self.symnames)), llvm)
        self.dem = dem
        for nm, ntype, nsect, value in raw:
            if ntype & 0xE0 or ntype & 0x0E != 0x0E or nsect == 0:
                continue
            self.syms.append((value, nsect, nm, bool(ntype & 1)))
        # import stubs and pointers: address -> raw symbol name (or a local address)
        self.stubs = {}
        self.ptrs = {}
        for s in self.sects:
            t = s["type"]
            if t == 8 and s["r2"]:
                for k in range(s["size"] // s["r2"]):
                    self.stubs[s["addr"] + k * s["r2"]] = self.indsym(s["r1"] + k)
            elif t in (6, 7):
                for k in range(s["size"] // 4):
                    nm = self.indsym(s["r1"] + k)
                    if nm is None:
                        nm = self.word(s["addr"] + 4 * k)
                    self.ptrs[s["addr"] + 4 * k] = nm
        # external relocations: data address -> raw symbol name
        base = 0
        if self.filetype != 2:
            first = [sg for sg in self.segs if sg[0] != "__PAGEZERO"]
            if self.flags & 0x20:
                first = [sg for sg in first if sg[0] != "__TEXT"]
            base = first[0][1] if first else 0
        self.extrel = {}
        for addr, info in self.extrel_raw:
            if self.e == ">":
                symnum, isext = info >> 8, (info >> 4) & 1
            else:
                symnum, isext = info & 0xFFFFFF, (info >> 27) & 1
            if isext and symnum < len(self.symnames):
                self.extrel[base + addr] = self.symnames[symnum]

    def indsym(self, k):
        if k >= len(self.indirect):
            return None
        v = self.indirect[k]
        if v & 0xC0000000:
            return None
        return self.symnames[v] if v < len(self.symnames) else None

    def sect_at(self, va):
        for s in self.sects:
            if s["addr"] <= va < s["addr"] + s["size"]:
                return s
        return None

    def raw(self, va, n):
        s = self.sect_at(va)
        if not s or s["type"] in (1, 0xC):  # zerofill
            return b""
        o = s["off"] + va - s["addr"]
        return self.d[o:o + min(n, s["addr"] + s["size"] - va)]

    def word(self, va):
        b = self.raw(va, 4)
        return struct.unpack(self.e + "I", b)[0] if len(b) == 4 else None

    def cstring(self, va):
        s = self.sect_at(va)
        if not s:
            return None
        o = s["off"] + va - s["addr"]
        end = self.d.find(b"\0", o, s["off"] + s["size"])
        if end < 0:
            return None
        return self.d[o:end].decode("latin-1")

    def is_code(self, s):
        return s is not None and s["type"] != 8 and bool(s["flags"] & 0x80000400)


def demangle(names, llvm):
    """Demangled names for raw Mach-O symbol names (llvm-cxxfilt, leading underscore stripped)."""
    exe = os.path.join(llvm, "llvm-cxxfilt.exe")
    if not os.path.exists(exe):
        exe = os.path.join(llvm, "llvm-cxxfilt")
    todo = [n for n in names if n.startswith("__Z")]
    out = {}
    if todo:
        r = subprocess.run([exe, "-_"], input="\n".join(todo) + "\n", capture_output=True, text=True,
                           encoding="latin-1")
        lines = r.stdout.split("\n")
        for n, d in zip(todo, lines):
            out[n] = d.strip()
    for n in names:
        if n not in out:
            out[n] = n[1:] if n.startswith("_") else n
    return out


# ---------------------------------------------------------------------------------------------------
# Mac side: functions and what each one uses

STD_PREFIXES = ("std::", "__gnu_cxx::", "__cxxabiv1::", "__gnu_debug::", "Metrowerks::")
RUNTIME = re.compile(r"^(start|_start|__start|dyld_stub_binding_helper|_?_dyld_\w+|_cxa_atexit_check\w*|"
                     r"__static_initialization_and_destruction_\d+|global constructors keyed to.*|"
                     r"global destructors keyed to.*|__tcf_\d+|___?i686\.get_pc_thunk\.\w+|"
                     r"_*darwin_\w+|__?call_mod_init_funcs|_?_?mh_execute_header|guard variable for.*|"
                     r"__GLOBAL__.*|_?_?keymgr\w*|_?_?eprintf|_?restFP|_?saveFP|_?_?savef\w*|_?_?restf\w*)$")

FW_CLASSES = set("""
Buffer ButtonListener ButtonWidget Checkbox CheckboxListener Color Common CritSect CursorWidget D3DInterface
D3DTester D3DTestImage DDImage DDInterface DSoundInstance DSoundManager DataElement DescParser Dialog
DialogButton DialogListener EditListener EditWidget EncodingParser Flags FlagsMod Font FontData FontLayer
ActiveFontLayer CharData Graphics GraphicsState HTTPTransfer HyperlinkWidget Image ImageFont ImageLib
InternetManager Insets KeyCodes ListDataElement ListListener ListWidget MTRand MemoryImage MusicInterface
NativeDisplay PakInterface PakInterfaceBase PerfTimer PropertiesParser Quantize Ratio ResourceManager
ScrollListener ScrollbarWidget ScrollbuttonWidget SexyApp SexyAppBase SexyMatrix3 SexyTransform2D SexyVector2
SexyVector3 SharedImage SharedImageRef SingleDataElement Slider SliderListener SoundInstance SoundManager
SysFont TextWidget TextListener TRect TPoint Widget WidgetContainer WidgetManager WidgetMover XMLParser
XMLWriter XMLElement BigInt BetaSupport TriVertex SWHelper Rect Point FPoint SexyString TodList Debug
BassMusicInterface BassMusicInfo FModMusicInterface FModMusicInfo BASS_INSTANCE FMOD_INSTANCE BassLoader
FModLoader MusicInfo ImageLib::Image AutoCrit DataSync DataReader DataWriter FileDataReader BufferedReader
RegistryInterface MacAppBase CarbonApp MacImage MacInterface MacSoundManager MacSoundInstance GLInterface
GLImage GLDisplay MacDisplay ResourceManagerException SoundLoader WidgetAnimator TodStringFile
""".split())


def fw_classes_from_headers():
    out = set()
    d = os.path.join(os.path.dirname(HERE), "framework")
    if not os.path.isdir(d):
        return out
    for root, _, files in os.walk(d):
        for fn in files:
            if fn.endswith(".h"):
                try:
                    src = open(os.path.join(root, fn), encoding="latin-1").read()
                except OSError:
                    continue
                out.update(re.findall(r"^\s*(?:class|struct)\s+([A-Z]\w+)\s*[:{\n]", src, re.M))
    return out


def split_params(s):
    """Top-level comma split."""
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch in "<(":
            depth += 1
        elif ch in ">)":
            depth -= 1
        if ch == "," and depth == 0:
            out.append(cur.strip())
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur.strip())
    return out


def parse_demangled(d):
    """(qualified name, parameter list or None, const) from a demangled function name."""
    s = d
    if s.endswith(" const"):
        s, const = s[:-6], True
    else:
        const = False
    if not s.endswith(")"):
        return s, None, const
    depth = 0
    for i in range(len(s) - 1, -1, -1):
        if s[i] == ")":
            depth += 1
        elif s[i] == "(":
            depth -= 1
            if depth == 0:
                break
    qual, params = s[:i], s[i + 1:-1]
    # a template function's return type comes first ("void Sexy::Foo<int>(int)")
    depth = 0
    for j, ch in enumerate(qual):
        if ch in "<(":
            depth += 1
        elif ch in ">)":
            depth -= 1
        elif ch == " " and depth == 0 and not qual[:j].endswith("operator") and "operator" not in qual[:j].split("::")[-1]:
            qual = qual[j + 1:]
            break
    plist = [] if params in ("", "void") else split_params(params)
    return qual, plist, const


WORD1 = re.compile(r"^(?:unsigned |signed )?(?:int|long|short|char|bool|float|wchar_t)(?: const)?$")


def param_words(plist):
    """Stack words the parameters take on i386, or None when a by-value type is unknown."""
    if plist is None:
        return None
    n = 0
    for p in plist:
        p = p.strip()
        if p.endswith("*") or p.endswith("&") or p.endswith("* const") or p.endswith("& const"):
            n += 1
        elif WORD1.match(p):
            n += 1
        elif p in ("double", "long long", "unsigned long long", "double const"):
            n += 2
        elif p in ("Sexy::TRect<int>", "Sexy::Color", "Sexy::TRect<int> const"):
            n += 4
        elif p in ("Sexy::TPoint<int>",):
            n += 2
        else:
            return None
    return n


def split_qual(q):
    """Components of a qualified name, '::' inside template arguments kept."""
    out, depth, cur, i = [], 0, "", 0
    while i < len(q):
        ch = q[i]
        if ch in "<(":
            depth += 1
        elif ch in ">)":
            depth -= 1
        if depth == 0 and q.startswith("::", i):
            out.append(cur)
            cur = ""
            i += 2
            continue
        cur += ch
        i += 1
    out.append(cur)
    return out


def ghidra_name(qual):
    parts = split_qual(qual)
    parts = [re.sub(r"\s+", "_", p.replace("::", "_")) for p in parts]
    return "::".join(parts)


class MacFunc:
    __slots__ = ("key", "img", "addr", "end", "raw", "dem", "qual", "plist", "words", "kind", "cls", "method",
                 "calls", "toks", "vrefs", "frefs", "callers", "sect", "dtor", "thunk_of", "imp", "size")

    def __init__(self):
        self.calls, self.toks, self.vrefs, self.frefs, self.callers = [], set(), [], set(), set()
        self.dtor = None
        self.thunk_of = None
        self.imp = False


def fmt_float(v):
    if v != v or v in (float("inf"), float("-inf")):
        return None
    return "%.6g" % v


class MacSide:
    def __init__(self, paths, llvm, cache):
        self.imgs = [MachO(p, i, llvm) for i, p in enumerate(paths)]
        self.funcs = {}  # key -> MacFunc
        self.byname = {}  # raw exported name -> key (for resolving imports across images)
        self.fwc = FW_CLASSES | fw_classes_from_headers()
        for img in self.imgs:
            self.load_functions(img)
        if len(self.imgs) > 1:
            # a framework dylib: its classes are the framework's
            for k, f in self.funcs.items():
                if f.img > 0 and f.cls:
                    self.fwc.add(f.cls.split("::")[0].split("<")[0])
            for f in self.funcs.values():
                self.describe(f)
        for img in self.imgs:
            if img.ppc:
                self.analyze_ppc(img)
            else:
                self.analyze_i386(img, llvm, cache)
        self.vtables = {}
        self.bases = {}
        self.ti_addrs = {}
        for img in self.imgs:
            self.load_vtables(img)
        self.resolve()
        cnt = collections.Counter(f.dem for f in self.funcs.values() if not f.imp)
        self.by_dem = {f.dem: k for k, f in self.funcs.items() if not f.imp and cnt[f.dem] == 1}

    def key(self, img, addr):
        return (img.index << 32) | addr

    def load_functions(self, img):
        bysect = collections.defaultdict(dict)
        for value, nsect, nm, ext in img.syms:
            s = img.sects[nsect - 1] if nsect - 1 < len(img.sects) else None
            if not img.is_code(s) or nm.endswith(".eh") or not s["addr"] <= value < s["addr"] + s["size"]:
                continue
            cur = bysect[nsect].get(value)
            if cur is None or (ext and not cur[1]):
                bysect[nsect][value] = (nm, ext)
            if ext:
                self.byname.setdefault(nm, self.key(img, value))
        for nsect, m in bysect.items():
            s = img.sects[nsect - 1]
            starts = sorted(m)
            for i, a in enumerate(starts):
                end = starts[i + 1] if i + 1 < len(starts) else s["addr"] + s["size"]
                f = MacFunc()
                f.img, f.addr, f.end, f.size = img.index, a, end, end - a
                f.key = self.key(img, a)
                f.raw = m[a][0]
                f.dem = img.dem.get(f.raw, f.raw)
                f.sect = s["name"]
                self.describe(f)
                self.funcs[f.key] = f

    def describe(self, f):
        d = f.dem
        if d.startswith("non-virtual thunk to ") or d.startswith("virtual thunk to "):
            f.thunk_of = d.split(" to ", 1)[1]
            d = f.thunk_of
        f.qual, f.plist, _ = parse_demangled(d)
        f.words = param_words(f.plist)
        parts = split_qual(f.qual)
        if parts and parts[0] == "Sexy":
            parts = parts[1:]
        f.method = parts[-1] if parts else f.qual
        f.cls = "::".join(parts[:-1]) if len(parts) > 1 else None
        raw = f.raw
        if re.search(r"D0Ev$", raw):
            f.dtor = "D0"
        elif re.search(r"D[124]Ev$", raw):
            f.dtor = "D1"
        if f.qual.startswith(STD_PREFIXES) or RUNTIME.match(f.dem) or RUNTIME.match(f.raw) or \
                re.match(r"^_*(Unwind_\w+|cxa_\w+|gxx_personality\w*|stack_chk\w*|dyld\w*)$", f.raw) or \
                re.match(r"^[^(]*\(\*(std|__gnu_cxx|Metrowerks)::", d) or \
                f.dem.startswith(("typeinfo", "vtable for", "VTT for", "construction vtable")):
            f.kind = "lib"
        elif f.plist is None and not f.raw.startswith("__Z"):
            f.kind = "c"  # a C function (zlib, libpng, ...) or the game's own C code
        else:
            base = (f.cls or "").split("::")[0].split("<")[0]
            f.kind = "fw" if base in self.fwc else "game"
            if f.cls is None:
                f.kind = "free"

    # ---- PowerPC
    def analyze_ppc(self, img):
        d, e = img.d, img.e
        for f in [f for f in self.funcs.values() if f.img == img.index]:
            s = img.sect_at(f.addr)
            o = s["off"] + f.addr - s["addr"]
            n = (f.end - f.addr) // 4
            ws = struct.unpack_from(">%dI" % n, d, o)
            regs = [None] * 32
            pic = None
            refs = []
            for i, ins in enumerate(ws):
                pc = f.addr + 4 * i
                op = ins >> 26
                if op == 18:
                    li = ins & 0x03FFFFFC
                    if li & 0x02000000:
                        li -= 0x04000000
                    tgt = li & 0xFFFFFFFF if ins & 2 else (pc + li) & 0xFFFFFFFF
                    if ins & 1:
                        f.calls.append(("d", tgt))
                        for r in (0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12):
                            regs[r] = None
                    elif not f.addr <= tgt < f.end:
                        f.calls.append(("t", tgt))
                    continue
                if op == 16:
                    bd = ins & 0xFFFC
                    if bd & 0x8000:
                        bd -= 0x10000
                    if ins & 1 and bd == 4:
                        pic = pc + 4
                    continue
                if op == 19:
                    xo = (ins >> 1) & 0x3FF
                    if xo == 528 and ins & 1:
                        f.calls.append(("i", None))
                        for r in (0, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12):
                            regs[r] = None
                    continue
                rd = (ins >> 21) & 31
                ra = (ins >> 16) & 31
                simm = ins & 0xFFFF
                if simm & 0x8000:
                    simm -= 0x10000
                if op == 31:
                    xo = (ins >> 1) & 0x3FF
                    if xo == 339:
                        spr = ((ins >> 16) & 31) | (((ins >> 11) & 31) << 5)
                        regs[rd] = (pic, True) if spr == 8 and pic is not None else None
                        if spr == 8:
                            pic = None
                    elif xo == 444 and rd == ((ins >> 11) & 31):  # mr ra, rs
                        regs[ra] = regs[rd]
                    elif xo in X_LOGICAL:
                        regs[ra] = None
                    elif xo in X_STORE:
                        if xo in X_STORE_UPDATE:
                            regs[ra] = None
                    elif xo in (0, 32, 4, 86, 54, 278, 246, 982, 1014, 598, 854, 467, 144, 150):
                        pass
                    else:
                        regs[rd] = None
                        if xo in X_LOAD_UPDATE:
                            regs[ra] = None
                    continue
                if op == 15:  # addis / lis
                    if ra == 0:
                        regs[rd] = ((simm << 16) & 0xFFFFFFFF, True)
                    elif regs[ra] is not None:
                        regs[rd] = ((regs[ra][0] + (simm << 16)) & 0xFFFFFFFF, True)
                    else:
                        regs[rd] = None
                    continue
                if op == 14:  # addi / li
                    if ra == 0:
                        regs[rd] = (simm & 0xFFFFFFFF, False)
                    elif regs[ra] is not None:
                        v = (regs[ra][0] + simm) & 0xFFFFFFFF
                        regs[rd] = (v, regs[ra][1])
                        if regs[ra][1]:
                            refs.append((v, "a"))
                    else:
                        regs[rd] = None
                    continue
                if op == 24:  # ori
                    rs = rd
                    if regs[rs] is not None:
                        v = regs[rs][0] | (ins & 0xFFFF)
                        regs[ra] = (v, regs[rs][1])
                        if regs[rs][1] and ins & 0xFFFF:
                            refs.append((v, "a"))
                    else:
                        regs[ra] = None
                    continue
                if 32 <= op <= 55:
                    ea = None
                    if ra and regs[ra] is not None and regs[ra][1]:
                        ea = (regs[ra][0] + simm) & 0xFFFFFFFF
                        refs.append((ea, PPC_MEMKIND.get(op, "m")))
                    if op in (32, 33, 34, 35, 40, 41, 42, 43):
                        regs[rd] = None
                        # a pointer loaded from the image's data (-mdynamic-no-pic code reaches vtables and
                        # coalesced objects through such pointers): what it points at is used
                        if ea is not None and op in (32, 33) and ea not in img.ptrs:
                            w = img.word(ea)
                            if w and w >= 0x1000 and img.sect_at(w) is not None:
                                regs[rd] = (w, True)
                                refs.append((w, "a"))
                    elif op == 46:
                        for r in range(rd, 32):
                            regs[r] = None
                    if op & 1 and op != 47 and op != 46:  # update forms
                        regs[ra] = None
                    continue
                if op in (20, 21, 23, 25, 26, 27, 28, 29):
                    regs[ra] = None
                elif op in (7, 8, 12, 13):
                    regs[rd] = None
            self.classify_refs(img, f, refs)

    # ---- i386 (llvm-objdump)
    def analyze_i386(self, img, llvm, cache):
        lines = None
        cpath = None
        if cache:
            cpath = os.path.join(cache, "mac_%s.dis" % img.tag)
            if os.path.exists(cpath) and os.path.getmtime(cpath) > os.path.getmtime(img.path):
                lines = open(cpath, encoding="latin-1").read().split("\n")
        if lines is None:
            exe = os.path.join(llvm, "llvm-objdump.exe")
            if not os.path.exists(exe):
                exe = os.path.join(llvm, "llvm-objdump")
            r = subprocess.run([exe, "--macho", "-d", "--no-show-raw-insn", "--no-symbolic-operands", img.path],
                               capture_output=True, text=True, encoding="latin-1")
            text = r.stdout
            if cpath:
                open(cpath, "w", encoding="latin-1", newline="").write(text)
            lines = text.split("\n")
        starts = sorted(f.addr for f in self.funcs.values() if f.img == img.index)
        fby = {f.addr: f for f in self.funcs.values() if f.img == img.index}
        thunks = {}
        for f in fby.values():
            m = re.search(r"get_pc_thunk\.(\w\w)", f.raw)
            if m:
                thunks[f.addr] = "%" + ("e" + m.group(1))
        insn = re.compile(r"^\s*([0-9a-f]+):\s+(\S+)\s*(.*)$")
        cur = None
        regs = {}
        prev_call = None
        refs = []
        for ln in lines:
            m = insn.match(ln)
            if not m:
                continue
            pc = int(m.group(1), 16)
            mn, ops = m.group(2), m.group(3)
            i = bisect.bisect_right(starts, pc) - 1
            if i < 0:
                continue
            f = fby[starts[i]]
            if pc >= f.end:
                continue
            if f is not cur:
                if cur is not None:
                    self.classify_refs(img, cur, refs)
                cur, regs, refs, prev_call = f, {}, [], None
            if prev_call is not None and mn.startswith("pop") and prev_call == pc:
                regs[ops.strip()] = (pc, True)
                prev_call = None
                continue
            prev_call = None
            if mn in ("calll", "call"):
                if ops.startswith("*"):
                    f.calls.append(("i", None))
                else:
                    try:
                        t = int(ops.split()[0], 16)
                    except ValueError:
                        continue
                    if t in thunks:
                        regs[thunks[t]] = (None, True)  # filled in by the next instruction's address
                        regs["_pending"] = thunks[t]
                    elif f.addr <= t < f.end:
                        prev_call = t
                    else:
                        f.calls.append(("d", t))
                    for r in ("%eax", "%ecx", "%edx"):
                        regs.pop(r, None)
                continue
            if "_pending" in regs:
                regs[regs.pop("_pending")] = (pc, True)
            if mn in ("jmp", "jmpl") and not ops.startswith("*"):
                try:
                    t = int(ops.split()[0], 16)
                except ValueError:
                    t = None
                if t is not None and not f.addr <= t < f.end:
                    f.calls.append(("t", t))
                continue
            kind = "m"
            if mn.startswith("f") and not mn.startswith(("fild", "fist", "fnst", "fldcw", "fstcw")):
                kind = "f4" if mn.endswith("s") else ("f8" if mn.endswith("l") else "m")
            elif "ss" in mn[-3:]:
                kind = "f4"
            elif mn.endswith("sd") and mn not in ("movsd",) or mn == "movsd" and "%xmm" in ops:
                kind = "f8"
            for mm in re.finditer(r"(\$?)(-?0x[0-9a-f]+)?\((%e\w\w)?(?:,%e\w\w)?(?:,\d)?\)|(\$?)(-?0x[0-9a-f]+)", ops):
                if mm.group(3) is not None or (mm.group(2) and mm.group(0).endswith(")")):
                    disp = int(mm.group(2), 16) if mm.group(2) else 0
                    base = mm.group(3)
                    if base in regs and regs[base][0] is not None:
                        refs.append(((regs[base][0] + disp) & 0xFFFFFFFF, kind))
                elif mm.group(5):
                    v = int(mm.group(5), 16) & 0xFFFFFFFF
                    refs.append((v, "a" if mm.group(4) else kind))
            # a write to a tracked register ends its tracking
            dest = ops.rsplit(",", 1)[-1].strip() if "," in ops else (ops.strip() if mn.startswith(("pop", "inc", "dec", "not", "neg")) else None)
            if dest in regs and not mn.startswith(("cmp", "test", "push")):
                regs.pop(dest, None)
        if cur is not None:
            self.classify_refs(img, cur, refs)

    def classify_refs(self, img, f, refs):
        for va, kind in refs:
            s = img.sect_at(va)
            if s is None:
                continue
            nm = s["name"]
            if nm == "__cstring" or (nm == "__const" and s["seg"] == "__TEXT" and kind == "a" and self._is_text(img, va)):
                t = img.cstring(va)
                if t:
                    f.toks.add("s:" + t)
            elif nm == "__cfstring":
                p = img.word(va - (va - s["addr"]) % 16 + 8)
                t = img.cstring(p) if p else None
                if t:
                    f.toks.add("s:" + t)
            elif kind in ("f4", "f8") or (nm in ("__literal4", "__literal8") and kind in ("lfs", "lfd")):
                size = 8 if kind in ("f8", "lfd") else 4
                b = img.raw(va, size)
                if len(b) == size:
                    v = struct.unpack(img.e + ("d" if size == 8 else "f"), b)[0]
                    t = fmt_float(v)
                    if t is not None:
                        f.toks.add("f:" + t)
            elif kind in ("lfs", "lfd"):
                size = 8 if kind == "lfd" else 4
                b = img.raw(va, size)
                if len(b) == size and s["seg"] == "__TEXT":
                    t = fmt_float(struct.unpack(img.e + ("d" if size == 8 else "f"), b)[0])
                    if t is not None:
                        f.toks.add("f:" + t)
            elif va in img.ptrs:
                p = img.ptrs[va]
                if isinstance(p, str):
                    f.vrefs.append(("n", p, 0))  # a symbol through a pointer: vtable, function or data
                elif p is not None:
                    self._ref_local(img, f, p)
            elif img.is_code(s):
                if kind == "a":
                    f.frefs.add(self.key(img, va))
            else:
                self._ref_local(img, f, va)

    def _is_text(self, img, va):
        b = img.raw(va, 64)
        z = b.find(b"\0")
        return z > 1 and all(32 <= c < 127 or c in (9, 10, 13) for c in b[:z])

    def _ref_local(self, img, f, va):
        s = img.sect_at(va)
        if s and img.is_code(s):
            f.frefs.add(self.key(img, va))
        else:
            f.vrefs.append(("a", self.key(img, va), 0))

    # ---- vtables
    def load_vtables(self, img):
        syms = sorted((v, nm) for v, ns, nm, ext in img.syms)
        vals = [v for v, _ in syms]
        for i, (v, nm) in enumerate(syms):
            if not nm.startswith("__ZTV"):
                continue
            cls = img.dem.get(nm, nm)
            if not cls.startswith("vtable for "):
                continue
            cls = cls[len("vtable for "):]
            j = bisect.bisect_right(vals, v)
            end = vals[j] if j < len(vals) else v + 0x400
            s = img.sect_at(v)
            if s:
                end = min(end, s["addr"] + s["size"])
            subs = []
            ti = img.extrel.get(v + 4) or img.word(v + 4)
            a = v
            cur = None
            while a + 4 <= end:
                ott = img.word(a)
                tiw = (img.extrel.get(a + 4) or img.word(a + 4)) if a + 8 <= end else None
                if cur is None or (ott is not None and -0x10000 <= s32(ott) < 0 and tiw == ti):
                    cur = {"off": -s32(ott or 0), "addr": a + 8, "slots": []}
                    subs.append(cur)
                    a += 8
                    continue
                w = img.extrel.get(a)
                if w is not None:
                    cur["slots"].append(("n", w))
                else:
                    w = img.word(a)
                    ss = img.sect_at(w) if w else None
                    cur["slots"].append(("k", self.key(img, w)) if img.is_code(ss) else ("z", w))
                a += 4
            for sub in subs:  # zero words at the end are the padding before the next symbol
                while sub["slots"] and sub["slots"][-1][0] == "z" and not sub["slots"][-1][1]:
                    sub["slots"].pop()
            self.vtables[(img.index, cls)] = {"cls": cls, "img": img.index, "addr": v, "end": end, "subs": subs,
                                              "ti": ti}
        # typeinfo: primary base of each class
        for i, (v, nm) in enumerate(syms):
            if not nm.startswith("__ZTI"):
                continue
            cls = img.dem.get(nm, nm)
            if not cls.startswith("typeinfo for "):
                continue
            cls = cls[len("typeinfo for "):]
            kindsym = img.extrel.get(v) or ""
            tiset = self.ti_addrs.setdefault(img.index, {a for a, n in syms if n.startswith("__ZTI")})

            def is_ti(a):
                x = img.extrel.get(a)
                return x.startswith("__ZTI") if x else img.word(a) in tiset

            bl = []
            cnt = img.word(v + 12) or 0
            if "si_class" in kindsym or (not kindsym and is_ti(v + 8)):
                bl.append(img.extrel.get(v + 8) or img.word(v + 8))
            elif "vmi_class" in kindsym or (not kindsym and 1 <= cnt <= 16 and is_ti(v + 16)):
                for k in range(min(cnt, 16)):
                    b = img.extrel.get(v + 16 + 8 * k) or img.word(v + 16 + 8 * k)
                    of = img.word(v + 20 + 8 * k) or 0
                    if not of & 1 and (of >> 8) == 0:
                        bl.append(b)
            names = []
            for b in bl:
                if isinstance(b, str):
                    names.append(img.dem.get(b, b).replace("typeinfo for ", ""))
                else:
                    k = bisect.bisect_left(vals, b)
                    if k < len(vals) and vals[k] == b:
                        names.append(img.dem.get(syms[k][1], "").replace("typeinfo for ", ""))
            self.bases.setdefault(cls, names[:1])

    def resolve(self):
        """Turn raw call targets into function keys or import nodes; build callers."""
        stubmap = {}
        for img in self.imgs:
            for a, nm in img.stubs.items():
                stubmap[self.key(img, a)] = nm
        self.imports = {}
        for f in list(self.funcs.values()):
            img = self.imgs[f.img]
            out = []
            for kind, t in f.calls:
                if kind == "i":
                    out.append("I")
                    continue
                k = self.key(img, t)
                if k in stubmap:
                    nm = stubmap[k]
                    k = self.byname.get(nm) if nm else None
                    if k is None:
                        k = self.import_node(nm or "?")
                elif k not in self.funcs:
                    continue
                out.append(k)
            f.calls = out
            vr = []
            for kind, x, o in f.vrefs:
                if kind == "n":
                    dm = img.dem.get(x, x)
                    if dm.startswith("vtable for "):
                        vr.append(("v", dm[len("vtable for "):]))
                    elif x in self.byname:
                        f.frefs.add(self.byname[x])
                    else:
                        vr.append(("d", dm))
                else:
                    vt = self.vtable_at(x)
                    if vt:
                        vr.append(("v", vt))
            f.vrefs = vr
        # the complete and base object constructors and destructors (C1/C2, D1/D2, and Apple GCC's C4/D4
        # that both branch to) are one node, as MSVC's one constructor and destructor are
        self.alias = {}
        byraw = {(f.img, f.raw): k for k, f in self.funcs.items()}
        for k, f in self.funcs.items():
            m = re.match(r"^(__ZN.*)([CD])([124])(E.*)$", f.raw)
            if not m or (m.group(2), m.group(3)) in (("C", "4"), ("D", "4")):
                continue
            pre, cd, suf = m.group(1), m.group(2), m.group(4)
            rep = byraw.get((f.img, pre + cd + "4" + suf))
            if rep is None and m.group(3) == "2":
                rep = byraw.get((f.img, pre + cd + "1" + suf))
            if rep is not None and rep != k:
                self.alias[k] = rep
        for f in self.funcs.values():
            f.calls = [self.alias.get(c, c) for c in f.calls]
        for k, f in self.funcs.items():
            for c in f.calls:
                if c != "I" and c in self.funcs:
                    self.funcs[c].callers.add(k)
        for vt in self.vtables.values():
            for sub in vt["subs"]:
                sl = []
                for kind, x in sub["slots"]:
                    if kind == "n":
                        k = self.byname.get(x)
                        sl.append(k if k is not None else ("imp", x))
                    elif kind == "k":
                        sl.append(self.alias.get(x, x) if x in self.funcs else None)
                    else:
                        sl.append(None)
                sub["keys"] = sl

    def vtable_at(self, key):
        img = key >> 32
        a = key & 0xFFFFFFFF
        for (i, cls), vt in self.vtables.items():
            if i == img and vt["addr"] <= a < vt["end"]:
                return cls
        return None

    def import_node(self, raw):
        k = "imp:" + raw
        if k not in self.funcs:
            f = MacFunc()
            f.key, f.img, f.addr, f.end, f.size = k, -1, 0, 0, 0
            f.raw = raw
            f.dem = self.imgs[0].dem.get(raw) or demangle([raw], LLVM_BIN)[raw]
            f.sect = "import"
            self.describe(f)
            f.imp = True
            f.kind = "imp"
            self.funcs[k] = f
        return k


X_LOGICAL = {28, 60, 444, 412, 316, 476, 124, 284, 24, 536, 792, 824, 26, 954, 922}
X_STORE = {151, 183, 215, 247, 407, 439, 662, 918, 663, 695, 727, 759, 725, 661, 983, 150}
X_STORE_UPDATE = {183, 247, 439, 695, 759}
X_LOAD_UPDATE = {55, 119, 311, 375, 567, 631}
PPC_MEMKIND = {48: "lfs", 49: "lfs", 50: "lfd", 51: "lfd", 32: "lwz", 33: "lwz"}

# ---------------------------------------------------------------------------------------------------
# PC side


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
            ch = struct.unpack_from("<I", d, off + 36)[0]
            self.secs.append((name, self.base + va, max(vsz, rsz), rp, rsz, ch))
            off += 40
        self.iat = {}
        imp_rva = struct.unpack_from("<I", d, pe + 24 + 104)[0]
        if imp_rva:
            o = self.rva_off(imp_rva)
            while o is not None:
                oft, _, _, name_rva, ft = struct.unpack_from("<5I", d, o)
                if not ft:
                    break
                dll = self.cstr(name_rva)
                t = oft or ft
                k = 0
                while True:
                    to = self.rva_off(t + 4 * k)
                    if to is None:
                        break
                    v = struct.unpack_from("<I", d, to)[0]
                    if not v:
                        break
                    nm = "#%d" % (v & 0xFFFF) if v & 0x80000000 else self.cstr(v + 2)
                    self.iat[self.base + ft + 4 * k] = nm
                    k += 1
                o += 20

    def rva_off(self, rva):
        va = self.base + rva
        for name, sva, vs, rp, rs, ch in self.secs:
            if sva <= va < sva + rs:
                return rp + va - sva
        return None

    def cstr(self, rva):
        o = self.rva_off(rva)
        if o is None:
            return ""
        return self.d[o:self.d.find(b"\0", o)].decode("latin-1")

    def raw(self, va, n):
        for name, sva, vs, rp, rs, ch in self.secs:
            if sva <= va < sva + rs:
                return self.d[rp + va - sva:rp + va - sva + min(n, sva + rs - va)]
        return b""

    def word(self, va):
        b = self.raw(va, 4)
        return struct.unpack("<I", b)[0] if len(b) == 4 else None

    def sec_of(self, va):
        for s in self.secs:
            if s[1] <= va < s[1] + s[2]:
                return s
        return None


PC_RUNTIME = re.compile(r"^(__security_check_cookie|_?_chkstk|_?_alloca_probe\w*|_?_EH_prolog\d?|_?_SEH_\w+|"
                        r"_?_CxxThrowException.*|__RTC_\w+|_?_ftol\w*|__aull\w+|__all\w+|__CI\w+|_?_purecall|"
                        r"__local_unwind\w*|__global_unwind\w*|_?_except_handler\w*|__EH_epilog\w*|"
                        r"FID_conflict:___CxxFrameHandler\w*|___?CxxFrameHandler\w*)$")
FUNCLET = re.compile(r"^(Catch@|Catch_All@|Unwind@)")
PLACEHOLDER = re.compile(r"^(FUN_|thunk_FUN_|Catch@|Catch_All@|Unwind@|LAB_|SUB_|switchD)|::vf\d+$|ctor_or_dtor_|^vf\d+$")
# test byte [esp+n] or [ebp+8], 1 or 2; jz: the flag test of a deleting destructor
SDD_TEST = re.compile(rb"\xf6(?:\x44\x24[\x04\x08\x0c]|\x45\x08)[\x01\x02]\x74")
FLOAT_OP = re.compile(rb"[\xd8\xd9\xdc\xdd][\x05\x0d\x15\x1d\x25\x2d\x35\x3d]", re.S)
VT_STORE = re.compile(rb"\xc7[\x00-\x03\x06\x07]|\xc7[\x40-\x43\x45-\x47].|\xc7[\x80-\x83\x85-\x87]....", re.S)


class PCFunc:
    __slots__ = ("addr", "name", "full", "sig", "size", "body", "calls", "toks", "vrefs", "callers", "retn",
                 "thunk", "adj", "kind", "frefs")

    def __init__(self):
        self.calls, self.toks, self.vrefs, self.callers, self.frefs = [], set(), [], set(), set()
        self.retn = None
        self.thunk = None
        self.adj = None


class PCSide:
    def __init__(self, exe, functions, rtti, cache):
        self.pe = PE(exe)
        self.funcs = {}
        for line in open(functions, encoding="utf-8"):
            j = json.loads(line)
            if j.get("external"):
                continue
            f = PCFunc()
            f.addr = int(j["addr"], 16)
            f.name, f.full, f.sig = j["name"], j.get("full") or j["name"], j.get("sig") or ""
            f.size = j.get("size") or 0
            f.toks = {"s:" + s for s in j.get("strings") or []}
            f.thunk = j.get("thunk")
            n = f.full
            if n.startswith(STD_PREFIXES) or n.startswith(("`", "_CRT", "__CRT")) or PC_RUNTIME.match(n):
                f.kind = "lib"
            elif FUNCLET.match(n):
                f.kind = "funclet"
            else:
                f.kind = "fn"
            self.funcs[f.addr] = f
        self.starts = sorted(self.funcs)
        st = set(self.starts)
        for i, a in enumerate(self.starts):
            f = self.funcs[a]
            n = f.size
            if i + 1 < len(self.starts):
                n = min(n, self.starts[i + 1] - a) if n else self.starts[i + 1] - a
            f.body = self.pe.raw(a, n)
        # thunks: a jmp to a function or through the IAT
        for f in self.funcs.values():
            b = f.body
            if len(b) >= 6 and b[:2] == b"\xff\x25":
                f.thunk = "imp:" + self.pe.iat.get(struct.unpack_from("<I", b, 2)[0], "?")
            elif len(b) >= 5 and b[0] == 0xE9:
                t = (f.addr + 5 + struct.unpack_from("<i", b, 1)[0]) & 0xFFFFFFFF
                f.thunk = t if t in st else None
            else:
                adj = None
                if len(b) >= 8 and b[:2] in (b"\x83\xe9", b"\x83\xc1") and b[3] == 0xE9:
                    adj, j = 3, 4
                elif len(b) >= 11 and b[:2] in (b"\x81\xe9", b"\x81\xc1") and b[6] == 0xE9:
                    adj, j = 6, 7
                if adj:
                    t = (f.addr + j + 4 + struct.unpack_from("<i", b, j)[0]) & 0xFFFFFFFF
                    if t in st:
                        f.adj = t
                f.thunk = None
        self.rtti = self.load_rtti(exe, rtti, cache)
        self.vt = {}  # vtable va -> {"slots": [...], "cls": name or None, "offset": n}
        for cls, c in self.rtti.items():
            for v in c["vtables"]:
                self.vt[int(v["va"], 16)] = {"slots": [int(x, 16) for x in v["slots"]], "cls": cls,
                                             "offset": v["offset"]}
        rdata = [s for s in self.pe.secs if not s[5] & 0x20000000]
        for f in self.funcs.values():
            self.scan(f, st)
        # vtables without RTTI: the values that functions store at [reg] / [reg+d] and that point at
        # an array of function addresses
        found = collections.defaultdict(set)
        for f in self.funcs.values():
            for m in VT_STORE.finditer(f.body):
                o = m.end()
                if o + 4 > len(f.body):
                    continue
                v = struct.unpack_from("<I", f.body, o)[0]
                if self.word(v) in st and self.pe.sec_of(v) in rdata:
                    found[v].add(f.addr)
        known = set(self.vt) | set(found)
        for v in sorted(found):
            if v in self.vt:
                continue
            slots = []
            a = v
            while True:
                w = self.word(a)
                if w not in st or (a != v and a in known):
                    break
                slots.append(w)
                a += 4
            self.vt[v] = {"slots": slots, "cls": None, "offset": None}
        vset = set(self.vt)
        for f in self.funcs.values():
            for m in VT_STORE.finditer(f.body):
                o = m.end()
                if o + 4 <= len(f.body):
                    v = struct.unpack_from("<I", f.body, o)[0]
                    if v in vset and v not in f.vrefs:
                        f.vrefs.append(v)
        # resolve thunks in call lists, build callers
        for f in self.funcs.values():
            out = []
            for c in f.calls:
                seen = 0
                while isinstance(c, int) and self.funcs[c].thunk is not None and seen < 4:
                    c = self.funcs[c].thunk
                    seen += 1
                out.append(c)
            f.calls = out
        for a, f in self.funcs.items():
            for c in f.calls:
                if isinstance(c, int):
                    self.funcs[c].callers.add(a)

    def word(self, va):
        return self.pe.word(va)

    def scan(self, f, st):
        b = f.body
        a = f.addr
        calls = []
        i = 0
        n = len(b)
        while i < n - 4:
            c = b[i]
            if c == 0xE8 or c == 0xE9:
                t = (a + i + 5 + struct.unpack_from("<i", b, i + 1)[0]) & 0xFFFFFFFF
                if t in st and t != a and (c == 0xE8 or not a <= t < a + n):
                    calls.append((i, t))
                    i += 5
                    continue
            elif c == 0xFF and i + 6 <= n and b[i + 1] in (0x15, 0x25):
                v = struct.unpack_from("<I", b, i + 2)[0]
                if v in self.pe.iat:
                    calls.append((i, "imp:" + self.pe.iat[v]))
                    i += 6
                    continue
            i += 1
        f.calls = [t for _, t in calls]
        for m in FLOAT_OP.finditer(b):
            o = m.end()
            if o + 4 > n:
                continue
            op, modrm = m.group(0)[0], m.group(0)[1]
            reg = (modrm >> 3) & 7
            if op == 0xD9 and reg != 0 or op == 0xDD and reg != 0:
                continue
            va = struct.unpack_from("<I", b, o)[0]
            size = 4 if op in (0xD8, 0xD9) else 8
            raw = self.pe.raw(va, size)
            if len(raw) == size:
                t = fmt_float(struct.unpack("<f" if size == 4 else "<d", raw)[0])
                if t is not None:
                    f.toks.add("f:" + t)
        if n >= 3 and b[-3] == 0xC2 and b[-1] == 0:
            f.retn = b[-2]
        elif n >= 1 and b[-1] == 0xC3:
            f.retn = 0

    def load_rtti(self, exe, rtti, cache):
        if rtti and os.path.exists(rtti):
            return json.load(open(rtti, encoding="utf-8"))
        if not cache:
            return {}
        out = os.path.join(cache, "pc_rtti")
        cj = out + ".classes.json"
        if not os.path.exists(cj) or os.path.getmtime(cj) < os.path.getmtime(exe):
            subprocess.run([sys.executable, os.path.join(HERE, "rtti_msvc70.py"), exe, out], capture_output=True)
        return json.load(open(cj, encoding="utf-8")) if os.path.exists(cj) else {}


# ---------------------------------------------------------------------------------------------------
# Matching


def norm_import(n):
    n = n.split("!")[-1]
    if n.startswith("imp:"):
        n = n[4:]
    n = n.split("(")[0].strip()
    n = re.sub(r"@\d+$", "", n)
    n = n.replace(" ", "_")
    return n.lstrip("_")


NOISE_TOKS = {"f:0", "f:1", "f:-1", "f:-0", "f:4.5036e+15", "f:2", "f:0.5"}
NOISE_STR = ("s:basic_string", "s:vector::", "s:deque::", "s:map::", "s:list::", "s:std::")


PATHLIKE = re.compile(r"^(%s)?[\w.%/-]*/?[\w%-]+\.[A-Za-z]\w{1,3}$")


def norm_tok(t):
    """Strings as both builds can share them: one path separator, and a file name for a path (the Mac
    prefixes its folder with %s where the PC names a folder of its own)."""
    if not t.startswith("s:"):
        return t
    v = t[2:].replace("\\", "/")
    if PATHLIKE.match(v):
        v = v.rsplit("/", 1)[-1]
        if v.startswith("%s"):
            v = v[2:]
    return "s:" + v


class Matcher:
    def __init__(self, mac, pc, expected_ratio):
        self.mac, self.pc = mac, pc
        self.m2p, self.p2m = {}, {}
        self.how, self.score = {}, {}
        self.ratio = expected_ratio
        self.rejected = set()
        self.log = collections.Counter()
        # what may be paired
        self.mok = {k for k, f in mac.funcs.items() if f.kind != "lib" and k not in mac.alias}
        self.pok = {a for a, f in pc.funcs.items() if f.kind == "fn" and f.thunk is None}
        self.static = {}
        self._mslots = {}
        self._sdd = {}
        self.folded = set()
        for f in list(mac.funcs.values()) + list(pc.funcs.values()):
            f.toks = {norm_tok(t) for t in f.toks if t not in NOISE_TOKS and not t.startswith(NOISE_STR)}
        mt = collections.defaultdict(set)
        pt = collections.defaultdict(set)
        for k in self.mok:
            for t in mac.funcs[k].toks:
                mt[t].add(k)
        for a in self.pok:
            for t in pc.funcs[a].toks:
                pt[t].add(a)
        self.mt, self.pt = mt, pt
        # PC import tokens for call sequences
        self.pimp = {}

    # -- bookkeeping
    def add(self, m, p, how, score):
        if m not in self.mac.funcs or p not in self.pc.funcs:
            return False
        if m in self.m2p or p in self.p2m or (m, p) in self.rejected:
            return False
        if m not in self.mok or p not in self.pok:
            return False
        self.m2p[m] = p
        self.p2m[p] = m
        self.how[m] = how
        self.score[m] = round(score, 3)
        self.log[how] += 1
        return True

    def jac(self, m, p):
        a, b = self.mac.funcs[m].toks, self.pc.funcs[p].toks
        if not a and not b:
            return None
        return len(a & b) / len(a | b)

    # -- pass: strings and constants
    def pass_strings(self):
        cand = collections.defaultdict(int)
        for t, ms in self.mt.items():
            ps = self.pt.get(t)
            if ps and len(ms) == 1 and len(ps) == 1:
                cand[(next(iter(ms)), next(iter(ps)))] += 1
        by_m = collections.defaultdict(list)
        by_p = collections.defaultdict(list)
        for (m, p), u in cand.items():
            by_m[m].append(p)
            by_p[p].append(m)
        order = sorted(cand.items(), key=lambda kv: (-kv[1], -self.jac(*kv[0])))
        for (m, p), u in order:
            j = self.jac(m, p)
            if len(by_m[m]) > 1 or len(by_p[p]) > 1:
                # several partners: only the clearly best one
                best = max(by_m[m], key=lambda q: self.jac(m, q))
                bestm = max(by_p[p], key=lambda q: self.jac(q, p))
                if best != p or bestm != m:
                    continue
                if sorted((self.jac(m, q) for q in by_m[m]), reverse=True)[1:2] and \
                        sorted((self.jac(m, q) for q in by_m[m]), reverse=True)[1] > j - 0.2:
                    continue
            if j >= 0.5 or (u >= 2 and j >= 0.3):
                if self.compatible(m, p, strict=False)[0]:
                    self.add(m, p, "str", 0.5 + 0.5 * j)
        # sets of shared constants that are unique as sets
        ms = collections.defaultdict(list)
        ps = collections.defaultdict(list)
        for k in self.mok:
            t = self.mac.funcs[k].toks
            if len(t) >= 2:
                ms[frozenset(t)].append(k)
        for a in self.pok:
            t = self.pc.funcs[a].toks
            if len(t) >= 2:
                ps[frozenset(t)].append(a)
        for key, l in ms.items():
            if len(l) == 1 and len(ps.get(key, ())) == 1:
                m, p = l[0], ps[key][0]
                if self.compatible(m, p, strict=False)[0]:
                    self.add(m, p, "str", 0.8)

    # -- compatibility of a candidate pair
    def compatible(self, m, p, strict=True):
        """(ok, evidence): no hard contradiction (stack cleanup, size, matched neighbours); evidence
        counts the matched neighbours and the strings and constants the two share."""
        if m not in self.mac.funcs or p not in self.pc.funcs:
            return False, 0
        mf, pf = self.mac.funcs[m], self.pc.funcs[p]
        ev = 0.0
        j = self.jac(m, p)
        if j:
            ev += 2 * j
        # stack cleanup against the Mac's parameter list
        if not mf.imp and pf.retn is not None and mf.words is not None and mf.cls is not None:
            if pf.retn > 0 and pf.retn // 4 not in (mf.words, mf.words + 1):
                return False, 0
        # size
        if not mf.imp and mf.size > 32 and len(pf.body) > 24:
            r = math.log(mf.size / max(1, len(pf.body)) / self.ratio)
            if abs(r) > math.log(5):
                return False, 0
        # matched neighbours
        mc = {self.m2p[c] for c in mf.calls if c in self.m2p}
        pc_ = {c for c in pf.calls if c in self.p2m}
        mcl = {self.m2p[c] for c in mf.callers if c in self.m2p}
        pcl = {c for c in pf.callers if c in self.p2m}
        agree = len(mc & pc_) + len(mcl & pcl)
        if len(mc) >= 2 and len(pc_) >= 2 and not mc & pc_ and len(mc | pc_) >= 5:
            return False, 0
        if len(mcl) >= 2 and len(pcl) >= 2 and not mcl & pcl and len(mcl | pcl) >= 5:
            return False, 0
        ev += agree
        # imports called in both
        mi = {norm_import(self.mac.funcs[c].raw) for c in mf.calls if c != "I" and self.mac.funcs[c].imp}
        pi = {norm_import(c) for c in pf.calls if isinstance(c, str)}
        if mi and pi:
            ev += 0.5 * len(mi & pi) / len(mi | pi)
        return True, ev

    # -- pass: vtables
    def mac_slots(self, cls):
        if cls not in self._mslots:
            self._mslots[cls] = self._mac_slots(cls)
        return self._mslots[cls]

    def _mac_slots(self, cls):
        """The Mac primary and secondary vtables of a class with its destructor pair made one slot and the
        new virtuals of each class reordered as MSVC lays them out: [(offset, [keys])]."""
        out = []
        for vt in [v for (i, c), v in self.mac.vtables.items() if c == cls]:
            for n, sub in enumerate(vt["subs"]):
                keys = list(sub["keys"])
                keys = self.collapse_dtor(keys)
                if n == 0:
                    perm = self.msvc_perm(cls, keys)
                    keys = [keys[i] for i in perm]
                out.append((sub["off"], keys))
            break
        return out

    def collapse_dtor(self, keys):
        out = []
        i = 0
        while i < len(keys):
            k = keys[i]
            f = self.mac.funcs.get(k) if not isinstance(k, tuple) and k is not None else None
            nxt = keys[i + 1] if i + 1 < len(keys) else None
            g = self.mac.funcs.get(nxt) if nxt is not None and not isinstance(nxt, tuple) else None
            if f is not None and g is not None and f.dtor == "D1" and g.dtor == "D0":
                out.append(nxt)
                i += 2
                continue
            if isinstance(k, tuple) and isinstance(nxt, tuple) and "D1Ev" in k[1] and "D0Ev" in nxt[1]:
                out.append(nxt)
                i += 2
                continue
            out.append(k)
            i += 1
        return out

    def slot_name(self, k):
        if k is None:
            return None
        if isinstance(k, tuple):
            q = parse_demangled(self.mac.imgs[0].dem.get(k[1], k[1]))[0]
            return split_qual(q)[-1]
        return self.mac.funcs[k].method

    def msvc_perm(self, cls, keys, depth=0):
        base = (self.mac.bases.get(cls) or [None])[0]
        nb = 0
        perm = []
        if base and depth < 12:
            bvt = [v for (i, c), v in self.mac.vtables.items() if c == base]
            if bvt and bvt[0]["subs"]:
                bkeys = self.collapse_dtor(list(bvt[0]["subs"][0]["keys"]))
                if len(bkeys) <= len(keys):
                    nb = len(bkeys)
                    perm = self.msvc_perm(base, bkeys, depth + 1)
        new = list(range(nb, len(keys)))
        names = [self.slot_name(keys[i]) for i in new]
        seen = set()
        for x, nm in zip(new, names):
            if nm is not None and nm in seen:
                continue
            group = [y for y, n2 in zip(new, names) if n2 == nm and nm is not None] or [x]
            if nm is not None:
                seen.add(nm)
            perm.extend(reversed(group))
        return perm

    def pass_vtables(self):
        """Pair vtables by RTTI class name, then their slots."""
        pcbyname = {}
        for va, v in self.pc.vt.items():
            if v["cls"] and v["offset"] == 0:
                pcbyname[v["cls"]] = va
        self.vt_pairs = {}
        for (i, c) in self.mac.vtables:
            if c in pcbyname:
                self.vt_pairs[c] = pcbyname[c]
        return self.pair_vtables()

    def vtables_from_ctors(self):
        """A matched pair whose functions each store one vtable not yet paired pairs those vtables."""
        new = 0
        used = set(self.vt_pairs.values())
        for m, p in list(self.m2p.items()):
            mv = [c for kind, c in self.mac.funcs[m].vrefs if kind == "v"]
            pv = [v for v in self.pc.funcs[p].vrefs if self.pc.vt.get(v, {}).get("offset") in (0, None)]
            mv = [c for c in dict.fromkeys(mv) if c not in self.vt_pairs]
            pv = [v for v in dict.fromkeys(pv) if v not in used]
            if len(mv) == 1 and len(pv) == 1 and self.mac_slots(mv[0]):
                self.vt_pairs[mv[0]] = pv[0]
                used.add(pv[0])
                new += 1
        return self.pair_vtables() if new else 0

    def vt_prepare(self):
        """PC functions that sit at different slot numbers in different vtables are folded copies of
        several functions (MSVC's identical COMDAT folding) and pair with none."""
        where = collections.defaultdict(set)
        for v in self.pc.vt.values():
            for j, g in enumerate(v["slots"]):
                where[g].add(j)
        self.folded = {g for g, js in where.items() if len(js) > 1}

    def mbase(self, cls):
        return (self.mac.bases.get(cls) or [None])[0]

    def slot_maps(self):
        """For each paired class, which PC slot each of its Mac slots is: the base's map for the
        inherited slots and, for the slots the class adds, map_range. The framework versions of the
        two builds often differ (the PC one has more virtuals and has moved some into a base), so
        neither the counts nor the order need agree."""
        def depth(c):
            d = 0
            while c and d < 20:
                c = self.mbase(c)
                d += 1
            return d

        desc = collections.defaultdict(list)
        for c in self.vt_pairs:
            ms = self.mac_slots(c)
            if not ms:
                continue
            b = self.mbase(c)
            seen = 0
            while b and seen < 20:
                if b in self.vt_pairs:
                    desc[b].append((ms[0][1], self.pc.vt[self.vt_pairs[c]]["slots"]))
                b = self.mbase(b)
                seen += 1
        maps = {}
        for cls in sorted(self.vt_pairs, key=depth):
            msl = self.mac_slots(cls)
            if not msl:
                continue
            mk = msl[0][1]
            pk = self.pc.vt[self.vt_pairs[cls]]["slots"]
            b = self.mbase(cls)
            m = {}
            lo_m = lo_p = 0
            if b in maps and self.mac_slots(b):
                lo_m = len(self.mac_slots(b)[0][1])
                lo_p = len(self.pc.vt[self.vt_pairs[b]]["slots"])
                if lo_m > len(mk) or lo_p > len(pk):
                    maps[cls] = {}
                    continue
                m = dict(maps[b])
            I = list(range(lo_m, len(mk)))
            J = list(range(lo_p, len(pk)))
            if I and J and len(I) * len(J) <= 60000:
                m.update(self.map_range(mk, pk, I, J, desc.get(cls, [])))
            maps[cls] = m
        return maps

    def slot_score(self, mk, pk, i, j, ds, Dm, Dp):
        """(score, sure) for Mac slot i against PC slot j, or None when the functions already
        matched say no. The evidence: the slot functions themselves matched to each other, and the
        derived classes, which override both slots or neither, with their overriding functions
        matched to each other or not."""
        m2p, p2m = self.m2p, self.p2m
        a, b = mk[i], pk[j]
        if isinstance(a, int) and b in self.pc.funcs and a in self.mac.funcs:
            if self.is_sdd(b) != (self.mac.funcs[a].dtor is not None):
                return None  # a deleting destructor pairs only with a destructor
        if isinstance(a, tuple) and "pure_virtual" in str(a[-1]):
            if b in self.pc.funcs and "purecall" not in self.pc.funcs[b].full:
                return None  # a pure virtual pairs with _purecall
        elif isinstance(a, int) and b not in self.pc.funcs:
            return None
        own = 0
        if isinstance(a, int) and a in m2p:
            if m2p[a] != b:
                return None
            own = 1
        elif b in p2m:
            return None
        dm, dp = Dm[i], Dp[j]
        both = dm & dp
        diff = len(dm ^ dp)
        good = bad = 0
        for n in both:
            x, y = ds[n][0][i], ds[n][1][j]
            if isinstance(x, int) and x in m2p:
                if m2p[x] == y:
                    good += 1
                else:
                    bad += 1
            elif y in p2m:
                bad += 1
        s = 6.0 * own + len(both) - 0.75 * diff + 3.0 * good - 3.0 * bad
        sure = own == 1 or (good >= 1 and bad == 0 and diff <= len(both)) or \
            (len(both) >= 2 and bad == 0 and diff * 4 <= len(both))
        return s, sure, diff <= len(both)

    def is_sdd(self, g):
        """An MSVC scalar or vector deleting destructor: it tests bit 0 or 1 of its flag argument."""
        v = self._sdd.get(g)
        if v is None:
            body = self.pc.funcs[g].body[:256]
            v = self._sdd[g] = SDD_TEST.search(body) is not None
        return v

    def pair_ssim(self, a, b):
        if isinstance(a, int) and a in self.mac.funcs and b in self.pc.funcs:
            return self.ssim(a, b)
        return None

    def map_range(self, mk, pk, I, J, ds):
        """Slots I of the Mac vtable against slots J of the PC one. First the pairs the evidence
        is sure of, each the clear best of its row and its column (they may cross: the PC framework
        moved some virtuals); then, between the sure pairs that keep the order, the slots left: as
        many on both sides go in order when they look alike, otherwise by an order-keeping
        alignment of the static similarity."""
        Dm = {i: frozenset(n for n, (d_mk, _) in enumerate(ds) if i < len(d_mk) and d_mk[i] != mk[i]) for i in I}
        Dp = {j: frozenset(n for n, (_, d_pk) in enumerate(ds) if j < len(d_pk) and d_pk[j] != pk[j]) for j in J}
        sc = {}
        for i in I:
            for j in J:
                r = self.slot_score(mk, pk, i, j, ds, Dm, Dp)
                if r is not None:
                    sc[(i, j)] = r
        row = collections.defaultdict(list)
        col = collections.defaultdict(list)
        for (i, j), r in sc.items():
            row[i].append(r[0])
            col[j].append(r[0])

        def second(lst, v):
            rest = sorted(lst, reverse=True)
            rest.remove(v)
            return rest[0] if rest else -99.0

        sure = {}
        for (i, j), (s, ok, _) in sc.items():
            if not ok:
                continue
            if s - second(row[i], s) >= 1.0 and s - second(col[j], s) >= 1.0:
                sure[i] = j
        # the longest chain of sure pairs that keeps the order bounds the gaps
        pairs = sorted(sure.items())
        chain = []
        if pairs:
            prev, idx = [None] * len(pairs), []
            for n, (i, j) in enumerate(pairs):
                k = bisect.bisect_left([pairs[t][1] for t in idx], j)
                if k < len(idx):
                    idx[k] = n
                else:
                    idx.append(n)
                prev[n] = idx[k - 1] if k else None
            n = idx[-1] if idx else None
            while n is not None:
                chain.append(pairs[n])
                n = prev[n]
            chain.reverse()
        out = dict(sure)
        used_j = set(sure.values())
        bounds = [(I[0] - 1, J[0] - 1)] + chain + [(I[-1] + 1, J[-1] + 1)]
        for (i0, j0), (i1, j1) in zip(bounds, bounds[1:]):
            rows = [i for i in range(i0 + 1, i1) if i not in out]
            cols = [j for j in range(j0 + 1, j1) if j not in used_j]
            if not rows or not cols:
                continue
            if len(rows) == len(cols):
                zs = [self.pair_ssim(mk[i], pk[j]) for i, j in zip(rows, cols)]
                ok = all((i, j) in sc and sc[(i, j)][2] for i, j in zip(rows, cols))
                vals = [z for z in zs if z is not None]
                mean = sum(vals) / len(vals) if vals else 1.0
                need = 0.0 if len(rows) <= 2 else 0.35 if len(rows) <= 6 else 0.5
                if ok and (not vals or (min(vals) >= 0.1 and mean >= need)):
                    for i, j in zip(rows, cols):
                        out[i] = j
                    continue
            sim = []
            for i in rows:
                r = []
                for j in cols:
                    t = sc.get((i, j))
                    z = self.pair_ssim(mk[i], pk[j])
                    if t is None or not t[2] or z is None:
                        r.append(-1.0)
                    else:
                        r.append(t[0] + 2.0 * (z - 0.6))
                sim.append(r)
            for x, y in self.dp_align(sim):
                i, j = rows[x], cols[y]
                if sc[(i, j)][0] >= 1:  # the static similarity alone is not enough here
                    out[i] = j
        return out

    @staticmethod
    def dp_align(sim):
        """The order-keeping pairing of rows and columns with the largest total positive score."""
        n, m = len(sim), len(sim[0])
        best = [[0.0] * (m + 1) for _ in range(n + 1)]
        for i in range(n - 1, -1, -1):
            row, nxt, si = best[i], best[i + 1], sim[i]
            for j in range(m - 1, -1, -1):
                v = max(nxt[j], row[j + 1])
                w = si[j]
                if w > 0 and w + nxt[j + 1] > v:
                    v = w + nxt[j + 1]
                row[j] = v
        out = []
        i = j = 0
        while i < n and j < m:
            w = sim[i][j]
            if w > 0 and abs(best[i][j] - (w + best[i + 1][j + 1])) < 1e-9:
                out.append((i, j))
                i += 1
                j += 1
            elif abs(best[i][j] - best[i + 1][j]) < 1e-9:
                i += 1
            else:
                j += 1
        return out

    def pair_vtables(self):
        self.vt_prepare()
        votes = collections.defaultdict(float)
        maps = self.slot_maps()
        # a PC function that slots of different Mac functions map to is a folded copy (MSVC folds
        # identical functions, such as the deleting destructors of classes that add nothing)
        seen = collections.defaultdict(set)
        for cls, m in maps.items():
            mk = self.mac_slots(cls)[0][1]
            pk = self.pc.vt[self.vt_pairs[cls]]["slots"]
            for i, j in m.items():
                if isinstance(mk[i], int):
                    seen[pk[j]].add(mk[i])
        shared = {g for g, ms in seen.items() if len(ms) > 1}
        for cls, va in self.vt_pairs.items():
            msl = self.mac_slots(cls)
            if not msl:
                continue
            mk = msl[0][1]
            pv = self.pc.vt[va]
            pk = pv["slots"]
            base = self.mbase(cls)
            bva = self.vt_pairs.get(base)
            bmsl = self.mac_slots(base) if base else []
            bm = bmsl[0][1] if bmsl and bva else []
            bp = self.pc.vt[bva]["slots"] if bva and bm else []
            for i, j in maps.get(cls, {}).items():
                a, g = mk[i], pk[j]
                if not isinstance(a, int) or g not in self.pc.funcs or g in self.folded or g in shared:
                    continue
                if (i < len(bm) and bm[i] == a) or (j < len(bp) and bp[j] == g):
                    continue  # inherited on either side: the base's pair
                if self.m2p.get(a) != g and len(self.pc.funcs[g].body) <= 8:
                    continue  # a getter or an empty body: MSVC folds those across classes
                if a in self.mok and g in self.pok and self.ssim(a, g) < 0.15 and self.m2p.get(a) != g:
                    continue
                votes[(a, g)] += 1.0
            # secondary vtables: by offset order, when there are as many and their counts agree
            msec = sorted(msl[1:], key=lambda x: x[0])
            psec = sorted([(v["offset"], v["slots"]) for v in self.pc.vt.values()
                           if v["cls"] == cls and v["offset"]], key=lambda x: x[0]) if pv["cls"] else []
            if len(msec) != len(psec):
                continue
            for (mo, mks), (po, pks) in zip(msec, psec):
                if len(mks) != len(pks):
                    continue
                for a, b in zip(mks, pks):
                    if a is None or isinstance(a, tuple) or b not in self.pc.funcs:
                        continue
                    f = self.mac.funcs[a]
                    pf = self.pc.funcs[b]
                    if not f.thunk_of:
                        continue
                    tgt = self.mac.by_dem.get(f.thunk_of)
                    if pf.adj:
                        votes[(a, b)] += 1.0
                        if tgt is not None:
                            votes[(tgt, pf.adj)] += 1.0
                    elif tgt is not None:
                        votes[(tgt, b)] += 1.0
        return self.select(votes, "vtbl", 0.7, strict=False, score=lambda v: 0.9)

    # -- pass: library functions by name
    def pass_imports(self):
        byname = collections.defaultdict(list)
        for a in self.pok:
            f = self.pc.funcs[a]
            n = f.full
            if PLACEHOLDER.search(n) or n.startswith("FID_conflict") or "::" in n:
                continue
            if re.match(r"^(_\w+|[a-z][a-z0-9_]*|operator_?\w.*)$", n):
                byname[norm_import(n)].append(a)
        mby = collections.defaultdict(list)
        for k in self.mok:
            f = self.mac.funcs[k]
            if f.imp or (f.kind in ("c", "free") and not f.qual.startswith("Sexy::")):
                mby[norm_import(f.qual)].append(k)
        n = 0
        for nm, ks in mby.items():
            ps = byname.get(nm)
            if ps and len(ps) == 1 and len(ks) == 1:
                n += self.add(ks[0], ps[0], "imp", 0.95)
            elif ps:
                # a library name both builds have more than once (operator delete and delete[]): left
                # out, so that the call graph does not pair the copies with something else
                self.mok.difference_update(ks)
                self.pok.difference_update(ps)
        return n

    # -- the static part of a pair's similarity (sizes, strings, parameters, degrees)
    def ssim(self, x, y):
        key = (x, y)
        v = self.static.get(key)
        if v is not None:
            return v
        mf, pf = self.mac.funcs[x], self.pc.funcs[y]
        v = 1.0
        a, b = mf.toks, pf.toks
        if a and b:
            j = len(a & b) / len(a | b)
            v *= 0.75 + 0.5 * min(1.0, 2 * j) if j else (0.6 if len(a) >= 2 and len(b) >= 2 else 0.8)
        elif len(a) >= 3 or len(b) >= 3:
            v *= 0.7
        if not mf.imp and pf.retn is not None and mf.words is not None and mf.cls is not None:
            if pf.retn > 0 and pf.retn // 4 not in (mf.words, mf.words + 1):
                v = 0.0
            elif pf.retn == 0 and mf.words > 0:
                v *= 0.7
            elif pf.retn > 0 or mf.words == 0:
                v *= 1.1
        if not mf.imp and mf.size > 32 and len(pf.body) > 24:
            lr = math.log(mf.size / len(pf.body) / self.ratio)
            v *= 0.0 if abs(lr) > math.log(5) else math.exp(-lr * lr / (2 * 0.6 * 0.6))
        else:
            v *= 0.8
        dm, dp = self.outdeg_m.get(x, 0), self.outdeg_p.get(y, 0)
        v *= 0.5 + 0.5 * (min(dm, dp) + 1) / (max(dm, dp) + 1)
        im, ip = len(mf.callers), len(pf.callers)
        v *= 0.7 + 0.3 * (min(im, ip) + 1) / (max(im, ip) + 1)
        v = min(v, 1.0)
        self.static[key] = v
        return v

    def degrees(self):
        self.outdeg_m = {k: len({c for c in f.calls if c != "I" and c in self.mok}) for k, f in self.mac.funcs.items()}
        self.outdeg_p = {a: len({c for c in f.calls if isinstance(c, int) and c in self.pok})
                         for a, f in self.pc.funcs.items()}

    # -- pass: call graph
    def pass_calls(self):
        votes = collections.defaultdict(float)
        for m, p in list(self.m2p.items()):
            mf, pf = self.mac.funcs[m], self.pc.funcs[p]
            um = collections.OrderedDict()
            for c in mf.calls:
                if c != "I" and c not in self.m2p and c in self.mok:
                    um[c] = um.get(c, 0) + 1
            up = collections.OrderedDict()
            for c in pf.calls:
                if isinstance(c, int) and c not in self.p2m and c in self.pok:
                    up[c] = up.get(c, 0) + 1
            if um and up:
                nm, np_ = len(um), len(up)
                if nm <= 30 and np_ <= 30:
                    norm = 1.0 / max(nm, np_)
                    for i, (x, cx) in enumerate(um.items()):
                        for j, (y, cy) in enumerate(up.items()):
                            s = self.ssim(x, y)
                            if s <= 0:
                                continue
                            pos = 1.0 - abs((i + 0.5) / nm - (j + 0.5) / np_)
                            mult = min(cx, cy) / max(cx, cy)
                            votes[(x, y)] += norm * s * (0.5 + 0.5 * pos) * (0.6 + 0.4 * mult)
                # calls aligned around the matched ones
                sm = [t for t in (self.mtok(c) for c in mf.calls) if t is not None]
                sp = [t for t in (self.ptok(c) for c in pf.calls) if t is not None]
                i = j = 0
                for blk in difflib.SequenceMatcher(None, sm, sp, autojunk=False).get_matching_blocks():
                    gm = list(dict.fromkeys(t[1] for t in sm[i:blk.a] if t[0] == "m"))
                    gp = list(dict.fromkeys(t[1] for t in sp[j:blk.b] if t[0] == "p"))
                    if gm and len(gm) == len(gp) <= 3 and (i or j or blk.size):
                        for x, y in zip(gm, gp):
                            s = self.ssim(x, y)
                            if s > 0:
                                votes[(x, y)] += 0.5 * s
                    i, j = blk.a + blk.size, blk.b + blk.size
            cm = [c for c in mf.callers if c not in self.m2p and c in self.mok]
            cp = [c for c in pf.callers if c not in self.p2m and c in self.pok]
            if cm and cp and len(cm) <= 30 and len(cp) <= 30:
                norm = 1.0 / max(len(cm), len(cp))
                for x in cm:
                    for y in cp:
                        s = self.ssim(x, y)
                        if s > 0:
                            votes[(x, y)] += norm * s
        return self.select(votes, "call", 0.45)

    def mtok(self, c):
        if c == "I":
            return None
        if c in self.m2p:
            return ("P", self.m2p[c])
        f = self.mac.funcs[c]
        if f.imp:
            return ("I", norm_import(f.raw))
        if c not in self.mok:
            return None
        return ("m", c)

    def ptok(self, c):
        if isinstance(c, str):
            return ("I", norm_import(c))
        if c in self.p2m:
            return ("P", c)
        if self.pc.funcs[c].kind == "lib":
            return None
        return ("p", c)

    def select(self, votes, how, thresh, margin=1.5, strict=True, score=None):
        bym = collections.defaultdict(list)
        byp = collections.defaultdict(list)
        for (x, y), v in votes.items():
            if x in self.m2p or y in self.p2m or x not in self.mok or y not in self.pok:
                continue
            bym[x].append((v, y))
            byp[y].append((v, x))
        for l in list(bym.values()) + list(byp.values()):
            l.sort(key=lambda t: -t[0])
        added = 0
        for (x, y), v in sorted(votes.items(), key=lambda kv: -kv[1]):
            if v < thresh:
                break
            if x in self.m2p or y in self.p2m or (x, y) in self.rejected:
                continue
            sx, sy = bym.get(x), byp.get(y)
            if not sx or not sy or sx[0][1] != y or sy[0][1] != x:
                continue
            if len(sx) > 1 and sx[1][0] * margin > v or len(sy) > 1 and sy[1][0] * margin > v:
                continue
            ok, ev = self.compatible(x, y, strict=strict)
            if not ok:
                self.rejected.add((x, y))
                continue
            sc = score(v) if score else min(0.9, 0.35 + 0.2 * v + 0.05 * ev)
            if self.add(x, y, how, sc):
                added += 1
        return added

    # -- pass: address order
    def pass_order(self):
        votes = collections.defaultdict(float)
        for img in range(len(self.mac.imgs)):
            mlist = sorted(k for k in self.mok if self.mac.funcs[k].img == img and self.mac.funcs[k].sect == "__text")
            matched = [k for k in mlist if k in self.m2p]
            pos = {k: i for i, k in enumerate(mlist)}
            for a, b in zip(matched, matched[1:]):
                pa, pb = self.m2p[a], self.m2p[b]
                if not pa < pb:
                    continue
                gm = mlist[pos[a] + 1:pos[b]]
                ia = bisect.bisect_right(self.pc.starts, pa)
                ib = bisect.bisect_left(self.pc.starts, pb)
                gp = [x for x in self.pc.starts[ia:ib] if x in self.pok]
                if not gm or len(gm) != len(gp) or len(gm) > 8:
                    continue
                sims = [self.ssim(x, y) for x, y in zip(gm, gp)]
                if min(sims) < 0.35:
                    continue
                for (x, y), s in zip(zip(gm, gp), sims):
                    votes[(x, y)] += 0.5 + s
        return self.select(votes, "order", 0.7)

    def run(self):
        self.degrees()
        self.pass_strings()
        self.pass_vtables()
        self.pass_imports()
        for _ in range(40):
            n = self.vtables_from_ctors()
            n += self.pass_calls()
            if not n:
                n = self.pass_order()
            if not n:
                break


# ---------------------------------------------------------------------------------------------------
# Measurement against port sources and export names

DEF = re.compile(r"^(?:[\w:<>\*&,]+\s+)*?[\*&]?((?:\w+::)+)(~?\w+|operator\s*\S+?)\s*\(")
ADDR_COMMENT = re.compile(r"^//\s*0x([0-9a-fA-F]{6,8})\b(.*)$")


def truth_from_src(src):
    """{PC address: (class, method, where)} from `// 0x4118c9` comments above the definitions they name."""
    out = {}
    for root, _, files in os.walk(src):
        for fn in files:
            if not fn.endswith((".cpp", ".h", ".inc")):
                continue
            lines = open(os.path.join(root, fn), encoding="utf-8", errors="replace").read().split("\n")
            for i, ln in enumerate(lines):
                m = ADDR_COMMENT.match(ln)
                if not m:
                    continue
                rest = m.group(2)
                if re.match(r"\s*(\+|,|and|/|-|or)\s*0x", rest) or re.match(r"\s*\.\.", rest):
                    continue
                addr = int(m.group(1), 16)
                for k in range(i + 1, min(i + 12, len(lines))):
                    l2 = lines[k]
                    if ADDR_COMMENT.match(l2):
                        break  # another address comment comes first: this one names something else
                    if l2.startswith("//") or not l2.strip():
                        continue
                    d = DEF.match(l2)
                    if d:
                        out.setdefault(addr, (d.group(1)[:-2], d.group(2).replace(" ", ""), "%s:%d" % (fn, k + 1)))
                    break
    return out


CLASS_ALIASES = {"SexyAppBase": "SexyApp", "Rect": "TRect<int>", "FRect": "TRect<double>", "Point": "TPoint<int>",
                 "Sexy": ""}
OPERATORS = {"operator_assign": "operator=", "operator_eq": "operator==", "operator_ne": "operator!=",
             "operator_lt": "operator<", "operator_index": "operator[]", "operator_call": "operator()"}
TRUTH_PLACEHOLDER = re.compile(r"^(vfunction\d+|vf\d+|ctor_or_dtor\w*|FUN_\w+|thunk\w*)$")
DELETING = re.compile(r"^(scalar_deleting_destructor|scalar_deleting_dtor|deleting_dtor|vector_deleting\w*|"
                      r"deleting_destructor)$")


def norm_cm(cls, method, alias):
    cls = (cls or "").replace("Sexy::", "")
    old = cls.rsplit("::", 1)[-1]
    cls = alias.get("classes", {}).get(cls, CLASS_ALIASES.get(cls, cls))
    method = OPERATORS.get(method, method)
    if old and method in (old, "~" + old):  # a constructor or destructor follows its class
        method = method.replace(old, cls.rsplit("::", 1)[-1])
    k = "%s::%s" % (cls, method)
    return alias.get("methods", {}).get(k, k)


def same_name(t, mac):
    """Port or export name against the Mac name, both 'Class::method' after the aliases."""
    if t == mac:
        return True
    tc, tm = t.rsplit("::", 1)
    mc, mm = mac.rsplit("::", 1)
    if tc != mc:
        return False
    if DELETING.match(tm) and DELETING.match(mm):
        return True
    short = "~" + tc.rsplit("::", 1)[-1]
    if (DELETING.match(tm) and mm == short) or (DELETING.match(mm) and tm == short):
        return True  # the deleting destructor, named after the destructor on one side
    if mm == short and tm in ("dtor", "dtor_body", "destructor"):
        return True
    for suf in ("_body", "_inner", "_impl"):
        if tm.endswith(suf):
            tm = tm[:-len(suf)]
    return tm == mm or tm.startswith(mm + "_")


def check(matcher, args, out_rows):
    alias = json.load(open(args.check_alias, encoding="utf-8")) if args.check_alias else {}
    truth = {}
    if args.check_src:
        for a, (c, m, where) in truth_from_src(args.check_src).items():
            truth[a] = (norm_cm(c, m, alias), "src " + where)
    if args.check_names:
        for a, f in matcher.pc.funcs.items():
            if a in truth or PLACEHOLDER.search(f.name) or f.full.startswith(STD_PREFIXES):
                continue
            parts = split_qual(f.full)
            if len(parts) >= 2 and parts[0] == "Sexy":
                truth[a] = (norm_cm("::".join(parts[1:-1]), parts[-1], alias), "export")
    truth = {a: t for a, t in truth.items() if not TRUTH_PLACEHOLDER.match(t[0].rsplit("::", 1)[-1])}
    stats = collections.defaultdict(lambda: [0, 0, 0])
    bad = []
    for r in out_rows:
        a = int(r["a"], 16)
        how = r["how"]
        stats[how][0] += 1
        if a not in truth:
            continue
        mf = matcher.mac.funcs[r["_m"]]
        last = r["name"].rsplit("::", 1)[-1]
        mac_cm = norm_cm(mf.cls, last if DELETING.match(last) or last.startswith("thunk_") else mf.method, {})
        t, where = truth[a]
        if same_name(t, mac_cm):
            stats[how][1] += 1
        else:
            stats[how][2] += 1
            bad.append({"a": r["a"], "b": r["b"], "how": how, "mac": mf.dem, "truth": t, "where": where,
                        "score": r["score"]})
    lines = []
    for how in sorted(stats):
        n, ok, ko = stats[how]
        lines.append("check %-6s matched %5d  checkable %4d  same name %4d  other name %3d" % (how, n, ok + ko, ok, ko))
    lines.append("check truth: %d PC functions with a known name" % len(truth))
    if args.check_out:
        with open(args.check_out, "w", encoding="utf-8", newline="") as fo:
            for b in bad:
                fo.write(json.dumps(b) + "\n")
    return lines


def coverage(matcher):
    lines = []
    for img in range(len(matcher.mac.imgs)):
        tot = collections.Counter()
        got = collections.Counter()
        for k in matcher.mok:
            f = matcher.mac.funcs[k]
            if f.img != img:
                continue
            tot[f.kind] += 1
            if k in matcher.m2p:
                got[f.kind] += 1
        lines.append("coverage %s: " % matcher.mac.imgs[img].tag + ", ".join(
            "%s %d/%d (%.0f%%)" % (kd, got[kd], tot[kd], 100.0 * got[kd] / max(1, tot[kd])) for kd in sorted(tot)))
    return lines


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--mac", action="append", required=True)
    ap.add_argument("--exe", required=True)
    ap.add_argument("--functions", required=True)
    ap.add_argument("--rtti")
    ap.add_argument("--out", required=True)
    ap.add_argument("--apply-names")
    ap.add_argument("--rename-all", action="store_true")
    ap.add_argument("--cache")
    ap.add_argument("--llvm", default=LLVM_BIN)
    ap.add_argument("--check-src")
    ap.add_argument("--check-alias")
    ap.add_argument("--check-names", action="store_true")
    ap.add_argument("--check-out")
    args = ap.parse_args()
    if args.cache:
        os.makedirs(args.cache, exist_ok=True)
    mac = MacSide(args.mac, args.llvm, args.cache)
    pc = PCSide(args.exe, args.functions, args.rtti, args.cache)
    ratio = 1.7 if mac.imgs[0].ppc else 1.1
    mt = Matcher(mac, pc, ratio)
    mt.run()
    rows = []
    for m, p in sorted(mt.m2p.items(), key=lambda kv: kv[1]):
        f = mac.funcs[m]
        if f.imp and not PLACEHOLDER.search(pc.funcs[p].name):
            continue
        name = ghidra_name(f.qual)
        if f.dtor == "D0":
            name = name.rsplit("::", 1)[0] + "::deleting_destructor"
        elif f.thunk_of:
            q = name.rsplit("::", 1)
            name = q[0] + "::thunk_" + q[1] if len(q) == 2 else "thunk_" + name
        r = {"a": "%08x" % p, "b": "%08x" % (f.addr if not f.imp else 0), "how": mt.how[m], "name": name,
             "comment": f.dem + (" (deleting destructor)" if f.dtor == "D0" else ""), "score": mt.score[m]}
        if f.img > 0:
            r["mac"] = mac.imgs[f.img].tag
        if f.imp:
            r["mac"] = "import"
        r["_m"] = m
        rows.append(r)
    with open(args.out, "w", encoding="utf-8", newline="") as fo:
        for r in rows:
            fo.write(json.dumps({k: v for k, v in r.items() if k != "_m"}) + "\n")
    if args.apply_names:
        with open(args.apply_names, "w", encoding="utf-8", newline="") as fo:
            for r in rows:
                if not args.rename_all and not PLACEHOLDER.search(pc.funcs[int(r["a"], 16)].name):
                    continue
                c = "Mac %s%s (macmatch %s %.2f): %s" % (r.get("mac", "") + " " if "mac" in r else "", r["b"], r["how"],
                                                          r["score"], r["comment"])
                fo.write(json.dumps({"addr": r["a"], "name": r["name"], "comment": c}) + "\n")
    cnt = collections.Counter(r["how"] for r in rows)
    for how in ("str", "vtbl", "imp", "call", "order"):
        print("pass %-5s %5d" % (how, cnt.get(how, 0)))
    print("Mac functions %d (pairable %d), PC functions %d (pairable %d), matched %d" % (
        len(mac.funcs), len(mt.mok), len(pc.funcs), len(mt.pok), len(rows)))
    for ln in coverage(mt):
        print(ln)
    if args.check_src or args.check_names:
        for ln in check(mt, args, rows):
            print(ln)


if __name__ == "__main__":
    main()
