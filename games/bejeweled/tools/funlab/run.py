# The fun lab's runner (games/bejeweled/docs/GAME_MODES.md): plays each candidate mode of a modes file with
# bejeweled_verify funlab and keeps the games' lines in <out>/<label>.txt, then prints report.py's summary.
#
#   python games/bejeweled/tools/funlab/run.py <modes.json> [--seeds 1..20] [--only a,b] [--jobs 2] [--out build/funlab]
#       [--wait 100] [--mode both] [--policy both]
#
# A modes file maps a label to the mods it plays, lowest priority first (an id under games/bejeweled/mods, or a path to a
# mod.json), and their settings, as the game's mod-settings.json holds them:
#   {"original": {"mods": []},
#    "sparks": {"mods": ["example-power-gems"], "settings": {"example-power-gems": {"radius": 1}}}}
# A label's "args" (a list) go to funlab as they are. Run it from the repository root, under build/native/rundesk.exe
# for its low priority; the verifier opens no window and plays no sound.
import argparse, json, os, subprocess, sys, time
from concurrent.futures import ThreadPoolExecutor

ROOT = os.getcwd()
VERIFY = os.path.join(ROOT, 'build', 'native', 'bejeweled_verify.exe')
MODS = os.path.join(ROOT, 'games', 'bejeweled', 'mods')


def mod_path(m):
    return m if m.endswith('.json') else os.path.join(MODS, m, 'mod.json')


def run(label, mode, a):
    out = os.path.join(a.out, label + '.txt')
    settings = os.path.join(a.out, label + '.settings.json')
    with open(settings, 'w', newline='\n') as f:
        json.dump({'v': 1, 'mods': mode.get('settings', {})}, f)
    cmd = [VERIFY, 'funlab'] + [mod_path(m) for m in mode.get('mods', [])] + ['--settings', settings, '--seeds', a.seeds,
           '--wait', str(a.wait), '--mode', a.mode, '--policy', a.policy] + mode.get('args', [])
    start = time.time()
    with open(out, 'wb') as f:
        p = subprocess.run(cmd, stdout=f, stderr=subprocess.PIPE)
    err = p.stderr.decode('utf-8', 'replace').strip()
    print('%-24s exit %d in %.0f s%s' % (label, p.returncode, time.time() - start, (': ' + err[:300]) if err else ''))
    sys.stdout.flush()
    return p.returncode


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('modes')
    ap.add_argument('--seeds', default='1..20')
    ap.add_argument('--only', default='')
    ap.add_argument('--jobs', type=int, default=2)
    ap.add_argument('--out', default=os.path.join('build', 'funlab'))
    ap.add_argument('--wait', type=int, default=100)
    ap.add_argument('--mode', default='both')
    ap.add_argument('--policy', default='both')
    a = ap.parse_args()
    a.jobs = max(1, min(3, a.jobs))  # at most 3 game processes at a time (CLAUDE.md)
    os.makedirs(a.out, exist_ok=True)
    modes = json.load(open(a.modes, encoding='utf-8'))
    only = [x for x in a.only.split(',') if x]
    labels = [k for k in modes if not k.startswith('_') and (not only or k in only)]
    with ThreadPoolExecutor(a.jobs) as pool:
        codes = list(pool.map(lambda k: run(k, modes[k], a), labels))
    report = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'report.py')
    subprocess.run([sys.executable, report] + [os.path.join(a.out, k + '.txt') for k in labels])
    return 1 if any(codes) else 0


sys.exit(main())
