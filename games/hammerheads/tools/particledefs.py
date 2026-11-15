#!/usr/bin/env python3
"""Reads the particle definitions out of Hammer Heads's exe.

The exe builds its particle emitter and fragment definitions in code (0x475500-0x4767e0: a manager made of two lists, with
a function call and a few dozen loads of constants for each definition). This walks that function's instructions with a
very small interpreter (the stack, the registers, the floating-point stack, the objects the function allocates) and
prints each definition's fields, so that games/hammerheads/src/ParticleDefs.inc is written from the exe's own numbers.

    python games/hammerheads/tools/particledefs.py            # each definition's fields by offset
    python games/hammerheads/tools/particledefs.py --cpp      # as the rows of ParticleDefs.inc
"""
import os
import re
import struct
import sys

import asm
import pe

IMAGE = pe.Image()
RESGLOBALS = {}
for _line in open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "docs", "resglobals.txt")):
    _parts = _line.split()
    if len(_parts) == 2:
        RESGLOBALS[int(_parts[0], 16)] = _parts[1]


def cstring(address):
    """The text at a virtual address, when it is a short NUL-terminated string in the data sections."""
    if IMAGE.section(address) not in (".rdata", ".data"):
        return None
    data = IMAGE.bytes(address, 40)
    end = data.find(bytes([0]))
    if end < 3 or any(b < 32 or b > 126 for b in data[:end]):
        return None
    return data[:end].decode("latin-1")


# Statics that the exe sets while it starts (0x54e3f0: the default envelope) or never writes (the zero vector at 0x5980b8)
STATICS = {0x5980d0: 0.0, 0x5980d8: 0.2, 0x5980e0: 0.0, 0x5980e8: 0.0, 0x5980f0: 0.0}


def f64(address):
    if address in STATICS:
        return STATICS[address]
    if IMAGE.offset(address) is None:
        return 0.0
    return struct.unpack("<d", IMAGE.bytes(address, 8))[0]


class Obj:
    count = 0

    def __init__(self, size):
        self.id = Obj.count
        Obj.count += 1
        self.size = size
        self.f = {}
        self.kind = None
        self.name = None


