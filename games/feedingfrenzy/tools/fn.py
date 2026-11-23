#!/usr/bin/env python3
"""Print functions of Feeding Frenzy's export (the main checkout's re/feedingfrenzy/gh/all.c) for reading, compacted:
without blank lines, the exception-frame bookkeeping (ExceptionList, the unwind state local_4, uStack_4 or uStack_8,
and puStack_8), the return-address stores (uStack_88 = 0x40925b) and the declarations of locals the body never uses;
and with the engine's idioms collapsed: RELEASE(p) for a counted reference released through its virtual base, ADDREF(p) for one added,
p->v3c(...) for a call of a virtual slot (by its byte offset), p->m44 for a field (m44_f a float, m44_b a byte, m44_w
a short, m44_d a double), and the std::string's destructor (a free when its capacity is over 15) left out.

    python -I games/feedingfrenzy/tools/fn.py <addr> [<addr>...]     (hex, with or without 0x)
    python -I games/feedingfrenzy/tools/fn.py --raw <addr>            as Ghidra wrote it
    python -I games/feedingfrenzy/tools/fn.py --vt <addr>             a vtable's slots, read from the exe up to the first
                                                                    word that is not an address in the code
"""
import os
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ALL_C = os.path.join(subprocess.check_output(["git", "rev-parse", "--show-toplevel"], cwd=HERE, text=True).strip(), "re", "feedingfrenzy", "gh", "all.c")
HEAD = re.compile(r"^// ([0-9a-f]{8})  ")
NOISE = re.compile(r"^\s*(\w+ = ExceptionList|ExceptionList = [^;]+);$|^\s*local_4(\._0_1_)? = |^\s*puStack_8 = |^\s*pvStack_c = "
                   r"|^\s*u[A-Za-z]*Stack_[0-9a-f]+ = 0x4[0-9a-f]{5};|^\s*u[A-Za-z]*Stack_[48](\._0_1_)? = (0x)?[0-9a-f]+;"
                   r"|^\s*local_4 = CONCAT31|^\s*u[A-Za-z]*Stack_[0-9a-f]+ = CONCAT31|/\* WARNING: Removing unreachable"
                   r"|/\* WARNING: Function: __security|/\* WARNING: Could not recover jumptable|/\* WARNING: Treating indirect jump")

# A release through the virtual base: the object's vbtable gives the offset of its Interface part, whose slot 1 is
# release(); an add is slot 0
P_REL1 = (r"(\w+) = ([^;\n]+);\n\s*if \(\1 != 0\) \{\n"
          r"\s*(\w+) = \(int \*\)\(\*\(int \*\)\(\*\(int \*\)\(\1 \+ 4\) \+ 4\) \+ 4 \+ \1\);\n"
          r"\s*\(\*\*\(code \*\*\)\(\*\3 \+ 4\)\)\(\(undefined \*\)\3\);\n\s*\}")
P_REL2 = (r"(\w+) = ([^;\n]+);\n\s*if \(\1 != 0\) \{\n"
          r"\s*(\w+) = \*\(int \*\)\(\*\(int \*\)\(\1 \+ 4\) \+ 4\);\n"
          r"\s*\(\*\*\(code \*\*\)\(\*\(int \*\)\(\3 \+ 4 \+ \1\) \+ 4\)\)\(\(undefined \*\)\(\3 \+ 4 \+ \1\)\);\n\s*\}")
P_REL3 = (r"(\w+) = ([^;\n]+);\n\s*if \(\1 != 0\) \{\n"
          r"\s*\(\*\*\(code \*\*\)\(\*\(int \*\)\(\*\(int \*\)\(\*\(int \*\)\(\1 \+ 4\) \+ 4\) \+ 4 \+ \1\) \+ 4\)\)\(\);\n\s*\}")
P_ADD1 = (r"if \((\w+) != \([\w ]+\*\)0x0\) \{\n"
          r"\s*\(\*\(code \*\)\*\*\(undefined4 \*\*\)\(\*\(int \*\)\(\1\[1\] \+ 4\) \+ 4 \+ \(int\)\1\)\)\s*"
          r"\(\(undefined \*\)\(\*\(int \*\)\(\1\[1\] \+ 4\) \+ 4 \+ \(int\)\1\)\);\n\s*\}")
