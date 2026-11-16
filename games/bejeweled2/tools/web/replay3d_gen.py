"""Writes games/bejeweled2/tools/web/replay3d_gen.js: replay3d.js with a native commands file, the player's profile and the seed in front.
    python games/bejeweled2/tools/web/replay3d_gen.py <commands.txt> [--profile build/logs/bej2/profile] [--seed 909]"""
import base64, json, os, sys

a = sys.argv[1:]
cmds = a[0]
profile = a[a.index('--profile') + 1] if '--profile' in a else 'build/logs/bej2/profile'
seed = int(a[a.index('--seed') + 1]) if '--seed' in a else 909
here = os.path.dirname(os.path.abspath(__file__))
files = {}
users = os.path.join(profile, 'users')
for root, _, names in os.walk(users):
    for n in names:
        p = os.path.join(root, n)
        files[os.path.relpath(p, profile).replace(os.sep, '/')] = base64.b64encode(open(p, 'rb').read()).decode()
lines = [l.strip() for l in open(cmds) if l.strip()]
head = 'var CMDS = %s, FILES = %s, SEED = %d;\n' % (json.dumps(lines), json.dumps(files), seed)
open(os.path.join(here, 'replay3d_gen.js'), 'w').write(head + open(os.path.join(here, 'replay3d.js')).read())
print('%d commands, %d profile files' % (len(lines), len(files)))
