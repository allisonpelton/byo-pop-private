#!/usr/bin/env python3
"""Read PopLua .luc bytecode (Bookworm Adventures' Lua 5.0.2 fork, header version byte 0x56).

    python games/bookwormadv/tools/luc.py stats <dir>          # parse every .luc below <dir>, opcode histogram
    python games/bookwormadv/tools/luc.py dis <file.luc>       # disassemble one file, nested functions indented
    python games/bookwormadv/tools/luc.py names <dir>          # every global / field name the scripts use, with counts

Chunk header (23 bytes): ESC "Lua", 0x56, endianness 1, 01, sizeof int/size_t/Instruction 4 4 4, SIZE_OP 6,
SIZE_A 8, SIZE_B 9, SIZE_C 9, sizeof(lua_Number) 8, then the test number. A function is: source, linedefined,
the implicit-self flag (int), nups, numparams, is_vararg, maxstacksize (bytes), line info, local variables (name,
startpc, endpc), upvalue names, constants (tag 0 nil, 1 bool, 3 double, 4 int32, 5 string), nested functions, code.
An instruction is OP:6 C:9 B:9 A:8 from the low bit; an RK operand >= 250 (MAXSTACK) is constant RK-250.
"""
import collections
import glob
import os
import struct
import sys

OPS = ('MOVE LOADK LOADBOOL LOADNIL GETUPVAL GETGLOBAL GETTABLE SETGLOBAL SETUPVAL SETTABLE NEWTABLE SELF '
       'ADD SUB MUL DIV POW MOD UNM NOT SIZ CONCAT JMP EQ LT LE TEST CALL TAILCALL RETURN FORLOOP TFORLOOP '
       'TFORPREP SETLIST SETLISTO CLOSE ALTSELF CONSTGLOBAL CONSTTABLE DEFGLOBAL DEFTABLE SETSELFORGLOBAL '
       'GETSELFORGLOBAL SELFORGLOBAL CALLSELFORGLOBAL TAILCALLSELFORGLOBAL INT BREAK CLOSURE').split()
MAXSTACK = 250
BX_OPS = {'LOADK', 'GETGLOBAL', 'SETGLOBAL', 'CLOSURE', 'CONSTGLOBAL', 'DEFGLOBAL', 'GETSELFORGLOBAL',
          'SETSELFORGLOBAL', 'SELFORGLOBAL', 'CALLSELFORGLOBAL', 'TAILCALLSELFORGLOBAL'}
SBX_OPS = {'JMP', 'FORLOOP', 'TFORPREP'}
RK_OPS = {'GETTABLE', 'SETTABLE', 'SELF', 'ADD', 'SUB', 'MUL', 'DIV', 'POW', 'MOD', 'EQ', 'LT', 'LE',
          'CONSTTABLE', 'DEFTABLE', 'ALTSELF'}


class Reader:
    def __init__(self, data):
        self.d = data
        self.p = 0

    def byte(self):
        v = self.d[self.p]
        self.p += 1
        return v

    def int(self):
        v = struct.unpack_from('<i', self.d, self.p)[0]
        self.p += 4
        return v

    def num(self):
        v = struct.unpack_from('<d', self.d, self.p)[0]
        self.p += 8
        return v

    def str(self):
        n = struct.unpack_from('<I', self.d, self.p)[0]
        self.p += 4
        if n == 0:
            return None
        v = self.d[self.p:self.p + n - 1]
        self.p += n
        return v.decode('latin-1')


def func(r):
    f = {'source': r.str(), 'line': r.int(), 'self': r.int(), 'nups': r.byte(), 'nparams': r.byte(),
         'vararg': r.byte(), 'maxstack': r.byte()}
    f['lines'] = [r.int() for _ in range(r.int())]
    f['locals'] = [(r.str(), r.int(), r.int()) for _ in range(r.int())]
    f['upvals'] = [r.str() for _ in range(r.int())]
    ks = []
    for _ in range(r.int()):
        t = r.byte()
        if t == 3:
            ks.append(r.num())
        elif t == 4:
            ks.append(r.int())
        elif t == 5:
            ks.append(r.str())
        elif t == 1:
            ks.append(bool(r.byte()))
        elif t == 0:
            ks.append(None)
        else:
            raise ValueError('constant tag %d at %#x' % (t, r.p))
    f['consts'] = ks
    f['protos'] = [func(r) for _ in range(r.int())]
    n = r.int()
    f['code'] = list(struct.unpack_from('<%dI' % n, r.d, r.p))
    r.p += 4 * n
    return f


