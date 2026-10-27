"""List the vtables of the game's classes, which have no RTTI: runs of function addresses in .rdata, with the
functions that store each one (constructors and destructors) and, slot by slot, the framework vtable it extends.

    python games/dynomite/tools/vtscan.py                 # every vtable outside the RTTI ones
    python games/dynomite/tools/vtscan.py 4f8a10 ...      # the slots of these vtables, with names
"""
import json, struct, sys, bisect, glob

EXE = open('re/dynomite/bin/DynomiteGame.exe', 'rb').read()
BASE = 0x400000
pe = struct.unpack_from('<I', EXE, 0x3c)[0]
nsec = struct.unpack_from('<H', EXE, pe + 6)[0]
sec0 = pe + 24 + struct.unpack_from('<H', EXE, pe + 20)[0]
SECS = {}
for i in range(nsec):
    nm, vs, va, rs, rp = struct.unpack_from('<8sIIII', EXE, sec0 + i * 40)
    SECS[nm.rstrip(b'\0').decode()] = (BASE + va, vs, rp, rs)


def read32(va):
    for s, (v, vs, rp, rs) in SECS.items():
        if v <= va < v + rs:
            return struct.unpack_from('<I', EXE, rp + va - v)[0]
    return None


funcs = {}
for l in open('re/dynomite/gh/functions.jsonl'):
    j = json.loads(l)
    funcs[int(j['addr'], 16)] = j
starts = sorted(funcs)
names = {}
for p in sorted(glob.glob('games/dynomite/docs/symbols/*.jsonl')):
    for l in open(p):
        if l.strip():
            j = json.loads(l)
            names[int(j['addr'], 16)] = j['name']
known = {}
for l in open('re/dynomite/gh/vtables.jsonl'):
    j = json.loads(l)
    known[int(j['addr'], 16)] = j['symbol'][8:]
    for s in j['slots']:
        names.setdefault(int(s['addr'], 16), s['name'])


def fname(a):
    return names.get(a) or funcs.get(a, {}).get('name') or '%08x' % a


def containing(a):
    i = bisect.bisect_right(starts, a) - 1
    if i >= 0 and starts[i] <= a < starts[i] + max(1, funcs[starts[i]]['size']):
        return starts[i]
    return None


text_va, text_vs, text_rp, _ = SECS['.text']
TEXT = EXE[text_rp:text_rp + text_vs]


def storers(vt):
    # mov dword ptr [reg(+disp)], imm32 (C7 /0) whose imm32 is the vtable
    out = set()
    pat = struct.pack('<I', vt)
    i = TEXT.find(pat)
    while i >= 0:
        for back in (2, 3, 6):
            if i - back >= 0 and TEXT[i - back] == 0xC7 and (TEXT[i - back + 1] >> 3) & 7 == 0:
                f = containing(text_va + i - back)
                if f:
                    out.add(f)
        i = TEXT.find(pat, i + 1)
    return sorted(out)


def is_code(v):
    return v is not None and text_va <= v < text_va + text_vs


def scan():
    rv, rvs, rrp, rrs = SECS['.rdata']
    tables = []
    a = rv
    while a < rv + rrs:
        v = read32(a)
        if is_code(v):
            start = a
            n = 0
            while is_code(read32(a)):
                n += 1
                a += 4
                if a in known and a != start:
                    break
            tables.append((start, n))
        else:
            a += 4
    return tables


if len(sys.argv) > 1:
    for arg in sys.argv[1:]:
        vt = int(arg, 16)
        print('== vtable %08x %s; stored by %s' % (vt, known.get(vt, ''), ' '.join(fname(f) for f in storers(vt))))
        i = 0
        while True:
            f = read32(vt + 4 * i)
            if not is_code(f) or (i and vt + 4 * i in known):
                break
            print('  %2d +%02x %08x %s' % (i, 4 * i, f, fname(f)))
            i += 1
else:
    for start, n in scan():
        if start in known:
            continue
        st = storers(start)
        if not st and n < 2:
            continue
        print('%08x %3d slots  stored by %s' % (start, n, ' '.join('%08x' % f for f in st)))
