#!/usr/bin/env python3
"""Print functions of Talismania's export (re/talismania/gh/all.c, git-ignored) for reading, compacted:
without blank lines, the exception-frame bookkeeping, the return-address stores and the declarations of locals the body
never uses; with the engine's idioms collapsed: RELEASE(p) for a counted reference released through its virtual base,
ADDREF(p) for one added, p->v3c(...) for a call of a virtual slot (by its byte offset), p->m44 for a field (m44_f a
float, m44_b a byte, m44_w a short, m44_d a double), and the std::string's destructor (a free when its capacity is
over 15) left out.

    python -I games/talismania/tools/fn.py <addr> [<addr>...]     (hex, with or without 0x)
    python -I games/talismania/tools/fn.py --raw <addr>            as Ghidra wrote it
    python -I games/talismania/tools/fn.py --asm <addr>            the function's disassembly (llvm-objdump, Intel)
    python -I games/talismania/tools/fn.py --info <addr>           name, size, callers, callees and strings
    python -I games/talismania/tools/fn.py --vt <addr>             a vtable's slots, read from the exe up to the
                                                                       first word that is not an address in the code
The export is read from the worktree's re/talismania when it has one, else from the main checkout's.
"""
import os
import json
import re
import struct
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
COMMON = subprocess.check_output(["git", "rev-parse", "--git-common-dir"], cwd=HERE, text=True).strip()
MAIN = os.path.dirname(os.path.abspath(os.path.join(HERE, COMMON)))
RE = os.path.join(ROOT, "re", "talismania")
if not os.path.isfile(os.path.join(RE, "gh", "all.c")):
    RE = os.path.join(MAIN, "re", "talismania")
