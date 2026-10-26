import struct,sys
exe=open(r"C:/Program Files (x86)/Popcap Game Collection/Water Bugs/WaterBugs.exe","rb").read()
pe=struct.unpack_from("<I",exe,0x3c)[0]
nsec=struct.unpack_from("<H",exe,pe+6)[0]
optsz=struct.unpack_from("<H",exe,pe+20)[0]
base=struct.unpack_from("<I",exe,pe+24+28)[0]
secs=[]
off=pe+24+optsz
for i in range(nsec):
    name,vs,va,rs,ra=struct.unpack_from("<8sIIII",exe,off+40*i)
    secs.append((va+base,vs,ra,rs))
def rd(addr,n):
    for va,vs,ra,rs in secs:
        if va<=addr<va+max(vs,rs): return exe[ra+addr-va:ra+addr-va+n]
for a in sys.argv[1:]:
    a=int(a,16); b=rd(a,64)
    print(hex(a), b.split(b'\0')[0] if b else None, b[:8].hex() if b else '')
