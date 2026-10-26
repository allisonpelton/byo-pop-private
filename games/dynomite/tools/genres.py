"""Draft the port's resource fields and loading code from the folded loader (tools/loadtable.py): one field per
`+<offset> = Kind(...)` line, named after its file, with the original's offset in a comment, and the loads in the
original's order. Lines loadtable.py did not fold are written as `// RAW:` comments, to be translated by hand.

    python games/dynomite/tools/decread.py 419c30 | python games/dynomite/tools/loadtable.py > re/dynomite/inithook.tbl
    python games/dynomite/tools/genres.py re/dynomite/inithook.tbl > re/dynomite/genres.txt
"""
import re, sys

TYPES = {'Image': 'DDImage*', 'Pic': 'Pic*', 'Cell': 'Cell*', 'Rect': 'Cell*', 'Sound': 'GameSound*'}
fields = {}      # offset -> (type, name)
used = set()
body = []
line_re = re.compile(r'^\+([0-9a-f]+) = (\w+)\((.*)\)$')


def base(path):
    return re.sub(r'[^A-Za-z0-9]', '', path.split('\\\\')[-1].split('\\')[-1]).capitalize()


for ln in open(sys.argv[1], encoding='latin1').read().split('\n'):
    m = line_re.match(ln)
    if not m:
        if ln.strip() and not ln.startswith(('//', '/*', 'void', '{', '}')):
            body.append('\t// RAW: ' + ln.strip())
        continue
    off, kind, args = int(m.group(1), 16), m.group(2), m.group(3)
    if kind in ('Image', 'Pic', 'Sound'):
        path = re.match(r'"([^"]*)"', args).group(1)
        nm = {'Image': 'mImg', 'Pic': 'mPic', 'Sound': 'mSnd'}[kind] + base(path)
        if nm in used:
            nm += '_%x' % off
        used.add(nm)
        fields[off] = (TYPES[kind], nm)
        if kind == 'Image':
            body.append('\t%s = GetGameImage("%s");' % (nm, path))
        elif kind == 'Pic':
            flag = 'true' if "\\x01" in args or "'\\x01'" in args else 'false'
            body.append('\t%s = new Pic(this, "%s", %s);' % (nm, path, flag))
        else:
            body.append('\t%s = new GameSound(this, "%s");' % (nm, path))
    else:
        src = re.match(r'\+([0-9a-f]+), param_1\.\w+,(.*)', args)
        soff = int(src.group(1), 16)
        nums = [int(x, 0) for x in src.group(2).split(',')]
        sname = fields[soff][1] if soff in fields else 'FIELD_%x' % soff
        if kind == 'Cell':
            nm = '%s_c%d' % (sname, nums[2])
            if nm in used:
                nm += '_%x' % off
            body.append('\t%s = new Cell(%s, %d, %d, %d);' % (nm, sname, nums[0], nums[1], nums[2]))
        else:
            nm = '%s_r%x' % (sname, off)
            body.append('\t%s = new Cell(%s, %d, %d, %d, %d);' % (nm, sname, nums[0], nums[1], nums[2], nums[3]))
        used.add(nm)
        fields[off] = ('Cell*', nm)
        body[-1] += ''

print('// ---- fields')
for off in sorted(fields):
    t, nm = fields[off]
    print('\t%-12s %-40s // +0x%x' % (t, nm + ';', off))
print('// ---- loading')
print('\n'.join(body))
