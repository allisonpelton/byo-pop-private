#!/usr/bin/env python3
"""Identical-play pass, phase one: run the original (through origrun) and the port on the same scenario and compare the
game's state, update by update.

    python games/atomica/tools/identical/compare.py <scenario.txt> [--seed N] [--end N] [--names a,b,...]

The scenario has one command per line in the original's terms, "<update> <command> <args>": move x y, click x y,
down x y, up x y (window coordinates), and "end <update>". The original runs it as "at <update> ..."; the port gets the
same lines one update later (its script runs at the start of an update, the original's input between two updates).
Values come from the original's memory by origrun --watch (four at a time, so one run per group of four names) and
from the port's -statetrace file (src/testhooks/Trace.cpp); WATCH below lists each name's address in the original (the app
object is static at 0x4cfc60). A pointer is compared as set or not. Both sides are compared up to the scenario's end.
"""
import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', '..', '..'))
BASH = os.environ.get('BASH_EXE') or 'C:/Program Files/Git/usr/bin/bash.exe'
REGKEY = r'HKLM\SOFTWARE\WOW6432Node\PopCap\Atomica'
SAVEFILE = None
BOARD = '[0x4d00f4]'
GRID = '[[0x4d00f4]+0x78]'

WATCH = {
    'board_p': '0x4d00f4', 'menu_p': '0x4d00f8', 'title_p': '0x4d00fc',
    'mode': '0x4d3b04', 'pause': '0x4d3cc8', 'rng_j': '0x4d3a08', 'rng_k': '0x4d3a0c',
    'score': BOARD + '+0x1bc', 'level': BOARD + '+0xc8', 'molecules': BOARD + '+0xcc',
    'meter_value': BOARD + '+0xd0', 'meter_max': BOARD + '+0xd8', 'meter_tick': BOARD + '+0xdc',
    'meter_countdown': BOARD + '+0xe0', 'meter_over': BOARD + '+0xe8', 'f8c': BOARD + '+0x8c',
    'progress': BOARD + '+0x130', 'numcolors_start': BOARD + '+0x13c', 'stat_time': BOARD + '+0x1c4',
    'stat_atoms': BOARD + '+0x1cc', 'stat_molecules': BOARD + '+0x1d0', 'stat_combo': BOARD + '+0x1d4',
    'grid_p': BOARD + '+0x78', 'swirls': BOARD + '+0x114', 'i90': BOARD + '+0x90',
    'combo': GRID + '+0x330', 'numcolors': GRID + '+0x328', 'g_overtick': GRID + '+0x338', 'rounds': GRID + '+0x3b0',
    'bonus_atoms': GRID + '+0x364', 'selected_p': GRID + '+0x1d0',
}
for c in range(9):
    for r in range(8):
        WATCH['cell_%d_%d' % (c, r)] = '[%s+0x%x]+0x8c' % (GRID, 0xb0 + 4 * (c * 8 + r))
for t, base in enumerate((0x4d3b10, 0x4d3bec)):
    # the high score tables (app+0x3eb0, +0x3f8c): scores, the first four letters of each name, the best of the day
    for i in range(5):
        WATCH['hs%d_s%d' % (t, i)] = '0x%x' % (base + 8 + 4 * i)
        WATCH['hs%d_n%d' % (t, i)] = '0x%x' % (base + 0x1c + 30 * i)
    WATCH['hs%d_best' % t] = '0x%x' % (base + 0xb4)
POINTERS = {n for n in WATCH if n.endswith('_p')}
SCORE_NAMES = ['hs%d_%s%d' % (t, k, i) for t in range(2) for k in 'sn' for i in range(5)] + ['hs0_best', 'hs1_best']
DEFAULT_GROUPS = [
    ['board_p', 'menu_p', 'title_p', 'rng_j'],
    ['score', 'level', 'molecules', 'combo'],
    ['rng_k', 'selected_p', 'g_overtick', 'numcolors'],
    ['meter_value', 'meter_tick', 'meter_countdown', 'meter_over'],
    ['f8c', 'progress', 'stat_atoms', 'stat_molecules'],
    ['mode', 'pause', 'grid_p', 'bonus_atoms'],
    ['meter_max', 'numcolors_start', 'stat_time', 'rounds'],
    ['stat_combo'],
]


def fwd(path):
    return path.replace(chr(92), '/')


def s32(v):
    v &= 0xffffffff
    return v - (1 << 32) if v & 0x80000000 else v


def norm(name, v):
    if v == '-':
        return '-'
    v = int(v)
    if name in POINTERS:
        return '1' if (v & 0xffffffff) else '0'
    return str(s32(v))


def read_scenario(path):
    lines = []
    end = None
    for l in open(path):
        l = l.split('#')[0].strip()
        if not l:
            continue
        w = l.split()
        if w[0] == 'end':
            end = int(w[1])
        else:
            lines.append((int(w[0]), w[1], w[2:]))
    return lines, end


