#!/usr/bin/env python3
"""Infer stack parameters of Bonnie's Bookstore's functions from their disassembly, for Ghidra signatures.

The engine passes floats on the stack and converts them with __ftol, and Ghidra's analysis leaves most of those
functions as `void f(void)` with unnamed stack reads, so their callers lose the arguments. This scans each function of
an llvm-objdump (Intel syntax) disassembly linearly, tracks the stack depth through push/pop/sub/add esp and an ebp
frame, and records every read of the caller's stack: [esp+K] above the return address or [ebp+K] above the saved ebp.
A slot read by an x87 instruction is a float. ECX read before it is written (or a call) makes the function __thiscall. Functions
whose Ghidra signature already has at least as many parameters are skipped.
    python games/bonnies/tools/stackparams.py re/bonnies/game_intel.txt re/bonnies/gh/functions.jsonl \
        > games/bonnies/docs/symbols/01_stackparams.jsonl
"""
import json
import re
import sys

insn = re.compile(r"^\s*([0-9a-f]+):\s+(\S+)\s*(.*)$")
mem = re.compile(r"(byte|word|dword|qword|tbyte) ptr \[(esp|ebp)\s*([+-])\s*(0x[0-9a-f]+|\d+)\]")
fpu = re.compile(r"^f")

funcs = {int(f["addr"], 16): f for f in (json.loads(l) for l in open(sys.argv[2]))}
starts = sorted(a for a in funcs if not funcs[a]["name"].startswith("Unwind") and not funcs[a]["thunk"])

lines = []
for line in open(sys.argv[1]):
    m = insn.match(line)
    if m:
        lines.append((int(m.group(1), 16), m.group(2), m.group(3)))


def nparams(sig):
    inner = sig[sig.index("(") + 1:sig.rindex(")")].strip()
    if inner in ("", "void"):
        return 0
    n = inner.count(",") + 1
    if "__thiscall" in sig:
        n -= 1  # the this pointer
    return n


i = 0
for k, start in enumerate(starts):
    end = starts[k + 1] if k + 1 < len(starts) else 1 << 32
    while i < len(lines) and lines[i][0] < start:
        i += 1
    depth = 0  # bytes pushed since entry
    ebp_frame = None  # depth at which ebp == esp
    slots = {}  # parameter offset (from the first parameter) -> float?
    ecx_written = False
    epilogue = None
    thiscall = False
    j = i
    while j < len(lines) and lines[j][0] < end:
        _, op, args = lines[j]
        j += 1
        if not ecx_written and op != "call" and re.search(r"(ecx|cx|cl|ch)", args):
            ops = [o.strip() for o in args.split(",", 1)]
            src = ops[1] if len(ops) > 1 else ""
            writes = op in ("mov", "lea", "pop", "movzx", "movsx") and re.fullmatch(r"ecx|cx|cl", ops[0])
            if args.replace(" ", "") == "ecx,ecx" and op in ("xor", "sub"):
                writes, src = True, ""
            if re.search(r"(ecx|cx|cl|ch)", src) or not writes:
                thiscall = True
            ecx_written = True
        if op == "call":
            ecx_written = True  # a call clobbers it
        for m in mem.finditer(args):
            off = int(m.group(4), 0) * (1 if m.group(3) == "+" else -1)
            if m.group(2) == "esp":
                p = off - depth - 4
            elif ebp_frame is not None:
                p = off - 4 - ebp_frame
            else:
                continue
            if 0 <= p < 0x60:
                isf = bool(fpu.match(op)) and m.group(1) == "dword"
                slots[p & ~3] = slots.get(p & ~3, False) or isf
        # an epilogue (pops, add esp, mov esp,ebp) followed by ret: the code after the ret has the depth from before it
        if op == "pop" or (op == "add" and args.startswith("esp,")) or args.replace(" ", "") == "esp,ebp":
            if epilogue is None:
                epilogue = depth
        elif op == "ret":
            if epilogue is not None:
                depth = epilogue
                epilogue = None
                continue
        else:
            epilogue = None
        if op == "push":
            depth += 4
        elif op == "pop":
            depth -= 4
        elif op in ("sub", "add") and args.startswith("esp,"):
            v = args.split(",")[1].strip()
            if re.fullmatch(r"0x[0-9a-f]+|\d+", v):
                depth += int(v, 0) * (1 if op == "sub" else -1)
        elif op == "mov" and args.replace(" ", "") == "ebp,esp":
            ebp_frame = depth
        elif op == "mov" and args.replace(" ", "") == "esp,ebp":
            depth = ebp_frame
    if not slots:
        continue
    n = max(slots) // 4 + 1
    f = funcs[start]
    if nparams(f["sig"]) >= n:
        continue
    ret = f["sig"].split("(")[0].rsplit(" ", 1)[0]
    ret = re.sub(r"\b(__thiscall|__cdecl|__stdcall|__fastcall)\b", "", ret).strip() or "void"
    if ret in ("float10",):
        ret = "float"
    params = ["%s param_%d" % ("float" if slots.get(4 * q) else "undefined4", q + 1) for q in range(n)]
    cc = "__thiscall" if thiscall else "__cdecl"
    print(json.dumps({"addr": f["addr"], "name": f["name"],
                      "sig": "%s %s %s(%s)" % (ret, cc, f["name"], ", ".join(params))}))