ALL_C = os.path.join(RE, "gh", "all.c")
INDEX = os.path.join(RE, "gh", "all.idx")
OBJDUMP = r"E:\retools\llvm-mingw-20261006-ucrt-x86_64\bin\llvm-objdump.exe"
HEAD = re.compile(r"^// ([0-9a-f]{8})  ")
NOISE = re.compile(r"member function inherited by |^\s*(\w+ = ExceptionList|ExceptionList = [^;]+);$|^\s*local_4(\._0_1_)? = |^\s*puStack_8 = |^\s*pvStack_c = "
                   r"|^/\* WARNING: Function: __EH_(prolog|epilog)3|^/\* WARNING: Globals starting with"
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


EXE = os.path.join(MAIN, "re", "talismania", "bin", "TalismaniaGame.exe")
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
    # the checked STL's tests (FUN_004a9ee6 is _invalid_parameter_noinfo): a block that only reports and goes on
    text = CHECKED.sub("", text)
    text = re.sub(r"\n\s*FUN_004a9ee6\(\);(?=\n)", "", text)
    # a std::deque's back(): the index of its last element wrapped around its map, then the map's entry
    text = DEQUE_BACK.sub(lambda m: "\n%s%s = BACK(%s)%s" % (m.group(1), m.group(5), m.group(3), m.group(6)), text)
    text = DEQUE_INDEX.sub("", text)
    text = drawcontext_back(text)
    return text


def drawcontext_back(text):
    """The draw context's state stack (a std::deque of pointers: its map at +0x28, the map's size +0x2c, the first
    element's index +0x30, the count +0x34) read at its back: the index arithmetic dropped, the entry named BACK."""
    fields = re.compile(r"\(this->DrawContext_data\)\.(?:offset_0x3[04]|field_0x2c)")
    entry = re.compile(r"\*\(int \*\)\(\*\(int \*\)&\(this->DrawContext_data\)\.field_0x28 \+ (?:\(int\))?(\w+) \* 4\)")
    lines = text.split("\n")
    index_vars = set()
    for line in lines:
        m = re.match(r"\s*(\w+) = .*;", line)
        if m and fields.search(line):
            index_vars.add(m.group(1))
    out = []
    for line in lines:
        m = re.match(r"\s*(\w+) = ", line)
        if fields.search(line) and (m or line.strip().startswith("if (")):
            if line.strip().startswith("if (") and line.rstrip().endswith("{"):
                out.append("  if (DROP) {")
            elif line.strip().startswith("if ("):
                out.append("  if (DROP)")
            continue
        if m and m.group(1) in index_vars and not entry.search(line):
            continue
        if re.match(r"\s*if \((\w+) <= (\w+)\) \{\s*$", line) and set(re.findall(r"\w+", line)[1:]) & index_vars:
            out.append("  if (DROP) {")
            continue
        out.append(entry.sub("BACK", line))
    text = "\n".join(out)
    for _ in range(3):
        text = re.sub(r"\n\s*if \(DROP\) \{\n\s*\}", "", text)
        text = re.sub(r"\n\s*if \(DROP\)\n\s*\{\n\s*\}", "", text)
        text = re.sub(r"\n\s*if \([^\n]*\) \{\n\s*\}(?!\s*else)", "", text)
    return text


CHECKED = re.compile(r"\n\s*if \([^{};]*\) \{\n\s*FUN_004a9ee6\(\);(?:\n\s*\w+ = \(float10\)\w+;)?\n\s*\}")
DEQUE_BACK = re.compile(r"\n(\s*)(\w+) = (\w+)->m([0-9a-f]+) \+ \3->m[0-9a-f]+;\n\s*\2 = \2 - 1;\n"
                        r"\s*if \(\3->m[0-9a-f]+ <= \2\) \{\n\s*\2 = \2 - \3->m[0-9a-f]+;\n\s*\}\n"
                        r"\s*([^=\n]+?) = \*\([\w *]+\)\(\3->m[0-9a-f]+ \+ \2 \* 4\)(.*)")
DEQUE_INDEX = re.compile(r"(?!x)x")


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
        if not 0x401000 <= v < 0x545000:
            break
        n = names.get(v, "")
        out.append("%02x:%x%s" % (4 * i, v, "" if not n or n.startswith("FUN_") else "=" + n))
        i += 1
    print("%x %d: %s" % (va, i, " ".join(out)))


def functions():
    table = {}
    with open(os.path.join(RE, "gh", "functions.jsonl"), encoding="utf-8") as f:
        for line in f:
            record = json.loads(line)
            table[int(record["addr"], 16)] = record
    return table


def index():
    """The byte offset of each function's header in all.c, built once into all.idx."""
    if os.path.isfile(INDEX) and os.path.getmtime(INDEX) >= os.path.getmtime(ALL_C):
        with open(INDEX, encoding="ascii") as f:
            return {int(a, 16): int(o) for a, o in (l.split() for l in f)}
    offsets = {}
    with open(ALL_C, "rb") as f:
        position = 0
        for line in f:
            if line.startswith(b"// ") and len(line) > 12 and line[11:13] == b"  ":
                try:
                    offsets[int(line[3:11], 16)] = position
                except ValueError:
                    pass
            position += len(line)
    with open(INDEX, "w", encoding="ascii") as f:
        for a in sorted(offsets):
            f.write("%x %d\n" % (a, offsets[a]))
    return offsets


def bodies(wanted):
    offsets = index()
    out = []
    with open(ALL_C, "rb") as f:
        for a in wanted:
            if a not in offsets:
                out.append(["// %08x  (no function starts here)" % a])
                continue
            f.seek(offsets[a])
            lines = [f.readline().decode("utf-8", "replace").rstrip()]
            for raw_line in f:
                line = raw_line.decode("utf-8", "replace").rstrip()
                if HEAD.match(line):
                    break
                lines.append(line)
            out.append(lines)
    return out


def rtti_names():
    """The virtual functions' names from tools/rtti_msvc70.py's symbols (re/talismania/rtti/tal.jsonl), by address."""
    names = {}
    path = os.path.join(RE, "rtti", "tal.jsonl")
    if os.path.isfile(path):
        with open(path, encoding="utf-8") as f:
            for line in f:
                record = json.loads(line)
                if "name" in record:
                    names[int(record["addr"], 16)] = record["name"]
    return names


RTTI = rtti_names()


def annotate(text):
    """A call of FUN_<addr> where RTTI names the function: its class and slot after it."""
    def name(m):
        a = int(m.group(1), 16)
        return "FUN_%s[%s]" % (m.group(1), RTTI[a]) if a in RTTI else m.group(0)
    return re.sub(r"FUN_([0-9a-f]{8})", name, text)


def info(addresses):
    table = functions()
    strings = {}
    with open(os.path.join(RE, "gh", "strings.jsonl"), encoding="utf-8") as f:
        for line in f:
            record = json.loads(line)
            for x in record["xrefs"]:
                if ":" not in x:
                    strings.setdefault(int(x, 16), []).append(record["value"])

    def name(x):
        if ":" in x or int(x, 16) not in table:
            return x
        return "%s=%s" % (x.lstrip("0"), annotate(table[int(x, 16)]["full"]))
    for a in addresses:
        record = table.get(a)
        if not record:
            print("%x: no function" % a)
            continue
        end = a + record["size"]
        texts = [s for x in range(a, end) for s in strings.get(x, [])]
        print("%x %s size %d" % (a, record["full"], record["size"]))
        print("  sig     " + record["sig"])
        print("  callers " + " ".join(name(c) for c in record["callers"][:40]))
        print("  callees " + " ".join(name(c) for c in record["callees"][:60]))
        if texts:
            print("  strings " + " | ".join(t.replace("\n", "\\n")[:60] for t in texts[:30]))


def disassemble(addresses):
    table = functions()
    for a in addresses:
        size = table[a]["size"] if a in table else 0x40
        result = subprocess.run([OBJDUMP, "-d", "--no-show-raw-insn", "-M", "intel", "--start-address=0x%x" % a,
                                 "--stop-address=0x%x" % (a + size), EXE], capture_output=True, text=True)
        lines = [l for l in result.stdout.splitlines() if re.match(r"^\s+[0-9a-f]+:", l)]
        print("\n".join(lines))
        print()


def main():
    args = sys.argv[1:]
    if args and args[0] == "--vt":
        for a in args[1:]:
            vtable(int(a, 16))
        return
    if args and args[0] == "--info":
        info([int(a, 16) for a in args[1:]])
        return
    if args and args[0] == "--asm":
        disassemble([int(a, 16) for a in args[1:]])
        return
    raw = "--raw" in args
    wanted = [int(a, 16) for a in args if a != "--raw"]
    for body in bodies(wanted):
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
