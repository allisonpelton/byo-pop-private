"""Runs an origrun script with the original in 3D on the user's visible desktop, with the coverage recorder on, for the screen wipe: the
wipe plays only in 3D (Board::RedrawAfterDialog, 0x43a890, closes the board first when the screen is not accelerated), and the original
gets no Direct3D in Windows Sandbox or under rundesk with dgVoodoo2 (GAME_NOTES section 10). Made from Typer Shark's
games/typershark/tools/phase2/visrun3d.py (a run in the manner of Bookworm Adventures' visrun.py) without the shots:

    python visrun3d.py <script.txt> [--3d on|off] [--out <dir>] [--anytime]

ONLY between 22:00 and 08:00 local time (CLAUDE.md, "Overnight, desktop and sound allowed"): the game's window is on the user's desktop.
The script is the original's ("at <update> click|rclick|move|key|text ..." lines, from tools/tour/tour.py: shots, waitfor and the like are
skipped). Only the game's own window is touched: input is posted to it (never the user's cursor or keyboard), the window is kept topmost
and never activated, the game's GetCursorPos reads origrun's scripted point (the stand-in's cursor hook), and the game runs silent (the
stand-in's sound) on the real clock at below-normal priority. The game is a copy of build/origrun/apw/game (made by
games/astropop/tools/origcmp/stage_original.sh apw, then run once with origrun --coverage so that its exe has the recorder's import)
under build/ap-3d/game, without dgVoodoo's files and with a clean users folder. The game's registry key (HKCU\\Software\\PopCap\\AstroPop,
where the stand-in's registry map puts HKLM\\SOFTWARE\\PopCap\\AstroPop) is exported first, set to Is3D 1 and ScreenMode 0 for the run, and
put back afterwards (or deleted, if it was not there). The recorder's hits go to build/ap-3d/coverage.txt (a run that ends early keeps
what it had); the log, with the 3D flag read at each step, to build/ap-3d/run.log.

Each script line "at N <cmd>" waits for the update counter ([[0x5dc220]+0x4a0]) to reach N, holds the game (NtSuspendProcess), posts the
input and lets it go. The real clock gives no exact N; that does not matter here (coverage, not a comparison).
"""
import ctypes
import ctypes.wintypes as wt
import datetime
import mmap
import os
import shutil
import subprocess
import sys
import time
import uuid
import winreg

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.normpath(os.path.join(HERE, "..", "..", "..", ".."))

APP, COUNTER, FLAG3D = 0x5dc220, 0x4a0, (0x320, 0x10)     # the app pointer's global, the update counter, mDDInterface and its mIs3D byte
KEY = r"Software\PopCap\AstroPop"
SOURCE = os.path.join(ROOT, "build", "origrun", "apw", "game")
WORK = os.path.join(ROOT, "build", "ap-3d")
GAME = os.path.join(WORK, "game")
WRAPPER_FILES = ("DDraw.dll", "D3DImm.dll", "dgVoodoo.conf", "origrun-wrapper.txt")

k32 = ctypes.WinDLL("kernel32", use_last_error=True)
u32 = ctypes.WinDLL("user32", use_last_error=True)
ntdll = ctypes.WinDLL("ntdll")
k32.OpenProcess.restype = wt.HANDLE
u32.PostMessageW.argtypes = [wt.HWND, ctypes.c_uint, wt.WPARAM, wt.LPARAM]
u32.SetWindowPos.argtypes = [wt.HWND, wt.HWND, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_uint]


class CurShared(ctypes.Structure):
    """tools/origrun/cursor.h, the block the stand-in's GetCursorPos reads (CUR_MAGIC 'CURS', CUR_VERSION 2)."""
    _fields_ = [("magic", ctypes.c_uint32), ("version", ctypes.c_uint32), ("size", ctypes.c_uint32), ("set", ctypes.c_int32),
                ("pos", ctypes.c_int64), ("posts", ctypes.c_int32), ("installed", ctypes.c_int32), ("reads", ctypes.c_int32),
                ("realReads", ctypes.c_int32), ("keysSet", ctypes.c_int32), ("keyPosts", ctypes.c_int32),
                ("keysInstalled", ctypes.c_int32), ("keyReads", ctypes.c_int32), ("keyRealReads", ctypes.c_int32),
                ("keys", ctypes.c_uint8 * 256)]


