"""Print the address of each game string that contains the given text (to write GT(0x...) in the port; for
reading only). The strings come from the Ghidra export, plus the ones it did not mark (found in the exe).

    python games/atomica/tools/stringaddr.py "Version " "CLICK HERE"
"""
import json, re, sys
rows = {}
for l in open('re/atomica/gh/strings.jsonl'):
    j = json.loads(l)
    if j['type'] == 'string':
        rows[int(j['addr'], 16)] = j['value']
exe = open('re/atomica/bin/Atomica.exe', 'rb').read()
for m in re.finditer(rb'[\x20-\x7e\t\n\r]{2,}\x00', exe[0xae000:0xcb000]):
    a = 0x4ae000 + m.start()
    rows.setdefault(a, m.group(0)[:-1].decode('latin1'))
for pat in sys.argv[1:]:
    for a in sorted(rows):
        if pat.lower() in rows[a].lower():
            print('0x%08x' % a, repr(rows[a]))
