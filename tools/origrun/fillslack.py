"""Write text into the bytes just after a PE's section table, where Mahjong Escape's exes keep their copyright text, so
that covtest.sh can check that --coverage still finds room for its section header (origrun then moves the headers up
over the DOS stub).
  python -I tools/origrun/fillslack.py <exe>"""
import struct
import sys

path = sys.argv[1]
b = bytearray(open(path, 'rb').read())
nt = struct.unpack_from('<I', b, 0x3c)[0]
sections = struct.unpack_from('<H', b, nt + 6)[0]
optional_size = struct.unpack_from('<H', b, nt + 20)[0]
size_of_headers = struct.unpack_from('<I', b, nt + 24 + 60)[0]
table_end = nt + 24 + optional_size + 40 * sections
text = b'Copyright (C) a test of the bytes after the section table'
if table_end + len(text) > size_of_headers:
    sys.exit('fillslack: no slack after the section table of %s' % path)
b[table_end:table_end + len(text)] = text
open(path, 'wb').write(bytes(b))
print('fillslack: %d bytes at 0x%x' % (len(text), table_end))