P_ADD2 = (r"if \((\w+) != 0\) \{\n\s*(\w+) = \*\(int \*\)\(\*\(int \*\)\(\1 \+ 4\) \+ 4\);\n"
          r"\s*\(\*\(code \*\)\*\*\(undefined4 \*\*\)\(\2 \+ 4 \+ \1\)\)\(\(undefined \*\)\(\2 \+ 4 \+ \1\)\);\n\s*\}")
P_STRFREE = r"\n\s*if \((?:0xf < [\w.]+|[\w.]+ > 0xf)\) \{\n(?:\s*/\* WARNING: Subroutine does not return \*/\n)?\s*_free\([^;]*\);\n\s*\}"
# A std::string made empty (capacity 15, length 0, the first character 0) or cut back, and one assigned a literal by
# 0x403d30 (assign(const char*, length))
P_STRINIT = r"\n\s*[\w.]+ = 0xf;\n\s*[\w.]+ = 0;(?=\n)"
P_STRCUT = r"\n\s*([\w.]+) = \([\w ]+\*?\)\(\(uint\)\1 & 0xffffff00\);(?=\n)"
P_STRCUT2 = r"\n\s*([\w.]+) = \1 & 0xffffff00;(?=\n)"
P_ASSIGN = r"FUN_00403d30\(&?([\w.]+),\(undefined4 \*\)(\"(?:[^\"\\]|\\.)*\"),\d+\);"
P_ASSIGN0 = r"FUN_00403d30\(&?([\w.]+),\(undefined4 \*\)&DAT_[0-9a-f]+,0\);"
IDIOMS = [(re.compile(P_STRINIT), ""), (re.compile(P_STRCUT), ""), (re.compile(P_STRCUT2), ""),
          (re.compile(P_ASSIGN), r"\1 = \2;"), (re.compile(P_ASSIGN0), r'\1 = "";'),
          (re.compile(P_REL1), r"RELEASE(\2);"), (re.compile(P_REL2), r"RELEASE(\2);"), (re.compile(P_REL3), r"RELEASE(\2);"),
          (re.compile(P_ADD1), r"ADDREF(\1);"), (re.compile(P_ADD2), r"ADDREF(\1);"),
          (re.compile(P_STRFREE), ""), (re.compile(r"\n\s*/\* WARNING: Subroutine does not return \*/"), "")]
VCALL = re.compile(r"\(\*\*\(code \*\*\)\(\*(.+?) \+ (0x[0-9a-f]+|\d+)\)\)\(\(undefined \*\)\1(,|\))")
VCALL0 = re.compile(r"\(\*\(code \*\)\*\*\(undefined4 \*\*\)(.+?)\)\(\(undefined \*\)\1(,|\))")
SUFFIX = {"float": "_f", "char": "_b", "byte": "_b", "bool": "_b", "undefined1": "_b", "short": "_w", "ushort": "_w",
          "undefined2": "_w", "double": "_d", "undefined8": "_d", "longlong": "_d"}
FIELD = re.compile(r"\*\((\w+) \*\*?\)\(\(int\)(\w+) \+ (0x[0-9a-f]+|\d+)\)|\*\((\w+) \*\*?\)\((\w+) \+ (0x[0-9a-f]+|\d+)\)")


EXE = os.path.join(os.path.dirname(ALL_C), "..", "bin", "FeedingFrenzy.exe")
_SECTIONS = []


def read_exe(va, n):
    """n bytes of the exe at the address va, or None."""
    if not _SECTIONS:
        d = open(EXE, "rb").read()
        pe = struct.unpack_from("<I", d, 0x3C)[0]
        count = struct.unpack_from("<H", d, pe + 6)[0]
        opt = struct.unpack_from("<H", d, pe + 20)[0]
        base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
        for i in range(count):
            vs, rva, rs, ro = struct.unpack_from("<IIII", d, pe + 24 + opt + i * 40 + 8)
            _SECTIONS.append((base + rva, rs, d[ro:ro + rs]))
    for lo, size, data in _SECTIONS:
        if lo <= va and va + n <= lo + size:
            return data[va - lo:va - lo + n]
    return None


