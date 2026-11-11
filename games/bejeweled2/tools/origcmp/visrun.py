"""Bejeweled 2's copy of Bookworm Adventures' visrun (games/bookwormadv/tools/pilot/visrun.py): the original in its 3D game
(1024 by 768, Direct3D) on the user's visible desktop, since Windows Sandbox cannot give it a Direct3D device (its display
adapter, "Microsoft Remote Display Adapter", fails CreateDevice: docs/ORIGRUN.md, "Hardware acceleration") and rundesk's
desktop gives it none either. The user allowed it on 2026-11-11 ("Yes, both routes": the sandbox with the host's GPU
first, then the desktop if the original still falls back to 2D there), on these terms: silent, a copy under build/, the
picture taken only from the game's own window (PrintWindow), input posted to that window only (never the cursor or the
keyboard), no link, Buy or More Games button clicked, and the window, the game's registry key and its users folder put
back afterwards.

    python visrun.py <exe of a copy under build/> <origrun script> <shots dir> [--hires 0|1] [--timeout SEC]
    python visrun.py --restore <backup dir>        (puts back what an interrupted run left in <shots dir>/../visrun-backup)

The script is an origrun script (to_origrun.py makes it from a recording): "at N <cmd>" holds the game at the first wait
after its update counter ([0x67ccd8]+0x484) reaches N, posts the input and lets it go on, so the input is read in update N+1,
as origrun's runs read it. Commands: shot, move, click, key, text, focus, mark, peek, profile, menu, counters, glows, gate, quit. "at 330 gate 562" waits for
the title's loaded flag (its loading runs on the wall clock, so the update the Play link shows at differs from run to run) and the
link's fade-in, and moves every later command on by the updates that took past 562; landed.tsv has the updates the commands
really landed at, which the port then replays (landed_to_events.py). A window that opened without the focus is given it by
the messages Windows sends (WM_ACTIVATEAPP, WM_ACTIVATE, WM_SETFOCUS), since a game without it ignores the inputs. The game runs on origrun's virtual
clock (docs/ORIGRUN.md, "The virtual clock"): the copy's dsound.dll stand-in makes its time functions read a clock that moves
only by the game's own waits, so the game runs update for update as in the 2D runs of the sandbox (as fast as it can draw,
the same draws, the same random numbers), and visrun is the clock's host: it opens the stand-in's shared block (vclock.h),
arms its barriers and waits for the holds, as origrun does. A shot steps the held game to its next waits until nothing is
left to draw (mHasPendingDraw, +0x45a, clear; mUpdateAppState, +0x488, 0 or 3), then has the window draw itself with
PrintWindow, which the held game answers; it is refused while another visible window overlaps the game's, which is kept
topmost (and never activated). The client area is saved as it is (1024 by 768 in the 3D game). landed.tsv lists every
command with the update it landed at and the board's values then (score, level, state and the board generator's index, and
the global generator's index, as the port's -trace prints them), and <shots dir>/../<name>.watch.log has origrun's "watch N:
a -> b at update U" lines of four board values, for compare.py.

Silent: the copy carries origrun's sound stand-in (dsound.dll beside the exe), which mutes the process's default audio
session on every render device as it loads, so what the game plays by DirectSound, waveOut or BASS (through the system's
DirectSound) joins a muted session; the run starts the copy with ORIGRUN_DIR, ORIGRUN_SOUND=stub and ORIGRUN_MSGBOX=hook and
stops it if the stand-in's log does not say that the session is muted before the first input.
"""
import ctypes
import ctypes.wintypes as wt
import os
import shutil
import struct
import subprocess
import sys
import threading
import time

from PIL import Image

APP, COUNTER, PENDING, BOARD, GENIDX = 0x67ccd8, 0x484, 0x45a, 0xe68, 0x67d910
TITLE, TITLE_LOADED = 0xe78, 0xa0   # the app's title screen and its "loaded" flag (the Play link shows)
GATE_SECONDS, GATE_FADE = 300, 120  # how long a gate waits for the loading, and the updates the link's fade-in takes after it
KEY = r'Software\Steam\Bejeweled2'
USERS = r'C:\ProgramData\Steam\Bejeweled2\users'

