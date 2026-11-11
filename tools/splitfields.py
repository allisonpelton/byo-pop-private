#!/usr/bin/env python3
"""Finds fields of a class that a port declares twice: one field of the original as two members, by name in one file and
by its offset (or another name) in another, so that what one side writes the other never reads.

Bejeweled 2's board had fourteen (2026-11-11): ExplodeAt set `mExplodeHappened` while the update that plays the blast's
sound read `m15e78`, both the original's +0x15e78, so a flame gem exploded in silence; the combo sound's step, the stars'
turn, Cognito's counters and the intro puzzle's flag were split the same way. Nothing fails to compile, and no comparison
of the game's state shows it when the field only feeds a sound, a picture or a statistic.

    python tools/splitfields.py <game> [--export <folder>]... [--class <name>] [--min 0x40] [--all]

It reads games/<game>/src (or --src) and reports, class by class:

  same offset   two members of one class with the same offset, taken from a `// +0x1c4` comment on the member's line or
                from a member's name (m1c4, mField1C4, unk1c4). Certain, unless the two are meant to overlay each other.
  never written a scalar or pointer member that is read somewhere and assigned nowhere, not even in a constructor: the
                other half of a split field, or a read of memory nobody set. Only for classes with a constructor in the
                sources, and not for one most of whose members are like that (a record read from a file as a whole).
  write only    a scalar or pointer member that is assigned outside the constructors and read nowhere. One without a known
                offset is the likely half of a split field (the port named it, wrote it, and reads the field under its
                other name); one with an offset is listed with --all only, since the original has many fields it never reads.
  export        with --export: a member whose offset a function of the Ghidra export touches while the port's translation
                of that function never names the member, so the translation uses another name for the field there (or
                leaves the access out). This is the check that found every one of Bejeweled 2's; the members that the
                export's function writes come first ("export"), those it only reads after them ("export, read only":
                more of these are harmless, a value the translation gets another way). It needs the port's
                functions to carry their address at the start of a comment line above them (`// 0x5a68b0`) and the
                export as one file per function named <8 hex digits>_<name>.c, in any folder below --export.

The last line counts the findings: `splitfields <game>: same offset N, never written N, write only N (+N with an offset),
export N members in N functions (N of them written there)`. Every finding is a place to read, not a verdict: open the function in the port and in the
export and see which name it used.

Limits. Reads and writes are told apart by the text around a name (`x = `, `x++`, `&x`, a NAME(...) macro's argument counts
as both), over the game's own sources only, and only for members whose name no other class of the game declares, since a
use is matched by name alone. A split whose two halves are each read and written in their own files shows only with the
export. The export check matches accesses written as `this + 0x1c4)` or `this[0x71]` (taken as four bytes a slot) on the
names Ghidra gives the object (this, param_1, in_ECX, extraout_ECX, unaff_*), so it misses other shapes, flags an offset of
another object that a function happens to hold in param_1, and is off by the base's size in the methods of a second base
class. --min leaves out the low offsets, where those collisions are.
"""
import argparse
import os
import re
import sys

SCALAR = re.compile(r'^(?:(?:unsigned|signed|const|volatile|mutable|short|long|int|char|bool|float|double|uchar|ushort|uint|'
                    r'ulong|u?int(?:8|16|32|64)_t|size_t|BYTE|WORD|DWORD|BOOL|SexyChar)\s*)+$')
LITERALS = re.compile(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'', re.S)
OBJECT = r'(?:this|param_1|in_ECX|extraout_ECX|unaff_\w+)'
TOUCH = re.compile(OBJECT + r'(?:\s*\+\s*0x([0-9a-f]+)\)(\s*=[^=])?|\[0x([0-9a-f]+)\](\s*=[^=])?)')
SKIP = re.compile(r'^(?:typedef|using|friend|enum|class|struct|union|template|static|virtual|extern|return|public|private|'
                  r'protected|operator)\b')


def blank(text):
    """The text with its comments and string literals turned to spaces, every position kept."""
    return LITERALS.sub(lambda m: re.sub(r'[^\n]', ' ', m.group(0)), text)


