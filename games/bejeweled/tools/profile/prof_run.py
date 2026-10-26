# A bot game's input logged as a script, then played again from it in real time with memreport lines, under memprof.
# The memory reports come from the game's script command memreport (src/testhooks/MemReport.h): mem00150.txt at the
# title, mem01500.txt and mem02950.txt in the game, next to memprof.json in <out-dir>/prof.
# Run under rundesk from the worktree root: python prof_run.py <out-dir> <mods,...|-> [settings.json|-] [env K=V ...]
import json, os, shutil, subprocess, sys

ROOT = os.getcwd()
HERE = os.path.dirname(os.path.abspath(__file__))
EXE = os.path.join(ROOT, 'build', 'native', 'bejeweled.exe')
MODS = os.path.join(ROOT, 'games', 'bejeweled', 'mods')
ASSETS = os.path.join(ROOT, 'assets', 'bejeweled')
out = os.path.abspath(sys.argv[1])
mods = [m for m in sys.argv[2].split(',') if m and m != '-']
settings = json.load(open(sys.argv[3])) if len(sys.argv) > 3 and sys.argv[3] != '-' else {}
env = sys.argv[4:]
ALL = sorted(d for d in os.listdir(MODS) if os.path.isfile(os.path.join(MODS, d, 'mod.json')))
shutil.rmtree(out, ignore_errors=True)
os.makedirs(os.path.join(out, 'user'))
with open(os.path.join(out, 'user', 'registry.ini'), 'w', newline='\n') as f:
    f.write('; test settings\n\n[SOFTWARE\\Steam\\Bejeweled]\nScreenMode=dword:0\n')
with open(os.path.join(out, 'user', 'mods.json'), 'w', newline='\n') as f:
    json.dump({'order': mods, 'disabled': [m for m in ALL if m not in mods], 'enabled': mods}, f)
with open(os.path.join(out, 'user', 'mod-settings.json'), 'w', newline='\n') as f:
    json.dump({'v': 1, 'mods': settings}, f)
base = [EXE, '-gamedir=' + ASSETS.replace('\\', '/'), '-modsdir=' + MODS.replace('\\', '/'), '-seed=3']
p = subprocess.run(base + ['-userdir=user', '-fast', '-autoplay=3', '-autoplaycap=3000', '-autoplaylog=log.txt'], cwd=out,
                   capture_output=True, timeout=300)
for f in os.listdir(os.path.join(out, 'user')):
    if f.startswith('savegame'):
        os.remove(os.path.join(out, 'user', f))
lines = [l for l in open(os.path.join(out, 'log.txt')).read().splitlines() if l.strip() and l.split()[1:2] != ['quit']]
reports = ['%d memreport mem%05d.txt' % (t, t) for t in (150, 1500, 2950)]


def tick(l):
    try:
        return int(l.split()[0])
    except ValueError:
        return -1


with open(os.path.join(out, 'script.txt'), 'w', newline='\n') as f:
    f.write('\n'.join(sorted(lines + reports, key=tick) + ['3000 quit']) + '\n')
r = subprocess.run([sys.executable, os.path.join(HERE, 'memprof.py'), os.path.join(out, 'prof'), '1.5,15,29'] + env +
                   ['--'] + base + ['-userdir=' + os.path.join(out, 'user').replace('\\', '/'), '-script=' +
                                    os.path.join(out, 'script.txt').replace('\\', '/')], cwd=out, capture_output=True, text=True)
print('bot exit %d; %s' % (p.returncode, r.stdout.strip().splitlines()[0] if r.stdout.strip() else r.stderr[-400:]))
