"""bwa.py: read and write a player's file (users/<name>.bwa) as the game keeps it (src/Profile.cpp, "the file"): per value
its key, its type and the value; a table's entries each say whether one follows. For the coverage runs' start states.

    python -I bwa.py dump <file.bwa> [key...]           the values (tables nested), or only the keys named
    python -I bwa.py set <in.bwa> <out.bwa> key=value...  a copy with values changed or added: i:<n>, f:<x>, b:0|1,
                                                          s:<text>; key=- deletes the key
"""
import struct
import sys

INT, FLOAT, BOOL, STRING, TABLE, NONE = range(6)


class Reader:
    def __init__(self, data):
        self.d, self.p = data, 0

    def take(self, n):
        b = self.d[self.p:self.p + n]
        self.p += n
        return b

    def long(self):
        return struct.unpack('<i', self.take(4))[0]

    def boolean(self):
        return self.take(1) != b'\0'

    def string(self):
        n = struct.unpack('<H', self.take(2))[0]
        return self.take(n).decode('latin-1')

    def value(self):
        t = self.long()
        if t == INT:
            return (t, self.long())
        if t == FLOAT:
            return (t, self.string())
        if t == BOOL:
            return (t, self.boolean())
        if t == STRING:
            return (t, self.string())
        if t == TABLE:
            entries = []
            while True:
                more = self.boolean()
                skey = self.boolean()
                k = self.string() if skey else self.long()
                entries.append((skey, k, self.value()))
                if not more or self.p >= len(self.d):
                    break
            return (t, entries)
        return (t, None)


def read(path):
    r = Reader(open(path, 'rb').read())
    out = []
    while r.p < len(r.d):
        k = r.string()
        out.append((k, r.value()))
    return out


def w_value(buf, v):
    t, x = v
    buf += struct.pack('<i', t)
    if t == INT:
        buf += struct.pack('<i', x)
    elif t in (FLOAT, STRING):
        s = x.encode('latin-1')
        buf += struct.pack('<H', len(s)) + s
    elif t == BOOL:
        buf += b'\1' if x else b'\0'
    elif t == TABLE:
        for i, (skey, k, ev) in enumerate(x):
            buf += b'\1' if i + 1 < len(x) else b'\0'
            buf += b'\1' if skey else b'\0'
            if skey:
                s = k.encode('latin-1')
                buf += struct.pack('<H', len(s)) + s
            else:
                buf += struct.pack('<i', k)
            w_value(buf, ev)


def write(path, values):
    buf = bytearray()
    for k, v in values:
        s = k.encode('latin-1')
        buf += struct.pack('<H', len(s)) + s
        w_value(buf, v)
    open(path, 'wb').write(buf)


def show(v, indent=0):
    t, x = v
    if t != TABLE:
        return repr(x)
    pad = '  ' * (indent + 1)
    return '{\n' + ''.join('%s%s = %s\n' % (pad, k, show(ev, indent + 1)) for _, k, ev in x) + '  ' * indent + '}'


def parse(text):
    if text == '-':
        return None
    kind, _, rest = text.partition(':')
    return {'i': lambda: (INT, int(rest)), 'f': lambda: (FLOAT, '%f' % float(rest)),
            'b': lambda: (BOOL, rest == '1'), 's': lambda: (STRING, rest)}[kind]()


def main():
    if len(sys.argv) >= 3 and sys.argv[1] == 'dump':
        keys = set(sys.argv[3:])
        for k, v in read(sys.argv[2]):
            if not keys or k in keys:
                print('%s = %s' % (k, show(v)))
    elif len(sys.argv) >= 4 and sys.argv[1] == 'set':
        values = read(sys.argv[2])
        for arg in sys.argv[4:]:
            k, _, text = arg.partition('=')
            v = parse(text)
            values = [(kk, vv) for kk, vv in values if kk != k]
            if v is not None:
                values.append((k, v))
        values.sort(key=lambda kv: kv[0])
        write(sys.argv[3], values)
    else:
        sys.exit(__doc__)


main()
