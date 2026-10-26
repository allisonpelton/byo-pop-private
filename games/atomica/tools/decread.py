"""Print functions of the Ghidra export (re/atomica/gh/decomp) in a compact, readable form: names from
games/atomica/docs/symbols, game strings inlined (for reading only, never written to a file in the repository),
the MSVC 7.0 std::string idioms (inline length loops, temporaries, releases) collapsed.

    python games/atomica/tools/decread.py 4092a0 ...            # functions by address
    python games/atomica/tools/decread.py 406f90-407400         # every function in an address range
    python games/atomica/tools/decread.py -f 6                  # ... with at least 6 lines of output only
"""
import sys, glob, os, re, json
root = 're/atomica/gh/decomp'
strs = {}
for l in open('re/atomica/gh/strings.jsonl'):
    j = json.loads(l)
    if j['type'] == 'string':
        strs[j['addr']] = j['value']
# strings Ghidra did not define: read straight from the exe
EXE = open('re/atomica/bin/Atomica.exe', 'rb').read()


def exe_str(addr):
    off = int(addr, 16) - 0x400000
    if not (0xae000 <= off < 0xcb000 + 0x5000):
        return None
    e = EXE.find(bytes([0]), off)
    b = EXE[off:e]
    if 0 < len(b) < 400 and all(32 <= c < 127 or c in (9, 10, 13) for c in b):
        return b.decode('latin1')
    return None


names = {}
for p in sorted(glob.glob('games/atomica/docs/symbols/*.jsonl')):
    for l in open(p):
        l = l.strip()
        if l:
            j = json.loads(l)
            names[j['addr'].lower().zfill(8)] = j['name'].split('::')[-1]
decl = re.compile(r'^\s*(undefined\d?|int|uint|char|byte|bool|float|double|long|short|ushort|longlong|ulonglong|void|LPVOID|DWORD|code|_String_base|[A-Za-z_]+ \*)\s*\*?\s*(\*)?[A-Za-z_]\w*(\[\d+\])?;\s*$')
BS = chr(92)
NL = BS + 'n'


def esc(s):
    return '"' + s.replace(BS, BS + BS).replace('"', BS + '"').replace('\n', NL).replace('\r', BS + 'r') + '"'


# the inline strlen of a literal followed by string::assign(dest, literal, length)
STRASSIGN = re.compile(r"[ ]*(\w+) = (\"[^\n]*\"|&?DAT_\w+|\(char \*\)&DAT_\w+|\(str\)\"[^\n]*\");\n\s*do \{\n\s*(\w+) = \1;\n\s*\1 = \3 \+ 1;\n\s*\} while \(\*\3 != '" + re.escape(BS) + r"0'\);\n\s*FUN_004036d0\(([^,\n]+),(?:\(_String_base \*\))?([^,\n]+),\(uint\)\(\3 \+ -0x[0-9a-f]+\)\);\n")
STRINIT = re.compile(r"[ ]*(\w+) = 0xf;\n[ ]*(\w+) = 0;\n[ ]*(\w+) = \(void \*\)\(\(uint\)\3 & 0xffffff00\);\n")
STRFREE = re.compile(r"[ ]*if \(0xf < (\w+)\) \{\n[ ]*_free\((\w+)\);\n[ ]*\}\n")
EHSTATE = re.compile(r"^\s*local_8(\._\d_\d_)? = [^\n]*;\n", re.M)


# Field accesses: *(int *)((int)this + 0x18) -> this.i18 ; *(int *)(this.i0c + 0x2f4) -> this.i0c->i2f4
TYPECODE = {'int': 'i', 'uint': 'u', 'float': 'f', 'char': 'b', 'undefined1': 'b', 'byte': 'b', 'bool': 'b',
            'undefined4': 'd', 'short': 's', 'undefined2': 'w', 'ushort': 'w', 'double': 'F', 'undefined8': 'F',
            'longlong': 'L', 'ulonglong': 'L'}


def tcode(ty):
    ty = ty.strip()
    if ty.endswith('*'):
        ty = ty[:-1].strip()
    if ty.endswith('*'):
        return 'p'
    return TYPECODE.get(ty, 'x')


def offs(o):
    v = int(o, 16) if o.startswith('0x') else int(o)
    return '%x' % v


FIELD1 = re.compile(r"\*\(([A-Za-z_][\w ]*?(?: \*+)?) ?\)\(\(int\)(\w+) \+ (0x[0-9a-f]+|\d+)\)")
FIELD2 = re.compile(r"\*\(([A-Za-z_][\w ]*?(?: \*+)?) ?\)\(([A-Za-z_]\w*(?:(?:\.|->)[a-z]\w*)*) \+ (0x[0-9a-f]+|\d+)\)")
FIELD3 = re.compile(r"\*\(([A-Za-z_][\w ]*?(?: \*+)?) ?\)\(\(int\)(\w+(?:(?:\.|->)[a-z]\w*)*) \+ (0x[0-9a-f]+|\d+)\)")


