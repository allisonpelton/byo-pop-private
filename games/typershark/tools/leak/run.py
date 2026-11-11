"""run.py <exe> <outdir> <tag> <seed> <script> [extra args...]: one port run under rundesk (docs/LEAKS.md), a fresh copy of the port-clean users
fixture (build/ts-p1/fixtures/port-clean) unless RUN_USERDIR names a user directory to use as it is. Waits until gamecount.py has room."""
import sys, os, subprocess, shutil, time
ROOT = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))
exe, outdir, tag, seed, script = sys.argv[1:6]
extra = sys.argv[6:]
os.makedirs(outdir, exist_ok=True)
ud = os.environ.get('RUN_USERDIR')
if not ud:
    ud = os.path.join(outdir, 'u_' + tag)
    shutil.rmtree(ud, ignore_errors=True)
    shutil.copytree(os.path.join(ROOT, 'build', 'ts-p1', 'fixtures', 'port-clean'), ud)
for _ in range(7200):
    if subprocess.call([sys.executable, os.path.join(ROOT, 'tools', 'gamecount.py'), '--limit', '6'], stdout=subprocess.DEVNULL) == 0:
        break
    time.sleep(1)
else:
    raise SystemExit('no room')
sl = os.path.join(outdir, tag + '.sl')
t0 = time.time()
rc = subprocess.call([os.path.join(ROOT, 'build', 'native', 'rundesk.exe'), exe, '-fast', f'-seed={seed}', f'-userdir={ud}',
                      f'-gamedir={os.path.join(ROOT, "assets", "typershark")}', f'-script={script}', f'-statelog={sl}'] + extra,
                     stdout=open(os.path.join(outdir, tag + '.out'), 'w'), stderr=subprocess.STDOUT, cwd=outdir)
print(tag, 'exit', rc, 'secs %.1f' % (time.time() - t0))