def name_offset(name):
    """The offset a member's name carries (m1c4, mField1C4, mFlag_e0, unk10c), or None."""
    m = re.match(r'^m(?=[0-9a-f]*\d)([0-9a-f]{2,5})$', name)
    if not m:
        m = re.match(r'^(?:m|unk|field)_?(?:Unk|Unknown|Field|Unused|Flag|F)?_?(?:0x)?(?=[0-9A-Fa-f]*\d)([0-9A-Fa-f]{2,5})$', name)
        if m and not re.match(r'^(?:unk|field|m(?:Unk|Unknown|Field|Unused|Flag|F|_))', name):
            m = None
    return int(m.group(1), 16) if m else None


class Member:
    def __init__(self, cls, name, type_, offset, named, header, line, inst):
        self.cls, self.name, self.type, self.offset, self.named, self.header, self.line = cls, name, type_, offset, named, header, line
        self.inst = inst  # the declaration it stands in: two classes of one name in two headers are two
        self.reads, self.writes, self.ctor, self.both = [], [], [], []

    def scalar(self):
        t = self.type.replace('*', ' * ').strip()
        return t.endswith('*') or bool(SCALAR.match(t + ' '))


def parse_header(path, members, classes):
    raw = open(path, errors='replace').read()
    text = blank(raw)
    spans = []
    for m in re.finditer(r'\b(?:class|struct)\s+(\w+)\s*(?:final\s*)?(?::[^{;]*)?\{', text):
        depth, i, start, stmt = 0, m.end(), m.end(), m.end()
        body = []
        while i < len(text):
            c = text[i]
            if c == '{':
                depth += 1
            elif c == '}':
                if depth == 0:
                    break
                depth -= 1
                if depth == 0:
                    stmt = i + 1
            elif c == ';' and depth == 0:
                body.append((stmt, i))
                stmt = i + 1
            elif c == ':' and depth == 0 and re.search(r'\b(?:public|private|protected)\s*$', text[stmt:i]):
                stmt = i + 1
            i += 1
        cls = m.group(1)
        classes.setdefault(cls, path)
        for a, b in body:
            s = text[a:b].strip()
            if not s or '(' in s or '~' in s or SKIP.match(s):
                continue
            parts, level, cur = [], 0, ''
            for ch in s:
                if ch in '<[{':
                    level += 1
                elif ch in '>]}':
                    level -= 1
                if ch == ',' and level == 0:
                    parts.append(cur)
                    cur = ''
                else:
                    cur += ch
            parts.append(cur)
            first = re.match(r'^(.*?)([A-Za-z_]\w*)\s*((?:\[[^\]]*\]\s*)*)(?:=.*|:\s*\d+)?$', parts[0].strip(), re.S)
            if not first or not first.group(1).strip():
                continue
            base = first.group(1).strip()
            line_end = raw.find('\n', b)
            comment = raw[b:line_end if line_end >= 0 else len(raw)]
            off = re.search(r'//\s*\+(?:0x)?([0-9a-fA-F]+)\b', comment)
            line = raw.count('\n', 0, b) + 1
            for k, part in enumerate(parts):
                d = first if k == 0 else re.match(r'^(\s*\**\s*)([A-Za-z_]\w*)\s*((?:\[[^\]]*\]\s*)*)(?:=.*|:\s*\d+)?$', part.strip(), re.S)
                if not d:
                    continue
                type_ = base if k == 0 else re.sub(r'[\s\*&]+$', '', base) + ' ' + d.group(1).strip()
                o = int(off.group(1), 16) if off and k == 0 else None
                named = False
                if o is None:
                    o = name_offset(d.group(2))
                    named = o is not None
                mem = Member(cls, d.group(2), type_.strip(), o, named, path, line, (path, m.start()))
                if k == 0 and '=' in parts[0]:
                    mem.ctor.append('%s:%d' % (os.path.basename(path), line))
                members.append(mem)
                spans.append((path, a, b))
    return spans


