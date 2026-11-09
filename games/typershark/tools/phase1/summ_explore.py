import sys
sys.path.insert(0,'games/typershark/tools/phase1')
from peekparse import *
from tsp import *
d=parse(sys.argv[1])
prev=None
n=0
for u in sorted(d):
    r=d[u]
    cur=(r.get(board(0xd78)),r.get(board(0xd7c)),r.get(board(0xd88)),r.get(board(0xd8c)), r.get(app(0x624)))
    sz=lambda a,b:(None if a is None or b is None else (b-a)//4)
    key=(sz(cur[0],cur[1]),sz(cur[2],cur[3]),cur[4]!=0)
    if key!=prev:
        print(u,r.get(board(0x64)),key,'score',r.get(board(0x1c4)), 'sharks first',hex(cur[0] or 0))
        prev=key
        n+=1
        if n>int(sys.argv[2]): break
