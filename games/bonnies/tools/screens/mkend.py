"""mkend.py L...: for each level L, a saved game one word from winning level L (every cell filled but the best word's)"""
import json, os, re, shutil, subprocess, sys
sys.path.insert(0, 'build/scr')
from gen import gen
R = os.getcwd()
def run(script, out, saves):
    env = dict(os.environ, SAVES=saves)
    subprocess.run(['bash', 'build/scr/run_port.sh', script, out], env=env, check=True, capture_output=True)
res = {}
ACT = os.environ.get('MODE') == 'action'
PFX = 'A' if ACT else 'L'
MENU = (205,360) if ACT else (180,306)
for L in map(int, sys.argv[1:]):
    d = f'build/scr/be/{PFX}{L:02d}'
    shutil.rmtree(d, ignore_errors=True); os.makedirs(d)
    st = [('click',MENU[0],MENU[1],1000),('click',460,372,1000)] + [('key','LEFT',0x25,60)]*60 + [('key','RIGHT',0x27,60)]*(L-1) + [('wait',300),('click',397,416,1000),('click',460,280,2500)]
    port, _, t = gen(st, 1200, '')
    open(f'{d}/a.txt','w').write("\n".join(port)+f"\n{t+300} set score 1000\n{t+400} state\n{t+600} quit\n")
    run(f'{d}/a.txt', f'{d}/a', 'build/scr/sv50')
    a_state = [l for l in open(f'{d}/a/out.log') if l.startswith('STATE')][0]
    assert f'"level":{L},' in a_state, (L, a_state[:200])
    shutil.copytree(f'{d}/a/user', f'{d}/sa')
    open(f'{d}/b.txt','w').write("1200 click 180 306\n1700 click 460 330\n2500 move\n2700 quit\n")
    run(f'{d}/b.txt', f'{d}/b', f'{d}/sa')
    mv = [l for l in open(f'{d}/b/out.log') if l.startswith('MOVE')][0]
    m = json.loads(mv[5:])
    assert m['kind'] == 'word', (L, mv[:200])
    os.makedirs(f'{d}/save/SavedGames')
    for f in os.listdir(f'{d}/sa/SavedGames'): shutil.copy(f'{d}/sa/SavedGames/{f}', f'{d}/save/SavedGames/{f}')
    subprocess.run([sys.executable, 'build/scr/patchsave.py', f'{d}/sa/SavedGames/te.GAM', f'{d}/save/SavedGames/te.GAM', ','.join(map(str, m['cells']))], check=True)
    res[L] = {'word': m['word'], 'clicks': [c['click'] for c in m['input']], 'state': a_state[:160]}
    print(L, m['word'], flush=True)
json.dump(res, open('build/scr/be/moves_%s%s.json' % ('A' if ACT else '', '_'.join(sys.argv[1:])), 'w'))
