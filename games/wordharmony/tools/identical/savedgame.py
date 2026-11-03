# savedgame.py <name> <value name> <source>: writes <name>.ini (the port's registry.ini) and <name>.reg (the original's
# registry) holding one saved game, so that both programs start with the same one (case.sh's PORTREG and REG). The
# source is a file of the XML itself, a port's registry.ini, or a `reg export` of the original's key (UTF-16), so a game
# saved by either program can be continued in both. The .reg writes the text as hex(1), which keeps its newlines.
import re, sys

name, value, src = sys.argv[1], sys.argv[2], sys.argv[3]
raw = open(src, 'rb').read()
if raw[:2] == b'\xff\xfe':
    # a reg export: "Name"="text" or "Name"=hex(1):.. over lines ending in a backslash
    text = raw.decode('utf-16').replace('\\\r\n', '').replace('\\\n', '')
    # a REG_SZ keeps its newlines as they are, so the string runs to its closing quote
    m = re.search(r'^"%s"=(hex\(1\):[0-9a-f,\s]*|"(?:[^"\\]|\\.)*")' % re.escape(value), text, re.M)
    if not m:
        sys.exit('%s: no value %s' % (src, value))
    v = m.group(1).strip()
    if v.startswith('hex(1):'):
        data = bytes(int(h, 16) for h in re.sub(r'\s', '', v[7:]).split(',') if h)
        xml = data.decode('utf-16-le').rstrip('\0')
    else:
        xml = re.sub(r'\\(.)', r'\1', v[1:-1]).replace('\r\n', '\n')
else:
    text = raw.decode('utf-8')
    m = re.search(r'^%s=sz:(.*)$' % re.escape(value), text, re.M)
    if m:
        xml = re.sub(r'\\(.)', lambda e: {'n': '\n', 'r': '\r'}.get(e.group(1), e.group(1)), m.group(1).rstrip('\r'))
    else:
        xml = text.strip()
if not xml:
    sys.exit('%s: %s is empty' % (src, value))
esc = xml.replace('\\', '\\\\').replace('\n', '\\n').replace('\r', '\\r')
with open(name + '.ini', 'w', encoding='utf-8', newline='\n') as f:
    f.write('[SOFTWARE\\GameHouse\\WordHarmony]\n%s=sz:%s\n' % (value, esc))
hexs = ','.join('%02x' % b for b in (xml + '\0').encode('utf-16-le'))
with open(name + '.reg', 'w', encoding='utf-16', newline='') as f:
    f.write('Windows Registry Editor Version 5.00\r\n\r\n[HKEY_CURRENT_USER\\Software\\GameHouse\\WordHarmony]\r\n'
            '"%s"=hex(1):%s\r\n' % (value, hexs))
print('%s: %s, %d characters' % (name, value, len(xml)))
