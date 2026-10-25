import sys,re
names=[l.split('=')[0].strip().replace('IMAGE_','') for l in open('games/noahsark/src/Res.h') if l.strip().startswith('IMAGE_') and '=' in l and 'NUM_IMAGES' not in l]
for a in sys.argv[1:]:
    v=int(a,16) if a.startswith('0x') else int(a)
    print(a, v//4, names[v//4])