k32 = ctypes.WinDLL('kernel32', use_last_error=True)
u32 = ctypes.WinDLL('user32', use_last_error=True)
g32 = ctypes.WinDLL('gdi32', use_last_error=True)
ntdll = ctypes.WinDLL('ntdll')
k32.OpenProcess.restype = wt.HANDLE
u32.GetDC.restype = wt.HDC
g32.CreateCompatibleDC.restype = wt.HDC
g32.CreateCompatibleBitmap.restype = wt.HBITMAP
g32.SelectObject.restype = wt.HGDIOBJ
g32.CreateCompatibleDC.argtypes = [wt.HDC]
g32.CreateCompatibleBitmap.argtypes = [wt.HDC, ctypes.c_int, ctypes.c_int]
g32.SelectObject.argtypes = [wt.HDC, wt.HGDIOBJ]
g32.GetDIBits.argtypes = [wt.HDC, wt.HBITMAP, ctypes.c_uint, ctypes.c_uint, ctypes.c_void_p, ctypes.c_void_p,
                          ctypes.c_uint]
g32.DeleteObject.argtypes = [wt.HGDIOBJ]
g32.DeleteDC.argtypes = [wt.HDC]
u32.ReleaseDC.argtypes = [wt.HWND, wt.HDC]
u32.PostMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
u32.PrintWindow.argtypes = [wt.HWND, wt.HDC, ctypes.c_uint]
u32.GetWindow.restype = wt.HWND
u32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]


class Game:
    def __init__(self, exe, env):
        si = subprocess.STARTUPINFO()
        si.dwFlags = 1            # STARTF_USESHOWWINDOW
        si.wShowWindow = 4        # SW_SHOWNOACTIVATE: the window opens without taking the focus from whatever the user is doing
        self.p = subprocess.Popen([exe], cwd=os.path.dirname(exe), env=env, startupinfo=si,
                                  creationflags=0x4000)   # BELOW_NORMAL_PRIORITY_CLASS
        self.h = k32.OpenProcess(0x0010 | 0x0400 | 0x0800, False, self.p.pid)   # VM_READ, QUERY_INFO, SUSPEND_RESUME
        self.hwnd = None

    def rd(self, addr, size=4):
        v = ctypes.c_uint32()
        n = ctypes.c_size_t()
        if not addr or not k32.ReadProcessMemory(self.h, ctypes.c_void_p(addr), ctypes.byref(v), size, ctypes.byref(n)):
            return None
        return v.value & (0xff if size == 1 else 0xffffffff)

    def app(self):
        return self.rd(APP)

    def counter(self):
        a = self.app()
        return self.rd(a + COUNTER) if a else None

    def pending(self):
        a = self.app()
        return self.rd(a + PENDING, 1) if a else None

    def board_values(self):
        """The four values the port's -trace prints (score, level, state, generator index of the board) and the global
        generator's index, or '-' where unreadable."""
        a = self.app()
        b = self.rd(a + BOARD) if a else None
        out = []
        for addr in ((b + 0x15e20, b + 0x15e4, b + 0x170c, b + 0x15e0) if b else (0, 0, 0, 0)):
            v = self.rd(addr)
            out.append('-' if v is None else str(v - (1 << 32) if v >= 1 << 31 else v))
        v = self.rd(GENIDX)
        out.append('-' if v is None else str(v))
        return out

    def hold(self):
        ntdll.NtSuspendProcess(self.h)

    def release(self):
        ntdll.NtResumeProcess(self.h)

    def window(self):
        if self.hwnd and u32.IsWindow(self.hwnd):
            return self.hwnd
        found = []

        @ctypes.WINFUNCTYPE(wt.BOOL, wt.HWND, wt.LPARAM)
        def cb(hw, lp):
            pid = wt.DWORD()
            u32.GetWindowThreadProcessId(hw, ctypes.byref(pid))
            r = wt.RECT()
            u32.GetClientRect(hw, ctypes.byref(r))
            if pid.value == self.p.pid and u32.IsWindowVisible(hw) and r.right >= 640 and r.bottom >= 480:
                found.append((r.right * r.bottom, hw))
            return True
        u32.EnumWindows(cb, 0)
        self.hwnd = max(found)[1] if found else None
        if self.hwnd:
            # topmost, not activated: nothing of the user's can come over it while it is captured
            u32.SetWindowPos(self.hwnd, wt.HWND(-1), 0, 0, 0, 0, 0x0001 | 0x0002 | 0x0010)
        return self.hwnd


