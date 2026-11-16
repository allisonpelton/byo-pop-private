"""The lite mod's offline measurement (games/<game>/docs/LITE_MOD.md): the shuffle's redeal (Deal::Start with theRedeal) on the board of
every escape stage after a quarter, a half and three quarters of the moves that win it, a few clock seeds each, with the original's
search (a second of the machine's clock) or the lite mod's (200 clock checks).

    python -I games/mahjongjapan/tools/profile/stageredeal.py <game> <first stage> <last stage> <seeds> <seconds per stage> [quick]

<game> is mahjongjapan or mahjongchina (their <game>_dealtest.exe is built by cmake and the game's files are in assets/<game>); a stage
whose own solve takes longer than <seconds per stage> is reported as not measured. One "stage N took ..." line a stage, then one
"R <stage> <layout> <moves played> <moves> <seed> <ms> <clock checks> <hash of the kinds dealt>" line a redeal. Run it from the
repository's root, one game process at a time (tools/gamecount.py), under tools/pincore.py when the milliseconds matter.
"""
import os
import re
import subprocess
import sys
import time

game, first, last, seeds, timeout = sys.argv[1], int(sys.argv[2]), int(sys.argv[3]), sys.argv[4], int(sys.argv[5])
exe = os.path.abspath('build/native/%s_dealtest.exe' % game)
folder = os.path.abspath('assets/' + game)
env = dict(os.environ, STAGE_REDEAL=seeds)
if len(sys.argv) > 6 and sys.argv[6] == 'quick':
    env['QUICK_REDEAL'] = '1'
for stage in range(first, last + 1):
    start = time.time()
    try:
        done = subprocess.run([exe, folder, '--stage', str(stage)], capture_output=True, text=True, timeout=timeout, env=env)
    except subprocess.TimeoutExpired:
        print('stage', stage, 'solve timed out after', timeout, 's', flush=True)
        continue
    rows = re.findall(r'^redeal stage layout (\d+): (\d+) of (\d+) moves played, seed (\d+), (\d+) ms, (\d+) clock checks, kinds ([0-9a-f]+)',
                      done.stdout, re.M)
    ms = [int(r[4]) for r in rows]
    print('stage', stage, 'took %.1f s;' % (time.time() - start), len(rows), 'redeals, max', max(ms) if ms else None,
          'ms; at the limit (>=900):', sum(1 for m in ms if m >= 900), flush=True)
    for r in rows:
        print('R', stage, *r, flush=True)
