#!/usr/bin/env python3
"""A builder function's objects, keys and tracks as C++-like lines, straight from the machine code.

    python -I games/bananabugs/tools/keygen.py <function address> [--raw]

Most of Banana Bugs' scene code is builders: they make objects (0x4509f0), keys (0x453290) and tracks (0x453930) and wire
them with register-argument calls the decompile loses. This walks the function in address order like callargs.py, but
also keeps the stack slots (a key saved to [esp+0x18] and read back later keeps its name) and names each new object,
key and track in the order made (o1, k1, t1...). Each call it knows prints as the port's call:

    k3->AddValue(STATE_ALPHA, -0.2f, MOD_TOWARD_NEXT);
    t1->AddKey(k1, nullptr);
    [ebp+0x300]->AddTrack(t1, nullptr);   // -> index

Other calls print as callargs does. Stores into a named object or key print as `name->+0x10 = 300`. It reads
straight-line code: at a join after an if/else the value is the one from the unconditional jump into it, which is what
the `new` pattern (`test eax,eax / je / call ctor / jmp`) needs; check real branches (loops, ifs on game state) in the
decompile. --raw prints every instruction with the state beside it.
"""
import json
import os
import re
import struct
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import decread
import pe

OBJDUMP = "E:/retools/llvm-mingw-20261006-ucrt-x86_64/bin/llvm-objdump.exe"
EXE = os.path.join(pe.ROOT, "re", "bananabugs", "bin", "BananaBugsGame.exe")
REGS = ["eax", "ecx", "edx", "ebx", "esi", "edi", "ebp"]
LOW = {"al": "eax", "cl": "ecx", "dl": "edx", "bl": "ebx", "ah": "eax"}
STATES = ["STATE_X", "STATE_Y", "STATE_Z", "STATE_SCALE_X", "STATE_SCALE_Y", "STATE_OFFSET_X", "STATE_OFFSET_Y",
          "STATE_RED", "STATE_GREEN", "STATE_BLUE", "STATE_ALPHA", "STATE_ROTATION", "STATE_ORBIT_ANGLE",
          "STATE_FRAME", "STATE_ORBIT_RADIUS", "STATE_EXTRA_WIDTH", "STATE_EXTRA_HEIGHT", "STATE_PATH"]
KINDS = ["MOD_HOLD", "MOD_TOWARD_NEXT", "MOD_SINE", "MOD_ORBIT_SIN", "MOD_ORBIT_COS", "MOD_ADD_VALUE",
         "MOD_FACE_MOTION", "MOD_TURN_TOWARD", "MOD_COUNT_UP", "MOD_COUNT_DOWN", "MOD_COUNT_BY_PULSE", "MOD_PING_PONG",
         "MOD_BASE_PLUS_VALUE", "MOD_RANDOM", "MOD_RANDOM_HUNDREDTHS", "MOD_WRAP_DEGREES", "MOD_SPIN",
         "MOD_SPIN_BY_SPEED", "MOD_SPIN_BY_START_SPEED", "MOD_FLICKER", "MOD_COPY_ORBIT_ANGLE", "MOD_DECAY", "MOD_PATH"]
KEY_FIELDS = {0x0: "mEndCondition", 0x4: "mEndChannel", 0x8: "mEndValue", 0xc: "mDuration", 0x10: "mEventId",
              0x18: "mRemoveAtEnd", 0x20: "mDrawMode", 0x24: "mEasing", 0x134: "mAnim", 0x138: "mNext"}
OBJ_FIELDS = {0x28: "mAnim", 0x2c: "mOwnsAnim", 0x30: "mDrawMode", 0x23c: "mEventTarget"}

_rets = {}
# the arguments each known callee takes from the stack
# the callees that leave EAX as it was (checked in their code)
KEEPS_EAX = {0x451830, 0x451870, 0x453590, 0x453570, 0x4535c0, 0x4514a0, 0x453580}
ARG_COUNTS = {0x59f27f: 1, 0x451830: 1, 0x451870: 1, 0x453590: 1, 0x4533c0: 6, 0x4533b0: 1, 0x4539f0: 1,
              0x451880: 1, 0x401c80: 7, 0x473440: 2}