class Machine:
    def __init__(self):
        self.regs = {}
        self.mem = {}
        self.esp = 0x100000
        self.fpu = []
        self.fragments = []
        self.emitters = []
        self.flag_zero = False

    def split_operand(self, text):
        return re.sub(r"<[^>]*>", "", text).strip()

    def address_of(self, expr):
        expr = expr.strip()
        m = re.match(r"^([a-z]{3})\s*([+-])\s*(0x[0-9a-f]+|\d+)$", expr)
        if m:
            base = self.regs[m.group(1)]
            disp = int(m.group(3), 16 if m.group(3).startswith("0x") else 10)
            if m.group(2) == "-":
                disp = -disp
            return base, disp
        if re.match(r"^[a-z]{3}$", expr):
            return self.regs[expr], 0
        return None, int(expr, 16)

    def read_mem(self, size, expr):
        base, disp = self.address_of(expr)
        if base is None:
            if size == 4 and disp in RESGLOBALS:
                return ("sym", RESGLOBALS[disp])
            return int.from_bytes(IMAGE.bytes(disp, size), "little")
        return self.read_at(base, disp)

    def read_at(self, base, disp):
        if isinstance(base, tuple) and base[0] == "stack":
            return self.mem.get(base[1] + disp)
        if isinstance(base, tuple) and base[0] == "obj":
            return base[1].f.get(disp)
        raise ValueError("read at %r" % (base,))

    def write_at(self, base, disp, value):
        if isinstance(base, tuple) and base[0] == "stack":
            self.mem[base[1] + disp] = value
        elif isinstance(base, tuple) and base[0] == "obj":
            base[1].f[disp] = value
        else:
            raise ValueError("write at %r" % (base,))

    def value_of(self, raw):
        text = self.split_operand(raw)
        m = re.match(r"^(byte|word|dword|qword) ptr \[(.*)\]$", text)
        if m:
            size = {"byte": 1, "word": 2, "dword": 4, "qword": 8}[m.group(1)]
            return self.read_mem(size, m.group(2))
        if re.match(r"^[a-z]{2,3}$", text):
            if text in ("cl", "dl", "al", "bl"):
                return self.regs.get("e" + text[0] + "x")
            return self.regs.get(text)
        value = int(text, 16) if text.startswith(("0x", "-0x")) else int(text)
        if value > 0x400000:
            string = cstring(value)
            if string is not None:
                return ("str", string)
        return value

    def store(self, text, value):
        text = self.split_operand(text)
        m = re.match(r"^(byte|word|dword|qword) ptr \[(.*)\]$", text)
        if m:
            base, disp = self.address_of(m.group(2))
            self.write_at(base, disp, value)
            return
        if text in ("cl", "dl", "al", "bl"):
            self.regs["e" + text[0] + "x"] = value
        else:
            self.regs[text] = value

    def push(self, value):
        self.esp -= 4
        self.mem[self.esp] = value

    def call(self, address):
        if address == 0x526360:
            self.regs["eax"] = ("obj", Obj(self.mem.get(self.esp)))
        elif address == 0x473d20:
            self.regs["eax"] = ("obj", Obj(8))
        elif address == 0x475280:
            # the emitter definition's constructor: the defaults, then the name
            obj = self.regs["ecx"][1]
            name = self.mem[self.esp][1]
            self.esp += 4
            obj.kind = "emitter"
            obj.name = name
            for i in range(5):
                obj.f[0x50 + 8 * i] = f64(0x5980d0 + 8 * i)
            obj.f[0x98] = f64(0x5980b8)
            obj.f[0xa0] = f64(0x5980c0)
            obj.f[0xa8] = f64(0x5980c8)
            obj.f[0xb0] = 0.05
            obj.f[0xb8] = 0.05
            obj.f[0xc0] = 0.0
            obj.f[0xd8] = f64(0x5980b8)
            obj.f[0xe0] = f64(0x5980c0)
            obj.f[0xe8] = f64(0x5980c8)
            obj.f[0x48] = 10.0
            obj.f[0x40] = 10
            obj.f[0x78] = 2
            obj.f[0x80] = 40.0
            obj.f[0x88] = 30.0
            obj.f[0x90] = 0.0
            obj.f[0xc8] = 5.0
            obj.f[0xd0] = 2.0
            obj.f[0xf0] = 0.0
            obj.f[0xf8] = 1.0
            obj.f[0x100] = 0.0
            self.regs["eax"] = ("obj", obj)
        elif address == 0x473860:
            # the envelope's constructor: four numbers and a zero, in a buffer on the stack
            buf = self.regs["ecx"]
            args = [self.mem[self.esp + 8 * i] for i in range(4)]
            self.esp += 0x20
            for i, value in enumerate(args + [0.0]):
                self.mem[buf[1] + 8 * i] = value
            self.regs["eax"] = buf
        elif address == 0x4745d0:
            # the fragment definition's constructor: its arguments in the order of the original's prototype
            obj = self.regs["ecx"][1]
            at = self.esp

            def take(size):
                nonlocal at
                value = self.mem.get(at)
                at += size
                return value

            name = take(4)[1]
            image = take(4)
            mode = take(4)
            p5 = take(4)
            p6 = take(8)
            p7 = take(4)
            p8 = take(8)
            p9 = take(4)
            p10 = take(8)
            p11 = take(4)
            rest = [take(8) for _ in range(12)]
            p24 = take(8)
            self.esp = at
            obj.kind = "fragment"
            obj.name = name
            obj.f = {"image": image, "mode": mode, "p5": p5, "rate": p6, "p7": p7, "spin": p8, "p9": p9, "fade": p10,
                     "draw": p11, "rest": rest, "life": p24}
            self.regs["eax"] = ("obj", obj)
        elif address == 0x473ff0:
            obj = self.mem[self.esp]
            self.esp += 4
            self.fragments.append(obj[1])
        elif address == 0x474040:
            obj = self.mem[self.esp]
            self.esp += 4
            self.emitters.append(obj[1])
        else:
            raise ValueError("call %x" % address)

    def run(self, lines):
        by_address = {a: i for i, (a, t) in enumerate(lines)}
        i = 0
        while i < len(lines):
            address, text = lines[i]
            i += 1
            self.regs["esp"] = ("stack", self.esp)
            parts = text.split(None, 1)
            op = parts[0]
            rest = parts[1] if len(parts) > 1 else ""
            args = [a.strip() for a in re.split(r",(?![^\[]*\])", rest)] if rest else []
            if op in ("int3", "ret"):
                break
            if op == "push":
                self.push(self.value_of(args[0]))
            elif op == "pop":
                self.regs[self.split_operand(args[0])] = self.mem.get(self.esp)
                self.esp += 4
            elif op == "sub" and args[0] == "esp":
                self.esp -= int(args[1], 16)
            elif op == "add" and args[0] == "esp":
                self.esp += int(args[1], 16)
            elif op == "fld":
                m = re.match(r"^qword ptr \[(0x[0-9a-f]+)\]$", self.split_operand(args[0]))
                if not m:
                    raise ValueError("fld %s" % text)
                self.fpu.append(f64(int(m.group(1), 16)))
            elif op == "fstp":
                self.store(args[0], self.fpu.pop())
            elif op == "mov":
                self.store(args[0], self.value_of(args[1]))
            elif op == "lea":
                m = re.match(r"^\[(.*)\]$", self.split_operand(args[1]))
                base, disp = self.address_of(m.group(1))
                if base[0] == "obj":
                    self.regs[args[0]] = base		# only used as the destination of the envelope copy (+0x50)
                else:
                    self.regs[args[0]] = ("stack", base[1] + disp)
            elif op == "xor":
                if args[0] != args[1]:
                    raise ValueError("xor %s" % text)
                self.store(args[0], 0)
            elif op == "or":
                self.store(args[0], -1)
            elif op == "and" or op == "cmp":
                pass
            elif op == "test":
                self.flag_zero = self.value_of(args[0]) in (0, None)
            elif op == "je":
                if self.flag_zero:
                    i = by_address[int(args[0], 16)]
            elif op == "jmp":
                i = by_address[int(args[0], 16)]
            elif op == "rep":
                # the envelope copied into the definition at +0x50 (ten dwords)
                src = self.regs["esi"][1]
                dst = self.regs["edi"]
                for k in range(5):
                    dst[1].f[0x50 + 8 * k] = self.mem.get(src + 8 * k, 0.0)
            elif op == "call":
                self.call(int(args[0], 16))
            else:
                raise ValueError("unhandled %s" % text)


