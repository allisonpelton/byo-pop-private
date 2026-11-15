"""cyc.py's measurement with the game's window on the user's visible desktop, for what a hidden desktop cannot give either game:
the original's 3D mode, and a display that signals its vertical blank (GAME_NOTES.md section 11). Only in the hours that CLAUDE.md
allows ("Overnight, desktop and sound allowed": 22:00 to 08:00 local time; a run started in that window ends by 08:00).

    python -I tools/pincore.py -- <python.exe> -I games/typershark/tools/profile/cyc3d.py port3d <out dir> [scenario] [port args]
    python -I tools/pincore.py -- <python.exe> -I games/typershark/tools/profile/cyc3d.py orig3d <out dir> [scenario]

Both games run windowed at 640x480 (the port with -renderscale=1, silent through SDL's dummy audio driver; the original with
origrun's silent stand-in as its dsound.dll and Is3D set to 1), their window shown without activation and kept topmost so that
nothing of the user's covers it, and nothing of the user's is touched: the input is posted to the game's own window. The port plays the
scenario from its -script; the original is driven as tools/phase2/visrun3d.py drives it (build/ts-3d/game, registry staged and put
back afterwards, the cursor through origrun's shared block), but its input is posted while it runs (no hold of the process) and the
driver sleeps between its checks of the counter, so that the driver does not keep a logical processor of the core busy.
"""

import json
import os
import re
import shutil
import subprocess
import sys
import time
import uuid

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, '..', 'phase2'))
import cyc  # the meter
import perf
import visrun3d as vr

k32 = perf.k32
SAMPLE = cyc.SAMPLE_EVERY


def overnight():
    h = time.localtime().tm_hour
    return h >= 22 or h < 8


def stretch_bounds(end):
    return end - perf.STRETCH, end


def port3d(out, src, extra):
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    lines, end = perf.scenario(src)
    sc = os.path.join(out, 'script.txt')
    with open(sc, 'w', newline='\n') as f:
        for n, c, a in lines:
            f.write('%d %s %s\n' % (n, c, ' '.join(a)))
        f.write('%d quit\n' % (end + 150))
    exe, gamedir = perf.port_exe()
    ud = os.path.join(out, 'ud')
    perf.fresh_users(ud)
    sym, imagebase = cyc.exe_symbol(exe, '__ZN4Sexy12gSexyAppBaseE')
    others0 = perf.game_processes(perf.mine_prefixes())
    s0 = perf.system_times()
    env = dict(os.environ, SDL_AUDIODRIVER='dummy', ORACLE_VISIBLE='1')   # ORACLE_VISIBLE: the port's guard lets a test flag run off rundesk (overnight only)
    si = subprocess.STARTUPINFO()
    si.dwFlags = 1
    si.wShowWindow = 4                      # SW_SHOWNOACTIVATE
    log = open(os.path.join(out, 'game.log'), 'wb')
    p = subprocess.Popen([exe, '-gamedir=' + gamedir, '-userdir=' + ud, '-seed=1', '-script=' + sc, '-renderscale=1'] + extra, cwd=out,
                         env=env, stdout=log, stderr=subprocess.STDOUT, startupinfo=si)
    h = k32.OpenProcess(perf.QUERY | perf.VM_READ | perf.QUERY_LIMITED, False, p.pid)
    created = perf.creation_time(h)
    state = {'addr': None, 'topmost': False}

    def counter():
        if state['addr'] is None:
            mods = cyc.modules(h)
            m = next((m for m in mods if m[2].lower() == os.path.basename(exe).lower()), None)
            if m is None or sym is None:
                return None
            state['addr'] = m[0] + (sym - imagebase if sym >= imagebase else sym)
        app = perf.read32(h, state['addr'])
        return perf.read32(h, app + cyc.APP_COUNTER_OFFSET) if app else None

    meter = cyc.Meter(h, p.pid, created, counter)
    while p.poll() is None:
        meter.take()
        if not state['topmost']:
            hw = find_window(p.pid)
            if hw:
                vr.u32.SetWindowPos(hw, vr.wt.HWND(-1), 0, 0, 0, 0, 0x0001 | 0x0002 | 0x0010)   # topmost, not activated
                state['topmost'] = True
        time.sleep(SAMPLE)
        if len(meter.samples) % 50 == 0:
            meter.label_threads()
    s1 = perf.system_times()
    meter.label_threads()
    rep = meter.report(*stretch_bounds(end))
    res = {'side': 'port3d', 'renderer': 'SDL, Direct3D 11, windowed 640x480 on the visible desktop, vsync', 'exit': p.returncode,
           'machine_busy': round(perf.busy_fraction(s0, s1), 2), 'others_start': len(others0),
           'others_end': len(perf.game_processes(perf.mine_prefixes()))}
    if rep:
        res.update(rep)
        res['updates_per_s'] = round(rep['updates'] / rep['window_s'], 1)
    json.dump({'samples': meter.samples, 'labels': meter.labels, 'result': res}, open(os.path.join(out, 'cyc.json'), 'w'))
    k32.CloseHandle(h)
    print(json.dumps(res), flush=True)


def find_window(pid):
    found = []

    @vr.ctypes.WINFUNCTYPE(vr.wt.BOOL, vr.wt.HWND, vr.wt.LPARAM)
    def cb(hw, lp):
        wp = vr.wt.DWORD()
        vr.u32.GetWindowThreadProcessId(hw, vr.ctypes.byref(wp))
        r = vr.wt.RECT()
        vr.u32.GetClientRect(hw, vr.ctypes.byref(r))
        if wp.value == pid and vr.u32.IsWindowVisible(hw) and r.right >= 640 and r.bottom >= 480:
            found.append(hw)
        return True
    vr.u32.EnumWindows(cb, 0)
    return found[0] if found else None