# origrun's virtual clock, from the host's side (tools/origrun/vclock.h, origrun.c vc_hold_until): the offsets are those of the
# 32-bit build of the stand-in (a long long aligns to 4 in a struct there; compiled and printed by a probe)
VC_MAGIC, VC_SIZE, VC_RING = 0x4b4c4356, 262352, 16384
VC_OFF = dict(magic=0, connected=24, v=32, counter=40, counterOk=44, bar=48, holdKind=52, holdUpdate=56, barrierOff=72,
              barrierOffWhy=76, barrierOffCounter=80, held=84, heldSeq=88, heldCounter=92, heldV=104, ringHead=192,
              ringTail=196, ring=204)
BAR_FREE, BAR_ARMED, BAR_HELD = 0, 1, 2
HOLD_NEXT, HOLD_UPDATE = 0, 1
k32.OpenFileMappingW.restype = wt.HANDLE
k32.OpenFileMappingW.argtypes = [wt.DWORD, wt.BOOL, wt.LPCWSTR]
k32.MapViewOfFile.restype = ctypes.c_void_p
k32.MapViewOfFile.argtypes = [wt.HANDLE, wt.DWORD, wt.DWORD, wt.DWORD, ctypes.c_size_t]
k32.OpenEventW.restype = wt.HANDLE
k32.OpenEventW.argtypes = [wt.DWORD, wt.BOOL, wt.LPCWSTR]
k32.SetEvent.argtypes = [wt.HANDLE]
k32.WaitForSingleObject.argtypes = [wt.HANDLE, wt.DWORD]


class Clock:
    """The stand-in's clock block of one game process: arm a barrier, wait for the hold, let go."""

    def __init__(self, cid, g, timeout=30):
        self.g = g
        name = 'Local\\origrun-vclock-%s-%d' % (cid, g.p.pid)
        t0 = time.time()
        while True:
            h = k32.OpenFileMappingW(0xF001F, False, name)
            if h:
                break
            if g.p.poll() is not None or time.time() - t0 > timeout:
                raise SystemExit('visrun: the game made no virtual clock (%s)' % name)
            time.sleep(0.005)
        self.base = k32.MapViewOfFile(h, 0xF001F, 0, 0, VC_SIZE)
        while self.get('magic', ctypes.c_uint32) != VC_MAGIC:   # the stand-in writes it last
            if time.time() - t0 > timeout:
                raise SystemExit('visrun: the clock block never got its magic')
            time.sleep(0.002)
        self.held_ev = k32.OpenEventW(0x1F0003, False, name + '-held')
        self.rel_ev = k32.OpenEventW(0x1F0003, False, name + '-release')
        self.set('connected', 1)
        self.last = ['?'] * 4

    def get(self, key, t=ctypes.c_int32):
        return t.from_address(self.base + VC_OFF[key]).value

    def set(self, key, v):
        ctypes.c_int32.from_address(self.base + VC_OFF[key]).value = v

    def hold_until(self, kind, upd, timeout):
        """True: held (at once, when a held game already meets it); False: the game ended or the wait ran out; None: the game's
        holds are off."""
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
            if self.g.p.poll() is not None:
                return False
            if time.time() - t0 >= timeout:
                break
            k32.WaitForSingleObject(self.held_ev, 5)
        if self.get('bar') == BAR_ARMED:   # take the barrier back (the game has not claimed it)
            self.set('bar', BAR_FREE)
        return False

    def counter(self):
        return self.get('heldCounter')

    def let_go(self):
        self.set('bar', BAR_FREE)
        k32.SetEvent(self.rel_ev)

    def drain(self, f):
        """The --watch changes the game has queued, as origrun's "watch N: a -> b at update U" lines."""
        head, tail = self.get('ringHead'), self.get('ringTail')
        while tail != head:
            u, idx, val, ok = struct.unpack('<iiii', ctypes.string_at(self.base + VC_OFF['ring'] + (tail % VC_RING) * 16, 16))
            new = str(val) if ok else '-'
            if 0 <= idx < 4:
                f.write('watch %d: %s -> %s at update %d\n' % (idx + 1, self.last[idx], new, u))
                self.last[idx] = new
            tail += 1
        self.set('ringTail', tail)
        f.flush()


