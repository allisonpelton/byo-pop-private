#!/usr/bin/env python3
"""Compare two PopLua chunks (a compiled script of the game and the same script compiled by the port's compiler).

    python games/bookwormadv/tests/poplua/cmpluc.py <a.luc> <b.luc> [--lines] [--locals] [--quiet] [--fa=N] [--fb=M] [--la=LINE] [--lb=LINE]

Functions are compared in order (the nested ones recursively): code, constants, upvalue names, parameters, vararg
flag, self flag, maximum stack, and with --lines the line information, with --locals the local variable names and
ranges. The first differing instructions of a function are printed side by side, disassembled. --fa and --fb compare only the Nth function of
a (or the Mth of b) in the order of luc.walk (0 is the main chunk); --la and --lb pick the first function
defined on that line. The exit status is the number of functions
that differ."""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(__file__), '..', '..', 'tools'))
import luc  # noqa: E402


def disasm(f, pc):
    op, a, b, c, bx = luc.decode(f['code'][pc])
    name = luc.OPS[op] if op < len(luc.OPS) else 'OP%d' % op
    k = f['consts']
    if name in luc.BX_OPS:
        args = 'A=%d Bx=%d' % (a, bx)
        if name != 'CLOSURE' and bx < len(k):
            args += ' %r' % (k[bx],)
    elif name in luc.SBX_OPS:
        args = 'A=%d -> %d' % (a, pc + 1 + bx - 131071)
    else:
        args = 'A=%d B=%d C=%d' % (a, b, c)
        if name in luc.RK_OPS:
            def rk(x):
                return repr(k[x - luc.MAXSTACK]) if x >= luc.MAXSTACK and x - luc.MAXSTACK < len(k) else 'r%d' % x
            args += ' %s %s' % (rk(b), rk(c))
    line = f['lines'][pc] if pc < len(f['lines']) else '?'
    return 'L%-4s %-18s %s' % (line, name, args)


def compare(a, b, path, opts, out):
    diffs = 0
    msgs = []
    for key in ('self', 'nups', 'nparams', 'vararg', 'maxstack'):
        if a[key] != b[key]:
            msgs.append('%s: %r vs %r' % (key, a[key], b[key]))
    if a['line'] != b['line'] and '--lines' in opts:
        msgs.append('line: %r vs %r' % (a['line'], b['line']))
    if a['upvals'] != b['upvals']:
        msgs.append('upvalues: %r vs %r' % (a['upvals'], b['upvals']))
    if a['consts'] != b['consts'] or [type(x) for x in a['consts']] != [type(x) for x in b['consts']]:
        msgs.append('constants: %r vs %r' % (a['consts'], b['consts']))
    if '--lines' in opts and a['lines'] != b['lines']:
        msgs.append('line info differs')
    if '--locals' in opts and a['locals'] != b['locals']:
        msgs.append('locals: %r vs %r' % (a['locals'], b['locals']))
    code_diff = a['code'] != b['code']
    if msgs or code_diff:
        diffs += 1
        out.append('function %s (line %d): DIFFERENT' % (path, a['line']))
        for m in msgs:
            out.append('  ' + m)
        if code_diff:
            n = min(len(a['code']), len(b['code']))
            first = next((i for i in range(n) if a['code'][i] != b['code'][i]), n)
            out.append('  code: %d vs %d instructions, first difference at pc %d' % (len(a['code']), len(b['code']), first))
            for pc in range(max(0, first - 2), min(max(len(a['code']), len(b['code'])), first + 6)):
                la = disasm(a, pc) if pc < len(a['code']) else ''
                lb = disasm(b, pc) if pc < len(b['code']) else ''
                out.append('   %4d %-52s | %s%s' % (pc, la, lb, '' if a['code'][pc:pc + 1] == b['code'][pc:pc + 1] else '   <<'))
        elif '--lines' in opts and a['lines'] != b['lines']:
            n = len(a['lines'])
            for pc in range(min(n, len(b['lines']))):
                if a['lines'][pc] != b['lines'][pc]:
                    out.append('   pc %d: line %d vs %d (%s)' % (pc, a['lines'][pc], b['lines'][pc], disasm(a, pc)))
                    break
    if len(a['protos']) != len(b['protos']):
        diffs += 1
        out.append('function %s: %d vs %d nested functions' % (path, len(a['protos']), len(b['protos'])))
    for i, (pa, pb) in enumerate(zip(a['protos'], b['protos'])):
        diffs += compare(pa, pb, '%s/%d' % (path, i), opts, out)
    return diffs


def main():
    args = [x for x in sys.argv[1:] if not x.startswith('--')]
    opts = [x for x in sys.argv[1:] if x.startswith('--')]
    if len(args) != 2:
        sys.exit(__doc__)
    a = luc.load(args[0])
    b = luc.load(args[1])
    for o in opts:
        if o.startswith('--fa='):
            a = list(luc.walk(a))[int(o[5:])]
        elif o.startswith('--fb='):
            b = list(luc.walk(b))[int(o[5:])]
        elif o.startswith('--la='):
            a = next(g for g in luc.walk(a) if g['line'] == int(o[5:]))
        elif o.startswith('--lb='):
            b = next(g for g in luc.walk(b) if g['line'] == int(o[5:]))
    out = []
    n = compare(a, b, '0', opts, out)
    if '--quiet' not in opts or n:
        print('\n'.join(out) if out else 'identical')
    sys.exit(min(n, 255))


if __name__ == '__main__':
    main()
