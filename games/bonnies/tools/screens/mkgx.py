import os, shutil, sys
sys.path.insert(0, 'build/scr')
from gen import gen
d = 'build/scr/gx'
shutil.rmtree(d, ignore_errors=True); os.makedirs(d + '/SavedGames')
src = {'ga': 'build/scr/g1a/user/SavedGames/te.GAM', 'gb': 'build/scr/sv_g2/SavedGames/te.GAM', 'gc': 'build/scr/g1d/user/SavedGames/te.GAM'}
for n, f in src.items(): shutil.copy(f, f'{d}/SavedGames/{n}.GAM')
p = bytearray(open('build/scr/be/L04/save/SavedGames/General_Preferences.cfg','rb').read()); p[8:10] = b'ga'
open(f'{d}/SavedGames/General_Preferences.cfg','wb').write(p)
rows = [182, 219, 256]
st = [('shot','menu')]
for i, n in enumerate(['ga','gb','gc']):
    if i > 0: st += [('click',670,170,1500),('click',400,rows[i],600),('click',335,442,1500)]
    st += [('click',180,306,1500),('shot',f'{n}_resume'),('click',460,330,3500),('shot',f'{n}_load')]
    if n == 'gb':
        st += [('click',20,300,200),('click',701,368,200),('click',701,414,200),('click',747,391,300),('shot','gb_word'),('click',452,560,2500)]
    else:
        st += [('click',695,548,1500),('shot',f'{n}_resign'),('click',393,413,2500)]
    st += [('shot',f'{n}_over'),('click',440,412,2500)]
    if n != 'gc': st += [('shot',f'{n}_scores'),('click',590,48,2000),('shot',f'{n}_scores2'),('click',590,48,2000),('click',400,300,2500)]
    st += [('shot',f'{n}_menu')]
port, orig, t = gen(st, 1200, 'gx_')
open('build/scr/gx_port.txt','w').write("\n".join(port)+f"\n{t+500} quit\n")
open('build/scr/o/gx.txt','w').write("waitshot logo 20000\nwait 6000\n"+"\n".join(orig)+"\nquit\n")
