import struct,sys
d=open('re/alchemy/bin/AlchemyGame.exe','rb').read()
pe=struct.unpack_from('<I',d,0x3c)[0]
nsec=struct.unpack_from('<H',d,pe+6)[0]
opt=struct.unpack_from('<H',d,pe+20)[0]
sec=pe+24+opt
secs=[]
for i in range(nsec):
    vs,va,rs,ro=struct.unpack_from('<IIII',d,sec+i*40+8)
    secs.append((va+0x400000,vs,ro,rs))
def rd(va,n):
    for a,vs,ro,rs in secs:
        if a<=va<a+max(vs,rs): return d[ro+va-a:ro+va-a+n]
def cstr(va): return rd(va,100).split(b'\0')[0].decode('latin1')
def table(base,n):
    return [cstr(struct.unpack('<I',rd(base+4*i,4))[0]) for i in range(n)]

def vt(base, n):
    out = []
    for i in range(n):
        a = struct.unpack('<I', rd(base + 4 * i, 4))[0]
        if 0x401000 <= a < 0x415000:
            out.append('%d:%x' % (i, a))
    return ' '.join(out)