def short_strings(line):
    """A literal of n characters Ghidra left as &DAT_x, with its length after it, read from the exe."""
    def lit(m):
        b = read_exe(int(m.group(2), 16), int(m.group(3), 0))
        if b is None or not all(32 <= c < 127 for c in b):
            return m.group(0)
        return '%s"%s",%s' % (m.group(1), b.decode("ascii").replace("\\", "\\\\").replace('"', '\\"'), m.group(3))
    return re.sub(r"(\((?:undefined4|byte|char) \*\))&DAT_([0-9a-f]{8}),(0x[0-9a-f]+|\d+)(?=\))", lit, line)


def compact(text):
    text = re.sub(r"\)\n\s+\(", ")(", text)  # Ghidra breaks a long call between the callee and its arguments
    for pat, rep in IDIOMS:
        text = pat.sub(rep, text)
    for _ in range(3):
        text = VCALL.sub(lambda m: "%s->v%x(%s" % (m.group(1), int(m.group(2), 0), "" if m.group(3) == "," else ")"), text)
        text = VCALL0.sub(lambda m: "%s->v0(%s" % (m.group(1), "" if m.group(2) == "," else ")"), text)

    def field(m):
        typ, base, off = (m.group(1), m.group(2), m.group(3)) if m.group(1) else (m.group(4), m.group(5), m.group(6))
        return "%s->m%x%s" % (base, int(off, 0), SUFFIX.get(typ, ""))
    for _ in range(3):
        text = FIELD.sub(field, text)
    return text


def vtable(va):
    names = {}
    with open(os.path.join(os.path.dirname(ALL_C), "functions.jsonl"), encoding="utf-8") as f:
        for line in f:
            m = re.match(r'\{"addr":"([0-9a-f]{8})","name":"([^"]*)"', line)
            if m:
                names[int(m.group(1), 16)] = m.group(2)
    out = []
    i = 0
    while True:
        b = read_exe(va + 4 * i, 4)
        v = struct.unpack("<I", b)[0] if b else 0
        if not 0x401000 <= v < 0x4f8d4e:
            break
        n = names.get(v, "")
        out.append("%02x:%x%s" % (4 * i, v, "" if not n or n.startswith("FUN_") else "=" + n))
        i += 1
    print("%x %d: %s" % (va, i, " ".join(out)))


def main():
    args = sys.argv[1:]
    if args and args[0] == "--vt":
        for a in args[1:]:
            vtable(int(a, 16))
        return
    raw = "--raw" in args
    wanted = {int(a, 16) for a in args if a != "--raw"}
    out, keep = [], False
    with open(ALL_C, encoding="utf-8", errors="replace") as f:
        for line in f:
            m = HEAD.match(line)
            if m:
                keep = int(m.group(1), 16) in wanted
                if keep:
                    out.append([line.rstrip()])
                continue
            if keep:
                out[-1].append(line.rstrip())
    for body in out:
        if raw:
            print("\n".join(body))
            continue
        body = [re.sub(r"\(ExceptionList = [^,()]+, ", "(", l) for l in body]
        # the exception frame's unwind state: the first local set to -1, before the handler's address is stored
        unwind = None
        for a, b in zip(body, body[1:]):
            m = re.match(r"^\s*(\w+) = 0xffffffff;$", a)
            if m and re.match(r"^\s*\w+ = &LAB_[0-9a-f]+;$", b):
                unwind = m.group(1)
                break
        if unwind:
            drop = re.compile(r"^\s*(%s = (0x)?[0-9a-f]+;|\w+ = &LAB_[0-9a-f]+;|\w+ = &stack0x[0-9a-f]+;)$" % unwind)
            body = [l for l in body if not drop.match(l)]
        body = [short_strings(l) for l in body]
        text = [l for l in body if l.strip() and not NOISE.search(l)]
        code = "\n".join(l for l in text if not re.match(r"^\s+[\w ]+\*? +\*?\w+(\s*\[[0-9a-fx]+\])?;$", l))
        used = set(re.findall(r"\b\w+\b", code))
        final = []
        for l in text:
            d = re.match(r"^\s+[\w ]+?\*? *\*?(\w+)(\s*\[[0-9a-fx]+\])?;$", l)
            # a declaration has a type and a name (break; and continue; are statements)
            if d and len(re.findall(r"\w+", l.split("[")[0])) >= 2 and d.group(1) not in used                     and not l.strip().startswith("return"):
                continue
            final.append(l)
        print(compact("\n".join(final)))
        print()


if __name__ == "__main__":
    main()
