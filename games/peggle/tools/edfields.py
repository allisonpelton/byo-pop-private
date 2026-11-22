#!/usr/bin/env python3
"""Lists the properties that a class's EditGetSetValHook names (the level editor's own names for its fields) with the field
each one reads and writes: "physobj_collidable +0x24 EditValSyncBool". The hook compares the property asked for with each name
(call 0x4097c0), then calls a sync function with the address of the field in the object. The names are the original's own words,
which is where the port's field names come from.

    python games/peggle/tools/edfields.py 47f970 [<addr> ...]
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
STRING = re.compile(r'^[0-9a-f]{8} push 0x[0-9a-f]+\s+; "([^"]+)"')
CALL = re.compile(r'^[0-9a-f]{8} call 0x([0-9a-f]+)\s*(?:; (.*))?$')
LEA = re.compile(r'^[0-9a-f]{8} lea (\w+), \[(\w+) \+ 0x([0-9a-f]+)\]')
LEA_NOOFF = re.compile(r'^[0-9a-f]{8} lea (\w+), \[(\w+)\]')


def main():
    for arg in sys.argv[1:]:
        out = subprocess.run([sys.executable, os.path.join(HERE, "disasm.py"), "0x" + arg], capture_output=True, text=True).stdout
        lines = out.splitlines()
        print("== %s" % arg)
        i = 0
        while i < len(lines):
            m = STRING.match(lines[i])
            if not m or not re.search(r"^[a-z]+_", m.group(1)):
                i += 1
                continue
            name = m.group(1)
            # the compare call, then the first lea of a field and the sync call after it
            j = i + 1
            while j < len(lines) and not (CALL.match(lines[j]) and CALL.match(lines[j]).group(1) == "4097c0"):
                j += 1
                if j - i > 4:
                    break
            if j >= len(lines) or j - i > 4:
                i += 1
                continue
            k = j + 1
            offsets = []
            callee = ""
            while k < len(lines) and k - j < 14:
                lm = LEA.match(lines[k])
                if lm:
                    offsets.append("+0x" + lm.group(3))
                cm = CALL.match(lines[k])
                if cm:
                    callee = (cm.group(2) or cm.group(1)).strip()
                    break
                if STRING.match(lines[k]):
                    break
                k += 1
            print("%-34s %-12s %s" % (name, ",".join(offsets) or "?", callee))
            i = j + 1


if __name__ == "__main__":
    main()
