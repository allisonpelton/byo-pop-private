import sys; sys.path.insert(0, 'build/scr')
from gen import gen
# the dialog that chooses the level, with levels still locked: a player who has reached level 12
st=[('shot','menu'),('click',180,306,1500),('shot','resume'),('click',460,372,2000),('shot','c0')]
for i in range(1,4): st+=[('key','RIGHT',0x27,500),('shot',f'c{i}')]
st+=[('key','RIGHT',0x27,300)]*10+[('wait',400),('shot','c13'),('click',521,416,1500),('shot','after')]
port,orig,t=gen(st,1200,'lk_')
open('build/scr/lk_port.txt','w').write("\n".join(port)+f"\n{t+300} quit\n")
open('build/scr/o/lk.txt','w').write("waitshot logo 20000\nwait 6000\n"+"\n".join(orig)+"\nquit\n")
# a new player to the first board: the name panel, the difficulty, the five pages of the story
st=[('shot','menu'),('click',180,306,1500),('shot','name'),('type','te'),('key','RETURN',0x0d,2000),('shot','difficulty'),('click',460,280,3000)]
for i in range(1,4): st+=[("wait",9000),('shot',f'page{i}'),('click',745,565,3000)]
st+=[('wait',4000),('shot','board'),('click',400,300,2500),('shot','tutorial')]
port,orig,t=gen(st,1200,'st_')
open('build/scr/st_port.txt','w').write("\n".join(port)+f"\n{t+300} quit\n")
open('build/scr/o/st.txt','w').write("waitshot logo 20000\nwait 6000\n"+"\n".join(orig)+"\nquit\n")
