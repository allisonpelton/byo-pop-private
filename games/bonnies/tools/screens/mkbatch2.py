"""mid-book chapter ends: two batches, three players each"""
import json, os, shutil, sys
sys.path.insert(0, 'build/scr')
from gen import gen
mv = {int(k): v for k, v in json.load(open('build/scr/be/moves_3_7_14_23_34_43.json')).items()}
batches = [[3,7,14],[23,34,43]]
rows = [182, 219, 256]
for bi, lv in enumerate(batches):
    d = f'build/scr/be/mb{bi}'
    shutil.rmtree(d, ignore_errors=True); os.makedirs(d + '/SavedGames')
    names = ['ma','mb','mc'][:len(lv)]
    for n, L in zip(names, lv): shutil.copy(f'build/scr/be/L{L:02d}/save/SavedGames/te.GAM', f'{d}/SavedGames/{n}.GAM')
    p = bytearray(open('build/scr/be/L04/save/SavedGames/General_Preferences.cfg','rb').read()); p[8:10] = names[0].encode()
    open(f'{d}/SavedGames/General_Preferences.cfg','wb').write(p)
    st = [('shot','menu')]
    for i, (n, L) in enumerate(zip(names, lv)):
        if i > 0: st += [('click',670,170,1500),('click',400,rows[i],600),('click',335,442,1500)]
        st += [('click',180,306,1500),('shot',f'resume{L:02d}'),('click',460,330,3000),('shot',f'load{L:02d}')]
        for x, y in mv[L]['clicks']: st.append(('click',x,y,300))
        st += [('wait',6000),('shot',f'end{L:02d}'),('click',745,565,13000),('shot',f'next{L:02d}'),('click',745,565,5000),('shot',f'board{L:02d}'),('key','ESCAPE',0x1b,1500),('click',70,548,1500),('shot',f'menu{L:02d}'),('click',335,337,2500)]
    port, orig, t = gen(st, 1200, 'mw_')
    open(f'build/scr/mw{bi}_port.txt','w').write("\n".join(port)+f"\n{t+500} quit\n")
    open(f'build/scr/o/mw{bi}.txt','w').write("waitshot logo 20000\nwait 6000\n"+"\n".join(orig)+"\nquit\n")
