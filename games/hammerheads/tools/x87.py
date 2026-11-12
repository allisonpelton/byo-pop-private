#!/usr/bin/env python3
"""Reads the x87 code of a function of the original exe, where the decompiler loses the argument of _ftol() (the value is
passed in st0), of fsin/fcos/fsqrt calls, the operand of a compare, and the values of stores: a symbolic evaluator of the
x87 stack over the function's instructions, in order. Branches are not followed: the stack carries on through the
fall-through code, and a line is marked "?" when the stack was not the same on every path in (a label was crossed).

    python games/hammerheads/tools/x87.py 42da00

Each line: the address, then what the instruction does with the values: "_ftol(expr)", "[mem] = expr", "cmp expr, expr",
"call f(expr)". Integer memory (fild) reads as (double)[mem]; [esi+0xa8] is the register's value plus a displacement
(ecx or esi is usually `this`, ebp or esp a local or an argument).
"""
import os
import re
import struct
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import asm
import decread


def operand(text):
    match = re.match(r"(?:(dword|qword|word|tbyte) ptr )?(.*)$", text.strip())
    kind, rest = match.group(1), match.group(2)
    # a constant in the exe's data: its value
    constant = re.match(r"\[(0x5[0-9a-f]{5})(?:<[^>]*>)?\]$", rest)
    if constant and kind in ("qword", "dword"):
        try:
            address = int(constant.group(1), 16)
            if kind == "qword":
                value = struct.unpack("<d", decread.image().bytes(address, 8))[0]
                rest = "%r" % value
            else:
                raw = decread.image().bytes(address, 4)
                value = struct.unpack("<f", raw)[0]
                rest = "%rf" % value
        except Exception:
            pass
    return kind, rest


