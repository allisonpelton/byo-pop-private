import sys; sys.path.insert(0, 'build/scr')
from gen import gen
steps = [('click',670,170),('shot','players'),('click',335,392),('shot','newname'),('type','te'),('key','RETURN',0x0d),('shot','exists'),
 ('key','RETURN',0x0d),('shot','exists2'),('key','ESCAPE',0x1b),('shot','afteresc'),('click',400,182,500),('click',462,392),('shot','delete'),
 ('click',521,416),('shot','nodelete'),('click',462,442,1000)]
port, orig, t = gen(steps, 61600, 'z_')
open('build/scr/tail_port.txt','w').write("# the player list, a new player's name (taken and free) and deleting a player\n"+"\n".join(port)+"\n")
open('build/scr/tail_orig.txt','w').write("# the same steps on the original, after the time's up message and the main menu\n"+"\n".join(orig)+"\n")
print(t)
