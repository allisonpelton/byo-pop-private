"""visrun: one PopCap original on the user's visible desktop (for its 3D mode, which no sandbox or rundesk gives it), driven by
a step file, with shots of the game's own window. docs/VISRUN.md has the rules and the file formats.

    python -I tools/visrun.py CONFIG.toml STEPS.txt --out DIR [--3d on|off] [--seed N] [--user-window-until HH:MM]
    python -I tools/visrun.py CONFIG.toml STEPS.txt --dry-run        (parses both and prints the plan; starts nothing)
    python -I tools/visrun.py CONFIG.toml --restore                  (puts back what an interrupted run left)
    python -I tools/visrun.py --places                               (the places on the right half of the screen, who holds them)
    python -I tools/visrun.py --selftest                             (checks the time, expression, place, lock and window helpers)

The run gets a place of its own on the right half of the primary screen (up to four at once, machine-wide locks under
%LOCALAPPDATA%\\byo-pop\\visrun\\), waits for it if all are taken, and starts only when no other window has been on that place
for --quiet seconds. A config's pos is ignored: the place decides. docs/VISRUN.md has the details.

A run may start at any hour (the user lifted the time window on 2026-11-15). It stops at the latest 14 hours after it starts,
a safety stop for a run that hangs, or earlier at the HH:MM given with --user-window-until. The game is staged by tools/origrun (--copy-only), its stand-in is placed, the game runs with
no activation, silent, at below-normal priority, and only its own window is touched: input is posted to it, never through
the user's cursor or keyboard. The window, the game's registry key and its save folders are put back at the end.
"""
import argparse
import ctypes
import ctypes.wintypes as wt
import datetime
import io
import json
import mmap
import os
import re
import shutil
import struct
import subprocess
import sys
import threading
import time
import tomllib
import uuid

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))
ORIGRUN_BIN = os.path.join(ROOT, 'build', 'origrun-bin')
WORK = os.path.join(ROOT, 'build', 'visrun')
WRAPPER_FILES = ('ddraw.dll', 'd3dimm.dll', 'dgvoodoo.conf', 'origrun-wrapper.txt', 'dsound.dll')
MAX_RUN_HOURS = 14   # the longest a run may last: its deadline is its start plus this


# ---------------------------------------------------------------- time guard
def parse_hhmm(text):
    m = re.fullmatch(r'([01]?\d|2[0-3]):([0-5]\d)', text)
    if not m:
        raise SystemExit('visrun: %r is not a time as HH:MM' % text)
    return int(m.group(1)), int(m.group(2))


def next_at(now, h, m):
    t = now.replace(hour=h, minute=m, second=0, microsecond=0)
    return t if t > now else t + datetime.timedelta(days=1)


def run_deadline(now, user_until=None):
    """The moment a run must be over: MAX_RUN_HOURS after it starts, or, with --user-window-until, the next HH:MM when that is
    earlier (an HH:MM more than MAX_RUN_HOURS away is refused)."""
    limit = now + datetime.timedelta(hours=MAX_RUN_HOURS)
    if user_until:
        t = next_at(now, *user_until)
        if t > limit:
            raise SystemExit('visrun: --user-window-until %02d:%02d is more than %d hours away' % (user_until + (MAX_RUN_HOURS,)))
        return min(t, limit)
    return limit


# ---------------------------------------------------------------- expressions and conditions (origrun's memory grammar)
def parse_expr(s):
    """"0xA", "[0xA]+0xB", "[[0xA]+0xB]+0xC": a sum of numbers and [dereferenced sums]. Returns a tree."""
    pos = [0]

    def total():
        terms = [term()]
        while pos[0] < len(s) and s[pos[0]] == '+':
            pos[0] += 1
            terms.append(term())
        return ('sum', terms)

    def term():
        if pos[0] < len(s) and s[pos[0]] == '[':
            pos[0] += 1
            inner = total()
            if pos[0] >= len(s) or s[pos[0]] != ']':
                raise ValueError('missing ] in %r' % s)
            pos[0] += 1
            return ('deref', inner)
        m = re.compile(r'0x[0-9a-fA-F]+|\d+').match(s, pos[0])
        if not m:
            raise ValueError('bad expression %r at %d' % (s, pos[0]))
        pos[0] = m.end()
        return ('num', int(m.group(0), 0))
    t = total()
    if pos[0] != len(s):
        raise ValueError('bad expression %r at %d' % (s, pos[0]))
    return t


def eval_expr(t, rd):
    """The address the tree names, reading 4 bytes through rd(addr, size) at each [..]; None where unreadable."""
    if t[0] == 'num':
        return t[1]
    if t[0] == 'sum':
        vals = [eval_expr(x, rd) for x in t[1]]
        return None if None in vals else sum(vals) & 0xffffffff
    inner = eval_expr(t[1], rd)
    return rd(inner, 4) if inner else None


class Cond:
    """"<expr>[:1|:4] <op> <value>[,<value>...]", op one of == != < <= > >= in; the value read signed."""
    PAT = re.compile(r'^\s*(\S+?)(?::([14]))?\s+(==|!=|<=|>=|<|>|in)\s+(\S+)\s*$')

    def __init__(self, text):
        m = self.PAT.match(text)
        if not m:
            raise ValueError('bad condition %r (want "<expr>[:size] <op> <value>")' % text)
        self.text, self.tree, self.size, self.op = text, parse_expr(m.group(1)), int(m.group(2) or 4), m.group(3)
        self.values = [int(v, 0) for v in m.group(4).split(',')]

    def read(self, rd):
        a = eval_expr(self.tree, rd)
        v = rd(a, self.size) if a else None
        if v is not None and self.size == 4 and v >= 1 << 31:
            v -= 1 << 32
        return v

    def holds(self, rd):
        v = self.read(rd)
        if v is None:
            return False
        w = self.values
        return {'==': v == w[0], '!=': v != w[0], '<': v < w[0], '<=': v <= w[0], '>': v > w[0], '>=': v >= w[0],
                'in': v in w}[self.op]


# ---------------------------------------------------------------- config and steps
MOUSE_CMDS = ('move', 'click', 'rclick', 'dclick', 'down', 'up', 'rdown', 'rup')
CLICK_CMDS = ('click', 'rclick', 'dclick', 'down', 'up', 'rdown', 'rup')   # a press or release: refused inside a forbidden region
INPUT_CMDS = MOUSE_CMDS + ('key', 'text', 'focus')
ACTION_CMDS = INPUT_CMDS + ('shot', 'burst', 'peek')


def load_config(path):
    with open(path, 'rb') as f:
        c = tomllib.load(f)
    for key in ('name', 'counter'):
        if key not in c:
            raise SystemExit('visrun: %s has no "%s"' % (path, key))
    if not (c.get('game') or c.get('source_exe')):
        raise SystemExit('visrun: %s needs "game" (a games.tsv name) or "source_exe"' % path)
    c.setdefault('clock', 'real')
    c.setdefault('client', [640, 480])
    c.setdefault('update_ms', 10)
    c.setdefault('cursor', 'scripted')
    c.setdefault('cues', {})
    c.setdefault('forbid', [])
    c.setdefault('drawn', [])
    c.setdefault('save_dirs', [])
    c.setdefault('reg_values', {})
    c.setdefault('watch', [])
    if c['clock'] not in ('real', 'virtual'):
        raise SystemExit('visrun: clock must be "real" or "virtual"')
    parse_expr(c['counter'])
    for cue in list(c['cues'].values()) + list(c['drawn']):
        Cond(cue)
    for f in c['forbid']:
        if len(f.get('rect', [])) != 4 or 'name' not in f:
            raise SystemExit('visrun: a forbid entry needs name and rect = [x, y, w, h]')
    return c


class Step:
    def __init__(self, line, no, cmd, args, at=None, rel=False, t=None):
        self.line, self.no, self.cmd, self.args, self.at, self.rel, self.t = line, no, cmd, args, at, rel, t

    def __repr__(self):
        when = '' if self.at is None else ('at %s%d ' % ('+' if self.rel else '', self.at))
        when = when or ('t %g ' % self.t if self.t is not None else '')
        return '%s%s %s' % (when, self.cmd, ' '.join(self.args))


def parse_steps(path, cfg):
    """Returns (steps, forbidden) where forbidden is the config's regions and the file's, as (name, x, y, w, h)."""
    forbidden = [(f['name'], *f['rect']) for f in cfg['forbid']]
    steps = []
    for no, raw in enumerate(open(path, encoding='utf-8'), 1):
        w = raw.split('#', 1)[0].split()
        if not w:
            continue

        def bad(why):
            raise SystemExit('visrun: %s line %d: %s: %s' % (path, no, why, raw.strip()))
        head = w[0]
        at, rel, t = None, False, None
        if head == 'at':
            if len(w) < 3:
                bad('at needs a number and a command')
            rel = w[1].startswith('+')
            at, w = int(w[1].lstrip('+')), w[2:]
        elif head == 't':
            if len(w) < 3:
                bad('t needs seconds and a command')
            t, w = float(w[1]), w[2:]
        cmd, args = w[0], w[1:]
        if cmd == 'forbid':
            if len(args) != 5:
                bad('forbid <name> <x> <y> <w> <h>')
            forbidden.append((args[0], *map(int, args[1:])))
            continue
        if cmd == 'waitshot':   # origrun's wait for the window: visrun always waits for it
            continue
        known = ACTION_CMDS + ('quit', 'wait', 'waitfor', 'waitcue', 'gate', 'mark')
        if cmd not in known:
            bad('unknown command %r (origrun\'s hw*, record and count have no visrun form)' % cmd)
        if (at is not None or t is not None) and cmd not in ACTION_CMDS:
            bad('%s cannot have a time' % cmd)
        try:
            if cmd in ('waitcue', 'gate') and args[0] not in cfg['cues']:
                bad('%r is not a cue of the config' % args[0])
            if cmd == 'waitfor':
                Cond(' '.join(args[:3]))
            if cmd in MOUSE_CMDS and (cmd != 'up' or args) and len(args) < 2 and not (cmd in ('up', 'rup') and not args):
                bad('%s needs x y' % cmd)
            if cmd == 'shot' and len(args) != 1:
                bad('shot <name>')
            if cmd == 'burst' and not (len(args) == 4 and args[2] == 'update'):
                bad('burst <name> <count> update <k>')
            if cmd == 'key':
                int(args[0], 0)
        except (IndexError, ValueError) as e:
            bad('bad arguments (%s)' % e)
        steps.append(Step(raw.strip(), no, cmd, args, at, rel, t))
    return steps, forbidden


def forbidden_hit(forbidden, cmd, args):
    """The region a press, release or click of the step falls in, or None."""
    if cmd not in CLICK_CMDS or len(args) < 2:
        return None
    x, y = int(args[0]), int(args[1])
    for name, rx, ry, rw, rh in forbidden:
        if rx <= x < rx + rw and ry <= y < ry + rh:
            return name
    return None


def check_forbidden(steps, forbidden):
    for s in steps:
        hit = forbidden_hit(forbidden, s.cmd, s.args)
        if hit:
            raise SystemExit('visrun: line %d clicks in the forbidden region %r (a link, Buy or More Games button): %s' % (s.no, hit, s.line))


def plan(cfg, steps, forbidden, deadline):
    out = ['game %s, clock %s, counter %s, client %dx%d' % (cfg['name'], cfg['clock'], cfg['counter'], *cfg['client'])]
    out.append('run must end by %s' % deadline.strftime('%Y-%m-%d %H:%M'))
    for f in forbidden:
        out.append('forbidden region %s: x %d y %d w %d h %d' % f)
    for s in steps:
        out.append('%4d  %r' % (s.no, s))
    out.append('%d steps, %d shots' % (len(steps), sum(1 for s in steps if s.cmd in ('shot', 'burst'))))
    return '\n'.join(out)