class Cursor:
    def __init__(self, run_id):
        self.name = "Local\\origrun-cursor-" + run_id
        self.mm = mmap.mmap(-1, ctypes.sizeof(CurShared), tagname=self.name)
        self.blk = CurShared.from_buffer(self.mm)
        self.blk.magic, self.blk.version, self.blk.size = 0x52535543, 2, ctypes.sizeof(CurShared)

    def point(self, x, y):
        self.blk.pos = (y << 32) | (x & 0xffffffff)
        self.blk.set = 1
        self.blk.posts += 1

    def stats(self):
        return "installed %d, reads %d, real reads %d" % (self.blk.installed, self.blk.reads, self.blk.realReads)


class Game:
    def __init__(self, exe, env):
        si = subprocess.STARTUPINFO()
        si.dwFlags = 1          # STARTF_USESHOWWINDOW: the game's first ShowWindow takes this, so the window never takes the focus
        si.wShowWindow = 4      # SW_SHOWNOACTIVATE
        self.p = subprocess.Popen([exe], cwd=os.path.dirname(exe), env=env, startupinfo=si, creationflags=0x4000)   # BELOW_NORMAL
        self.h = k32.OpenProcess(0x0010 | 0x0400 | 0x0800, False, self.p.pid)   # VM_READ, QUERY_INFO, SUSPEND_RESUME
        self.hwnd = None

    def rd(self, addr, size=4):
        v = ctypes.c_uint32()
        n = ctypes.c_size_t()
        if not addr or not k32.ReadProcessMemory(self.h, ctypes.c_void_p(addr), ctypes.byref(v), size, ctypes.byref(n)):
            return None
        return v.value & (0xff if size == 1 else 0xffffffff)

    def counter(self):
        a = self.rd(APP)
        return self.rd(a + COUNTER) if a else None

    def flag3d(self):
        a = self.rd(APP)
        dd = self.rd(a + FLAG3D[0]) if a else None
        return self.rd(dd + FLAG3D[1], 1) if dd else None

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
            if pid.value == self.p.pid and u32.IsWindowVisible(hw) and r.right == 640 and r.bottom == 480:
                found.append(hw)
            return True
        u32.EnumWindows(cb, 0)
        self.hwnd = found[0] if found else None
        if self.hwnd:
            u32.SetWindowPos(self.hwnd, wt.HWND(-1), 0, 0, 0, 0, 0x0001 | 0x0002 | 0x0010)   # topmost, not activated
        return self.hwnd


def client_origin(hw):
    pt = wt.POINT(0, 0)
    u32.ClientToScreen(hw, ctypes.byref(pt))
    return pt.x, pt.y


def xy(x, y):
    return (y << 16) | (x & 0xffff)


def post_input(g, cur, cmd, args):
    hw = g.window()
    cx, cy = client_origin(hw)
    if cmd in ("move", "click", "rclick", "down", "up"):
        x, y = (int(args[0]), int(args[1])) if len(args) >= 2 else (0, 0)
        if len(args) >= 2:
            cur.point(cx + x, cy + y)         # the point the game's GetCursorPos reads, before the message
        lp = xy(x, y)
        if cmd == "move":
            u32.PostMessageW(hw, 0x0200, 0, lp)
        elif cmd == "click":
            u32.PostMessageW(hw, 0x0200, 0, lp)
            u32.PostMessageW(hw, 0x0201, 1, lp)
            u32.PostMessageW(hw, 0x0202, 0, lp)
        elif cmd == "rclick":
            u32.PostMessageW(hw, 0x0200, 0, lp)
            u32.PostMessageW(hw, 0x0204, 2, lp)
            u32.PostMessageW(hw, 0x0205, 0, lp)
        elif cmd == "down":
            u32.PostMessageW(hw, 0x0200, 0, lp)
            u32.PostMessageW(hw, 0x0201, 1, lp)
        elif cmd == "up":
            u32.PostMessageW(hw, 0x0202, 0, lp)
    elif cmd == "focus":
        if args[0] == "regain":
            u32.PostMessageW(hw, 0x001C, 1, 0)   # WM_ACTIVATEAPP
            u32.PostMessageW(hw, 0x0006, 1, 0)   # WM_ACTIVATE
            u32.PostMessageW(hw, 0x0007, 0, 0)   # WM_SETFOCUS
        else:
            u32.PostMessageW(hw, 0x001C, 0, 0)
            u32.PostMessageW(hw, 0x0006, 0, 0)
            u32.PostMessageW(hw, 0x0008, 0, 0)   # WM_KILLFOCUS
    elif cmd == "key":
        vk = int(args[0])
        sc = u32.MapVirtualKeyW(vk, 0)
        u32.PostMessageW(hw, 0x0100, vk, 1 | (sc << 16))
        u32.PostMessageW(hw, 0x0101, vk, 1 | (sc << 16) | (1 << 30) | (1 << 31))
    elif cmd == "text":
        for ch in " ".join(args):
            u32.PostMessageW(hw, 0x0102, ord(ch), 1)
    else:
        raise SystemExit("visrun3d: cannot do %s" % cmd)