def bounds(address):
    with open(os.path.join(pe.ROOT, "re", "bananabugs", "game", "functions.jsonl")) as f:
        for line in f:
            j = json.loads(line)
            if int(j["addr"], 16) == address:
                return address, address + j["size"]
    return None


def disassemble(low, high):
    out = subprocess.run([OBJDUMP, "-d", "--x86-asm-syntax=intel", "--start-address=0x%x" % low,
                          "--stop-address=0x%x" % high, EXE], capture_output=True, text=True).stdout
    lines = []
    for line in out.splitlines():
        m = re.match(r"\s*([0-9a-f]+):\s+(?:[0-9a-f]{2} )+\s*\t?(.*)$", line)
        if m:
            text = re.sub(r"\s*<\.text\+0x[0-9a-f]+>", "", m.group(2)).strip()
            lines.append((int(m.group(1), 16), re.sub(r"\s+", " ", text)))
    return lines


def callee_pops(target):
    """The bytes a callee removes from the stack on return (ret N), 0 for cdecl."""
    if target in _rets:
        return _rets[target]
    b = bounds(target)
    pops = 0
    if b:
        for _, text in disassemble(*b):
            m = re.match(r"ret (0x[0-9a-f]+|\d+)$", text)
            if m:
                pops = int(m.group(1), 0)
                break
    _rets[target] = pops
    return pops


def as_value(text, image):
    m = re.fullmatch(r"(-?0x[0-9a-f]+|\d+)", text)
    if not m:
        return text
    v = int(text, 0) & 0xffffffff
    if 0x5ca000 <= v < 0x6a8000:
        try:
            s = image.bytes(v, 80).split(b"\0")[0]
            if len(s) >= 3 and all(32 <= c < 127 for c in s):
                return '"%s"' % s.decode()
        except Exception:
            pass
    if v > 0xffff0000:
        return str(v - (1 << 32))
    if 0x30000000 <= v < 0x50000000 or 0xb0000000 <= v < 0xd0000000:
        return short_float(struct.pack("<I", v)) + "f"
    return hex(v) if v > 9 else str(v)


def short_float(raw):
    """The shortest decimal that reads back as the same single-precision float (%g alone loses digits)."""
    value = struct.unpack("<f", raw)[0]
    for digits in range(1, 10):
        text = "%.*g" % (digits, value)
        if struct.pack("<f", float(text)) == raw:
            return text if ("." in text or "e" in text or "n" in text) else text + ".0"
    return repr(value)


def mem_global(operand, image, names):
    m = re.search(r"(dword|qword) ptr \[0x([0-9a-f]+)\]", operand)
    if not m:
        return None
    target = int(m.group(2), 16)
    name = names.get("%08x" % target, "")
    if name:
        return name
    if 0x5ca000 <= target < 0x600000:
        if m.group(1) == "qword":
            return repr(struct.unpack("<d", image.bytes(target, 8))[0])
        return short_float(image.bytes(target, 4)) + "f"
    return "g_%x" % target


def channel(v):
    try:
        i = int(v, 0)
        return STATES[i] if 0 <= i < len(STATES) else v
    except ValueError:
        return v


def kind(v):
    try:
        i = int(v, 0)
        return KINDS[i] if 0 <= i < len(KINDS) else v
    except ValueError:
        return v


def flt(v):
    if re.fullmatch(r"-?\d+", v):
        return v + ".0f"
    if re.fullmatch(r"-?[\d.]+(e[-+]?\d+)?", v):
        return v + "f"
    return v


def null(v):
    return "nullptr" if v == "0" else v


