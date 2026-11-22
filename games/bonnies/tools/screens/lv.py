import sys; sys.path.insert(0, 'build/scr')
from gen import gen
def out(name, steps, prefix='l_'):
    port, orig, t = gen(steps, 1200, prefix)
    open(f'build/scr/{name}_port.txt','w').write("\n".join(port)+"\n"+f"{t+500} quit\n")
    open(f'build/scr/o/{name}.txt','w').write("waitshot logo 20000\nwait 6000\n"+"\n".join(orig)+"\nquit\n")
# thumbnails
st=[('shot','menu'),('click',180,306),('shot','resume'),('click',460,372),('shot','choose0')]
st+=[('key','LEFT',0x25,100)]*60+[('wait',500),('shot','t01')]
for L in range(2,51): st+=[('key','RIGHT',0x27,300),('shot',f't{L:02d}')]
st+=[('click',521,416,1500),('shot','aftercancel')]
out('lvt', st)
# starts: chunks of 10 levels
for k in range(5):
    st=[('shot','menu')]
    for L in range(2+10*k, min(52,12+10*k)):
        if L>50: break
        st+=[('click',180,306,1000),('click',460,372 if (L==2+10*k and k==0) else 372,1000)] if False else [('click',180,306,1000)]
        # the first iteration meets the resume dialog (a game in progress), later ones the dialog at once
        if L==2+10*k: st+=[('click',460,372,1000)]
        st+=[('key','LEFT',0x25,200)]*(50-L)+[('wait',400),('click',397,416,1200),('click',460,280,2500),('shot',f'b{L:02d}'),('key','ESCAPE',0x1b,1500),('click',335,337,2500)]
    out(f'lvb{k}', st)