def scan_uses(files, by_name, classes, declared, with_ctor):
    macro = re.compile(r'\b[A-Z][A-Z0-9_]{2,}\s*\([^()\n]*$')
    for path in files:
        raw = open(path, errors='replace').read()
        text = blank(raw)
        ctors = []
        for cls in classes:
            for m in re.finditer(r'\b%s::%s\s*\(' % (cls, cls), text):
                end = text.find('\n}', m.end())
                ctors.append((m.start(), end if end >= 0 else len(text)))
                with_ctor.add(cls)
        skip = [(a, b) for p, a, b in declared if p == path]
        for m in re.finditer(r'[A-Za-z_]\w*', text):
            mem = by_name.get(m.group(0))
            if mem is None:
                continue
            s, e = m.start(), m.end()
            if any(a <= s < b for a, b in skip):
                continue
            where = '%s:%d' % (os.path.basename(path), text.count('\n', 0, s) + 1)
            rest = text[e:e + 120]
            rest = rest[re.match(r'\s*(?:\[[^\]]*\]\s*)*', rest).end():]
            before = text[max(0, s - 8):s].rstrip()
            line = text[text.rfind('\n', 0, s) + 1:s]
            if (before.endswith('&') and not before.endswith('&&')) or macro.search(line):
                mem.both.append(where)
            elif re.match(r'=[^=]|\+\+|--|(?:[-+*/|&^%]|<<|>>)=', rest) or before.endswith('++') or before.endswith('--'):
                (mem.ctor if any(a <= s < b for a, b in ctors) else mem.writes).append(where)
            elif rest[:1] in ('(', '{') and re.search(r'[,:]\s*$', text[max(0, s - 40):s]) and any(a <= s < b for a, b in ctors):
                mem.ctor.append(where)  # a constructor's initialiser list
            else:
                mem.reads.append(where)


def port_functions(files):
    """address -> (class, function, body without comments) for the definitions that carry their address in a comment."""
    out = {}
    for path in files:
        if not path.endswith(('.cpp', '.cc', '.inl')):
            continue
        raw = open(path, errors='replace').read()
        text = blank(raw)
        for m in re.finditer(r'(?m)^//\s*0x([0-9a-fA-F]{5,8})\b', raw):
            d = re.compile(r'(?m)^[^\s#/][^\n;{}]*?\b(\w+)::(~?\w+)\s*\(').search(text, m.end())
            if not d or raw.count('\n', m.end(), d.start()) > 14:
                continue
            nxt = re.compile(r'(?m)^//\s*0x[0-9a-fA-F]{5,8}\b').search(raw, m.end())
            if nxt and nxt.start() < d.start():
                continue
            end = text.find('\n}', d.end())
            out.setdefault(int(m.group(1), 16), (d.group(1), d.group(2), text[d.start():end if end >= 0 else len(text)]))
    return out


def export_index(folders):
    index = {}
    for folder in folders:
        for root, _, names in os.walk(folder):
            for n in names:
                m = re.match(r'^([0-9a-fA-F]{8})_.*\.c$', n)
                if m:
                    index.setdefault(int(m.group(1), 16), os.path.join(root, n))
    return index