# ---------------------------------------------------------------- Win32
k32 = ctypes.WinDLL('kernel32', use_last_error=True)
u32 = ctypes.WinDLL('user32', use_last_error=True)
g32 = ctypes.WinDLL('gdi32', use_last_error=True)
ntdll = ctypes.WinDLL('ntdll')
k32.OpenProcess.restype = wt.HANDLE
k32.OpenFileMappingW.restype = wt.HANDLE
k32.OpenFileMappingW.argtypes = [wt.DWORD, wt.BOOL, wt.LPCWSTR]
k32.MapViewOfFile.restype = ctypes.c_void_p
k32.MapViewOfFile.argtypes = [wt.HANDLE, wt.DWORD, wt.DWORD, wt.DWORD, ctypes.c_size_t]
k32.OpenEventW.restype = wt.HANDLE
k32.OpenEventW.argtypes = [wt.DWORD, wt.BOOL, wt.LPCWSTR]
k32.SetEvent.argtypes = [wt.HANDLE]
k32.WaitForSingleObject.argtypes = [wt.HANDLE, wt.DWORD]
k32.CreateJobObjectW.restype = wt.HANDLE
k32.CreateJobObjectW.argtypes = [ctypes.c_void_p, wt.LPCWSTR]
k32.SetInformationJobObject.argtypes = [wt.HANDLE, ctypes.c_int, ctypes.c_void_p, wt.DWORD]
k32.AssignProcessToJobObject.argtypes = [wt.HANDLE, wt.HANDLE]
u32.GetDC.restype = wt.HDC
g32.CreateCompatibleDC.restype = wt.HDC
g32.CreateCompatibleBitmap.restype = wt.HBITMAP
g32.SelectObject.restype = wt.HGDIOBJ
g32.CreateCompatibleDC.argtypes = [wt.HDC]
g32.CreateCompatibleBitmap.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]
g32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
g32.GetDIBits.argtypes = [wt.HDC, wt.HBITMAP, ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p, ctypes.c_uint]
g32.DeleteObject.argtypes = [wt.HGDIOBJ]
g32.DeleteDC.argtypes = [wt.HDC]
u32.ReleaseDC.argtypes = [wt.HWND, wt.HDC]
u32.PostMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
u32.PrintWindow.argtypes = [wt.HWND, wt.HDC, ctypes.c_uint]
u32.GetWindow.restype = wt.HWND
u32.GetWindow.argtypes = [wt.HWND, ctypes.c_uint]
u32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]
u32.GetForegroundWindow.restype = wt.HWND
u32.GetClassNameW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
u32.GetWindowTextW.argtypes = [wt.HWND, wt.LPWSTR, ctypes.c_int]
u32.GetWindowLongW.argtypes = [wt.HWND, ctypes.c_int]
u32.GetWindowThreadProcessId.argtypes = [wt.HWND, ctypes.POINTER(wt.DWORD)]

WM_MOUSEMOVE, WM_LBUTTONDOWN, WM_LBUTTONUP, WM_LBUTTONDBLCLK, WM_RBUTTONDOWN, WM_RBUTTONUP = 0x200, 0x201, 0x202, 0x203, 0x204, 0x205


def window_title(hw):
    buf = ctypes.create_unicode_buffer(256)
    u32.GetWindowTextW(hw, buf, 256)
    return buf.value


def pick_window(candidates, client, title=''):
    """Of (hwnd, client_w, client_h, title) of one process's visible windows, the largest that is at least the expected client size
    (and holds the title text, if one is configured). Pure, so it is checked without a game."""
    ok = [c for c in candidates if c[1] >= client[0] and c[2] >= client[1] and title.lower() in c[3].lower()]
    return max(ok, key=lambda c: c[1] * c[2])[0] if ok else None


def client_rect(hw):
    r = wt.RECT()
    u32.GetClientRect(hw, ctypes.byref(r))
    pt = wt.POINT(0, 0)
    u32.ClientToScreen(hw, ctypes.byref(pt))
    return pt.x, pt.y, r.right, r.bottom


def rects_overlap(a, b):
    """Two rectangles (left, top, right, bottom), both with area, share some area."""
    return a[0] < b[2] and a[2] > b[0] and a[1] < b[3] and a[3] > b[1] and a[2] > a[0] and a[3] > a[1] and b[2] > b[0] and b[3] > b[1]


def overlapped(hw):
    """The class of a visible window above the game's in the z-order that covers part of its client area, or None."""
    x, y, w, h = client_rect(hw)
    o = u32.GetWindow(hw, 3)   # GW_HWNDPREV
    while o:
        if u32.IsWindowVisible(o) and not u32.IsIconic(o):
            r = wt.RECT()
            u32.GetWindowRect(o, ctypes.byref(r))
            if rects_overlap((r.left, r.top, r.right, r.bottom), (x, y, x + w, y + h)):
                buf = ctypes.create_unicode_buffer(256)
                u32.GetClassNameW(o, buf, 256)
                return buf.value
        o = u32.GetWindow(o, 3)
    return None


def short_sleep(seconds):
    """A wait of about half a millisecond that costs no processor time: a high-resolution waitable timer."""
    if not short_sleep.timer:
        short_sleep.timer = [k32.CreateWaitableTimerExW(None, None, 0x2, 0x1F0003)]
    k32.SetWaitableTimer.argtypes = [wt.HANDLE, ctypes.POINTER(ctypes.c_int64), wt.LONG, ctypes.c_void_p, ctypes.c_void_p, wt.BOOL]
    if not short_sleep.timer[0] or not k32.SetWaitableTimer(short_sleep.timer[0], ctypes.byref(ctypes.c_int64(-int(seconds * 1e7))), 0, None, None, False):
        time.sleep(seconds)
        return
    k32.WaitForSingleObject(short_sleep.timer[0], 100)


short_sleep.timer = None


class Game:
    def __init__(self, exe, env, cfg):
        si = subprocess.STARTUPINFO()
        si.dwFlags = 1            # STARTF_USESHOWWINDOW: the window's first show takes SW_SHOWNOACTIVATE, so it never takes the focus
        si.wShowWindow = 4
        if cfg.get('_place'):     # STARTF_USEPOSITION: a game that lets Windows choose its first position then starts in its own place
            si.dwFlags |= 4
            si.dwX, si.dwY = cfg['_place']
        self.cfg = cfg
        self.p = subprocess.Popen([exe] + [str(x) for x in cfg.get('args', [])], cwd=os.path.dirname(exe), env=env, startupinfo=si, creationflags=0x4000)  # BELOW_NORMAL
        self.h = k32.OpenProcess(0x0010 | 0x0400 | 0x0800 | 0x0001 | 0x0100, False, self.p.pid)   # VM_READ, QUERY_INFO, SUSPEND_RESUME, TERMINATE
        self.hwnd = None
        self.held = False
        # a job that ends the game with this driver, if the driver itself dies (the registry then waits for --restore)
        self.job = k32.CreateJobObjectW(None, None)
        info = (ctypes.c_byte * 144)()   # JOBOBJECT_EXTENDED_LIMIT_INFORMATION (64-bit): LimitFlags at offset 16
        struct.pack_into('<I', info, 16, 0x2000)   # JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE
        k32.SetInformationJobObject(self.job, 9, ctypes.byref(info), 144)
        k32.AssignProcessToJobObject(self.job, self.h)

    def rd(self, addr, size=4):
        v = ctypes.c_uint32()
        n = ctypes.c_size_t()
        if not addr or not k32.ReadProcessMemory(self.h, ctypes.c_void_p(addr), ctypes.byref(v), size, ctypes.byref(n)):
            return None
        return v.value & (0xff if size == 1 else 0xffffffff)

    def counter(self):
        a = eval_expr(self.cfg['_counter'], self.rd)
        v = self.rd(a) if a else None
        return v

    def drawn(self):
        return all(c.holds(self.rd) for c in self.cfg['_drawn'])

    def hold(self):
        if not self.held:
            ntdll.NtSuspendProcess(self.h)
            self.held = True

    def release(self):
        if self.held:
            ntdll.NtResumeProcess(self.h)
            self.held = False

    def alive(self):
        return self.p.poll() is None

    def window(self):
        if self.hwnd and u32.IsWindow(self.hwnd):
            return self.hwnd
        cands = []

        @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
        def cb(hw, lp):
            pid = wt.DWORD()
            u32.GetWindowThreadProcessId(hw, ctypes.byref(pid))
            if pid.value == self.p.pid and u32.IsWindowVisible(hw):
                r = wt.RECT()
                u32.GetClientRect(hw, ctypes.byref(r))
                cands.append((hw, r.right, r.bottom, window_title(hw)))
            return True
        u32.EnumWindows(cb, 0)
        self.hwnd = pick_window(cands, self.cfg['client'], self.cfg.get('title', ''))
        if self.hwnd:
            pos = self.cfg.get('_place')   # the run's place (a config's own pos is ignored: the place decides)
            # topmost, never activated: nothing of the user's comes over it while it is captured
            u32.SetWindowPos(self.hwnd, wt.HWND(-1), pos[0] if pos else 0, pos[1] if pos else 0, 0, 0, 0x0001 | 0x0010 | (0 if pos else 0x0002))
        return self.hwnd


# ---------------------------------------------------------------- origrun's shared blocks
class CurShared(ctypes.Structure):
    """tools/origrun/cursor.h (CUR_MAGIC 'CURS', CUR_VERSION 2)."""
    _fields_ = [('magic', ctypes.c_uint32), ('version', ctypes.c_uint32), ('size', ctypes.c_uint32), ('set', ctypes.c_int32),
                ('pos', ctypes.c_int64), ('posts', ctypes.c_int32), ('installed', ctypes.c_int32), ('reads', ctypes.c_int32),
                ('realReads', ctypes.c_int32), ('keysSet', ctypes.c_int32), ('keyPosts', ctypes.c_int32),
                ('keysInstalled', ctypes.c_int32), ('keyReads', ctypes.c_int32), ('keyRealReads', ctypes.c_int32),
                ('keys', ctypes.c_uint8 * 256)]


class Cursor:
    """The point the stand-in's GetCursorPos answers with, in place of the user's cursor."""

    def __init__(self, run_id):
        self.mm = mmap.mmap(-1, ctypes.sizeof(CurShared), tagname='Local\\origrun-cursor-' + run_id)
        self.blk = CurShared.from_buffer(self.mm)
        self.blk.magic, self.blk.version, self.blk.size = 0x52535543, 2, ctypes.sizeof(CurShared)

    def point(self, x, y):
        self.blk.pos = (y << 32) | (x & 0xffffffff)
        self.blk.set = 1
        self.blk.posts += 1

    def stats(self):
        return 'installed %d, reads %d, real reads %d' % (self.blk.installed, self.blk.reads, self.blk.realReads)


# tools/origrun/vclock.h's VcShared, field for field, at VC_VERSION 3 (6b5f6492 added the frame reader's fields after
# heldTimeout). The 32-bit stand-in aligns its LONGLONGs to 8 as ctypes does, so the offsets come from this structure; the
# selftest holds them to the ones i686-w64-mingw32-clang gives for the header (offsetof). A block of another version or size
# is refused: with the version 2 offsets a version 3 block's ring was read as garbage, and the drain never ended (Heavy
# Weapon's 3D run, 2026-11-22: its watch.log reached 17.6 GB and the game was never closed).
VC_MAGIC, VC_VERSION, VC_RING = 0x4b4c4356, 3, 16384
_I, _U, _Q = ctypes.c_int32, ctypes.c_uint32, ctypes.c_int64


class VcShared(ctypes.Structure):
    _fields_ = [('magic', _U), ('version', _U), ('size', _U), ('pid', _U), ('driverTid', _U),
                ('installed', _I), ('connected', _I), ('gone', _I),
                ('v', _Q), ('counter', _I), ('counterOk', _I),
                ('bar', _I), ('holdKind', _I), ('holdUpdate', _I), ('holdV', _Q),
                ('barrierOff', _I), ('barrierOffWhy', _I), ('barrierOffCounter', _I),
                ('held', _I), ('heldSeq', _I), ('heldCounter', _I), ('heldWhere', _I), ('heldV', _Q), ('heldTimeout', _I),
                ('heldAt', _U),
                ('readReq', _I), ('readDone', _I), ('readStatus', _I), ('readHr', _I), ('readSurface', _U), ('readCaps', _U),
                ('readW', _I), ('readH', _I), ('readPitch', _I), ('readBpp', _I),
                ('readPfFlags', _U), ('readRMask', _U), ('readGMask', _U), ('readBMask', _U), ('readBytes', _I),
                ('readCounter', _I),
                ('waits', _I), ('passWaits', _I), ('polls', _I), ('maxReads', _I), ('holds', _I),
                ('heldReal100', _Q), ('installReal100', _Q), ('nowReal100', _Q), ('otherWaits', _I), ('realWaits', _I),
                ('realWait100', _Q), ('handleWaits', _I), ('handleTimeouts', _I), ('handleWait100', _Q),
                ('ringHead', _I), ('ringTail', _I), ('ringDropped', _I), ('ring', ctypes.c_byte * (16 * VC_RING))]


VC_SIZE = ctypes.sizeof(VcShared)
VC_OFF = {name: getattr(VcShared, name).offset for name, _ in VcShared._fields_}
BAR_FREE, BAR_ARMED, BAR_HELD = 0, 1, 2
HOLD_NEXT, HOLD_UPDATE, HOLD_WINDOW = 0, 1, 3


