"""worklist.py <sort.tsv> <out.md>: the functions nonelines.py left to read ("read" in its tsv), each with what helps
place it: its size, strings and vtables (the export), its callers and callees that the port defines (the port
function and where it is, from tools/coverage.py's reading of the sources), how many callers the port does not define,
and any mention in the port, with the game's own name for it where docs/symbols has one. Grouped by the exe's regions, in address order."""
import importlib.util
import json
import os
import sys
from collections import defaultdict

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))
spec = importlib.util.spec_from_file_location('coverage', os.path.join(ROOT, 'tools', 'coverage.py'))
cov = importlib.util.module_from_spec(spec)
spec.loader.exec_module(cov)
REGIONS = ((0x4cce30, 'the game (0x401000-0x4cce30)'), (0x52f000, 'FlashWidget and PopAnim (0x527ea0-0x52efc0)'),
           (0x6a0000, 'LuaApp (0x650000-0x6a0000)'), (0x6c9a00, 'JeffLib (0x6a0000-0x6c9a00)'),
           (0x6ee000, 'LuaLib (0x6c9a00-0x6ee000)'), (0x71f000, 'PopLua, the Lua VM (0x6ee000-0x71efb0)'))


def main():
    tsv, out = sys.argv[1], sys.argv[2]
    cfg = cov.settings('bookwormadv')
    fns = cov.load_export(cfg)
    sites = cov.port_sites(cfg, fns)
    defs = {a: [s for s in ss if s[0] == 'def'] for a, ss in sites.items()}
    owners = defaultdict(set)
    for line in open(os.path.join(ROOT, cfg['export'].replace('functions.jsonl', 'vtables.jsonl')), encoding='utf-8'):
        v = json.loads(line)
        for s in v['slots']:
            owners[int(s['addr'], 16)].add(v['class'])

    def port(a):
        d = defs.get(a)
        return '%s:%d %s' % (d[0][1], d[0][2], d[0][3]) if d else None

    names = {}     # the game's own names for the export (docs/symbols, as tools/asm.py reads them)
    sym = os.path.join(ROOT, 'games', 'bookwormadv', 'docs', 'symbols')
    for n in sorted(os.listdir(sym)):
        for line in open(os.path.join(sym, n), encoding='utf-8'):
            if line.startswith('{'):
                e = json.loads(line)
                names[int(e['addr'], 16)] = e['name']
    todo = [int(l.split('\t')[0], 16) for l in open(tsv) if l.split('\t')[1] == 'read']
    groups = defaultdict(list)
    for a in todo:
        groups[next(name for hi, name in REGIONS if a < hi)].append(a)
    with open(out, 'w', encoding='utf-8', newline='\n') as o:
        for _, name in REGIONS:
            if not groups[name]:
                continue
            o.write('## %s: %d\n\n' % (name, len(groups[name])))
            for a in groups[name]:
                f = fns[a]
                callers = [int(c, 16) for c in f.get('callers', []) if ':' not in c]
                callees = [int(c, 16) for c in f.get('callees', []) if ':' not in c]
                pc = [port(c) for c in callers if port(c)]
                pe = [port(c) for c in callees if port(c)]
                line = '- %08x size %d' % (a, f.get('size', 0))
                if a in names:
                    line += '; named %s' % names[a]
                if f.get('strings'):
                    line += '; strings %s' % ', '.join(repr(s[:40]) for s in f['strings'][:4])
                if owners.get(a):
                    line += '; vtables %s' % ', '.join(sorted(owners[a])[:3])
                line += '; callers %d (%d not in the port)' % (len(callers), len(callers) - len(pc))
                if pc:
                    line += ': ' + '; '.join(sorted(set(pc))[:4])
                if pe:
                    line += '; calls ' + '; '.join(sorted(set(pe))[:3])
                ms = [s for s in sites.get(a, []) if s[0] != 'def']
                if ms:
                    line += '; mentioned %s:%d' % (ms[0][1], ms[0][2])
                o.write(line + '\n')
            o.write('\n')
    print('%d functions -> %s' % (len(todo), out))


main()
