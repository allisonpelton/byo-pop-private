"""usage: python -I games/feedingfrenzy/tools/vtdiff.py <base_vt_hex> <vt_hex>... -- prints the slots of each vtable that differ from the base"""
import subprocess, sys, re
import os
root = os.getcwd()
def slots(vt):
    out = subprocess.run([sys.executable, '-I', root + '/games/feedingfrenzy/tools/fn.py', '--vt', vt], capture_output=True, text=True, cwd=root).stdout
    d = {}
    for m in re.finditer(r'\b([0-9a-f]+):([0-9a-f]{6})', out):
        d[int(m.group(1), 16)] = m.group(2)
    return d
base = slots(sys.argv[1])
for vt in sys.argv[2:]:
    s = slots(vt)
    print('==', vt, 'slots', len(s))
    print('  ' + '  '.join('%x:%s' % (k, s[k]) for k in sorted(s) if base.get(k) != s[k]))
