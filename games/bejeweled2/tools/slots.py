#!/usr/bin/env python3
"""Names the virtual functions of the framework classes by vtable slot offset, as the exe lays them out.

    python games/bejeweled2/tools/slots.py Widget 9c a0 a4      # the slots 0x9c, 0xa0, 0xa4 of Widget's vtable
    python games/bejeweled2/tools/slots.py Dialog -l             # all of Dialog's slots
Reads the declarations in framework/*.h in order, base classes first (the primary base only, as the first vtable is the
primary base's), and puts a group of overloads of one name, declared one after another, in reverse (MSVC's layout).
The destructor is slot 0.
"""
import os
import re
import sys

FW = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', 'framework')
_classes = {}


def load():
    if _classes:
        return
    for fn in os.listdir(FW):
        if not fn.endswith('.h'):
            continue
        text = open(os.path.join(FW, fn), encoding='utf-8', errors='replace').read()
        text = re.sub(r'//[^\n]*', '', text)
        text = re.sub(r'/\*.*?\*/', '', text, flags=re.S)
        for m in re.finditer(r'\b(?:class|struct)\s+(\w+)\s*(?::\s*([^{;]+))?\{', text):
            name = m.group(1)
            bases = [b.split()[-1] for b in (m.group(2) or '').split(',') if b.strip()]
            # the class body: to the matching brace
            depth, i = 1, m.end()
            while depth and i < len(text):
                depth += (text[i] == '{') - (text[i] == '}')
                i += 1
            body = text[m.end():i - 1]
            # only the top level of the body (nested braces dropped)
            flat, d = [], 0
            for ch in body:
                if ch == '{':
                    d += 1
                elif ch == '}':
                    d -= 1
                elif d == 0:
                    flat.append(ch)
            body = ''.join(flat)
            virt = []
            for s in body.split(';'):
                s = ' '.join(s.split())
                mm = re.search(r'\bvirtual\s+(?:[\w:<>*&\s]+?[\s*&])?(~?\w+)\s*\(', s)
                if mm:
                    virt.append(mm.group(1))
            if name not in _classes or len(virt) > len(_classes[name][1]):
                _classes[name] = (bases, virt)


def table(name):
    load()
    if name not in _classes:
        sys.exit('no class ' + name)
    bases, virt = _classes[name]
    slots = table(bases[0]) if bases and bases[0] in _classes else []
    own = []
    for v in virt:
        if v.startswith('~'):
            if slots:
                slots[0] = name + '::' + v
            else:
                own.append(name + '::' + v)
            continue
        label = name + '::' + v
        # an override of a base's virtual keeps the base's slot
        idx = [i for i, s in enumerate(slots) if s.split('::')[1] == v]
        if idx:
            slots[idx[0]] = label
            continue
        own.append(label)
    # reverse each run of one name in own
    out, i = [], 0
    while i < len(own):
        j = i
        while j < len(own) and own[j].split('::')[1] == own[i].split('::')[1]:
            j += 1
        out += own[i:j][::-1]
        i = j
    return slots + out


def main():
    a = sys.argv[1:]
    if not a:
        sys.exit(__doc__)
    t = table(a[0])
    if len(a) == 1 or a[1] == '-l':
        for i, s in enumerate(t):
            print('%03x %s' % (i * 4, s))
    else:
        for x in a[1:]:
            k = int(x, 16) // 4
            print('%s %s' % (x, t[k] if k < len(t) else '(beyond %d slots)' % len(t)))


main()