def main():
    start = int(sys.argv[1], 16)
    names = {int(address, 16): name for address, name in decread.load_symbols().items()}
    lines = asm.disassemble(start, start + asm.sizes()[start])
    targets = set()
    for address, text in lines:
        match = re.match(r"j\w+ 0x([0-9a-f]+)", text)
        if match:
            targets.add(int(match.group(1), 16))
    stack = []
    last_compare = None
    for index, (address, text) in enumerate(lines):
        text = asm.annotate(text, names)
        confused = ""
        if address in targets and stack:
            confused = "?"
        parts = text.split(None, 1)
        op = parts[0]
        arg = parts[1] if len(parts) > 1 else ""
        out = None
        if op in ("fld", "fild", "fild_ll") and arg:
            kind, rest = operand(arg)
            if rest.startswith("st("):
                stack.append(stack[-1 - int(rest[3])] if len(stack) > int(rest[3]) else "st?")
            else:
                stack.append(("(double)" if op != "fld" else "") + rest)
        elif op == "fld1":
            stack.append("1.0")
        elif op == "fldz":
            stack.append("0.0")
        elif op == "fldpi":
            stack.append("pi")
        elif op in ("fstp", "fst", "fistp", "fist") and arg:
            kind, rest = operand(arg)
            if stack:
                if rest.startswith("st("):
                    if op == "fstp" and len(stack) > int(rest[3]):
                        stack[-1 - int(rest[3])] = stack[-1]
                else:
                    out = "[%s]%s = %s" % (rest, "" if op[1] == "s" else "(int)", stack[-1])
                if op.endswith("p") and stack:
                    stack.pop()
        elif op in ("fadd", "fsub", "fsubr", "fmul", "fdiv", "fdivr", "faddp", "fsubp", "fsubrp", "fmulp", "fdivp", "fdivrp",
                    "fiadd", "fisub", "fisubr", "fimul", "fidiv", "fidivr") and stack:
            symbol = {"add": "+", "sub": "-", "mul": "*", "div": "/"}
            base = re.sub(r"^fi?", "", op)
            pop = base.endswith("p") and not base.startswith("p")
            base = base[:-1] if pop else base
            reverse = base.endswith("r")
            base = base[:-1] if reverse else base
            integer = op.startswith("fi")
            if arg.startswith("st"):
                registers = re.findall(r"st\((\d)\)|st", arg)
                # forms: "st(1), st" (dest st1), "st, st(1)" (dest st0), "st(1)" (dest st1 in the p forms)
                if not pop and arg.startswith("st, "):
                    index = int(re.search(r"st\((\d)\)", arg).group(1)) if "(" in arg else 0
                    left, right = stack[-1], stack[-1 - index]
                    if reverse:
                        left, right = right, left
                    stack[-1] = "(%s %s %s)" % (left, symbol[base], right)
                else:
                    index = int(re.search(r"st\((\d)\)", arg).group(1)) if "(" in arg else 1
                    if len(stack) > index:
                        left, right = stack[-1 - index], stack[-1]
                        if reverse:
                            left, right = right, left
                        stack[-1 - index] = "(%s %s %s)" % (left, symbol[base], right)
                    if pop:
                        stack.pop()
            elif arg:
                kind, rest = operand(arg)
                value = ("(double)" if integer else "") + rest
                left, right = stack[-1], value
                if reverse:
                    left, right = right, left
                stack[-1] = "(%s %s %s)" % (left, symbol[base], right)
            else:
                if len(stack) > 1:
                    left, right = stack[-2], stack[-1]
                    if reverse:
                        left, right = right, left
                    stack[-2] = "(%s %s %s)" % (left, symbol[base], right)
                    stack.pop()
        elif op == "fchs" and stack:
            stack[-1] = "-%s" % stack[-1]
        elif op == "fabs" and stack:
            stack[-1] = "fabs(%s)" % stack[-1]
        elif op in ("fsin", "fcos", "fsqrt", "fptan", "frndint") and stack:
            stack[-1] = "%s(%s)" % (op[1:], stack[-1])
        elif op == "fxch":
            index = int(re.search(r"st\((\d)\)", arg).group(1)) if "(" in arg else 1
            if len(stack) > index:
                stack[-1], stack[-1 - index] = stack[-1 - index], stack[-1]
        elif op in ("fcom", "fcomp", "fcompp", "fucom", "fucomp", "fcomi", "fcomip", "fucomi", "fucomip") and stack:
            if arg and not arg.startswith("st"):
                kind, rest = operand(arg)
                out = "cmp %s, %s" % (stack[-1], rest)
            elif arg:
                index = int(re.search(r"st\((\d)\)", arg).group(1)) if "(" in arg else 0
                out = "cmp %s, %s" % (stack[-1], stack[-1 - index] if len(stack) > index else "st?")
            else:
                out = "cmp %s, %s" % (stack[-1], stack[-2] if len(stack) > 1 else "st?")
            if op.endswith("pp"):
                stack.pop()
                if stack:
                    stack.pop()
            elif op.endswith("p"):
                stack.pop()
        elif op == "ftst" and stack:
            out = "cmp %s, 0.0" % stack[-1]
        elif op in ("fnstsw", "sahf") or op.startswith(("fnstcw", "fldcw")):
            pass
        elif op == "fstp" and not arg and stack:
            stack.pop()
        elif op == "call":
            target = arg
            following = lines[index + 1][1].split(None, 1)[0] if index + 1 < len(lines) else ""
            if following.startswith("f") and not following.startswith(("fld", "fild", "fstp", "fnstsw")) and not stack and "0x527760" not in target:
                # a function that returns its value in st0 and is used straight away
                stack.append("ret(%s)" % target)
                continue
            if "0x527760" in target and stack:
                out = "_ftol(%s)" % stack.pop()
            elif stack and re.search(r"0x(5[0-9a-f]{5})", target) and not target.startswith("dword"):
                out = "call %s with st0 = %s" % (target, stack[-1])
            else:
                out = None
        if out:
            print("%x:%s %s" % (address, confused, out))
        if op == "ret":
            stack = []


if __name__ == "__main__":
    main()