PTRSIZE = {'pi': 4, 'pu': 4, 'pf': 4, 'ps': 2, 'pw': 2, 'pc': 1, 'pb': 1}


def ptr_sizes(t):
    # a variable declared as a pointer type scales its offsets in `(p + n)`: 4 for int *, undefined4 *, float *
    sizes = {}
    for m in re.finditer(r'\b(int|uint|undefined4|float|char|byte|undefined1|short|ushort|undefined2|void|[A-Za-z_]+) (\*+)(\w+)', t):
        ty, stars, name = m.group(1), m.group(2), m.group(3)
        sizes[name] = 1 if ty in ('char', 'byte', 'undefined1', 'void') and len(stars) == 1 else (2 if ty in ('short', 'ushort', 'undefined2') and len(stars) == 1 else 4)
    return sizes


def fields(t):
    sizes = ptr_sizes(t)

    def f2(m):
        base = m.group(2)
        head = base.split('.')[0].split('->')[0]
        scale = sizes.get(head, 1) if ('.' not in base and '->' not in base) else 1
        v = int(m.group(3), 16) if m.group(3).startswith('0x') else int(m.group(3))
        return '%s->%s%x' % (base, tcode(m.group(1)), v * scale)
    for _ in range(6):
        t0 = t
        t = FIELD1.sub(lambda m: '%s.%s%s' % (m.group(2), tcode(m.group(1)), offs(m.group(3))), t)
        t = FIELD3.sub(lambda m: '%s->%s%s' % (m.group(2), tcode(m.group(1)), offs(m.group(3))), t)
        t = FIELD2.sub(f2, t)
        if t == t0:
            break
    return t


def comp(t):
    t = re.sub(r'\b(?:thunk_)?FUN_([0-9a-f]{8})\b', lambda m: names.get(m.group(1), m.group(0)), t)
    t = re.sub(r's_\w+?_([0-9a-f]{8})\b', lambda m: esc(strs[m.group(1)]) if m.group(1) in strs else m.group(0), t)
    t = re.sub(r'&?DAT_([0-9a-f]{8})\b', lambda m: ('(str)' + esc(strs[m.group(1)])) if m.group(1) in strs else
               (('(str)' + esc(exe_str(m.group(1)))) if exe_str(m.group(1)) is not None else m.group(0)), t)
    t = re.sub(r"\(_String_base \*\)(\"[^\n]*?\")", r"\1", t)
    t = STRASSIGN.sub(lambda m: "  STRASSIGN(" + m.group(4) + ", " + m.group(5) + ");\n", t)
    t = STRINIT.sub("  STRINIT;\n", t)
    t = STRFREE.sub("  STRFREE;\n", t)
    t = EHSTATE.sub("", t)
    t = fields(t)
    return t


def addr_list(a):
    if '-' in a:
        lo, hi = [int(x, 16) for x in a.split('-')]
        out = []
        for p in glob.glob(f'{root}/*/*.c'):
            ad = int(os.path.basename(p)[:8], 16)
            if lo <= ad < hi:
                out.append((ad, p))
        return [p for _, p in sorted(out)]
    a = a.lower()[2:].zfill(8) if a.lower().startswith('0x') else a.lower().zfill(8)
    return (glob.glob(f'{root}/*/{a}_*.c') + glob.glob(f'{root}/{a}_*.c'))[:1] or [a]


minlines = 0
args = sys.argv[1:]
if args and args[0] == '-f':
    minlines = int(args[1])
    args = args[2:]
for a in args:
    for p in addr_list(a):
        if not os.path.exists(p):
            print('// no decomp for', p)
            continue
        t = open(p, encoding='latin1').read()
        t = comp(t)
        out = []
        for ln in t.split('\n'):
            if not ln.strip():
                continue
            if decl.match(ln):
                continue
            ln = re.sub(r'(?:ExceptionList = &local_10|local_8 = [^,;()]+|puStack_c = [^,;()]+|local_10 = ExceptionList),\s*', '', ln)
            if re.search(r'(pvStack_\w+|puStack_\w+|ExceptionList)\s*(=|;)', ln):
                continue
            if re.match(r'\s*(local_4|uStack_4)\b[^\n]*= ', ln):
                continue
            out.append(ln)
        if len(out) >= minlines:
            print('\n'.join(out))
            print()
