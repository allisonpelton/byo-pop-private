#!/usr/bin/env python3
"""Print the virtual tables of the entity classes with the compiler's thunks resolved.

    python games/typershark/tools/entity_slots.py            one line per class: its sub-objects and their slots
    python games/typershark/tools/entity_slots.py --slots    one line per GameObject slot: the targets and how many classes use each

Every concrete entity class (those whose RTTI bases include GameObject) has up to four vtables (re/typershark/rtti.classes.json
gives their offsets inside the complete object): its own part (1 slot, the deleting destructor), the GameObject part
(12 slots), the Enemy part (3 slots) and, for deeper classes, the parts of its base classes. The slots of the GameObject
and Enemy parts point at small thunks, `sub ecx, [ecx - 4]` (vtordisp: back to the start of the complete object), often
`add ecx, imm`, then `jmp real`. This prints `real` followed by `(vt)` or `(vt+imm)`; a slot printed without a
suffix is not a thunk (a base class's default, e.g. 4010a0 = GameObject::vf05). See docs/ENTITIES_BRIEF.md.
"""
import json
import os
import struct
import sys
from collections import Counter

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
EXE = os.path.join(ROOT, "re", "typershark", "bin", "TyperSharkGame.exe")
RTTI = os.path.join(ROOT, "re", "typershark", "rtti.classes.json")

d = open(EXE, "rb").read()
pe = struct.unpack_from("<I", d, 0x3C)[0]
opt = struct.unpack_from("<H", d, pe + 20)[0]
secs = []
for i in range(struct.unpack_from("<H", d, pe + 6)[0]):
    s = pe + 24 + opt + i * 40
    vs, rva, rs, rp = struct.unpack_from("<IIII", d, s + 8)
    secs.append((0x400000 + rva, max(vs, rs), rp))


def off(va):
    for v, n, rp in secs:
        if v <= va < v + n:
            return rp + va - v
    raise ValueError(hex(va))


def resolve(a):
    o = off(a)
    b = d[o:o + 16]
    if b[:3] == b"\x2b\x49\xfc":
        i = 3
        adj = "vt"
        if b[i] == 0x81 and b[i + 1] == 0xC1:
            adj += "+%x" % struct.unpack_from("<I", b, i + 2)[0]
            i += 6
        elif b[i] == 0x83 and b[i + 1] == 0xC1:
            adj += "+%x" % b[i + 2]
            i += 3
        if b[i] == 0xE9:
            t = a + i + 5 + struct.unpack_from("<i", b, i + 1)[0]
            return "%x(%s)" % (t, adj)
    return "%x" % a


def main():
    rt = json.load(open(RTTI))
    classes = {c: v for c, v in rt.items() if "Sexy::GameObject" in v["bases"] or c == "Sexy::GameObject"}
    if "--slots" in sys.argv:
        tabs = {}
        for c, v in classes.items():
            for vt in v["vtables"]:
                if len(vt["slots"]) == 12:
                    tabs[c.replace("Sexy::", "")] = [resolve(int(s, 16)) for s in vt["slots"]]
        for slot in range(12):
            cnt = Counter(t[slot] for t in tabs.values())
            print("GameObject slot %2d: %d distinct; %s" % (slot, len(cnt), ", ".join("%s x%d" % kv for kv in cnt.most_common(6))))
        return
    for c in sorted(classes):
        v = classes[c]
        parts = []
        for vt in sorted(v["vtables"], key=lambda x: x["offset"]):
            parts.append("@%d[%s]" % (vt["offset"], " ".join(resolve(int(s, 16)) for s in vt["slots"])))
        print("%s bases=%s\n    %s" % (c.replace("Sexy::", ""), ",".join(b.replace("Sexy::", "") for b in v["bases"]), "  ".join(parts)))


if __name__ == "__main__":
    main()
