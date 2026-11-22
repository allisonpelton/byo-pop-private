import os, shutil, glob
R='build/scr'
orig_src=[f'{R}/shots_tour',f'{R}/shots_action']+[f'build/origrun/bn_{n}/shots' for n in ['ae','lvt','lvb0','lvb1','lvb2','lvb3','lvb4','be0','be1','be2','mw0','mw1','aw0','gx','lk','st','lgrt']]
port_src=[f'{R}/p_tour']+[f'{R}/p_{n}' for n in ['ae','lvt','lvb0','lvb1','lvb2','lvb3','lvb4','be0','be1','be2','mw0','mw1','aw0','gx','lk','st','lgrt']]
def collect(srcs):
    d={}
    for s in srcs:
        for f in sorted(glob.glob(s+'/*.png')):
            d.setdefault(os.path.basename(f),f)
    return d
o=collect(orig_src); p=collect(port_src)
out='build/shots/bonnies-screens'
shutil.rmtree(out,ignore_errors=True)
for k in ('orig','port'): os.makedirs(f'{out}/{k}')
both=sorted(set(o)&set(p))
for n in both:
    shutil.copy(o[n],f'{out}/orig/{n}'); shutil.copy(p[n],f'{out}/port/{n}')
print(len(both),'paired; orig only',sorted(set(o)-set(p))[:60],'port only',sorted(set(p)-set(o))[:60])
