#!/usr/bin/env python3
"""Compact, annotated disassembly of one function, for the places where the decompiler loses x87 arithmetic or an operand.

    python -I games/peggle/tools/disasm.py <function address> [<from> <to>] [--math]

--math keeps only the x87 instructions and the calls (a std::vector's bounds checks and the like are left out).

Each line is "address instruction ; note". Calls show the function's name (the Mac build's names, our own, and the resource
globals), memory operands the name of the global or the constant read from the exe (float, double), strings that are pushed
their text, and the four pushes of a ModVal call become one line, "ModValInt(tag line,column) = default", since the game
ships with every tweakable constant at its default. Needs llvm-objdump (E:/retools/llvm-mingw).
"""
import os
import re
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import decread
import pe

OBJDUMP = "E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-objdump.exe"
MODVALS = {0x523c70: "ModValInt", 0x523ca0: "ModValDouble", 0x523cd0: "ModValFloat", 0x523d00: "ModValString"}
FLOAT_OPS = ("fld", "fmul", "fadd", "fsub", "fsubr", "fdiv", "fdivr", "fcom", "fcomp", "fild", "fimul", "fiadd")


def function_bounds(address):
    import json
    with open(os.path.join(pe.ROOT, "re", "peggle", "deluxe", "gh", "functions.jsonl")) as f:
        for line in f:
            j = json.loads(line)
            if int(j["addr"], 16) == address:
                return address, address + j["size"]
    raise SystemExit("function %x not found" % address)


def read_c_string(image, address):
    out = bytearray()
    while len(out) < 200:
        b = image.bytes(address, 1)[0]
        if b == 0:
            return out.decode("latin-1")
        out.append(b)
        address += 1
    return out.decode("latin-1")


def tag_of(image, address):
    text = read_c_string(image, address)
    return re.sub(r"^(SEXY_SEXYMODVAL)?c:.*\\", "", text)


def main():
    math_only = "--math" in sys.argv
    if math_only:
        sys.argv.remove("--math")
    start = int(sys.argv[1], 16)
    low, high = function_bounds(start)
    if len(sys.argv) >= 4:
        low, high = int(sys.argv[2], 16), int(sys.argv[3], 16)
    names = decread.load_symbols()
    image = pe.Image()
    dump = subprocess.run([OBJDUMP, "-d", "--x86-asm-syntax=intel", "--start-address=0x%x" % start, "--stop-address=0x%x" % function_bounds(start)[1],
                           os.path.join(pe.ROOT, "re", "peggle", "deluxe", "bin", "PeggleGame.exe")], capture_output=True, text=True).stdout
    instructions = []
    for line in dump.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s*\t?(.*)$", line)
        if m:
            text = re.sub(r"\s*<\.text\+0x[0-9a-f]+>", "", m.group(2)).strip()
            text = re.sub(r"\s+", " ", text)
            instructions.append((int(m.group(1), 16), text))
    skip = 0
    for index, (address, text) in enumerate(instructions):
        if address < low or address >= high:
            continue
        if skip:
            skip -= 1
            continue
        note = ""
        m = re.match(r"call 0x([0-9a-f]+)", text)
        if m:
            target = int(m.group(1), 16)
            if target in MODVALS:
                # push default, push tag, push 0 come just before; collapse them with the call
                pushes = []
                j = index - 1
                while j >= 0 and len(pushes) < 3:
                    mm = re.match(r"push (-?0x[0-9a-f]+)$", instructions[j][1])
                    if mm:
                        pushes.append(int(mm.group(1), 16))
                    j -= 1
                if len(pushes) == 3 and pushes[0] == 0:
                    tag = tag_of(image, pushes[1] & 0xffffffff)
                    default = pushes[2]
                    if MODVALS[target] == "ModValInt":
                        if default >= 0x80000000:
                            default -= 1 << 32
                        note = "%s(%s) default %d (%#x)" % (MODVALS[target], tag, default, default & 0xffffffff)
                    else:
                        note = "%s(%s)" % (MODVALS[target], tag)
                else:
                    note = MODVALS[target]
            else:
                note = names.get("%08x" % target, "")
        else:
            m = re.search(r"(dword|qword|byte|word) ptr \[0x([0-9a-f]+)\]", text)
            if m:
                target = int(m.group(2), 16)
                name = names.get("%08x" % target, "")
                value = ""
                try:
                    if text.split()[0] in FLOAT_OPS and 0x5d2000 <= target < 0x68b000:
                        if m.group(1) == "dword" and not text.startswith("fild") and not text.startswith("fimul") and not text.startswith("fiadd"):
                            value = "%g" % struct.unpack("<f", image.bytes(target, 4))[0]
                        elif m.group(1) == "qword":
                            value = "%g" % struct.unpack("<d", image.bytes(target, 8))[0]
                except Exception:
                    pass
                note = name or value
                if name and value:
                    note += " " + value
            else:
                m = re.match(r"push (0x[0-9a-f]+)$", text)
                if m and 0x5d2000 <= int(m.group(1), 16) < 0x68b000:
                    try:
                        s = tag_of(image, int(m.group(1), 16))
                        if s and all(32 <= ord(c) < 127 for c in s):
                            note = '"%s"' % s
                    except Exception:
                        pass
        if math_only and not (text.startswith("f") or text.startswith("call") and "0x59cd0b" not in text):
            continue
        print("%08x %-34s%s" % (address, text, ("; " + note) if note else ""))


main()