def orig_steps(src):
    """The scenario as (counter to wait for, command, args): the port's line N is the original's update N - 1 (tools/phase2/p2lib.py)."""
    lines, end = perf.scenario(src)
    steps = []
    for n, c, a in lines:
        if c == 'char':
            steps.append((n - 1, 'key', ['32']) if a[0] == 'SPACE' else (n - 1, 'text', [a[0]]))
        else:
            steps.append((n - 1, c, a))
    return steps, end


def orig3d(out, src):
    out = os.path.abspath(out)
    shutil.rmtree(out, ignore_errors=True)
    os.makedirs(out)
    steps, end = orig_steps(src)
    backup = os.path.join(vr.ROOT, 'build', 'ts-3d', 'registry-backup')
    vr.prepare(True)
    if os.environ.get('STANDIN'):          # a stand-in other than origrun's own (what its module rescan costs, GAME_NOTES.md section 11)
        shutil.copy(os.environ['STANDIN'], os.path.join(vr.GAME, 'dsound.dll'))
    vr.stage_registry(backup, True)
    run_id = uuid.uuid4().hex[:12]
    cur = vr.Cursor(run_id)
    env = dict(os.environ)
    env.update({'ORIGRUN_DIR': os.path.join(vr.ROOT, 'build', 'ts-3d', 'logs'), 'ORIGRUN_SOUND': 'stub', 'ORIGRUN_REGMAP': 'popcap',
                'ORIGRUN_CURSOR': 'scripted', 'ORIGRUN_CURSOR_ID': run_id})
    os.makedirs(env['ORIGRUN_DIR'], exist_ok=True)
    others0 = perf.game_processes(perf.mine_prefixes())
    s0 = perf.system_times()
    g = None
    res = {'side': 'orig3d', 'renderer': 'DirectDraw 7 / Direct3D 7 (3D flag set), windowed 640x480 on the visible desktop'}
    try:
        g = vr.Game(os.path.join(vr.GAME, 'TyperSharkGame.exe'), env)
        t0 = time.time()
        while not g.window() and time.time() - t0 < 90 and g.p.poll() is None:
            time.sleep(0.05)
        if not g.window():
            res['error'] = 'no window'
            return
        vr.post_input(g, cur, 'focus', ['regain'])
        hp = k32.OpenProcess(perf.QUERY | perf.VM_READ | perf.QUERY_LIMITED, False, g.p.pid)
        created = perf.creation_time(hp)
        meter = cyc.Meter(hp, g.p.pid, created, g.counter)
        next_take = 0
        lo, hi = stretch_bounds(end - 1)
        states, state_at = {}, [lo, hi]

        def state():
            # the board's state (phase 2's peeks: the board, its +0x64 and +0x1c4, two fields of the sharks' list): two runs played the same game
            b = g.rd(g.app() + 0x634) if g.app() else None
            d = g.rd(b + 0xd68) if b else None
            return [g.rd(b + 0x64), g.rd(b + 0x1c4), g.rd(d + 0xf4), g.rd(d + 0x110)] if b and d else None

        def look(c):
            while state_at and c is not None and c >= state_at[0]:
                states[state_at.pop(0)] = state()

        for n, cmd, args in steps:
            while True:
                c = g.counter()
                look(c)
                if time.time() >= next_take:
                    meter.take()
                    next_take = time.time() + SAMPLE
                if c is not None and c >= n:
                    break
                if g.p.poll() is not None:
                    res['error'] = 'the game ended'
                    return
                time.sleep(0.002)
            vr.post_input(g, cur, cmd, args)
        while True:                                     # on past the stretch's end
            c = g.counter()
            look(c)
            if time.time() >= next_take:
                meter.take()
                next_take = time.time() + SAMPLE
            if c is not None and c >= hi + 100 or g.p.poll() is not None:
                break
            time.sleep(0.01)
        meter.label_threads()
        rep = meter.report(lo, hi)
        s1 = perf.system_times()
        res.update({'exit': g.p.poll(), 'machine_busy': round(perf.busy_fraction(s0, s1), 2), 'others_start': len(others0),
                    'others_end': len(perf.game_processes(perf.mine_prefixes())), 'flag3d': g.flag3d(),
                    'state_at_stretch_start_and_end': [states.get(lo), states.get(hi)]})
        if rep:
            res.update(rep)
            res['updates_per_s'] = round(rep['updates'] / rep['window_s'], 1)
        json.dump({'samples': meter.samples, 'labels': meter.labels, 'result': res}, open(os.path.join(out, 'cyc.json'), 'w'))
    finally:
        if g is not None and g.p.poll() is None:
            hw = g.window()
            if hw:
                vr.u32.PostMessageW(hw, 0x0010, 0, 0)
            try:
                g.p.wait(20)
            except subprocess.TimeoutExpired:
                g.p.kill()
                g.p.wait(10)
        time.sleep(0.5)
        vr.restore_registry(backup)
        print(json.dumps(res), flush=True)


if __name__ == '__main__':
    mode = sys.argv[1] if len(sys.argv) > 1 else ''
    scn = sys.argv[3] if len(sys.argv) > 3 and not sys.argv[3].startswith('-') else perf.DEFAULT_SCENARIO
    if mode not in ('port3d', 'orig3d'):
        sys.exit(__doc__)
    if not overnight():
        sys.exit('cyc3d: the visible desktop only between 22:00 and 08:00 (CLAUDE.md)')
    if mode == 'port3d':
        port3d(sys.argv[2], scn, [a for a in sys.argv[3:] if a.startswith('-')])
    else:
        orig3d(sys.argv[2], scn)