def load(path):
    d = open(path, 'rb').read()
    if d[:5] != b'\x1bLuaV':
        raise ValueError('%s: not a PopLua chunk' % path)
    r = Reader(d)
    r.p = 23
    f = func(r)
    if r.p != len(d):
        raise ValueError('%s: %d trailing bytes' % (path, len(d) - r.p))
    return f


def walk(f):
    yield f
    for p in f['protos']:
        yield from walk(p)


def decode(i):
    op = i & 63
    return op, i >> 24, (i >> 15) & 511, (i >> 6) & 511, (i >> 6) & 0x3ffff


def dis(f, ind=''):
    print('%sfunction line %d self %d params %d vararg %d ups %d maxstack %d' % (
        ind, f['line'], f['self'], f['nparams'], f['vararg'], f['nups'], f['maxstack']))
    if f['locals']:
        print('%s  locals %s' % (ind, ' '.join('%s[%d-%d]' % l for l in f['locals'])))
    if f['upvals']:
        print('%s  upvalues %s' % (ind, f['upvals']))
    k = f['consts']

    def rk(x):
        return repr(k[x - MAXSTACK]) if x >= MAXSTACK and x - MAXSTACK < len(k) else 'r%d' % x

    for pc, ins in enumerate(f['code']):
        op, a, b, c, bx = decode(ins)
        name = OPS[op] if op < len(OPS) else 'OP%d' % op
        line = f['lines'][pc] if pc < len(f['lines']) else '?'
        if name in BX_OPS:
            args = 'A=%d Bx=%d' % (a, bx)
            if name != 'CLOSURE' and bx < len(k):
                args += '  %r' % (k[bx],)
        elif name in SBX_OPS:
            args = 'A=%d -> %d' % (a, pc + 1 + bx - 131071)
        else:
            args = 'A=%d B=%d C=%d' % (a, b, c)
            if name in RK_OPS:
                args += '  %s %s' % (rk(b), rk(c))
        print('%s  [%4d] L%-5s %-20s %s' % (ind, pc, line, name, args))
    for p in f['protos']:
        dis(p, ind + '    ')


def files_under(root):
    return sorted(glob.glob(os.path.join(root, '**', '*.luc'), recursive=True))


def main():
    if len(sys.argv) < 3:
        sys.exit(__doc__)
    cmd, arg = sys.argv[1], sys.argv[2]
    if cmd == 'dis':
        dis(load(arg))
    elif cmd == 'stats':
        nfun = nins = nfile = 0
        ops = collections.Counter()
        bad = []
        for path in files_under(arg):
            try:
                f = load(path)
            except Exception as e:  # noqa: BLE001
                bad.append((path, repr(e)))
                continue
            nfile += 1
            for g in walk(f):
                nfun += 1
                nins += len(g['code'])
                for i in g['code']:
                    ops[i & 63] += 1
        print('files %d functions %d instructions %d bad %d %s' % (nfile, nfun, nins, len(bad), bad[:3]))
        print(' '.join('%s:%d' % (OPS[o] if o < len(OPS) else o, n) for o, n in sorted(ops.items())))
    elif cmd == 'names':
        names = collections.Counter()
        for path in files_under(arg):
            for g in walk(load(path)):
                for s in g['consts']:
                    if isinstance(s, str) and s.isidentifier():
                        names[s] += 1
        for n, c in sorted(names.items()):
            print(c, n)
    else:
        sys.exit(__doc__)


if __name__ == '__main__':
    main()
