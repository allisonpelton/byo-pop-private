"""Fold the app's loading functions (FossilizedApp::InitHook 0x419c30, Init 0x41f820) into one line per resource:
`+<app field> = <what>(<arguments>)`, dropping the new/NULL checks and the loading-step counts. Lines it does not
recognise are printed as they are, so nothing of the logic between the loads is lost.

    python games/dynomite/tools/decread.py 419c30 | python games/dynomite/tools/loadtable.py
"""
import re, sys

KIND = {'GetImage': 'Image', 'FUN_00437210': 'Pic', 'FUN_00436d40': 'Rect', 'FUN_00436cb0': 'Cell',
        'FUN_00435b50': 'Sound', 'Sexy::ImageFont::ImageFont': 'Font', 'FUN_00478110': 'Font'}
lines = sys.stdin.read().split('\n')
pending = {}
skip = re.compile(r'^\s*(\w+ = operator_new\(0x[0-9a-f]+\);|if \(\w+ == \(void \*\)0x0\) \{|\w+ = \(\w+ \*?\)0x0;|\w+ = 0;|\}|else \{|'
                  r'local_\w+ = (?:\(void \*\))?0x[0-9a-f]+;|local_\w+ = \w+;)\s*$')
call = re.compile(r'^\s*(\w+) = (?:\(\w+ \*?\))?([\w:]+)\((.*)\);\s*$')
store = re.compile(r'^\s*param_1(?:\.[a-z]+|\[)(0x[0-9a-f]+|[0-9a-f]+)\]?(?:\.unused)? = (?:\(\w+\))?(\w+);\s*$')
for ln in lines:
    m = call.match(ln)
    if m and m.group(2) in KIND:
        args = re.sub(r'^\s*(?:\w+|\(\w+ \*\)\w+),\s*(?:param_1|\(int\)param_1),\s*', '', m.group(3))
        args = re.sub(r'^\s*\w+,\s*', '', args) if KIND[m.group(2)] in ('Rect', 'Cell') else args
        args = re.sub(r'^param_1,', '', args)
        src = re.match(r'\s*\w+,\s*(param_1\.\w+)', m.group(3))
        pending[m.group(1)] = '%s(%s%s)' % (KIND[m.group(2)], (src.group(1).replace('param_1.', '+')[1:] and '+' + src.group(1).split('.')[-1][1:] + ', ') if src and KIND[m.group(2)] in ('Rect', 'Cell') else '', args.strip())
        continue
    m = store.match(ln)
    if m and m.group(2) in pending:
        off = m.group(1)
        off = int(off, 16) if '[' not in ln or off.startswith('0x') else int(off, 16)
        if '[' in ln:
            off *= 4
        print('+%x = %s' % (off, pending.pop(m.group(2))))
        continue
    if re.match(r'^\s*param_1\.i4e8 = param_1\.i4e8 \+ 1;\s*$', ln):
        print('STEP')
        continue
    if skip.match(ln):
        continue
    print(ln)
