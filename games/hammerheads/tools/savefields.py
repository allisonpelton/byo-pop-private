"""Lists what a SaveExtra function (the ValueList writers at vtable slot 9) puts in the list, in order.

    python savefields.py 435e90 4361d0 ...

Each line is one value: the field it comes from ("+0xc0 double", "+0x58 int", "+0x7a bool"), or "expr" when it is not a plain
field of this. The reader (slot 10, the LoadExtra just after it) takes the same fields in the same order.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import asm

ADD_DOUBLE = 0x41e050
ADD_INT = 0x41e030
ADD_BOOL = 0x41e060


def main():
    for argument in sys.argv[1:]:
        start = int(argument, 16)
        lines = asm.disassemble(start, start + 0x400)
        out = []
        last = None
        for address, text in lines:
            if text.startswith("ret"):
                break
            match = re.match(r"fld (?:qword|dword) ptr \[(\w+) \+ (0x[0-9a-f]+)\]", text)
            if match:
                last = ("+%s double" % match.group(2), match.group(1))
            match = re.match(r"fild dword ptr \[(\w+) \+ (0x[0-9a-f]+)\]", text)
            if match:
                last = ("+%s int" % match.group(2), match.group(1))
            match = re.match(r"mov (e\w\w), dword ptr \[(\w+) \+ (0x[0-9a-f]+)\]", text)
            if match and match.group(2) in ("esi", "edi", "ebx", "ebp", "ecx", "eax"):
                last = ("+%s int" % match.group(3), match.group(2))
            match = re.match(r"(?:mov|movzx) (e\w\w|\w\w), byte ptr \[(\w+) \+ (0x[0-9a-f]+)\]", text)
            if match and match.group(2) in ("esi", "edi", "ebx", "ebp", "ecx", "eax"):
                last = ("+%s bool" % match.group(3), match.group(2))
            if text.startswith("fld qword ptr [0x") or text.startswith("fld1") or text.startswith("fldz"):
                last = ("constant %s" % text, "")
            match = re.match(r"call (0x[0-9a-f]+)", text)
            if match:
                target = int(match.group(1), 16)
                if target in (ADD_DOUBLE, ADD_INT, ADD_BOOL):
                    kind = {ADD_DOUBLE: "double", ADD_INT: "int", ADD_BOOL: "bool"}[target]
                    out.append("%s as %s" % (last[0] if last else "?", kind))
        print("%x: %s" % (start, "; ".join(out)))


if __name__ == "__main__":
    main()