k32.CreateWaitableTimerExW.restype = wt.HANDLE
k32.CreateWaitableTimerExW.argtypes = [ctypes.c_void_p, wt.LPCWSTR, wt.DWORD, wt.DWORD]
k32.SetWaitableTimer.argtypes = [wt.HANDLE, ctypes.POINTER(ctypes.c_int64), wt.LONG, ctypes.c_void_p, ctypes.c_void_p, wt.BOOL]
k32.WaitForSingleObject.argtypes = [wt.HANDLE, wt.DWORD]
_TIMER = []


def short_sleep(seconds):
    """A wait of about half a millisecond that costs no processor time: a high-resolution waitable timer (Windows 10 1803 or later; it
    changes no system-wide timer resolution), where time.sleep gives 1 to 15 ms and a spin would take the game's own core when the run is
    pinned to one (tools/pincore.py)."""
    if not _TIMER:
        _TIMER.append(k32.CreateWaitableTimerExW(None, None, 0x2, 0x1F0003))   # CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS
    if not _TIMER[0] or not k32.SetWaitableTimer(_TIMER[0], ctypes.byref(ctypes.c_int64(-int(seconds * 1e7))), 0, None, None, False):
        time.sleep(seconds)
        return
    k32.WaitForSingleObject(_TIMER[0], 100)


def wait_counter(g, n, log, timeout=180):
    t0 = time.time()
    while time.time() - t0 < timeout:
        c = g.counter()
        if c is not None and c >= n:
            return c
        if g.p.poll() is not None:
            return None
        if c is None or n - c > 8:
            time.sleep(0.004)
        else:
            short_sleep(0.0005)
    print("counter %d not reached in %d s" % (n, timeout), file=log, flush=True)
    return None


def prepare():
    """A fresh copy of the staged original (with the recorder's import) without dgVoodoo's files and with a clean users folder."""
    if os.path.isdir(GAME):
        shutil.rmtree(GAME)
    shutil.copytree(SOURCE, GAME, ignore=shutil.ignore_patterns("users", "logs", "crash.txt", "*.log", *WRAPPER_FILES))
    os.makedirs(os.path.join(GAME, "users"))


def stage_registry(backup, threed):
    if os.path.exists(backup):
        raise SystemExit("visrun3d: %s exists: an earlier run was not restored" % backup)
    os.makedirs(backup)
    had = subprocess.run(["reg", "export", "HKCU\\" + KEY, os.path.join(backup, "astropop.reg"), "/y"], capture_output=True)
    open(os.path.join(backup, "had.txt"), "w").write("1" if had.returncode == 0 else "0")
    subprocess.run(["reg", "delete", "HKCU\\" + KEY, "/f"], capture_output=True)
    with winreg.CreateKey(winreg.HKEY_CURRENT_USER, KEY) as k:
        winreg.SetValueEx(k, "Is3D", 0, winreg.REG_DWORD, 1 if threed else 0)
        winreg.SetValueEx(k, "ScreenMode", 0, winreg.REG_DWORD, 0)


