"""usage: python -I syn.py <worktree> <source.cpp>...   -- syntax-checks each file with the flags of StageInfo.cpp"""
import json, subprocess, sys, shlex

import os
root = os.getcwd()
cc = json.load(open(root + '/build/native/compile_commands.json'))
base = None
for e in cc:
    if e['file'].replace('\\', '/').endswith(os.environ.get('SYN_BASE','feedingfrenzy/game/StageInfo.cpp')):
        base = e
        break
if base is None:
    print('no base command')
    sys.exit(2)
parts = shlex.split(base['command'], posix=False)
# drop -o <x>, -c <file>, -MD/-MT/-MF stuff
out = []
skip = 0
for i, p in enumerate(parts):
    if skip:
        skip -= 1
        continue
    if p in ('-o', '-MT', '-MF'):
        skip = 1
        continue
    if p in ('-c', '-MD', '-MMD'):
        continue
    if p.endswith(os.environ.get('SYN_BASE','feedingfrenzy/game/StageInfo.cpp').split('/')[-1]):
        continue
    out.append(p)
failed = 0
for f in sys.argv[1:]:
    import os
    cmd = out + ["-fsyntax-only", os.path.abspath(f)]
    r = subprocess.run(cmd, cwd=base['directory'], capture_output=True, text=True)
    text = (r.stdout + r.stderr)
    lines = [l for l in text.splitlines() if 'error' in l or 'warning' in l]
    print('==', f, 'exit', r.returncode, 'errors/warnings:', len(lines))
    for l in lines[:40]:
        print(l[:300])
    failed |= r.returncode
    if os.environ.get('SYN_FULL'):
        print(text[:8000])
sys.exit(1 if failed else 0)
