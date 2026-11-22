#!/usr/bin/env python3
"""Every call in one function with the arguments it gets: the registers set before it and the pushes, in order.

    python -I games/bananabugs/tools/callargs.py <function address> [--all] [--only <callee>,<callee>...]

The original passes many arguments in registers (link-time code generation), which the decompile loses. This walks
the function's code in address order and keeps, for each register and for the x87 stack, the last thing written to it
(a constant, a global, a field such as [esi+0x10], or the name of another register's value), so each call prints as

    0045ed12 call 004533c0 FUN_004533c0(1.0, 0, 0, 0, 0, 0) eax=0x1 esi=0x2 edi=<new 0x13c>

Pushed arguments are listed first to last as the callee sees them; dwords that look like floats print as floats.
Registers show only when written since the last call (--all shows the stale ones too, marked "~"). It reads
straight-line code: at a join after a branch the value is the one on the path just above, so a register set in only
one arm of an `if` shows that arm's value. Good enough to read builders of keys and objects; check branches by hand.
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
REGS = ["eax", "ecx", "edx", "ebx", "esi", "edi", "ebp"]
LOW = {"al": "eax", "cl": "ecx", "dl": "edx", "bl": "ebx"}


def bounds(address):
    import json
    with open(os.path.join(pe.ROOT, "re", "bananabugs", "game", "functions.jsonl")) as f:
        for line in f:
            j = json.loads(line)
            if int(j["addr"], 16) == address:
                return address, address + j["size"]
    raise SystemExit("function %x not found" % address)


def as_value(text, image):
    """A constant as the code means it: small ints as ints, float bit patterns as floats, strings as text."""
    m = re.fullmatch(r"(-?0x[0-9a-f]+|\d+)", text)
    if not m:
        return text
    v = int(text, 0) & 0xffffffff
    if 0x5ca000 <= v < 0x6a8000:
        try:
            raw = image.bytes(v, 80)
            s = raw.split(b"\0")[0]
            if len(s) >= 3 and all(32 <= c < 127 for c in s):
                return '"%s"' % s.decode()
        except Exception:
            pass
    if v >= 0x80000000 and v > 0xffff0000:
        return str(v - (1 << 32))
    if 0x30000000 <= v < 0x50000000 or 0xb0000000 <= v < 0xd0000000:
        f = struct.unpack("<f", struct.pack("<I", v))[0]
        return "%gf" % f
    return hex(v) if v > 9 else str(v)


def mem_value(operand, image, names):
    m = re.search(r"(dword|qword) ptr \[0x([0-9a-f]+)\]", operand)
    if m:
        target = int(m.group(2), 16)
        name = names.get("%08x" % target, "")
        if name:
            return name
        if 0x5ca000 <= target < 0x600000:
            if m.group(1) == "qword":
                return "%g" % struct.unpack("<d", image.bytes(target, 8))[0]
            return "%gf" % struct.unpack("<f", image.bytes(target, 4))[0]
        return "[%x]" % target
    m = re.search(r"\[(.*)\]", operand)
    return "[%s]" % m.group(1) if m else operand


def main():
    args = sys.argv[1:]
    show_all = "--all" in args
    only = None
    if "--only" in args:
        only = set(int(a, 16) for a in args[args.index("--only") + 1].split(","))
    start = int(args[0], 16)
    low, high = bounds(start)
    names = decread.load_symbols()
    image = pe.Image()
    dump = subprocess.run([OBJDUMP, "-d", "--x86-asm-syntax=intel", "--start-address=0x%x" % low,
                           "--stop-address=0x%x" % high,
                           os.path.join(pe.ROOT, "re", "bananabugs", "bin", "BananaBugsGame.exe")],
                          capture_output=True, text=True).stdout
    regs = {r: ("~" + r, False) for r in REGS}
    fpu = []
    pushes = []
    joins = {}  # address -> register state at an unconditional jump to it (the "then" arm of an if/else)
    for line in dump.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s*\t?(.*)$", line)
        if not m:
            continue
        address = int(m.group(1), 16)
        text = re.sub(r"\s*<\.text\+0x[0-9a-f]+>", "", m.group(2)).strip()
        text = re.sub(r"\s+", " ", text)
        op, _, rest = text.partition(" ")
        if address in joins:
            regs = dict(joins.pop(address))
        if op == "jmp" and rest.startswith("0x"):
            joins[int(rest, 16)] = dict(regs)
        operands = [o.strip() for o in re.split(r",(?![^\[]*\])", rest)] if rest else []

        def value_of(operand):
            if operand in regs:
                return regs[operand][0]
            if operand in LOW:
                return regs[LOW[operand]][0]
            if "ptr" in operand:
                inner = mem_value(operand, image, names)
                for r in REGS:
                    inner = re.sub(r"\b%s\b" % r, "%s" % r, inner)
                return inner
            return as_value(operand, image)

        if op == "call":
            target = operands[0]
            t = int(target, 16) if target.startswith("0x") else None
            if only is None or t in only:
                label = names.get("%08x" % t, "") if t else target
                arglist = ", ".join(reversed(pushes))
                shown = []
                for r in REGS:
                    v, fresh = regs[r]
                    if fresh or show_all:
                        shown.append("%s=%s%s" % (r, "" if fresh else "~", v))
                fp = (" st0=" + fpu[-1]) if fpu else ""
                print("%08x call %s %s(%s) %s%s" % (address, ("%08x" % t) if t else target, label, arglist,
                                                   " ".join(shown), fp))
            pushes = []
            for r in ("eax", "ecx", "edx"):
                regs[r] = ("ret:%x" % t if (r == "eax" and t) else "?", False)
            for r in REGS:
                regs[r] = (regs[r][0], False)
            fpu = []
            continue
        if op == "push":
            pushes.append(value_of(operands[0]))
            continue
        if op == "sub" and operands[0] == "esp":
            pushes.extend(["?"] * (int(operands[1], 0) // 4))
            continue
        if op == "add" and operands[0] == "esp":
            n = int(operands[1], 0) // 4
            if n and n <= len(pushes):
                del pushes[-n:]
            continue
        if op in ("fld", "fild", "fld1", "fldz"):
            if op == "fld1":
                fpu.append("1.0")
            elif op == "fldz":
                fpu.append("0.0")
            elif operands[0].startswith("st("):
                k = int(operands[0][3:-1])
                fpu.append(fpu[-1 - k] if k < len(fpu) else "?")
            else:
                v = value_of(operands[0])
                fpu.append(("(float)int " + v) if op == "fild" else v)
            continue
        if op in ("fstp", "fst") and operands and "esp" in operands[0]:
            mm = re.search(r"\[esp(?: \+ (0x[0-9a-f]+|\d+))?\]", operands[0])
            top = fpu[-1] if fpu else "?"
            if mm is not None and "qword" not in operands[0]:
                k = int(mm.group(1), 0) // 4 if mm.group(1) else 0
                if k < len(pushes):
                    pushes[len(pushes) - 1 - k] = top
            if op == "fstp" and fpu:
                fpu.pop()
            continue
        if op in ("fmul", "fadd", "fsub", "fdiv", "fsubr", "fdivr") and len(operands) == 1 and fpu:
            sym = {"fmul": "*", "fadd": "+", "fsub": "-", "fdiv": "/", "fsubr": "r-", "fdivr": "r/"}[op]
            v = value_of(operands[0])
            if sym.startswith("r"):
                fpu[-1] = "(%s %s %s)" % (v, sym[1], fpu[-1])
            else:
                fpu[-1] = "(%s %s %s)" % (fpu[-1], sym, v)
            continue
        if op.startswith("f"):
            if op.endswith("p") and fpu and op not in ("fxch",):
                fpu.pop()
            continue
        if op in ("mov", "movzx", "movsx") and len(operands) == 2:
            dest = operands[0]
            v = value_of(operands[1])
            if "ptr" in operands[1] and operands[1].startswith("byte"):
                v = "(u8)" + v
            if dest in regs:
                regs[dest] = (v, True)
            elif dest in LOW:
                regs[LOW[dest]] = (v, True)
            continue
        if op == "lea" and operands[0] in regs:
            mm = re.fullmatch(r"\[(\w+)(?: \+ (e[a-z]{2}))?(?: ([+-]) (0x[0-9a-f]+|\d+))?\]", operands[1])
            folded = None
            if mm and mm.group(1) in regs and not mm.group(2):
                base = regs[mm.group(1)][0]
                if re.fullmatch(r"-?(0x[0-9a-f]+|\d+)", base):
                    k = int(mm.group(4), 0) if mm.group(4) else 0
                    folded = str(int(base, 0) + (k if mm.group(3) != "-" else -k))
            regs[operands[0]] = (folded or ("&" + mem_value(operands[1], image, names)), True)
            continue
        if op == "xor" and len(operands) == 2 and operands[0] == operands[1]:
            r = LOW.get(operands[0], operands[0])
            if r in regs:
                regs[r] = ("0", True)
            continue
        if op in ("add", "sub", "inc", "dec", "imul", "shl", "sar", "shr", "and", "or", "neg", "not", "setne",
                  "sete", "cdq", "idiv", "div") and operands:
            r = LOW.get(operands[0], operands[0])
            if r in regs:
                before = regs[r][0]
                if op in ("add", "sub") and len(operands) == 2:
                    regs[r] = ("(%s %s %s)" % (before, "+" if op == "add" else "-", value_of(operands[1])), True)
                else:
                    regs[r] = ("%s(%s)" % (op, before), True)
            continue


main()