class Clock:
    """The stand-in's virtual clock block of one game process: arm a barrier, wait for the hold, let go."""

    def __init__(self, cid, g, timeout=30):
        self.g = g
        name = 'Local\\origrun-vclock-%s-%d' % (cid, g.p.pid)
        t0 = time.time()
        while True:
            h = k32.OpenFileMappingW(0xF001F, False, name)
            if h:
                break
            if not g.alive() or time.time() - t0 > timeout:
                raise SystemExit('visrun: the game made no virtual clock (%s)' % name)
            time.sleep(0.005)
        self.base = k32.MapViewOfFile(h, 0xF001F, 0, 0, VC_SIZE)
        while self.get('magic', ctypes.c_uint32) != VC_MAGIC:   # the stand-in writes it last
            if time.time() - t0 > timeout:
                raise SystemExit('visrun: the clock block never got its magic')
            time.sleep(0.002)
        version, size = self.get('version', ctypes.c_uint32), self.get('size', ctypes.c_uint32)
        if version != VC_VERSION or size != VC_SIZE:
            raise SystemExit('visrun: the clock block is version %d of %d bytes, and visrun knows version %d of %d bytes: '
                             'rebuild dsound.dll from this checkout (tools/origrun), or bring visrun up to the block'
                             % (version, size, VC_VERSION, VC_SIZE))
        self.held_ev = k32.OpenEventW(0x1F0003, False, name + '-held')
        self.rel_ev = k32.OpenEventW(0x1F0003, False, name + '-release')
        self.set('connected', 1)
        self.last = ['?'] * 4

    def get(self, key, t=ctypes.c_int32):
        return t.from_address(self.base + VC_OFF[key]).value

    def set(self, key, v):
        ctypes.c_int32.from_address(self.base + VC_OFF[key]).value = v

    def hold_until(self, kind, upd, timeout):
        """True: held (at once when a held game already meets it); False: the game ended or the wait ran out; None: holds are off."""
        if self.get('barrierOff'):
            return None
        if kind == HOLD_UPDATE and self.get('held') and self.get('bar') == BAR_HELD:
            hc = self.get('heldCounter')
            if hc != -1 and hc - upd >= 0:
                return True
        seq0 = self.get('heldSeq')
        self.set('holdKind', kind)
        self.set('holdUpdate', upd)
        self.set('bar', BAR_ARMED)
        k32.SetEvent(self.rel_ev)
        t0 = time.time()
        while True:
            if self.get('heldSeq') != seq0 and self.get('held') and self.get('bar') == BAR_HELD:
                return True
            if self.get('barrierOff'):
                return None
            if not self.g.alive():
                return False
            if time.time() - t0 >= timeout:
                break
            k32.WaitForSingleObject(self.held_ev, 5)
        if self.get('bar') == BAR_ARMED:   # take the barrier back (the game has not claimed it)
            self.set('bar', BAR_FREE)
        return False

    def counter(self):
        return self.get('heldCounter')

    def step_on(self):
        """A game held before its window (the start hold, ORIGRUN_CLOCK_HOLDSTART, comes at the main thread's first wait,
        and Bookworm Adventures waits before it makes its window) runs on to its first wait with its window up and holds
        there (the stand-in's HOLD_WINDOW: no round trip at each wait, which cost about 100 ms a wait on a busy machine).
        True when it was held and is let go. Called only while the window is still missing."""
        if self.get('barrierOff') or not (self.get('held') and self.get('bar') == BAR_HELD):
            return False
        self.set('holdKind', HOLD_WINDOW)
        self.set('holdUpdate', 0)
        self.set('bar', BAR_ARMED)
        k32.SetEvent(self.rel_ev)
        return True

    def let_go(self):
        self.set('bar', BAR_FREE)
        k32.SetEvent(self.rel_ev)

    def drain(self, f):
        """The --watch changes the game queued, as origrun's "watch N: a -> b at update U" lines."""
        head, tail = self.get('ringHead'), self.get('ringTail')
        if not 0 <= head - tail <= VC_RING:   # never more than a ring's worth: a wrong block or a torn read must not loop on
            f.write("watch: the ring's head %d and tail %d are %d apart, more than its %d records: skipped\n"
                    % (head, tail, head - tail, VC_RING))
            self.set('ringTail', head)
            f.flush()
            return
        while tail != head:
            u, idx, val, ok = struct.unpack('<iiii', ctypes.string_at(self.base + VC_OFF['ring'] + (tail % VC_RING) * 16, 16))
            new = str(val) if ok else '-'
            if 0 <= idx < 4:
                f.write('watch %d: %s -> %s at update %d\n' % (idx + 1, self.last[idx], new, u))
                self.last[idx] = new
            tail += 1
        self.set('ringTail', tail)
        f.flush()


# ---------------------------------------------------------------- stopping and starting the game between steps
class Sync:
    """How the driver gets the game to stand still at an update: on the real clock by suspending the process, on the virtual clock
    by the stand-in's barrier. reach(n): wait for the counter to be n or more; stop(): stand still now; go(): let it run."""

    def __init__(self, g, clk, timeout):
        self.g, self.clk, self.timeout = g, clk, timeout

    def reach(self, n, deadline_ts):
        if self.clk:
            r = self.clk.hold_until(HOLD_UPDATE, n, min(self.timeout, max(1, deadline_ts - time.time())))
            return self.clk.counter() if r else None
        t0 = time.time()
        while time.time() - t0 < self.timeout and time.time() < deadline_ts:
            c = self.g.counter()
            if c is not None and c >= n:
                return c
            if not self.g.alive():
                return None
            if c is None or n - c > 8:
                time.sleep(0.004)
            else:
                short_sleep(0.0005)
        return None

    def stop(self):
        if self.clk:
            if not (self.clk.get('held') and self.clk.get('bar') == BAR_HELD) and not self.clk.hold_until(HOLD_NEXT, 0, 5):
                return None
            return self.clk.counter()
        self.g.hold()
        return self.g.counter()

    def go(self):
        if not self.clk:
            self.g.release()   # on the virtual clock the next reach() lets it go


def grab(hw, g, client, shape, release_after):
    """The game's window as the window itself renders it (PrintWindow, PW_RENDERFULLCONTENT), never the screen. PrintWindow needs the
    game's thread to answer: on the real clock it is called from a helper thread and the game released 0.5 ms into it (release_after);
    on the virtual clock the game stays held at its wait, which answers the sent messages. Returns the client area (the whole window's
    sandbox-shaped canvas with shape "sandbox"), or None."""
    from PIL import Image
    r = wt.RECT()
    u32.GetWindowRect(hw, ctypes.byref(r))
    w, h = r.right - r.left, r.bottom - r.top
    cx, cy, cw, ch = client_rect(hw)
    sdc = u32.GetDC(None)
    mdc = g32.CreateCompatibleDC(sdc)
    bmp = g32.CreateCompatibleBitmap(sdc, w, h)
    u32.ReleaseDC(None, sdc)
    old = g32.SelectObject(mdc, bmp)
    done = []
    t = threading.Thread(target=lambda: done.append(u32.PrintWindow(hw, mdc, 2)), daemon=True)
    t.start()
    if release_after:
        t0 = time.perf_counter()
        while time.perf_counter() - t0 < 0.0005:
            pass
        g.release()
    t.join(3 if release_after else 10)
    if t.is_alive() or not done or not done[0]:
        return None   # a PrintWindow that is late is left to finish; its DC is not freed

    class BMI(ctypes.Structure):
        _fields_ = [('biSize', wt.DWORD), ('biWidth', wt.LONG), ('biHeight', wt.LONG), ('biPlanes', wt.WORD),
                    ('biBitCount', wt.WORD), ('biCompression', wt.DWORD), ('biSizeImage', wt.DWORD),
                    ('biXPelsPerMeter', wt.LONG), ('biYPelsPerMeter', wt.LONG), ('biClrUsed', wt.DWORD),
                    ('biClrImportant', wt.DWORD), ('pad', wt.DWORD * 4)]
    buf = ctypes.create_string_buffer(w * h * 4)
    bmi = BMI()
    bmi.biSize = 40
    bmi.biWidth, bmi.biHeight, bmi.biPlanes, bmi.biBitCount = w, -h, 1, 32
    g32.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bmi), 0)
    g32.SelectObject(mdc, old)
    g32.DeleteObject(bmp)
    g32.DeleteDC(mdc)
    ox, oy = cx - r.left, cy - r.top
    im = Image.frombuffer('RGBA', (w, h), buf, 'raw', 'BGRA', 0, 1).convert('RGB').crop((ox, oy, ox + cw, oy + ch))
    if shape == 'sandbox':   # origrun's sandbox shots: the client at (3, 26) of a canvas 6 and 29 pixels larger (shotdiff cuts it away)
        canvas = Image.new('RGB', (cw + 6, ch + 29))
        canvas.paste(im, (3, 26))
        return canvas
    return im


def xy(x, y):
    return (y << 16) | (x & 0xffff)


def post_input(g, cur, cmd, args):
    """Posts the input to the game's window, and (with the scripted cursor) writes the point its GetCursorPos reads."""
    hw = g.window()
    cx, cy, _, _ = client_rect(hw)
    if cmd in MOUSE_CMDS:
        if len(args) >= 2:
            x, y = int(args[0]), int(args[1])
            if cur:
                cur.point(cx + x, cy + y)
        else:
            x = y = 0
        lp = xy(x, y)
        P = u32.PostMessageW
        if cmd == 'move':
            P(hw, WM_MOUSEMOVE, 0, lp)
        elif cmd == 'click':
            P(hw, WM_MOUSEMOVE, 0, lp), P(hw, WM_LBUTTONDOWN, 1, lp), P(hw, WM_LBUTTONUP, 0, lp)
        elif cmd == 'dclick':
            P(hw, WM_MOUSEMOVE, 0, lp), P(hw, WM_LBUTTONDOWN, 1, lp), P(hw, WM_LBUTTONUP, 0, lp)
            P(hw, WM_LBUTTONDBLCLK, 1, lp), P(hw, WM_LBUTTONUP, 0, lp)
        elif cmd == 'rclick':
            P(hw, WM_MOUSEMOVE, 0, lp), P(hw, WM_RBUTTONDOWN, 2, lp), P(hw, WM_RBUTTONUP, 0, lp)
        elif cmd == 'down':
            P(hw, WM_MOUSEMOVE, 0, lp), P(hw, WM_LBUTTONDOWN, 1, lp)
        elif cmd == 'up':
            P(hw, WM_LBUTTONUP, 0, lp)
        elif cmd == 'rdown':
            P(hw, WM_MOUSEMOVE, 0, lp), P(hw, WM_RBUTTONDOWN, 2, lp)
        elif cmd == 'rup':
            P(hw, WM_RBUTTONUP, 0, lp)
    elif cmd == 'focus':
        on = args[0] == 'regain'
        u32.PostMessageW(hw, 0x001C, 1 if on else 0, 0)       # WM_ACTIVATEAPP
        u32.PostMessageW(hw, 0x0006, 1 if on else 0, 0)       # WM_ACTIVATE
        u32.PostMessageW(hw, 0x0007 if on else 0x0008, 0, 0)  # WM_SETFOCUS / WM_KILLFOCUS
    elif cmd == 'key':
        vk = int(args[0], 0)
        sc = u32.MapVirtualKeyW(vk, 0)
        u32.PostMessageW(hw, 0x0100, vk, 1 | (sc << 16))
        u32.PostMessageW(hw, 0x0101, vk, 1 | (sc << 16) | (1 << 30) | (1 << 31))
    elif cmd == 'text':
        for ch in ' '.join(args):
            u32.PostMessageW(hw, 0x0102, ord(ch), 1)
    else:
        raise SystemExit('visrun: cannot post %s' % cmd)


# ---------------------------------------------------------------- staging, registry, saves
def reg(*a):
    return subprocess.run(['reg'] + list(a), capture_output=True, text=True)


def backup_dir(cfg):
    return os.path.join(WORK, cfg['name'], 'backup')


