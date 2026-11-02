#!/usr/bin/env python3
"""Write games/bejeweledtwist/docs/symbols/30_res.jsonl: the resource globals of BejeweledTwist.exe, named by their ids.

ResourceGen3's extract functions (at 0x73e900-0x749000) store each resource in a global, e.g.
DAT_0085dde0 = FUN_0073e93c("IMAGE_INGAMEUI_GRIDBAR_BOT"), and fonts and sounds the same way with their index
(FUN_0073ea05(2, "FONT_..."), FUN_0073ea76(0x2fd, "SOUND_...")). This reads them from the Ghidra export
(re/bejeweledtwist/gh/all.c) and names each global after its id; the ids are identifiers, not game text.

    python games/bejeweledtwist/tools/ressyms.py
"""
import json
import re

GETTERS = {'0073e93c': 'image', '0073ea05': 'font', '0073ea76': 'sound'}
pat = re.compile(r'_?DAT_([0-9a-f]{8}) = FUN_(0073e93c|0073ea05|0073ea76)\((?:(0x[0-9a-f]+|\d+),)?"((?:IMAGE|FONT|SOUND)_\w+)"\)')
seen = {}
for m in pat.finditer(open('re/bejeweledtwist/gh/all.c', encoding='latin1').read()):
    addr, fn, idx, rid = m.groups()
    if addr in seen and seen[addr]['label'] != rid:
        raise SystemExit('%s is %s and %s' % (addr, seen[addr]['name'], rid))
    seen[addr] = {'addr': addr, 'label': rid}
    if idx:
        seen[addr]['comment'] = '%s index %d' % (GETTERS[fn], int(idx, 0))
with open('games/bejeweledtwist/docs/symbols/30_res.jsonl', 'w', newline='\n') as f:
    f.write('// the resource globals, named by their ids (tools/ressyms.py)\n')
    for a in sorted(seen):
        f.write(json.dumps(seen[a]) + '\n')
print(len(seen), 'resource globals')
