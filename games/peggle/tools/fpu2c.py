#!/usr/bin/env python3
"""Reads the x87 code of one function as C-like statements, for the places where the decompiler loses the order of an
expression, which sine is which, or what a compare tests. Every instruction that is not floating point is printed as it
is (after a "//"), the floating point ones become statements that keep the stack's expressions, so what is rounded to a
float and what stays a double is plain:

    python -I games/peggle/tools/fpu2c.py <function address> [<from> <to>] [--this esi] [--terse]

--terse leaves out the plain integer instructions except calls and compares

    f32(esi+0x54) = (float)(f32(esi+0x5c) + ...);      a store to a float (fstp dword)
    f64(...)  and  i32(...)                            a double, a 32-bit integer in memory
    eax = (int)(EXPR);                                   a call to _ftol (0x59c5a0) truncates
    if (st0 <= f32(esi+0x3c)) goto L479234;              the compare, fnstsw and test ah idiom read out

The MSVC runtime sets the x87 to 53 bits, so the arithmetic is double precision and only a store to a dword rounds to float:
write the port's expression with (double) casts and one (float) around what is stored. The runtime calls are named: 0x59bdd0
cos, 0x59c070 sin, 0x59c5a0 ftol, 0x59e56a atan2 (st1 over st0), 0x59bffa fmod, 0x59e6a0 sqrt. A jump table is read out
and the stack at every target is the one at the jump. Needs llvm-objdump like disasm.py does.
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


def function_bounds(address):
    import json
    with open(os.path.join(pe.ROOT, "re", "peggle", "deluxe", "gh", "functions.jsonl")) as f:
        for line in f:
            j = json.loads(line)
            if int(j["addr"], 16) == address:
                return address, address + j["size"]
    raise SystemExit("function %x not found" % address)


def instructions_of(address):
    low, high = function_bounds(address)
    dump = subprocess.run([OBJDUMP, "-d", "--x86-asm-syntax=intel", "--start-address=0x%x" % low, "--stop-address=0x%x" % high,
                           os.path.join(pe.ROOT, "re", "peggle", "deluxe", "bin", "PeggleGame.exe")], capture_output=True, text=True).stdout
    out = []
    for line in dump.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s*	?(.*)$", line)
        if m:
            text = re.sub(r"\s*<\.text\+0x[0-9a-f]+>", "", m.group(2)).strip()
            text = re.sub(r"\s+", " ", text)
            out.append((int(m.group(1), 16), text))
    return out

CALLS = {0x59bdd0: ("cos", 1), 0x59c070: ("sin", 1), 0x59e6a0: ("sqrt", 1), 0x59e56a: ("atan2", 2), 0x59bffa: ("fmod", 2)}
FTOL = 0x59c5a0


def parse_mem(text, this_reg):
    """'dword ptr [esi + 0x5c]' -> (width, address expression) or None."""
    m = re.search(r"(byte|word|dword|qword|tbyte) ptr \[([^\]]+)\]", text)
    if not m:
        return None
    inner = m.group(2).replace(" ", "")
    return m.group(1), inner


def const_value(image, width, inner, integer):
    m = re.fullmatch(r"0x([0-9a-f]+)", inner)
    if not m:
        return None
    address = int(m.group(1), 16)
    try:
        if integer:
            return str(struct.unpack("<i", image.bytes(address, 4))[0]) if width == "dword" else None
        if width == "dword":
            return "%r" % struct.unpack("<f", image.bytes(address, 4))[0]
        if width == "qword":
            return "%r" % struct.unpack("<d", image.bytes(address, 8))[0]
    except Exception:
        return None
    return None


def main():
    args = sys.argv[1:]
    this_reg = None
    terse = "--terse" in args
    if terse:
        args.remove("--terse")
    if "--this" in args:
        i = args.index("--this")
        this_reg = args[i + 1]
        del args[i:i + 2]
    start = int(args[0], 16)
    low, high = function_bounds(start)
    if len(args) >= 3:
        low, high = int(args[1], 16), int(args[2], 16)
    names = decread.load_symbols()
    image = pe.Image()
    instructions = instructions_of(start)
    targets = set()
    for _, text in instructions:
        m = re.match(r"j\w+ 0x([0-9a-f]+)", text)
        if m:
            targets.add(int(m.group(1), 16))
    # jump tables: jmp dword ptr [4*eax + 0x479418]
    tables = {}
    for address, text in instructions:
        m = re.match(r"jmp dword ptr \[(?:4\*\w+|\w+\*4) \+ 0x([0-9a-f]+)\]", text)
        if m:
            table = int(m.group(1), 16)
            entries = []
            while True:
                target = image.u32(table + 4 * len(entries))
                if not (start <= target < function_bounds(start)[1]):
                    break
                entries.append(target)
            tables[address] = entries
            targets.update(entries)

    def mem_name(inner, width):
        text = inner
        if this_reg:
            text = re.sub(r"\b" + this_reg + r"\b", "this", text)
        kind = {"dword": "f32", "qword": "f64", "word": "i16", "byte": "i8"}[width]
        return kind, text

    class Stack(list):
        """The x87 stack: a read below the bottom gives a name for the unknown value instead of failing."""
        def __getitem__(self, index):
            try:
                return list.__getitem__(self, index)
            except IndexError:
                return "st_unknown"

        def __setitem__(self, index, value):
            try:
                list.__setitem__(self, index, value)
            except IndexError:
                pass

        def pop(self, *a):
            return list.pop(self) if len(self) else "st_unknown"

    stack = Stack()
    temp = [0]
    saved = {}
    flags = None  # the compare the next test ah tests: (left, right)
    lines = []

    def emit(s):
        lines.append(s)

    def push(e):
        stack.append(e)

    def tempify(e):
        if len(e) > 60 or "(" in e:
            temp[0] += 1
            name = "t%d" % temp[0]
            emit("    double %s = %s;" % (name, e))
            return name
        return e

    last_cmp = None  # an integer cmp or test: (a, b) for the jcc that follows
    for index, (address, text) in enumerate(instructions):
        if address < low or address >= high:
            continue
        if address in targets:
            if address in saved:
                stack[:] = saved[address]
            elif lines and (lines[-1].strip().startswith('goto') or lines[-1].strip().startswith('return')):
                stack[:] = []
            emit("L%x:" % address)
        op = text.split()[0]
        mem = parse_mem(text, this_reg)
        note = ""
        if op.startswith("f") and op not in ("fnstsw", "fnstcw", "fldcw", "fwait"):
            def operand_value(integer=False):
                width, inner = mem
                c = const_value(image, width, inner, integer)
                if c is not None:
                    return c
                if integer:
                    return "(double)i%s(%s)" % ("32" if width == "dword" else "16", inner if not this_reg else re.sub(r"\b" + this_reg + r"\b", "this", inner))
                kind, name = mem_name(inner, width)
                return "(double)%s(%s)" % (kind, name)

            def store_target(width, inner):
                kind, name = mem_name(inner, width)
                return "%s(%s)" % (kind, name)

            if op == "fld" and mem:
                value = operand_value()
                if re.fullmatch(r"-?[0-9.e+-]+", value):
                    push(value)
                else:
                    temp[0] += 1
                    emit("    double v%d = %s;" % (temp[0], value.replace("(double)", "", 1) if value.startswith("(double)") else value))
                    push("v%d" % temp[0])
            elif op == "fld":
                m = re.search(r"st\((\d)\)", text)
                push(tempify(stack[-1 - int(m.group(1))]))
            elif op == "fild" and mem:
                temp[0] += 1
                emit("    double v%d = %s;" % (temp[0], operand_value(True)))
                push("v%d" % temp[0])
            elif op == "fldz":
                push("0.0")
            elif op == "fld1":
                push("1.0")
            elif op in ("fstp", "fst") and mem:
                width, inner = mem
                value = stack[-1]
                target = store_target(width, inner)
                emit("    %s = %s;" % (target, ("(float)(%s)" % value) if width == "dword" else value))
                if op == "fstp":
                    stack.pop()
            elif op in ("fstp", "fst"):
                m = re.search(r"st\((\d)\)", text)
                index = int(m.group(1))
                if index != 0:
                    stack[-1 - index] = stack[-1]
                if op == "fstp":
                    stack.pop()
            elif op in ("fistp", "fist") and mem:
                width, inner = mem
                emit("    %s = (int)(%s);" % (store_target("dword", inner).replace("f32", "i32"), stack[-1]))
                if op == "fistp":
                    stack.pop()
            elif op == "fxch":
                m = re.search(r"st\((\d)\)", text)
                index = int(m.group(1)) if m else 1
                stack[-1], stack[-1 - index] = stack[-1 - index], stack[-1]
            elif op == "fchs":
                stack[-1] = "-(%s)" % stack[-1]
            elif op == "fabs":
                stack[-1] = "fabs(%s)" % stack[-1]
            elif op == "fsqrt":
                stack[-1] = "sqrt(%s)" % stack[-1]
            elif op in ("fadd", "fsub", "fsubr", "fmul", "fdiv", "fdivr", "fiadd", "fisub", "fisubr", "fimul", "fidiv", "fidivr") and mem:
                base = op[1:]
                integer = base.startswith("i")
                if integer:
                    base = base[1:]
                value = operand_value(integer)
                a = stack[-1]
                symbol = {"add": "+", "sub": "-", "mul": "*", "div": "/", "subr": "-", "divr": "/"}[base]
                stack[-1] = ("(%s %s %s)" % (value, symbol, a)) if base in ("subr", "divr") else ("(%s %s %s)" % (a, symbol, value))
            elif re.match(r"f(add|sub|subr|mul|div|divr)p? st\((\d)\)?", text) or re.match(r"f(add|sub|subr|mul|div|divr)p? st", text):
                pop = op.endswith("p")
                base = op[1:-1] if pop else op[1:]
                regs = re.findall(r"st(?:\((\d)\))?", text)
                dst = int(regs[0] or 0)
                src = int(regs[1] or 0) if len(regs) > 1 else 0
                a = stack[-1 - dst]
                b = stack[-1 - src]
                symbol = {"add": "+", "sub": "-", "mul": "*", "div": "/", "subr": "-", "divr": "/"}[base]
                stack[-1 - dst] = ("(%s %s %s)" % (b, symbol, a)) if base in ("subr", "divr") else ("(%s %s %s)" % (a, symbol, b))
                if pop:
                    stack.pop()
            elif op in ("fcom", "fcomp", "fcompp", "fucom", "fucomp", "fucompp"):
                if mem:
                    right = operand_value()
                else:
                    m = re.search(r"st\((\d)\)", text)
                    right = stack[-1 - int(m.group(1))] if m else (stack[-2] if op.endswith("pp") else stack[-1])
                flags = (stack[-1], right)
                emit("    // compare %s with %s" % flags)
                if op.endswith("pp"):
                    stack.pop()
                    stack.pop()
                elif op.endswith("p"):
                    stack.pop()
            else:
                emit("    // %s" % text)
            continue
        if op == "fnstsw":
            emit("    // fnstsw ax")
            continue
        m = re.match(r"test ah, 0x([0-9a-f]+)", text)
        if m and flags:
            mask = int(m.group(1), 16)
            pending_mask = mask
            emit("    // test ah, %#x" % mask)
            flags_mask = mask
            continue
        m = re.match(r"(cmp|test) ([^,]+), (.+)$", text)
        if m:
            last_cmp = (m.group(1), m.group(2), m.group(3))
        m = re.match(r"(j\w+) 0x([0-9a-f]+)", text)
        if m:
            cond, target = m.group(1), int(m.group(2), 16)
            saved.setdefault(target, list(stack))
            if cond == "jmp":
                emit("    goto L%x;" % target)
                stack[:] = []
            elif flags and "flags_mask" in locals() and cond in ("jne", "je", "jp", "jnp", "jnz", "jz"):
                outcomes = {"less": (1, 0, 0), "equal": (0, 1, 0), "greater": (0, 0, 0)}
                taken = []
                for name, (c0, c3, c2) in outcomes.items():
                    ah = (0x01 if c0 else 0) | (0x40 if c3 else 0) | (0x04 if c2 else 0)
                    value = ah & flags_mask
                    zf = value == 0
                    pf = bin(value).count("1") % 2 == 0
                    jump = {"jne": not zf, "jnz": not zf, "je": zf, "jz": zf, "jp": pf, "jnp": not pf}[cond]
                    if jump:
                        taken.append(name)
                left, right = flags
                words = {("less",): "<", ("equal",): "==", ("greater",): ">", ("less", "equal"): "<=", ("equal", "greater"): ">=",
                         ("less", "greater"): "!="}
                sign = words.get(tuple(taken))
                if sign:
                    emit("    if (%s %s %s) goto L%x;" % (left, sign, right, target))
                else:
                    emit("    if (compare %s,%s is %s) goto L%x;" % (left, right, "/".join(taken), target))
            else:
                shown = "(%s)" % cond
                if last_cmp:
                    kind, a, b = last_cmp
                    ops = {"je": "==", "jz": "==", "jne": "!=", "jnz": "!=", "jl": "<", "jb": "<u", "jle": "<=", "jbe": "<=u",
                           "jg": ">", "ja": ">u", "jge": ">=", "jae": ">=u"}
                    if kind == "test" and a == b and cond in ops:
                        shown = "%s %s 0" % (a, ops[cond]) if cond in ("je", "jz", "jne", "jnz") else "%s %s 0" % (a, ops[cond])
                    elif kind == "cmp" and cond in ops:
                        shown = "%s %s %s" % (a, ops[cond], b)
                    elif cond in ("js", "jns"):
                        shown = "%s %s 0" % (a, "<" if cond == "js" else ">=")
                emit("    if (%s) goto L%x;" % (shown, target))
            continue
        m = re.match(r"jmp dword ptr \[", text)
        if m and address in tables:
            for target in tables[address]:
                saved.setdefault(target, list(stack))
            emit("    switch table: " + ", ".join("L%x" % t for t in tables[address]))
            continue
        m = re.match(r"call 0x([0-9a-f]+)", text)
        if m:
            target = int(m.group(1), 16)
            if target == FTOL:
                emit("    eax = (int)(%s);" % stack.pop())
            elif target in CALLS:
                name, count = CALLS[target]
                operands = [stack.pop() for _ in range(count)][::-1]
                push("%s(%s)" % (name, ", ".join(operands)))
                if name in ("cos", "sin", "sqrt", "atan2", "fmod"):
                    stack[-1] = tempify(stack[-1])
            else:
                label = names.get("%08x" % target, "") or "sub_%x" % target
                emit("    // call 0x%x %s" % (target, label))
                following = instructions[index + 1][1] if index + 1 < len(instructions) else ""
                if following.startswith("f") and not following.startswith("fnstsw"):
                    stack.append("%s(...)" % label)
            continue
        emit("    // %s" % text)
    print("\n".join(lines))


main()