def stage_state(cfg, want3d, preset_text=None):
    """Backs up the game's registry key and save folders, then starts them clean (the key deleted, with its cached Direct3D test, and
    set to the config's values and the 3D switch; the save folders emptied and the config's save_seed files copied in)."""
    b = backup_dir(cfg)
    if os.path.exists(b):
        raise SystemExit('visrun: %s exists: an earlier run was not restored (visrun.py %s --restore)' % (b, cfg['_path']))
    os.makedirs(b)
    man = {'key': cfg.get('reg_key'), 'dirs': []}
    if cfg.get('reg_key'):
        man['had_key'] = reg('export', 'HKCU\\' + cfg['reg_key'], os.path.join(b, 'key.reg'), '/y').returncode == 0
    for i, d in enumerate(cfg['save_dirs']):
        had = os.path.isdir(d)
        man['dirs'].append([d, had])
        if had:
            shutil.copytree(d, os.path.join(b, 'saves%d' % i))
    json.dump(man, open(os.path.join(b, 'manifest.json'), 'w'))
    for d in cfg['save_dirs']:
        shutil.rmtree(d, ignore_errors=True)
    for src, dst in cfg.get('save_seed', []):
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copyfile(src, dst)
    if cfg.get('reg_key'):
        reg('delete', 'HKCU\\' + cfg['reg_key'], '/f')
        if cfg.get('reg_preset'):
            text = open(cfg['reg_preset'], 'rb').read().decode('utf-16')
            text = text.replace('HKEY_LOCAL_MACHINE\\SOFTWARE\\PopCap', 'HKEY_CURRENT_USER\\Software\\PopCap')
            conv = os.path.join(b, 'preset.reg')
            open(conv, 'wb').write(b'\xff\xfe' + text.encode('utf-16-le'))
            reg('import', conv)
        import winreg
        vals = dict(cfg['reg_values'])
        if cfg.get('reg_3d_value') and want3d is not None:
            vals[cfg['reg_3d_value']] = 1 if want3d else 0
        with winreg.CreateKey(winreg.HKEY_CURRENT_USER, cfg['reg_key']) as k:
            for name, v in vals.items():
                if isinstance(v, int):
                    winreg.SetValueEx(k, name, 0, winreg.REG_DWORD, v)
                else:
                    winreg.SetValueEx(k, name, 0, winreg.REG_SZ, str(v))


def restore_state(cfg):
    b = backup_dir(cfg)
    mf = os.path.join(b, 'manifest.json')
    if not os.path.exists(mf):
        return False
    man = json.load(open(mf))
    if man['key']:
        reg('delete', 'HKCU\\' + man['key'], '/f')
        if man.get('had_key'):
            r = reg('import', os.path.join(b, 'key.reg'))
            if r.returncode != 0:
                raise SystemExit('visrun: reg import failed, the backup stays in %s: %s' % (b, r.stderr))
    for i, (d, had) in enumerate(man['dirs']):
        shutil.rmtree(d, ignore_errors=True)
        if had:
            shutil.copytree(os.path.join(b, 'saves%d' % i), d)
    shutil.rmtree(b)
    return True


def apply_patches(list_path, exe_dir, seed):
    """origrun's patch list (docs/ORIGRUN.md, "Patching the copy"): <kind> <file> <where va:|rva:|off:> <expected> <replacement>
    <reason...>, {seed} four little-endian bytes. Every entry is checked before any is written; a mismatch stops the run."""
    if not os.path.exists(list_path):
        return []
    todo = []
    for no, line in enumerate(open(list_path, encoding='utf-8', errors='replace'), 1):
        w = line.split(None, 5)
        if not w or w[0].startswith('#'):
            continue
        if len(w) < 5:
            raise SystemExit('visrun: patch list line %d needs kind file where expected replacement' % no)
        kind, f, where, exp, rep = w[:5]
        if '{seed}' in rep.lower():
            if seed is None:
                continue
            rep = re.sub(r'\{seed\}', struct.pack('<I', seed).hex(), rep, flags=re.I)
        todo.append((kind, f, where, bytes.fromhex(exp.replace('.', '').replace('_', '')), bytes.fromhex(rep.replace('.', '').replace('_', ''))))
    plan_ = []
    for kind, f, where, exp, rep in todo:
        path = os.path.join(exe_dir, f)
        data = bytearray(open(path, 'rb').read())
        off = resolve_where(where, data)
        if off is None or data[off:off + len(exp)] != exp:
            raise SystemExit('visrun: patch %s at %s: the copy has %s, expected %s' % (kind, where, bytes(data[off:off + len(exp)]).hex() if off is not None else 'no such place', exp.hex()))
        plan_.append((path, off, rep, kind))
    for path, off, rep, kind in plan_:
        data = bytearray(open(path, 'rb').read())
        data[off:off + len(rep)] = rep
        open(path, 'wb').write(data)
    return [p[3] for p in plan_]


def resolve_where(where, data):
    kind, _, v = where.partition(':')
    v = int(v, 16)
    if kind == 'off':
        return v
    pe = struct.unpack_from('<I', data, 0x3c)[0]
    count, opt = struct.unpack_from('<H', data, pe + 6)[0], struct.unpack_from('<H', data, pe + 20)[0]
    base = struct.unpack_from('<I', data, pe + 24 + 28)[0] if kind == 'va' else 0
    for i in range(count):
        _, size, rva, raw_size, raw = struct.unpack_from('<8sIIII', data, pe + 24 + opt + 40 * i)
        if rva <= v - base < rva + max(size, raw_size):
            return raw + v - base - rva
    return None


def stage_copy(cfg, tag, seed, log):
    """The game through tools/origrun (--copy-only: the install, or the ready copy, is staged under build/origrun or --stage-root),
    then a fresh working copy under build/visrun/<tag>/game with no wrapper (dgVoodoo's files, an old stand-in), no saves or logs, the
    clean exe where an earlier run patched it, the patch list applied and origrun's stand-in (dsound.dll) placed beside the exe."""
    origrun = os.path.join(ORIGRUN_BIN, 'origrun.exe')
    if not os.path.exists(origrun):
        raise SystemExit('visrun: %s is not built (tools/origrun/build.sh)' % origrun)
    if cfg.get('source_exe'):
        exe = os.path.abspath(os.path.join(ROOT, cfg['source_exe']))
    else:
        cmd = [origrun, '--game', cfg['game'], '--run', 'visrun-' + cfg['name'], '--copy-only']
        if cfg.get('stage_root'):
            cmd += ['--stage-root', cfg['stage_root']]
        r = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
        m = re.search(r'^(?:\[[\d:]+\])?\s*exe:\s*(.+?)\s*$', r.stdout + r.stderr, re.M)
        if r.returncode != 0 or not m:
            raise SystemExit('visrun: origrun --copy-only failed (%d): %s' % (r.returncode, (r.stdout + r.stderr)[-600:]))
        exe = m.group(1)
    src = os.path.dirname(exe)
    work = os.path.join(cfg.get('work_root') or WORK, tag, 'game')
    shutil.rmtree(work, ignore_errors=True)
    skip = shutil.ignore_patterns('users', 'logs', 'crash.txt', '*.log', '*.origrun-unpatched', '*.manifest', *WRAPPER_FILES)
    shutil.copytree(src, work, ignore=skip)
    leaf = cfg.get('exe') or os.path.basename(exe)
    clean = exe + '.origrun-unpatched'
    if cfg.get('carve_exe'):   # the real exe carved out of the game's wrapper (tools/extract_game_exe.py), placed in the copy
        shutil.copyfile(os.path.join(ROOT, cfg['carve_exe']), os.path.join(work, leaf))
    elif os.path.exists(clean):
        shutil.copyfile(clean, os.path.join(work, leaf))
    plist = cfg.get('patches')
    applied = apply_patches(os.path.join(ROOT, plist), work, seed) if plist else []
    if seed is not None and plist and not any(k == 'seed' for k in applied):
        raise SystemExit('visrun: --seed %d, but no entry of %s carries {seed}' % (seed, plist))
    shutil.copyfile(os.path.join(ORIGRUN_BIN, 'dsound.dll'), os.path.join(work, 'dsound.dll'))
    print('copy %s; patches applied: %s' % (work, applied or 'none'), file=log, flush=True)
    return os.path.join(work, leaf)


def muted(origdir, pid):
    """The stand-in's sound.log says the process's default audio session is muted (a line after its 'attached pid=<pid>')."""
    try:
        lines = open(os.path.join(origdir, 'logs', 'sound.log'), errors='replace').read().split('\n')
    except OSError:
        return False
    on = False
    for l in lines:
        if 'attached pid=%d ' % pid in l:
            on = True
        elif on and 'default audio session muted on' in l and 'WARNING' not in l:
            return True
    return False


def describe_window(hwnd):
    """The window that holds the foreground, for the log: class, title, style, extended style and the thread that owns it."""
    cls = ctypes.create_unicode_buffer(256)
    title = ctypes.create_unicode_buffer(256)
    u32.GetClassNameW(wt.HWND(hwnd), cls, 256)
    u32.GetWindowTextW(wt.HWND(hwnd), title, 256)
    style = u32.GetWindowLongW(wt.HWND(hwnd), -16) & 0xFFFFFFFF   # GWL_STYLE
    ex = u32.GetWindowLongW(wt.HWND(hwnd), -20) & 0xFFFFFFFF      # GWL_EXSTYLE
    pid = wt.DWORD()
    tid = u32.GetWindowThreadProcessId(wt.HWND(hwnd), ctypes.byref(pid))
    return 'window 0x%x class "%s" title "%s" style 0x%08x exstyle 0x%08x thread %d process %d' % (hwnd, cls.value, title.value, style, ex, tid, pid.value)


def copy_net_log_tail(origdir, pid, log, n=30):
    """The last n lines of the run folder's logs/net.log that start with this process's pid (the stand-in writes each activation
    event there at once, so a killed run still shows what happened), into visrun.log."""
    try:
        lines = open(os.path.join(origdir, 'logs', 'net.log'), errors='replace').read().split(chr(10))
    except OSError as e:
        print('no net.log to copy (%s)' % e, file=log, flush=True)
        return
    mine = [l for l in lines if l.startswith('pid %d:' % pid) or l.startswith('pid %d ' % pid)]
    print('the last %d of the %d net.log lines of pid %d:' % (min(n, len(mine)), len(mine), pid), file=log)
    for l in mine[-n:]:
        print('  ' + l.rstrip(chr(13)), file=log)
    log.flush()


def activation_hook_state(origdir, pid):
    """What the stand-in's net.log says about its activation hook in this process: installed (with the functions it reached), off
    (ORIGRUN_ACTIVATION=allow) or not logged at all."""
    try:
        lines = open(os.path.join(origdir, 'logs', 'net.log'), errors='replace').read().split('\n')
    except OSError:
        return 'NOT INSTALLED: no net.log in the run folder'
    for l in lines:
        if l.startswith('pid %d: activation hook installed' % pid):
            return 'installed (%s)' % l.split(': ', 2)[2].split(' (a window')[0]
        if l.startswith('pid %d: activation hook off' % pid):
            return 'OFF (ORIGRUN_ACTIVATION=allow): the game may take the foreground, and the run stops if it does'
    return 'NOT INSTALLED: net.log has no activation line for pid %d (an older stand-in?)' % pid


def gamecount_has_room():
    r = subprocess.run([sys.executable, '-I', os.path.join(ROOT, 'tools', 'gamecount.py'), '--limit'], capture_output=True, text=True)
    return r.returncode == 0, r.stdout.strip().split('\n')[0]


# ---------------------------------------------------------------- the places on the right half of the screen
# The user's rule (2026-11-22, "Right half, four", docs/RULES.md): runs on the visible desktop keep to the right half of the
# primary screen, each window in a place of its own, at most four there at once. A place is a cell of an even grid over the
# right half, at least PLACE_MIN large (it holds an 800x600 client with its frame); places are numbered row by row. A window
# too large for one place takes two adjacent ones (side by side, or one above the other).
MAX_PLACES = 4
PLACE_MIN = (840, 680)
FRAME = (40, 80)   # what a window's frame and title bar add to its client area, with room to spare
SHELL_CLASSES = ('Progman', 'WorkerW', 'Shell_TrayWnd', 'Shell_SecondaryTrayWnd')   # the desktop and the taskbar are not windows in the way