def client_rect(hw):
    r = wt.RECT()
    u32.GetClientRect(hw, ctypes.byref(r))
    pt = wt.POINT(0, 0)
    u32.ClientToScreen(hw, ctypes.byref(pt))
    return pt.x, pt.y, r.right, r.bottom


def overlapped(hw):
    """A visible window above the game's in the z-order that covers part of its client area, or None."""
    x, y, w, h = client_rect(hw)
    o = u32.GetWindow(hw, 3)   # GW_HWNDPREV
    while o:
        if u32.IsWindowVisible(o) and not u32.IsIconic(o):
            r = wt.RECT()
            u32.GetWindowRect(o, ctypes.byref(r))
            if r.left < x + w and r.right > x and r.top < y + h and r.bottom > y and r.right > r.left and r.bottom > r.top:
                buf = ctypes.create_unicode_buffer(256)
                u32.GetClassNameW(o, buf, 256)
                return buf.value
        o = u32.GetWindow(o, 3)
    return None


def grab(hw, g, release=True):
    """The game's window as the window itself renders it (PrintWindow, PW_RENDERFULLCONTENT), never the screen. PrintWindow
    needs the game's thread to answer, so, as origrun does, it is called from a helper thread and the game released just
    after: the picture shows the held update. Returns the client area, or None if PrintWindow fails or does not return
    within 3 s."""
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
    if release:
        t0 = time.perf_counter()
        while time.perf_counter() - t0 < 0.0005:
            pass
        g.release()
    t.join(3 if release else 10)
    if t.is_alive() or not done or not done[0]:
        return None   # a PrintWindow that is late is left to finish; its DC is not freed
    buf = ctypes.create_string_buffer(w * h * 4)

    class BMI(ctypes.Structure):
        _fields_ = [('biSize', wt.DWORD), ('biWidth', wt.LONG), ('biHeight', wt.LONG), ('biPlanes', wt.WORD),
                    ('biBitCount', wt.WORD), ('biCompression', wt.DWORD), ('biSizeImage', wt.DWORD),
                    ('biXPelsPerMeter', wt.LONG), ('biYPelsPerMeter', wt.LONG), ('biClrUsed', wt.DWORD),
                    ('biClrImportant', wt.DWORD), ('pad', wt.DWORD * 4)]
    bmi = BMI()
    bmi.biSize = 40
    bmi.biWidth, bmi.biHeight, bmi.biPlanes, bmi.biBitCount = w, -h, 1, 32
    g32.GetDIBits(mdc, bmp, 0, h, buf, ctypes.byref(bmi), 0)
    g32.SelectObject(mdc, old)
    g32.DeleteObject(bmp)
    g32.DeleteDC(mdc)
    ox, oy = cx - r.left, cy - r.top
    return Image.frombuffer('RGBA', (w, h), buf, 'raw', 'BGRA', 0, 1).convert('RGB').crop((ox, oy, ox + cw, oy + ch))


def xy(x, y):
    return (y << 16) | (x & 0xffff)


def post_input(g, cmd, args):
    hw = g.window()
    if cmd == 'move':
        u32.PostMessageW(hw, 0x0200, 0, xy(int(args[0]), int(args[1])))
    elif cmd == 'click':
        lp = xy(int(args[0]), int(args[1]))
        u32.PostMessageW(hw, 0x0200, 0, lp)
        u32.PostMessageW(hw, 0x0201, 1, lp)
        u32.PostMessageW(hw, 0x0202, 0, lp)
    elif cmd == 'focus':
        if args[0] == 'regain':
            u32.PostMessageW(hw, 0x001C, 1, 0)
            u32.PostMessageW(hw, 0x0006, 1, 0)
            u32.PostMessageW(hw, 0x0007, 0, 0)
        else:
            u32.PostMessageW(hw, 0x001C, 0, 0)
            u32.PostMessageW(hw, 0x0006, 0, 0)
            u32.PostMessageW(hw, 0x0008, 0, 0)
    elif cmd == 'key':
        vk = int(args[0])
        sc = u32.MapVirtualKeyW(vk, 0)
        u32.PostMessageW(hw, 0x0100, vk, 1 | (sc << 16))
        u32.PostMessageW(hw, 0x0101, vk, 1 | (sc << 16) | (1 << 30) | (1 << 31))
    elif cmd == 'text':
        for ch in ' '.join(args):
            u32.PostMessageW(hw, 0x0102, ord(ch), 1)
    else:
        raise SystemExit('visrun: cannot do %s' % cmd)