def run_port(lines, end, seed):
    d = tempfile.mkdtemp(prefix='atomport')
    script = os.path.join(d, 's.txt')
    with open(script, 'w', newline='\n') as f:
        for n, c, a in lines:
            if c == 'text':
                # the original types a string in one command, the port one character at a time
                for ch in ' '.join(a):
                    f.write('%d char %s\n' % (n + 1, 'SPACE' if ch == ' ' else ch))
            else:
                f.write('%d %s %s\n' % (n + 1, c, ' '.join(a)))
        f.write('%d quit\n' % (end + 1))
    trace = os.path.join(d, 'trace.txt')
    ud = os.path.join(d, 'user')
    if SAVEFILE:
        # a saved game for both sides to continue (savecmp.py leaves the original's)
        os.makedirs(ud)
        shutil.copy(SAVEFILE, os.path.join(ud, 'SAVEGAME.DAT'))
    subprocess.run([BASH, fwd(os.path.join(ROOT, 'games/atomica/tools/run_script.sh')), fwd(script), fwd(ud), '600',
                    '-seed=%d' % seed, '-drawevery', '-statetrace=' + fwd(trace)], cwd=ROOT, stdout=subprocess.DEVNULL,
                   stderr=subprocess.DEVNULL)
    ev = {}
    for l in open(trace):
        u, n, v = l.split()
        ev.setdefault(n, []).append((max(int(u), 1), norm(n, v)))
    shutil.rmtree(d, ignore_errors=True)
    return ev


def run_original(lines, end, seed, names, tag):
    d = tempfile.mkdtemp(prefix='atomorig')
    script = os.path.join(d, 's.txt')
    with open(script, 'w', newline='\n') as f:
        for n, c, a in lines:
            f.write('at %d %s %s\n' % (n, c, ' '.join(a)))
        f.write('at %d move 1 1\nquit\n' % end)
    run = 'cmp' + tag
    # a fresh game: the original keeps the hints shown and the settings in the registry (the port's are in its user folder);
    # the user allowed the originals to use their registry (CLAUDE.md)
    for value in ('GameHints', 'ShowSymbols', 'ShowTips'):
        subprocess.run(['reg', 'delete', REGKEY, '/v', value, '/f'], capture_output=True)
    if SAVEFILE:
        # the staged copy savecmp.py made, with the saved game put back in it
        run = 'savecmp'
        shutil.copy(SAVEFILE, os.path.join('C:/byo-og', run, 'game', 'SAVEGAME.DAT'))
    else:
        shutil.rmtree(os.path.join('C:/byo-og', run), ignore_errors=True)
        shutil.rmtree(os.path.join(ROOT, 'build/origrun', run), ignore_errors=True)
    cmd = [os.path.join(ROOT, 'build/origrun-bin/origrun.exe'), '--game', 'atomica', '--run', run, '--stage-root',
           'C:/byo-og', '--script', script, '--clock', 'virtual', '--cursor', 'scripted', '--seed', str(seed),
           '--timeout', '900']
    for n in names:
        cmd += ['--watch', WATCH[n]]
    p = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    ev = {n: [] for n in names}
    for l in p.stdout.splitlines():
        m = re.search(r'watch (\d+): (\S+) -> (\S+) at update (\d+)', l)
        if m:
            k = int(m.group(1)) - 1
            ev[names[k]].append((int(m.group(4)), norm(names[k], m.group(3))))
    shutil.rmtree(d, ignore_errors=True)
    return ev


def series(events, end):
    out = {}
    for u, v in events:
        if 0 < u <= end:
            out[u] = v
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('scenario')
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--end', type=int)
    ap.add_argument('--names')
    ap.add_argument('--savefile', help='a SAVEGAME.DAT both sides start with (see savecmp.py)')
    ap.add_argument('--scores', action='store_true', help='also compare the high score tables')
    ap.add_argument('--limit', type=int, default=6)
    ap.add_argument('--dump', type=int, default=0, help='print the first N changes of each name on both sides')
    a = ap.parse_args()
    lines, end = read_scenario(a.scenario)
    end = a.end or end
    global SAVEFILE
    SAVEFILE = os.path.abspath(a.savefile) if a.savefile else None
    groups = DEFAULT_GROUPS
    if a.names:
        names = a.names.split(',')
        groups = [names[i:i + 4] for i in range(0, len(names), 4)]
    if a.scores and not a.names:
        groups = groups + [SCORE_NAMES[i:i + 4] for i in range(0, len(SCORE_NAMES), 4)]
    port = run_port(lines, end, a.seed)
    bad = 0
    for gi, g in enumerate(groups):
        orig = run_original(lines, end, a.seed, g, str(gi))
        for n in g:
            o = series(orig[n], end)
            p = series(port.get(n, []), end)
            vo = vp = '0'
            diffs = []
            for u in sorted(set(o) | set(p)):
                vo = o.get(u, vo)
                vp = p.get(u, vp)
                if vo != vp:
                    diffs.append((u, vo, vp))
            if a.dump:
                print('%s orig: %s' % (n, ' '.join('%d:%s' % e for e in orig[n][:a.dump])))
                print('%s port: %s' % (n, ' '.join('%d:%s' % e for e in port.get(n, [])[:a.dump])))
            if diffs:
                bad += 1
                print('DIFF %s: %d update(s) differ; first %s' % (
                    n, len(diffs), ', '.join('u%d orig=%s port=%s' % d for d in diffs[:a.limit])))
            else:
                print('same %s (%d changes)' % (n, len(o)))
    print('compared %d names up to update %d: %d differ' % (sum(len(g) for g in groups), end, bad))
    sys.exit(1 if bad else 0)


main()
