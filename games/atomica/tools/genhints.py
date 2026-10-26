#!/usr/bin/env python3
"""Generate games/atomica/src/HintData.inc: the lines of the 28 tutorial boxes (Atomica.exe 0x42a810, the box's draw).

The draw is one long switch on the hint; each case sets a colour, draws a line of text at a height from the box's top
and, where a word is lit, draws it again in white after the width of the text before it. The decompilation reads the
texts inline and drops some arguments, so the texts' addresses come from the disassembly: a text is built with
`push <address>; lea ecx,[ebp-<slot>]; call 0x4037b0`, and the decompiler's local_<slot+4> is the same slot.

The output holds numbers only (colours, heights, addresses of the exe's texts): no text. It also lists the addresses
in tools/extra_strings.txt, because Ghidra's string list misses short strings such as the text before a lit word;
make_gametext.py reads that file, so run it again after this one.

    python games/atomica/tools/genhints.py     (from the repository root; needs re/atomica and llvm-objdump)
"""
import glob
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", ".."))
EXE = os.path.join(ROOT, "re", "atomica", "bin", "Atomica.exe")
FIRST = 0x42a810
LAST = 0x42fb30
HINTS = 28


def find_objdump():
    if os.environ.get("OBJDUMP"):
        return os.environ["OBJDUMP"]
    for pattern in ("E:/retools/llvm-mingw-*/bin/llvm-objdump.exe", "/e/retools/llvm-mingw-*/bin/llvm-objdump.exe"):
        found = glob.glob(pattern)
        if found:
            return found[0]
    return "llvm-objdump"


def unescape(s):
    return (s.replace("\\'", "'").replace('\\"', '"').replace("\\n", "\n").replace("\\t", "\t")
            .replace("\\\\", "\\"))


def exe_string(exe, va):
    o = va - 0x400000
    return exe[o:exe.index(b"\0", o)].decode("latin1")


