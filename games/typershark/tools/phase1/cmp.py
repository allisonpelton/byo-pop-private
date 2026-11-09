#!/usr/bin/env python3
"""Phase 1 comparison: the original's state at each update (read from its memory by origrun's `at N peek`) against the port's
-statelog, for the same input at the same updates.

    python cmp.py <scenario.py> [--seed S] [--lo U] [--hi U] [--step K] [--tag NAME] [--keep] [--max N]

A scenario module defines STEPS (the inputs, see tsp.py), LO and HI (the updates to compare, inclusive) and optionally
PORT_EXTRA (more -script lines for the port), FIXTURE (the users folder fixture of the original) and PORT_ARGS.
Two runs of the original: pass 1 reads the creature lists' sizes and each creature's vtable pointer (its class), pass 2 reads
the fields of each creature at its class's offsets (the classes' GameObject and Enemy parts sit at other offsets: RTTI).
Both runs are the same input from the same state, so pass 2 reads the creatures pass 1 listed. The port's run writes the
statelog; the two are compared update by update, field by field. Output goes to build/ts-p1/<tag>.*; the last line printed is
the verdict.
"""
import argparse
import importlib.util
import json
import os
import re
import subprocess
import struct
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from tsp import *            # noqa: E402,F401
from peekparse import parse, dbl   # noqa: E402

ROOT = os.path.normpath(os.path.join(HERE, '..', '..', '..', '..'))
# the port's exe: TSEXE=<absolute path> runs another build (the frozen baseline), so that the sources can be rebuilt meanwhile
PORT_EXE = os.environ.get('TSEXE') or os.path.join(ROOT, 'build', 'native', 'typershark.exe')
OUT = os.path.join(ROOT, 'build', 'ts-p1')

# ---- the class layouts from RTTI: primary vptr -> (name, GameObject part offset, Enemy part offset) ----
_classes = None


def classes():
    """vtable va -> (class name, offset of the GameObject part, offset of the Enemy part), both relative to the address that holds
    that vtable pointer: a Board list holds pointers to the part of the complete object that its element type is (offset 0 for a
    Shark, 536 for a StealthShark), so the offsets are negative where the part lies before it."""
    global _classes
    if _classes is not None:
        return _classes
    d = json.load(open(os.path.join(ROOT, 're', 'typershark', 'rtti.classes.json')))
    out = {}
    for name, c in d.items():
        short = name.split('::')[-1]
        g = e = None
        for v in c['vtables']:
            if len(v['slots']) == 12:
                g = v['offset']
            if v['slots'] and v['slots'][0] == '00403130':
                e = v['offset']
        for v in c['vtables']:
            k = v['offset']
            out[int(v['va'], 16)] = (short, None if g is None else g - k, None if e is None else e - k)
    _classes = out
    return out


def s32(v):
    return None if v is None else struct.unpack('<i', struct.pack('<I', v & 0xffffffff))[0]


def u32(v):
    return None if v is None else v & 0xffffffff


# list slots on the Board: name -> (begin offset, end offset, max elements read)
# a list is (name, address of its begin pointer, address of its end pointer or None for a single slot, most elements read)
LISTS = [('sharks', board(0xd78), board(0xd7c), 40), ('piranhas', board(0xd88), board(0xd8c), 40),
         ('clams', board(0xd98), board(0xd9c), 24), ('bonus', board(0xdc8), board(0xdcc), 24),
         ('boss', board(0xeb8), None, 1),
         ('shots', ind(board(0xeb8), 0xec), ind(board(0xeb8), 0xf0), 12)]


def elem_slot(list_begin, i, single=False):
    """address expression of the slot that holds element i's pointer (list_begin: the address of the list's begin pointer, or a
    Board offset)"""
    if isinstance(list_begin, int):
        list_begin = board(list_begin)
    if single:
        return list_begin
    return f"[{list_begin}]+0x{4 * i:x}"


