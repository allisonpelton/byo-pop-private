"""patchsave.py <in.GAM> <out.GAM> <cell,cell,...>: marks every cell of the board filled except the given (col*9+row) ones"""
import struct, sys
d = bytearray(open(sys.argv[1], 'rb').read())
keep = {int(x) for x in sys.argv[3].split(',') if x}
for idx in range(81):
    o = 8 + idx * 22 + 5
    f, = struct.unpack_from('<i', d, o)
    if (f & 1) and idx not in keep:
        struct.pack_into('<i', d, o, f | 0x20)
open(sys.argv[2], 'wb').write(d)