def drawn_now(g):
    """The game shows the update its counter holds: nothing waits to be drawn and no update runs (origrun's drawn_now)."""
    a = g.app()
    if not a:
        return False
    st = g.rd(a + 0x488)
    return g.pending() == 0 and st in (0, 3)


def shot(g, clk, out, name, landed, log):
    hw = g.window()
    o = overlapped(hw)
    if o:
        print('shot %s refused: a window (%s) overlaps the game' % (name, o), file=log, flush=True)
        return
    t0 = time.time()
    while not drawn_now(g):
        if time.time() - t0 > 3 or not clk.hold_until(HOLD_NEXT, 0, 2):
            print('shot %s refused: the game did not show a drawn update in 3 s' % name, file=log, flush=True)
            return
    seq = clk.get('heldSeq')
    c = clk.counter()
    vals = g.board_values()
    im = grab(hw, g, release=False)   # the game stays held at its wait, which answers the sent messages of PrintWindow
    if im is None or clk.get('heldSeq') != seq:
        print('shot %s refused: PrintWindow failed or did not return' % name, file=log, flush=True)
        return
    path = os.path.join(out, name + '.png')
    os.makedirs(os.path.dirname(path), exist_ok=True)
    im.save(path)
    landed.write('shot\t%s\t%s\t%s\t%dx%d\n' % (name, c, ' '.join(vals), im.size[0], im.size[1]))
    landed.flush()


def reg(*a):
    return subprocess.run(['reg'] + list(a), capture_output=True, text=True)


def stage(backup, hires):
    if os.path.exists(backup):
        raise SystemExit('visrun: %s exists: an earlier run was not restored (visrun.py --restore %s)' % (backup, backup))
    os.makedirs(backup)
    # the game's key and its users folder, as the user's own play left them
    had_key = reg('export', 'HKCU\\' + KEY, os.path.join(backup, 'bej2.reg'), '/y').returncode == 0
    open(os.path.join(backup, 'had_key.txt'), 'w').write('1' if had_key else '0')
    if os.path.isdir(USERS):
        shutil.copytree(USERS, os.path.join(backup, 'users'))
        shutil.rmtree(USERS)
    # a start like run_original.sh's: no player, no cached Direct3D test, windowed, and the game's 1024 by 768 (3D) setting
    reg('delete', 'HKCU\\' + KEY, '/f')
    for name, val in (('ScreenMode', 0), ('HighResolution', 1 if hires else 0)):
        reg('add', 'HKCU\\' + KEY, '/v', name, '/t', 'REG_DWORD', '/d', str(val), '/f')


def restore(backup):
    reg('delete', 'HKCU\\' + KEY, '/f')
    if open(os.path.join(backup, 'had_key.txt')).read() == '1':
        r = reg('import', os.path.join(backup, 'bej2.reg'))
        if r.returncode != 0:
            raise SystemExit('visrun: reg import failed: %s' % r.stderr)
    if os.path.isdir(USERS):
        shutil.rmtree(USERS)
    if os.path.isdir(os.path.join(backup, 'users')):
        shutil.copytree(os.path.join(backup, 'users'), USERS)
    shutil.rmtree(backup)


def muted(origdir, pid):
    """The stand-in's log says that the process's default audio session is muted (a line after its 'attached pid=<pid>')."""
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