def slots_of(lst, i):
    return elem_slot(lst[1], i, lst[2] is None)


def count_of(lst, r):
    """how many elements a list holds, from a pass-1 record"""
    name, b, e, kmax = lst
    if e is None:
        return 1 if u32(r.get(b)) else 0
    first, last = u32(r.get(b)), u32(r.get(e))
    if first is None or last is None or last < first:
        return 0
    return min(kmax, (last - first) // 4)


def pass1_exprs():
    ex = [board(0x64)]
    for lst in LISTS:
        ex += [lst[1]] + ([lst[2]] if lst[2] is not None else [])
    for lst in LISTS:
        for i in range(lst[3]):
            ex.append(ind(slots_of(lst, i), 0))      # the creature's primary vptr
    return ex


def scalar_exprs():
    t = ind(board(0xd34), 0)       # the game timer's field: [B+0xd34] is the address of the pointer slot; WSTimer fields below
    return [board(0x64), board(0x1c4), player(0xf4), player(0x110), ind(board(0xd34), 0x4c), ind(board(0xd34), 0x5c),
            ind(board(0xd34), 0x58), ind(board(0xd34), 0x60), board(0xe60), app(0x624), board(0xdf8),
            board(0xde8), board(0xdec), board(0xdf0), board(0xdf4), board(0xd48), board(0xd4c), player(0xb8),
            player(0xf8), player(0xfc), board(0xe00), board(0xeb8), board(0xd78), board(0xd7c), board(0xd88), board(0xd8c),
            board(0xd98), board(0xd9c), board(0xdc8), board(0xdcc), '[0x565384]+0x9c0', board(0x16c), ind(app(0x65c), 0x8c),
            board(0xd6c), board(0xd70), board(0xf38), board(0xf3c), app(0x690), board(0xe94), ind(app(0x65c), 0x9c)]


def creature_exprs(lst, i, g, e):
    slot = slots_of(lst, i)
    ex = [slot]
    for off in (g + 0x98, g + 0x9c, g + 0xa0, g + 0xa4, g + 0xb8, e + 0x38, e + 0x3c, e + 0x40, e + 0x44, e + 0x48, e + 0x58,
               g + 0xe4):
        ex.append(ind(slot, off))
    if lst[0] == 'clams':
        # P1b: the Clam's own fields (kind +0xe8, opened and pulse flags +0xf0/+0xf1 of the Enemy part) and the GameObject's frame and tick
        for off in (e + 0xe8, e + 0xf0, g + 0xd8, g + 0xe8):
            ex.append(ind(slot, off))
    return ex


def class_of(vptr):
    return classes().get(u32(vptr))


# ---- the port's statelog ----
CRE = re.compile(r'(\w+):(.*?)@(-?[\d.]+),(-?[\d.]+),(-?\d+),(-?\d+)(?= |$)')


def parse_port_line(line):
    head, _, tail = line.partition('|')
    h = {}
    for m in re.finditer(r'(\w+)=(\S*)', head):
        h[m.group(1)] = m.group(2)
    creatures = [(m.group(1), m.group(2), float(m.group(3)), float(m.group(4)), int(m.group(5)), int(m.group(6)))
                 for m in CRE.finditer(tail)]
    return h, creatures


def read_port(path):
    out = {}
    for line in open(path, errors='replace'):
        if not line.startswith('u='):
            continue
        h, c = parse_port_line(line)
        out[int(h['u'])] = (h, c)
    return out


# ---- running things ----
def gamecount_wait():
    import time
    for _ in range(120):
        if subprocess.call([sys.executable, os.path.join(ROOT, 'tools', 'gamecount.py'), '--limit', '3'],
                           stdout=subprocess.DEVNULL) == 0:
            return
        time.sleep(10)
    raise SystemExit('no room for a game process')


def run_orig(script_text, tag, seed, fixture, timeout=240):
    timeout = int(os.environ.get('TSORIGTIMEOUT', timeout))      # a busy machine slows the original down
    sp = os.path.join(OUT, tag + '.orig.txt')
    lg = os.path.join(OUT, tag + '.orig.log')
    open(sp, 'w', newline='\n').write(script_text)
    subprocess.call(['bash', os.path.join(HERE, 'runorig.sh'), sp, lg, str(seed), str(timeout), fixture])
    return lg


def run_port(script_text, tag, seed, userfix=None, extra_args=()):
    sp = os.path.join(OUT, tag + '.port.txt')
    sl = os.path.join(OUT, tag + '.port.sl')
    ud = os.path.join(OUT, 'u_' + tag)
    subprocess.call(['rm', '-rf', ud])
    if userfix:
        subprocess.call(['cp', '-r', os.path.join(OUT, 'fixtures', userfix), ud])
    open(sp, 'w', newline='\n').write(script_text)
    gamecount_wait()
    exe = PORT_EXE
    subprocess.call([os.path.join(ROOT, 'build', 'native', 'rundesk.exe'), exe, '-fast', f'-seed={seed}', f'-userdir={ud}',
                     f'-script={sp}', f'-statelog={sl}'] + list(extra_args), stdout=open(os.path.join(OUT, tag + '.port.out'), 'w'),
                    stderr=subprocess.STDOUT)
    return sl


def record_typist(steps, tag, seed, opts, hi, userfix=None):
    """run the port once with its bot (-typist) on top of the setup steps; return the setup steps plus the inputs the bot played
    (as steps in the original's convention: the port's line N is the original's `at N-1`)"""
    rec = os.path.join(OUT, tag + '.typist.txt')
    sp = os.path.join(OUT, tag + '.rec.txt')
    ud = os.path.join(OUT, 'u_' + tag + '_rec')
    subprocess.call(['rm', '-rf', ud])
    if userfix:
        subprocess.call(['cp', '-r', os.path.join(OUT, 'fixtures', userfix), ud])
    open(sp, 'w', newline='\n').write(port_script(steps, [f'{hi + 5} quit']))
    gamecount_wait()
    subprocess.call([os.path.join(ROOT, 'build', 'native', 'rundesk.exe'), PORT_EXE,
                     '-fast', f'-seed={seed}', f'-userdir={ud}', f'-script={sp}', f'-typist={rec};{opts}', f'-statelog={os.path.join(OUT, tag + ".rec.sl")}'],
                    stdout=open(os.path.join(OUT, tag + '.rec.out'), 'w'), stderr=subprocess.STDOUT)
    out = list(steps)
    for line in open(rec):
        f = line.split()
        if len(f) < 3:
            continue
        u = int(f[0]) - PORT_SHIFT
        if f[1] == 'char':
            out.append((u, 'text1', ' ' if f[2] == 'SPACE' else f[2]))
        elif f[1] == 'click':
            out.append((u, 'click', int(f[2]), int(f[3])))
        elif f[1] == 'key':
            out.append((u, 'key', 13 if f[2] == 'RETURN' else int(f[2])))
    return out


def peek_steps(updates, exprs_fn):
    steps = []
    for u in updates:
        steps.append((u, 'peek', exprs_fn(u)))
    return steps


def run_orig_checked(script_text, tag, seed, fixture, tries=3):
    """run the original and parse its peeks; when it never had a board at any peek (its loading thread ran late on a busy machine and
    the script's menu clicks came too soon), run it again, up to `tries` times"""
    d = {}
    for attempt in range(tries):
        d = parse(run_orig(script_text, tag, seed, fixture))
        if any(r.get(board(0x64)) is not None for r in d.values()):
            break
        print(f'{tag}: the original never had a board at a peek (late loading), run {attempt + 1} of {tries}', flush=True)
    return d


def orig_state(steps, lo, hi, step, tag, seed, fixture):
    """two passes; returns {update: dict}"""
    ups = lo if isinstance(lo, list) else list(range(lo, hi + 1, step))
    # pass 1
    p1 = list(steps) + [(u, 'peek', pass1_exprs()) for u in ups]
    d1 = run_orig_checked(orig_script(p1), tag + '.p1', seed, fixture)
    # the class of each creature at each update
    cls = {}
    for u in ups:
        r = d1.get(u)
        if r is None:
            continue
        per = {}
        k = 0
        for lst in LISTS:
            per[lst[0]] = []
        for lst in LISTS:
            for i in range(count_of(lst, r)):
                vp = r.get(ind(slots_of(lst, i), 0))
                per[lst[0]].append(class_of(vp))
        cls[u] = per

    def exprs2(u):
        ex = list(scalar_exprs())
        per = cls.get(u)
        if per:
            for lst in LISTS:
                for i, c in enumerate(per[lst[0]]):
                    if c is None:
                        ex.append(slots_of(lst, i))
                        continue
                    ex += creature_exprs(lst, i, c[1] if c[1] is not None else 0, c[2] if c[2] is not None else 0)
        return ex
    p2 = list(steps) + [(u, 'peek', exprs2(u)) for u in ups]
    d2 = run_orig_checked(orig_script(p2), tag + '.p2', seed, fixture)
    out = {}
    for u in ups:
        r = d2.get(u)
        if r is None:
            continue
        out[u] = render_orig(u, r, cls.get(u), d1.get(u))
    return out


def tutor_exprs():
    T = app(0x72c)       # the address of the app's typing tutor pointer
    return [app(0x72c), ind(ind(T, 0x6c), 0x14), ind(T, 0x23c), ind(T, 0x78), ind(T, 0x7c), ind(T, 0x80), ind(T, 0x84), ind(T, 0x8c),
            ind(T, 0x90), ind(T, 0x94), ind(T, 0x9c), ind(T, 0xa0), ind(T, 0x74), ind(T, 0x254), ind(T, 0x244), ind(T, 0x248),
            ind(T, 0x258), '[0x565384]+0x9c0']


def render_tutor(u, r):
    T = app(0x72c)
    h = {'u': u}
    h['tut'] = 0 if not r.get(app(0x72c)) else 1
    for key, off in (('tst', 0x23c), ('ttask', 0x78), ('terr', 0x7c), ('tclean', 0x80), ('tcorr', 0x84), ('trwpm', 0x8c),
                     ('tawpm', 0x90), ('tsec', 0x94), ('tacc', 0x9c), ('tleft', 0x74), ('tfirst', 0x254), ('tpause', 0x258)):
        h[key] = s32(r.get(ind(T, off)))
    h['tles'] = s32(r.get(ind(ind(T, 0x6c), 0x14)))
    e = r.get(ind(T, 0xa0))
    h['texp'] = None if e is None else e & 0xff
    b, en = u32(r.get(ind(T, 0x244))), u32(r.get(ind(T, 0x248)))
    h['tcre'] = None if b is None or en is None else (en - b) // 4
    if h.get('tst') == 0:
        # before the first key: the original has not set these yet (they hold what was in the memory; the port starts them at 0)
        for k in ('trwpm', 'tawpm', 'tacc'):
            h[k] = None
    h['rng'] = s32(r.get('[0x565384]+0x9c0'))
    return h, []


def orig_state_tutor(steps, lo, hi, step, tag, seed, fixture):
    ups = lo if isinstance(lo, list) else list(range(lo, hi + 1, step))
    p = list(steps) + [(u, 'peek', tutor_exprs()) for u in ups]
    d = {}
    for attempt in range(3):
        d = parse(run_orig(orig_script(p), tag + '.t', seed, fixture))
        if any(r.get(app(0x72c)) for r in d.values()):
            break
        print(f'{tag}: the original never had a typing tutor at a peek (late loading), run {attempt + 1} of 3', flush=True)
    return {u: render_tutor(u, d[u]) for u in ups if u in d}


def double_at(r, expr_lo, expr_hi):
    return dbl(r.get(expr_lo), r.get(expr_hi))


def render_orig(u, r, per, r1):
    h = {}
    h['u'] = u
    h['bupd'] = s32(r.get(board(0x64)))
    h['bscore'] = s32(r.get(board(0x1c4)))
    h['score'] = s32(r.get(player(0xf4)))
    h['lives'] = s32(r.get(player(0x110)))
    tc, ts, tp, tpz = (s32(r.get(ind(board(0xd34), o))) for o in (0x4c, 0x5c, 0x58, 0x60))
    h['tcnt'], h['tstart'], h['tpaused'] = tc, ts, tp
    if tc is not None:
        h['time'] = 0 if (tpz & 0xff) else (tc - ts) - tp
    h['paused'] = None if r.get(board(0xe60)) is None else (r.get(board(0xe60)) & 0xff)
    h['hint'] = None if r.get(app(0x624)) is None else (1 if r.get(app(0x624)) else 0)
    tsz = s32(r.get(board(0xdf8)))
    h['typed'] = tsz
    if tsz is not None and 0 <= tsz <= 15:
        raw = b''.join(struct.pack('<I', (r.get(board(o)) or 0) & 0xffffffff) for o in (0xde8, 0xdec, 0xdf0, 0xdf4))
        h['ty'] = raw[:tsz].decode('latin1')
    else:
        h['ty'] = '?'
    h['dive'] = double_at(r, board(0xd48), board(0xd4c))
    h['dstate'] = s32(r.get(player(0xb8)))
    h['emp'] = double_at(r, player(0xf8), player(0xfc))
    h['rng'] = s32(r.get('[0x565384]+0x9c0'))
    h['lvl'] = s32(r.get(ind(app(0x65c), 0x8c)))
    h['gameover'] = None if r.get(board(0x16c)) is None else (1 if r.get(board(0x16c)) else 0)
    h['diff'] = s32(r.get(app(0x690)))
    h['e94'] = s32(r.get(board(0xe94)))           # the typos counted towards a penalty piranha
    h['cwt'] = s32(r.get(ind(app(0x65c), 0x9c)))     # the lesson's current wave type
    h['pairs'] = s32(r.get(board(0xd70)))
    h['f38'] = double_at(r, board(0xf38), board(0xf3c))
    h['open'] = -1
    if r.get(board(0xd6c)) and per:
        for i in range(len(per.get('clams', []))):
            if u32(r.get(slots_of(LISTS[2], i))) == u32(r.get(board(0xd6c))):
                h['open'] = i
    cre = []
    if per:
        for lst in LISTS:
            for i, c in enumerate(per[lst[0]]):
                if c is None:
                    cre.append(('?', '?', 0.0, 0.0, 0, 0))
                    continue
                short, g, en = c
                ex = creature_exprs(lst, i, g or 0, en or 0)
                v = [r.get(x) for x in ex]
                x = dbl(v[1], v[2])
                y = dbl(v[3], v[4])
                state = s32(v[5])
                size = s32(v[10])
                raw = b''.join(struct.pack('<I', (w or 0) & 0xffffffff) for w in v[6:10])
                word = raw[:size].decode('latin1') if size is not None and 0 <= size <= 15 else '?'
                typed = s32(v[11])
                if lst[0] == 'clams' and len(v) >= 17:
                    ctype, cflags, cframe, ctick = s32(v[13]), v[14], s32(v[15]), s32(v[16])
                    if None not in (ctype, cflags, cframe, ctick):
                        typed = (ctype * 100000 + cframe * 1000 + (100 if cflags & 0xff else 0) + (10 if (cflags >> 8) & 0xff else 0)
                                 + ctick % 4)
                cre.append((short, word, x, y, state, typed))
    return h, cre


def fmt(v):
    if isinstance(v, float):
        return f'{v:.6f}'
    return str(v)


def close(a, b, tol=1e-6):
    return abs(a - b) <= tol


def compare(o, p, keys, tol=1e-6):
    """differences between an original record and a port record at one update: list of strings"""
    diffs = []
    ho, co = o
    hp, cp = p
    for k in keys:
        if k not in ho or ho[k] is None or k not in hp:
            continue
        a, b = ho[k], hp[k]
        if isinstance(a, float):
            if not close(a, float(b), tol):
                diffs.append(f'{k}: orig {a:.6f} port {b}')
        elif str(a) != str(b):
            diffs.append(f'{k}: orig {a} port {b}')
    if len(co) != len(cp):
        diffs.append(f'creature count: orig {len(co)} port {len(cp)}')
    for i in range(min(len(co), len(cp))):
        a, b = co[i], cp[i]
        if a[0] != b[0]:
            diffs.append(f'#{i} kind: orig {a[0]} port {b[0]}')
        if a[1] != b[1]:
            diffs.append(f'#{i} word: orig {a[1]} port {b[1]}')
        for j, nm in ((2, 'x'), (3, 'y')):
            if a[j] is None or not close(a[j], b[j], tol):
                diffs.append(f'#{i} {nm}: orig {a[j]} port {b[j]}')
        if a[4] != b[4]:
            diffs.append(f'#{i} state: orig {a[4]} port {b[4]}')
        if a[5] != b[5]:
            diffs.append(f'#{i} typedcount: orig {a[5]} port {b[5]}')
    return diffs


TUTOR_KEYS = ['tut', 'tles', 'tst', 'ttask', 'terr', 'tclean', 'tcorr', 'trwpm', 'tawpm', 'tsec', 'tacc', 'texp', 'tleft', 'tfirst',
              'tcre', 'tpause', 'rng']
KEYS = ['bupd', 'score', 'bscore', 'lives', 'time', 'tcnt', 'tstart', 'tpaused', 'paused', 'hint', 'typed', 'ty', 'dive',
        'dstate', 'emp', 'rng', 'lvl', 'pairs', 'f38', 'open', 'diff', 'e94', 'cwt']


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('scenario')
    ap.add_argument('--seed', type=int, default=1)
    ap.add_argument('--lo', type=int)
    ap.add_argument('--hi', type=int)
    ap.add_argument('--step', type=int, default=1)
    ap.add_argument('--ranges', help='lo:hi:step[,lo:hi:step...] instead of --lo/--hi/--step (a fine window and coarse ones around it)')
    ap.add_argument('--tag')
    ap.add_argument('--max', type=int, default=12, help='differing updates to print')
    ap.add_argument('--keys', default=','.join(KEYS))
    ap.add_argument('--rec-only', action='store_true', help='only record the typist run and summarize it (summ.py)')
    ap.add_argument('--skip-orig', action='store_true', help='reuse the last original state (tag.orig.json)')
    a = ap.parse_args()
    spec = importlib.util.spec_from_file_location('scen', a.scenario)
    sc = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(sc)
    lo = a.lo if a.lo is not None else sc.LO
    hi = a.hi if a.hi is not None else sc.HI
    tag = a.tag or os.path.splitext(os.path.basename(a.scenario))[0] + f'_s{a.seed}'
    os.makedirs(OUT, exist_ok=True)
    fixture = getattr(sc, 'FIXTURE', 'users-clean')
    steps = sc.STEPS
    if getattr(sc, 'TYPIST', None) is not None:
        steps = record_typist(sc.STEPS, tag, a.seed, sc.TYPIST, hi, getattr(sc, 'PORT_USERFIX', 'port-clean'))
        open(os.path.join(OUT, tag + '.steps.json'), 'w').write(json.dumps(steps))
        if a.rec_only:
            subprocess.call([sys.executable, os.path.join(HERE, 'summ.py'), os.path.join(OUT, tag + '.rec.sl')])
            return
    ranges = a.ranges or os.environ.get('RANGES')
    win = getattr(sc, 'WINDOW', None)
    if win and not ranges and getattr(sc, 'TYPIST', None) is not None:
        # P1b: a fine window (step 1) around the updates where the lesson level is one of win['lvl'], found in the port's own log of
        # the recorded run, and a coarse step (win['coarse']) over lo..hi
        levels = set(win['lvl'])
        first = last = None
        for line in open(os.path.join(OUT, tag + '.rec.sl'), errors='replace'):
            m = re.match(r'u=(\d+) lvl=(-?\d+) ', line)
            if m and int(m.group(2)) in levels:
                last = int(m.group(1))
                if first is None:
                    first = last
        parts = []
        if first is not None:
            parts.append(f"{max(lo, first - win.get('pad', 200))}:{min(hi, last + win.get('pad', 200))}:1")
            print(f'{tag}: fine window {parts[0]} (levels {sorted(levels)})', flush=True)
        else:
            print(f'{tag}: levels {sorted(levels)} never played in the recorded run', flush=True)
        if win.get('coarse'):
            parts.append(f"{lo}:{hi}:{win['coarse']}")
        ranges = ','.join(parts) if parts else None
    updates = None
    if ranges:
        ups = set()
        for part in ranges.split(','):
            r_lo, r_hi, r_st = (int(x) for x in part.split(':'))
            ups.update(range(r_lo, r_hi + 1, r_st))
        updates = sorted(ups)
        lo, hi = updates[0], updates[-1]
    cache = os.path.join(OUT, tag + '.orig.json')
    if a.skip_orig and os.path.exists(cache):
        raw = json.load(open(cache))
        orig = {int(u): (h, [tuple(c) for c in cr]) for u, (h, cr) in raw.items()}
    else:
        if getattr(sc, 'TUTOR', False):
            orig = orig_state_tutor(steps, updates or lo, hi, a.step, tag, a.seed, fixture)
        else:
            orig = orig_state(steps, updates or lo, hi, a.step, tag, a.seed, fixture)
        json.dump({str(u): v for u, v in orig.items()}, open(cache, 'w'))
    sl = run_port(port_script(steps, [f'{hi + 5} quit'] + list(getattr(sc, 'PORT_EXTRA', []))), tag, a.seed,
                  getattr(sc, 'PORT_USERFIX', 'port-clean'), getattr(sc, 'PORT_ARGS', []))
    port = read_port(sl)
    keys = a.keys.split(',')
    if getattr(sc, 'TUTOR', False):
        keys = TUTOR_KEYS
    n_cmp = n_bad = 0
    shown = 0
    missing = 0
    first_bad = None
    for u in sorted(orig):
        if u not in port:
            missing += 1
            continue
        n_cmp += 1
        ukeys = keys
        if getattr(sc, 'TUTOR', False) and orig[u][0].get('tst') != 2:
            # the words per minute and the accuracy are set when the lesson ends; until then the original holds what was in the memory
            ukeys = [k for k in keys if k not in ('trwpm', 'tawpm', 'tacc')]
        d = compare(orig[u], port[u], ukeys)
        if d:
            n_bad += 1
            if first_bad is None:
                first_bad = u
            if shown < a.max:
                shown += 1
                print(f'u={u}: ' + '; '.join(d[:8]))
    note = ''
    last = max(orig) if orig else None
    if last is not None and last in port and orig[last][0].get('rng') == 624 and port[last][0].get('rng') != 624:
        # 624 is the generator before its first number: the original had not finished loading when the script's first input came
        # (its loading thread runs on the wall clock, so a busy machine makes it late): not a game difference, run it again
        note = ' (THE ORIGINAL NEVER LOADED THE MENU: it was late, run again)'
    print(f'{tag}: compared {n_cmp} updates ({lo}..{hi}), {n_bad} differ, {missing} missing in the port'
          + (f', first difference at u={first_bad}' if first_bad is not None else ', all equal') + note)


if __name__ == '__main__':
    main()
