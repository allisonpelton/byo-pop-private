# ini2reg.py <registry.ini>: prints the port's emulated registry (a user folder's registry.ini: "[key]" sections of
# "name=dword:N" and "name=sz:text" lines under HKEY_CURRENT_USER) as a .reg file for `reg import`, so the original
# can start with the same player as the port (orig.sh's REG=).
import sys

out = ['REGEDIT4', '']
for raw in open(sys.argv[1], encoding='utf-8'):
    line = raw.rstrip('\r\n')
    if not line or line.startswith(';'):
        continue
    if line.startswith('[') and line.endswith(']'):
        out.append('')
        out.append('[HKEY_CURRENT_USER\\%s]' % line[1:-1])
        continue
    name, _, value = line.partition('=')
    kind, _, data = value.partition(':')
    if kind == 'dword':
        out.append('"%s"=dword:%08x' % (name, int(data) & 0xffffffff))
    elif kind == 'sz':
        out.append('"%s"="%s"' % (name, data.replace('\\', '\\\\').replace('"', '\\"')))
    else:
        sys.exit('ini2reg.py: a value of a kind not known: ' + line)
out.append('')
sys.stdout.write('\r\n'.join(out))