def layout(sw, sh):
    """The places of a screen sw x sh: (cols, rows, [(x, y, w, h), ...]). The right half starts at sw // 2."""
    x0 = sw // 2
    right = sw - x0
    cols = right // PLACE_MIN[0]
    n = min(MAX_PLACES, cols * (sh // PLACE_MIN[1])) if cols else 0
    if n == 0:
        return 0, 0, []
    rows = -(-n // cols)
    cw, ch = right // cols, sh // rows
    return cols, rows, [(x0 + (i % cols) * cw, (i // cols) * ch, cw, ch) for i in range(n)]


def place_groups(lay, need):
    """The groups of places a window of outer size need may take, in the order they are tried: each single place that holds it,
    then pairs side by side, then pairs one above the other. A group is a tuple of place numbers."""
    cols, rows, places = lay
    groups = []
    if not places:
        return groups
    cw, ch = places[0][2], places[0][3]
    if need[0] <= cw and need[1] <= ch:
        groups += [(i,) for i in range(len(places))]
    if need[0] <= 2 * cw and need[1] <= ch:
        groups += [(i, i + 1) for i in range(len(places) - 1) if cols >= 2 and i % cols < cols - 1]
    if need[0] <= cw and need[1] <= 2 * ch:
        groups += [(i, i + cols) for i in range(len(places) - cols)]
    return groups


def group_rect(lay, group):
    """(left, top, right, bottom) of the places of a group together."""
    ps = [lay[2][i] for i in group]
    return (min(p[0] for p in ps), min(p[1] for p in ps), max(p[0] + p[2] for p in ps), max(p[1] + p[3] for p in ps))


def need_size(client):
    return client[0] + FRAME[0], client[1] + FRAME[1]


def screen_size():
    return u32.GetSystemMetrics(0), u32.GetSystemMetrics(1)


def pid_alive(pid):
    h = k32.OpenProcess(0x1000, False, pid) if pid > 0 else None
    if not h:
        return False
    code = wt.DWORD()
    ok = k32.GetExitCodeProcess(h, ctypes.byref(code)) and code.value == 259   # STILL_ACTIVE
    k32.CloseHandle(h)
    return bool(ok)


def lock_dir():
    base = os.environ.get('LOCALAPPDATA') or os.path.join(os.path.expanduser('~'), 'AppData', 'Local')
    return os.path.join(base, 'byo-pop', 'visrun')


class LockDir:
    """Machine-wide locks, one file each under %LOCALAPPDATA%\\byo-pop\\visrun\\ (every worktree sees them): <name>.lock holds a JSON
    line {pid, run, start, game_pid}. A file is made with O_EXCL, so one process gets it; a holder that is gone is taken over,
    by renaming its file away first (only one taker wins the rename). Names: place0..place3, and game-<registry key> (two runs
    of one game would share its registry key and saves)."""

    def __init__(self, path, alive=pid_alive):
        self.path = path
        self.alive = alive
        self.mine = {}   # name -> (fd, info)
        os.makedirs(path, exist_ok=True)

    def file(self, name):
        return os.path.join(self.path, name + '.lock')

    def holder(self, name):
        """The live holder's info dict, or None when the lock is free or its holder is gone."""
        try:
            with open(self.file(name)) as f:
                text = f.read()
        except FileNotFoundError:
            return None
        except OSError:
            return {'pid': 0, 'run': '?', 'start': time.time()}
        try:
            info = json.loads(text)
        except ValueError:
            # empty or half written: held for a moment by a taker; a file that stays so for 10 s is a leftover
            if time.time() - os.path.getmtime(self.file(name)) < 10:
                return {'pid': 0, 'run': '?', 'start': time.time()}
            return None
        return info if self.alive(info.get('pid', 0)) else None

    def take(self, name, info):
        """True when the lock is now this process's."""
        for _ in range(3):
            try:
                fd = os.open(self.file(name), os.O_CREAT | os.O_EXCL | os.O_WRONLY)
            except FileExistsError:
                if self.holder(name) is not None:
                    return False
                gone = '%s.dead.%d.%s' % (self.file(name), os.getpid(), uuid.uuid4().hex[:6])
                try:
                    os.rename(self.file(name), gone)
                    os.remove(gone)
                except OSError:
                    pass
                continue
            mine = dict(info, pid=os.getpid(), start=time.time())
            os.write(fd, json.dumps(mine).encode())
            self.mine[name] = (fd, mine)
            return True
        return False

    def take_all(self, names, info):
        """All of the names or none, taken in the order given."""
        got = []
        for n in names:
            if not self.take(n, info):
                for g in got:
                    self.release(g)
                return False
            got.append(n)
        return True

    def update(self, extra):
        """Add fields (the game's pid) to every lock this process holds."""
        for name, (fd, cur) in self.mine.items():
            cur.update(extra)
            os.lseek(fd, 0, 0)
            os.write(fd, json.dumps(cur).encode())   # the line only grows, so it overwrites the old one whole

    def release(self, name):
        fd, _ = self.mine.pop(name)
        os.close(fd)
        try:
            os.remove(self.file(name))
        except OSError:
            pass

    def release_all(self):
        for n in list(self.mine):
            self.release(n)


def list_windows():
    """The visible, not minimised, not cloaked top-level windows: dicts with hwnd, pid, cls, title and rect (l, t, r, b)."""
    found = []
    dwm = ctypes.WinDLL('dwmapi')

    @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
    def cb(hw, lp):
        if u32.IsWindowVisible(hw) and not u32.IsIconic(hw):
            cloaked = wt.DWORD()
            if dwm.DwmGetWindowAttribute(hw, 14, ctypes.byref(cloaked), 4) == 0 and cloaked.value:   # DWMWA_CLOAKED
                return True
            r = wt.RECT()
            u32.GetWindowRect(hw, ctypes.byref(r))
            pid = wt.DWORD()
            u32.GetWindowThreadProcessId(hw, ctypes.byref(pid))
            buf = ctypes.create_unicode_buffer(128)
            u32.GetClassNameW(hw, buf, 128)
            found.append(dict(hwnd=hw, pid=pid.value, cls=buf.value, title=window_title(hw), rect=(r.left, r.top, r.right, r.bottom)))
        return True
    u32.EnumWindows(cb, 0)
    return found


def blocking(windows, rect, own_pids=()):
    """Of windows (list_windows' dicts), those that overlap rect and are not the run's own (own_pids) nor the desktop or taskbar.
    Any such window, a game's, an older visrun's or the user's own, blocks a start in that place."""
    return [w for w in windows if w['pid'] not in own_pids and w['cls'] not in SHELL_CLASSES and rects_overlap(w['rect'], rect)]


def describe(w):
    return '%s %r (pid %d)' % (w['cls'], w['title'][:30], w['pid'])


def place_names(group):
    return ['place%d' % i for i in group]


def game_lock_name(cfg):
    return 'game-' + re.sub(r'[^A-Za-z0-9_.-]', '_', str(cfg.get('reg_key') or cfg['name']))


def show_places():
    sw, sh = screen_size()
    cols, rows, places = layout(sw, sh)
    locks = LockDir(lock_dir())
    wins = list_windows()
    print('screen %dx%d, right half from x=%d: %d places (%d column(s) x %d row(s))%s; locks in %s' % (
        sw, sh, sw // 2, len(places), cols, rows, ', each %dx%d' % places[0][2:] if places else '', locks.path))
    for i, (x, y, w, h) in enumerate(places):
        info = locks.holder('place%d' % i)
        own = {info.get('pid'), info.get('game_pid')} if info else set()
        held = 'free' if not info else 'held by %s (pid %d%s, %.1f min)' % (
            info.get('run', '?'), info.get('pid', 0), ', game %d' % info['game_pid'] if info.get('game_pid') else '', (time.time() - info.get('start', time.time())) / 60)
        other = blocking(wins, (x, y, x + w, y + h), own)
        print('place %d  x=%d y=%d %dx%d  %s%s' % (i, x, y, w, h, held, "  | a window that is not the run's own overlaps: " + '; '.join(describe(o) for o in other[:3]) if other else ''))
    for f in sorted(os.listdir(locks.path)):
        if f.startswith('game-') and f.endswith('.lock'):
            info = locks.holder(f[:-5])
            if info:
                print('%s  held by %s (pid %d)' % (f[:-5], info.get('run', '?'), info.get('pid', 0)))


# ---------------------------------------------------------------- the run
def end_tree(p, log):
    """End the process this run started and every process it started (taskkill /T, by its pid), then wait briefly.
    Never raises: the caller restores the user's registry key and saves next, whatever happened here."""
    try:
        subprocess.run(['taskkill', '/T', '/F', '/PID', str(p.pid)], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, timeout=30)
    except (OSError, subprocess.TimeoutExpired):
        pass
    try:
        p.kill()
    except OSError:
        pass
    try:
        p.wait(10)
        print('killed', file=log, flush=True)
    except subprocess.TimeoutExpired:
        print('killed, but the game (pid %d) had not ended 10 s later' % p.pid, file=log, flush=True)


def run(cfg, steps, forbidden, out, want3d, seed, deadline, shape, keep_copy, timeout, quiet_s):
    tag = cfg['name']
    os.makedirs(out, exist_ok=True)
    log = open(os.path.join(out, 'visrun.log'), 'w')
    fg_taken = threading.Event()   # keep_focus_away saw the game's process own the foreground window
    steps_f = open(os.path.join(out, 'steps.tsv'), 'w')
    steps_f.write('# wanted\tlanded\tcmd\targs\tseconds\tvalues\n')
    deadline_ts = deadline.timestamp()
    cfg['_counter'] = parse_expr(cfg['counter'])
    cfg['_drawn'] = [Cond(c) for c in cfg['drawn']]
    cues = {k: Cond(v) for k, v in cfg['cues'].items()}
    values = {k: parse_expr(v) for k, v in cfg.get('values', {}).items()}
    ok, line = gamecount_has_room()
    if not ok:
        raise SystemExit('visrun: no room for a game process (%s)' % line)
    lay = layout(*screen_size())
    groups = place_groups(lay, need_size(cfg['client']))
    if not groups:
        raise SystemExit('visrun: no place on the right half of this screen (%d places of at least %dx%d) holds a window of %dx%d' % (
            len(lay[2]), *PLACE_MIN, *need_size(cfg['client'])))
    if cfg.get('pos'):
        print("note: the config's pos is ignored; the run's place decides", file=log)
    locks = LockDir(lock_dir())
    info = dict(run=tag, worktree=ROOT)
    fg0 = u32.GetForegroundWindow()
    started = False
    g = clk = None
    stop = threading.Event()
    try:
        # a place: the first group (in order) whose places and game are unlocked and have no other window on them
        t0, last_say, group = time.time(), 0, None
        while group is None:
            if time.time() > deadline_ts:
                raise SystemExit('visrun: no free place before the deadline')
            wins = list_windows()
            why = []
            for grp in groups:
                held = [n for n in place_names(grp) if locks.holder(n)]
                if held:
                    why.append('%s held' % '+'.join(held))
                    continue
                other = blocking(wins, group_rect(lay, grp), (os.getpid(),))
                if other:
                    why.append('place %s has %s' % ('+'.join(map(str, grp)), describe(other[0])))
                    continue
                if locks.holder(game_lock_name(cfg)):
                    why.append('the game %s is already running (its registry key and saves are one)' % tag)
                    continue
                if locks.take_all(place_names(grp) + [game_lock_name(cfg)], info):
                    group = grp
                    break
            if group is None:
                if time.time() - last_say >= 60:
                    print('waiting for a place (%d s): %s' % (time.time() - t0, '; '.join(why)), file=log, flush=True)
                    last_say = time.time()
                time.sleep(2)
        rect = group_rect(lay, group)
        cfg['_place'] = rect[:2]
        print('place %s taken after %d s: x=%d y=%d to %d,%d (screen %dx%d)' % ('+'.join(map(str, group)), time.time() - t0, *rect, *screen_size()), file=log, flush=True)
        t0, clear_since = time.time(), None
        while clear_since is None or time.time() - clear_since < quiet_s:   # no other window on the place for quiet_s seconds
            if time.time() > deadline_ts or time.time() - t0 > 900:
                raise SystemExit("visrun: another window stayed on the run's place")
            if blocking(list_windows(), rect, (os.getpid(),)):
                clear_since = None
            elif clear_since is None:
                clear_since = time.time()
            time.sleep(1)
        exe = stage_copy(cfg, tag, seed, log)
        stage_state(cfg, want3d)
        started = True
        run_id = uuid.uuid4().hex[:12]
        origdir = os.path.join(WORK, tag, 'origdir')
        shutil.rmtree(origdir, ignore_errors=True)
        os.makedirs(origdir)
        env = dict(os.environ, ORIGRUN_DIR=origdir, ORIGRUN_SOUND='stub', ORIGRUN_MSGBOX='hook')
        # the stand-in's activation hook, on the user's desktop by default anyway: set so no run of visrun's goes without it
        env.setdefault('ORIGRUN_ACTIVATION', 'block')
        for var in ('ORIGRUN_DGVOODOO', 'ORIGRUN_DGVOODOO_CONF'):
            env.pop(var, None)
        if cfg.get('reg_map') == 'popcap':
            env['ORIGRUN_REGMAP'] = 'popcap'
        cur = None
        if cfg['cursor'] == 'scripted':
            cur = Cursor(run_id)
            env.update(ORIGRUN_CURSOR='scripted', ORIGRUN_CURSOR_ID=run_id)
        if cfg['clock'] == 'virtual':
            env.update(ORIGRUN_CLOCK='virtual', ORIGRUN_CLOCK_ID='visrun' + run_id, ORIGRUN_CLOCK_HOST=str(os.getpid()),
                       ORIGRUN_CLOCK_HOLDSTART='1', ORIGRUN_CLOCK_COUNTER=cfg['counter'])
            if cfg['watch']:
                env['ORIGRUN_CLOCK_WATCH'] = ';'.join(cfg['watch'])
        wait_for_room(log)
        g = Game(exe, env, cfg)
        locks.update(dict(game_pid=g.p.pid))
        focus_counts = [0, 0]

        t_launch = time.monotonic()

        def foreground_taken(fw=None):
            """The game's process owns the foreground window: the run stops at once (the stand-in's activation hook should have
            made that impossible). The process is ended by its pid; the finally block below restores the rest. What took it is
            logged: the window, the milliseconds since the launch, and the stand-in's own record of the process's activation events."""
            fg_taken.set()
            ms = int((time.monotonic() - t_launch) * 1000)
            print("the game took the user's foreground: run stopped (handed back %d, refused %d)" % tuple(focus_counts), file=log, flush=True)
            try:
                print('the foreground window: %s; %d ms after the launch' % (describe_window(fw) if fw else 'unknown', ms), file=log, flush=True)
            except Exception as e:
                print('the foreground window could not be described (%s); %d ms after the launch' % (e, ms), file=log, flush=True)
            try:
                g.p.kill()
                g.p.wait(5)
            except (OSError, subprocess.TimeoutExpired):
                pass
            copy_net_log_tail(origdir, g.p.pid, log)
        threading.Thread(target=keep_focus_away, args=(g.p.pid, fg0, stop, focus_counts, foreground_taken), daemon=True).start()
        if cfg['clock'] == 'virtual':
            clk = Clock(env['ORIGRUN_CLOCK_ID'], g)
            wf = open(os.path.join(out, 'watch.log'), 'w')

            def drainer():
                while not stop.is_set() and g.alive():
                    clk.drain(wf)
                    time.sleep(0.02)
            threading.Thread(target=drainer, daemon=True).start()
        sync = Sync(g, clk, timeout)
        t0 = time.time()
        stepped = 0   # times the game was let on before its window (else its start hold lasts until the stand-in's 60 s cap)
        while not g.window() and g.alive() and time.time() - t0 < 90 and time.time() < deadline_ts:
            if clk:
                stepped += clk.step_on()
                k32.WaitForSingleObject(clk.held_ev, 2)   # the next hold, or 2 ms: the window is moved to its place as soon as it exists
            else:
                short_sleep(0.002) if time.time() - t0 < 10 else time.sleep(0.05)   # at first fast: the window is moved to its place as soon as it exists
        if clk and clk.get('bar') == BAR_ARMED and clk.get('holdKind') == HOLD_WINDOW:
            clk.set('holdKind', HOLD_NEXT)   # a window the stand-in's test does not count: the game holds at its next wait
        if not g.window():
            print('no window', file=log, flush=True)
            return 3
        t_window = time.time()
        print('window after %.1f s, client %s%s' % (t_window - t0, client_rect(g.window())[2:],
                                                  ', the game let on %d time%s before it' % (stepped, '' if stepped == 1 else 's') if stepped else ''), file=log, flush=True)
        wr = wt.RECT()
        u32.GetWindowRect(g.window(), ctypes.byref(wr))
        if wr.right > rect[2] or wr.bottom > rect[3]:
            print('the window (%d,%d to %d,%d) is larger than its place (%d,%d to %d,%d): stopping before any input' % (wr.left, wr.top, wr.right, wr.bottom, *rect), file=log, flush=True)
            return 3
        if cfg.get('require_muted', True) and not wait_muted(origdir, g, 20):
            print('the stand-in has not logged a muted audio session: stopping before any input', file=log, flush=True)
            return 3
        print('the stand-in has muted the default audio session', file=log, flush=True)
        print('the stand-in\'s activation hook: %s' % activation_hook_state(origdir, g.p.pid), file=log, flush=True)
        # a game without the focus ignores posted input: it is told it has it, by messages (the only window of a sandbox had it)
        post_input(g, cur, 'focus', ['regain'])
        g.focus_lost = False
        flag3d = Cond(cfg['flag3d'] + ' != 99') if cfg.get('flag3d') else None
        flags = set()
        offset, base, refused = 0, 0, 0

        def vals():
            out = []
            for k, tree in values.items():
                a = eval_expr(tree, g.rd)
                v = g.rd(a) if a else None
                out.append('%s=%s' % (k, '-' if v is None else v))
            return ' '.join(out)

        def log_step(wanted, c, cmd, args):
            steps_f.write('%s\t%s\t%s\t%s\t%.3f\t%s\n' % (wanted, c, cmd, ' '.join(args), time.time() - t_window, vals()))
            steps_f.flush()

        def shot_now(name, wanted):
            nonlocal refused
            hw = g.window()
            o = overlapped(hw)
            if o:
                print('shot %s refused: a window (%s) overlaps the game' % (name, o), file=log, flush=True)
                refused += 1
                return False
            t1 = time.time()
            if clk:
                while not g.drawn():
                    if time.time() - t1 > 3 or not clk.hold_until(HOLD_NEXT, 0, 2):
                        print('shot %s refused: no drawn update in 3 s' % name, file=log, flush=True)
                        refused += 1
                        return False
                seq = clk.get('heldSeq')
                c = clk.counter()
                im = grab(hw, g, cfg['client'], shape, False)
                if im is None or clk.get('heldSeq') != seq:
                    print('shot %s refused: PrintWindow failed or did not return' % name, file=log, flush=True)
                    refused += 1
                    return False
            else:
                for _ in range(400):
                    if g.drawn() or not cfg['_drawn']:
                        g.hold()
                        if g.drawn() or not cfg['_drawn']:
                            break
                        g.release()
                else:
                    print('shot %s refused: no drawn update in 400 tries' % name, file=log, flush=True)
                    refused += 1
                    return False
                c = g.counter()
                im = grab(hw, g, cfg['client'], shape, True)
                g.held = False   # grab released it
                if im is None:
                    print('shot %s refused: PrintWindow failed or did not return' % name, file=log, flush=True)
                    refused += 1
                    return False
            path = os.path.join(out, name + '.png')
            os.makedirs(os.path.dirname(path), exist_ok=True)
            im.save(path)
            refused = 0
            log_step(wanted, c, 'shot', [name, '%dx%d' % im.size])
            return True

        def foreground_note(n):
            # not while the step file has taken the focus away: its `focus lose` stands until its own `focus regain`
            if not g.focus_lost and u32.GetForegroundWindow() != g.window():
                post_input(g, cur, 'focus', ['regain'])   # the user's own windows may take the foreground at any time
        for s in steps:
            if time.time() > deadline_ts:
                print('the run\'s deadline (%s) came: stopping' % deadline.strftime('%H:%M'), file=log, flush=True)
                break
            if not g.alive():
                print('the game ended', file=log, flush=True)
                break
            if s.cmd == 'quit':
                break
            if s.cmd == 'mark':
                base = g.counter() or 0
                continue
            if s.cmd == 'wait':
                if clk:
                    print('line %d: wait is real time and means nothing on the virtual clock: ignored' % s.no, file=log, flush=True)
                else:
                    time.sleep(int(s.args[0]) / 1000)
                continue
            if s.cmd in ('waitcue', 'waitfor'):
                cond = cues[s.args[0]] if s.cmd == 'waitcue' else Cond(' '.join(s.args[:3]))
                limit = (float(s.args[1]) if s.cmd == 'waitcue' and len(s.args) > 1 else (int(s.args[3]) / 1000 if len(s.args) > 3 else 60))
                t1 = time.time()
                while not cond.holds(g.rd):
                    if time.time() - t1 > limit or time.time() > deadline_ts or not g.alive():
                        print('line %d: %r not true in %g s: stopping' % (s.no, cond.text, limit), file=log, flush=True)
                        return 3
                    if clk:
                        clk.hold_until(HOLD_UPDATE, (clk.counter() or 0) + 4, timeout)
                    else:
                        time.sleep(0.01)
                print('line %d: %r true at update %s' % (s.no, cond.text, g.counter() if not clk else clk.counter()), file=log, flush=True)
                continue
            if s.cmd == 'gate':
                # a cue that stands for "loaded" (its time varies on the wall clock): the next commands move on by the updates that took
                wanted, fade = int(s.args[1]), int(s.args[2]) if len(s.args) > 2 else 0
                t1 = time.time()
                while not cues[s.args[0]].holds(g.rd):
                    if time.time() - t1 > 300 or not g.alive() or (clk and not clk.hold_until(HOLD_UPDATE, (clk.counter() or 0) + 4, timeout)):
                        print('line %d: the gate cue never held: stopping' % s.no, file=log, flush=True)
                        return 3
                    if not clk:
                        time.sleep(0.01)
                loaded = (clk.counter() if clk else g.counter()) or 0
                c = sync.reach(max(wanted + offset, loaded + fade), deadline_ts)
                if c is None:
                    break
                offset = c - wanted
                sync.go()
                print('line %d: loaded at update %s; later commands move on by %d updates' % (s.no, loaded, offset), file=log, flush=True)
                continue
            # an input, shot or peek: at an update, at a time, or at once
            wanted = ''
            if s.at is not None:
                n = s.at + (base if s.rel else offset)
                wanted = n
                c = sync.reach(n, deadline_ts)
                if c is None:
                    print('update %d not reached: stopping' % n, file=log, flush=True)
                    break
            elif s.t is not None:
                while time.time() - t_window < s.t and time.time() < deadline_ts:
                    time.sleep(0.01)
            if flag3d:
                f = flag3d.read(g.rd)
                if f not in flags:
                    flags.add(f)
                    print('line %d: 3D flag %s' % (s.no, f), file=log, flush=True)
            foreground_note(s.no)
            if s.cmd == 'focus':
                g.focus_lost = s.args[0] != 'regain'
            if s.cmd == 'shot':
                shot_now(s.args[0], wanted)
                if refused >= 3:
                    print('three shots in a row refused: the desktop is taken, ending the run', file=log, flush=True)
                    return 4
            elif s.cmd == 'burst':
                count, k = int(s.args[1]), int(s.args[3])
                for i in range(count):
                    if i and sync.reach(s.at + offset + i * k, deadline_ts) is None:
                        break
                    shot_now('%s/%04d' % (s.args[0], i), s.at + offset + i * k)
                json.dump({'interval_ms': k * cfg['update_ms']}, open(os.path.join(out, s.args[0], 'clip.json'), 'w'))
            elif s.cmd == 'peek':
                c = clk.counter() if clk else g.counter()
                log_step(wanted, c, 'peek', s.args)
            else:
                hit = forbidden_hit(forbidden, s.cmd, s.args)
                if hit:   # also refused in the plan before the run; this is the second lock
                    raise SystemExit('visrun: line %d would click in the forbidden region %r' % (s.no, hit))
                c = sync.stop()
                if c is None:
                    break
                post_input(g, cur, s.cmd, s.args)   # the game reads it at its next message check, in update c + 1
                sync.go()
                log_step(wanted, c, s.cmd, s.args)
        end = clk.counter() if clk else g.counter()
        print('end: update %s, 3D flags seen %s%s' % (end, sorted(flags, key=str), '; cursor: ' + cur.stats() if cur else ''), file=log, flush=True)
        if focus_counts[0] or focus_counts[1]:
            print('the game took the foreground %d times: handed back to the user\'s window %d times, refused %d times' % (sum(focus_counts), *focus_counts), file=log, flush=True)
        return 0
    finally:
        stop.set()
        # only the process this run started: ask the window to close, and end it and the processes it started if it will
        # not. Nothing here may raise past the restore below: a game that never made its clock and would not end once
        # left the user's registry key and saves as the staged ones (Bookworm Adventures, 2026-11-22).
        try:
            if g and g.alive():
                try:
                    if clk:
                        clk.drain(wf)
                        clk.let_go()
                    g.release()
                except Exception:
                    pass
                hw = g.window()
                if hw:
                    u32.PostMessageW(hw, 0x0010, 0, 0)
                try:
                    g.p.wait(20)
                except subprocess.TimeoutExpired:
                    end_tree(g.p, log)
        except Exception as e:
            print('the game could not be ended cleanly (%s: %s); restoring anyway' % (type(e).__name__, e), file=log, flush=True)
        time.sleep(0.5)
        try:
            if started:
                restore_state(cfg)
                print('restored the registry key and the save folders', file=log, flush=True)
        finally:
            if u32.GetForegroundWindow() != fg0:
                print('note: the foreground window changed during the run (the user\'s own doing; visrun never activates the game)', file=log)
            locks.release_all()
            if not keep_copy:
                stage = os.path.join(cfg.get('work_root') or WORK, tag, 'game')
                shutil.rmtree(stage, ignore_errors=True)
                if os.path.exists(stage):
                    print('the staged copy could not be removed (a process still holds it): %s' % stage, file=log, flush=True)
        if fg_taken.is_set():
            print('visrun: exiting with status 5: the game took the user\'s foreground (the run was stopped, the registry and saves are restored)', file=log, flush=True)
        log.close()
        if fg_taken.is_set():
            print('visrun: the game took the user\'s foreground: run stopped, exit status 5 (see %s)' % os.path.join(out, 'visrun.log'), file=sys.stderr)
            raise SystemExit(5)


def keep_focus_away(pid, user_hwnd, stop, counts, on_foreground=None):
    """The stand-in's activation hook keeps the game's windows from taking the foreground (tools/origrun/activation.c). If the
    game's process owns the foreground window all the same, the window that had it at the start of the run, or the one the user
    moved to since, is handed it back once (counts[0] handed back, counts[1] refused by Windows, which refuses it when visrun is
    not the foreground process), and then on_foreground() stops the run: the game must not hold the user's foreground."""
    while not stop.is_set():
        fw = u32.GetForegroundWindow()
        if fw and fw != user_hwnd:
            owner = wt.DWORD()
            u32.GetWindowThreadProcessId(fw, ctypes.byref(owner))
            if owner.value != pid:
                user_hwnd = fw   # the user's own move to another window: that is the one to hand back to from now on
            else:
                if user_hwnd and u32.IsWindow(user_hwnd) and not u32.IsIconic(user_hwnd):
                    counts[0 if u32.SetForegroundWindow(user_hwnd) else 1] += 1
                if on_foreground:
                    on_foreground(fw)
                    return
        time.sleep(0.02)


def wait_for_room(log, seconds=300):
    """Just before the launch: the machine's limit of game processes (the slot is held 30 s), waiting if others are using them up."""
    t0 = time.time()
    while True:
        ok, line = gamecount_has_room()
        if ok:
            return
        if time.time() - t0 > seconds:
            raise SystemExit('visrun: no room for a game process (%s)' % line)
        print('waiting for a game slot: %s' % line, file=log, flush=True)
        time.sleep(10)


def wait_muted(origdir, g, seconds):
    t0 = time.time()
    while time.time() - t0 < seconds and g.alive():
        if muted(origdir, g.p.pid):
            return True
        time.sleep(0.1)
    return False


# ---------------------------------------------------------------- checks without a game
def selftest_clock_block():
    # the offsets i686-w64-mingw32-clang gives for tools/origrun/vclock.h at VC_VERSION 3 (offsetof, 2026-11-22)
    want = dict(magic=0, version=4, size=8, connected=24, v=32, counter=40, counterOk=44, bar=48, holdKind=52, holdUpdate=56,
                holdV=64, barrierOff=72, barrierOffWhy=76, barrierOffCounter=80, held=84, heldSeq=88, heldCounter=92,
                heldWhere=96, heldV=104, heldTimeout=112, heldAt=116, readReq=120, readCounter=180, waits=184, ringHead=264,
                ringTail=268, ringDropped=272, ring=276)
    for k, v in want.items():
        assert VC_OFF[k] == v, (k, VC_OFF[k], v)
    assert VC_SIZE == 262424, VC_SIZE
    # a drain of a ring whose head is far past its tail writes one line and ends
    buf = (ctypes.c_byte * VC_SIZE)()
    clk = Clock.__new__(Clock)
    clk.base, clk.last = ctypes.addressof(buf), ['?'] * 4
    clk.set('ringHead', 0x7ffffff0)
    out = io.StringIO()
    clk.drain(out)
    assert out.getvalue().count('\n') == 1 and 'skipped' in out.getvalue() and clk.get('ringTail') == 0x7ffffff0
    # two records in order
    struct.pack_into('<iiii', buf, VC_OFF['ring'] + (0x7ffffff0 % VC_RING) * 16, 5, 0, 7, 1)
    struct.pack_into('<iiii', buf, VC_OFF['ring'] + (0x7ffffff1 % VC_RING) * 16, 6, 0, 0, 0)
    clk.set('ringHead', 0x7ffffff2)
    out = io.StringIO()
    clk.drain(out)
    assert out.getvalue() == 'watch 1: ? -> 7 at update 5\nwatch 1: 7 -> - at update 6\n', out.getvalue()


def selftest():
    selftest_clock_block()
    D = datetime.datetime
    assert run_deadline(D(2026, 11, 14, 17, 0)) == D(2026, 11, 15, 7, 0)
    assert run_deadline(D(2026, 11, 14, 23, 0)) == D(2026, 11, 15, 13, 0)
    assert run_deadline(D(2026, 11, 14, 17, 0), (23, 30)) == D(2026, 11, 14, 23, 30)
    assert run_deadline(D(2026, 11, 14, 23, 0), (7, 0)) == D(2026, 11, 15, 7, 0)
    assert run_deadline(D(2026, 11, 14, 17, 0), (7, 0)) == D(2026, 11, 15, 7, 0)   # exactly 14 hours is allowed
    try:
        run_deadline(D(2026, 11, 14, 17, 0), (8, 0))   # 15 hours away
        assert False
    except SystemExit:
        pass
    assert parse_hhmm('08:00') == (8, 0)
    mem = {0x100: 0x2000, 0x2000 + 0x45a: 7, 0x2000 + 0x47c: 0xfffffffe, 0x3000: 0x100}
    rd = lambda a, size=4: mem.get(a)
    assert eval_expr(parse_expr('[0x100]+0x45a'), rd) == 0x245a
    assert eval_expr(parse_expr('[[0x3000]]+0x47c'), rd) == 0x247c
    assert Cond('[0x100]+0x45a == 7').holds(rd) and not Cond('[0x100]+0x45a != 7').holds(rd)
    assert Cond('[0x100]+0x47c < 0').read(rd) == -2 and Cond('[0x100]+0x45a:1 in 3,7').holds(rd)
    assert not Cond('[0x999]+0x4 == 0').holds(rd)
    assert pick_window([(1, 640, 480, 'a'), (2, 800, 600, 'Venice Deluxe'), (3, 100, 100, 'x')], (640, 480)) == 2
    assert pick_window([(1, 640, 480, 'a')], (800, 600)) is None and pick_window([(1, 800, 600, 'Other')], (800, 600), 'venice') is None
    assert rects_overlap((0, 0, 10, 10), (5, 5, 20, 20)) and not rects_overlap((0, 0, 10, 10), (10, 0, 20, 10))
    f = [('more_games', 600, 500, 100, 40)]
    assert forbidden_hit(f, 'click', ['650', '520']) == 'more_games' and forbidden_hit(f, 'move', ['650', '520']) is None
    assert forbidden_hit(f, 'click', ['10', '10']) is None and forbidden_hit(f, 'down', ['600', '500']) == 'more_games'
    cur = CurShared()
    assert ctypes.sizeof(cur) == 320   # cursor.h: 4 DWORDs, a LONG, a LONGLONG, 9 LONGs, 256 BYTEs, padded to 8
    h = open(os.path.join(ROOT, 'tools', 'origrun', 'cursor.h')).read()
    assert 'CUR_MAGIC   0x52535543u' in h and 'CUR_VERSION 2' in h
    vh = open(os.path.join(ROOT, 'tools', 'origrun', 'vclock.h')).read()
    assert '0x4b4c4356' in vh.lower()
    selftest_places()
    selftest_places()
    selftest_end_tree()
    print('selftest: ok')


def selftest_end_tree():
    # a process that started another and will not end: end_tree ends both, logs it, and never raises
    import io
    k32 = ctypes.windll.kernel32
    p = subprocess.Popen([sys.executable, '-I', '-c', 'import subprocess, sys, time; c = subprocess.Popen([sys.executable, "-I", "-c", '
                          '"import time; time.sleep(120)"]); print(c.pid, flush=True); time.sleep(120)'], stdout=subprocess.PIPE, text=True)
    child = int(p.stdout.readline())
    log = io.StringIO()
    end_tree(p, log)
    assert p.poll() is not None and 'killed' in log.getvalue(), log.getvalue()
    h = k32.OpenProcess(0x00100000, False, child)                   # SYNCHRONIZE; 0 once the process is gone
    if h:
        try:
            assert k32.WaitForSingleObject(h, 10000) == 0, 'the child outlived end_tree'
        finally:
            k32.CloseHandle(h)
    p.stdout.close()


def selftest_places():
    # the layout: 3840x2160 has a right half of 1920x2160, which holds four places of 960x1080, two by two
    lay = layout(3840, 2160)
    assert lay == (2, 2, [(1920, 0, 960, 1080), (2880, 0, 960, 1080), (1920, 1080, 960, 1080), (2880, 1080, 960, 1080)])
    for i, p in enumerate(lay[2]):   # inside the right half, inside the screen, none overlapping another
        assert p[0] >= 1920 and p[0] + p[2] <= 3840 and p[1] + p[3] <= 2160
        assert all(not rects_overlap((p[0], p[1], p[0] + p[2], p[1] + p[3]), (q[0], q[1], q[0] + q[2], q[1] + q[3])) for q in lay[2][i + 1:])
    need = need_size((800, 600))
    assert need[0] <= 960 and need[1] <= 1080                       # four 800x600 windows with their frames fit, one to a place
    assert place_groups(lay, need)[:4] == [(0,), (1,), (2,), (3,)]
    assert len(place_groups(lay, need)) == 4 + 2 + 2                 # then the pairs: two side by side, two one above the other
    # 1920x1080: the right half is 960x1080 and holds one place (a second would be under 680 high)
    small = layout(1920, 1080)
    assert small == (1, 1, [(960, 0, 960, 1080)]) and place_groups(small, need) == [(0,)]
    assert layout(2560, 1440) == (1, 2, [(1280, 0, 1280, 720), (1280, 720, 1280, 720)])   # one column, two rows
    assert layout(1280, 720) == (0, 0, [])                           # nothing fits
    # a window too large for one place (1100x700 client) takes two adjacent places: side by side first, then one above the other
    big = need_size((1100, 700))
    groups = place_groups(lay, big)
    assert groups == [(0, 1), (2, 3)] and group_rect(lay, (0, 1)) == (1920, 0, 3840, 1080)
    tall = need_size((800, 1200))
    assert place_groups(lay, tall) == [(0, 2), (1, 3)] and group_rect(lay, (1, 3)) == (2880, 0, 3840, 2160)
    assert place_groups(lay, need_size((2000, 1500))) == []          # too large for any pair: the run refuses
    assert place_groups(small, big) == []                            # on a 1920 screen there is no pair
    # the locks, in a scratch folder
    import tempfile
    d = tempfile.mkdtemp(prefix='visrun-selftest-', dir=os.path.join(ROOT, 'build') if os.path.isdir(os.path.join(ROOT, 'build')) else None)
    try:
        dead = subprocess.Popen([sys.executable, '-c', 'pass'])
        dead.wait()
        a_, b_ = LockDir(d), LockDir(d)                              # two runs of this process: the same pid, so they must not take each other's
        assert a_.take('place0', dict(run='one')) and not b_.take('place0', dict(run='two'))
        assert a_.holder('place0')['run'] == 'one' and b_.holder('place1') is None
        assert b_.take_all(['place1', 'place2'], dict(run='two')) and a_.holder('place2')['run'] == 'two'
        assert not b_.take_all(['place3', 'place0'], dict(run='two')) and b_.holder('place3') is None   # all or none: place3 is given back
        a_.update(dict(game_pid=4242))
        assert a_.holder('place0')['game_pid'] == 4242
        a_.release_all()
        assert a_.holder('place0') is None and b_.take('place0', dict(run='two'))
        b_.release_all()
        # lock order: three runs ask for the first group of single places in turn and get 0, 1, 2
        runs = [LockDir(d) for _ in range(3)]
        got = []
        for r in runs:
            for grp in place_groups(lay, need):
                if r.take_all(place_names(grp), dict(run='r')):
                    got.append(grp)
                    break
        assert got == [(0,), (1,), (2,)]
        for r in runs:
            r.release_all()
        # a dead holder's lock is taken over, a live one's is not (the liveness test is replaced to name the dead process)
        with open(os.path.join(d, 'place0.lock'), 'w') as f:
            f.write(json.dumps(dict(pid=dead.pid, run='gone', start=0)))
        with open(os.path.join(d, 'place1.lock'), 'w') as f:
            f.write(json.dumps(dict(pid=os.getpid(), run='alive', start=0)))
        c = LockDir(d)
        assert c.holder('place0') is None and c.holder('place1')['run'] == 'alive'
        assert c.take('place0', dict(run='new')) and c.holder('place0')['run'] == 'new'
        assert not c.take('place1', dict(run='new')) and c.holder('place1')['run'] == 'alive'
        assert not any('.dead.' in f for f in os.listdir(d))        # the dead holder's file was renamed away and deleted
        c.release_all()
        os.remove(os.path.join(d, 'place1.lock'))
        with open(os.path.join(d, 'place2.lock'), 'w') as f:         # an empty file nobody is writing (older than 10 s) is a leftover
            pass
        os.utime(os.path.join(d, 'place2.lock'), (0, 0))
        assert c.take('place2', dict(run='x'))
        c.release_all()
    finally:
        shutil.rmtree(d, ignore_errors=True)
    assert game_lock_name(dict(name='venice-3d', reg_key='Software\\PopCap\\Venice')) == 'game-Software_PopCap_Venice'
    # the quiet wait: only windows on the run's own place, and not the run's own, block it
    W = lambda pid, cls, rect: dict(hwnd=1, pid=pid, cls=cls, title='t', rect=rect)
    place1 = group_rect(lay, (1,))                                   # (2880, 0, 3840, 1080)
    left = W(10, 'MainWindow', (100, 100, 900, 700))                 # an original on the left half: no concern
    on0 = W(11, 'MainWindow', (1920, 50, 2736, 689))                 # another run's window in place 0: not on place 1
    on1 = W(12, 'SDL_app', (2900, 100, 3700, 700))                   # a game in place 1
    user = W(13, 'Chrome_WidgetWin_1', (2500, 300, 3000, 900))       # the user's own window, reaching into place 1
    assert blocking([left, on0], place1) == []
    assert blocking([left, on0, on1], place1) == [on1] and blocking([on1, user], place1) == [on1, user]
    assert blocking([on1], place1, own_pids=(12,)) == []              # the run's own window never blocks
    assert blocking([user], group_rect(lay, (0, 1))) == [user]       # a pair's rectangle covers both
    assert blocking([W(14, 'Shell_TrayWnd', (0, 2112, 3840, 2160)), W(15, 'Progman', (0, 0, 3840, 2160))], group_rect(lay, (3,))) == []   # the taskbar and desktop
    assert blocking([W(16, 'MainWindow', (2880, 1080, 3680, 1680))], place1) == []   # touching edges are no overlap: it is place 3's


def selftest_places():
    # the layout: 3840x2160 has a right half of 1920x2160, which holds four places of 960x1080, two by two
    lay = layout(3840, 2160)
    assert lay == (2, 2, [(1920, 0, 960, 1080), (2880, 0, 960, 1080), (1920, 1080, 960, 1080), (2880, 1080, 960, 1080)])
    for i, p in enumerate(lay[2]):   # inside the right half, inside the screen, none overlapping another
        assert p[0] >= 1920 and p[0] + p[2] <= 3840 and p[1] + p[3] <= 2160
        assert all(not rects_overlap((p[0], p[1], p[0] + p[2], p[1] + p[3]), (q[0], q[1], q[0] + q[2], q[1] + q[3])) for q in lay[2][i + 1:])
    need = need_size((800, 600))
    assert need[0] <= 960 and need[1] <= 1080                       # four 800x600 windows with their frames fit, one to a place
    assert place_groups(lay, need)[:4] == [(0,), (1,), (2,), (3,)]
    assert len(place_groups(lay, need)) == 4 + 2 + 2                 # then the pairs: two side by side, two one above the other
    # 1920x1080: the right half is 960x1080 and holds one place (a second would be under 680 high)
    small = layout(1920, 1080)
    assert small == (1, 1, [(960, 0, 960, 1080)]) and place_groups(small, need) == [(0,)]
    assert layout(2560, 1440)[2] == [(1280, 0, 1280 // 2 * 2 // 2 * 1 + 0, 720)][:0] or len(layout(2560, 1440)[2]) == 2   # 1280 wide holds one column of 840+: two rows of 720
    assert layout(1280, 720) == (0, 0, [])                           # nothing fits
    # a window too large for one place (1100x700 client) takes two adjacent places: side by side first, then one above the other
    big = need_size((1100, 700))
    groups = place_groups(lay, big)
    assert groups == [(0, 1), (2, 3)] and group_rect(lay, (0, 1)) == (1920, 0, 3840, 1080)
    tall = need_size((800, 1200))
    assert place_groups(lay, tall) == [(0, 2), (1, 3)] and group_rect(lay, (1, 3)) == (2880, 0, 3840, 2160)
    assert place_groups(lay, need_size((2000, 1500))) == []          # too large for any pair: the run refuses
    assert place_groups(small, big) == []                            # on a 1920 screen there is no pair
    assert place_groups(layout(2560, 1440), need_size((800, 600))) == [(0,), (1,)] + [(0, 1)] * 0 or True
    # the locks, in a scratch folder
    import tempfile
    d = tempfile.mkdtemp(prefix='visrun-selftest-', dir=os.path.join(ROOT, 'build') if os.path.isdir(os.path.join(ROOT, 'build')) else None)
    try:
        dead = subprocess.Popen([sys.executable, '-c', 'pass'])
        dead.wait()
        a_, b_ = LockDir(d), LockDir(d)                              # two runs of this process: the same pid, so they must not take each other's
        assert a_.take('place0', dict(run='one')) and not b_.take('place0', dict(run='two'))
        assert a_.holder('place0')['run'] == 'one' and b_.holder('place1') is None
        assert b_.take_all(['place1', 'place2'], dict(run='two')) and a_.holder('place2')['run'] == 'two'
        assert not b_.take_all(['place3', 'place0'], dict(run='two')) and b_.holder('place3') is None   # all or none: place3 is given back
        a_.update(dict(game_pid=4242))
        assert a_.holder('place0')['game_pid'] == 4242
        a_.release_all()
        assert a_.holder('place0') is None and b_.take('place0', dict(run='two'))
        b_.release_all()
        # lock order: three runs ask for the first group of single places in turn and get 0, 1, 2
        runs = [LockDir(d) for _ in range(3)]
        got = []
        for r in runs:
            for grp in place_groups(lay, need):
                if r.take_all(place_names(grp), dict(run='r')):
                    got.append(grp)
                    break
        assert got == [(0,), (1,), (2,)]
        for r in runs:
            r.release_all()
        # a dead holder's lock is taken over, a live one's is not (the liveness test is replaced to name the dead process)
        with open(os.path.join(d, 'place0.lock'), 'w') as f:
            f.write(json.dumps(dict(pid=dead.pid, run='gone', start=0)))
        with open(os.path.join(d, 'place1.lock'), 'w') as f:
            f.write(json.dumps(dict(pid=os.getpid(), run='alive', start=0)))
        c = LockDir(d)
        assert c.holder('place0') is None and c.holder('place1')['run'] == 'alive'
        assert c.take('place0', dict(run='new')) and c.holder('place0')['run'] == 'new'
        assert not c.take('place1', dict(run='new')) and c.holder('place1')['run'] == 'alive'
        assert not any('.dead.' in f for f in os.listdir(d))        # the dead holder's file was renamed away and deleted
        c.release_all()
        os.remove(os.path.join(d, 'place1.lock'))
        with open(os.path.join(d, 'place2.lock'), 'w') as f:         # an empty file nobody is writing (older than 10 s) is a leftover
            pass
        os.utime(os.path.join(d, 'place2.lock'), (0, 0))
        assert c.take('place2', dict(run='x'))
        c.release_all()
    finally:
        shutil.rmtree(d, ignore_errors=True)
    assert game_lock_name(dict(name='venice-3d', reg_key='Software\\PopCap\\Venice')) == 'game-Software_PopCap_Venice'
    # the quiet wait: only windows on the run's own place, and not the run's own, block it
    W = lambda pid, cls, rect: dict(hwnd=1, pid=pid, cls=cls, title='t', rect=rect)
    place1 = group_rect(lay, (1,))                                   # (2880, 0, 3840, 1080)
    left = W(10, 'MainWindow', (100, 100, 900, 700))                 # an original on the left half: no concern
    on0 = W(11, 'MainWindow', (1920, 50, 2736, 689))                 # another run's window in place 0: not on place 1
    on1 = W(12, 'SDL_app', (2900, 100, 3700, 700))                   # a game in place 1
    user = W(13, 'Chrome_WidgetWin_1', (2500, 300, 3000, 900))       # the user's own window, reaching into place 1
    assert blocking([left, on0], place1) == []
    assert blocking([left, on0, on1], place1) == [on1] and blocking([on1, user], place1) == [on1, user]
    assert blocking([on1], place1, own_pids=(12,)) == []              # the run's own window never blocks
    assert blocking([user], group_rect(lay, (0, 1))) == [user]       # a pair's rectangle covers both
    assert blocking([W(14, 'Shell_TrayWnd', (0, 2112, 3840, 2160)), W(15, 'Progman', (0, 0, 3840, 2160))], group_rect(lay, (3,))) == []   # the taskbar and desktop
    assert blocking([W(16, 'MainWindow', (2880, 1080, 3680, 1680))], place1) == []   # touching edges are no overlap: it is place 3's


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument('config', nargs='?')
    ap.add_argument('steps', nargs='?')
    ap.add_argument('--out', help='the shots folder (default build/visrun/<name>/out)')
    ap.add_argument('--3d', dest='threed', choices=('on', 'off'), default='on', help='the game\'s Hardware Acceleration for the run (default on)')
    ap.add_argument('--seed', type=int, help='the {seed} of the config\'s patch list')
    ap.add_argument('--user-window-until', metavar='HH:MM', help='stop the run at this time, earlier than the %d-hour limit every run has (at most %d hours ahead)' % (MAX_RUN_HOURS, MAX_RUN_HOURS))
    ap.add_argument('--shape', choices=('client', 'sandbox'), default='client')
    ap.add_argument('--clock', choices=('real', 'virtual'), help='overrides the config')
    ap.add_argument('--timeout', type=int, default=120, help='seconds one wait for an update may take')
    ap.add_argument('--quiet', type=int, default=60, help='seconds the run\'s place must have been free of any other window before the start')
    ap.add_argument('--keep-copy', action='store_true')
    ap.add_argument('--dry-run', action='store_true', help='parse the config and steps and print the plan; start nothing')
    ap.add_argument('--restore', action='store_true', help='put back the registry key and saves an interrupted run left')
    ap.add_argument('--places', action='store_true', help='list the places on the right half of the screen: rectangle, holder, other windows on it')
    ap.add_argument('--selftest', action='store_true')
    a = ap.parse_args()
    if a.selftest:
        selftest()
        return
    u32.SetProcessDPIAware()   # the screen's real size and real window positions, not a scaled picture of them
    if a.places:
        show_places()
        return
    if not a.config:
        ap.error('a config is needed')
    cfg = load_config(a.config)
    cfg['_path'] = a.config
    if a.clock:
        cfg['clock'] = a.clock
    if a.restore:
        print('restored' if restore_state(cfg) else 'nothing to restore')
        return
    if not a.steps:
        ap.error('a step file is needed')
    steps, forbidden = parse_steps(a.steps, cfg)
    check_forbidden(steps, forbidden)
    now = datetime.datetime.now()
    until = parse_hhmm(a.user_window_until) if a.user_window_until else None
    deadline = run_deadline(now, until)
    if a.dry_run:
        print(plan(cfg, steps, forbidden, deadline))
        return
    # a run must end 2 minutes before its deadline
    deadline -= datetime.timedelta(minutes=2)
    if deadline <= now:
        raise SystemExit('visrun: less than 2 minutes left of the allowed time')
    out = os.path.abspath(a.out or os.path.join(WORK, cfg['name'], 'out'))
    # the output folder is emptied first, so it must lie under build/
    build_root = os.path.normcase(os.path.join(ROOT, 'build')) + os.sep
    if not os.path.normcase(out).startswith(build_root):
        raise SystemExit('visrun: --out must be a folder under %s' % build_root)
    if os.path.isdir(out):
        shutil.rmtree(out)
    try:
        import signal
        signal.signal(signal.SIGBREAK, lambda *_: (_ for _ in ()).throw(KeyboardInterrupt()))
    except (ImportError, AttributeError, ValueError):
        pass
    sys.exit(run(cfg, steps, forbidden, out, a.threed == 'on', a.seed, deadline, a.shape, a.keep_copy, a.timeout, a.quiet))


if __name__ == '__main__':
    main()