def main():
    exe = open(EXE, "rb").read()
    dec = subprocess.run([sys.executable, os.path.join(HERE, "decread.py"), "%x-%x" % (FIRST, LAST)], cwd=ROOT,
                         capture_output=True, text=True, check=True).stdout
    asm = subprocess.run([find_objdump(), "-d", "--no-show-raw-insn", "-M", "intel", "--start-address=0x%x" % FIRST,
                          "--stop-address=0x%x" % LAST, EXE], capture_output=True, text=True, check=True).stdout

    # slot (ebp-<slot>) -> address of the text built there
    slot_text = {}
    push = None
    lea = None
    for line in asm.split("\n"):
        m = re.search(r"\bpush\s+(0x4[ab][0-9a-f]{4})\b", line)
        if m:
            push = int(m.group(1), 16)
        m = re.search(r"lea\s+ecx, \[ebp - (0x[0-9a-f]+)\]", line)
        if m:
            lea = int(m.group(1), 16)
        if re.search(r"call\s+0x4037b0\b", line):
            if push is None or lea is None:
                sys.exit("string build without address: " + line)
            slot_text.setdefault(lea, set()).add(push)

    # the decompiler's statements, continuation lines joined
    body = dec[dec.index("switch(local_14.d68) {"):]
    stmts = []
    for line in body.split("\n"):
        if re.match(r"\s{12,}\S", line) and stmts:
            stmts[-1] += " " + line.strip()
        else:
            stmts.append(line.strip())

    ops = {}
    case = None
    collecting = False
    last_y = 0
    last_pref = None
    width_src = {}
    draws = 0
    used = set()
    for s in stmts:
        m = re.match(r"case (0x[0-9a-f]+|\d+):$", s)
        if m:
            case = int(m.group(1), 0)
            ops[case] = []
            collecting = True
            last_y = 0
            last_pref = None
            width_src = {}
            strings = {}
            continue
        if case is None or not collecting:
            continue
        if s.startswith("default:") or s.startswith("}"):
            break
        if (re.search(r"FUN_00436070\(param_1,1\)", s) or s.startswith("pvVar7 = local_14;") or s.startswith("break;")
                or s.startswith("return;") or s.startswith("goto ")):
            collecting = False
            continue
        m = re.search(r"FUN_004371a0\(local_[0-9a-f]+,(\w+),(\w+),(\w+),([\w.]+)\)", s)
        if m:
            if "d7c" not in m.group(4):
                sys.exit("alpha is not the box's: " + s)
            ops[case].append(("color", int(m.group(1), 0), int(m.group(2), 0), int(m.group(3), 0)))
            continue
        m = re.search(r'FUN_004037b0\((local_[0-9a-f]+),"(.*)"\);', s)
        if m:
            slot = int(m.group(1)[6:], 16) - 4
            addrs = slot_text.get(slot)
            if not addrs:
                sys.exit("no address for " + s)
            text = unescape(m.group(2))
            found = [a for a in addrs if exe_string(exe, a) == text]
            if len(found) != 1:
                sys.exit("text does not match the exe at its slot: " + s)
            strings[m.group(1)] = found[0]
            continue
        m = re.match(r"puVar8 = (local_[0-9a-f]+);", s)
        if m:
            last_pref = m.group(1)
            continue
        m = re.match(r"(\w+) = FUN_00436970\(", s)
        if m:
            width_src[m.group(1)] = last_pref
            continue
        m = re.search(r"FUN_00436920\(param_1,(local_[0-9a-f]+),([^,]+),(.+)\);", s)
        if m:
            draws += 1
            var, x, y = m.group(1), m.group(2).strip(), m.group(3).strip()
            prefix = 0
            if x != "iVar9":
                mx = re.match(r"(\w+) \+ iVar9$", x)
                if not mx or width_src.get(mx.group(1)) is None:
                    sys.exit("unknown x: " + s)
                prefix = strings[width_src[mx.group(1)]]
                used.add(prefix)
            if y == "iVar10":
                y = 0x17
            elif y == "iVar11":
                y = 0x2b
            elif y == "puVar8":
                y = last_y          # the decompiler lost it: a lit word is on the line drawn before
            else:
                my = re.match(r"iVar6 \+ (0x[0-9a-f]+)$", y)
                if not my:
                    sys.exit("unknown y: " + s)
                y = int(my.group(1), 16)
            last_y = y
            used.add(strings[var])
            ops[case].append(("text", y, strings[var], prefix))
            continue

    if sorted(ops) != list(range(HINTS)):
        sys.exit("cases found: %s" % sorted(ops))
    total = sum(1 for c in ops for o in ops[c] if o[0] == "text")
    # the draw's calls of the draw-string function, less the "Continue" of the button
    asm_draws = len(re.findall(r"call\s+0x436920\b", asm)) - 1
    if total != asm_draws:
        sys.exit("%d texts parsed, %d draws in the code" % (total, asm_draws))

    out = ["// Generated by games/atomica/tools/genhints.py from the draw at 0x42a810: the lines of the tutorial boxes",
           "// (colour, height from the box's top, the exe's address of the text and of the text before a lit word).",
           "// No text: the port reads the texts from the player's exe (GameText.h)."]
    for c in range(HINTS):
        out.append("static const HintOp kHintOps%d[] = {" % c)
        for o in ops[c]:
            if o[0] == "color":
                out.append("\t{HOP_COLOR, %d, %d, %d, 0, 0, 0}," % (o[1], o[2], o[3]))
            else:
                out.append("\t{HOP_TEXT, 0, 0, 0, %d, 0x%x, 0x%x}," % (o[1], o[2], o[3]))
        out.append("};")
    out.append("static const HintLines kHintLines[%d] = {" % HINTS)
    for c in range(HINTS):
        out.append("\t{kHintOps%d, %d}," % (c, len(ops[c])))
    out.append("};")
    open(os.path.join(ROOT, "games", "atomica", "src", "HintData.inc"), "w", newline="\n").write("\n".join(out) + "\n")

    addrs = sorted(used | {0x4b2bd4})
    open(os.path.join(HERE, "extra_strings.txt"), "w", newline="\n").write(
        "# addresses of texts Ghidra's string list lacks (genhints.py); make_gametext.py adds them to the table\n" +
        "".join("0x%08x\n" % a for a in addrs))
    print("%d cases, %d texts, %d addresses" % (HINTS, total, len(addrs)))


main()