def main():
    args = sys.argv[1:]
    raw = "--raw" in args
    start = int(args[0], 16)
    b = bounds(start)
    if not b:
        raise SystemExit("function %x not found" % start)
    names = decread.load_symbols()
    image = pe.Image()
    regs = {r: r for r in REGS}
    stack = {}
    esp = 0
    fpu = []
    pushes = []
    joins = {}
    counters = {"o": 0, "k": 0, "t": 0}
    out = []

    def fresh(prefix):
        counters[prefix] += 1
        return "%s%d" % (prefix, counters[prefix])

    def read_stack(offset):
        absolute = esp + offset
        if absolute in stack:
            return stack[absolute]
        if absolute >= 4:
            return "arg%d" % ((absolute - 4) // 4)
        return "[esp%+d]" % absolute

    def value_of(operand):
        if operand in regs:
            return regs[operand]
        if operand in LOW:
            return regs[LOW[operand]]
        if "ptr" in operand:
            g = mem_global(operand, image, names)
            if g:
                return g
            m = re.search(r"\[esp(?: \+ (0x[0-9a-f]+|\d+))?\]", operand)
            if m:
                return read_stack(int(m.group(1), 0) if m.group(1) else 0)
            m = re.search(r"\[(e[a-z]{2})(?: ([+-]) (0x[0-9a-f]+|\d+))?\]", operand)
            if m:
                base = regs.get(m.group(1), m.group(1))
                off = int(m.group(3), 0) if m.group(3) else 0
                if m.group(2) == "-":
                    off = -off
                return "%s->+0x%x" % (base, off) if off >= 0 else "%s->-0x%x" % (base, -off)
            m = re.search(r"\[(.*)\]", operand)
            return "[%s]" % m.group(1)
        return as_value(operand, image)

    def emit(address, text):
        out.append("%08x  %s" % (address, text))

    for address, text in disassemble(*b):
        op, _, rest = text.partition(" ")
        if address in joins:
            saved = joins.pop(address)
            regs, esp, stack = dict(saved[0]), saved[1], dict(saved[2])
        operands = [o.strip() for o in re.split(r",(?![^\[]*\])", rest)] if rest else []
        if raw:
            out.append("%08x  %-40s eax=%s ecx=%s edx=%s ebx=%s esi=%s edi=%s esp=%d" % (
                address, text, regs["eax"], regs["ecx"], regs["edx"], regs["ebx"], regs["esi"], regs["edi"], esp))
        if op == "jmp" and rest.startswith("0x"):
            joins[int(rest, 16)] = (dict(regs), esp, dict(stack))
            continue
        if op == "call":
            target = int(operands[0], 16) if operands[0].startswith("0x") else None
            pops = callee_pops(target) if target else 4 * len(pushes)
            count = ARG_COUNTS.get(target, max(len(pushes), pops // 4))
            a = [stack.get(esp + 4 * i, "?") for i in range(count)]  # first argument first
            r = regs
            line = None
            result = "ret"
            if target == 0x59f27f:
                result = "new(%s)" % (a[0] if a else "?")
            elif target == 0x4509f0:
                if r["esi"].startswith("new(") or r["eax"].startswith("new("):
                    result = fresh("o")
                    line = "ObjBase* %s = new ObjBase();" % result
                else:
                    line = "ObjBase::ObjBase()  // base constructor (this = esi %s)" % r["esi"]
            elif target == 0x453290:
                result = fresh("k")
                line = "AnimKey* %s = new AnimKey();" % result
            elif target == 0x453930:
                result = fresh("t")
                line = "AnimTrack* %s = new AnimTrack();" % result
            elif target == 0x401c80:
                result = "Anim::Get(%s)" % ", ".join(a[:7])
                line = "anim = " + result
            elif target == 0x401210:
                result = r["eax"]
                line = "%s->AddRef();" % r["eax"]
            elif target == 0x401220:
                line = "%s->Release();" % r["eax"]
            elif target in (0x401950, 0x401960):
                fpu.append("%s->GetSource%s()" % (r["eax"], "CelWidth" if target == 0x401950 else "CelHeight"))
            elif target == 0x473440:
                fpu.append("FromCorner(%s, %s)" % (flt(a[0]), flt(a[1])))
            elif target == 0x451580:
                line = "%s->SetParent(%s, %s);" % (r["esi"], null(r["eax"]), null(r["ebx"]))
            elif target == 0x4514a0:
                line = "%s->SetEventTarget(%s);" % (r["eax"], r["ecx"])
            elif target == 0x451830:
                line = "%s->SetState(%s, %s);" % (r["ecx"], channel(r["eax"]), flt(a[0] if a else "?"))
            elif target == 0x451870:
                line = "%s->SetStateInt(%s, %s);" % (r["ecx"], channel(r["eax"]), a[0] if a else "?")
            elif target == 0x453590:
                line = "%s->SetDuration(%s);" % (r["eax"], a[0] if a else "?")
            elif target == 0x4533c0:
                value, k, rate, amount, base, table = (a + ["?"] * 6)[:6]
                keep = r["eax"]
                if keep in ("0", "eax") and rate in ("0.0", "0") and amount in ("0.0", "0") and \
                        base in ("0.0", "0") and table == "0":
                    line = "%s->AddValue(%s, %s, %s);" % (r["edi"], channel(r["esi"]), flt(value), kind(k))
                else:
                    line = "%s->AddModifier(%s, %s, %s, %s, %s, %s, %s, %s);" % (
                        r["edi"], channel(r["esi"]), "true" if keep not in ("0",) else "false", flt(value), kind(k),
                        flt(rate), flt(amount), flt(base), null(table))
            elif target == 0x4533b0:
                line = "%s->mEndCondition = %s; %s->mEndChannel = %s; %s->mEndValue = %s;  // sound key" % (
                    r["eax"], r["ecx"], r["eax"], r["edx"], r["eax"], flt(a[0] if a else "?"))
            elif target == 0x453570:
                line = "%s->mEasing = %s;" % (r["eax"], r["ecx"])
            elif target == 0x453580:
                line = "%s->SetRemoveAtEnd();" % r["eax"]
            elif target == 0x4535c0:
                line = "%s->mDrawMode = 1;  // additive" % r["eax"]
            elif target == 0x4539f0:
                line = "%s->AddKey(%s, %s);" % (r["ecx"], r["eax"], null(a[0] if a else "?"))
            elif target == 0x451880:
                result = "trackIndex(%s)" % r["esi"]
                line = "%s->AddTrack(%s, %s);" % (r["edx"], r["esi"], null(a[0] if a else "?"))
            elif target == 0x451940:
                line = "%s->StartTrack(%s);" % (r["edi"], r["eax"])
            elif target == 0x451990:
                line = "%s->ComputeDrawn();" % r["ebx"]
            elif target == 0x453a90:
                line = "%s->SetDelay(%s, %s);" % (r["edx"], r["ecx"], "true" if r["eax"] not in ("0",) else "false")
            elif target is None:
                m = re.search(r"->\+0x([0-9a-f]+)$", value_of(operands[0]))
                slot = int(m.group(1), 16) // 4 if m else "?"
                line = "%s->slot%s(%s)  // %s" % (r["ecx"], slot, ", ".join(a), operands[0])
            else:
                label = names.get("%08x" % target, "FUN_%08x" % target)
                shown = " ".join("%s=%s" % (k, v) for k, v in r.items() if v != k)
                line = "%s(%s)  %s" % (label, ", ".join(a), shown)
            if line:
                emit(address, line)
            esp += pops
            pushes = []
            # ECX and EDX are kept: with link-time code generation the caller knows which callees leave them
            if target not in KEEPS_EAX:
                regs["eax"] = result
            if target not in (0x401950, 0x401960, 0x473440):
                fpu = [] if target not in (None,) else fpu
            continue
        if op == "push":
            v = value_of(operands[0])
            esp -= 4
            stack[esp] = v
            pushes.append(v)
            continue
        if op == "pop":
            regs[LOW.get(operands[0], operands[0])] = stack.get(esp, "?")
            esp += 4
            continue
        if op in ("sub", "add") and operands and operands[0] == "esp":
            n = int(operands[1], 0)
            if op == "sub":
                esp -= n
                pushes.extend(["?"] * (n // 4))
            else:
                esp += n
                k = n // 4
                if k and k <= len(pushes):
                    del pushes[-k:]
                else:
                    pushes = []
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
                fpu.append(("(float)" + v) if op == "fild" else v)
            continue
        if op in ("fstp", "fst"):
            top = fpu[-1] if fpu else "?"
            m = re.search(r"\[esp(?: \+ (0x[0-9a-f]+|\d+))?\]", operands[0]) if operands else None
            if m and "qword" not in operands[0]:
                k = int(m.group(1), 0) if m.group(1) else 0
                stack[esp + k] = top
                if k // 4 < len(pushes):
                    pushes[len(pushes) - 1 - k // 4] = top
            elif operands and "ptr" in operands[0]:
                dest = value_of(operands[0])
                emit(address, "%s = %s;" % (dest, top))
            if op == "fstp" and fpu:
                fpu.pop()
            continue
        if op in ("fmul", "fadd", "fsub", "fdiv", "fsubr", "fdivr", "fmulp", "faddp", "fsubp", "fdivp",
                  "fsubrp", "fdivrp") and fpu:
            sym = {"fmul": "*", "fadd": "+", "fsub": "-", "fdiv": "/", "fsubr": "r-", "fdivr": "r/", "fmulp": "*",
                   "faddp": "+", "fsubp": "-", "fdivp": "/", "fsubrp": "r-", "fdivrp": "r/"}[op]
            if len(operands) == 1 and not operands[0].startswith("st"):
                v = value_of(operands[0])
                if sym.startswith("r"):
                    fpu[-1] = "(%s %s %s)" % (v, sym[1], fpu[-1])
                else:
                    fpu[-1] = "(%s %s %s)" % (fpu[-1], sym, v)
            elif len(fpu) >= 2:
                top = fpu.pop()
                below = fpu.pop()
                if sym.startswith("r"):
                    fpu.append("(%s %s %s)" % (top, sym[1], below))
                else:
                    fpu.append("(%s %s %s)" % (below, sym, top))
            continue
        if op.startswith("f"):
            if op == "fchs" and fpu:
                fpu[-1] = "-" + fpu[-1]
            elif op.endswith("p") and fpu and op not in ("fxch",):
                fpu.pop()
            continue
        if op in ("mov", "movzx", "movsx") and len(operands) == 2:
            dest = operands[0]
            v = value_of(operands[1])
            if dest in regs:
                regs[dest] = v
            elif dest in LOW:
                regs[LOW[dest]] = v
            elif "ptr" in dest:
                m = re.search(r"\[esp(?: \+ (0x[0-9a-f]+|\d+))?\]", dest)
                if m:
                    stack[esp + (int(m.group(1), 0) if m.group(1) else 0)] = v
                else:
                    target = value_of(dest)
                    mm = re.match(r"(.*)->\+0x([0-9a-f]+)$", target)
                    if mm and re.fullmatch(r"k\d+", mm.group(1)):
                        f = KEY_FIELDS.get(int(mm.group(2), 16))
                        target = "%s->%s" % (mm.group(1), f) if f else target
                    elif mm and re.fullmatch(r"o\d+", mm.group(1)):
                        f = OBJ_FIELDS.get(int(mm.group(2), 16))
                        target = "%s->%s" % (mm.group(1), f) if f else target
                    emit(address, "%s = %s;" % (target, v))
            continue
        if op == "lea" and operands[0] in regs:
            mm = re.fullmatch(r"\[(\w+)(?: ([+-]) (0x[0-9a-f]+|\d+))?\]", operands[1])
            if mm and mm.group(1) in regs:
                base = regs[mm.group(1)]
                k = int(mm.group(3), 0) if mm.group(3) else 0
                if mm.group(2) == "-":
                    k = -k
                if re.fullmatch(r"-?(0x[0-9a-f]+|\d+)", base):
                    regs[operands[0]] = str(int(base, 0) + k)
                else:
                    regs[operands[0]] = "(%s + %d)" % (base, k) if k else base
            else:
                regs[operands[0]] = "&" + operands[1]
            continue
        if op == "xor" and len(operands) == 2 and operands[0] == operands[1]:
            r = LOW.get(operands[0], operands[0])
            if r in regs:
                regs[r] = "0"
            continue
        if op in ("add", "sub", "inc", "dec", "imul", "shl", "sar", "shr", "and", "or", "neg", "not", "setne",
                  "sete", "cdq", "idiv", "div") and operands:
            r = LOW.get(operands[0], operands[0])
            if r in regs:
                before = regs[r]
                if op in ("add", "sub") and len(operands) == 2:
                    regs[r] = "(%s %s %s)" % (before, "+" if op == "add" else "-", value_of(operands[1]))
                elif op in ("inc", "dec"):
                    regs[r] = "(%s %s 1)" % (before, "+" if op == "inc" else "-")
                else:
                    regs[r] = "%s(%s)" % (op, before)
            elif "ptr" in operands[0]:
                emit(address, "%s %s %s" % (op, value_of(operands[0]), value_of(operands[1]) if len(operands) > 1 else ""))
            continue
    print("\n".join(out))


main()
