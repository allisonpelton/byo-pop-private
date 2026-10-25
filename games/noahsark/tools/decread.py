"""Print a function of the Ghidra export (re/noahsark/gh/decomp) in a compact, readable form: names from
games/noahsark/docs/symbols, game strings inlined (for reading only, never written to a file in the repository),
inlined MSVC 6 string and vector idioms collapsed.

    python games/noahsark/tools/decread.py 4092a0 ...
"""
import sys, glob, os, re, json
root = 're/noahsark/gh/decomp'
strs = {}
for l in open('re/noahsark/gh/strings.jsonl'):
    j = json.loads(l)
    if j['type'] == 'string':
        strs[j['addr']] = j['value']
names = {}
for p in glob.glob('games/noahsark/docs/symbols/*.jsonl'):
    for l in open(p):
        l = l.strip()
        if l:
            j = json.loads(l)
            names[j['addr'].lower().zfill(8)] = j['name'].split('::')[-1]
decl = re.compile(r'^\s*(undefined\d?|int|uint|char|byte|bool|float|double|long|short|ushort|longlong|ulonglong|void|LPVOID|DWORD|code|[A-Za-z_]+ \*)\s*\*?\s*(\*)?[A-Za-z_]\w*(\[\d+\])?;\s*$')
BS = chr(92)
NL = BS + 'n'
Z = BS + '0'          # the two characters backslash-zero as Ghidra prints them


def esc(s):
    return '"' + s.replace(BS, BS + BS).replace('"', BS + '"').replace('\n', NL).replace('\r', BS + 'r') + '"'


STRLEN = re.compile(r"[ ]*(\w+) = 0xffffffff;\n\s*(\w+) = ([^;\n]+);\n\s*do \{\n[^\n]*\n[^\n]*\n[^\n]*\n[^\n]*\n\s*\} while \([^\n]*\);\n")
LOOPS = re.compile(r"\n[ ]*for \([^\n]*!= 0; [^\n]*- 1\) \{\n(?:[^\n]*\n){3,4}?[ ]*\}")
REFC = re.compile(r"if \(([^\n]*?) != (?:0|\(\w+ \*\)0x0)\) \{\n\s*(\w+) = \*\(char \*\)\(?([^\n]*?) \+ -1\)?;\n\s*if \(\(\2 == '" + re.escape(Z) + r"'\) \|\| \(\2 == -1\)\) \{\n\s*operator_delete\([^\n]*\);\n\s*\}\n\s*else \{\n[^\n]*\n\s*\}\n\s*\}")
STRSET = re.compile(r"[ ]*\w+ = string_grow\(([^,\n]+),[^\n]*\);\n([ ]*)if \(\(char\)\w+ != '" + re.escape(Z) + r"'\) \{\n[ ]*\w+ = (\"[^\n]*\");\n(?:[^\n]*\n)*?\2\}\n")
PATA = re.compile(r"[ ]*string_tidy\(([^,\n]+),'" + re.escape(Z) + r"'\);\n((?:[^\n]*\n){0,3}?)[ ]*\w+ = string_grow\(\1,[^\n]*\n[ ]*if \(\(char\)\w+ != '" + re.escape(Z) + r"'\) \{\n(?:[^\n]*\n){0,6}?[ ]*string_eos\([^\n]*\n[ ]*\}\n")


def pata_sub(m):
    inner = m.group(2)
    sm = re.search(r"strlen\(([^\n]*)\)", inner)
    src = sm.group(1) if sm else "?"
    return "  STRCPY(" + m.group(1) + ", " + src + ");\n"


def strlen_pass(t):
    while True:
        m = STRLEN.search(t)
        if not m:
            return t
        v, e = m.group(1), m.group(3)
        rest = t[m.end():]
        rest2 = re.sub(r"~" + v + r" - 1", "strlen(" + e + ")", rest, count=1)
        t = t[:m.start()] + rest2



def block_end(lines, i):
    """index of the line closing the statement that opens at lines[i] (an if/else chain or loop)"""
    depth = 0
    j = i
    while j < len(lines):
        depth += lines[j].count('{') - lines[j].count('}')
        if depth <= 0:
            nxt = lines[j + 1].strip() if j + 1 < len(lines) else ''
            if lines[j].strip().startswith('}') and (nxt.startswith('else') ):
                j += 1
                continue
            return j
        j += 1
    return len(lines) - 1


def collapse_blocks(t):
    lines = t.split('\n')
    out = []
    i = 0
    while i < len(lines):
        ln = lines[i]
        st = ln.strip()
        rep = None
        m = re.match(r'(\s*)(\w+) = operator_new\(0x68\);', ln)
        if m and i + 1 < len(lines) and re.match(r'\s*if \(' + m.group(2) + r' == ', lines[i + 1]):
            e = block_end(lines, i + 1)
            rep = (m.group(1) + m.group(2) + ' = NEW_PIECE();', e)
        elif re.match(r'\s*if \(.*>> 2 == 0\) \{', ln):
            e = block_end(lines, i)
            rep = (re.match(r'\s*', ln).group(0) + 'VPUSH;', e)
        elif st.startswith('while (') and st.endswith('{'):
            e = block_end(lines, i)
            body = '\n'.join(lines[i:e + 1])
            if 'operator_delete' in body and '+ -4;' in body:
                rep = (re.match(r'\s*', ln).group(0) + 'VCLEAR_ELEMENTS;', e)
            elif '+ -4;' in body and 'while' in st and '*(undefined4 *)' in body and 'operator_delete' not in body:
                rep = (re.match(r'\s*', ln).group(0) + 'VCLEAR_ELEMENTS;', e)
        if rep:
            out.append(rep[0])
            i = rep[1] + 1
        else:
            out.append(ln)
            i += 1
    return '\n'.join(out)


def comp(t):
    t = re.sub(r'\b(?:thunk_)?FUN_([0-9a-f]{8})\b', lambda m: names.get(m.group(1), m.group(0)), t)
    t = strlen_pass(t)
    t = LOOPS.sub("\n  /*memcpy*/", t)
    t = re.sub(r's_\w+?_([0-9a-f]{8})\b', lambda m: esc(strs[m.group(1)]) if m.group(1) in strs else m.group(0), t)
    t = re.sub(r'&?DAT_([0-9a-f]{8})\b', lambda m: ('(str)' + esc(strs[m.group(1)])) if m.group(1) in strs else m.group(0), t)
    t = PATA.sub(pata_sub, t)
    t = STRSET.sub(lambda m: "  STRSET(" + m.group(1) + ", " + m.group(3) + ");\n", t)
    t = REFC.sub(r'STRFREE(\1);', t)
    t = collapse_blocks(t)
    return t


for a in sys.argv[1:]:
    a = a.lower()[2:].zfill(8) if a.lower().startswith('0x') else a.lower().zfill(8)
    fs = glob.glob(f'{root}/*/{a}_*.c') + glob.glob(f'{root}/{a}_*.c')
    if not fs:
        print('// no decomp for', a)
        continue
    t = open(fs[0], encoding='latin1').read()
    t = comp(t)
    out = []
    for ln in t.split('\n'):
        if not ln.strip():
            continue
        if decl.match(ln):
            continue
        if re.search(r'(pvStack_\w+|puStack_\w+|ExceptionList)\s*(=|;)', ln):
            continue
        if re.match(r'\s*(local_4|uStack_4)\b[^\n]*= ', ln):
            continue
        out.append(ln)
    print('\n'.join(out))
    print()
