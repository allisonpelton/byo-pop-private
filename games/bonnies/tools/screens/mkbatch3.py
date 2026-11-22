"""action games won: chapter 5 (mid-book) and 20 (a book's last)"""
import json, os, shutil, sys
sys.path.insert(0, 'build/scr')
from gen import gen
mv = {int(k): v for k, v in json.load(open('build/scr/be/moves_A5_20.json')).items()}
d = 'build/scr/be/ab0'
shutil.rmtree(d, ignore_errors=True); os.makedirs(d + '/SavedGames')
names = {5: 'aa', 20: 'ab'}
for L, n in names.items(): shutil.copy(f'build/scr/be/A{L:02d}/save/SavedGames/te.GAM', f'{d}/SavedGames/{n}.GAM')
p = bytearray(open('build/scr/be/L04/save/SavedGames/General_Preferences.cfg','rb').read()); p[8:10] = b'aa'
open(f'{d}/SavedGames/General_Preferences.cfg','wb').write(p)
st = [('shot','menu')]
for i, (L, n) in enumerate(names.items()):
    if i > 0: st += [('click',670,170,1500),('click',400,219,600),('click',335,442,1500)]
    st += [('click',205,360,1500),('shot',f'resume{L:02d}'),('click',460,330,3000),('shot',f'load{L:02d}'),('click',20,300,300)]
    for x, y in mv[L]['clicks'][1:-1]: st.append(('click',x,y,300))
    st.append(('click',506,532,300))
    st += [('wait',6000),('shot',f'end{L:02d}'),('click',745,565,4500),('shot',f'letter{L:02d}')]
    if L == 20: st += [('click',745,565,13000),('shot',f'story{L:02d}'),('click',745,565,5000),('shot',f'next{L:02d}')]
    else: st += [('wait',13000),('shot',f'next{L:02d}')]
    st += [('key','ESCAPE',0x1b,1500),('click',70,548,1500),('shot',f'menu{L:02d}'),('click',335,337,2500)]
port, orig, t = gen(st, 1200, 'aw_')
open('build/scr/aw0_port.txt','w').write("\n".join(port)+f"\n{t+500} quit\n")
open('build/scr/o/aw0.txt','w').write("waitshot logo 20000\nwait 6000\n"+"\n".join(orig)+"\nquit\n")