def number(value):
    """A number as C++ source: a double that reads back as itself."""
    return repr(float(value))


def emit_cpp(machine):
    """The definitions as the two tables that games/hammerheads/src/Particles.cpp includes."""
    out = []
    out.append("// ParticleDefs.inc: the particle definitions that the exe builds in code (0x475500-0x4767e0), as rows.")
    out.append("// Written by: python games/hammerheads/tools/particledefs.py --cpp > games/hammerheads/src/ParticleDefs.inc")
    out.append("// Included by Particles.cpp after the row structs EmitterRow and FragmentRow; do not edit by hand.")
    out.append("")
    out.append("static const EmitterRow EMITTER_ROWS[] =")
    out.append("{")
    for obj in machine.emitters:
        f = obj.f
        envelope = ", ".join(number(f[0x50 + 8 * k]) for k in range(5))
        scatter = ", ".join(number(f[k]) for k in (0x80, 0x88, 0x90))
        direction = ", ".join(number(f[k]) for k in (0x98, 0xa0, 0xa8))
        jitter = ", ".join(number(f[k]) for k in (0xb0, 0xb8, 0xc0))
        gravity = ", ".join(number(f[k]) for k in (0xd8, 0xe0, 0xe8))
        out.append("	{ \"%s\", %d, %s, { %s }, %d, { %s }, { %s }, { %s }, %s, %s, { %s }, %s, %s, %s }," % (
            obj.name, f[0x40], number(f[0x48]), envelope, f[0x78], scatter, direction, jitter, number(f[0xc8]),
            number(f[0xd0]), gravity, number(f[0xf0]), number(f[0xf8]), number(f[0x100])))
    out.append("};")
    out.append("")
    out.append("static const FragmentRow FRAGMENT_ROWS[] =")
    out.append("{")
    for obj in machine.fragments:
        f = obj.f
        rest = f["rest"]
        for k in (0, 2, 5, 8, 9, 11):
            if rest[k] != 0.0:
                raise ValueError("%s: an argument the code never reads is not zero" % obj.name)
        image = f["image"]
        if not (isinstance(image, tuple) and image[0] == "sym"):
            raise ValueError("%s: the image is not a resource global" % obj.name)
        out.append("	{ \"%s\", &%s, %d, %s, %s, %s, %s, %s, %s, %d, %s, { %s, %s, %s }, %s, %s, %s, %s, %s }," % (
            obj.name, image[1], f["mode"], "true" if f["p5"] else "false", number(f["rate"]),
            "true" if f["p7"] else "false", number(f["spin"]), "true" if f["p9"] else "false", number(f["fade"]),
            f["draw"], number(f["life"]), number(rest[0]), number(rest[1]), number(rest[2]), number(rest[3]),
            number(rest[4]), number(rest[6]), number(rest[7]), number(rest[10])))
    out.append("};")
    sys.stdout.write("\n".join(out) + "\n")


def main():
    lines = asm.disassemble(0x475500, 0x4767e0)
    first = [i for i, (a, t) in enumerate(lines) if t == "call 0x526360"][0]
    lines = lines[first - 1:]
    machine = Machine()
    machine.regs["ebp"] = ("obj", Obj(8))
    machine.regs["esi"] = -1
    machine.regs["edi"] = 3
    machine.run(lines)

    if "--cpp" in sys.argv:
        emit_cpp(machine)
        return

    print("// emitters: %d, fragments: %d" % (len(machine.emitters), len(machine.fragments)))
    for kind, objects in (("emitter", machine.emitters), ("fragment", machine.fragments)):
        for obj in objects:
            print("%s %s" % (kind, obj.name))
            for key in sorted(obj.f, key=lambda k: (isinstance(k, str), k)):
                print("   %s = %r" % (key if isinstance(key, str) else hex(key), obj.f[key]))


if __name__ == "__main__":
    main()