def main():
    ap = argparse.ArgumentParser(description='fields of a class that a port declares twice (see the header of this file)')
    ap.add_argument('game')
    ap.add_argument('--src', default=None, help='the sources (default games/<game>/src)')
    ap.add_argument('--export', action='append', default=[], help="a folder of the Ghidra export's per-function files (<8 hex>_<name>.c, anywhere below it)")
    ap.add_argument('--class', dest='only', default=None, help='this class only')
    ap.add_argument('--min', default='0x40', help='the export check leaves out offsets below this (default 0x40)')
    ap.add_argument('--all', action='store_true', help='also list the write-only members that have a known offset')
    a = ap.parse_args()
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    src = a.src or os.path.join(root, 'games', a.game, 'src')
    files = sorted(os.path.join(r, n) for r, _, ns in os.walk(src) for n in ns if n.endswith(('.h', '.hpp', '.cpp', '.cc', '.inl')))
    if not files:
        sys.exit('splitfields: no sources in %s' % src)
    members, classes, declared = [], {}, []
    for f in files:
        if f.endswith(('.h', '.hpp')):
            declared += parse_header(f, members, classes)
    if a.only:
        members = [m for m in members if m.cls == a.only]
    owners = {}
    for m in members:
        owners.setdefault(m.name, set()).add(m.cls)
    by_name = dict((m.name, m) for m in members if len(owners[m.name]) == 1)
    with_ctor = set(m.cls for m in members if m.ctor)
    scan_uses(files, by_name, classes, declared, with_ctor)

    found = {}

    def note(cls, kind, text):
        found.setdefault(cls, {}).setdefault(kind, []).append(text)

    count = dict(same=0, never=0, wonly=0, wonly_off=0, exp_members=0, exp_funcs=0, exp_written=0)
    per_class, per_inst, inst_class = {}, {}, {}
    for m in members:
        inst_class[m.inst] = m.cls
        if m.offset is not None:
            for table, key in ((per_class, m.cls), (per_inst, m.inst)):
                table.setdefault(key, {}).setdefault(m.offset, [])
                if m.name not in table[key][m.offset]:
                    table[key][m.offset].append(m.name)
    for inst, offs in per_inst.items():
        for off, names in sorted(offs.items()):
            if len(names) > 1:
                count['same'] += 1
                note(inst_class[inst], 'same offset', '+0x%x %s' % (off, ' / '.join(names)))
    scalars, unset = {}, {}
    for m in by_name.values():
        if m.scalar():
            scalars[m.cls] = scalars.get(m.cls, 0) + 1
            if m.reads and not m.writes and not m.ctor and not m.both:
                unset.setdefault(m.cls, []).append(m)
    whole = set(c for c, ms in unset.items() if c not in with_ctor or (len(ms) >= 8 and len(ms) * 2 > scalars[c]))
    for m in by_name.values():
        if not m.scalar():
            continue
        if m.reads and not m.writes and not m.ctor and not m.both:
            if m.cls in whole:
                continue
            count['never'] += 1
            note(m.cls, 'never written', '%s (%s%s) read %dx, first %s' % (m.name, m.type, '' if m.offset is None else ', +0x%x' % m.offset, len(m.reads), m.reads[0]))
        elif m.writes and not m.reads and not m.both:
            if m.offset is None:
                count['wonly'] += 1
                note(m.cls, 'write only', '%s (%s) written %dx, first %s' % (m.name, m.type, len(m.writes), m.writes[0]))
            else:
                count['wonly_off'] += 1
                if a.all:
                    note(m.cls, 'write only, with an offset', '%s (%s, +0x%x) written %dx, first %s' % (m.name, m.type, m.offset, len(m.writes), m.writes[0]))

    if a.export:
        low = int(a.min, 16)
        index = export_index(a.export)
        funcs = port_functions(files)
        have = 0
        hits = {}
        for addr, (cls, fn, body) in sorted(funcs.items()):
            if addr not in index or cls not in per_class:
                continue
            have += 1
            touched = {}
            for t in TOUCH.finditer(open(index[addr], errors='replace').read()):
                off = int(t.group(1), 16) if t.group(1) else int(t.group(3), 16) * 4
                n, w = touched.get(off, (0, 0))
                touched[off] = (n + 1, w + (1 if (t.group(2) or t.group(4)) else 0))
            for off, (n, w) in touched.items():
                names = per_class[cls].get(off)
                if off < low or not names or any(re.search(r'\b%s\b' % re.escape(x), body) for x in names):
                    continue
                hits.setdefault((cls, off, names[0]), []).append((w, '0x%x %s (%d, %d written)' % (addr, fn, n, w)))
        for (cls, off, name), where in sorted(hits.items()):
            count['exp_members'] += 1
            count['exp_funcs'] += len(where)
            written = any(w for w, _ in where)
            count['exp_written'] += 1 if written else 0
            note(cls, 'export' if written else 'export, read only', '%s +0x%x: %s' % (name, off, '; '.join(t for _, t in where)))
        print('export: %d of the port\'s %d functions with an address have a file' % (have, len(funcs)))

    for cls in sorted(found):
        print('%s (%s)' % (cls, os.path.basename(classes.get(cls, '?'))))
        for kind in ('same offset', 'never written', 'write only', 'write only, with an offset', 'export', 'export, read only'):
            for line in found[cls].get(kind, []):
                print('  %s: %s' % (kind, line))
    shared = sum(1 for n in owners if len(owners[n]) > 1)
    print('splitfields %s: same offset %d, never written %d, write only %d (+%d with an offset)%s; %d classes, %d members, %d with a known '
          'offset, %d names shared by classes and left out of the read and write lists'
          % (a.game, count['same'], count['never'], count['wonly'], count['wonly_off'],
             ', export %d members in %d functions (%d of them written there)' % (count['exp_members'], count['exp_funcs'], count['exp_written']) if a.export else '',
             len(set(m.cls for m in members)), len(members), sum(1 for m in members if m.offset is not None), shared))
    return 0


if __name__ == '__main__':
    sys.exit(main())
