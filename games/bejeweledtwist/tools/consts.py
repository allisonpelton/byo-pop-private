#!/usr/bin/env python3
"""Print the float constants a function of BejeweledTwist.exe reads (its `qword/dword ptr [0x7.....]` operands) with
their values: many are floats widened to doubles (0.1 is 0.10000000149011612), and the port writes them as the exe has
them.

    python games/bejeweledtwist/tools/consts.py 69cd1e [more addresses]
"""
import struct,subprocess,sys,re
d=open('assets/bejeweledtwist/bejeweledtwist.exe','rb').read()
pe=struct.unpack_from('<I',d,0x3c)[0]
n=struct.unpack_from('<H',d,pe+6)[0]; so=pe+24+struct.unpack_from('<H',d,pe+20)[0]
def rd(va,f):
    for i in range(n):
        vs,va0,rs,ro=struct.unpack_from('<IIII',d,so+i*40+8)
        if va0<=va-0x400000<va0+max(vs,rs): return struct.unpack_from(f,d,ro+va-0x400000-va0)[0]
for fn in sys.argv[1:]:
    asm=subprocess.run(['python','games/bejeweledtwist/tools/asm.py','0x'+fn],capture_output=True,text=True).stdout
    seen={}
    for m in re.finditer(r'(qword|dword) ptr \[(0x[78][0-9a-f]{5})\]',asm):
        a=int(m.group(2),16)
        if a in seen: continue
        seen[a]=rd(a,'<d' if m.group(1)=='qword' else '<f')
    print(fn, ' '.join('%x=%r'%(a,v) for a,v in seen.items() if a<0x830000))