def main():
    a = sys.argv[1:]
    if a and a[0] == '--restore':
        restore(a[1])
        print('restored')
        return
    hires, timeout, w2 = True, 90, False
    if '--w2' in a:   # the global generator's index in the place of the level, as run_original.sh's W2=0x67d910 (the port's watch 5)
        w2 = True
        a.remove('--w2')
    if '--hires' in a:
        i = a.index('--hires')
        hires = a[i + 1] == '1'
        del a[i:i + 2]
    if '--timeout' in a:
        i = a.index('--timeout')
        timeout = int(a[i + 1])
        del a[i:i + 2]
    exe, script, out = a
    exe = os.path.abspath(exe)
    out = os.path.abspath(out)
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)
    base = os.path.dirname(out)
    tag = os.path.basename(out)
    backup = os.path.join(base, 'visrun-backup')
    origdir = os.path.join(base, tag + '.origdir')
    shutil.rmtree(origdir, ignore_errors=True)
    os.makedirs(origdir)
    log = open(os.path.join(base, tag + '.visrun.log'), 'w')
    landed = open(os.path.join(out, 'landed.tsv'), 'w')
    try:
        stage(backup, hires)
    except BaseException:
        if os.path.exists(os.path.join(backup, 'had_key.txt')):
            restore(backup)
        raise
    g = None
    stop = threading.Event()
    try:
        B = '[[0x67ccd8]+0xe68]'
        env = dict(os.environ, ORIGRUN_DIR=origdir, ORIGRUN_SOUND='stub', ORIGRUN_MSGBOX='hook', ORIGRUN_CLOCK='virtual',
                   ORIGRUN_CLOCK_ID='visrun%d' % os.getpid(), ORIGRUN_CLOCK_HOST=str(os.getpid()), ORIGRUN_CLOCK_HOLDSTART='1',
                   ORIGRUN_CLOCK_COUNTER='[0x67ccd8]+0x484',
                   ORIGRUN_CLOCK_WATCH=';'.join(['%s+0x15e20' % B, '0x67d910' if w2 else '%s+0x15e4' % B, '%s+0x170c' % B, '%s+0x15e0' % B]))
        g = Game(exe, env)
        clk = Clock(env['ORIGRUN_CLOCK_ID'], g)
        wf = open(os.path.join(base, tag + '.watch.log'), 'w')

        def drainer():
            while not stop.is_set() and g.p.poll() is None:
                clk.drain(wf)
                time.sleep(0.02)
        threading.Thread(target=drainer, daemon=True).start()
        flags = set()
        offset = 0
        gated = False
        for line in open(script):
            w = line.split()
            if not w or w[0].startswith('#'):
                continue
            if w[0] == 'quit':
                break
            if w[0] != 'at':
                print('skipped: %s' % line.strip(), file=log)
                continue
            n, cmd, args = int(w[1]), w[2], w[3:]
            if cmd == 'mark':
                continue
            n += offset
            r = clk.hold_until(HOLD_UPDATE, n, timeout)
            if not r:
                print('update %d not reached (%s): stopping' % (n, 'the holds went off' if r is None else 'the game ended or did not get there in %d s' % timeout),
                      file=log, flush=True)
                break
            c = clk.counter()
            if not gated:
                gated = True
                if not muted(origdir, g.p.pid):
                    print('the stand-in has not logged a muted audio session: stopping the game before any input', file=log,
                          flush=True)
                    break
                print('update %d: the stand-in has muted the default audio session; window %s' % (
                    c, client_rect(g.window())[2:]), file=log, flush=True)
            fg = u32.GetForegroundWindow() == g.window()
            if not fg:
                # the window opens without being activated, and a game without the focus ignores the inputs posted to it (the title's
                # Play link among them): the messages Windows sends to a window that gets the focus go to it, as a 'focus regain' does.
                # Checked before every command, since the user's own windows take the foreground again at any time
                post_input(g, 'focus', ['regain'])
                print('update %d: the game is not the foreground window (the user has it); the focus is given to it by messages' % c,
                      file=log, flush=True)
            if cmd == 'gate':
                # the title's loading runs on the wall clock while the updates run on the virtual one, so the update at which the
                # Play link shows differs from run to run: the commands after the gate (the first input among them) wait for the
                # title's loaded flag (TitleScreen +0xa0, the Play link shows) and the fade-in after it: "at 330 gate 562" looks from
                # update 330 on and moves the commands after it on by as many updates as the next one, wanted at 562, has to wait
                # (the port loads at the machine's speed and replays the landed updates, so it is ready by then)
                t0 = time.time()
                ok = False
                while time.time() - t0 < GATE_SECONDS:
                    a = g.app()
                    ts = g.rd(a + TITLE) if a else None
                    if ts and g.rd(ts + TITLE_LOADED, 1):
                        ok = True
                        break
                    if not clk.hold_until(HOLD_UPDATE, clk.counter() + 4, timeout):
                        break
                if not ok:
                    print('update %s: the title was not loaded (%d s at most): stopping' % (clk.counter(), GATE_SECONDS), file=log,
                          flush=True)
                    break
                loaded = clk.counter()
                wanted = int(args[0])   # the update the next command is wanted at, when the title was loaded in time
                if not clk.hold_until(HOLD_UPDATE, max(wanted + offset, loaded + GATE_FADE), timeout):
                    break
                c = clk.counter()
                offset = c - wanted
                print('update %s: the title was loaded at update %s; the commands after the gate move on by %d updates' % (
                    c, loaded, offset), file=log, flush=True)
                continue
            if cmd == 'shot':
                shot(g, clk, out, args[0], landed, log)
            elif cmd == 'profile':   # the counters that open the secret modes (Profile +0x7c, +0x80, +0x84, +0x8c), in the log
                pf = g.rd(g.app() + 0xe74)
                print('update %s: the counters of the profile: %s' % (c, ' '.join(str(g.rd(pf + o)) for o in (0x7c, 0x80, 0x84, 0x8c))),
                      file=log, flush=True)
            elif cmd == 'menu':   # the main menu, the fancy backdrop and the board's portal, in the log
                ap = g.app()
                mm, fb, bd = g.rd(ap + 0xe80), g.rd(ap + 0xe7c), g.rd(ap + 0xe68)
                ov = g.rd(bd + 0x98) if bd else None
                prog = g.rd(ov + 0xcc) if ov else None
                print('update %s: menu %s state %s backdrop %s board %s portal flag %s progress %s delay %s' % (
                    c, hex(mm or 0), g.rd(mm + 0x15c) if mm else '-', hex(fb or 0), hex(bd or 0), g.rd(ov + 0xc9, 1) if ov else '-',
                    struct.unpack('<f', struct.pack('<I', prog))[0] if prog is not None else '-', g.rd(ov + 0xd0) if ov else '-'),
                      file=log, flush=True)
            elif cmd == 'counters':   # the update counters of the app, the board and its overlay (Widget::mUpdateCnt, +0x28), in the log
                ap = g.app()
                bd = g.rd(ap + 0xe68)
                ov = g.rd(bd + 0x98) if bd else None
                print('update %s: counters app %s board %s overlay %s' % (c, g.rd(ap + 0x484), g.rd(bd + 0x28) if bd else '-',
                                                                         g.rd(ov + 0x28) if ov else '-'), file=log, flush=True)
            elif cmd == 'glows':   # the mouse position the board has and the gems whose hover glow is on (+0x8c: the flag, +0x90: its level, +0x98: its step)
                ap = g.app()
                bd = g.rd(ap + 0xe68)
                items = []
                for row in range(8):
                    for col in range(8):
                        pc = g.rd(bd + 0x15ec + (row * 8 + col) * 4) if bd else 0
                        if pc and g.rd(pc + 0x8c, 1):
                            level = struct.unpack('<d', struct.pack('<II', g.rd(pc + 0x90), g.rd(pc + 0x94)))[0]
                            step = struct.unpack('<f', struct.pack('<I', g.rd(pc + 0x98)))[0]
                            items.append('(%d,%d: %.5f %.5f)' % (row, col, level, step))
                sg = lambda v: v - (1 << 32) if v >= (1 << 31) else v
                print('update %s: glows mouse %d,%d state %d %s' % (c, sg(g.rd(bd + 0x15f28)), sg(g.rd(bd + 0x15f2c)), g.rd(bd + 0x170c),
                                                                    ' '.join(items)), file=log, flush=True)
            elif cmd == 'peek':   # the board's values at this update, for a run that looks where a difference begins
                landed.write('peek\t%s\t%s\n' % (c, ' '.join(g.board_values())))
                landed.flush()
            else:
                post_input(g, cmd, args)   # the game reads it at its next message check, in update c + 1
                landed.write('%s\t%s\t%s\t%s\n' % (cmd, ' '.join(args), c, 'wanted %d' % n))
                landed.flush()
        print('end: update %s' % clk.counter(), file=log, flush=True)
    finally:
        stop.set()
        if g and g.p.poll() is None:
            try:
                clk.drain(wf)
                clk.let_go()
            except NameError:
                pass
            hw = g.window()
            if hw:
                u32.PostMessageW(hw, 0x0010, 0, 0)
            try:
                g.p.wait(20)
            except subprocess.TimeoutExpired:
                g.p.kill()
                g.p.wait(10)
                print('killed', file=log)
        restore(backup)
        print('restored the registry key and the users folder', file=log, flush=True)


if __name__ == '__main__':
    main()