def restore_registry(backup):
    subprocess.run(["reg", "delete", "HKCU\\" + KEY, "/f"], capture_output=True)
    if open(os.path.join(backup, "had.txt")).read() == "1":
        subprocess.run(["reg", "import", os.path.join(backup, "astropop.reg")], check=True, capture_output=True)
    shutil.rmtree(backup)


def main():
    a = sys.argv[1:]
    threed, anytime = True, False
    if "--3d" in a:
        i = a.index("--3d")
        threed = a[i + 1] == "on"
        del a[i:i + 2]
    if "--anytime" in a:
        anytime = True     # for a dry run with no window: the guard below refuses a run on the desktop outside the night window
        a.remove("--anytime")
    now = datetime.datetime.now()
    if not anytime and 8 <= now.hour < 22:
        raise SystemExit("visrun3d: the visible desktop is for 22:00 to 08:00 (CLAUDE.md); it is %s" % now.strftime("%H:%M"))
    script = os.path.abspath(a[0])
    os.makedirs(WORK, exist_ok=True)
    log = open(os.path.join(WORK, "run.log"), "w")
    cov_out = os.path.join(WORK, "coverage.txt")
    if os.path.exists(cov_out):
        os.remove(cov_out)
    backup = os.path.join(WORK, "registry-backup")
    prepare()
    stage_registry(backup, threed)
    run_id = uuid.uuid4().hex[:12]
    cur = Cursor(run_id)
    env = dict(os.environ)
    env.update({"ORIGRUN_DIR": os.path.join(WORK, "logs"), "ORIGRUN_SOUND": "stub", "ORIGRUN_REGMAP": "popcap",
                "ORIGRUN_CURSOR": "scripted", "ORIGRUN_CURSOR_ID": run_id,
                "ORIGRUN_COVERAGE": os.path.join(ROOT, "build", "coverage", "astropop", "functions.txt"),
                "ORIGRUN_COVERAGE_OUT": cov_out})
    os.makedirs(env["ORIGRUN_DIR"], exist_ok=True)
    g = None
    try:
        g = Game(os.path.join(GAME, "AstroPopGame.exe"), env)
        flags = set()
        t0 = time.time()
        while not g.window() and time.time() - t0 < 90 and g.p.poll() is None:
            time.sleep(0.05)
        if not g.window():
            print("no window", file=log, flush=True)
            return
        print("window after %.1f s" % (time.time() - t0), file=log, flush=True)
        post_input(g, cur, "focus", ["regain"])       # the game believes it has the focus
        # the loading thread runs in real time beside the update counter: the script's first click waits for the loading flags
        # ([[0x5dc220]+0x50c] reads 0x10101 when the title is loaded; the scripts' own "waitfor" line)
        t1 = time.time()
        while time.time() - t1 < 120 and g.p.poll() is None:
            a0 = g.rd(APP)
            if a0 and g.rd(a0 + 0x50c) == 0x10101:
                break
            time.sleep(0.05)
        print("loaded after %.1f s (counter %s)" % (time.time() - t0, g.counter()), file=log, flush=True)
        for line in open(script):
            w = line.split()
            if not w or w[0] == "quit":
                if w:
                    break
                continue
            if w[0] != "at" or w[2] in ("peek", "shot"):
                continue
            n, cmd, args = int(w[1]), w[2], w[3:]
            if wait_counter(g, n, log) is None:
                break
            f = g.flag3d()
            if f not in flags:
                flags.add(f)
                print("update %d: 3D flag %s" % (n, f), file=log, flush=True)
            g.hold()
            try:
                post_input(g, cur, cmd, args)
            finally:
                g.release()
        # the wipe runs after the last input: let the game play on
        time.sleep(5)
        print("end: update %s, 3D flags seen %s; cursor: %s" % (g.counter(), sorted(flags, key=str), cur.stats()), file=log, flush=True)
    finally:
        if g and g.p.poll() is None:
            hw = g.window()
            if hw:
                u32.PostMessageW(hw, 0x0010, 0, 0)
            try:
                g.p.wait(20)
            except subprocess.TimeoutExpired:
                g.p.kill()
                g.p.wait(10)
                print("killed", file=log)
        time.sleep(0.5)
        restore_registry(backup)
        print("restored the registry key", file=log, flush=True)


if __name__ == "__main__":
    main()
