# sbs.py <case> <run> <shot> [x y w h scale]: side by side (original left, port right)
import sys
from PIL import Image
case,run,n=sys.argv[1:4]
a=Image.open('build/origrun/%s/shots/%s.png'%(run,n)).convert('RGB').crop((3,26,643,506)); b=Image.open('%s/shots-port/%s.png'%(case,n)).convert('RGB')
if len(sys.argv)>4:
    x,y,w,h,s=map(int,sys.argv[4:9]); a=a.crop((x,y,x+w,y+h)); b=b.crop((x,y,x+w,y+h))
    a=a.resize((w*s,h*s),Image.NEAREST); b=b.resize((w*s,h*s),Image.NEAREST)
c=Image.new('RGB',(a.width*2+4,a.height)); c.paste(a,(0,0)); c.paste(b,(a.width+4,0)); c.save('build/sc/sbs.png')
